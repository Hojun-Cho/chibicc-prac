#include "chibicc.h"

static void gen_stmt(Node *node);

static int count(void) {
	static int i = 1;
	return i++;
}

static void push(void) {
	printf("	push %%rax\n");
}

static void pop_to(char *arg) {
	printf("	pop %s\n", arg);
}

static void gen_addr(Node *node) {
	if (node -> kind == ND_VAR) {
		printf("	lea %d(%%rbp), %%rax\n", node -> var -> offset);
		return;
	}
}

static void gen_expr(Node *node) {
	switch (node -> kind) {
		case ND_NUM:
			printf("	mov $%d, %%rax\n", node -> val);
			return;
		case ND_NEG:
			gen_expr(node -> lhs);
			printf("	neg %%rax\n");
			return;
		case ND_VAR:
			gen_addr(node);
			printf("	mov (%%rax), %%rax\n");
			return;
		case ND_ASSIGN:
			gen_addr(node -> lhs);
			push(); // push rax
			gen_expr(node -> rhs);
			pop_to("%rdi");
			printf("	mov %%rax, (%%rdi)\n");
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
	if (node -> kind == ND_BLOCK) {
		for (Node *n = node -> body; n; n = n->next)
			gen_stmt(n);
		return;
	}
	if (node -> kind == ND_RETURN) {
		gen_expr(node -> lhs);
		printf("	jmp .L.return\n");
		return;
	}
	if (node -> kind == ND_EXPR_STMT) {
		gen_expr(node->lhs);
		return;
	}

	if (node -> kind == ND_IF) {
		int c = count();
		gen_expr(node -> cond);
		printf("	cmp $0, %%rax\n");
		printf("	je .L.else.%d\n", c);
		gen_stmt(node -> then);
		printf("	jmp .L.end.%d\n", c);
		printf(".L.else.%d:\n", c);
		if (node -> _else)
			gen_stmt(node -> _else);
		printf(".L.end.%d:\n", c);
		return;
	}

	error("invalid stmt");
}

static void assign_lvar_offset(Function *prog) {
	int offset = 0;
	for (Obj *var = prog -> locals; var; var = var -> next) {
		offset += 8;
		var -> offset -= offset;
	}
	prog -> stack_size = offset;
}

void code_gen(Function *prog) {
	assign_lvar_offset(prog);

	printf("	.global main\n");
	printf("main:\n");

	printf("	push %%rbp\n");
	printf("	mov %%rsp, %%rbp\n");
	printf("	sub $%d, %%rsp\n", prog->stack_size);

	gen_stmt(prog->body);

	printf(".L.return:\n");
	printf("	mov %%rbp, %%rsp\n");
	printf("	pop %%rbp\n");
	printf("	ret\n");
}