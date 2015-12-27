/* 
 * File:   lib_string.h
 * Author: dev
 *
 * Created on October 19, 2015, 1:53 PM
 */

#ifndef LIB_STRING_H
#define	LIB_STRING_H
#include "lib_defines.h"
#ifdef	__cplusplus
extern "C" {
#endif
#include <stdarg.h>

int 	strcmp(const char *p1_str, const char *p2_str);
int     strncmp(const char *s1, const char *s2, size_t count);

char *	strncpy(char *dest, const char *src, size_t count);

size_t  strlen(const char *s);
size_t  strnlen(const char *pstr, int len_max);

char *	strchr(const char *s, int c);
char *	strchrnul(const char *str, int ch);

size_t  strcspn(const char *s, const char *reject);

char *strstr(const char *pstr, const char *pstr_srch);
char *strnstr(const char *pstr, const char *pstr_srch, int len_max);

void *	memset(void *s, int val, size_t count);
void    memcpy(void *pdest, const void *psrc, size_t size);
unsigned char  memcmp (const void *p1_mem, const void *p2_mem, unsigned int size);

#ifdef	__cplusplus
}
#endif

#endif	/* LIB_STRING_H */

