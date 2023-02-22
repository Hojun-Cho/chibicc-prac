#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	TK_IDENT,
	TK_PUNCT, // Punctuators
	TK_NUM, // Numberic literals
	TK_KEYWORD,
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

typedef enum {
	ND_ADD, // +
	ND_SUB, // -
	ND_MUL, // *
	ND_DIV, // /
	ND_NEG, // unary -
	ND_EQ,  // ==
	ND_NE,  // !=
	ND_LT,  // <
	ND_LE,  // <=
	ND_VAR,
	ND_ASSIGN,
	ND_NUM, // Integer
	ND_EXPR_STMT,
	ND_BLOCK, // {}
	ND_RETURN,
	ND_IF,
	ND_ADDR, // &
	ND_DEREF, //*
} Nodekind;

// AST node type
typedef struct Obj Obj;
typedef struct Node Node;
typedef struct Function Function;
typedef struct Type Type;

struct Node {
	Nodekind kind;	// Node kind
	Node *body;	// {...}
	Type *ty;
	Node*next;
	Node *lhs;	// Left-hand side
	Node *rhs;	// Right-hand side
	Obj *var;
	int val;	// Used if kind == ND_NUM
	
	// "if stmt
	Node *cond;
	Node *then;
	Node *_else;

};

struct Obj {
	Obj *next;
	Type *ty;
	char *name;
	int offset;
};

struct Function {
	Node *body;
	Obj *locals;
	int stack_size;
};

typedef enum {
	TY_INT,
	TY_PTR,
}Typekind;

struct Type {
	Typekind kind;
	Type *base;
	Token *decl;
};

extern Type *ty_int;

void error(char *fmt, ...);
bool startwith(char *p, char *q);
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *s);
bool consume_if_same(Token **rest, Token *tok, char *str);

Token *tokenize(char *p);
Function *parse(Token *tok);
void code_gen(Function *prog);
bool is_integer(Type *ty);
void add_type(Node *node);
Type *pointer_to(Type *base); 
