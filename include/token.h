#ifndef LIST_H
#define LIST_H

typedef enum
{
	TK_PUNCT, // https://www.ibm.com/docs/en/i/7.3?topic=tokens-punctuators-operators
	TK_NUM,
	TK_EOF,
} TokenKind;

typedef struct Token Token;
struct Token
{
	TokenKind	kind;
	Token	*next;	
	int	val;		// If kind is TK_NUM
	int	len;
	char *loc;		// Token string -> point to read form file
};

#endif
