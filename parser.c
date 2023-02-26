#include "chibicc.h"

static Obj *locals;
static Obj *globals;

static Obj *new_lvar(char *name, Type *ty) {
	Obj *var = new_var(name, ty);
	var -> is_local = true;
	var -> next = locals;
	locals = var;
	return var;

}

static Obj *new_gvar(char *name, Type *ty) {
	Obj *var = new_var(name, ty);
	var -> next = globals;
	globals = var;
	return var;
}

static Obj *new_anon_gvar(Type *ty) {
	static int id = 0;
	char *buf = calloc(1, 20);
	sprintf(buf, ".L..%d", id++);
	return new_gvar(buf, ty);
}

static Obj *new_string_literal(char *p, Type *ty) {
	Obj *var = new_anon_gvar(ty);
	var->init_data = p;
	return var;
}

static Node *declaration(Token **rest, Token *tok);
static Type *declspec(Token **rest, Token *tok);
static Type *declarator(Token **rest, Token *tok, Type *ty);
static Node *mul(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *expr_stmt(Token **reset, Token *tok);
static Node *expr (Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *compound_stmt(Token **rest, Token *tok);
static Node *stmt(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *postfix(Token **rest, Token *tok);

static Node *new_node(Nodekind kind) {
	Node *node = calloc(1, sizeof(Node));
	node -> kind = kind;
	return node;
}

static Node *new_var_node(Obj *var) {
	Node *node = new_node(ND_VAR);
	node -> var = var;
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

// compound-stmt = (declaration | stmt)* "}"
static Node *compound_stmt(Token **rest, Token *tok) {
	Node *node = new_node(ND_BLOCK);
	Node head = {};
	Node *cur = &head;
	enter_scope();
	// stmt type is result of stmt
	while (!equal(tok, "}")) {
		if (equal(tok, "int") || equal(tok, "char"))
			cur = cur -> next = declaration(&tok, tok);
		else
			cur = cur -> next = stmt(&tok, tok);
		add_type(cur);
	}

	node -> body = head.next;
	*rest = tok -> next;
	leave_scope();
	return node;
}

// stmt = expr-stmt
//		  | "{" compound_stmt
//		  | "return" expr ";"
//		  | "if" "(" expr ")" stmt ("else" stmt) ?
static Node *stmt(Token **rest, Token *tok) {
	Node *node;

	if (equal(tok, "return")) {
		node = new_node(ND_RETURN);
		node -> lhs = expr(&tok, tok -> next);
		*rest = skip(tok, ";");
		return node;
	}
	if (equal(tok, "if")) {
		node = new_node(ND_IF);
		tok = skip(tok -> next, "(");
		node -> cond = expr(&tok, tok);
		tok = skip(tok, ")");
		node -> then = stmt(&tok, tok);
		if (equal(tok, "else"))
			node -> _else = stmt(&tok, tok -> next);
		*rest = tok;
		return node;
	}

	if (equal(tok, "{"))
		return compound_stmt(rest, tok -> next);
	return expr_stmt(rest, tok);
}

static Type *func_params(Token **rest, Token *tok, Type *ty) {
	Type head = {};
	Type *cur = &head;
	Type *_ty;	
	while (!equal(tok, ")")) {
		if (cur != &head)
			tok = skip(tok, ",");
		Type *basety = declspec(&tok, tok);
		_ty = declarator(&tok, tok, basety);
		Type *temp = calloc(1, sizeof(Type));
		*temp = *_ty;
		cur = cur -> next = temp;
	}
	ty = func_type(ty);
	ty->params = head.next;
	*rest = tok -> next;
	return ty;
}

// type-suffix = ( "(" func-params? ")")?
// func-params = param ("," param)*
// 				| "[" num "]" type-suffix
static Type *type_suffix(Token **rest, Token *tok, Type *ty) {
	if (equal(tok, "(")) {
		return func_params(rest, tok -> next, ty);
	}
	if (equal(tok, "[")) {
		int size = get_number(tok -> next);
		tok = skip(tok->next->next, "]");
		ty = type_suffix(rest, tok, ty);
		return array_of(ty, size);
	}
	*rest = tok;
	return ty;
}

// declspec = "int"
static Type *declspec(Token **rest, Token *tok) {
	if (equal(tok, "char")) {
		*rest = tok -> next;
		return ty_char;
	}
	*rest = skip(tok, "int");
	return ty_int;
}

// declarator = "*"* ident type-suffix
static Type *declarator(Token **rest, Token *tok, Type *ty) {
	while (consume_if_same(&tok, tok, "*") == true) // eg. int ****x
		ty = pointer_to(ty);

	if (tok -> kind != TK_IDENT)
		error("declarator: expected ident");

	ty = type_suffix(rest, tok -> next, ty);
	ty -> decl = tok;
	return ty;
}

// declaration = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
static Node *declaration(Token **rest, Token *tok) {
	Node head = {};
	Node *cur = &head;
	int i = 0;
	Type *basety = declspec(&tok, tok);

	// int x,y,z;
	while (!equal(tok, ";")) {
		if (i++ > 0)
			tok = skip(tok, ",");
		Type *ty = declarator(&tok, tok, basety);
		Obj *var = new_lvar(get_ident(ty -> decl), ty);

		if (!equal(tok, "="))
			continue;
		// int x=1, y=2, z= 3;
		Node *lhs = new_var_node(var);
		Node *rhs = assign(&tok, tok -> next);
		Node *node = new_binary(ND_ASSIGN, lhs, rhs);
		// MUST!!  ND_EXPR_STMT (codegen.c - gen_stmt)
		cur = cur -> next = new_unary(ND_EXPR_STMT, node);
	}
	Node *node = new_node(ND_BLOCK);
	node -> body = head.next;
	*rest = tok -> next;
	return node;
}

// expr-stmt = expr ";"
static Node *expr_stmt(Token **rest, Token *tok) {
	Node *node = new_unary(ND_EXPR_STMT, expr(&tok, tok));
	*rest = skip(tok, ";");
	return node;
}

// expr = assign
static Node *expr(Token **rest, Token *tok) {
	return assign(rest, tok);
}

// assign = equality ("=" assign);
static Node *assign(Token **rest, Token *tok) {
	Node *node = equality(&tok, tok);

	if (equal(tok, "="))
		// x = y = 2;
		node = new_binary(ND_ASSIGN, node, assign(&tok, tok -> next));
	*rest = tok;
	return node;
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

static Node *new_add(Node *lhs, Node *rhs) {
	add_type(lhs);
	add_type(rhs);

	if (is_integer(lhs -> ty) && is_integer(rhs -> ty))
		return new_binary(ND_ADD, lhs, rhs);
	if (lhs -> ty -> base && rhs -> ty -> base)
		error("lhs and rhs both have base");
	if (is_integer(lhs -> ty) && rhs -> ty -> base) {
		Node *tmp = lhs;
		lhs = rhs;
		rhs = tmp;
	}

	// ptr + num
	rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size));
	return new_binary(ND_ADD, lhs, rhs);
}

// pointer - int
static Node *new_sub(Node *lhs, Node *rhs) {
	add_type(lhs);
	add_type(rhs);

	if (is_integer(lhs -> ty) && is_integer(rhs -> ty))
		return new_binary(ND_SUB, lhs, rhs);

	// ptr - num
	if (lhs -> ty -> base && is_integer(rhs -> ty)) {
		rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size));
		add_type(rhs);
		Node *node = new_binary(ND_SUB, lhs , rhs);
		node -> ty = lhs -> ty;
		return node;
	}
	// ptr - ptr
	if (lhs -> ty -> base && rhs -> ty -> base) {
		Node *node = new_binary(ND_SUB, lhs, rhs);
		node -> ty = ty_int;
		return new_binary(ND_DIV, node, new_num(lhs->ty->base->size));
	}
	error("invalid op in new_sub");
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *tok) {
	Node *node = mul(&tok, tok);

	while (1) {
		if (equal(tok, "+")) {
			node = new_add(node, mul(&tok, tok -> next));
			continue;
		}
		if (equal(tok, "-")) {
			node = new_sub(node, mul(&tok, tok -> next));
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

// unary = ("+" | "-" | "*" | "&" ) unary
//		   | postfix
static Node *unary(Token **rest, Token *tok) {
	if (equal(tok, "+"))
		return unary(rest, tok -> next);
	if (equal(tok, "-"))
		return new_unary(ND_NEG, unary(rest, tok -> next));
	if (equal(tok, "&"))
		return new_unary(ND_ADDR, unary(rest, tok -> next));
	if (equal(tok, "*"))
		return new_unary(ND_DEREF, unary(rest, tok -> next));
	return postfix(rest, tok);
}

// postfix = primary ("[" expr "]")*
static Node *postfix(Token **rest, Token *tok) {
	Node *node = primary(&tok, tok);

	while (equal(tok, "[")) {
		Node *idx = expr(&tok, tok -> next);
		tok = skip(tok, "]");
		node = new_unary(ND_DEREF, new_add(node, idx));
	}
	*rest = tok;
	return node;
}

// funcall = ident "(" (assign ("," assign)*)? ")"
static Node *funcall(Token **rest, Token *tok) {
	Node *node = new_node(ND_FUNCALL);
	node -> funcname = strndup(tok->loc, tok->len);
	tok = tok -> next -> next;
	Node head = {};
	Node *cur = &head;

	while (!equal(tok, ")")) {
		if (cur != &head)
			tok = skip(tok, ",");
		cur = cur -> next = assign(&tok, tok);
	}
	*rest = skip(tok, ")");
	node->args = head.next;
	return node;
}

// primary = "(" expr ")" | funcall | sizeof unary | num
//				| char | str
static Node *primary(Token **rest, Token *tok) {
	Node *node;

	if (equal(tok, "(")) {
		node = expr(&tok, tok -> next);
		*rest = skip(tok, ")");
		return node;
	}

	if (equal(tok, "sizeof")) {
		node = unary(rest, tok -> next);
		add_type(node);
		return new_num(node -> ty -> size);
	}

	if (tok -> kind == TK_IDENT) {
		if (equal(tok -> next, "("))
			return funcall(rest, tok);
		Obj *var = find_var(tok);
		if (var == NULL)
			error("undefined variable");
		*rest = tok -> next;
		return new_var_node(var);
	}
	if (tok -> kind == TK_STR) {
		Obj * var = new_string_literal(tok -> str, tok -> ty);
		*rest = tok -> next;
		return new_var_node(var);
	}
	if (tok -> kind == TK_NUM || tok -> kind == TK_CHAR) {
		node = new_num(tok -> val);
		*rest = tok -> next;
		return node;
	}
	error("expected an expression");
}

static void create_func_params(Type *param) {
	if (param) {
		create_func_params(param->next);
		new_lvar(get_ident(param->decl), param);
	}
}

static Token *function(Token *tok, Type *basety) {
	enter_scope();

	locals = NULL;
	Type *ty = declarator(&tok, tok, basety);
	Obj *fn = new_gvar(get_ident(ty -> decl), ty);
	create_func_params(ty->params);
	fn -> params  = locals;
	fn -> is_function = true;
	tok = skip(tok, "{");
	fn -> body = compound_stmt(&tok, tok);
	fn -> locals = locals;
	leave_scope();
	return tok;
}

static Token *global_variable(Token *tok, Type *basety) {
	bool first = true;

	while (consume_if_same(&tok, tok, ";") == false) {
		if (first == false)
			tok = skip(tok, ",");
		first = false;
		Type *ty = declarator(&tok, tok, basety);
		new_gvar(get_ident(ty -> decl), ty);
	}
	return tok;
}

static bool is_function(Token *tok) {
	if (equal(tok -> next, ";"))
		return false;
	Type dummy = {};
	Type *ty = declarator(&tok, tok, &dummy);
	return ty -> kind == TY_FUNC;
}

// program = (function-definition | global_variable)*
Obj *parse(Token *tok) {
	globals = NULL;
	while (tok -> kind != TK_EOF) {
		Type *basety = declspec(&tok, tok);
		if (is_function(tok)) {
			tok = function(tok, basety);
			continue;
		}
		tok = global_variable(tok, basety);
	}
	return globals;
}
