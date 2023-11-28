/*
 *
 *               INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *    This listing is supplied under the terms of a license agreement
 *      with INTEL Corporation and may not be copied nor disclosed
 *        except in accordance with the terms of that agreement.
 *
 *
 *
 *               Copyright (c) 1994-1995 Intel Corporation.
 *                         All Rights Reserved.
 *
 */

/*************************************************************************
 *	clutap.c			prototype in: decproci.h
 * 
 *  C_YVU9toActivePalette() --
 *	Convert YVU9 planar to CLUT8, using Active Palette.  
 *  If there is a transparency or local decode mask, apply these masks and
 *  render the specified pixels.
 *
 *************************************************************************/

/* ENDIAN-NESS DEPENDENCIES:
 *   The macros are endian dependent, as is the method for obtaining the masks
 *   when transparency or local decode is present.
 */
#include "datatype.h"
#include "pia_main.h"
#include "yrgb8.h"
#include "colconv.h"
/*#include "dynclut.h"
#include "colconv.h"
*/
extern PU8 gpu8ClutTables;


/*************************************************************************
 * There are several #define's with code in order to make the main function
 * readable without the overhead of a function call.
 */

/* If a pixel is lit in transparency and local decode masks, write it;
 * if it is on only in local decode, then fill it; else, skip it
 */
#define LOCAL_DECODE_FILL(pu8) {										\
	if ((u8LDNibble & PEL0_BITMASK) && (u8Mask & PEL0_BITMASK))  		\
		*(pu8) =   (U8) ((uOutputData&PEL0_BYTEMASK) >> PEL0_SHIFT); 	\
	else if (u8LDNibble & PEL0_BITMASK) /* Render anything? */ 			\
		*(pu8) =   (U8) uFillValue; 									\
																		\
	if ((u8LDNibble & PEL1_BITMASK) && (u8Mask & PEL1_BITMASK)) 		\
		*(pu8+1) = (U8) ((uOutputData&PEL1_BYTEMASK) >> PEL1_SHIFT); 	\
	else if (u8LDNibble & PEL1_BITMASK) 								\
		*(pu8+1) = (U8) uFillValue; 									\
																		\
	if ((u8LDNibble & PEL2_BITMASK) && (u8Mask & PEL2_BITMASK))  		\
		*(pu8+2) = (U8) ((uOutputData&PEL2_BYTEMASK) >> PEL2_SHIFT);	\
	else if (u8LDNibble & PEL2_BITMASK) 								\
		*(pu8+2) = (U8) uFillValue; 									\
																		\
	if ((u8LDNibble & PEL3_BITMASK) && (u8Mask & PEL3_BITMASK))  		\
		*(pu8+3) = (U8) ((uOutputData&PEL3_BYTEMASK) >> PEL3_SHIFT);	\
	else if (u8LDNibble & PEL3_BITMASK) 								\
		*(pu8+3) = (U8) uFillValue; 									\
}

/* Only write the pels that are lit; write nothing to to the non-lit pixels */
#define SELECTIVE_WRITE(pu8)	{	 									\
	if (u8Mask & 8) 								/* Pel 0 lit? */	\
		*(pu8)   = (U8) ((uOutputData&PEL0_BYTEMASK) >> PEL0_SHIFT);						\
	if (u8Mask & 4)  								/* Pel 1 lit? */	\
		*(pu8+1) = (U8) ((uOutputData&PEL1_BYTEMASK) >> PEL1_SHIFT); 						\
	if (u8Mask & 2) 								/* Pel 2 lit? */	\
		*(pu8+2) = (U8) ((uOutputData&PEL2_BYTEMASK) >> PEL2_SHIFT); 						\
	if (u8Mask & 1) 								/* Pel 3 lit? */	\
		*(pu8+3) = (U8) ((uOutputData&PEL3_BYTEMASK) >> PEL3_SHIFT); 								\
}

/* 
 * If not every pel is on, need to write individual bytes.
 */
#define SELECTIVE_UPDATE(pu8) {			 								\
	if (u8LDNibble && bWantsFill)   /* Process through LD Mask first */ \
		LOCAL_DECODE_FILL(pu8)											\
	else if (bWantsFill) { /* Transparency w/fill -- write all pels */ 	\
		*((PU32)(pu8)) = 												\
		   ((u8Mask & PEL0_BITMASK  ? uOutputData&PEL0_BYTEMASK 		\
		   							: uFillValue<<PEL0_SHIFT) 	|  		\
			(u8Mask & PEL1_BITMASK  ? uOutputData&PEL1_BYTEMASK 		\
									: uFillValue<<PEL1_SHIFT)  	| 		\
			(u8Mask & PEL2_BITMASK  ? uOutputData&PEL2_BYTEMASK 		\
									: uFillValue<<PEL2_SHIFT) 	| 		\
			(u8Mask & PEL3_BITMASK  ? uOutputData&PEL3_BYTEMASK 		\
									: uFillValue<<PEL3_SHIFT)); 		\
	}  else  /* No fill -- only write necessary pels */ 				\
		SELECTIVE_WRITE(pu8)											\
}

/* When transparency, local decode, and fill are being used, and the 
 * transparency mask is off ... fill if local decode is on */
#define SELECTIVE_FILL(pu8) {							 				\
	if (u8LDNibble & PEL3_BITMASK) /* Pel 0? */ 						\
		*(pu8) = (U8) uFillValue;		 								\
	if (u8LDNibble & PEL2_BITMASK) /* Pel 1? */ 						\
		*(pu8+1) = (U8) uFillValue;		 								\
	if (u8LDNibble & PEL1_BITMASK) /* Pel 2? */ 						\
		*(pu8+2) = (U8) uFillValue;		 								\
	if (u8LDNibble & PEL0_BITMASK) /* Pel 3? */ 						\
		*(pu8+3) = (U8) uFillValue;		 								\
}	


/* Often-used expressions */
#define EVEN_ROW_PATTERN  \
					((U32)*(gpu8ClutTables + uY3 + uVUb) << PEL0_SHIFT) | 	\
					((U32)*(gpu8ClutTables + uY2 + uVUa) << PEL1_SHIFT) |	\
					((U32)*(gpu8ClutTables + uY1 + uVUb) << PEL2_SHIFT) |	\
					((U32)*(gpu8ClutTables + uY0 + uVUa) << PEL3_SHIFT) 		

#define ODD_ROW_PATTERN   \
					((U32)*(gpu8ClutTables + uY3 + uVUd) << PEL0_SHIFT) | 	\
					((U32)*(gpu8ClutTables + uY2 + uVUc) << PEL1_SHIFT) |	\
					((U32)*(gpu8ClutTables + uY1 + uVUd) << PEL2_SHIFT) | 	\
					((U32)*(gpu8ClutTables + uY0 + uVUc) << PEL3_SHIFT) 

#define MAKE_BYTE(pu8, dither) (*(pu8) + (I32)((dither)*Y_SCALE)) << Y_SHIFT
/*************************************************************************/


/* Arguments are explained fully in clut8.c */
void
C_YVU9toActivePalette_44(U32 uRows, U32 uCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 iOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uMStride,
	Boo bWantsFill, U32 uFillValue)
{
	const I32 Y_SCALE  = 4; /* YDither based on 6-bit pels; scale to 8 bits */
	/* Access to CLUT table is a 16-bit value, yyyyyyyyvvvvuuuu, so shift Y
	 * by 8 bits to be or'd in with the v and u for this index */
	const I32 Y_SHIFT  = 8;	

	const I32 YDither00 = 0;  /* Y Dither patterns for a 4x4 block */
	const I32 YDither01 = 4;
	const I32 YDither02 = 1;
	const I32 YDither03 = 5;
	const I32 YDither10 = 6;
	const I32 YDither11 = 2;
	const I32 YDither12 = 7;
	const I32 YDither13 = 3;
	const I32 YDither20 = 1;
	const I32 YDither21 = 5;
	const I32 YDither22 = 0;
	const I32 YDither23 = 4;
	const I32 YDither30 = 7;
	const I32 YDither31 = 3;
	const I32 YDither32 = 6;
	const I32 YDither33 = 2;

	U32 i, j;
	PU32 puTblU, puTblV; 				  /* U and V dither tables */
	PU8 pu8Y,pu8V,pu8U;			  		  /* Pointers within planes by rows */
	PU8 pu8Out;							  /* Same as above for output */
	U32 uOutLineDelta, uYLineDelta, uVULineDelta;
	U32 uRow0, uRow1, uRow2, uRow3;  /* Amount to add to pu8Y for each row */
	U32 uOutRow0, uOutRow1, uOutRow2, uOutRow3; /*       pu8Out for each row */
	U8  u8U, u8V;						  /* For clipping U and V data */
	U32 uVUVUVUVU;						  /* block dither info */
	U32 uVUa, uVUb, uVUc, uVUd;	  		  /* dither info w/in block */
	U32 uY0, uY1, uY2, uY3;		  		  /* for a row of a block */
	U32 uOutputData;							 
									  
	/* The tables with U and V information are immediately after the 
	 * CLUT Table (the table with entries specifying the palette entry
	 * to use for a given Y,V,U value)
	 */
	puTblU = (PU32)(gpu8ClutTables + (CLUT_TABLE_SIZE));
	puTblV = puTblU + (TABLE_U_SIZE/sizeof(*puTblU));	


	/* Set the amount to add to the pointersfor the next block of 4 rows */
	uOutLineDelta = iOutPitch*4 - uCols;
	uYLineDelta = 4 * uYPitch - uCols;
	uVULineDelta = uVUPitch - (uCols/4);  /* one to 4 Y rows */
	uRow0 = 0*uYPitch; 		uOutRow0 = 0*iOutPitch;
	uRow1 = 1*uYPitch;		uOutRow1 = 1*iOutPitch;
	uRow2 = 2*uYPitch;		uOutRow2 = 2*iOutPitch;
	uRow3 = 3*uYPitch;		uOutRow3 = 3*iOutPitch;

    /* Initialize the pointers that move within the buffers */
	pu8Out = pOut; 
	pu8Y = pY;
	pu8V = pV;
	pu8U = pU;
	if (!pLMask && !pTMask) { /* No masking -- can do simple conversion */
		/* Loop through each block */
		for (i = 0; i < uRows; i += 4) {
			for (j=0; j < uCols; j+=4, pu8Out+=4, pu8Y+=4, pu8V++, pu8U++) {
		        /* get chroma with dither for entire block */
				u8V = *pu8V;
				u8U = *pu8U;
				uVUVUVUVU = *(puTblV + u8V) | *(puTblU + u8U);
	
				uVUa = (uVUVUVUVU&PEL0_BYTEMASK)>>PEL0_SHIFT;
				uVUb = (uVUVUVUVU&PEL1_BYTEMASK)>>PEL1_SHIFT;
				uVUc = (uVUVUVUVU&PEL2_BYTEMASK)>>PEL2_SHIFT;
				uVUd = (uVUVUVUVU&PEL3_BYTEMASK)>>PEL3_SHIFT; 

				/**** write row 0 of block */
				uY3 = MAKE_BYTE(pu8Y,  YDither00);
				uY2 = MAKE_BYTE(pu8Y+uRow0+1,YDither01);
				uY1 = MAKE_BYTE(pu8Y+uRow0+2,YDither02);
				uY0 = MAKE_BYTE(pu8Y+uRow0+3,YDither03);
				uOutputData = EVEN_ROW_PATTERN; 
				*((PU32)(pu8Out+uOutRow0)) = uOutputData;
		
				/**** write row 1 of block */
				uY3 = MAKE_BYTE(pu8Y+uRow1,  YDither10);
				uY2 = MAKE_BYTE(pu8Y+uRow1+1,YDither11);
				uY1 = MAKE_BYTE(pu8Y+uRow1+2,YDither12);
				uY0 = MAKE_BYTE(pu8Y+uRow1+3,YDither13);
				uOutputData = ODD_ROW_PATTERN; 
				*((PU32)(pu8Out+uOutRow1)) = uOutputData;
		
				/**** write row 2  of block */
				uY3 = MAKE_BYTE(pu8Y+uRow2,  YDither20);
				uY2 = MAKE_BYTE(pu8Y+uRow2+1,YDither21);
				uY1 = MAKE_BYTE(pu8Y+uRow2+2,YDither22);
				uY0 = MAKE_BYTE(pu8Y+uRow2+3,YDither23);
				uOutputData = EVEN_ROW_PATTERN; 
				*((PU32)(pu8Out+uOutRow2)) = uOutputData;
		
				/**** write row 3  of block */
				uY3 = MAKE_BYTE(pu8Y+uRow3,  YDither30);
				uY2 = MAKE_BYTE(pu8Y+uRow3+1,YDither31);
				uY1 = MAKE_BYTE(pu8Y+uRow3+2,YDither32);
				uY0 = MAKE_BYTE(pu8Y+uRow3+3,YDither33);
				uOutputData = ODD_ROW_PATTERN; 
				*((PU32)(pu8Out+uOutRow3)) = uOutputData;
			}	/* for j... blocks across row */
			pu8Out += uOutLineDelta;  /* Update to next 4 rows */
		    pu8Y += uYLineDelta;
		    pu8V += uVULineDelta;
		    pu8U += uVULineDelta;
		}	/* for i... rows of blocks */
	} else { /* Local decode, transparency, or both are present */

		PU8 pu8LD, pu8T;		/* Mobile pointers within masks */
		PU32 puLD, puT;			/* 32 bit mobile mask pointers, for ANDing */
		PU8 pu8LDBase, pu8TBase;/* Base pointers for the masks */
		U8  u8Mask;				/* byte of local decode  & transp mask */
		U8  u8LDNibble;			/* If non-zero, process through LD Mask */
		U32 uFillWord;			/* Fill Value in each byte */
		I32 iMaskAdjust;		/* Amount to adjust masks after 4 rows */
		U32 uMaskLineDelta;		/* Amount to adjust mask pointers in looping */
		Boo bOdd;				/* odd or even iteration of j? */
		Boo bBothValid;			/* Both Transparency and local decode */

		pu8LD = pLMask;
		if (pTMask)
			pu8T = pTMask;
		else 
			pu8T = pLMask; /* Guarantee this pointer is valid */
		pu8TBase  = pu8T;   
		pu8LDBase =  pu8LD;
		uFillWord =  uFillValue      | (uFillValue<<8) |
					(uFillValue<<16) | (uFillValue<<24);
		iMaskAdjust = - (I32)(3*uMStride);	  /* Back up 3 rows */
		uMaskLineDelta = (4*uMStride) - (uCols/8);
		bBothValid = (pTMask && pLMask);

		/* AND together local decode and transparency masks into transparency
		 * buffer.  This produces a mask that specifies which pixels to 
		 * color convert.  If fill is on, still need to compare to LDMask.
		 */
		if (bBothValid)	{
			puT = (PU32)pTMask;
			puLD = (PU32)pLMask;
			for (i=0; i < (uRows*uMStride)/4; i++, puT++, puLD++) 
				*puT = *puT & *puLD;
		}

		for (i = 0; i < uRows; i += 4) {
			bOdd = 0;     /* First row is not odd */
			for (j=0; j < uCols; j+=4, pu8Out+=4, pu8Y+=4, pu8V++, pu8U++) {
		        /* get chroma with dither for entire block */
				u8V = *pu8V;
				u8U = *pu8U;
				uVUVUVUVU = *(puTblV + u8V) | *(puTblU + u8U);

				uVUa = (uVUVUVUVU&PEL0_BYTEMASK)>>PEL0_SHIFT;
				uVUb = (uVUVUVUVU&PEL1_BYTEMASK)>>PEL1_SHIFT;
				uVUc = (uVUVUVUVU&PEL2_BYTEMASK)>>PEL2_SHIFT;
				uVUd = (uVUVUVUVU&PEL3_BYTEMASK)>>PEL3_SHIFT; 

				/**** write row 0 of block */
				uY3 = MAKE_BYTE(pu8Y,  YDither00);
				uY2 = MAKE_BYTE(pu8Y+uRow0+1,YDither01);
				uY1 = MAKE_BYTE(pu8Y+uRow0+2,YDither02);
				uY0 = MAKE_BYTE(pu8Y+uRow0+3,YDither03);
		
				u8Mask = bOdd ? (*pu8T) & 0xF : (*pu8T)>>4;
				u8LDNibble = pu8LD ? (bOdd ? (*pu8LD) & 0xF : (*pu8LD)>>4) : 0;
		
				if (u8Mask == 0xF) 			   /* Render all 4 pels? */
					*((PU32)(pu8Out+uOutRow0)) = EVEN_ROW_PATTERN;
				else if (u8Mask) {   				   /* Some pixels valid? */
					uOutputData = EVEN_ROW_PATTERN; 
					SELECTIVE_UPDATE(pu8Out+uOutRow0)            
				} else if (bWantsFill && (!pu8LD || u8LDNibble==0xF))	
					*((PU32)(pu8Out+uOutRow0)) = uFillWord; 
				else if (bWantsFill && u8LDNibble)	  /* Any filling needed? */
					SELECTIVE_FILL(pu8Out+uOutRow0)

				/**** write row 1 of block */
				uY3 = MAKE_BYTE(pu8Y+uRow1,  YDither10);
				uY2 = MAKE_BYTE(pu8Y+uRow1+1,YDither11);
				uY1 = MAKE_BYTE(pu8Y+uRow1+2,YDither12);
				uY0 = MAKE_BYTE(pu8Y+uRow1+3,YDither13);
		
				u8Mask = bOdd ? *(pu8T + (1*uMStride)) & 0xF : 
							    *(pu8T + (1*uMStride)) >>4;
				u8LDNibble = pu8LD ? (bOdd ? *(pu8LD + (1*uMStride)) & 0xF : 
										     *(pu8LD + (1*uMStride))>>4 ) : 0;

				if (u8Mask == 0xF) 			   /* Render all 4 pels? */
					*((PU32)(pu8Out+uOutRow1)) = ODD_ROW_PATTERN;
				else if (u8Mask) {   				   /* Some pixels valid? */
					uOutputData = ODD_ROW_PATTERN; 
					SELECTIVE_UPDATE(pu8Out+uOutRow1)            
				} else if (bWantsFill && (!pu8LD || u8LDNibble==0xF))	
					*((PU32)(pu8Out+uOutRow1)) = uFillWord; 
				else if (bWantsFill && u8LDNibble)	  /* Any filling needed? */
					SELECTIVE_FILL(pu8Out+uOutRow1)

				/**** write row 2  of block */
				uY3 = MAKE_BYTE(pu8Y+uRow2,  YDither20);
				uY2 = MAKE_BYTE(pu8Y+uRow2+1,YDither21);
				uY1 = MAKE_BYTE(pu8Y+uRow2+2,YDither22);
				uY0 = MAKE_BYTE(pu8Y+uRow2+3,YDither23);
		
				u8Mask = bOdd ? *(pu8T + (2*uMStride)) & 0xF : 
							    *(pu8T + (2*uMStride)) >>4;
				u8LDNibble = pu8LD ? (bOdd ? *(pu8LD + (2*uMStride)) & 0xF : 
										     *(pu8LD + (2*uMStride))>>4 ) : 0;
		
				if (u8Mask == 0xF) 			   /* Render all 4 pels? */
					*((PU32)(pu8Out+uOutRow2)) = EVEN_ROW_PATTERN;
				else if (u8Mask) {   				   /* Some pixels valid? */
					uOutputData = EVEN_ROW_PATTERN; 
					SELECTIVE_UPDATE(pu8Out+uOutRow2)            
				} else if (bWantsFill && (!pu8LD || u8LDNibble==0xF))	
					*((PU32)(pu8Out+uOutRow2)) = uFillWord; 
				else if (bWantsFill && u8LDNibble)	  /* Any filling needed? */
					SELECTIVE_FILL(pu8Out+uOutRow2)

				/**** write row 3  of block */
				uY3 = MAKE_BYTE(pu8Y+uRow3,  YDither30);
				uY2 = MAKE_BYTE(pu8Y+uRow3+1,YDither31);
				uY1 = MAKE_BYTE(pu8Y+uRow3+2,YDither32);
				uY0 = MAKE_BYTE(pu8Y+uRow3+3,YDither33);
		
				u8Mask = bOdd ? *(pu8T + (3*uMStride)) & 0xF : 
							    *(pu8T + (3*uMStride)) >>4;
				u8LDNibble = pu8LD ? (bOdd ? *(pu8LD + (3*uMStride)) & 0xF : 
										     *(pu8LD + (3*uMStride))>>4 ) : 0;

				if (u8Mask == 0xF) 			   /* Render all 4 pels? */
					*((PU32)(pu8Out+uOutRow3)) = ODD_ROW_PATTERN;
				else if (u8Mask) {   				   /* Some pixels valid? */
					uOutputData = ODD_ROW_PATTERN; 
					SELECTIVE_UPDATE(pu8Out+uOutRow3)            
				} else if (bWantsFill && (!pu8LD || u8LDNibble==0xF))	
					*((PU32)(pu8Out+uOutRow3)) = uFillWord; 
				else if (bWantsFill && u8LDNibble)	  /* Any filling needed? */
					SELECTIVE_FILL(pu8Out+uOutRow3)

				/* 8 bits for 8 pels, or two iterations through the loop, so
				 * only update on odd iterations */
				if (bOdd) {
					if (pu8LD) pu8LD++;
					pu8T++;
				}
				bOdd = !bOdd;
			}	/* for j... blocks across row */
			pu8Out += uOutLineDelta;  /* Update to next 4 rows */
		    pu8Y   += uYLineDelta;
		    pu8V   += uVULineDelta;
		    pu8U   += uVULineDelta;
			if (pu8LD) pu8LD  += uMaskLineDelta;
			pu8T   += uMaskLineDelta;
		}	/* for i... rows of blocks */
	}

	return;
}

