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

static Token *new_token(TokenKind kind, char *start, char *end)
{
	Token *tok = calloc(1, sizeof(Token));
	if (tok == NULL)
		perror("calloc");
	tok->kind = kind;
	tok->loc = start;
	tok->len = end - start;
	return (tok);
}

static int startswith(char *p, char *q)
{
	return strncmp(p, q, strlen(q)) == 0;
}

static int read_punct(char *p)
{
	static char *kw[] = {
		"+", "-",
	};

	for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++)
		if (startswith(p, kw[i]))
			return strlen(kw[i]);
	return (0);
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
		if (isdigit(p[0])) {
			char *q = p++;
			while (1) {
				if (isalnum(*p)) {
					p++;
				}
				else {
					break;
				}
			}
			cur->next = new_token(TK_NUM, q, p);
			cur = cur->next;
			continue;
		}
		int punct_len = read_punct(p);
		if (punct_len) {
			cur->next = new_token(TK_PUNCT, p, p + punct_len);
			cur = cur->next;
			p += punct_len;
			continue;
		}
		error("invalid token");
	}
	cur->next = new_token(TK_EOF, p, p);
	return head.next;
}

int main()
{
	Token *t;

	{
		char *str = "123";
		t = tokenize(str);
		ASSERT_TRUE(t->loc == str);
		ASSERT_TRUE(t->len == 3);
		t = t->next;
		ASSERT_EQUAL(t->kind, TK_EOF);
		ASSERT_EQUAL(t->loc[0], '\0');
	}
}
