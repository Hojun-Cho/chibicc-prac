#include "chibicc.h"

static bool is_ident1(char c)
{
	return isalpha(c) || c == '_';
}

static bool is_ident2(char c)
{
	return isalnum(c) || c == '_';
}

static int read_punct(char *p)
{
	static char *kw[] = {"==",
						 "!=",
						 "<=",
						 ">=",
						 "+=",
						 "-=",
						 "*=",
						 "/="};
	for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++)
	{
		if (startwith(p, kw[i]))
			return strlen(kw[i]);
	}
	return ispunct(*p) ? 1 : 0;
}

static int get_numer_from_tok(Token *tok)
{
	if (tok->kind != TK_NUM)
		error_tok(tok, "expected token kind TK_NUM, actual %d",
				  tok->kind);
	return tok->val;
}

static Token *new_token(Tokenkind kind, char *start, char *end)
{
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->loc = start;
	tok->len = end - start;
	return tok;
}

static bool is_keyword(Token *tok)
{
	static char *kw[] = {"return", "if", "else",
						 "int", "char", "short", "long", "void",
						 "sizeof", "struct"};

	for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++)
		if (equal(tok, kw[i]))
			return true;
	return false;
}

static void convert_if_keyword(Token *tok)
{
	for (Token *t = tok; t->kind != TK_EOF; t = t->next)
		if (is_keyword(t))
			t->kind = TK_KEYWORD;
}

static Token *read_char(char *start)
{
	char *p = start + 1;
	char res;

	if (*p == '\\' && *(p + 1) != '\'')
	{ // '\n' || '\0'
		p++;
		switch (*p)
		{
		case 'n':
			res = '\n';
			break;
		case '0':
			res = '\0';
			break;
		default:
			error_at(p, "unkown char");
		}
	}
	else
		res = *p;
	if (*(++p) != '\'')
		error_at(p, "unclosed char");
	Token *tok = new_token(TK_CHAR, start, p + 1);
	tok->val = res;
	return tok;
}

// unnamed string literal
static Token *read_string_literal(char *start)
{
	char *p = start + 1;
	for (; *p != '"'; p++)
		if (*p == '\0')
			error_at(p, "unclosed string");

	Token *tok = new_token(TK_STR, start, p + 1);
	tok->ty = array_of(ty_char, p - start);
	tok->str = strndup(start + 1, p - start - 1);
	return tok;
}

static void add_line_numbers(Token *tok, char *p)
{
	int n = 1;
	while (*p)
	{
		if (p == tok->loc)
		{
			tok->line_no = n;
			tok = tok->next;
		}
		if (*p == '\n')
			n++;
		p++;
	};
	// eof token
	tok->line_no = n;
}

Token *tokenize(char *p)
{
	char *_p = p;
	set_current_input(p);
	Token head = {};
	Token *cur = &head;
	int punct_len;

	while (*p)
	{
		if (isspace(*p))
		{
			p++;
			continue;
		}
		if (*p == '\'')
		{
			cur = cur->next = read_char(p);
			p += cur->len;
			continue;
		}
		if (*p == '"')
		{
			cur = cur->next = read_string_literal(p);
			p += cur->len;
			continue;
		}
		if (is_ident1(*p))
		{
			char *start = p;
			while (is_ident2(*p))
				p++;
			cur = cur->next = new_token(TK_IDENT, start, p);
			continue;
		}

		if (isdigit(*p))
		{
			cur = cur->next = new_token(TK_NUM, p, p);
			char *start = p;
			cur->val = strtoul(p, &p, 10);
			cur->len = p - start;
			continue;
		}
		punct_len = read_punct(p);
		if (punct_len > 0)
		{
			cur = cur->next = new_token(TK_PUNCT, p, p + punct_len);
			p += punct_len;
			continue;
		}
		error_at(p, "invalid token");
	}
	cur = cur->next = new_token(TK_EOF, p, p);
	add_line_numbers(head.next, _p);
	convert_if_keyword(head.next);
	return head.next;
}
