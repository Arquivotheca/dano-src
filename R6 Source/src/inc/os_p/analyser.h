/* ++++++++++
	analyser.h
	Copyright (C) 1994 Be Inc.  All Rights Reserved.
	Definitions for debugging with the logic analyser.
+++++ */

#ifndef _ANALYSER_H
#define _ANALYSER_H

#define DEBUG_LOC 0x2000

/* this should never be turned on for production kernels */
#undef ANALYZE 

#if defined(ANALYZE) && !defined(__INTEL__)

#define analyser(offset,value) {*(volatile long*)((DEBUG_LOC)+(offset))=((long)(value));}

#else
#define analyser(offset,value) 
#endif


#endif

