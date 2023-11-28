#ifndef _NO_INLINE_ASM
#define _NO_INLINE_ASM 1
#endif

#include <TLS.h>

#ifdef __cplusplus
extern "C" {
#endif

#if __INTEL__

void * tls_get(int32 index) {
	void	*ret;
	__asm__ __volatile__ ( 
		"movl	%%fs:(,%%edx, 4), %%eax \n\t"
		: "=a"(ret) : "d"(index) );
	return ret;
}

void ** tls_address(int32 index) {
	void	**ret;
	__asm__ __volatile__ ( 
		"movl	%%fs:0, %%eax \n\t"
		"leal	(%%eax, %%edx, 4), %%eax \n\t"
		: "=a"(ret) : "d"(index) );
	return ret;
}

void 	tls_set(int32 index, void *value) {
	__asm__ __volatile__ ( 
		"movl	%%eax, %%fs:(,%%edx, 4) \n\t"
		: : "d"(index), "a"(value) );
}

#endif

#ifdef __cplusplus
}
#endif
