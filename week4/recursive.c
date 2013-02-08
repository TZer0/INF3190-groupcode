#include<stdio.h>
#include<stdlib.h>

// Compile with -O3 for infinite loop!
void rec(int i) {
	fprintf(stderr, "%d\n", i);
	rec(i+1);
}

int main() {
	rec(0);

}
