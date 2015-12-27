extern int __errno ;
#include "string.h"
#include "str_def.h"
#include <limits.h>
/*
*********************************************************************************************************
*                                              Mem_Cmp()
*
* Description : Verifies that ALL data octets in two memory buffers are identical in sequence.
*
* Argument(s) : p1_mem      Pointer to first  memory buffer.
*
*               p2_mem      Pointer to second memory buffer.
*
*               size        Number of data buffer octets to compare (see Note #1).
*
* Return(s)   : DEF_YES, if 'size' number of data octets are identical in both memory buffers.
*
*               DEF_NO,  otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Null compares allowed (i.e. zero-length compares); 'DEF_YES' returned to indicate
*                   identical null compare.
*
*               (2) Many memory buffer comparisons vary ONLY in the least significant octets -- e.g.
*                   network address buffers.  Consequently, memory buffer comparison is more efficient
*                   if the comparison starts from the end of the memory buffers which will abort sooner
*                   on dissimilar memory buffers that vary only in the least significant octets.
*
*               (3) For best CPU performance, optimized to compare data buffers using 'CPU_ALIGN'-sized
*                   data words. Since many word-aligned processors REQUIRE that multi-octet words be accessed on
*                   word-aligned addresses, 'CPU_ALIGN'-sized words MUST be accessed on 'CPU_ALIGN'd
*                   addresses.
*
*               (4) Modulo arithmetic is used to determine whether a memory buffer starts on a 'CPU_ALIGN'
*                   address boundary.
*
*                   Modulo arithmetic in ANSI-C REQUIREs operations performed on integer values.  Thus
*                   address values MUST be cast to an appropriately-sized integer value PRIOR to any
*                  'mem_align_mod' arithmetic operation.
*********************************************************************************************************
*/

unsigned char  memcmp (const  void        *p1_mem,
                      const  void        *p2_mem,
                             unsigned int   size)
{
           CPU_SIZE_T    size_rem;
           CPU_ALIGN    *p1_mem_align;
           CPU_ALIGN    *p2_mem_align;
    const  CPU_INT08U   *p1_mem_08;
    const  CPU_INT08U   *p2_mem_08;
           CPU_DATA      i;
           CPU_DATA      mem_align_mod_1;
           CPU_DATA      mem_align_mod_2;
           CPU_BOOLEAN   mem_aligned;
           CPU_BOOLEAN   mem_cmp;


    if (size < 1) {                                             /* See Note #1.                                         */
        return (0);
    }
    if (p1_mem == (void *)0) {
        return (1);
    }
    if (p2_mem == (void *)0) {
        return (1);
    }


    mem_cmp         =  DEF_YES;                                 /* Assume mem bufs are identical until cmp fails.       */
    size_rem        =  size;
                                                                /* Start @ end of mem bufs (see Note #2).               */
    p1_mem_08       = (const CPU_INT08U *)p1_mem + size;
    p2_mem_08       = (const CPU_INT08U *)p2_mem + size;
                                                                /* See Note #4.                                         */
    mem_align_mod_1 = (CPU_INT08U)((CPU_ADDR)p1_mem_08 % sizeof(CPU_ALIGN));
    mem_align_mod_2 = (CPU_INT08U)((CPU_ADDR)p2_mem_08 % sizeof(CPU_ALIGN));

    mem_aligned     = (mem_align_mod_1 == mem_align_mod_2) ? DEF_YES : DEF_NO;

    if (mem_aligned == DEF_YES) {                               /* If mem bufs' alignment offset equal, ...             */
                                                                /* ... optimize cmp for mem buf alignment.              */
        if (mem_align_mod_1 != 0u) {                            /* If trailing octets avail,                  ...       */
            i = mem_align_mod_1;
            while ((mem_cmp == DEF_YES) &&                      /* ... cmp mem bufs while identical &         ...       */
                   (size_rem > 0)       &&                      /* ... start mem buf cmp with trailing octets ...       */
                   (i        > 0)) {                            /* ... until next CPU_ALIGN word boundary.              */
                p1_mem_08--;
                p2_mem_08--;
                if (*p1_mem_08 != *p2_mem_08) {                 /* If ANY data octet(s) NOT identical, cmp fails.       */
                     mem_cmp = DEF_NO;
                }
                size_rem -= sizeof(CPU_INT08U);
                i--;
            }
        }

        if (mem_cmp == DEF_YES) {                               /* If cmp still identical, cmp aligned mem bufs.        */
            p1_mem_align = (CPU_ALIGN *)p1_mem_08;              /* See Note #3.                                         */
            p2_mem_align = (CPU_ALIGN *)p2_mem_08;

            while ((mem_cmp  == DEF_YES) &&                     /* Cmp mem bufs while identical & ...                   */
                   (size_rem >= sizeof(CPU_ALIGN))) {           /* ... mem bufs aligned on CPU_ALIGN word boundaries.   */
                p1_mem_align--;
                p2_mem_align--;
                if (*p1_mem_align != *p2_mem_align) {           /* If ANY data octet(s) NOT identical, cmp fails.       */
                     mem_cmp = DEF_NO;
                }
                size_rem -= sizeof(CPU_ALIGN);
            }

            p1_mem_08 = (CPU_INT08U *)p1_mem_align;
            p2_mem_08 = (CPU_INT08U *)p2_mem_align;
        }
    }

    while ((mem_cmp == DEF_YES) &&                              /* Cmp mem bufs while identical ...                     */
           (size_rem > 0)) {                                    /* ... for unaligned mem bufs or trailing octets.       */
        p1_mem_08--;
        p2_mem_08--;
        if (*p1_mem_08 != *p2_mem_08) {                         /* If ANY data octet(s) NOT identical, cmp fails.       */
             mem_cmp = DEF_NO;
        }
        size_rem -= sizeof(CPU_INT08U);
    }

    return !(mem_cmp);
}
