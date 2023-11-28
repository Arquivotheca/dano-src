/*
 * $Header: u:/rcs/cv/rcs/w16_32.h 1.5 1993/09/23 17:23:07 geoffs Exp $
 * $Log: w16_32.h $
 * Revision 1.5  1993/09/23 17:23:07  geoffs
 * Now correctly processing status callback during compression
 * Revision 1.4  1993/07/06  16:50:54  geoffs
 * Based,far #defines, dword bdry padding, etc...
 * 
 * Revision 1.3  93/07/03  11:44:53  geoffs
 * Fixes needed to compile WIN16
 * 
 * Revision 1.2  93/07/02  17:22:28  geoffs
 * Added more typedefs for WIN16 portions
 * 
 * Revision 1.1  93/07/02  16:21:48  geoffs
 * Initial revision
 * 
 */

//
// Reconcile differences between WIN16 and WIN32/NT environments
//
#if	!defined(W16_32_DEFINED)
#if	!defined(WIN32)

#ifndef _TCHAR_DEFINED
typedef char 			TCHAR,*PTCHAR;
//typedef unsigned char 		TBYTE,*PTBYTE;
#define _TCHAR_DEFINED
#endif /* !_TCHAR_DEFINED */

#ifndef	_INC_WINDOWS
typedef	char far		*LPSTR;
typedef	const char far		*LPCSTR;
#endif /* !_INC_WINDOWS */

typedef LPSTR			LPTCH;
typedef	LPSTR			PTCH;
typedef LPSTR			PTSTR;
typedef LPSTR			LPTSTR;
typedef LPCSTR			LPCTSTR;
#define	TEXT(a)			a

#define	APIENTRY	FAR PASCAL _loadds

#define BASED_SELF		__based((__segment) __self)

#else	/* WIN32 */

#if	defined(GlobalAllocPtr)
#undef	GlobalAllocPtr
#define	GlobalAllocPtr(a,b)	LocalAlloc(LPTR,(b))
#endif

#if	defined(GlobalFreePtr)
#undef	GlobalFreePtr
#define	GlobalFreePtr(a)	LocalFree((a))
#endif

#define	GlobalFix(a)
#define	GlobalUnfix(a)

#ifndef __BEOS__
#define	__based(a)
#endif
#define	BASED_SELF
#define	_loadds
#define _LOADDS
#define	_far

#ifdef	far
#undef	far
#endif
#ifndef	_H2INC
#define	far
#endif

#define	_huge
#define	huge
#define	HUGE
#define GWW_ID			GWL_ID

#ifndef	setjmp
#define	setjmp	_setjmp
#endif

#ifndef	cktypeDIBcompressed
#define cktypeDIBcompressed     aviTWOCC('d','c')
#endif

#endif

#define	NUMCHARS(a)		(sizeof((a)) / sizeof(TCHAR))

#define	W16_32_DEFINED

#endif
