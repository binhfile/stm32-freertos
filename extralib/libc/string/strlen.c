extern int __errno ;
#include "string.h"
#include "str_def.h"
#include <limits.h>

/*
*********************************************************************************************************
*                                              Str_Len()
*
* Description : Calculate length of a string.
*
* Argument(s) : pstr        Pointer to string (see Note #1).
*
* Return(s)   : Length of string; number of characters in string before terminating NULL character
*                   (see Note #2b1).
*
* Caller(s)   : Application.
*
* Note(s)     : (1) String buffer NOT modified.
*
*               (2) (a) IEEE Std 1003.1, 2004 Edition, Section 'strlen() : DESCRIPTION' states that :
*
*                       (1) "The strlen() function shall compute the number of bytes in the string to
*                            which 's' ('pstr') points," ...
*                       (2) "not including the terminating null byte."
*
*                   (b) IEEE Std 1003.1, 2004 Edition, Section 'strlen() : RETURN VALUE' states that :
*
*                       (1) "The strlen() function shall return the length of 's' ('pstr');" ...
*                       (2) "no return value shall be reserved to indicate an error."
*
*               (3) String length calculation terminates when :
*
*                   (a) String pointer points to NULL.
*                       (1) String buffer overlaps with NULL address.
*                       (2) String length calculated for string up to but NOT beyond or including
*                           the NULL address.
*
*                   (b) Terminating NULL character found.
*                       (1) String length calculated for string up to but NOT           including
*                           the NULL character (see Note #2a2).
*********************************************************************************************************
*/
size_t  strlen(const char *pstr)
{
    CPU_SIZE_T  len;


    len = strnlen(pstr, STR_MAX_LEN);

    return (len);
}


/*
*********************************************************************************************************
*                                             Str_Len_N()
*
* Description : Calculate length of a string, up to a maximum number of characters.
*
* Argument(s) : pstr        Pointer to string (see Note #1).
*
*               len_max     Maximum number of characters to search (see Note #3c).
*
* Return(s)   : Length of string; number of characters in string before terminating NULL character,
*                   if terminating NULL character     found (see Note #2b1).
*
*               Requested maximum number of characters to search,
*                   if terminating NULL character NOT found (see Note #3c).
*
* Caller(s)   : Application.
*
* Note(s)     : (1) String buffer NOT modified.
*
*               (2) (a) IEEE Std 1003.1, 2004 Edition, Section 'strlen() : DESCRIPTION' states that :
*
*                       (1) "The strlen() function shall compute the number of bytes in the string to
*                            which 's' ('pstr') points," ...
*                       (2) "not including the terminating null byte."
*
*                   (b) IEEE Std 1003.1, 2004 Edition, Section 'strlen() : RETURN VALUE' states that :
*
*                       (1) "The strlen() function shall return the length of 's' ('pstr');" ...
*                       (2) "no return value shall be reserved to indicate an error."
*
*               (3) String length calculation terminates when :
*
*                   (a) String pointer points to NULL.
*                       (1) String buffer overlaps with NULL address.
*                       (2) String length calculated for string up to but NOT beyond or including
*                           the NULL address.
*
*                   (b) Terminating NULL character found.
*                       (1) String length calculated for string up to but NOT           including
*                           the NULL character (see Note #2a2).
*
*                   (c) 'len_max' number of characters searched.
*                       (1) 'len_max' number of characters does NOT include the terminating NULL character.
*********************************************************************************************************
*/
size_t  strnlen(const char *pstr, int len_max)
{
    const  CPU_CHAR    *pstr_len;
           CPU_SIZE_T   len;


    pstr_len = pstr;
    len      = 0u;
    while (( pstr_len != (const CPU_CHAR *)  0 ) &&             /* Calc str len until NULL ptr (see Note #3a) ...       */
           (*pstr_len != (      CPU_CHAR  )'\0') &&             /* ... or NULL char found      (see Note #3b) ...       */
           ( len      <  (      CPU_SIZE_T)len_max)) {          /* ... or max nbr chars srch'd (see Note #3c).          */
        pstr_len++;
        len++;
    }

    return (len);                                               /* Rtn str len (see Note #3b1).                         */
}
