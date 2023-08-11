#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "token.h"
#include "error.h"

#define ASSERT_EQUAL(a, b)	\
		if (a != b) {		\
			printf("%s: %s : %d : Assert Fail\n", __FILE__, __func__, __LINE__);\
			exit(1);		\
		}					

#define ASSERT_TRUE(a)	\
		if (a == 0) {		\
			printf("%s: %s : %d : Assert Fail\n", __FILE__, __func__, __LINE__);\
			exit(1);		\
		}					
		
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
			continue;
		}
		if (((*p == '+' || *p == '-') && isdigit(p[1]))
			|| isdigit(p[0])) {
		    cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			continue ;
		}
		if ((*p == '+' || *p == '-')) {
			cur = new_token(TK_RESERVED, cur, p++);
			continue;
		}
		error("invalid token");
	}
	cur = new_token(TK_EOF, cur, p);
	return head.next;
}

int main()
{
	Token *t;

	{
		t = tokenize("+");
		ASSERT_EQUAL(t->kind, TK_RESERVED);
		ASSERT_EQUAL(t->str[0], '+');
		t = t->next;
		ASSERT_EQUAL(t->kind, TK_EOF);
		ASSERT_EQUAL(t->str[0], '\0');
	}

	{
		t = tokenize("123");
		ASSERT_EQUAL(t->kind, TK_NUM);
		ASSERT_EQUAL(t->val, 123);
		t = t->next;
		ASSERT_EQUAL(t->kind, TK_EOF);
		ASSERT_EQUAL(t->str[0], '\0');
	}
	
	{
		t = tokenize("+123");
		ASSERT_EQUAL(t->kind, TK_NUM);
		ASSERT_EQUAL(t->val, 123);
		t = t->next;
		ASSERT_EQUAL(t->kind, TK_EOF);
		ASSERT_EQUAL(t->str[0], '\0');
	}
	
	{
		t = tokenize("-123");
		ASSERT_EQUAL(t->kind, TK_NUM);
		ASSERT_EQUAL(t->val, -123);
		t = t->next;
		ASSERT_EQUAL(t->kind, TK_EOF);
		ASSERT_EQUAL(t->str[0], '\0');
	}


	{
		t = tokenize("1 + 3");
		ASSERT_EQUAL(t->kind, TK_NUM);
		ASSERT_EQUAL(t->val, 1);
		t = t->next;
		ASSERT_EQUAL(t->kind, TK_RESERVED);
		ASSERT_TRUE(strncmp(t->str, "+", 1) == 0);
		t = t->next;
		ASSERT_EQUAL(t->kind, TK_NUM);
		ASSERT_EQUAL(t->val, 3);
		t = t->next;
		ASSERT_EQUAL(t->kind, TK_EOF);
		ASSERT_EQUAL(t->str[0], '\0');
	}
	
	{
		t = tokenize("+1 + -3");
		ASSERT_EQUAL(t->kind, TK_NUM);
		ASSERT_EQUAL(t->val, 1);
		t = t->next;
		ASSERT_EQUAL(t->kind, TK_RESERVED);
		ASSERT_TRUE(strncmp(t->str, "+", 1) == 0);
		t = t->next;
		ASSERT_EQUAL(t->kind, TK_NUM);
		ASSERT_EQUAL(t->val, -3);
		t = t->next;
		ASSERT_EQUAL(t->kind, TK_EOF);
		ASSERT_EQUAL(t->str[0], '\0');
	}

}
