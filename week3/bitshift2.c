#include<stdio.h>
#include<stdlib.h>

int main() {
	int32_t b = 36;
	int i, c = 0;
	for (i = 0; i < 31; i++) {
		c += (b & (1<<i)) != 0;
	}
	printf("Result: %d\n", c);

	while (b) {
		if (b&1) {
			if (b == 1) {
				printf("Yes.\n");
			} else {
				printf("No.\n");
			}
			break;
		}
		b >>= 1;
	}
}
