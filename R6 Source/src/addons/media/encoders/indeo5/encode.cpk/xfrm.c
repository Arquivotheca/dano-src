/************************************************************************
*                                                                       *
*               INTEL CORPORATION PROPRIETARY INFORMATION               *
*                                                                       *
*    This listing is supplied under the terms of a license agreement    *
*      with INTEL Corporation and may not be copied nor disclosed       *
*        except in accordance with the terms of that agreement.         *
*                                                                       *
*************************************************************************
*                                                                       *
*               Copyright (C) 1994-1997 Intel Corp.                       *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/
/* 
 * xfrm.c
 *	Transformation functions
 * 
 * Functions
 *	transform
 *	BlockGetDiff
 *	BlockPut
 *	BlockGet
 */


#include <setjmp.h>

#include "datatype.h"
#include "matrix.h"
#include "xfrm.h"
#include "common.h"

#define DIV2(x)     ((x)>0?(x)>>1:-(-(x))>>1)

#define bfly(x,y) t1 = x-y; x += y; y = t1;
#define bfly2(x,y) t1 = x-y; x += y; y = DIV2(t1); x = DIV2(x);

/* This is a reflection using a,b = 1/2, 5/4 */
#define reflect(s1,s2) \
t  =  s1 + (s1>>2) + (s2>>1); \
s2 = -s2 - (s2>>2) + (s1>>1); \
s1 = t;   

/* This is a reflection using a,b = 1/2, 5/4, more precision, rounding */
#define treflect(s1,s2)	\
	{ int t; \
	t  =  (s1*5 + s2*2 + 2)>>2; ; \
	s2 = (s1*2 - s2*5 + 2)>>2; ; \
	s1 = t;	\
	}


#define NUM1  40
#define NUM2  16
#define DEN   29

/* below is a reflection using a,b = 16/29, 40/29 with prescale */
#define freflectc(s1,s2)\
	t  =  NUM1*(s1/DEN) + NUM2*(s2/DEN);\
	s2 =  NUM2*(s1/DEN) - NUM1*(s2/DEN);\
	s1 = t;

/* below is a reflection using a,b = 16/29, 40/29 without prescale and with rounding */
#define freflect(s1,s2)\
	t  =  ((NUM1*s1) + (NUM2*s2) + DEN/2 )/DEN;\
	s2 =  ((NUM2*s1) - (NUM1*s2) + DEN/2 )/DEN;\
	s1 = t;

#define SlantPart1 \
bfly(r1,r4); \
bfly(r2,r3); \
bfly(r5,r8); \
bfly(r6,r7);

#define SlantPart2 \
bfly(r1,r2); \
reflect(r4,r3); \
bfly(r5,r6); \
reflect(r8,r7);

#define NewSlantPart2	\
	bfly(r1,r2)	\
	freflect(r4,r3)	\
	bfly(r5,r6)	\
	freflect(r8,r7)

#define ExpSlantPart2	\
	bfly(r1,r2)	\
	treflect(r4,r3)	\
	bfly(r5,r6)	\
	treflect(r8,r7)

/* This is a reflection of r4,r5 using a8,b8 = 7/8, 1/2, more precise & round */
#define ExpSlantPart4	\
	{ int t;\
	t  =  (r4*4 + r5*7 + 4)>>3;\
	r5 =  (r4*7 - r5*4 + 4)>>3;\
	r4 = t;\
	}

#define SlantPart3 \
bfly(r1,r5); \
bfly(r2,r6); \
bfly(r7,r3); \
bfly(r4,r8);

/* This is a reflection of r4,r5 using a8,b8 = 7/8, 1/2 */
#define SlantPart4 \
t  =  r5 - (r5>>3) + (r4>>1); \
r5 =  r4 - (r4>>3) - (r5>>1); \
r4 = t;    


static __inline void
ColInvSlant1d8 (PI32 piIn)
{
	I32 r1,r2,r3,r4,r5,r6,r7,r8;
	I32 t1;
	PI32 p;

	/* read in r1 through r8 from the row in the correct order */
	p  = piIn;
	r1 = *p;
	r4 = *(p + 8);
	r8 = *(p + 16);
	r5 = *(p + 24);
	r2 = *(p + 32);
	r6 = *(p + 40);
	r3 = *(p + 48);
	r7 = *(p + 56);

	ExpSlantPart4 SlantPart3 ExpSlantPart2 SlantPart1

	/* write out r1 through r8 back to the row in the correct order */
	p = piIn;
	*p        = r1;
	*(p + 8)  = r2;
	*(p + 16) = r3;
	*(p + 24) = r4;
	*(p + 32) = r5;
	*(p + 40) = r6;
	*(p + 48) = r7;
	*(p + 56) = r8;
	return;
}

static __inline void
ColInvSlant1d4 (PI32 piIn)
{
	I32 r1,r2,r3,r4;
	I32 t1;
	PI32 p;

	p = piIn;
	r1 = *p;
	r4 = *(p + 4);
	r2 = *(p + 8);
	r3 = *(p + 12);

	bfly(r1,r2); treflect(r4,r3);	/* part2 */
	bfly(r1,r4); bfly(r2,r3); 		/* part1 */

	p = piIn;
	*p        = r1;
	*(p + 4)  = r2;
	*(p + 8)  = r3;
	*(p + 12) = r4;
	return;
}

static __inline void
RowInvSlant1d8 (PI32 p)
{
	I32 r1,r2,r3,r4,r5,r6,r7,r8;
	I32 t1;

	/* read in r1 through r8 from the row in the correct order */
	r1 = *p;
	r4 = *(p+1);
	r8 = *(p+2);
	r5 = *(p+3);
	r2 = *(p+4);
	r6 = *(p+5);
	r3 = *(p+6);
	r7 = *(p+7);

	ExpSlantPart4 SlantPart3 ExpSlantPart2 SlantPart1

	/* write out r1 through r8 back to the row in the correct order */
	*p     = (r1+1) >> 1;
	*(p+1) = (r2+1) >> 1;
	*(p+2) = (r3+1) >> 1;
	*(p+3) = (r4+1) >> 1;
	*(p+4) = (r5+1) >> 1;
	*(p+5) = (r6+1) >> 1;
	*(p+6) = (r7+1) >> 1;
	*(p+7) = (r8+1) >> 1;
	return;
}

static __inline void
RowInvSlant1d4 (PI32 p)
{
	I32 r1,r2,r3,r4;
	I32 t1;

	r1 = *p;
	r4 = *(p+1);
	r2 = *(p+2);
	r3 = *(p+3);

	bfly(r1,r2); treflect(r4,r3);	/* part2 */
	bfly(r1,r4); bfly(r2,r3); 		/* part1 */

	*p     = (r1+1) >> 1;
	*(p+1) = (r2+1) >> 1;
	*(p+2) = (r3+1) >> 1;
	*(p+3) = (r4+1) >> 1;
	return;
}


/* dec_transform handles only the inverse transforms */
void
dec_transform(PI32 piTemp,	/* block to transform */
  			  I32 iXform )		/* which transform to use (slant/Haar/none) */
{
	U32 i;
	I32 iTemp;
	
	switch( iXform ) { /* switch based on which transform to use */
		case XFORM_NONE_8x8:	/* No transform. Don't do anything.	*/
		break;
		
 		case XFORM_SLANT_8x8:	 /* 2d slant transform */
			/* Inverse slant transform the columns */
			for (i = 0; i < 8; i++) {
				ColInvSlant1d8 (piTemp);
				piTemp++;
			}
			piTemp -= 8;
			/* now do the rows */
			for (i = 0; i < 8; i++) {
				RowInvSlant1d8 (piTemp);
				piTemp += 8;
			}
		break;

		case XFORM_SLANT_4x4:		 /* 2d slant transform */
			/* Inverse slant transform the columns */
			for (i = 0; i < 4; i++) {
				ColInvSlant1d4 (piTemp);
				piTemp++;
			}
			piTemp -= 4;
			/* now do the rows */
			for (i = 0; i < 4; i++) {
				RowInvSlant1d4 (piTemp);
				piTemp += 4;
			}
		break;

		case XFORM_SLANT_1x8:		 /* horizontal 1D slant transform */
			for ( i=0; i<8; i++ ) {
				RowInvSlant1d8 (piTemp);
				piTemp += 8;
			}
		break;

		case XFORM_SLANT_8x1:		/* vertical 1D slant transform */
			for ( i=0; i<8; i++ )  {
				ColInvSlant1d8 (piTemp);
				piTemp++;
			}
			piTemp -= 8;
			/* Scale down to satisfy precision requirement. Details in 
			Bitstream Spec */
			for ( i=0; i<64; i++ )
			{
				iTemp = *piTemp;
				*piTemp++ = iTemp+1 >> 1;
			}
		break;

		default:
		break;
	}  /* end switch xform type */
	return;
}


/* 
 * BlockPut
 *
 * Put the I32 input values into the I16 matrix that is the image.
 * Since the mDestPic is padded to be multiples of 8. And Since the block Size 
 * is 8. Therefore, It is safe here to do the straight block copy even when 
 * the block is partially outside the picture dimension.
 * If the assumption changes, some other solution is needed.
 * 
 * Note that since this BlockPut is used to put both spatial and transform block,
 * zeroing out the pixels outside the picture dimension may not be a good idea.
 * Further more, it should be designed in a way such that
 * when being used with BlockGet() repeatedly,  it still makes sense.
 */
void
BlockPut(
	PI32 piSrcBlock,	/* 8x8 source block */
	I32 iBlockSize,
	MatrixSt mDestPic,  /* Pic to set */
	RectSt rDest		/* block of mSrcPic to set */
)
{
	I32 iRow,iCol;
	PI16 pi16Dest;
	PI16 pi16DestRef;
	PI32 piSrc = piSrcBlock;
	I32 iColLim, iRowLim;

	iColLim = rDest.w + rDest.c;
	iRowLim = rDest.h + rDest.r;
	pi16Dest = pi16DestRef = mDestPic.pi16 + (rDest.r*mDestPic.Pitch) + rDest.c;
	/* Put the values and check for bottom edge of the picture */	
	for (iRow = rDest.r; iRow < iRowLim; iRow++) {
		for (iCol=rDest.c; iCol < iColLim; iCol++) {
			*pi16Dest++ = (I16)*piSrc++;
		}
		pi16DestRef += mDestPic.Pitch;
		pi16Dest = pi16DestRef;
		piSrcBlock += iBlockSize;
		piSrc = piSrcBlock;
	}
}

/* Note:
 * In this BlockGetDiff, we ignore the pixels outside the picture dimension and
 * try to use previous columns/rows to duplicate them. This is ok because this routine is
 * used only in spatial domain.
 */
void
BlockGetDiff(
MatrixSt mSrcPic,   	/* src picture */
MatrixSt mSrcMc,    	/* Motion Compensation */
RectSt rSrc,   			/* block of mSrcPic to get */
PI32 piDst            	/* 8x8 destination block */
)
{
	I32 iRow,iCol;
	PI16 pi16Src ;
	PI16 pi16Mc  ;
	
	
	for (iRow = 0; iRow < (I32)rSrc.h; iRow++) {
		pi16Src = mSrcPic.pi16 + (rSrc.r + iRow) * mSrcPic.Pitch + rSrc.c;
		pi16Mc  = mSrcMc.pi16  + (rSrc.r + iRow) * mSrcMc.Pitch  + rSrc.c;
		for (iCol=0;iCol<(I32)rSrc.w;iCol++) {
			if (( iRow + rSrc.r ) >= mSrcPic.NumRows) {
				/* pad with prev iRow pixels: we are at bottom edge of picture */
				*piDst = (I32) *(piDst - rSrc.w);
			} else if (( iCol + rSrc.c ) >= mSrcPic.NumCols) {
				/* pad with last iCol's pixels: we are at right edge of picture */
				*piDst = (I32) *(piDst - 1);
			} else {
				*piDst = (I32) *pi16Src++ - (I32) *pi16Mc++ ;
			}
			piDst++;
		}
	}
}

/* 
 * BlockGet
 *
 * Get the block data from the image.
 * Since the mSrcPic is padded to be multiples of 8. And Since the block Size 
 * is 8. Therefore, It is safe here to do the straight block copy even when 
 * the block is partially outside the picture dimension.
 * If the assumption changes, some other solution is needed.
 * 
 * Note that since this BlockPut is used to put both spatial and transform block,
 * zeroing out or duplicating by previous row/column for the pixels outside the 
 * picture dimension may not be a good idea.
 * Further more, it should be designed in a way such that
 * when being used with BlockPut() repeatedly,  it still makes sense.
 */

void
BlockGet(
	PI32 piDst,			/* 8x8 destination block */
	MatrixSt mSrcPic,   /* src picture */
	RectSt rSrc		/* block of mSrcPic to get */
)
{
	I32 iRow,iCol;
	PI16 pi16Src ;
	
	for (iRow = 0; iRow < (I32)rSrc.h; iRow++) {
		pi16Src = mSrcPic.pi16 + (rSrc.r + iRow ) * mSrcPic.Pitch + rSrc.c;
		for (iCol = 0; iCol < (I32) rSrc.w; iCol++) {
			*piDst = (I32) *pi16Src++;
			piDst++;
		}
	}
}


static void
slant1d( PI32 piIn, U16 u16BlockSize, PIA_Boolean bFwd )
{
	I32 r1,r2,r3,r4,r5,r6,r7,r8;
	I32 t,t1;
	PI32 p;

	switch( u16BlockSize ) {
		case 8:
			if (bFwd == FORWARD)	{	/* if forward transform */
				p  = piIn;  /* read in the row into r1 through r8 */
				r1 = *p++;
				r2 = *p++;
				r3 = *p++;
				r4 = *p++;
				r5 = *p++;
				r6 = *p++;
				r7 = *p++;
				r8 = *p++;
				SlantPart1 NewSlantPart2 SlantPart3 ExpSlantPart4
				/* write out r1 through r8 back to the row in the correct order */
		   		p = piIn;
		   		*p++ = r1;
		   		*p++ = r4;
		   		*p++ = r8;
			   	*p++ = r5;
		   		*p++ = r2;
		   		*p++ = r6;
		   		*p++ = r3;
		   		*p++ = r7;
	  		} else { /* inverse transform */
				/* read in r1 through r8 from the row in the correct order */
		   		p  = piIn;
		   		r1 = *p++;
		   		r4 = *p++;
		   		r8 = *p++;
		   		r5 = *p++;
		   		r2 = *p++;
		   		r6 = *p++;
		   		r3 = *p++;
		   		r7 = *p++;
	
				ExpSlantPart4 SlantPart3 ExpSlantPart2 SlantPart1

				/* write out r1 through r8 back to the row in the correct order */
		   		p = piIn;
		   		*p++ = r1;
		   		*p++ = r2;
		   		*p++ = r3;
		   		*p++ = r4;
		   		*p++ = r5;
		   		*p++ = r6;
		   		*p++ = r7;
		   		*p++ = r8;
	  		} /* endif inverse transform */
		break;

		case 4:
		   if (bFwd == FORWARD) {
				p  = piIn;
			   	r1 = *p++;
			   	r2 = *p++;
			   	r3 = *p++;
			  	r4 = *p++;
					
				bfly(r1,r4); bfly(r2,r3);		/* part1 */
 				freflect(r4,r3); bfly(r1,r2);	/* part2 */
  		  		p = piIn;
		  		*p++ = r1;
		  		*p++ = r4;
		  		*p++ = r2;
		  		*p++ = r3;
			} else {/* inverse transform */
				p = piIn;
				r1 = *p++;
				r4 = *p++;
				r2 = *p++;
				r3 = *p++;
			
				bfly(r1,r2); treflect(r4,r3);	/* part2 */
				bfly(r1,r4); bfly(r2,r3); 		/* part1 */
  	
				p = piIn;
				*p++ = r1;
				*p++ = r2;
				*p++ = r3;
				*p++ = r4;
		   } /* endif inverse transform */
		break;

		default:
		break;
	} /*end switch blocksize */
	return;
}

/* 
	The following routine transposes a matrix of numbers:
	Inputs:	a -- a pointer to the matrix
				u16BlockSize -- the length (and width) of the matrix
	Outputs: a -- the transposed matrix in a
	It is assumed that the matrix is composed of contiguous memory
	i.e. pitch = width.
*/

static void transpose(
 	PI32 piA,
	U16 u16BlockSize)	/* dimension of square block to transpose */
{
	I32 i,j,tmp;
	
	for (i=0; i<u16BlockSize; i++)
		for (j=0; j<i; j++){ 
			tmp = *(piA + u16BlockSize * i + j);	/* tmp = piA[i][j];  */
			*(piA + u16BlockSize * i + j) =
					 *(piA + u16BlockSize * j + i);	/* piA[i][j] = piA[j][i]; */
			*(piA + u16BlockSize * j + i) = tmp;	/* piA[j][i] = tmp; */
		}
}


void
transform(	PI32 piTemp,	/* block to transform */
				PIA_Boolean bFwd,	/* Forward or Inverse */
				I32 iXform )		/* which transform to use (slant/Haar/none) */
{
	U32 i;
	I32 iTemp;
	
	switch( iXform ) { /* switch based on which transform to use */
		case XFORM_NONE_8x8:	/* No transform. Don't do anything.	*/
		break;
		
		case XFORM_SLANT_8x8:	 /* 2d slant transform */
			/* If inverse Transpose so that we do column then row */
			if (bFwd == INVERSE) {
				transpose( piTemp, 8 );
			}
			/* now do the columns for inverse or rows for forward*/
			for ( i=0; i<8; i++ ) {
				slant1d( piTemp, 8, bFwd );
				piTemp += 8;
			}
			piTemp -= 64;   
			transpose( piTemp, 8 );
			/* now do the rows for inverse or columns for forward */
			for ( i=0; i<8; i++ ) {
				slant1d( piTemp, 8, bFwd );
				piTemp += 8;
			}
			piTemp -= 64;
			/* 
			now scale down to maintain 1 bit precision increase. If blocksize 
			is 8 then for forward, shift down 5 bits, for inverse shift down 1 bit 
			(2 bit asymmetric slant). for 4x4, use 1 bit symmetric implementation.
			Details	in Bitstream Spec 
			*/
			for ( i=0; i<64; i++ ) {
				iTemp = *piTemp;
				if ( bFwd == FORWARD )
					*piTemp++ = iTemp+16 >> 5;
				else
					*piTemp++ = iTemp+1 >> 1;
			} /* end for loop for scaling each coefficient in the block */
			piTemp -= 64;
			/* Transpose on forward to proper orientation */
			if(bFwd == FORWARD) 
				transpose( piTemp, 8 );
		break;
		case XFORM_SLANT_4x4:		 /* 2d slant transform */
			/* If inverse Transpose so that we do column then row */
			if (bFwd == INVERSE) {
				transpose( piTemp, 4 );
			}
			/* now do the columns for inverse or rows for forward*/
			for( i=0; i<4; i++ ) {
				slant1d( piTemp, 4, bFwd );
				piTemp += 4;
			}
			piTemp -= 16;   
			transpose( piTemp, 4 );
			/* now do the rows for inverse or columns for forward */
			for( i=0; i<4; i++ ) {
				slant1d( piTemp, 4, bFwd );
				piTemp += (I32) 4;
			}
			piTemp -= 16;
			/* 
			now scale down to maintain 1 bit precision increase. If blocksize 
			is 8 then for forward, shift down 5 bits, for inverse shift down 1 bit 
			(2 bit asymmetric slant). for 4x4, use 1 bit symmetric implementation.
			Details	in Bitstream Spec 
			*/
			for ( i=0; i<16; i++ ) {
				iTemp = *piTemp;
				if( bFwd == FORWARD )
					*piTemp++ = iTemp+4 >> 3;
				else
					*piTemp++ = iTemp+1 >> 1;
			} /* end for loop for scaling each coefficient in the block */
			piTemp -= 16;
			/* Transpose on forward to proper orientation */
			if (bFwd == FORWARD) 
				transpose( piTemp, 4 );
		break;
		case XFORM_SLANT_1x8:		 /* horizontal 1D slant transform */
			for ( i=0; i<8; i++ ) {
				slant1d( piTemp, 8, bFwd );
				piTemp += 8;
			}
			piTemp -= 64;
			/* Scale down to satisfy precision requirement. Details in 
			Bitstream Spec */
			if ( bFwd == INVERSE ) {
				for ( i=0; i<64; i++ ) {
					iTemp = *piTemp;
					*piTemp++ = iTemp+1 >> 1;
				}
				piTemp -= 64;
			} else { /* forward xform */
				for ( i=0; i<64; i++ ) {
					iTemp = *piTemp;
					*piTemp++ = iTemp+2 >> 2;
				}
				piTemp -= 64;
			}
		break;
		case XFORM_SLANT_8x1:		/* vertical 1D slant transform */
			transpose( piTemp, 8 );
			for ( i=0; i<8; i++ )  {
				slant1d( piTemp, 8, bFwd );
				piTemp += 8;
			}
			piTemp -= 64;
			transpose( piTemp, 8 );
			/* Scale down to satisfy precision requirement. Details in 
			Bitstream Spec */
			if ( bFwd == INVERSE ) {
				for ( i=0; i<64; i++ )
				{
					iTemp = *piTemp;
					*piTemp++ = iTemp+1 >> 1;
				}
				piTemp -= 64;
			} else { /* forward xform */
				for ( i=0; i<64; i++ ) {
					iTemp = *piTemp;
					*piTemp++ = iTemp+2 >> 2;
				}
				piTemp -= 64;
			}
		break;
		default:
		break;
	}  /* end switch xform type */
	return;
}
