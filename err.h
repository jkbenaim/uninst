#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if defined(__MINGW32__) || defined(__sgi)

extern void err(int eval, const char *fmt, ...);
extern void errx(int eval, const char *fmt, ...);
extern void warn(const char *fmt, ...);
extern void warnx(const char *fmt, ...);

extern void verr(int eval, const char *fmt, va_list args);
extern void verrx(int eval, const char *fmt, va_list args);
extern void vwarn(const char *fmt, va_list args);
extern void vwarnx(const char *fmt, va_list args);

#else
#include <err.h>
#endif
