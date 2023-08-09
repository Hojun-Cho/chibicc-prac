#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "error.h"

void error(const char *fmt, ...)
{
	va_list	ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}
