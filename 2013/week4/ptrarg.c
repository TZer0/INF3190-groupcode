#include<stdio.h>

void doubleValues(int *v, size_t s) {
	int i;
	for (i = 0; i < s; i++) {
		v[i] *= 2;
	}
}

int main() {
	int v[10], i;
	for (i = 0; i < 10; i++) {
		v[i] = i;
	}
	doubleValues(v, 10);
	for (i = 0; i < 10; i++) {
		printf("%d\n", v[i]);
	}
}
