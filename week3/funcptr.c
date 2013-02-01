#include<stdio.h>
#include<stdlib.h>

int lol(int b) {
	printf("asd %d\n", b);
	return -1;
}

int main(int args, char **argv) {
	int (*a)(int) = &lol;
	a(5);
}
