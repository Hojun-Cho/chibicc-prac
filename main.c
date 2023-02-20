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

		if (ispunct(*p)) {
			cur = cur -> next = new_token(TK_PUNCT, p, p + 1);
			p++;
			continue;
		}
		error("invalid token");
	}

	cur = cur -> next = new_token(TK_EOF, p, p);
	return head.next;
}

/*
 *	parser
 */
typedef enum {
	ND_ADD,
	ND_SUB,
	ND_MUL,
	ND_DIV,
	ND_NUM,
} Nodekind;

typedef struct Node Node;
struct Node {
	Nodekind kind;
	Node *lhs;
	Node *rhs;

	// ND_NUM
	int val;
};

static Node *expr(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

static Node *new_node(Nodekind kind) {
	Node *node = calloc(1, sizeof(Node));
	node -> kind = kind;
	return node;
}

static Node *new_binary(Nodekind kind, Node *lhs, Node *rhs) {
	Node *node = new_node(kind);
	node -> lhs = lhs;
	node -> rhs = rhs;
	return node;
}

static Node *new_num(int val) {
	Node *node = new_node(ND_NUM);
	node -> val = val;
	return node;
}

// expr = mul ("+" mul | "-" mul)*
static Node *expr(Token **rest, Token *tok) {
	Node *node = mul(&tok, tok);

	while(1) {
		if (equal(tok, "+")) {
			node = new_binary(ND_ADD, node, mul(&tok, tok -> next));
			continue;
		}

		if (equal(tok, "-")) {
			node = new_binary(ND_SUB, node, mul(&tok, tok -> next));
			continue;
		}

		*rest = tok;
		return node;
	}
}

// mul = primary ("*" primary | "/" primary)*
static Node *mul(Token **rest, Token *tok) {
	Node *node = primary(&tok, tok);

	while(1) {
		if (equal(tok, "*")) {
			node = new_binary(ND_MUL, node, primary(&tok, tok -> next));
			continue;
		}

		if (equal(tok ,"/")) {
			node = new_binary(ND_DIV, node ,primary(&tok, tok -> next));
			continue;
		}

		// if nothing
		*rest = tok;
		return node;
	}
}

// primary = "(" expr ")" | num
static Node *primary(Token **rest, Token *tok) {
	Node *node;

	if (equal(tok, "(")) {
		node = expr(&tok, tok -> next);
		*rest = skip(tok, ")");
		return node;
	}

	if (tok -> kind == TK_NUM) {
		node = new_num(tok -> val);
		*rest = tok -> next;
		return node;
	}

	error("expected an expression");
}

// gen code

static void push(void) {
	printf("	push %%rax\n");
}

static void pop_to(char *arg) {
	printf("	pop %s\n", arg);
}

static void gen_expr(Node *node) {
	if (node -> kind == ND_NUM) {
		printf("	mov $%d, %%rax\n", node -> val);
		return;
	}

	gen_expr(node -> rhs);
	push();
	gen_expr(node -> lhs);
	pop_to("%rdi");

	switch (node -> kind) {
		case ND_ADD:
			printf("	add %%rdi, %%rax\n");
			return;
		case ND_SUB:
			printf("	sub %%rdi, %%rax\n");
			return;
		case ND_MUL:
			printf("	imul %%rdi, %%rax\n");
			return;
		case ND_DIV:
			printf("	cqo\n");
			printf("	idiv %%rdi\n");
			return;
	}
	error("invalid expression");

}

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
