#include<stdio.h>

int main() {
	int v[10], i;
	int *ptr = v;
	for (i = 0; i < 10; i++) {
		// recommended: v[i] = i;
		*ptr = i;
		ptr++;
		printf("%d %ld\n", v[i], ptr);
	}
}
