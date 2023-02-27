#include "chibicc.h"

Type *ty_int = &(Type){TY_INT, 4};
Type *ty_char = &(Type){TY_CHAR, 1};
Type *ty_short = &(Type){TY_SHORT, 2};

Type *get_type_can_null(Token *tok) {
	if (equal(tok, "int")) 
		return ty_int;
	if (equal(tok, "char"))
		return ty_char;
	if (equal(tok, "short"))
		return ty_short;
	return NULL;
}

Type *func_type(Type *return_ty) {
	Type *ty = calloc(1, sizeof(Type));
	ty -> kind = TY_FUNC;
	ty -> return_ty = return_ty;
	return ty;
}

Type *pointer_to(Type *base) {
	Type *ty = calloc(1, sizeof(Type));
	ty -> kind = TY_PTR;
	ty -> size = 8;
	ty -> base = base;
	return ty;
}

Type *array_of(Type *base, int len) {
	Type *ty = calloc(1, sizeof(Type));
	ty -> kind = TY_ARRAY;
	ty -> size = base->size * len;
	ty -> base = base;
	ty -> array_len = len;
	return ty;
}

bool is_integer(Type *ty) {
	return ty -> kind == TY_INT || ty -> kind == TY_CHAR 
		|| ty -> kind == TY_SHORT;
}

void add_type(Node *node) {
	if (!node || node->ty)
		return;

	add_type(node->lhs);
	add_type(node->rhs);
	add_type(node->cond);
	add_type(node->then);
	add_type(node->_else);

	for (Node *n = node->body; n; n = n->next)
		add_type(n);

	switch (node->kind) {
		case ND_ADD:
		case ND_SUB:
		case ND_MUL:
		case ND_DIV:
		case ND_NEG:
		case ND_ASSIGN:
			node->ty = node->lhs->ty;
			return;
		case ND_EQ:
		case ND_NE:
		case ND_LT:
		case ND_LE:
		case ND_NUM: 
		case ND_FUNCALL:
			node -> ty = ty_int;
			return;
		case ND_VAR:
			node -> ty = node -> var -> ty;
			return;
		case ND_ADDR:
			if (node -> lhs -> ty -> kind == TY_ARRAY)
				node->ty = pointer_to(node->lhs->ty->base);
			else 
				node->ty = pointer_to(node->lhs->ty);
			return;
		case ND_DEREF:
			if (node->lhs->ty->base == NULL)
				error("invalid pointer in type.c:53");
			node -> ty = node -> lhs -> ty -> base;
			return;
	}
}
