#include "chibicc.h"

static Obj *current_fn;

static void gen_stmt(Node *node);
static void gen_expr(Node *node);

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

static void load(Type *ty) {
	if (ty -> kind == TY_ARRAY)
		return;
	printf("	mov (%%rax), %%rax\n");
}

static void store(void) {
	pop_to("%rdi");
	printf("	mov %%rax, (%%rdi)\n");
}

static void gen_addr(Node *node) {
	if (node -> kind == ND_VAR) {
		if (node -> var -> is_local) 
			printf("	lea %d(%%rbp), %%rax\n", node -> var -> offset);
		else
			printf("	lea %s(%%rip), %%rax\n", node -> var -> name);
		return;
	}
	else if (node -> kind == ND_DEREF)
		gen_expr(node -> lhs);
	return;
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
			load(node->ty);
			return;
		case ND_ADDR:
			gen_addr(node -> lhs);
			return;
		case ND_DEREF:
			gen_expr(node -> lhs);
			load(node->ty);
			return;
		case ND_ASSIGN:
			gen_addr(node -> lhs);
			push(); // push rax
			gen_expr(node -> rhs);
			store();
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
		printf("	jmp .L.return.%s\n", current_fn->name);
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

static void assign_lvar_offset(Obj *prog) {
	for (Obj *fn = prog; fn; fn = fn->next) {
		if (fn -> is_function == false)
			continue;
		int offset = 0;
		for (Obj *var = fn -> locals; var; var = var->next) {
			offset += var -> ty -> size;
			var -> offset = -offset;
		}
		fn -> stack_size = offset;
	}
}

static void emit_data(Obj *prog) {

}


void code_gen(Obj *prog) {
	assign_lvar_offset(prog);

	for (Obj *var = prog; var; var = var -> next) {
		if (var -> is_function) 
			continue;
		printf("	.data\n");
		printf("	.global %s\n", var -> name);
		printf("%s:\n", var -> name);
		printf("	.zero %d\n", var -> ty -> size);
	}
	
	for (Obj *fn = prog; fn; fn = fn->next) {
		if (fn -> is_function == false)
			continue;
		printf("	.global %s\n", fn->name);
		printf("	.text\n");
		printf("%s:\n", fn->name);
		current_fn = fn;

		// Prologue
		printf("	push %%rbp\n");
		printf("	mov %%rsp, %%rbp\n");
		printf("	sub $%d, %%rsp\n", fn->stack_size);

		// Emit code
		gen_stmt(fn->body);

		// Epilogue
		printf(".L.return.%s:\n", fn->name);
		printf("	mov %%rbp, %%rsp\n");
		printf("	pop %%rbp\n");
		printf("	ret\n");
	}
}
