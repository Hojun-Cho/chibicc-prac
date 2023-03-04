#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef enum {
	TK_IDENT,
	TK_PUNCT, // Punctuators
	TK_NUM, // Numberic literals
	TK_KEYWORD,
	TK_CHAR,
	TK_STR,
	TK_EOF,
} Tokenkind;

typedef struct Token Token;
typedef struct Type Type;
typedef struct Field Field;

struct Token{
	Tokenkind kind;
	Token *next;
	int64_t val;
	int len;
	Type *ty;
	char *str;
	
	// for error
	int line_no;
	char *loc;
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
	ND_COMMA, // ,
	ND_ASSIGN,
	ND_NUM, // Integer
	ND_FIELD,
	ND_EXPR_STMT,
	ND_BLOCK, // {}
	ND_RETURN,
	ND_IF,
	ND_ADDR, // &
	ND_DEREF, //*
	ND_FUNCALL,
} Nodekind;

// AST node type
typedef struct Obj Obj;
typedef struct Node Node;

struct Node {
	Nodekind kind;	// Node kind
	Node *body;	// {...}
	Type *ty;
	Token *tok; // for error
	Node*next;
	Node *lhs;	// Left-hand side
	Node *rhs;	// Right-hand side
	Obj *var;
	int64_t val;	// Used if kind == ND_NUM || CHAR
	
	// "if stmt
	Node *cond;
	Node *then;
	Node *_else;

	char *funcname;
	Node *args;

	// struct
	Field *field;
};

struct Obj {
	Obj *next;
	Type *ty;
	char *name;
	bool is_local;

	int offset; // Local var offset
	bool is_function; // func or var  
	bool is_definition;
	char *init_data; // only for global var

	// Function;
	Node *body;
	Obj *params;
	Obj *locals;
	int stack_size;
};

typedef enum {
	TY_INT,
	TY_CHAR,
	TY_SHORT,
	TY_LONG,
	TY_VOID,
	TY_PTR,
	TY_ARRAY,
	TY_FUNC,
	TY_STRUCT,
}Typekind;

struct Type {
	Typekind kind;
	int size;
	Type *base;
	Token *decl;
	int array_len;
	Field *fields;
	Type *return_ty;
	Type *params;
	Type *next;
};

struct Field {
	Field *next;
	Type *ty;
	Token *decl;
	int offset;
};

extern Type *ty_int;
extern Type *ty_char;
extern Type *ty_short;
extern Type *ty_long;
extern Type *ty_void;
void set_current_input(char *p);
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);

bool startwith(char *p, char *q);
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *s);
bool consume_if_same(Token **rest, Token *tok, char *str);
char *get_ident(Token *tok);
int get_number(Token *tok);
bool is_type(Token *tok);

// 
// scope
//
Obj *find_var(Token *tok);
void enter_scope(void);
void leave_scope(void);
Obj *new_var(char *name, Type *ty);
Type *find_tag(Token *tok);
void push_tag_scope(Token *tok, Type *ty);

Token *tokenize(char *p);
Obj *parse(Token *tok);
void code_gen(Obj *prog);
bool is_integer(Type *ty);
void add_type(Node *node);
Type *func_type(Type *return_ty);
Type *pointer_to(Type *base); 
Type *array_of(Type *bse, int size);
Type *get_type_can_null(Token *tok);
