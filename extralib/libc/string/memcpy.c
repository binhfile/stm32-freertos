extern int __errno ;
#include "string.h"
#include "str_def.h"
#include <limits.h>

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

