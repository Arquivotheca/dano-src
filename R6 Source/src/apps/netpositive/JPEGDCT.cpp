// ===========================================================================
//	JPEG.cpp
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#include "JPEG.h"

// ===========================================================================

// ===========================================================================

#ifndef	kScaledDCT

/* idct.c, inverse fast discrete cosine transform                           */


/**********************************************************/
/* inverse two dimensional DCT, Chen-Wang algorithm       */
/* (cf. IEEE ASSP-32, pp. 803-816, Aug. 1984)             */
/* 32-bit integer arithmetic (8 bit coefficients)         */
/* 11 mults, 29 adds per DCT                              */
/*                                      sE, 18.8.91       */
/**********************************************************/
/* coefficients extended to 12 bit for IEEE1180-1990      */
/* compliance                           sE,  2.1.94       */
/**********************************************************/

/* this code assumes >> to be a two's-complement arithmetic */
/* right shift: (-2)>>1 == -1 , (-3)>>1 == -2               */

#define W1 2841 /* 2048*sqrt(2)*cos(1*pi/16) */
#define W2 2676 /* 2048*sqrt(2)*cos(2*pi/16) */
#define W3 2408 /* 2048*sqrt(2)*cos(3*pi/16) */
#define W5 1609 /* 2048*sqrt(2)*cos(5*pi/16) */
#define W6 1108 /* 2048*sqrt(2)*cos(6*pi/16) */
#define W7 565  /* 2048*sqrt(2)*cos(7*pi/16) */

static inline void idctrow(short *blk)
{	
	long x0, x1, x2, x3, x4, x5, x6, x7, x8;
	long t0;
	
	/* shortcut */
	if (!((x1 = blk[4]<<11) | (x2 = blk[6]) | (x3 = blk[2]) |
		(x4 = blk[1]) | (x5 = blk[7]) | (x6 = blk[5]) | (x7 = blk[3]))) 
	{
		blk[0]=blk[1]=blk[2]=blk[3]=blk[4]=blk[5]=blk[6]=blk[7]=blk[0]<<3;
		return;
	}
	t0 = (short)W7 * (short)(x4+x5);

	x0 = (blk[0]<<11) + 128; /* for proper rounding in the fourth stage */
	
	/* first stage */

	x4 = t0 + (short)(W1-W7)*(short)x4;
	x5 = t0 - (short)(W1+W7)*(short)x5;
	
	t0 = (short)W3*(short)(x6+x7);
	x6 = t0 - (short)(W3-W5)*(short)x6;
	x7 = t0 - (short)(W3+W5)*(short)x7;
	
	/* second stage */
	x8 = x0 + x1;
	x0 -= x1;
	t0 = (short)W6*(short)(x3+x2);
	x1 = x4 + x6;
	x4 -= x6;
	x2 = t0 - (short)(W2+W6)*(short)x2;
	x6 = x5 + x7;
	x3 = t0 + (short)(W2-W6)*(short)x3;
	x5 -= x7;
	
	/* third stage */
	
	t0 = (181*(x4+x5)+128)>>8;
	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = t0;
	x4 = (181*(x4-x5)+128)>>8;
	
	/* fourth stage */
	blk[0] = (x7+x1)>>8;
	blk[1] = (x3+x2)>>8;
	blk[3] = (x8+x6)>>8;
	blk[4] = (x8-x6)>>8;
	blk[6] = (x3-x2)>>8;
	blk[7] = (x7-x1)>>8;
								// waiting on x4 result
	blk[2] = (x0+x4)>>8;
	blk[5] = (x0-x4)>>8;
}

static void inline idctcol(short *blk)
{
	long x0, x1, x2, x3, x4, x5, x6, x7, x8;
	long t0;

	/* shortcut */
	if (!((x1 = (blk[8*4]<<8)) | (x2 = blk[8*6]) | (x3 = blk[8*2]) |
	    (x4 = blk[8*1]) | (x5 = blk[8*7]) | (x6 = blk[8*5]) | (x7 = blk[8*3]))) 
	{
		blk[8*0]=blk[8*1]=blk[8*2]=blk[8*3]=blk[8*4]=blk[8*5]=blk[8*6]=blk[8*7]= (blk[8*0]+32)>>6;
		return;
	}
	t0 = (short)W7*(short)(x4+x5) + 4;

	x0 = (blk[8*0]<<8) + 8192;
	
	/* first stage */

	x4 = (t0+(short)(W1-W7)*(short)x4)>>3;
	x5 = (t0-(short)(W1+W7)*(short)x5)>>3;
	t0 = (short)W3*(short)(x6+x7) + 4;
	x6 = (t0-(short)(W3-W5)*(short)x6)>>3;
	x7 = (t0-(short)(W3+W5)*(short)x7)>>3;
	
	/* second stage */
	t0 = (short)W6*(short)(x3+x2) + 4;
	x8 = x0 + x1;
	x0 -= x1;
	x2 = (t0-(short)(W2+W6)*(short)x2)>>3;
	x1 = x4 + x6;
	x4 -= x6;
	x3 = (t0+(short)(W2-W6)*(short)x3)>>3;
	x6 = x5 + x7;
	x5 -= x7;
	
	/* third stage */
	
	t0 = (181*(x4+x5)+128)>>8;
	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = t0;
	x4 = (181*(x4-x5)+128)>>8;
	
	/* fourth stage */
	
	blk[8*0] = (x7+x1)>>14;
	blk[8*1] = (x3+x2)>>14;
	blk[8*3] = (x8+x6)>>14;
	blk[8*4] = (x8-x6)>>14;
	blk[8*6] = (x3-x2)>>14;
	blk[8*7] = (x7-x1)>>14;
	
		// waiting on x4 result
	blk[8*2] = (x0+x4)>>14;
	blk[8*5] = (x0-x4)>>14;
	
}

#else



//===================================================
//
//
//	Scaled DFT Algorithm, Arai, Agui and Nakajima
//
//	Five multiplies per 8x1 DFT, DCT values obtained by
//	scaling of the dequantization table ( 8 free multiplies )
//
//	Transactions of the IEICE November 1988
//
//
//


const int	kb1 = (int)(1.41421356 * (1<<M_SCALE_F));
const int	kb2 = (int)(2.61312587 * (1<<M_SCALE_F));
const int	kb3 = (int)(1.41421356 * (1<<M_SCALE_F));
const int	kb4 = (int)(1.08239220 * (1<<M_SCALE_F));	
const int	kb5 = (int)(0.76536886 * (1<<M_SCALE_F));


#define	O_SCALE(z)	((z) >> O_SCALE_F )		// pre-rounding is done as a separate op



#ifdef	DO_ROUNDING

#define	MUL(a,b)	((( (short)(a) * (short)(b) + (1<<(M_SCALE_F-1)) ) ) >> M_SCALE_F)

#else

#define	MUL(a,b)	( ((short)(a) * (short)(b)  ) >> M_SCALE_F)

#endif


#define	kOutSpace	8		// write as collumns


static void
ScaledIDCT(register const short *src,register short *dst, int oscale)
{
	register short	e0,e1,e2,e3,e4,e5,e6,e7;
	int				i;
	
#ifdef	DO_ROUNDING
	int	oround = oscale ? (1 << (oscale-1)) - 1 : 0;
#endif

	for ( i=0; i < 8; i++, dst++ ) 
	{
	
		// input and trivial bypass case
		
		e0 = src[0];
		e5 = src[1];
		e2 = src[2];
		e7 = src[3];
		e1 = src[4];
		e4 = src[5];
		e3 = src[6];
		e6 = src[7];
		src += 8;

		if ( (e5|e1|e4|e3|e6|e2|e7) == 0 )
		{
	#ifdef	DO_ROUNDING
		 	e0 += oround;
	#endif
			dst[0*kOutSpace]=dst[1*kOutSpace]=dst[2*kOutSpace]=dst[3*kOutSpace]=dst[4*kOutSpace]=dst[5*kOutSpace]=dst[6*kOutSpace] = dst[7*kOutSpace]= e0 >> oscale;
			continue;
		}
		
	//	Top Half
	
		register  short		d0,d1,d2,d3;
		register  short 	b1_3 = kb1;
			
	 	d0 = e0 + e1;
	 	d1 = e0 - e1;
		d3 = e2 + e3;
		d2 = MUL(kb1,e2 - e3) - d3;
	 	e0 = d0 + d3;
	 	e1 = d1 + d2;
	 	e2 = d1 - d2;
	 	e3 = d0 - d3;
	
		if ( (e5|e7|e4|e6) == 0 ) 
		{
	
			//	Skip Bottom Half if inputs are all zero
			
	#ifdef	DO_ROUNDING
	 		e0 += oround;
	 		e1 += oround;
	 		e2 += oround;
	 		e3 += oround;
	 #endif
		 	dst[7*kOutSpace] = dst[0*kOutSpace] = e0 >> oscale;
		 	dst[6*kOutSpace] = dst[1*kOutSpace] = e1 >> oscale;
		 	dst[5*kOutSpace] = dst[2*kOutSpace] = e2 >> oscale;
		 	dst[4*kOutSpace] = dst[3*kOutSpace] = e3 >> oscale;
	
		} 
		else 
		{
		
			register  short d4,d5,d6,d7;
			register  short t = kb2;
			register  short b4 = kb4;
			register  short b5 = kb5;
		
			//	do Bottom Half 
			
			d0 = e4 - e7;
			t = MUL(t, d0);
		 	d2 = e5 - e6;
			d4 = MUL(b5,d0 - d2);
		 	d1 = e5 + e6;
		 	d3 = e4 + e7;
			d7 = d1 + d3;
			d6 = MUL(b4,d2) - d4 - d7;
			d5 = MUL(b1_3,d1 - d3) - d6;
			e4 = d5 + (d4 - t);
			
		//	Output stage
	
		
	#ifdef	DO_ROUNDING
	 		e0 += oround;
	 		e1 += oround;
	 		e2 += oround;
	 		e3 += oround;
	 #endif
		 	dst[0*kOutSpace] = (e0 + d7) >> oscale;
		 	dst[1*kOutSpace] = (e1 + d6) >> oscale;
		 	dst[2*kOutSpace] = (e2 + d5) >> oscale;
		 	dst[3*kOutSpace] = (e3 - e4) >> oscale;
		 	dst[4*kOutSpace] = (e3 + e4) >> oscale;
		 	dst[5*kOutSpace] = (e2 - d5) >> oscale;
		 	dst[6*kOutSpace] = (e1 - d6) >> oscale;
		 	dst[7*kOutSpace] = (e0 - d7) >> oscale;
		}	
	}
}

#endif
//==================================================================
//
//	Transform the blocks from DCT space into pixels.
//
//

void
TransformBlocks(register JPEGDecoder *j,CompSpec *cspec,long count,register const short *block,register short *dqblock)
{
		// do dequantization if not already done in VLC decode
			// it would be smart to dezigzag the quantization table
			// but we like it zigzagged for non-progressive and
			// at the time we get the table, we dont know which kind
			// of image we are...
			
	if ( j->isProgressive ) {
		register int i,k,zzi;
		register const short *ip = block;
		register short *op = dqblock;
		register ushort	*Q = cspec->QTable;
		register const uchar *ZZ = j->ZZ;
		
		for (k=count; k--; )  { 	

			// also since block has a lot of zeros so it may be good to avoid
			// the multiplies there, but an R4000 does a short mul in 2/3 cycles
			// which is the same time to do a compare and branch (or less if
			// the icache/compiler sucks
			
			for (i=0; i < kMCUSize; i++ )  {
				zzi = ZZ[i];
				op[zzi] = ip[zzi] * Q[i];
			}
			op += kMCUSize;
			ip += kMCUSize;
		}
		block = dqblock;
	}
			
	while ( count-- ) 
	{
		
#ifdef kScaledDCT		
 		short	imd[kMCUSize];
		ScaledIDCT(block,imd,0);
		ScaledIDCT(imd,dqblock,O_SCALE_F);
#else
		short *op = dqblock;
		for (i = 8; i--; op += 8)
			idctrow(op);
		op = dqblock;
		for (i = 8; i--; op ++ )
			idctcol(op);
#endif
		dqblock += kMCUSize;
		block += kMCUSize;
	}			
}

void
DoTransform(JPEGDecoder *j,long mcuNum,const short *src)
{
	int	i;
	long bsize;
	short 	*dctbuffer = j->blocks;
	CompSpec *cpn;
	const short	*s = src;
	
	for ( i = 0; i < j->CompInFrame; i++   ) 
	{
		cpn = &j->Comp[i];
		bsize = cpn->blocksMCU << kMCUSizeShift;
		if ( src == NULL )
			s = cpn->blockBuffer + mcuNum * bsize;

		TransformBlocks(j,cpn,cpn->blocksMCU,s,dctbuffer);
		s += bsize;
		dctbuffer += bsize;
	}
}



