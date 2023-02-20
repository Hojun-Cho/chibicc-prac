#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// utils
//

static bool startwith(char *p, char *q) {
	return strncmp(p, q, strlen(q)) == 0;
}

//
//	tokens
//

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

static int read_punct(char *p) {
	if (startwith(p, "==") || startwith(p, "!=")
			|| startwith(p, "<=") || startwith(p, ">="))
		return 2;
	return ispunct(*p) ? 1 : 0;
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
	char *start;
	int punct_len;

	while (*p) {
		if (isspace(*p)) {
			p++;
			continue;
		}

		if (isdigit(*p)) {
			cur = cur -> next = new_token(TK_NUM, p, p);
			start = p;
			cur -> val = strtoul(p, &p, 10);
			cur -> len = p - start;
			continue;
		}
		punct_len = read_punct(p);
		if (punct_len > 0) {
			cur = cur -> next = new_token(TK_PUNCT, p, p + punct_len);
			p += punct_len;
			continue;
		}
		error("invalid token");
	}

	cur = cur -> next = new_token(TK_EOF, p, p);
	return head.next;
}

//
//	parser
//

typedef enum {
	ND_ADD,
	ND_SUB,
	ND_MUL,
	ND_DIV,
	ND_NEG, // unary -
	ND_EQ, // ==
	ND_NE, // !=
	ND_LT, // <
	ND_LE, // <
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
static Node *unary(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);

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

static Node *new_unary(Nodekind kind, Node *expr) {
	Node *node = new_node(kind);
	node -> lhs = expr;
	return node;
}

static Node *new_num(int val) {
	Node *node = new_node(ND_NUM);
	node -> val = val;
	return node;
}


// expr = equality
static Node *expr(Token **rest, Token *tok) {
	return equality(rest, tok);
}

// equality = relational ("==" relational | "!=" relational)
static Node *equality(Token **rest, Token *tok) {
	Node *node = relational(&tok, tok);

	while (1) {
		if (equal(tok, "==")) {
			node = new_binary(ND_EQ, node
					, relational(&tok, tok -> next));
			continue;
		}
		if (equal(tok, "!=")) {
			node = new_binary(ND_NE, node
					, relational(&tok, tok -> next));
			continue;
		}

		*rest = tok;
		return node;
	}
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *tok) {
	Node *node = add(&tok, tok);

	while (1) {
		if (equal(tok, "<")) {
			node = new_binary(ND_LT, node, add(&tok, tok -> next));
			continue;
		}
		if (equal(tok, "<=")) {
			node = new_binary(ND_LE, node, add(&tok, tok -> next));
			continue;
		}
		if (equal(tok, ">")) {
			node = new_binary(ND_LT, add(&tok, tok -> next), node);
			continue;
		}
		if (equal(tok, ">=")) {
			node = new_binary(ND_LE, add(&tok, tok->next), node);
			continue;
		}

		*rest = tok;
		return node;
	}
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *tok) {
	Node *node = mul(&tok, tok);

	while (1) {
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

// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *tok) {
	Node *node = unary(&tok, tok);

	while(1) {
		if (equal(tok, "*")) {
			node = new_binary(ND_MUL, node, unary(&tok, tok -> next));
			continue;
		}
		if (equal(tok ,"/")) {
			node = new_binary(ND_DIV, node, unary(&tok, tok -> next));
			continue;
		}
		// if nothing
		*rest = tok;
		return node;
	}
}

// unary = ("+" | "-") unary
//		   | primary
static Node *unary(Token **rest, Token *tok) {
	if (equal(tok, "+"))
		return unary(rest, tok -> next);
	if (equal(tok, "-"))
		return new_unary(ND_NEG, unary(rest, tok -> next));
	return primary(rest, tok);
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
	switch (node -> kind) {
		// if not call both hands
		case ND_NUM:
			printf("	mov $%d, %%rax\n", node -> val);
			return;
		case ND_NEG:
			gen_expr(node -> lhs);
			printf("	neg %%rax\n");
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
		case ND_EQ:
		case ND_NE:
		case ND_LT:
		case ND_LE:
			printf("	cmp %%rdi, %%rax\n");

			if (node -> kind == ND_EQ)
				printf("	sete %%al\n");
			else if (node -> kind == ND_NE)
				printf("	setne %%al\n");
			else if (node -> kind == ND_LT)
				printf("	setl %%al\n");
			else if (node -> kind == ND_LE)
				printf("	setle %%al\n");

			printf("	movzb %%al, %%rax\n");
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
