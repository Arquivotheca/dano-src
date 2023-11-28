/*****************************************************************************/
/*
**	X_Assert.h
**
**	This provides for platform specfic functions that need to be rewitten,
**	but will provide a API that is stable across all platforms
**
**	\xA9 Copyright 1995-1999 Beatnik, Inc, All Rights Reserved.
**	Written by Christopher Schardt
**
**	Beatnik products contain certain trade secrets and confidential and
**	proprietary information of Beatnik.  Use, reproduction, disclosure
**	and distribution by any means are prohibited, except pursuant to
**	a written license from Beatnik. Use of copyright notice is
**	precautionary and does not imply publication or disclosure.
**
**	Restricted Rights Legend:
**	Use, duplication, or disclosure by the Government is subject to
**	restrictions as set forth in subparagraph (c)(1)(ii) of The
**	Rights in Technical Data and Computer Software clause in DFARS
**	252.227-7013 or subparagraphs (c)(1) and (2) of the Commercial
**	Computer Software--Restricted Rights at 48 CFR 52.227-19, as
**	applicable.
**
**	Confidential-- Internal use only
**
** Overview
**	platform-dependent BAE_ASSERT() and BAE_VERIFY() macros
**
**	History	-
**	5/7/99		MOE: created
**	7/13/99		Renamed HAE to BAE
*/
/*****************************************************************************/

#ifndef X_Assert_H
#define X_Assert_H


#include "X_API.h"


#ifndef _DEBUG

	#define BAE_ASSERT(exp)			((void)0)
	#define BAE_VERIFY(exp)			(exp)
#else

	#if (X_PLATFORM == X_WIN95) || (X_PLATFORM == X_WIN_HARDWARE)
		#ifdef ASSERT
			#define BAE_ASSERT(exp)		ASSERT(exp)
			#define BAE_VERIFY(exp)		ASSERT(exp)
		#else
			#include <assert.h>
			#define BAE_ASSERT(exp)		assert(exp)
			#define BAE_VERIFY(exp)		assert(exp)
		#endif
	#else
		#include <assert.h>
		#define BAE_ASSERT(exp)		assert(exp)
		#define BAE_VERIFY(exp)		assert(exp)
	#endif

#endif
	
#define HAE_ASSERT						BAE_ASSERT
#define HAE_VERIFY						BAE_VERIFY

#endif	// X_Assert_H


