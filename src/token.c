#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "token.h"
#include "error.h"

Token *new_token(TokenKind kind, Token *cur, char *str)
{
	Token *tok = calloc(1, sizeof(Token));
	if (tok == NULL)
		perror("calloc");
	tok->kind = kind;
	tok->str = str;
	cur->next = tok;
	return (tok);
}

Token	*tokenize(char *p)
{
	Token head = {.next = NULL};
	Token *cur = &head;
	while (*p) {
		if (isspace(*p)) {
			p++;
			continue ;
		}
		if (*p == '+' || *p == '-') {
			cur = new_token(TK_RESERVED, cur, p++);
			continue ;
		}
		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			continue ;
		}
		error("invalid token");
	}
	new_token(TK_EOF, cur, p);
	return head.next;
}
