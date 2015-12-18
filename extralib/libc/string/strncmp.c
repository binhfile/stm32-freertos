
#include "string.h"
#include "str_def.h"
#include <limits.h>
extern int __errno ;

int     strncmp(const char *dst, const char *src, size_t siz){
	while(siz > 0){
		if(*dst == '\0' && *src != '\0') return -1;
		else if(*dst != '\0' && *src == '\0') return 1;
		else if(*dst == '\0' && *src == '\0') return 0;
		else if(*dst > *src) return 1;
		else if(*dst < *src) return -1;

		dst++;
		src++;
		siz--;
	}
	return 0;
}
