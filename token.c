#include "chibicc.h"

static bool is_ident1(char c) {
	return isalpha(c) || c == '_';
}

static bool is_ident2(char c) {
	return isalnum(c) || c == '_';
}

static int read_punct(char *p) {
	if (startwith(p, "==") || startwith(p, "!=")
			|| startwith(p, "<=") || startwith(p, ">="))
		return 2;
	return ispunct(*p) ? 1 : 0;
}

static int get_numer_from_tok(Token *tok) {
	if (tok -> kind != TK_NUM)
		error("expected a number");
	return tok -> val;
}

static Token *new_token(Tokenkind kind, char *start, char *end) {
	Token *tok = calloc(1, sizeof(Token));
	tok -> kind = kind;
	tok -> loc = start;
	tok -> len = end - start;
	return tok;
}

static bool is_keyword(Token *tok) {
	static char *kw[] = {"return", "if", "else",
		"int", "char", "short", "long", "void",
		"sizeof", "struct"};

	for (int i=0; i< sizeof(kw) / sizeof(*kw); i++)
		if (equal(tok, kw[i]))
			return true;
	return false;
}

static void convert_if_keyword(Token *tok) {
	for (Token *t = tok; t -> kind != TK_EOF; t = t->next)
		if (is_keyword(t))
			t -> kind = TK_KEYWORD;
}

static Token *read_char(char *start) {
	char *p = start + 1;
	char res;
	
	if (*p == '\\' && *(p + 1) != '\'') { // '\n' || '\0'
		p++;
		switch (*p) {
			case 'n':
				res = '\n';
				break;
			case '0' :
				res = '\0';
				break;
			default :
				error ("unkown char");
		}
	}
	else 
		res = *p;
	if (*(++p) != '\'')
		error ("unclosed char");
	Token *tok = new_token(TK_CHAR, start, p + 1);
	tok -> val = res;
	return tok;
}

// unnamed string literal
static Token *read_string_literal(char *start) {
	char *p = start + 1;
	for (; *p != '"'; p++)
		if (*p == '\0')
			error("unclosed string");

	Token *tok = new_token(TK_STR, start, p + 1);
	tok -> ty = array_of(ty_char, p - start);
	tok -> str = strndup(start + 1, p - start - 1);
	return tok;
}

Token *tokenize(char *p) {
	Token head = {};
	Token *cur = &head;
	char *start;
	int punct_len;

	while (*p) {
		if (isspace(*p)) {
			p++;
			continue;
		}
		if (*p == '\'') {
			cur = cur -> next = read_char(p);
			p += cur -> len;
			continue;
		}
		if (*p == '"') {
			cur = cur -> next = read_string_literal(p);
			p += cur -> len;
			continue;
		}
		if (is_ident1(*p)) {
			start = p;
			while (is_ident2(*p))
				p++;
			cur = cur -> next = new_token(TK_IDENT, start, p);
			continue;
		}

		if (isdigit(*p)) {
			cur = cur -> next = new_token(TK_NUM, p, p);
			start = p;
			cur -> val = strtoul(p, &p, 10);
			cur -> len = p - start;
			continue;
		}
		punct_len = read_punct(p);
		if (punct_len > 0) {
			cur = cur -> next = new_token(TK_PUNCT, p, p + punct_len);
			p += punct_len;
			continue;
		}
		error("invalid token");
	}

	cur = cur -> next = new_token(TK_EOF, p, p);
	convert_if_keyword(head.next);
	return head.next;
}


