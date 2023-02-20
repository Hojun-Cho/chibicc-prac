#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	TK_PUNCT, // Punctuators
	TK_NUM, // Numberic literals
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
  ND_NUM, // Integer
} Nodekind;

// AST node type
typedef struct Node Node;
struct Node {
  Nodekind kind; // Node kind
  Node *lhs;     // Left-hand side
  Node *rhs;     // Right-hand side
  int val;       // Used if kind == ND_NUM
};



void error(char *fmt, ...);
bool startwith(char *p, char *q);
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *s);

Token *tokenize(char *p);
Node *expr(Token **rest, Token *tok);
void gen_expr(Node *node);
