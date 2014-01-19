#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#define REPEAT 3
#define MASK 15
#define NEGFILT 255

char error = 0;

void secureWrite(FILE *fd, char c) {
	if (fwrite(&c, 1, 1, fd) <= 0) {
		error = 1;
		printf("Error writing to file.");
	}
#ifdef DEBUG
	printf("write: %c %d\n", c, c);
#endif
}

void decode (unsigned char *in, size_t size, FILE *outFd) {
	unsigned char doneRead = 0, inChar, outChar;
	int16_t buffer = 0;
	int offset = 0;
	int i = 0;
	int j;
	while (1 && !error) {
		// Fill the buffer if we have less than 8 bit left.
		while (offset <= 8 && i < size) {
			buffer |= (in[i] << (8-offset));
			offset += 8;
			i++;
		}

#ifdef DEBUG
		printf("offset: %d\n", offset);
		printf("buffer: ");
		for (j = 0; j < 16; j++) {
			printf("%d", buffer>>(15-j) & 1);
			if ((j+1)%4 == 0) {
				printf(" ");
			}
		}
		printf("\n");
#endif

		// When shifting to the left, we remove the 2 or 4 most significant bits.
		if (((buffer>>14) & REPEAT) == REPEAT) {
			// Repeat the last character.
			buffer <<= 2;
			offset -= 2;
		} else if (offset == 2 || (buffer>>12)&MASK == 0) {
			break;
		} else {
			// New character (found in the four most significant bits)
			outChar = (buffer>>12)&MASK+'a'-1;
			buffer <<=4;
			offset -=4;
		}
		// Write the in[i] character.
		secureWrite(outFd, outChar);

		if (offset <= 0) {
			break;
		}
	}
}

void encode (unsigned char *in, size_t size, FILE *outFd) {
	char prev = 255, filled = 0, outChar = 0;
	int i;
	for (i = 0; i < size; i++) {
		if (error) {
			break;
		}
		// Ignore \0 and \n, exit on anything outside a-j.
		if (in[i] == '\0' || in[i] =='\n') {
			continue;
		} else if ((in[i] < 'a' || 'j' < in[i])  ) {
			printf("Invalid character found. Aborting.\n");
			return;
		}

		if (in[i] == prev) {
			// The in[i] character matches the previous
			outChar |= REPEAT<<(6-filled);
			filled += 2;
		} else {
			// New character
			prev = in[i];
			outChar |= (in[i] - 'a' + 1)<<(4-filled);
			filled +=4;
		}
#ifdef DEBUG
		printf("read: %c\n", in[i]);
#endif

		if (8 <= filled) {

#ifdef DEBUG
			int j;
			printf("wrote: ");
			for (j = 0; j < 8; j++) {
				printf("%d", outChar>>(7-j) & 1);
			}
			printf("\n");
#endif

			filled %=8;
			secureWrite(outFd, outChar);
			outChar=0;
			if (filled == 2) {
				// Write remainder of previous character.
				outChar |= (in[i] - 'a' + 1)<<6;
			}
		}
	}
	if (filled != 0) {
		secureWrite(outFd, outChar);
	}
}

int main(int argc, char **argv) {
	// Check if we have the required number of arguments.
	if (argc != 4) {
		printf("Usage: %s d/e in out\n", argv[0]);
		return -1;
	}
	
	// Open files.
	FILE *inFd, *outFd;
	inFd = fopen(argv[2], "r");
	if (inFd < 0) {
		printf("Error, could not open in-file %s\n", argv[2]);
		return -1;
	}
	fseek(inFd, 0, SEEK_END);
	size_t size = ftell(inFd);
	rewind(inFd);
	unsigned char *data = malloc(size);
	fread(data, 1, size, inFd);
	outFd = fopen(argv[3], "w");
	if (outFd < 0) {
		close(inFd);
		printf("Error, could not open out-file %s\n", argv[3]);
		return -1;
	}
	
	// Check if we're encoding or decoding.
	if (strcmp(argv[1], "d") == 0) {
		decode(data, size, outFd);
	} else if (strcmp(argv[1], "e") == 0) {
		encode(data, size, outFd);
	} else {
		printf("Error, please provide e or d as 2nd argument\n");
	}

	// Close files.
	fclose(inFd);
	fclose(outFd);
	free(data);
 }
