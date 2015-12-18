#include "string.h"
#include "str_def.h"
#include <limits.h>
extern int __errno ;
int strcmp(const char *p1_str, const char *p2_str){
    const  CPU_CHAR    *p1_str_cmp;
    const  CPU_CHAR    *p2_str_cmp;
    const  CPU_CHAR    *p1_str_cmp_next;
    const  CPU_CHAR    *p2_str_cmp_next;
           CPU_INT16S   cmp_val;
           CPU_SIZE_T   cmp_len;

    if (p1_str == (const CPU_CHAR *)0) {
        if (p2_str == (const CPU_CHAR *)0) {
            return (0);                                         /* If BOTH str ptrs NULL, rtn 0       (see Note #3a1A). */
        }
        cmp_val = (CPU_INT16S)((CPU_INT16S)0 - (CPU_INT16S)(*p2_str));
        return (cmp_val);                                       /* If p1_str NULL, rtn neg p2_str val (see Note #3a1B1).*/
    }
    if (p2_str == (const CPU_CHAR *)0) {
        cmp_val = (CPU_INT16S)(*p1_str);
        return (cmp_val);                                       /* If p2_str NULL, rtn pos p1_str val (see Note #3a1B2).*/
    }


    p1_str_cmp      = p1_str;
    p2_str_cmp      = p2_str;
    p1_str_cmp_next = p1_str_cmp;
    p2_str_cmp_next = p2_str_cmp;
    p1_str_cmp_next++;
    p2_str_cmp_next++;
    cmp_len         = 0u;

    while ((*p1_str_cmp      == *p2_str_cmp)            &&      /* Cmp strs until non-matching chars (see Note #3c) ... */
           (*p1_str_cmp      != (      CPU_CHAR  )'\0') &&      /* ... or NULL chars                 (see Note #3b) ... */
           ( p1_str_cmp_next != (const CPU_CHAR *)  0 ) &&      /* ... or NULL ptr(s) found          (see Note #3a2).   */
           ( p2_str_cmp_next != (const CPU_CHAR *)  0 ) &&
           ( cmp_len         <  (      CPU_SIZE_T)STR_MAX_LEN)) {   /* ... or max nbr chars cmp'd        (see Note #3d2).   */
        p1_str_cmp++;
        p2_str_cmp++;
        p1_str_cmp_next++;
        p2_str_cmp_next++;
        cmp_len++;
    }


    if (cmp_len == STR_MAX_LEN) {                                   /* If strs     identical for max len nbr of chars, ...  */
        return (0);                                             /* ... rtn 0                 (see Note #3d2A).          */
    }

    if (*p1_str_cmp != *p2_str_cmp) {                           /* If strs NOT identical, ...                           */
                                                                /* ... calc & rtn char diff  (see Note #3c1).           */
         cmp_val = (CPU_INT16S)((CPU_INT16S)(*p1_str_cmp) - (CPU_INT16S)(*p2_str_cmp));

    } else if (*p1_str_cmp  == (CPU_CHAR)'\0') {                /* If NULL char(s) found, ...                           */
         cmp_val = (CPU_INT16S)0;                               /* ... strs identical; rtn 0 (see Note #3b).            */

    } else {
        if (p1_str_cmp_next == (const CPU_CHAR *)0) {
            if (p2_str_cmp_next == (const CPU_CHAR *)0) {       /* If BOTH next str ptrs NULL, ...                      */
                cmp_val = (CPU_INT16S)0;                        /* ... rtn 0                       (see Note #3a2A).    */
            } else {                                            /* If p1_str_cmp_next NULL, ...                         */
                                                                /* ... rtn neg p2_str_cmp_next val (see Note #3a2B1).   */
                cmp_val = (CPU_INT16S)((CPU_INT16S)0 - (CPU_INT16S)(*p2_str_cmp_next));
            }
        } else {                                                /* If p2_str_cmp_next NULL, ...                         */
            cmp_val = (CPU_INT16S)(*p1_str_cmp_next);           /* ... rtn pos p1_str_cmp_next val (see Note #3a2B2).   */
        }
    }


    return (cmp_val);
}
