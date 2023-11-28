/*
 * TSIMEM.C
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Sampo Kaasila
 *
 * This software is the property of Bitstream Inc. and it is furnished
 * under a license and may be used and copied only in accordance with the
 * terms of such license and with the inclusion of the above copyright notice.
 * This software or any other copies thereof may not be provided or otherwise
 * made available to any other person or entity except as allowed under license.
 * No title to and no ownership of the software or intellectual property
 * contained herein is hereby transferred. This information in this software
 * is subject to change without notice
 */

#include "syshead.h"


#include "dtypes.h"
#include "tsimem.h"
#include "config.h"


#define MAGIC1 0xab1500ff
#define MAGIC2 0xa5a55a5a
#define MAGIC3 0xaa53C5aa
#define MAGIC4 0x5a
#define MAGIC5 0xf0

/* This is *SLOW*. Only define if you are debugging or testing. */
/* #define ZAP_MEMORY */

void tsi_Error( tsiMemObject *t, int errcode )
{
	t->state = T2K_STATE_DEAD;
	longjmp( t->env, errcode );
}

tsiMemObject *tsi_NewMemhandler( int *errCode )
{
	tsiMemObject *t;
	register long i;
	
	assert( errCode != NULL );
	*errCode = 0;
	t = (tsiMemObject *) CLIENT_MALLOC( sizeof( tsiMemObject ) );
	if ( t != NULL ) {
		t->stamp1 = MAGIC1;
		t->state  = T2K_STATE_ALIVE;
		t->stamp2 = MAGIC2;
		
		t->numPointers = 0;
		t->maxPointers = 256 + 17;
		t->maxPointers = 501; /* prime number */

		t->ii = 0;
		t->base = (void **) CLIENT_MALLOC( sizeof( void *) * t->maxPointers );
		
		if ( t->base != NULL ) {
			for ( i = 0; i < t->maxPointers; i++ ) {
				t->base[i] = NULL;
			}
		} else {
			CLIENT_FREE( t ); t = NULL;
			*errCode = T2K_ERR_MEM_MALLOC_FAILED;
		}
	} else {
		*errCode = T2K_ERR_MEM_MALLOC_FAILED;
	}
	
	if (t != NULL)
	{
#ifdef TRACK_RAM
		t->totRAM = (long)(sizeof( tsiMemObject ) + sizeof( void *) * t->maxPointers);
		t->maxRAM = t->totRAM;
#endif
		for ( i = 0; i < T2K_MAX_FAST_BLOCKS; i++ ) {
			t->fast_base[i] = NULL; t->fast_size[i] = 0; t->fast_free[i] = true;
		}
	}
	return t; /*****/
}

void *tsi_FastAllocN(  register tsiMemObject *t, size_t size, int N )
{
	register void *p;
	
	assert( N < T2K_MAX_FAST_BLOCKS );
	if ( t->fast_free[N] ) {
		t->fast_free[N] = false;
		if ( size > t->fast_size[N] ) {
			tsi_DeAllocMem( t, t->fast_base[N] );
			t->fast_base[N]	= tsi_AllocMem( t, size );
			t->fast_size[N]	= size;
		} 
		p = t->fast_base[N];
	} else {
		p = tsi_AllocMem( t, size ); /* Already taken */
	}
	return p; /*****/
}

void tsi_FreeFastMemBlocks( tsiMemObject *t )
{
	register int i;
	for ( i = 0; i < T2K_MAX_FAST_BLOCKS; i++ ) {
		assert( t->fast_free[i] );
		tsi_DeAllocMem( t, t->fast_base[i] ); t->fast_base[i] = NULL; t->fast_size[i] = 0; t->fast_free[i] = true;
	}
}


void tsi_DeleteMemhandler( tsiMemObject *t )
{
#ifdef OLD
	tsi_Assert( t, t->stamp1 == MAGIC1 && t->stamp2 == MAGIC2 , T2K_ERR_BAD_MEM_STAMP );
	tsi_Assert( t, t->numPointers == 0, T2K_ERR_MEM_LEAK ); /* Check for dangling pointers */
#endif
	assert( t->stamp1 == MAGIC1 && t->stamp2 == MAGIC2 );
	tsi_FreeFastMemBlocks( t );
	assert( t->numPointers == 0 ); /* Check for dangling pointers */
	
#ifdef TRACK_RAM
	t->totRAM -= (sizeof( tsiMemObject ) + sizeof( void *) * t->maxPointers);
	printf("********************\n" );
	printf("t->totRAM = %d\n", t->totRAM );
	printf("t->maxRAM = %d\n", t->maxRAM );
	printf("********************\n" );
#endif
	CLIENT_FREE( t->base );
	CLIENT_FREE( t );
}

void tsi_EmergencyShutDown( tsiMemObject *t )
{
	if ( t != NULL ) {
		register long i, maxPointers = t->maxPointers;
		register void **base = t->base;
		for ( i = 0; i < maxPointers; i++ ) {
			if ( base[i] != NULL ) {
				CLIENT_FREE( base[i] );
			}
		}
		CLIENT_FREE( base );
		CLIENT_FREE( t );
	}
} 

/* MEMORY LAYOUT:
 *
 * unsigned long:	MAGIC3
 * unsigned long:	n
 * byte[]		:	n bytes of allocated data
 *
 * byte:			MAGIC4
 * byte:			MAGIC5
 *
 */
#if defined(OS400)
/* Here all structures and pointers to align be aligned on 16-byte boundaries. */
#define headerSize ((((3*sizeof(unsigned long) ))/16+1)*16)
#else
/* Make this headerSize the default for everybody. */
#define headerSize ((((3*sizeof(unsigned long) ))/16+1)*16)
/* #define headerSize ( 3*sizeof(unsigned long) ) */
#endif

#define tailSize (sizeof(char) + sizeof(char))


#ifdef BIG_TEST
void tsi_ValidatePointer( register tsiMemObject *t, void *pIn );
void  tsi_ValidatePointer( register tsiMemObject *t, void *pIn )
{
	int err;
	char *p = (char *) pIn;
	unsigned long size;
	register unsigned long *plong;
	
	tsi_Assert( t, t != NULL, T2K_ERR_NULL_MEM );

	p -= headerSize;
	plong = (unsigned long *)p;
	
	err = ( plong[0] != MAGIC3 );
	if ( err == 0 ) {
		size = plong[1];
		err |= ( ((unsigned char *)p)[headerSize + size] 		!= MAGIC4 );
		err |= ( ((unsigned char *)p)[headerSize + size + 1] 	!= MAGIC5 );
	}
	tsi_Assert( t, err == 0, T2K_ERR_MEM_INVALID_PTR );
}

void  tsi_ValidateMemory( register tsiMemObject *t )
{
	register long i, maxPointers;
	register void **base;
	
	tsi_Assert( t, t != NULL, T2K_ERR_NULL_MEM );

	
	base = t->base;
	maxPointers = t->maxPointers;
	for ( i = 0; i < maxPointers; i++ ) {
		if ( base[i] != NULL ) {
			tsi_ValidatePointer( t, (char *)base[i] + headerSize );
		}
	}
}
#endif



/*
 * Description:		Allocates a chunk of memory, and returns a pointer to it.
 * How used:		Just call with the size in bytes.
 * Side Effects: 	None.
 * Return value: 	A pointer to the memory.
 */
void *tsi_AllocMem( register tsiMemObject *t, size_t size )
{
	register long i, maxPointers;
	register unsigned char *p;
	register unsigned long *plong;
	register void **base;
	
#ifdef BIG_TEST
	tsi_ValidateMemory( t );
#endif
	tsi_Assert( t, t != NULL, T2K_ERR_NULL_MEM );
	/* 	tsi_ValidateMemory( t ); */

	/* tsi_Assert( t, size >=  0, T2K_ERR_NEG_MEM_REQUEST ); */
	p = (unsigned char *)CLIENT_MALLOC( headerSize + size + tailSize );
	tsi_Assert( t, p != NULL, T2K_ERR_MEM_MALLOC_FAILED );

#ifdef TRACK_RAM
	t->totRAM += headerSize + size + tailSize;
	if ( t->totRAM > t->maxRAM ) t->maxRAM = t->totRAM;
#endif
	
	plong = (unsigned long *)p;
	
	plong[0] = MAGIC3;
	plong[1] = size;
	p[headerSize + size]	= (unsigned char)MAGIC4;
	p[headerSize + size+1]	= (unsigned char)MAGIC5;
	
	tsi_Assert( t, t->numPointers < t->maxPointers, T2K_ERR_MEM_TOO_MANY_PTRS );
	
	base = t->base;
	maxPointers = t->maxPointers;

	{
		register unsigned long index; /*  = (unsigned long)((unsigned long)p ^ (unsigned long)size) % maxPointers; */
		
		index = t->ii;
		for ( i = 0; i < maxPointers; i++ ) {
			if ( base[index] == NULL ) {
				base[index] = p;
				plong[2] = index;
				t->numPointers++;
				break; /*****/
			}
			index = (index + 1 ) % maxPointers;
		}
		t->ii = index;
	}

	tsi_Assert( t, i < maxPointers, T2K_ERR_MEM_BAD_LOGIC );
	
	#ifdef ZAP_MEMORY
	{
		assert( false ); /* Just so that ZAP_MEMORY is not left on accidentally */
		for ( i = 0; i < size; i++ )  {
			((char *)p)[i+headerSize] = 0x5a;
		}

	}
	#endif
	
	return (p+headerSize); /*****/
}


/*
 * Description:		reallocs the memory the pointer "p" points at.
 * How used:		Call with the pointer to the memory, received from ag_AllocMem.
 *					It is OK to call this with a NULL pointer.
 * Side Effects: 	None.
 * Return value: 	None.
 */
void *tsi_ReAllocMem( register tsiMemObject *t, void *pIn, size_t size2 )
{
	register long i, maxPointers;
	register void **base;
	register unsigned long *plong;
	unsigned char *p = (unsigned char *) pIn;
	unsigned long size1;
	
#ifdef BIG_TEST
	tsi_ValidateMemory( t );
#endif
	if ( p != NULL ) {
		p -= headerSize;
		plong = (unsigned long *)p;
		
		tsi_Assert( t, plong[0] == MAGIC3, T2K_ERR_BAD_MEM_STAMP );

		size1 = plong[1];
		tsi_Assert( t, ((unsigned char *)p)[headerSize + size1] 		== MAGIC4, T2K_ERR_BAD_MEM_STAMP );
		tsi_Assert( t, ((unsigned char *)p)[headerSize + size1 + 1] 	== MAGIC5, T2K_ERR_BAD_MEM_STAMP );
		
		base = t->base;
		maxPointers = t->maxPointers;
		tsi_Assert( t, t->numPointers > 0 && t->numPointers <= maxPointers, T2K_ERR_BAD_PTR_COUNT );
		/* for ( i = 0; i < maxPointers; i++ ) { */
		i = (long)plong[2];
		assert( base[i] == p );
		base[i] = CLIENT_REALLOC( p, headerSize + size2 + tailSize );
#ifdef TRACK_RAM
t->totRAM -= size1;
t->totRAM += size2;
if ( t->totRAM > t->maxRAM ) t->maxRAM = t->totRAM;
#endif
		p = (unsigned char *) base[i];
		tsi_Assert( t, p != NULL, T2K_ERR_MEM_REALLOC_FAILED );

		plong = (unsigned long *)p;
		tsi_Assert( t, plong[0] == MAGIC3, T2K_ERR_BAD_MEM_STAMP );
		plong[1] = size2;
		/* plong[2] = (unsigned long)i; */
		p[headerSize + size2]	= (unsigned char)MAGIC4;
		p[headerSize + size2+1]	= (unsigned char)MAGIC5;
		tsi_Assert( t, i < t->maxPointers, T2K_ERR_MEM_BAD_PTR );
		return (p+headerSize); /*****/
	}
	return NULL; /*****/
}


/*
 * Description:		Free the memory the pointer "p" points at.
 * How used:		Call with the pointer to the memory, received from ag_AllocMem.
 *					It is OK to call this with a NULL pointer.
 * Side Effects: 	None.
 * Return value: 	None.
 */
void tsi_DeAllocMem( register tsiMemObject *t, void *pIn )
{
	register void **base;
	register unsigned long *plong;
	char *p = (char *) pIn;
	unsigned long size, index;

#ifdef BIG_TEST
	assert( false );
	tsi_ValidateMemory( t );
#endif
	assert(t != NULL);
	if ( p != NULL ) {
		p -= headerSize;
		plong = (unsigned long *)p;
		
		tsi_Assert( t, plong[0] == MAGIC3, T2K_ERR_BAD_MEM_STAMP );

		size = plong[1];
		tsi_Assert( t, ((unsigned char *)p)[headerSize + size] 		== MAGIC4, T2K_ERR_BAD_MEM_STAMP );
		tsi_Assert( t, ((unsigned char *)p)[headerSize + size + 1] 	== MAGIC5, T2K_ERR_BAD_MEM_STAMP );

		#ifdef ZAP_MEMORY
		{
			long i;
			for ( i = 0; i < size; i++ )  {
				((char *)pIn)[i] = 0xa5;
			}

		}
		#endif
		
		base = t->base;
		
		index = plong[2];
			
		tsi_Assert( t, base[index] == p, T2K_ERR_MEM_BAD_PTR );

		base[index] = NULL;
		t->numPointers--;
			
		CLIENT_FREE(p);
#ifdef TRACK_RAM
	t->totRAM -= (headerSize+size+tailSize);
#endif
	}
}

/*
void tsi_Assert( register tsiMemObject *t, int cond, int errcode  )
{
	if ( !cond ) {
		longjmp( t->env, errcode );
	}
}
*/



/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/tsimem.c 1.7 2000/10/18 17:08:41 reggers Exp $
 *                                                                           *
 *     $Log: tsimem.c $
 *     Revision 1.7  2000/10/18 17:08:41  reggers
 *     Downgraded a tsi_Assert() in tsi_DeAllocMem to assert() because
 *     if it ever tripped an error, a crash was immanent in the longjmp()
 *     caused by the very error condition detected!
 *     Revision 1.6  2000/10/06 15:49:40  reggers
 *     Ensure memory object is not evaluated unless it exists in tsi_NewMemhandler.
 *     Revision 1.5  2000/02/25 17:46:25  reggers
 *     STRICT warning cleanup.
 *     Revision 1.4  1999/10/18 17:02:49  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.3  1999/09/30 15:12:38  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:58:48  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

