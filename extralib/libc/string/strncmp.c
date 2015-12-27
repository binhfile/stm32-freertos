
#include "string.h"
#include "str_def.h"
#include <limits.h>

/*
*********************************************************************************************************
*                                             Str_Cmp_N()
*
* Description : Determine if two strings are identical for up to a maximum number of characters.
*
* Argument(s) : p1_str      Pointer to first  string (see Note #1).
*
*               p2_str      Pointer to second string (see Note #1).
*
*               len_max     Maximum number of characters to compare  (see Note  #3d).
*
* Return(s)   : 0,              if strings are identical             (see Notes #3a1A, #3a2A, #3b, & #3d).
*
*               Negative value, if 'p1_str' is less    than 'p2_str' (see Notes #3a1B1, #3a2B1, & #3c).
*
*               Positive value, if 'p1_str' is greater than 'p2_str' (see Notes #3a1B2, #3a2B2, & #3c).
*
*               See also Note #2b.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) String buffers NOT modified.
*
*               (2) (a) IEEE Std 1003.1, 2004 Edition, Section 'strncmp() : DESCRIPTION' states that :
*
*                       (1) "The strncmp() function shall compare ... the array pointed to by 's1' ('p1_str')
*                            to the array pointed to by 's2' ('p2_str)" ...
*                       (2)  but "not more than 'n' ('len_max') bytes" of either array.
*
*                   (b) (1) IEEE Std 1003.1, 2004 Edition, Section 'strncmp() : RETURN VALUE' states that
*                          "upon successful completion, strncmp() shall return an integer greater than,
*                           equal to, or less than 0".
*
*                       (2) IEEE Std 1003.1, 2004 Edition, Section 'strncmp() : DESCRIPTION' adds that
*                          "the sign of a non-zero return value is determined by the sign of the difference
*                           between the values of the first pair of bytes ... that differ in the strings
*                           being compared".
*
*               (3) String comparison terminates when :
*
*                   (a) (1) (A) BOTH string pointer(s) are passed NULL pointers.
*                               (1) NULL strings identical; 0 returned.
*
*                           (B) (1) 'p1_str' passed a NULL pointer.
*                                   (a) Return negative value of character pointed to by 'p2_str'.
*
*                               (2) 'p2_str' passed a NULL pointer.
*                                   (a) Return positive value of character pointed to by 'p1_str'.
*
*                       (2) (A) BOTH strings point to NULL.
*                               (1) Strings overlap with NULL address.
*                               (2) Strings identical up to but NOT beyond or including the NULL address;
*                                   0 returned.
*
*                           (B) (1) 'p1_str_cmp_next' points to NULL.
*                                   (a) 'p1_str' overlaps with NULL address.
*                                   (b) Strings compared up to but NOT beyond or including the NULL address.
*                                   (c) Return negative value of character pointed to by 'p2_str_cmp_next'.
*
*                               (2) 'p2_str_cmp_next' points to NULL.
*                                   (a) 'p2_str' overlaps with NULL address.
*                                   (b) Strings compared up to but NOT beyond or including the NULL address.
*                                   (c) Return positive value of character pointed to by 'p1_str_cmp_next'.
*
*                   (b) Terminating NULL character found in both strings.
*                       (1) Strings identical; 0 returned.
*                       (2) Only one NULL character test required in conditional since previous condition
*                           tested character equality.
*
*                   (c) Non-matching characters found.
*                       (1) Return signed-integer difference of the character pointed to by 'p2_str'
*                           from the character pointed to by 'p1_str'.
*
*                   (d) (1) 'len_max' passed a zero length.
*                           (A) Zero-length strings identical; 0 returned.
*
*                       (2) First 'len_max' number of characters identical.
*                           (A) Strings identical; 0 returned.
*
*                       See also Note #2a2.
*
*               (4) Since 16-bit signed arithmetic is performed to calculate a non-identical comparison
*                   return value, 'CPU_CHAR' native data type size MUST be 8-bit.
*********************************************************************************************************
*/
int     strncmp(const char *p1_str, const char *p2_str, size_t len_max)
{
    const  CPU_CHAR    *p1_str_cmp;
    const  CPU_CHAR    *p2_str_cmp;
    const  CPU_CHAR    *p1_str_cmp_next;
    const  CPU_CHAR    *p2_str_cmp_next;
           CPU_INT16S   cmp_val;
           CPU_SIZE_T   cmp_len;


    if (len_max < 1) {                                          /* If cmp len = 0,        rtn 0       (see Note #3d1A). */
        return (0);
    }

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
           ( cmp_len         <  (      CPU_SIZE_T)len_max)) {   /* ... or max nbr chars cmp'd        (see Note #3d2).   */
        p1_str_cmp++;
        p2_str_cmp++;
        p1_str_cmp_next++;
        p2_str_cmp_next++;
        cmp_len++;
    }


    if (cmp_len == len_max) {                                   /* If strs     identical for max len nbr of chars, ...  */
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
