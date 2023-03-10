#include "chibicc.h"

bool startwith(char *p, char *q)
{
	return strncmp(p, q, strlen(q)) == 0;
}

bool equal(Token *tok, char *op)
{
	return strncmp(tok->loc, op, tok->len) == 0 &&
		   op[tok->len] == '\0';
}

Token *skip(Token *tok, char *s)
{
	if (!equal(tok, s))
	{
		error_tok(tok, "expected token string '%s', actual '%s'",
				  s, tok->loc);
		exit(1);
	}
	return tok->next;
}

bool consume_if_same(Token **rest, Token *tok, char *str)
{
	if (equal(tok, str))
	{
		*rest = tok->next;
		return true;
	}
	*rest = tok;
	return false;
}

char *get_ident(Token *tok)
{
	if (tok->kind != TK_IDENT)
		error_tok(tok, "expected token kind %d, actual %d",
				  TK_IDENT, tok->kind);
	return strndup(tok->loc, tok->len);
}

int get_number(Token *tok)
{
	if (tok->kind != TK_NUM)
		error_tok(tok, "expected token kind %d, actual %d",
				  TK_NUM, tok->kind);
	return tok->val;
}

bool is_type(Token *tok)
{
	return equal(tok, "int") || equal(tok, "char") ||
		   equal(tok, "short") || equal(tok, "long") ||
		   equal(tok, "void") || equal(tok, "struct");
}