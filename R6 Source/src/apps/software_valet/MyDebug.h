/// MyDebug.h

#ifndef _MYDEBUG_H_
#define _MYDEBUG_H_

#define _dr9_
// main debug switch
#define DEBUG 0
#define MEMDEBUG 0

#if (!DEBUG)
	#if (MODULE_DEBUG)
		#undef DEBUG
		#define DEBUG 1
			#include<Debug.h> // module debugging on
		#undef DEBUG
		#define DEBUG 0
	#else
		#include <Debug.h>	// debugging off
	#endif
#else
	#include <Debug.h> // debugging on
#endif



#if DEBUG

extern void 	mlabel( void *p, const char *label);
extern void 	mdump( void (*f)(void *,uint n, char *label));

#if MEMDEBUG
void* operator new(size_t);
void  operator delete(void *);

void* operator new(size_t s)
{
	return malloc(s);
}

void operator delete(void *p)
{
	free(p);
}

#endif
#define label_object(p) mlabel((void *)(p),(const char *)(class_name((p))));

#else

	#define label_object(p) void(0)
#endif


#endif 
