extern int __errno ;
#include "string.h"
#include "str_def.h"
#include <limits.h>

size_t  strlen(const char *pstr)
{
    const  CPU_CHAR    *pstr_len;
           CPU_SIZE_T   len;


    pstr_len = pstr;
    len      = 0u;
    while (( pstr_len != (const CPU_CHAR *)  0 ) &&             /* Calc str len until NULL ptr (see Note #3a) ...       */
           (*pstr_len != (      CPU_CHAR  )'\0') &&             /* ... or NULL char found      (see Note #3b) ...       */
           ( len      <  (      CPU_SIZE_T)STR_MAX_LEN)) {          /* ... or max nbr chars srch'd (see Note #3c).          */
        pstr_len++;
        len++;
    }

    return (len);                                               /* Rtn str len (see Note #3b1).                         */
}
