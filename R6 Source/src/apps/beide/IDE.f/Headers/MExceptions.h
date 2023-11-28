//========================================================================
//	MExceptions.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MEXCEPTIONS_H
#define _MEXCEPTIONS_H


#if DEBUG
#define Throw_(err) throw(err)
#else
#define Throw_(err) throw(err)
#endif

#define ThrowIfError_(err)					\
	do {									\
		long	__theErr = err;				\
		if (__theErr != B_NO_ERROR){		\
			Throw_(__theErr);				\
		}									\
	} while (false)
	
#endif
