#pragma once
#include <stdarg.h>
#include "progname.h"
#include "stdnoreturn.h"

extern noreturn void vtryhelp(const char *fmt, va_list args);
extern noreturn void tryhelp(const char *fmt, ...);
