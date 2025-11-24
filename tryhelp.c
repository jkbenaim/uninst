#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "progname.h"
#include "tryhelp.h"

noreturn void vtryhelp(const char *fmt, va_list args)
{
	if (fmt && *fmt) {
		fprintf(stderr, "%s: ", __progname);
		vfprintf(stderr, fmt, args);
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "%s: use the -h option for usage information\n", __progname);
	exit(EXIT_FAILURE);
}

noreturn void tryhelp(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vtryhelp(fmt, ap);
	va_end(ap);
}
