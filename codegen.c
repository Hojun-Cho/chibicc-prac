#include "chibicc.h"

static char *argreg8[] = {"%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"};
static char *argreg16[] = {"%di", "%si", "%dx", "%cx", "%r8w", "%r9w"};
static char *argreg32[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
static char *argreg64[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static Obj *current_fn;

static void gen_stmt(Node *node);
static void gen_expr(Node *node);

static int count(void)
{
	static int i = 1;
	return i++;
}

static void push(void)
{
	printf("	push %%rax\n");
}

static void pop_to(char *arg)
{
	printf("	pop %s\n", arg);
}

// load_value value
static void load_value(Type *ty)
{
	if (ty->kind == TY_ARRAY || ty->kind == TY_STRUCT)
		return;
	if (ty->size == 1)
		printf("	movsbq (%%rax), %%rax\n");
	else if (ty->size == 2)
		printf("	movswq (%%rax), %%rax\n");
	else if (ty->size == 4)
		printf("	movsxd (%%rax), %%rax\n");
	else
		printf("	mov (%%rax), %%rax\n");
}

static void store(Type *ty)
{
	pop_to("%rdi");

	// shallow copy
	if (ty->kind == TY_STRUCT)
	{
		// rdi is node->lhs addr
		// copy all bytes
		for (int i = 0; i < ty->size; i++)
		{
			printf("	mov %d(%%rax), %%r8b\n", i);
			printf("	mov %%r8b, %d(%%rdi)\n", i);
		}
		return;
	}
	if (ty->size == 1)
		printf("	mov %%al, (%%rdi)\n");
	else if (ty->size == 2)
		printf("	mov %%ax, (%%rdi)\n");
	else if (ty->size == 4)
		printf("	mov %%eax, (%%rdi)\n");
	else
		printf("	mov %%rax, (%%rdi)\n");
}

static void gen_addr(Node *node)
{
	if (node->kind == ND_VAR)
	{
		if (node->var->is_local)
			printf("	lea %d(%%rbp), %%rax\n", node->var->offset);
		else
			printf("	lea %s(%%rip), %%rax\n", node->var->name);
		return;
	}
	else if (node->kind == ND_DEREF)
		gen_expr(node->lhs);
	else if (node->kind == ND_FIELD)
	{
		gen_addr(node->lhs);
		printf("	add $%d, %%rax\n", node->field->offset);
	}
	return;
}

static void gen_expr(Node *node)
{
	switch (node->kind)
	{
	case ND_NUM:
		printf("	mov $%ld, %%rax\n", node->val);
		return;
	case ND_NEG:
		gen_expr(node->lhs);
		printf("	neg %%rax\n");
		return;
	case ND_VAR:
		gen_addr(node);
		load_value(node->ty);
		return;
	case ND_ADDR:
		gen_addr(node->lhs);
		return;
	case ND_DEREF:
		gen_expr(node->lhs);
		load_value(node->ty);
		return;
	case ND_ASSIGN:
		gen_addr(node->lhs);
		push(); // push rax
		gen_expr(node->rhs);
		store(node->ty);
		return;
	case ND_FIELD:
		gen_addr(node);
		load_value(node->ty);
		return;
	case ND_FUNCALL:
	{
		int argc = 0;
		for (Node *arg = node->args; arg; arg = arg->next)
		{
			gen_expr(arg);
			push();
			argc++;
		}
		for (int i = argc - 1; i >= 0; i--)
			pop_to(argreg64[i]);

		printf("	call %s\n", node->funcname);
		return;
	}
	}

	gen_expr(node->rhs);
	push();
	gen_expr(node->lhs);
	pop_to("%rdi");

	switch (node->kind)
	{
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

		if (node->kind == ND_EQ)
			printf("	sete %%al\n");
		else if (node->kind == ND_NE)
			printf("	setne %%al\n");
		else if (node->kind == ND_LT)
			printf("	setl %%al\n");
		else if (node->kind == ND_LE)
			printf("	setle %%al\n");

		printf("	movzb %%al, %%rax\n");
		return;
	}
	error_tok(node->tok, "invalid expression");
}

static void gen_stmt(Node *node)
{
	if (node->kind == ND_BLOCK)
	{
		for (Node *n = node->body; n; n = n->next)
			gen_stmt(n);
		return;
	}
	if (node->kind == ND_RETURN)
	{
		gen_expr(node->lhs);
		printf("	jmp .L.return.%s\n", current_fn->name);
		return;
	}
	if (node->kind == ND_EXPR_STMT)
	{
		gen_expr(node->lhs);
		return;
	}

	if (node->kind == ND_IF)
	{
		int c = count();
		gen_expr(node->cond);
		printf("	cmp $0, %%rax\n");
		printf("	je .L.else.%d\n", c);
		gen_stmt(node->then);
		printf("	jmp .L.end.%d\n", c);
		printf(".L.else.%d:\n", c);
		if (node->_else)
			gen_stmt(node->_else);
		printf(".L.end.%d:\n", c);
		return;
	}

	error_tok(node->tok, "invalid stmt");
}

static int align_to(int n, int align)
{
	return (n + align - 1) / align * align;
}

static void assign_lvar_offset(Obj *prog)
{
	for (Obj *fn = prog; fn; fn = fn->next)
	{
		if (fn->is_function == false)
			continue;
		int offset = 0;
		for (Obj *var = fn->locals; var; var = var->next)
		{
			offset += var->ty->size;
			var->offset = -offset;
		}
		fn->stack_size = align_to(offset, 16);
	}
}

static void emit_data(Obj *prog)
{
	for (Obj *var = prog; var; var = var->next)
	{
		if (var->is_function)
			continue;
		printf("	.data\n");
		printf("	.global %s\n", var->name);
		printf("%s:\n", var->name);
		if (var->init_data)
			for (int i = 0; i < var->ty->size; i++) // array type
				printf("	.byte %d\n", var->init_data[i]);
		else
			printf("	.zero %d\n", var->ty->size);
	}
}

static void emit_text(Obj *prog)
{
	for (Obj *fn = prog; fn; fn = fn->next)
	{
		if (fn->is_function == false || fn->is_definition == false)
			continue;
		printf("	.global %s\n", fn->name);
		printf("	.text\n");
		printf("%s:\n", fn->name);
		current_fn = fn;

		// Prologue
		printf("	push %%rbp\n");
		printf("	mov %%rsp, %%rbp\n");
		printf("	sub $%d, %%rsp\n", fn->stack_size);

		int r = 0;
		for (Obj *var = fn->params; var; var = var->next)
			if (var->ty->size == 1)
				printf("	mov %s, %d(%%rbp)\n",
					   argreg8[r++], var->offset);
			else if (var->ty->size == 2)
				printf("	mov %s, %d(%%rbp)\n",
					   argreg16[r++], var->offset);
			else if (var->ty->size == 4)
				printf("	mov %s, %d(%%rbp)\n",
					   argreg32[r++], var->offset);
			else
				printf("	mov %s, %d(%%rbp)\n",
					   argreg64[r++], var->offset);
		// Emit code
		gen_stmt(fn->body);

		// Epilogue
		printf(".L.return.%s:\n", fn->name);
		printf("	mov %%rbp, %%rsp\n");
		printf("	pop %%rbp\n");
		printf("	ret\n");
	}
}

void code_gen(Obj *prog)
{
	assign_lvar_offset(prog);
	emit_data(prog);
	emit_text(prog);
}
