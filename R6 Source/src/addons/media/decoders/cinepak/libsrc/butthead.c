// $Header: u:/rcs/cv/rcs/butthead.c 3.1 1994/05/25 12:37:18 timr Exp $
// $Log: butthead.c $
// Revision 3.1  1994/05/25 12:37:18  timr
// Use MoveMemory instead of CopyMemory where overlap can occur.
//Revision 3.0  1993/12/10  14:23:53  timr
//Initial revision, NT C-only version.
//

#include <windows.h>

/*
;--------------------------------------------------------------------------
;
; void ButtHeadCopyBits(
;	unsigned long srcOffset32,
;	unsigned short srcSelector,
;	long srcYStep,
;	unsigned long dstOffset32,
;	unsigned short dstSelector,
;	long dstYStep,
;	unsigned short width,
;	unsigned short height
; );
;
; This is essentially a BitBlt operation.
;
;   for height scans in source, copy width # bytes to dest
;   no effort is made to align nicely; this works best if source and
;     dest are both dword aligned.
;
;--------------------------------------------------------------------------
*/

void ButtHeadCopyBits (
  unsigned char * src,
  long srcYStep,
  unsigned char * dst,
  long dstYStep,
  unsigned short width,
  unsigned short height
)
{
    while (height--)
    {
	MoveMemory (dst, src, width);
	dst += dstYStep;
	src += srcYStep;
    }
}
