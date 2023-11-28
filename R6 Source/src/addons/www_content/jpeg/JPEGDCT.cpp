// ===========================================================================
//	JPEG.cpp
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#include "JPEG.h"


//
//	Scaled DFT Algorithm, Arai, Agui and Nakajima
//
//	Five multiplies per 8x1 DFT, DCT values obtained by
//	scaling of the dequantization table (8 free multiplies)
//
//	Transactions of the IEICE November 1988
//
const int	kb1 = (int)(1.41421356 * (1<<M_SCALE_F));
const int	kb2 = (int)(2.61312587 * (1<<M_SCALE_F));
const int	kb3 = (int)(1.41421356 * (1<<M_SCALE_F));
const int	kb4 = (int)(1.08239220 * (1<<M_SCALE_F));	
const int	kb5 = (int)(0.76536886 * (1<<M_SCALE_F));

#define	O_SCALE(z)	((z) >> O_SCALE_F)		// pre-rounding is done as a separate op
#define	MUL(a,b)	(((short)(a) * (short)(b)) >> M_SCALE_F)
#define	kOutSpace	8		// write as collumns

static void ScaledIDCT(const short *src,short *dst, int oscale)
{
	short	e0,e1,e2,e3,e4,e5,e6,e7;
	int				i;
	
	for (i=0; i < 8; i++, dst++) {
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

		if ((e5|e1|e4|e3|e6|e2|e7) == 0) {
			dst[0*kOutSpace]=dst[1*kOutSpace]=dst[2*kOutSpace]=dst[3*kOutSpace]=dst[4*kOutSpace]=dst[5*kOutSpace]=dst[6*kOutSpace] = dst[7*kOutSpace]= e0 >> oscale;
			continue;
		}
		
		//	Top Half
		short		d0,d1,d2,d3;
		short 	b1_3 = kb1;
			
	 	d0 = e0 + e1;
	 	d1 = e0 - e1;
		d3 = e2 + e3;
		d2 = MUL(kb1,e2 - e3) - d3;
	 	e0 = d0 + d3;
	 	e1 = d1 + d2;
	 	e2 = d1 - d2;
	 	e3 = d0 - d3;
	
		if ((e5|e7|e4|e6) == 0) {
			//	Skip Bottom Half if inputs are all zero
		 	dst[7*kOutSpace] = dst[0*kOutSpace] = e0 >> oscale;
		 	dst[6*kOutSpace] = dst[1*kOutSpace] = e1 >> oscale;
		 	dst[5*kOutSpace] = dst[2*kOutSpace] = e2 >> oscale;
		 	dst[4*kOutSpace] = dst[3*kOutSpace] = e3 >> oscale;
		} else {
			short d4,d5,d6,d7;
			short t = kb2;
			short b4 = kb4;
			short b5 = kb5;
		
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

//	Transform the blocks from DCT space into pixels.
void TransformBlocks(JPEGDecoder *j,CompSpec *cspec,long count,const short *block,short *dqblock)
{
	// do dequantization if not already done in VLC decode
	// it would be smart to dezigzag the quantization table
	// but we like it zigzagged for non-progressive and
	// at the time we get the table, we dont know which kind
	// of image we are...
	if (j->isProgressive) {
		int i,k,zzi;
		const short *ip = block;
		short *op = dqblock;
		ushort	*Q = cspec->QTable;
		const uchar *ZZ = j->ZZ;
		for (k=count; k--;)  { 	
			// also since block has a lot of zeros so it may be good to avoid
			// the multiplies there, but an R4000 does a short mul in 2/3 cycles
			// which is the same time to do a compare and branch (or less if
			// the icache/compiler sucks
			for (i=0; i < kMCUSize; i++)  {
				zzi = ZZ[i];
				op[zzi] = ip[zzi] * Q[i];
			}

			op += kMCUSize;
			ip += kMCUSize;
		}
		block = dqblock;
	}
			
	while (count--) {
 		short	imd[kMCUSize];
		ScaledIDCT(block,imd,0);
		ScaledIDCT(imd,dqblock,O_SCALE_F);
		dqblock += kMCUSize;
		block += kMCUSize;
	}			
}

void DoTransform(JPEGDecoder *j,long mcuNum,const short *src)
{
	int	i;
	long bsize;
	short 	*dctbuffer = j->blocks;
	CompSpec *cpn;
	const short	*s = src;
	
	for (i = 0; i < j->CompInFrame; i++) {
		cpn = &j->Comp[i];
		bsize = cpn->blocksMCU << kMCUSizeShift;
		if (src == NULL)
			s = cpn->blockBuffer + mcuNum * bsize;

		TransformBlocks(j,cpn,cpn->blocksMCU,s,dctbuffer);
		s += bsize;
		dctbuffer += bsize;
	}
}
