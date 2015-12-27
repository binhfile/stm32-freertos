#include "string.h"
#include "str_def.h"
#include <limits.h>
#include <string.h>
/*
*********************************************************************************************************
*                                              Str_Str()
*
* Description : Search string for first occurence of a specific search string.
*
* Argument(s) : pstr            Pointer to        string (see Note #1).
*
*               pstr_srch       Pointer to search string (see Note #1).
*
* Return(s)   : Pointer to first occurrence of search string in string, if any                (see Note #2b1A).
*
*               Pointer to string,                                      if NULL search string (see Note #2b2).
*
*               Pointer to NULL,                                        otherwise             (see Note #2b1B).
*
* Caller(s)   : Application.
*
* Note(s)     : (1) String buffers NOT modified.
*
*               (2) (a) IEEE Std 1003.1, 2004 Edition, Section 'strstr() : DESCRIPTION' states that :
*
*                       (1) "The strstr() function shall locate the first occurrence  in the string
*                            pointed to by 's1' ('pstr') of the sequence of bytes ... in the string
*                            pointed to by 's2' ('pstr_srch')" ...
*                       (2) "(excluding the terminating null byte)."
*
*                   (b) IEEE Std 1003.1, 2004 Edition, Section 'strstr() : RETURN VALUE' states that :
*
*                       (1) "Upon successful completion, strstr() shall return" :
*                           (A) "a pointer to the located string" ...
*                           (B) "or a null pointer if the string is not found."
*                               (1) #### Although NO strstr() specification states to return NULL for
*                                   any other reason(s), NULL is also returned for any error(s).
*
*                       (2) "If 's2' ('pstr_srch') points to a string with zero length, the function
*                            shall return 's1' ('pstr')."
*
*               (3) String search terminates when :
*
*                   (a) String pointer(s) are passed NULL pointers.
*                       (1) No string search performed; NULL pointer returned.
*
*                   (b) String pointer(s) point to NULL.
*                       (1) String buffer(s) overlap with NULL address; NULL pointer returned.
*
*                   (c) Search string length equal to zero.
*                       (1) No string search performed; string pointer returned (see Note #2b2).
*
*                   (d) Search string length greater than string length.
*                       (1) No string search performed; NULL   pointer returned (see Note #2b1B).
*
*                   (e) Entire string has been searched.
*                       (1) Search string not found; NULL pointer returned (see Note #2b1B).
*
*                   (f) Search string found.
*                       (1) Return pointer to first occurrence of search string in string (see Note #2b1A).
*********************************************************************************************************
*/
char *strstr(const char *pstr, const char *pstr_srch)
{
    CPU_CHAR  *pstr_rtn;


    pstr_rtn = strnstr(pstr,
                         pstr_srch,
						 STR_MAX_LEN);

    return (pstr_rtn);
}


/*
*********************************************************************************************************
*                                             Str_Str_N()
*
* Description : Search string for first occurence of a specific search string, up to a maximum number
*                   of characters.
*
* Argument(s) : pstr            Pointer to        string (see Note #1).
*
*               pstr_srch       Pointer to search string (see Note #1).
*
*               len_max         Maximum number of characters to search (see Note #3g).
*
* Return(s)   : Pointer to first occurrence of search string in string, if any                (see Note #2b1A).
*
*               Pointer to string,                                      if NULL search string (see Note #2b2).
*
*               Pointer to NULL,                                        otherwise             (see Note #2b1B).
*
* Caller(s)   : Application.
*
* Note(s)     : (1) String buffers NOT modified.
*
*               (2) (a) IEEE Std 1003.1, 2004 Edition, Section 'strstr() : DESCRIPTION' states that :
*
*                       (1) "The strstr() function shall locate the first occurrence  in the string
*                            pointed to by 's1' ('pstr') of the sequence of bytes ... in the string
*                            pointed to by 's2' ('pstr_srch')" ...
*                       (2) "(excluding the terminating null byte)."
*
*                   (b) IEEE Std 1003.1, 2004 Edition, Section 'strstr() : RETURN VALUE' states that :
*
*                       (1) "Upon successful completion, strstr() shall return" :
*                           (A) "a pointer to the located string" ...
*                           (B) "or a null pointer if the string is not found."
*                               (1) #### Although NO strstr() specification states to return NULL for
*                                   any other reason(s), NULL is also returned for any error(s).
*
*                       (2) "If 's2' ('pstr_srch') points to a string with zero length, the function
*                            shall return 's1' ('pstr')."
*
*               (3) String search terminates when :
*
*                   (a) String pointer(s) are passed NULL pointers.
*                       (1) No string search performed; NULL pointer returned.
*
*                   (b) String pointer(s) point to NULL.
*                       (1) String buffer(s) overlap with NULL address; NULL pointer returned.
*
*                   (c) Search string length equal to zero.
*                       (1) No string search performed; string pointer returned (see Note #2b2).
*
*                   (d) Search string length greater than string length.
*                       (1) No string search performed; NULL   pointer returned (see Note #2b1B).
*
*                   (e) Entire string has been searched.
*                       (1) Search string not found; NULL pointer returned (see Note #2b1B).
*                       (2) Maximum size of the search is defined as the subtraction of the
*                           search string length from the string length.
*
*                   (f) Search string found.
*                       (1) Return pointer to first occurrence of search string in string (see Note #2b1A).
*                       (2) Search string found via Str_Cmp_N().
*
*                   (g) 'len_max' number of characters searched.
*                       (1) 'len_max' number of characters does NOT include terminating NULL character
*                           (see Note #2a2).
*********************************************************************************************************
*/
char *strnstr(const char *pstr, const char *pstr_srch, int len_max)
{
           CPU_SIZE_T    str_len;
           CPU_SIZE_T    str_len_srch;
           CPU_SIZE_T    len_max_srch;
           CPU_SIZE_T    srch_len;
           CPU_SIZE_T    srch_ix;
           CPU_BOOLEAN   srch_done;
           CPU_INT16S    srch_cmp;
    const  CPU_CHAR     *pstr_str;
    const  CPU_CHAR     *pstr_srch_ix;

                                                                /* Rtn NULL if str ptr(s) NULL (see Note #3a).          */
    if (pstr == (const CPU_CHAR *)0) {
        return ((CPU_CHAR *)0);
    }
    if (pstr_srch == (const CPU_CHAR *)0) {
        return ((CPU_CHAR *)0);
    }

    if (len_max < 1) {                                          /* Rtn NULL if srch len = 0    (see Note #3g).          */
        return ((CPU_CHAR *)0);
    }

                                                                /* Lim max srch str len (to chk > str len).             */
    len_max_srch = (len_max <       STR_MAX_LEN)
                 ? (len_max + 1u) : STR_MAX_LEN;

    str_len      = strnlen(pstr,      len_max);
    str_len_srch = strnlen(pstr_srch, len_max_srch);
    if (str_len_srch < 1) {                                     /* Rtn ptr to str if srch str len = 0 (see Note #2b2).  */
        return ((CPU_CHAR *)pstr);
    }
    if (str_len_srch > str_len) {                               /* Rtn NULL if srch str len > str len (see Note #3d).   */
        return ((CPU_CHAR *)0);
    }
                                                                /* Rtn NULL if NULL ptr found         (see Note #3b1).  */
    pstr_str = pstr      + str_len;
    if (pstr_str == (const CPU_CHAR *)0) {
        return ((CPU_CHAR *)0);
    }
    pstr_str = pstr_srch + str_len_srch;
    if (pstr_str == (const CPU_CHAR *)0) {
        return ((CPU_CHAR *)0);
    }


    srch_len  = str_len - str_len_srch;                         /* Calc srch len (see Note #3e2).                       */
    srch_ix   = 0u;
    srch_done = DEF_NO;

    do {
        pstr_srch_ix = (const CPU_CHAR *)(pstr + srch_ix);
        srch_cmp     =  strncmp(pstr_srch_ix, pstr_srch, str_len_srch);
        srch_done    = (srch_cmp == 0) ? DEF_YES : DEF_NO;
        srch_ix++;
    } while ((srch_done == DEF_NO) && (srch_ix <= srch_len));


    if (srch_cmp != 0) {                                        /* Rtn NULL if srch str NOT found (see Note #3e2).      */
        return ((CPU_CHAR *)0);
    }

    return ((CPU_CHAR *)pstr_srch_ix);                          /* Else rtn ptr to found srch str (see Note #3f1).      */
}
