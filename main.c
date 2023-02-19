#include <stdio.h>
#include <stdlib.h>

void println(char *str) {
	printf("%s\n", str);
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "expected argc 2, actual %d\n", argc);
		return 1;
	}

	printf("	.global main\n");
	printf("main:\n");
	printf("	mov $%d, %%rax\n", atoi(argv[1]));
	printf("	ret\n");
	return 0;
}
