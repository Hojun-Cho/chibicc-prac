#include "chibicc.h"

static int read_punct(char *p) {
	if (startwith(p, "==") || startwith(p, "!=")
			|| startwith(p, "<=") || startwith(p, ">="))
		return 2;
	return ispunct(*p) ? 1 : 0;
}

static int get_number(Token *tok) {
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
	return head.next;
}


