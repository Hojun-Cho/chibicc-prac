#include "chibicc.h"

// for struct tags
typedef struct TagScope TagScope;
struct TagScope {
	TagScope *next;
	char *name;
	Type *ty;
};


typedef struct VarScope VarScope;
struct VarScope {
	VarScope *next;
	Obj *var;
};

typedef struct Scope Scope;
struct Scope {
	Scope *next;
	VarScope *vars;
	TagScope *tags;
};

static Scope *scope = &(Scope){};

Obj *find_var(Token *tok) {
	for (Scope *sc = scope; sc; sc = sc -> next)
		for (VarScope *v_sc = sc -> vars; v_sc; v_sc = v_sc -> next)
			if (equal(tok, v_sc -> var -> name))
				return v_sc->var;
	return NULL;
}

void enter_scope(void) {
	Scope *sc = calloc(1, sizeof(Scope));
	sc->next = scope;
	scope = sc;
}

static VarScope *push_var_to_scope(Obj *var) {
	VarScope *sc = calloc(1, sizeof(VarScope));
	sc -> var = var;
	sc -> next = scope -> vars;
	scope -> vars = sc;
	return sc;
}

void leave_scope(void) {
	scope = scope->next;
}

Obj *new_var(char *name, Type *ty) {
	Obj *var = calloc(1, sizeof(Obj));
	var -> name = name;
	var -> ty = ty;
	push_var_to_scope(var);	// push to current scope;
	return var;
}

Type *find_tag(Token *tok) {
	for (Scope *sc = scope; sc; sc = sc->next)
		for (TagScope *tsc = sc->tags; tsc; tsc = tsc->next)
			if (equal(tok, tsc->name))
				return tsc->ty;
	return NULL;
}

void push_tag_scope(Token *tok, Type *ty) {
	TagScope *sc = calloc(1, sizeof(TagScope));
	sc->name = strndup(tok->loc, tok->len);
	sc->ty = ty;
	sc->next = scope->tags;
	scope->tags = sc;
}
