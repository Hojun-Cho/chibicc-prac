#include "chibicc.h"

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "expected argc 2, actual %d\n", argc);
		return 1;
	}

	Token *tok = tokenize(argv[1]);
	Node *node = parse(tok);
	code_gen(node);

	return 0;
}
