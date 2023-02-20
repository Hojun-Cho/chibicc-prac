#include "chibicc.h"

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "expected argc 2, actual %d\n", argc);
		return 1;
	}

	Token *tok = tokenize(argv[1]);
	Node *node = expr(&tok, tok);

	printf("	.global main\n");
	printf("main:\n");

	gen_expr(node);
	printf("	ret\n");

	return 0;
}
