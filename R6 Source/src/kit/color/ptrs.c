/*********************************************************************/
/*
	File:		PTRS.c	@(#)ptrs.c	1.6 1/21/94

	Contains:	This module contains routines to check the validity of
				memory pointers, in order to avoid any GPF's in MS Windows.

				Created by HTR, December 2, 1993

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

		 <1>	12/29/93	HTR		1'st checked in

	Windows Revision Level:

	SCCS Revision:
		@(#)ptrs.c	1.11 6/2/94

	To Do:
*/

/***********************************************************************/

#include	"kcms_sys.h"

/***********************************************************************/

#if defined(KPWIN32) && !defined(KPMSMAC)

bool	Kp_IsBadHugeReadPtr(const void KPHUGE* hPtr, KpUInt32_t nbytes)
{
	return (IsBadHugeReadPtr(hPtr, (UINT) nbytes));
}


bool	Kp_IsBadHugeWritePtr(void KPHUGE* hPtr, KpUInt32_t nbytes)
{
	return (IsBadHugeWritePtr(hPtr, (UINT) nbytes));
}


bool	Kp_IsBadReadPtr(const void FAR* lPtr, KpUInt32_t nbytes)
{
	return (IsBadReadPtr(lPtr, (UINT) nbytes));
}


bool	Kp_IsBadWritePtr(void FAR* lPtr, KpUInt32_t nbytes)
{
	return (IsBadWritePtr(lPtr, (UINT) nbytes));
}


bool	Kp_IsBadCodePtr(OsiProcPtr_t lpfn)
{
	return (IsBadCodePtr(lpfn));
}


bool	Kp_IsBadStringPtr(const void FAR* lPtr, KpUInt32_t nbytes)
{
	return (IsBadStringPtr(lPtr, (UINT) nbytes));
}

#elif defined(KPWIN16)

bool	Kp_IsBadHugeReadPtr(const void KPHUGE* hPtr, KpUInt32_t nbytes)
{
	return (IsBadHugeReadPtr(hPtr, (DWORD) nbytes));
}


bool	Kp_IsBadHugeWritePtr(void KPHUGE* hPtr, KpUInt32_t nbytes)
{
	return (IsBadHugeWritePtr(hPtr, (DWORD) nbytes));
}


bool	Kp_IsBadReadPtr(const void FAR* lPtr, KpUInt32_t nbytes)
{
	return (IsBadReadPtr(lPtr, (UINT) nbytes));
}


bool	Kp_IsBadWritePtr(void FAR* lPtr, KpUInt32_t nbytes)
{
	return (IsBadWritePtr(lPtr, (UINT) nbytes));
}


bool	Kp_IsBadCodePtr(OsiProcPtr_t lpfn)
{
	return (IsBadCodePtr(lpfn));
}


bool	Kp_IsBadStringPtr(const void FAR* lPtr, KpUInt32_t nbytes)
{
	return (IsBadStringPtr(lPtr, (UINT) nbytes));
}

	/* end of _WIN32 */
#else

bool	Kp_IsBadHugeReadPtr(const void KPHUGE* hPtr, KpUInt32_t nbytes)
{
	if (hPtr){}
	if (nbytes){}

	return (False);
}


bool	Kp_IsBadHugeWritePtr(void KPHUGE* hPtr, KpUInt32_t nbytes)
{
	if (hPtr){}
	if (nbytes){}

	return (False);
}


bool	Kp_IsBadReadPtr(const void FAR* lPtr, KpUInt32_t nbytes)
{
	if (lPtr){}
	if (nbytes){}

	return (False);
}


bool	Kp_IsBadWritePtr(void FAR* lPtr, KpUInt32_t nbytes)
{
	if (lPtr){}
	if (nbytes){}

	return (False);
}


bool	Kp_IsBadStringPtr(const void FAR* lPtr, KpUInt32_t nbytes)
{
	if (lPtr){}
	if (nbytes){}

	return (False);
}


bool	Kp_IsBadCodePtr(OsiProcPtr_t lpfn)
{
	if (lpfn){}

	return (False);
}

#endif

