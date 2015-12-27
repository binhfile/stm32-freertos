#ifndef STDIO_H__
#define STDIO_H__
#include "lib_defines.h"
#include <stdarg.h>


int 	vsnprintf(char *str, size_t size, const char *format, va_list args);
int 	snprintf(char *str, size_t size, const char *format, ...);
#endif
