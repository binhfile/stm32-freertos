#include "string.h"
#include "str_def.h"
#include <limits.h>

/*
*********************************************************************************************************
*                                            Str_Copy_N()
*
* Description : Copy source string to destination string buffer, up to a maximum number of characters.
*
* Argument(s) : pstr_dest   Pointer to destination string buffer to receive source string copy   (see Note #1a).
*
*               pstr_src    Pointer to source      string to copy into destination string buffer (see Note #1b).
*
*               len_max     Maximum number of characters  to copy (see Notes #2a2 & #3d).
*
* Return(s)   : Pointer to destination string, if NO error(s) [see Note #2b1].
*
*               Pointer to NULL,               otherwise      (see Note #2b2A).
*
* Caller(s)   : Application.
*
* Note(s)     : (1) (a) Destination buffer size NOT validated; buffer overruns MUST be prevented by caller.
*
*                       (1) Destination buffer size MUST be large enough to accommodate the entire source
*                           string size including the terminating NULL character.
*
*                   (b) Source string buffer NOT modified.
*
*               (2) (a) (1) IEEE Std 1003.1, 2004 Edition, Section 'strncpy() : DESCRIPTION' states that :
*
*                           (A) "The strncpy() function shall copy ... the array pointed to by 's2'
*                               ('pstr_src') to the array pointed to by 's1' ('pstr_dest')"; ...
*                           (B)  but "not more than 'n' ('len_max') bytes"                   ...
*                           (C)  &   "(bytes that follow a null byte are not copied)".
*
*                       (2) (A) IEEE Std 1003.1, 2004 Edition, Section 'strncpy() : DESCRIPTION' adds that
*                              "if the array pointed to by 's2' ('pstr_src') is a string that is shorter
*                               than 'n' ('len_max') bytes, null bytes shall be appended to the copy in
*                               the array pointed to by 's1' ('pstr_dest'), until 'n' ('len_max') bytes
*                               in all are written."
*
*                               (1) #### Since Str_Copy() limits the maximum number of characters to copy
*                                   via Str_Copy_N() by the CPU's maximum number of addressable characters,
*                                   this requirement is intentionally NOT implemented to avoid appending
*                                   a potentially large number of unnecessary terminating NULL characters.
*
*                           (B) IEEE Std 1003.1, 2004 Edition, Section 'strncpy() : APPLICATION USAGE' also
*                               states that "if there is no null byte in the first 'n' ('len_max') bytes of
*                               the array pointed to by 's2' ('pstr_src'), the result is not null-terminated".
*
*                   (b) IEEE Std 1003.1, 2004 Edition, Section 'strncpy() : RETURN VALUE' states that :
*
*                       (1) "The strncpy() function shall return 's1' ('pstr_dest');" ...
*                       (2) "no return value is reserved to indicate an error."
*                           (A) #### This requirement is intentionally ignored in order to return NULL
*                               for any error(s).
*
*                   (c) IEEE Std 1003.1, 2004 Edition, Section 'strncpy() : DESCRIPTION' states that "if
*                       copying takes place between objects that overlap, the behavior is undefined".
*
*               (3) String copy terminates when :
*
*                   (a) Destination/Source string pointer(s) are passed NULL pointers.
*                       (1) No string copy performed; NULL pointer returned.
*
*                   (b) Destination/Source string pointer(s) point to NULL.
*                       (1) String buffer(s) overlap with NULL address; NULL pointer returned.
*
*                   (c) Source string's terminating NULL character found.
*                       (1) Entire source string copied into destination string buffer (see Note #2a1A).
*
*                   (d) 'len_max' number of characters copied.
*                       (1) 'len_max' number of characters MAY include the terminating NULL character
*                           (see Note #2a1C).
*                       (2) Null copies allowed (i.e. zero-length copies).
*                           (A) No string copy performed; destination string returned  (see Note #2b1).
*********************************************************************************************************
*/
char *	strncpy(char *pstr_dest, const char *pstr_src, size_t len_max)
{
           CPU_CHAR    *pstr_copy_dest;
    const  CPU_CHAR    *pstr_copy_src;
           CPU_SIZE_T   len_copy;

                                                                /* Rtn NULL if str ptr(s) NULL (see Note #3a1).         */
    if (pstr_dest == (CPU_CHAR *)0) {
        return ((CPU_CHAR *)0);
    }
    if (pstr_src  == (const CPU_CHAR *)0) {
        return ((CPU_CHAR *)0);
    }


    pstr_copy_dest = pstr_dest;
    pstr_copy_src  = pstr_src;
    len_copy       = 0u;

    while (( pstr_copy_dest != (      CPU_CHAR *)  0 ) &&       /* Copy str until NULL ptr(s)  [see Note #3b]  ...      */
           ( pstr_copy_src  != (const CPU_CHAR *)  0 ) &&
           (*pstr_copy_src  != (      CPU_CHAR  )'\0') &&       /* ... or NULL char found      (see Note #3c); ...      */
           ( len_copy       <  (      CPU_SIZE_T)len_max)) {    /* ... or max nbr chars copied (see Note #3d).          */
       *pstr_copy_dest = *pstr_copy_src;
        pstr_copy_dest++;
        pstr_copy_src++;
        len_copy++;
    }
                                                                /* Rtn NULL if NULL ptr(s) found  (see Note #3b1).      */
    if ((pstr_copy_dest == (      CPU_CHAR *)0) ||
        (pstr_copy_src  == (const CPU_CHAR *)0)) {
         return ((CPU_CHAR *)0);
    }

    if (len_copy < len_max) {                                   /* If  copy str len < max buf len (see Note #2a2A), ... */
       *pstr_copy_dest = (CPU_CHAR)'\0';                        /* ... copy NULL char  (see Note #3c1).                 */
    }


    return (pstr_dest);                                         /* Rtn ptr to dest str (see Note #2b1).                 */
}
