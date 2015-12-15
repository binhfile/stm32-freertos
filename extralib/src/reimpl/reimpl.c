/*
 * reimpl.c
 *
 *  Created on: Dec 13, 2015
 *      Author: dev
 */


int __errno = 0;
#define CPU_CHAR	char
#define CPU_INT16S	short
#define CPU_SIZE_T	unsigned int
#define CPU_ALIGN	unsigned int
#define CPU_INT08U	unsigned char
#define CPU_DATA	unsigned int
#define CPU_BOOLEAN	unsigned char
#define CPU_ADDR	unsigned int
#define DEF_YES		1
#define DEF_NO		0
#define DEF_ENABLED	1
#define DEF_DISABLED 0
#define LIB_MEM_CFG_ARG_CHK_EXT_EN DEF_ENABLED

#define STR_MAX_LEN	4096
#include <string.h>
#include <limits.h>

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

void memcpy(void *pdest, const void *psrc, size_t size){
		CPU_SIZE_T    size_rem;
		CPU_SIZE_T    mem_gap_octets;
		CPU_ALIGN    *pmem_align_dest;
	const  CPU_ALIGN    *pmem_align_src;
		CPU_INT08U   *pmem_08_dest;
	const  CPU_INT08U   *pmem_08_src;
		CPU_DATA      i;
		CPU_DATA      mem_align_mod_dest;
		CPU_DATA      mem_align_mod_src;
		CPU_BOOLEAN   mem_aligned;


	#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
	if (size < 1) {                                             /* See Note #1.                                         */
	 return;
	}
	if (pdest == (void *)0) {
	 return;
	}
	if (psrc  == (void *)0) {
	 return;
	}
	#endif


	size_rem           =  size;

	pmem_08_dest       = (      CPU_INT08U *)pdest;
	pmem_08_src        = (const CPU_INT08U *)psrc;

	mem_gap_octets     = pmem_08_src - pmem_08_dest;


	if (mem_gap_octets >= sizeof(CPU_ALIGN)) {                  /* Avoid bufs overlap.                                  */
															 /* See Note #4.                                         */
	 mem_align_mod_dest = (CPU_INT08U)((CPU_ADDR)pmem_08_dest % sizeof(CPU_ALIGN));
	 mem_align_mod_src  = (CPU_INT08U)((CPU_ADDR)pmem_08_src  % sizeof(CPU_ALIGN));

	 mem_aligned        = (mem_align_mod_dest == mem_align_mod_src) ? DEF_YES : DEF_NO;

	 if (mem_aligned == DEF_YES) {                           /* If mem bufs' alignment offset equal, ...             */
															 /* ... optimize copy for mem buf alignment.             */
		 if (mem_align_mod_dest != 0u) {                     /* If leading octets avail,                   ...       */
			 i = mem_align_mod_dest;
			 while ((size_rem   >  0) &&                     /* ... start mem buf copy with leading octets ...       */
					(i          <  sizeof(CPU_ALIGN ))) {    /* ... until next CPU_ALIGN word boundary.              */
				*pmem_08_dest++ = *pmem_08_src++;
				 size_rem      -=  sizeof(CPU_INT08U);
				 i++;
			 }
		 }

		 pmem_align_dest = (      CPU_ALIGN *)pmem_08_dest;  /* See Note #3.                                         */
		 pmem_align_src  = (const CPU_ALIGN *)pmem_08_src;
		 while (size_rem      >=  sizeof(CPU_ALIGN)) {       /* While mem bufs aligned on CPU_ALIGN word boundaries, */
			*pmem_align_dest++ = *pmem_align_src++;          /* ... copy psrc to pdest with CPU_ALIGN-sized words.   */
			 size_rem         -=  sizeof(CPU_ALIGN);
		 }

		 pmem_08_dest = (      CPU_INT08U *)pmem_align_dest;
		 pmem_08_src  = (const CPU_INT08U *)pmem_align_src;
	 }
	}

	while (size_rem > 0) {                                      /* For unaligned mem bufs or trailing octets, ...       */
	*pmem_08_dest++ = *pmem_08_src++;                        /* ... copy psrc to pdest by octets.                    */
	 size_rem      -=  sizeof(CPU_INT08U);
	}
}
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
void *	memset(void *s, int val, size_t count){
	char* p = (char*)s;
	while(count > 0){
		*p = val;
		p++;
		count --;
	}
	return s;
}
static inline int
isupper(char c)
{
    return (c >= 'A' && c <= 'Z');
}

static inline int
isalpha(char c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}


static inline int
isspace(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\12');
}

static inline int
isdigit(char c)
{
    return (c >= '0' && c <= '9');
}
/*
 * Convert a string to a long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
long
strtol(nptr, endptr, base)
	const char *nptr;
	char **endptr;
	register int base;
{
	register const char *s = nptr;
	register unsigned long acc;
	register int c;
	register unsigned long cutoff;
	register int neg = 0, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	} else if ((base == 0 || base == 2) &&
	    c == '0' && (*s == 'b' || *s == 'B')) {
		c = s[1];
		s += 2;
		base = 2;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long)base;
	cutoff /= (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if ((any < 0 || acc > cutoff || acc == cutoff) && c > cutlim)
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LONG_MIN : LONG_MAX;
//		errno = ERANGE;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *)(any ? s - 1 : nptr);
	return (acc);
}

