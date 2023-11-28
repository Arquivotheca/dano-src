// ===========================================================================
//	JPEGDraw.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include	"JPEG.h"

#include <Bitmap.h>
#include <malloc.h>
#include <stdio.h>
#include <SupportDefs.h>

const int kDCScale = 3;

//====================================================================
//	Allocate memory to store some or all of each component
//

JPEGError SetupDrawing(JPEGDecoder *j)
{
	short	i,s = 3;
	CompSpec	*cspec;
	
	//JPEGMessage(("SetupDrawing"));
	
	if (j->thumbnail) 
		s = 0;
	
	for (i = 0; i < j->CompInFrame; i++) {
		cspec = &j->Comp[i];
				
		//JPEGMessage(("SetupDrawing %dx%d mcu jpeg component %d (%dx%d)",	j->WidthMCU,j->HeightMCU,cspec->Ci,cspec->Hi,cspec->Vi));
		
		if ( j->buffer[i] ) {
			if (j->rowBytes[i] != (cspec->Hi<< s) * j->WidthMCU) {
				printf("Rowbytes changed %ld %d",j->rowBytes[i],(cspec->Hi<< s) * j->WidthMCU);
				return kGenericError;
			}	
		} else {
			j->rowBytes[i] = (cspec->Hi<< s) * j->WidthMCU;
			if (j->SingleRowProc != NULL)
				j->buffer[i] = (uchar *)malloc((cspec->Vi << s) * j->rowBytes[i]);
			else
				j->buffer[i] = (uchar *)malloc((cspec->Vi << s) * j->rowBytes[i] * j->HeightMCU);
			if (j->buffer[i] == 0) {
				printf("JPEG: no memory for decode buffer");
				return kLowMemory;
			}
		}
	}

	return kNoError;
}

//	Cool Saturation code thingy

#ifdef	NON_RISC
	#define	CLIPBYTE(_x)				((_x) < 0 ? 0 : ( (_x) > 255 ? 255 : (_x)) )	
#else
	#define	CLIPBYTE(_x)				(((_x) & (~((_x) >> 9))) | (((ushort)(0xff - (_x))) >> 8))
#endif

//	Draw a block into a component
//  only used by DrawMCU in one place so inline is free 

static inline void	
DrawBlock(uchar *dst, long rowBytes, const short *block)
{
	int	x,y;
	short  a,b,c,d;
	ulong pix;
	
	rowBytes -= 8;
	for (y = kMCUHeight;y--; )  {
		for (x =kMCUWidth/4; x--; ) {
			a = 128 + block[0];
			b = 128 + block[1];
			c = 128 + block[2];
			d = 128 + block[3];
			block += 4;
			pix = CLIPBYTE(a);
			pix <<= 8;
			pix |= CLIPBYTE(b);
			pix <<= 8;
			pix |= CLIPBYTE(c);
			pix <<= 8;
			pix |= CLIPBYTE(d);
			*(long *)dst = LONG(&pix);
			dst += 4;
		}
		dst += rowBytes;
	}
}

//====================================================================
// Draw all components into their buffers

void	DrawMCU(JPEGDecoder *j, const short	*blocks,long h,long v)
{
	long		x,y,i;
	uchar	*dst,*mcu;
	long	rowBytes;			
	long  	mcuRowBytes;
	long 	xs;
	CompSpec *cspec;

	//printf("DrawMCU: %d,%d",h,v);
	
	if ( j->SingleRowProc != NULL) 
		v = 0;				// set v to zero to draw into single line buffer
	if ( j->thumbnail ) {
		
		for (i = 0; i < j->CompInFrame; i++) 
		{
			cspec = &j->Comp[i];
			int dcScale = kDCScale;
			
			dcScale -= (cspec->dcBitPos<<kDCScale);
			if ( dcScale < 0 )
				dcScale = 0;
			dcScale <<= 1;
			rowBytes = j->rowBytes[i];
			xs = cspec->Hi;
			mcu = j->buffer[i] + h * xs;
			if ( v ) 
				mcu += rowBytes * v * cspec->Vi;
			for ( y = cspec->Vi; y--; mcu += rowBytes ) 
			{
				dst = mcu;
				for (x = xs; x--; blocks += kMCUSize ) 
				{
					short q = 128 + (*blocks >> dcScale);
					*dst++ = CLIPBYTE(q);
				}
			}
		}
	} 
	else  {
		h <<= 3;
		v <<= 3;
		
		for (i = 0; i < j->CompInFrame; i++)  {
			cspec = &j->Comp[i];
			rowBytes = j->rowBytes[i];
			mcuRowBytes = rowBytes<<3;
			xs = cspec->Hi;
			mcu = j->buffer[i] + h * xs;
			if ( v )
				rowBytes *= ( v * cspec->Vi);
			for (y = cspec->Vi; y--; mcu += mcuRowBytes ) {
				dst = mcu;
				for (x = xs; x--; dst += kMCUWidth, blocks += kMCUSize)  {
					DrawBlock(dst,rowBytes,blocks);
				}
			}
		}
	}
}

//====================================================================
//	Draw a single component

void
DrawMCUPiece(JPEGDecoder *j, const short	*blocks,long h,long v)
{

	long		x,i;
	uchar		*dst,*mcu;
	long		rowBytes;			
	CompSpec 	*cSpec;

	
	
	h <<= 3;
		
	if ( !j->didClearDrawBuffer )  {
		for (i = 0; i < j->CompInFrame; i++)  {
			rowBytes = j->rowBytes[i];
			mcu = j->buffer[i];
			for ( x=0; x < rowBytes*8; x++ )
				mcu[x] = i == 0 ? 0 : 128;
		}
		j->didClearDrawBuffer = true;
	}
	for (i = 0; i < j->CompInFrame; i++)  {
		cSpec = &j->Comp[i];
		rowBytes = j->rowBytes[i];
		mcu = j->buffer[i];
		if ( cSpec->inScan  )  {
			if (h < 0 || h >= rowBytes)
				return;
			mcu  += h;
			if ( v % cSpec->Vi )
				dst = mcu + rowBytes * 8;
			else
				dst = mcu;
			DrawBlock(dst,rowBytes,blocks);
		}
	}
}
