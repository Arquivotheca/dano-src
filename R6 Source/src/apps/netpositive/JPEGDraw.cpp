// ===========================================================================
//	JPEGDraw.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include	"JPEG.h"
#include "MessageWindow.h"

#include <malloc.h>

const int	kDCScale = 3;

long LONG(void *l)
{
	        uchar *b = (uchar *)l;
			return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}     


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
			if ( IsError(j->rowBytes[i] != (cspec->Hi<< s) * j->WidthMCU) ) {
				pprint("Rowbytes changed %ld %d",j->rowBytes[i],(cspec->Hi<< s) * j->WidthMCU);
				return kGenericError;
			}	
		} else {
			j->rowBytes[i] = (cspec->Hi<< s) * j->WidthMCU;
			if (j->SingleRowProc != NULL)
				j->buffer[i] = (uchar *)malloc((cspec->Vi << s) * j->rowBytes[i]);
			else
				j->buffer[i] = (uchar *)malloc((cspec->Vi << s) * j->rowBytes[i] * j->HeightMCU);
			if ( IsWarning(j->buffer[i] == 0)) {
				pprint("JPEG: no memory for decode buffer");
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
	register int	x,y;
	register short  a,b,c,d;
	register ulong pix;
	
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

	//pprint("DrawMCU: %d,%d",h,v);
	
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
#ifdef	kScaledDCT
			dcScale <<= 1;
#endif
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
			if ( IsError(h < 0 || h >= rowBytes) )
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


//====================================================================

#define RGBOUT(_d)	_d++;	\
				*_d++ = (p = (y + v)) > 0 ? (p <= 255 ? p : 255) : 0;				\
				*_d++ = (p = (y - (v/2 + u/6))) > 0 ? (p <= 255 ? p : 255) : 0;	\
				*_d++ = (p = (y + u)) > 0 ? (p <= 255 ? p : 255) : 0;

//	Make a slice of a picture from buffer
//	NOTE: width must be the full width of the component buffers
//	otherwise the image will skew

short MakeColorSlice(JPEGDecoder *j, Pixels *pixels, short line, short width, short height)
{
	short	x,yy,y,u,v,p;
	uchar 	*ySrc,*ySrc2,*uSrc,*vSrc,*dst,*dst2;

//	See we understand the color subsampling
//	CompInFrame is a better measure than components in scan

	if (j->CompInFrame == 1) {
		j->sampling = 0x0000;			// Grayscale
	} else {
		CompSpec *y,*u,*v;
		
		y = &j->Comp[0];
		u = &j->Comp[1];
		v = &j->Comp[2];
		
		if ( IsWarning(u->Hi + u->Vi + v->Hi + v->Vi != 4))  {
			pprint("Can only draw 1:1 chroma <%d:%d> <%d:%d>",
			u->Hi , u->Vi , v->Hi , v->Vi);
			return kGenericError;
		}
		
		j->sampling = ( y->Hi << 4) | y->Vi;
		
		switch (j->sampling) {
			case	0x0011:
			case	0x0021:
			case	0x0022:
				break;
			default:
			if (IsWarning(j->sampling==j->sampling))
				pprint("Can only draw 1:1, 2:1 or 2:2 <%d:%d>",y->Hi,y->Vi);
			return kGenericError;
		}
	}

//	Draw the image into gWorld

	dst = pixels->GetLineAddr(line);
	ySrc = j->buffer[0];
	uSrc = j->buffer[1];
	vSrc = j->buffer[2];
	switch (j->sampling) {
		case	0x0000:								// Grayscale
			for (yy = 0; yy < height; yy++) {
				for (x = 0; x < width; x++) {
					y = (y = *ySrc++) > 0 ? (y <= 255 ? y : 255) : 0;
					dst++;
					*dst++ = y;
					*dst++ = y;
					*dst++ = y;
				}
				pixels->EndLine(line++);
				dst = pixels->GetLineAddr(line);
			}
			break;
		case	0x0011:								// 1:1 RGB
			for (yy = 0; yy < height; yy++) {
				for (x = 0; x < width; x++) {
					y = *ySrc++;
					u = (*uSrc++ - 128) << 1;		// u = u*2;
					v = ((*vSrc++ - 128) << 4)/10;	// v = v*1.6
					RGBOUT(dst);
				}
				pixels->EndLine(line++);
				dst = pixels->GetLineAddr(line);
			}
			break;
		case	0x0021:								// 2:1 RGB
			for (yy = 0; yy < height; yy++) {
				for (x = 0; x < (width >> 1); x++) {
					u = (*uSrc++ - 128) << 1;		// u = u*2;
					v = ((*vSrc++ - 128) << 4)/10;	// v = v*1.6
					y = *ySrc++;
					RGBOUT(dst);
					y = *ySrc++;
					RGBOUT(dst);
				}
				pixels->EndLine(line++);
				dst = pixels->GetLineAddr(line);
			}
			break;
		case	0x0022:								// 2:2 RGB
			dst2 = pixels->GetLineAddr(line+1);		// Pixels must support up to TWO simulaneous lines
			ySrc2 = j->buffer[0] + j->rowBytes[0];
			for (yy = 0; yy < (height >> 1); yy++) {
				for (x = 0; x < (width >> 1); x++) {
					u = (*uSrc++ - 128) << 1;		// u = u*2;
					v = ((*vSrc++ - 128) << 4)/10;	// v = v*1.6
					y = *ySrc++;
					RGBOUT(dst);
					y = *ySrc++;
					RGBOUT(dst);
					y = *ySrc2++;
					RGBOUT(dst2);
					y = *ySrc2++;
					RGBOUT(dst2);
				}
				ySrc += width;
				ySrc2 += width;
				pixels->EndLine(line++);
				pixels->EndLine(line++);
				dst = pixels->GetLineAddr(line);
				dst2 = pixels->GetLineAddr(line+1);
			}
			break;
	}
	return 0;
}
