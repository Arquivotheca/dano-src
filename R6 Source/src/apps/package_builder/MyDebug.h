/// MyDebug.h
#ifndef _MYDEBUG_H
#define _MYDEBUG_H


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



#if MEMDEBUG

#include "mkralloc_pub.h"

extern void mem_print_func(void *p,unsigned long nbyte, ulong serial, ulong *trace, long depth);

void* operator new(size_t);
void* operator new(size_t,const char *label);
void  operator delete(void *);

void* operator new(size_t s)
{
	void *p;
	p = malloc(s);
	return p;
}

void* operator new(size_t s, const char *label)
{
	void *p;
	p = malloc(s);
	return p;
}

void operator delete(void *p)
{
	free(p);
}

#endif
#endif
