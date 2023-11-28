#include <Be.h>
#include "MyDebug.h"


#if MEMDEBUG
void mem_print_func(void *p,unsigned long nbyte, ulong serial, ulong *trace, long depth)
{
	printf("Address 0x%lX, %d bytes, color %d, stacktrace ",p,nbyte,serial);
	for (long i = 0; i < depth; i++) {
		printf("0x%lX ",trace[i]);
	}
	printf("\n");
}

#endif
