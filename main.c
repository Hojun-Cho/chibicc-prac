#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	TK_PUNCT, // Punctuators
	TK_NUM, // Numberic literals
	TK_EOF,
} Tokenkind;

typedef struct Token Token;
struct Token{
	Tokenkind kind;
	Token *next;
	int val;
	char *loc;
	int len;
};

static void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

static bool equal(Token *tok, char *op) {
	return strncmp(tok -> loc, op, tok -> len) == 0
		&& op[tok -> len] == '\0';
}

static Token *skip(Token *tok, char *s) {
	if (equal(tok, s) == false)
		error("expected '%s'", s);
	return tok -> next;
}

static int get_number(Token *tok) {
	if (tok -> kind != TK_NUM)
		error("expected a number");
	return tok -> val;
}

static Token *new_token(Tokenkind kind, char *start, char *end) {
	Token *tok = calloc(1, sizeof(Token));
	tok -> kind = kind;
	tok -> loc = start;
	tok -> len = end - start;
	return tok;
}

static Token *tokenize(char *p) {
	Token head = {};
	Token *cur = &head;

	while (*p) {
		if (isspace(*p)) {
			p++;
			continue;
		}

		if (isdigit(*p)) {
			cur = cur -> next = new_token(TK_NUM, p, p);
			char *start = p;
			cur -> val = strtoul(p, &p, 10);
			cur -> len = p - start;
			continue;
		}

		if (*p == '+' || *p == '-') {
			cur = cur -> next = new_token(TK_PUNCT, p, p + 1);
			p++;
			continue;
		}
		error("invalid token");
	}

	cur = cur -> next = new_token(TK_EOF, p, p);
	return head.next;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "expected argc 2, actual %d\n", argc);
		return 1;
	}

	Token *tok = tokenize(argv[1]);

	printf("	.global main\n");
	printf("main:\n");
	printf("	mov $%d, %%rax\n", get_number(tok));
	tok = tok -> next;

	while (tok -> kind != TK_EOF) {
		if (equal(tok, "+")) {
			printf("	add $%d, %%rax\n", get_number(tok -> next));
			tok = tok -> next -> next;
			continue;
		}

		if (equal(tok, "-")) {
			printf("	sub $%d, %%rax\n", get_number(tok -> next));
			tok = tok -> next -> next;
			continue;
		}

		error("invalid tok");
	}
	printf("	ret\n");
}
