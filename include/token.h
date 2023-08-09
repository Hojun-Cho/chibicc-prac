#ifndef LIST_H
#define LIST_H

typedef enum
{
	TK_RESERVED,
	TK_NUM,
	TK_EOF,
} TokenKind;

typedef struct Token Token;
struct Token
{
	TokenKind	kind;
	Token	*next;	
	int	val;		// If kind is TK_NUM
	char *str;		// Token string -> point to read form file
};

#endif
