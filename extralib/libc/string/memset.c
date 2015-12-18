#include "string.h"
#include "str_def.h"
#include <limits.h>

void *	memset(void *s, int val, size_t count){
	char* p = (char*)s;
	while(count > 0){
		*p = val;
		p++;
		count --;
	}
	return s;
}
