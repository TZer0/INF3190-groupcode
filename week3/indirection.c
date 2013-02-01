#include<stdlib.h>
#include<stdio.h>

int main() {
	// This is abusing pointers - but it works!
	char ***a;
	char **b;
	char c = 'c';
	a = &c;
	*b = a;
	printf("%c\n", **b);
}
