#include "chibicc.h"

void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

bool startwith(char *p, char *q) {
	return strncmp(p, q, strlen(q)) == 0;
}

bool equal(Token *tok, char *op) {
	return strncmp(tok -> loc, op, tok -> len) == 0
		&& op[tok -> len] == '\0';
}

Token *skip(Token *tok, char *s) {
  if (!equal(tok, s)) {
    fprintf(stderr, "expected '%s'", s);
  	exit(1);
  }
  return tok->next;
}

bool consume_if_same(Token **rest, Token *tok, char *str) {
	if (equal(tok, str)) {
		*rest = tok -> next;
		return true;
	}
	*rest = tok;
	return false;
}
