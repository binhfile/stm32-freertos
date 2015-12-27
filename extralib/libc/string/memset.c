#include "string.h"
#include "str_def.h"
#include <limits.h>

void *	memset(void *pmem, int data_val, size_t size)
{
    CPU_SIZE_T   size_rem;
    CPU_ALIGN    data_align;
    CPU_ALIGN   *pmem_align;
    CPU_INT08U  *pmem_08;
    CPU_DATA     mem_align_mod;
    CPU_DATA     i;


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (size < 1) {                                             /* See Note #1.                                         */
        return pmem;
    }
    if (pmem == (void *)0) {
        return pmem;
    }
#endif


    data_align = 0u;
    for (i = 0u; i < sizeof(CPU_ALIGN); i++) {                  /* Fill each data_align octet with data val.            */
        data_align <<=  DEF_OCTET_NBR_BITS;
        data_align  |= (CPU_ALIGN)data_val;
    }

    size_rem      =  size;
    mem_align_mod = (CPU_INT08U)((CPU_ADDR)pmem % sizeof(CPU_ALIGN));   /* See Note #3.                                 */

    pmem_08 = (CPU_INT08U *)pmem;
    if (mem_align_mod != 0u) {                                  /* If leading octets avail,                   ...       */
        i = mem_align_mod;
        while ((size_rem > 0) &&                                /* ... start mem buf fill with leading octets ...       */
               (i        < sizeof(CPU_ALIGN ))) {               /* ... until next CPU_ALIGN word boundary.              */
           *pmem_08++ = data_val;
            size_rem -= sizeof(CPU_INT08U);
            i++;
        }
    }

    pmem_align = (CPU_ALIGN *)pmem_08;                          /* See Note #2.                                         */
    while (size_rem >= sizeof(CPU_ALIGN)) {                     /* While mem buf aligned on CPU_ALIGN word boundaries,  */
       *pmem_align++ = data_align;                              /* ... fill mem buf with    CPU_ALIGN-sized data.       */
        size_rem    -= sizeof(CPU_ALIGN);
    }

    pmem_08 = (CPU_INT08U *)pmem_align;
    while (size_rem > 0) {                                      /* Finish mem buf fill with trailing octets.            */
       *pmem_08++   = data_val;
        size_rem   -= sizeof(CPU_INT08U);
    }
    return pmem;
}
