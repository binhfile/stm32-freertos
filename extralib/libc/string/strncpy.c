#include "string.h"
#include "str_def.h"
#include <limits.h>

char *	strncpy(char *dst, const char *src, size_t siz){
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;
	if(!dst || !src) return 0;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return (char*)(s - src - 1);	/* count does not include NUL */
}
