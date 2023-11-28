/******************************************************************************
/
/	File:			BeBuild.h
/
/	Description:	Import/export macros
/
/	Copyright 1993-98, Be Incorporated
/
*******************************************************************************/

#ifndef _BE_BUILD_H
#define _BE_BUILD_H

#include <ProductFeatures.h>

#define B_BEOS_VERSION_4		0x0400
#define B_BEOS_VERSION_4_5		0x0450
#define B_BEOS_VERSION_5		0x0500
#define B_BEOS_VERSION_5_0_4	0x0504
#define B_BEOS_VERSION_DANO		0x0510

#define B_BEOS_VERSION_MAUI 	B_BEOS_VERSION_5

#define B_BEOS_VERSION			B_BEOS_VERSION_DANO

/* Originally, it wasn't possible to unset _R5_COMPATIBLE_, so make the
 * default behaviour the same.
*/
#ifndef _R5_COMPATIBLE_
#define _R5_COMPATIBLE_ 1
#endif

#if _R5_COMPATIBLE_
#if defined(__powerc) || defined(powerc)
	#define _PR2_COMPATIBLE_ 1
	#define _PR3_COMPATIBLE_ 1
	#define _R4_COMPATIBLE_ 1
	#define _R4_5_COMPATIBLE_ 1
	#define _R5_COMPATIBLE_ 1
	#define _R5_0_4_COMPATIBLE_ 1
#else
	#define _PR2_COMPATIBLE_ 0
	#define _PR3_COMPATIBLE_ 0
	#define _R4_COMPATIBLE_ 1
	#define _R4_5_COMPATIBLE_ 1
	#define _R5_COMPATIBLE_ 1
	#define _R5_0_4_COMPATIBLE_ 1
#endif
#else
	#define _PR2_COMPATIBLE_ 0
	#define _PR3_COMPATIBLE_ 0
	#define _R4_COMPATIBLE_ 0
	#define _R4_5_COMPATIBLE_ 0
	#define _R5_COMPATIBLE_ 0
	#define _R5_0_4_COMPATIBLE_ 0
#endif

#if __GNUC__
#define _UNUSED(x) x
#define _PACKED	__attribute__((packed))
#endif

#define	_EXPORT
#define	_IMPORT

#endif
