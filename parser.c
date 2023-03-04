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
static Type *struct_decl(Token **rest, Token *tok);
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

static Node *new_node(Nodekind kind, Token *tok) {
	Node *node = calloc(1, sizeof(Node));
	node -> kind = kind;
	node -> tok = tok;
	return node;
}

static Node *new_var_node(Obj *var, Token *tok) {
	Node *node = new_node(ND_VAR, tok);
	node -> var = var;
	return node;
}

static Node *new_binary(Nodekind kind, Node *lhs, Node *rhs, Token *tok) {
	Node *node = new_node(kind, tok);
	node -> lhs = lhs;
	node -> rhs = rhs;
	return node;
}

static Node *new_unary(Nodekind kind, Node *expr, Token *tok) {
	Node *node = new_node(kind, tok);
	node -> lhs = expr;
	return node;
}

static Node *new_num(int64_t val, Token *tok) {
	Node *node = new_node(ND_NUM, tok);
	node -> val = val;
	return node;
}

// compound-stmt = (declaration | stmt)* "}"
static Node *compound_stmt(Token **rest, Token *tok) {
	Node *node = new_node(ND_BLOCK, tok);
	Node head = {};
	Node *cur = &head;
	enter_scope();

	while (!equal(tok, "}")) {
		if (is_type(tok))
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
		node = new_node(ND_RETURN, tok);
		node -> lhs = expr(&tok, tok -> next);
		*rest = skip(tok, ";");
		return node;
	}
	if (equal(tok, "if")) {
		node = new_node(ND_IF, tok);
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
//				| "[" num "]" type-suffix
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

// declspec = ("int" | "short" | "char" | "long" | "void"
//			  | struct-decl) +
static Type *declspec(Token **rest, Token *tok) {
	enum {
		VOID = 1 << 0,   // 1
		CHAR = 1 << 2,   // 4
		SHORT = 1 << 4,  // 16
		INT = 1 << 6,
		LONG = 1 << 8,
		OTHER = 1 << 10,
	};
	Type *ty = NULL;
	int counter = 0;

	while (is_type(tok)) {
		if (equal(tok, "struct")) {
			ty = struct_decl(&tok, tok->next);
			counter += OTHER;
			continue;
		}
		if (equal(tok, "char"))
			counter += CHAR;
		else if (equal(tok, "short"))
			counter += SHORT;
		else if (equal(tok, "long"))
			counter += LONG;
		else if (equal(tok, "int"))
			counter += INT;
		else if (equal(tok, "void"))
			counter += VOID;
		else
			error_tok(tok, "unknown type name");

		switch (counter) {
			case VOID:
				ty = ty_void;
				break;
			case INT:
				ty = ty_int;
				break;
			case CHAR:
				ty = ty_char;
				break;
			case SHORT:
			case SHORT + INT: // short ; short int; int short;
				ty = ty_short;
				break;
			case LONG:
			case LONG + INT:
				ty = ty_long;
				break;
			default:
				error_tok(tok, "invalid type");
		}
		tok = tok->next;
	}
	*rest = tok;
	return ty;
}

// struct-members = (declspec declarator ";")*
static void struct_fields(Token **rest, Token *tok, Type *ty) {
	Field head = {};
	Field *cur = &head;

	while (!equal(tok, "}")) {
		Type *basety = declspec(&tok, tok);
		int i = 0;
		while (consume_if_same(&tok, tok, ";") == false) {
			if (i++ != 0)
				tok = skip(tok, ",");
			Field *field = calloc(1, sizeof(Field));
			field -> ty = declarator(&tok, tok, basety);
			field -> decl = field -> ty -> decl;
			cur = cur -> next = field;
		}
	}
	*rest = tok -> next;
	ty -> fields = head.next;
}

bool is_variable_decl(Token *tok, Token *tag) {
	return tag && equal(tok, "{") == false;
}

// struct-decl = ident? "{" struct fields
static Type *struct_decl(Token **rest, Token *tok) {
	Token *tag = NULL;

	if (tok->kind == TK_IDENT) {
		tag = tok;
		tok = tok->next;
	}

	// if var
	if (is_variable_decl(tok, tag) == true) {
		Type *ty = find_tag(tag);
		if (ty == NULL)
			error_tok(tok, "unknown struct type");
		*rest = tok;
		return ty;
	}
	tok = skip(tok, "{");
	Type *ty = calloc(1, sizeof(Type));
	ty->kind = TY_STRUCT;
	struct_fields(rest, tok, ty);

	int offset = 0;
	for (Field *field = ty->fields; field; field = field->next) {
		field->offset = offset;
		offset += field->ty->size;
	}
	ty->size = offset;
	if (tag)
		push_tag_scope(tag, ty);
	return ty;
}

static Field *get_struct_filed(Type *ty, Token *tok) {
	for (Field *field = ty -> fields; field; field = field->next) {
		if (field -> decl -> len == tok -> len &&
				!strncmp(field -> decl ->loc, tok -> loc, tok -> len))
			return field;
	}
	error_tok(tok, "no such member");
}

static Node *struct_ref(Node *node, Token *tok) {
	add_type(node);
	if (node -> ty -> kind != TY_STRUCT)
		error_tok(tok, "not a struct");

	Node *new_node = new_unary(ND_FIELD, node, tok);
	new_node->field = get_struct_filed(node->ty, tok);
	return new_node;
}

// declarator = "*"*  ( "(" ident ")" | "(" declarator ")" | ident ) type-suffix
static Type *declarator(Token **rest, Token *tok, Type *ty) {
	while (consume_if_same(&tok, tok, "*") == true) // eg. int ****x
		ty = pointer_to(ty);

	// char (*x)[3];
	if (equal(tok, "(")) {
		Token *start = tok;
		Type dummy = {};
		declarator(&tok, tok->next, &dummy); // use dummy to skip token
		tok = skip(tok, ")");
		ty = type_suffix(rest, tok, ty);
		return declarator(&tok, start -> next, ty); // make real declarator => char (*x)[3];
	}

	if (tok -> kind != TK_IDENT)
		error_tok(tok, "expected a variable name");
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
		if (ty -> kind == TY_VOID)
			error_tok(tok, "variable declared void");
		Obj *var = new_lvar(get_ident(ty -> decl), ty);

		if (!equal(tok, "="))
			continue;
		// int x=1, y=2, z= 3;
		Node *lhs = new_var_node(var, ty -> decl);
		Node *rhs = assign(&tok, tok -> next);
		Node *node = new_binary(ND_ASSIGN, lhs, rhs, tok);
		// MUST!!  ND_EXPR_STMT (codegen.c - gen_stmt)
		cur = cur -> next = new_unary(ND_EXPR_STMT, node, tok);
	}
	Node *node = new_node(ND_BLOCK, tok);
	node -> body = head.next;
	*rest = tok -> next;
	return node;
}

// expr-stmt = expr ";"
static Node *expr_stmt(Token **rest, Token *tok) {
	Node *node = new_unary(ND_EXPR_STMT, expr(&tok, tok), tok);
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
		node = new_binary(ND_ASSIGN, node, assign(&tok, tok -> next), tok);
	*rest = tok;
	return node;
}

// equality = relational ("==" relational | "!=" relational)
static Node *equality(Token **rest, Token *tok) {
	Node *node = relational(&tok, tok);

	while (1) {
		Token *start = tok;
		if (equal(tok, "==")) {
			node = new_binary(ND_EQ, node
					, relational(&tok, tok -> next), start);
			continue;
		}
		if (equal(tok, "!=")) {
			node = new_binary(ND_NE, node
					, relational(&tok, tok -> next), start);
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
		Token *start = tok;
		if (equal(tok, "<")) {
			node = new_binary(ND_LT, node, add(&tok, tok -> next), start);
			continue;
		}
		if (equal(tok, "<=")) {
			node = new_binary(ND_LE, node, add(&tok, tok -> next), start);
			continue;
		}
		if (equal(tok, ">")) {
			node = new_binary(ND_LT, add(&tok, tok -> next), node, start);
			continue;
		}
		if (equal(tok, ">=")) {
			node = new_binary(ND_LE, add(&tok, tok->next), node, start);
			continue;
		}

		*rest = tok;
		return node;
	}
}

static Node *new_add(Node *lhs, Node *rhs, Token *tok) {
	add_type(lhs);
	add_type(rhs);

	if (is_integer(lhs -> ty) && is_integer(rhs -> ty))
		return new_binary(ND_ADD, lhs, rhs, tok);
	if (lhs -> ty -> base && rhs -> ty -> base)
		error_tok(tok, "lhs and rhs both have base");
	if (is_integer(lhs -> ty) && rhs -> ty -> base) {
		Node *tmp = lhs;
		lhs = rhs;
		rhs = tmp;
	}

	// ptr + num
	rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size, tok), tok);
	return new_binary(ND_ADD, lhs, rhs, tok);
}

// pointer - int
static Node *new_sub(Node *lhs, Node *rhs, Token *tok) {
	add_type(lhs);
	add_type(rhs);

	if (is_integer(lhs -> ty) && is_integer(rhs -> ty))
		return new_binary(ND_SUB, lhs, rhs, tok);

	// ptr - num
	if (lhs -> ty -> base && is_integer(rhs -> ty)) {
		rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size, tok), tok);
		add_type(rhs);
		Node *node = new_binary(ND_SUB, lhs , rhs, tok);
		node -> ty = lhs -> ty;
		return node;
	}
	// ptr - ptr
	if (lhs -> ty -> base && rhs -> ty -> base) {
		Node *node = new_binary(ND_SUB, lhs, rhs, tok);
		node -> ty = ty_int;
		return new_binary(ND_DIV, node, new_num(lhs->ty->base->size, tok), tok);
	}
	error_tok(tok, "invalid op in new_sub");
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *tok) {
	Node *node = mul(&tok, tok);

	while (1) {
		Token *start = tok;
		if (equal(tok, "+")) {
			node = new_add(node, mul(&tok, tok -> next), start);
			continue;
		}
		if (equal(tok, "-")) {
			node = new_sub(node, mul(&tok, tok -> next), start);
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
		Token *start = tok;

		if (equal(tok, "*")) {
			node = new_binary(ND_MUL, node, unary(&tok, tok -> next), start);
			continue;
		}
		if (equal(tok ,"/")) {
			node = new_binary(ND_DIV, node, unary(&tok, tok -> next), start);
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
		return new_unary(ND_NEG, unary(rest, tok -> next), tok);
	if (equal(tok, "&"))
		return new_unary(ND_ADDR, unary(rest, tok -> next), tok);
	if (equal(tok, "*"))
		return new_unary(ND_DEREF, unary(rest, tok -> next), tok);
	return postfix(rest, tok);
}

// postfix = primary ("[" expr "]" | "." ident )*
static Node *postfix(Token **rest, Token *tok) {
	Node *node = primary(&tok, tok);

	while (1) {
		if (equal(tok, "[")) {
			Token *start = tok;
			Node *idx = expr(&tok, tok -> next);
			tok = skip(tok, "]");
			node = new_unary(ND_DEREF, new_add(node, idx, start), start);
			continue;
		}
		if (equal(tok, ".")) {
			node = struct_ref(node, tok -> next);
			tok = tok -> next -> next;
			continue;
		}
		*rest = tok;
		return node;
	}
}

// funcall = ident "(" (assign ("," assign)*)? ")"
static Node *funcall(Token **rest, Token *tok) {
	Obj * var = find_var(tok);
	if (var == NULL)
		error_tok(tok, "undefined function");
	if (var -> ty -> kind != TY_FUNC)
		error_tok(tok, "not a function");

	Node *node = new_node(ND_FUNCALL, tok);
	node -> funcname = strndup(tok->loc, tok->len);
	tok = tok -> next -> next;

	Type *ty = var -> ty -> return_ty;
	Node head = {};
	Node *cur = &head;

	while (!equal(tok, ")")) {
		if (cur != &head)
			tok = skip(tok, ",");
		cur = cur -> next = assign(&tok, tok);
		add_type(cur);
	}
	*rest = skip(tok, ")");
	node->args = head.next;
	node -> ty = ty;
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
		if (node -> ty == NULL)
			add_type(node);
		return new_num(node -> ty -> size, tok);
	}

	if (tok -> kind == TK_IDENT) {
		if (equal(tok -> next, "("))
			return funcall(rest, tok);
		Obj *var = find_var(tok);
		if (var == NULL)
			error_tok(tok, "undefined variable");
		*rest = tok -> next;
		return new_var_node(var, tok);
	}
	if (tok -> kind == TK_STR) {
		Obj * var = new_string_literal(tok -> str, tok -> ty);
		*rest = tok -> next;
		return new_var_node(var, tok);
	}
	if (tok -> kind == TK_NUM || tok -> kind == TK_CHAR) {
		node = new_num(tok -> val, tok);
		*rest = tok -> next;
		return node;
	}

	if (tok -> kind == TK_KEYWORD) {
		Type *ty = get_type_can_null(tok);
		if (ty == NULL)
			error_tok(tok, "can't find type");
		node = new_num(ty -> size, tok);
		node -> ty = ty;
		*rest = tok -> next;
		return node;
	}
	error_tok(tok, "expected an expression");
}

static void create_func_params(Type *param) {
	if (param) {
		create_func_params(param->next);
		new_lvar(get_ident(param->decl), param);
	}
}

static Token *func_decl(Token *tok, Type *ty, Obj *fn) {
	locals = NULL;
	enter_scope();

	create_func_params(ty->params);
	fn -> params  = locals;
	fn -> is_definition = true;
	tok = skip(tok, "{");
	fn -> body = compound_stmt(&tok, tok);
	fn -> locals = locals;
	leave_scope();
	return tok;
}

static Token *function(Token *tok, Type *basety) {
	Token *start = tok;
	Obj *var = find_var(tok);
	Type *ty = declarator(&tok, tok, basety);

	if (var == NULL)
	{
		Obj *fn = new_gvar(get_ident(ty -> decl), ty);
		fn -> is_function = true;
		if (consume_if_same(&tok, tok, ";")) {
			fn -> is_definition = false;
			return tok;
		}
		return func_decl(tok, ty, fn);
	}
	if (var -> is_definition == true)
		error_tok(start, "implicit declaration of a function");
	return func_decl(tok, ty, var);
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
