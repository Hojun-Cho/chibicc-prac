#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "expected argc 2, actual %d\n", argc);
		return 1;
	}

	char *p = argv[1];
	printf("	.global main\n");
	printf("main:\n");

	if (*p == '-')
	{
		p++;
		printf("	mov $-%ld, %%rax\n", strtol(p, &p, 10));
	}
	else if (isdigit(*p))
		printf("	mov $%ld, %%rax\n", strtol(p, &p, 10));
	else
	{
		fprintf(stderr, "unexpected char: '%c'\n", *p);
		return 1;
	}

	while (*p) {
			if (*p == '+') {
				p++;
				printf("	add $%ld, %%rax\n", strtol(p, &p, 10));
				continue;
			}

		if (*p == '-') {
			p++;
			printf("	sub $%ld, %%rax\n", strtol(p, &p, 10));
			continue;
		}

		fprintf(stderr, "unexpected char: '%c'\n", *p);
		return 1;
	}

	printf("	ret\n");
	return 0;
}
