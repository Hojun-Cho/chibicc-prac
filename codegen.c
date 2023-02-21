#include "chibicc.h"

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

static void gen_stmt(Node *node) {
	if (node -> kind == ND_EXPR_STMT) {
		gen_expr(node->lhs);
		return;
	}
	error("invalid stmt");
}

void code_gen(Node *node) {
	printf("	.global main\n");
	printf("main:\n");

	for (Node *n = node; n; n = n->next) {
		gen_stmt(n);
	}
	printf("ret\n");
}
