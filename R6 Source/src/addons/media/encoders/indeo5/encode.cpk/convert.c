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
*               Copyright (C) 1994-1997 Intel Corp.                     *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

/*
 *  Tabs set to 4
 *
 *  convert.c 
 *
 *  DESCRIPTION:
 *  Convert RGB8/16/24 to YVU9 planar.
 *
 *  Routines:                      
 *       ConvertToYVU9()        
 *       YUY2ToYVU9()
 *       YVU12ToYVU9()
 */                                                                           
#include "datatype.h"
#include "pia_main.h"
#include "icconv.h"
#include "pia_cin.h"                   /* ColorIn subsystem include file */
#include "convtabs.h"
#include "convert.h"


const I32 TRANSPARENCY_CUTOFF = 500;

const U32 uMaskarray[32] = {	0x00000080, 0x00000040, 0x00000020, 0x00000010,
 								0x00000008, 0x00000004, 0x00000002, 0x00000001,
								0x00008000, 0x00004000, 0x00002000, 0x00001000,
 								0x00000800, 0x00000400, 0x00000200, 0x00000100,
								0x00800000, 0x00400000, 0x00200000, 0x00100000,
 								0x00080000, 0x00040000, 0x00020000, 0x00010000,
								0x80000000, 0x40000000, 0x20000000, 0x10000000,
 								0x08000000, 0x04000000, 0x02000000, 0x01000000 };

/* private routine prototype */

static PIA_Boolean BuildTransparencyRange(U32 uThreshold, PU32 puBuckets,
								   PTR_COLOR_RANGE pTransparencyRanget);

static void SubSampling2YVU9(PTR_CCIN_INST pInst, PTR_COLORIN_INPUT_INFO pInput,
							 PTR_COLORIN_OUTPUT_INFO pOutput,
							 PU8 pu8TransPixels, U32 YAvg);

#ifdef SIMULATOR 
static void SubSampling2YVU12(PTR_CCIN_INST pInst, PTR_COLORIN_INPUT_INFO pInput,
							 PTR_COLORIN_OUTPUT_INFO pOutput,
							 PU8 pu8TransPixels);
#endif /* SIMULATOR */

static PIA_Boolean BuildTransparencyBitmask(PTR_CCIN_INST pInst, PTR_COLORIN_INPUT_INFO pInput,
									 PU8 pu8TransPixels);

static PIA_Boolean BuildTransparencyBytemask(PTR_CCIN_INST pInst, PTR_COLORIN_INPUT_INFO pInput,
									  PU8 pu8TransPixels);


#ifdef BETA_BANNER

static void InsertBetaBanner(PTR_CCIN_INST pInst,
							 PTR_COLORIN_OUTPUT_INFO pOutput,
							 PU8 pu8TransPixels);

#endif /* BETA_BANNER */


/*************************************************************************
           PUBLIC ROUTINES
*************************************************************************/

/***************************************************************************
 *
 * ConvertToYVU9() is called from ColorInFrame()
 *
 * Convert the passed bitmap into non-subsampled YUV
 * and place in the current context
 *
 * returns nothing
 *
 ***************************************************************************/

PIA_RETURN_STATUS ConvertToYVU9(PTR_CCIN_INST pInst,
                      PTR_COLORIN_INPUT_INFO pInput,
                      PTR_COLORIN_OUTPUT_INFO pOutput
                     )
{                                                
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
    PTR_PRIVATE_COLORIN_DATA   pPrivate = pInst->pColorInPrivate;                     
    PIA_RETURN_STATUS  prsRtn = PIA_S_OK;
    I32                iX, iY; 		 	 /* loop counters */	
    I32                iWidth, iHeight;	 /* resolution of output frame */
    U32                uBytePerScan;     /* number of bytes in a scan line */
    PU8                pu8Byte;        	 /* pointer to the bytes in the input bitmap */
    PU8                pu8YPlace;        /* pointer to the place storing Y pixels */
    PU8                pu8InputPixels;   /* pointer to input data stream */
	PU8				   pu8TransPixels;   /* pointer to transparency pixels */
    PU8                pu8UVPlace;       /* byte-wide pointer to non-subsampled UV for both Endians */
    U32                uYVUval;
	U32				   uYAvg, uYCnt;
    U8                 u8Rt,u8Gt,u8Bt;   /* temps while converting from rgb to yuv */
    U8                 u8BlueMin, u8BlueMax, u8GreenMin, u8GreenMax, u8RedMin, u8RedMax;
    U32                uThreshold;
    PU32               puRbucket, puGbucket, puBbucket;
	COLORIN_SEQUENCE_CONTROLS ciscSeqCtlUsed = pInst->ciscSequenceControlsUsed;
    PU8					pu8ThisColor;	/* pointer to individual color in color table */
    PTR_BGR_ENTRY		pbeCTable;		/* pointer to base of color table */
	PIA_Boolean			bUpsideDown;	/* Flag which signals an Upside down DIB. TRUE = Upside down */
	I32					iLine;			/* Looping variable that keeps track of image scanline number */

	/*------------------------- End of Declarations -------------------------*/

    pu8InputPixels = pInput->pu8InputData;
    pu8YPlace = pOutput->pu8ExternalOutputData ? pOutput->pu8ExternalOutputData : pOutput->pu8OutputData;
    pu8UVPlace = pPrivate->pu8TempBuffer;

    uBytePerScan = (U32)pInput->iInputStride;               
    iWidth  = (int)pInst->dImageDim.w;
    iHeight = (int)pInst->dImageDim.h;

	bUpsideDown = pInput->bFlipImage;  /*/ Set upside down flag here from passed struct.*/

	uYAvg = 0;
	uYCnt = 0;

/* First we do the non-subsampled Y and UV components
 * Under VfW the DIBs are "upside down" (stored in rows beginning with the
 * the bottom one).  This code turns the information over so the on the basis
 * of a flag passed by HIVE so that the YUV information is always stored with 
 * the orgin in the upper left.
 */
    switch (pInput->cfSrcFormat) {
		
		case CF_RGB16_565:
		/* Convert the inverted DIB to separate arrays, one containing the Y values in
		 * consecutive bytes, and one containing V and U in consecutive byte pairs:
		 *  Bit:   15 - 8   7 - 0	(assuming little-endian)
		 *       +-------+-------+
		 *       |   V   |   U   |
		 *       +-------+-------+
		 *
		 * These arrays are "rightside up" - that is, the upper left corner of the frame
		 * is in the lowest addresses of the arrays, and succeeding scan lines are stored
		 * left-to-right in successively higher addresses.
		 *
		 * This pass over all of the input pixels is the only one necessary, unless we are
		 * building the transparency range and building/using the mask in the same call. 
		 *
		 * There are, therefore, four modes for this conversion:
		 *	1. Building the transparency range, for which the YVU9 output will consist of every
		 *     pixel set to the YVU24 equivalent of "bgrRepresentativeColor", currently BLACK;
		 *     In this mode, CIN_BITMASK_TRANSPARENCY is off, and we won't build the mask.
		 *
		 *	2. Transparency processing, in which the transparency mask is created, and where the
		 *     output is the YUV24 equivalent of the input pixels, except where the mask indicates
		 *     a pixel is transparent, in which case the output pixel is set to the YVU24
		 *     equivalent of "bgrRepresentativeColor", currently BLACK;
		 *
		 *	3. A combination of (1) and (2), in which we build the range on one pass through the
		 *     input data, and then process that same input data on a second pass. This second
		 *     pass is provided by falling through to the (2) processing in the switch below.
		 *
		 *	4. No transparency processing, for which the output will simply be the YVU24
		 *     equivalent of the input image's RGB values.
		 */

			if (ciscSeqCtlUsed & CIN_TRANSPARENCY) {

				if (pParms->bBuildColorRange) {

					/* We're constructing the transparency range. This allocates memory for	a
					 * histogram of the R, G, and B components in this frame, and selects the
					 * threshold for noise rejection.
					 */

					uThreshold = (U32)iWidth*iHeight/TRANSPARENCY_CUTOFF;

					/* allocate memory for R, G, B buckets, each chunk size = 256*4, total=3K */
					puRbucket = HiveLocalAllocPtr(3*256*4, TRUE);

					if(!puRbucket) {
						prsRtn = PIA_S_OUT_OF_MEMORY; 
						goto bail;
					}        

					puGbucket = puRbucket+256;
					puBbucket = puRbucket+512;

					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {

							/* Form discrete R, G, and B values for use as indices */
							u8Bt = *pu8Byte++;
							u8Rt = *pu8Byte++;
							u8Gt = (u8Bt & 0xe0) >> 2;
							u8Gt |= (u8Rt & 0x03) << 6;
							u8Rt = (u8Rt & 0x7c) << 1;
							u8Bt = (u8Bt & 0x1f) << 3;

							/* build RGB histogram */
							puBbucket[u8Bt]++;
							puGbucket[u8Gt]++;
							puRbucket[u8Rt]++;

						} /* for (iX... */
					} /* for (iLine... */

					BuildTransparencyRange(uThreshold, puRbucket, &pParms->crTransColorRange);

					pParms->bBuildColorRange = FALSE;	   /* turn it off for the subsequent frames */

					/* free the memory that was allocated for the histogram */                     
					HiveLocalFreePtr(puRbucket);
				}

				if (pParms->bBuildBitmask) { /* Build the mask from the color range */

					/* Transparency processing:
					 *	Build the transparency mask by analyzing the frame for colors lying within the
					 *  transparency range.  Wherever such is found, put grey in the output. Otherwise,
					 *  convert the input from RGB to YVU.
			 		*/

					/* Calc ptr for transparency pixels temp space */
					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;	

	       			/* Make local copies of range limits */
    	   			u8BlueMax  = pParms->crTransColorRange.u8BlueHigh;
        	   		u8BlueMin  = pParms->crTransColorRange.u8BlueLow;
	           		u8GreenMax = pParms->crTransColorRange.u8GreenHigh;
    	       		u8GreenMin = pParms->crTransColorRange.u8GreenLow;
        	   		u8RedMax   = pParms->crTransColorRange.u8RedHigh;
           			u8RedMin   = pParms->crTransColorRange.u8RedLow;

					/* Set the transparency-representative color in the output context */
					pParms->bgrRepresentativeColor.u8R = 
	   					(U8)(((U16)pParms->crTransColorRange.u8RedHigh +
 							  (U16)pParms->crTransColorRange.u8RedLow) >> 1);
					pParms->bgrRepresentativeColor.u8G = 
  						(U8)(((U16)pParms->crTransColorRange.u8GreenHigh +
							  (U16)pParms->crTransColorRange.u8GreenLow) >> 1);
					pParms->bgrRepresentativeColor.u8B = 
   						(U8)(((U16)pParms->crTransColorRange.u8BlueHigh +
							  (U16)pParms->crTransColorRange.u8BlueLow) >> 1);

					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {

							/* Form discrete R, G, and B values for use as indices */

							u8Bt = *pu8Byte++;
							u8Rt = *pu8Byte++;
							u8Gt = (u8Bt & 0xe0) >> 2;
							u8Gt |= (u8Rt & 0x03) << 6;
							u8Rt = (u8Rt & 0x7c) << 1;
							u8Bt = (u8Bt & 0x1f) << 3;

							/* Test to see if this pixel is opaque (outside the transparency range) */

							if ( u8Bt < u8BlueMin  || u8Bt > u8BlueMax  ||
								 u8Gt < u8GreenMin || u8Gt > u8GreenMax ||
								 u8Rt < u8RedMin   || u8Rt > u8RedMax ) 
					
							{
								/* If opaque, use the given values, and set the mask byte */
								*(pu8TransPixels++) = 1;

								/* Sum the translations of R, G, and B components to form a YVU24 value */
								uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];
								
								/* Only Accumulate the Y Avg for Opaque Pixels */
								uYAvg += (U32)(uYVUval >> 16);
								uYCnt++;
								
							} else { /* If transparent, fill with Grey */
								*(pu8TransPixels++) = 0;
								u8Bt = u8Rt = u8Gt = 0;	

								/* Sum the translations of R, G, and B components to form a YVU24 value */
								uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];
							}

							/* Store Y-component and U-V components in their separate arrays. */
							*pu8UVPlace++ = (U8)uYVUval;
							*pu8UVPlace++ = (U8)(uYVUval >> 8);
							*pu8YPlace++  = (U8)(uYVUval >> 16);
						} /* for (iX... */
					} /* for (iLine ... */

					/* Now, convert the transparency array to a bitmask */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;

					if (!BuildTransparencyBitmask(pInst, pInput, pu8TransPixels)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}

				} 	else  {		/* The mask comes from the external */

					/*  
					 *	Just convert each pixel from RGB to YVU...  grey out transparent pixels.
					 */

					/* If the transparency mask is passed in from the encoding app, */
					/* then convert it from 1bpp to 8bpp for subsampling. */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
					if (!BuildTransparencyBytemask(pInst, pInput, pu8TransPixels)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}

					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {

							/* Form discrete R, G, and B values for use as indices */
							u8Bt = *pu8Byte++;
							u8Rt = *pu8Byte++;

							if (!*pu8TransPixels++) {  /* If pixel is transparent, make it grey */
								u8Gt = 128;
								u8Rt = 128;
								u8Bt = 128;

								/* Sum the translations of R, G, and B components to form a YVU24 value */
								uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

							}
							else {
								u8Gt = (u8Bt & 0xe0) >> 2;	/* Else, complete getting the colors */
								u8Gt |= (u8Rt & 0x03) << 6;
								u8Rt = (u8Rt & 0x7c) << 1;
								u8Bt = (u8Bt & 0x1f) << 3;

								/* Sum the translations of R, G, and B components to form a YVU24 value */
								uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

								uYAvg += (U32)(uYVUval >> 16);
								uYCnt++;
							}

							/* Store Y-component and U-V components in their separate arrays. */
							*pu8UVPlace++ = (U8) uYVUval;
							*pu8UVPlace++ = (U8)(uYVUval >> 8);
							*pu8YPlace++  = (U8)(uYVUval >> 16);
						} /* for (iX... */
					} /* for (iLine ... */

					/* Reset the pointer to what it should be */
					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
			}
		}  /* end of if (transparency) */	else {

			/* No transparency processing:
			 *	Just convert each pixel from RGB to YVU...
			 */

			for (iLine = 0; iLine < iHeight; iLine++) {
				iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
				pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
				for (iX = 0; iX < iWidth; iX++) {

					/* Form discrete R, G, and B values for use as indices */
					u8Bt = *pu8Byte++;
					u8Rt = *pu8Byte++;
					u8Gt = (u8Bt & 0xe0) >> 2;
					u8Gt |= (u8Rt & 0x03) << 6;
					u8Rt = (u8Rt & 0x7c) << 1;
					u8Bt = (u8Bt & 0x1f) << 3;

					/* Sum the translations of R, G, and B components to form a YVU24 value */
					uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

					/* Store Y-component and U-V components in their separate arrays. */
					*pu8UVPlace++ = (U8) uYVUval;
					*pu8UVPlace++ = (U8)(uYVUval >> 8);
					*pu8YPlace++  = (U8)(uYVUval >> 16);
				} /* for (iX... */
			} /* for (iLine ... */
		}

		break;	/* end of case: CF_RGB16_565 */

		case CF_CLUT_8:

			/* Other than the means by which the RGB value is developed, this conversion is
			 * identical to the RGB16_565 conversion above.
			 */

			/* get palette entry */
			if(!pInput->ppalCurrentPalette)	{
				prsRtn = PIA_S_BAD_SETUP_VALUE; 
				goto bail;
			}

			pbeCTable = &(pInput->ppalCurrentPalette->pbgrTable[0]);
			
			if (ciscSeqCtlUsed & CIN_TRANSPARENCY) {

				if (pParms->bBuildColorRange) {


					/* We're constructing the transparency range. This allocates memory for	a
					 * histogram of the R, G, and B components in this frame, and selects the
					 * threshold for noise rejection.
					 */

					uThreshold = (U32)iWidth*iHeight/TRANSPARENCY_CUTOFF;

					/* allocate memory for R, G, B buckets, each chunk size = 256*4, total=3K */
					puRbucket = HiveLocalAllocPtr(3*256*4, TRUE);

					if(!puRbucket) {
						prsRtn = PIA_S_OUT_OF_MEMORY; 
						goto bail;
					}

					puGbucket = puRbucket+256;
					puBbucket = puRbucket+512;
			
					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {
							/* Form discrete R, G, and B values for use as indices */
							pu8ThisColor = (PU8) (pbeCTable + *pu8Byte++);
							u8Bt = *(pu8ThisColor++);
							u8Gt = *(pu8ThisColor++);
							u8Rt = *(pu8ThisColor);

							/* build RGB histogram */
							puBbucket[u8Bt]++;
							puGbucket[u8Gt]++;
							puRbucket[u8Rt]++;
						
						} /* for (iX... */
					} /* for (iLine... */

					if (!BuildTransparencyRange(uThreshold, puRbucket, &pParms->crTransColorRange)) {
						prsRtn=FALSE;
						goto bail;
					}

					pParms->bBuildColorRange = FALSE;	   /* turn it off for the subsequent frames */
					
					/* free the memory that was allocated for the histogram */                     
					HiveLocalFreePtr(puRbucket);
				
				}

			if (pParms->bBuildBitmask) {

				/* Transparency processing:
				 *	Build the transparency mask by analyzing the frame for colors lying within the
				 *  transparency range.  Wherever such is found, put grey in the output. Otherwise,
				 *  convert the input from RGB to YVU.
				 */

				/* Calc ptr for transparency pixels temp space */
				pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;	

	       		/* Make local copies of range limits */
    	   		u8BlueMax  = pParms->crTransColorRange.u8BlueHigh;
        	   	u8BlueMin  = pParms->crTransColorRange.u8BlueLow;
           		u8GreenMax = pParms->crTransColorRange.u8GreenHigh;
	           	u8GreenMin = pParms->crTransColorRange.u8GreenLow;
    	       	u8RedMax   = pParms->crTransColorRange.u8RedHigh;
        	   	u8RedMin   = pParms->crTransColorRange.u8RedLow;
				
			    /* Set the transparency-representative color  */
			    pParms->bgrRepresentativeColor.u8R = 
   					(U8)(((U16)pParms->crTransColorRange.u8RedHigh +
 					      (U16)pParms->crTransColorRange.u8RedLow) >> 1);
				pParms->bgrRepresentativeColor.u8G = 
  					(U8)(((U16)pParms->crTransColorRange.u8GreenHigh +
					      (U16)pParms->crTransColorRange.u8GreenLow) >> 1);
				pParms->bgrRepresentativeColor.u8B = 
   					(U8)(((U16)pParms->crTransColorRange.u8BlueHigh +
					      (U16)pParms->crTransColorRange.u8BlueLow) >> 1);

				for (iLine = 0; iLine < iHeight; iLine++) {
					iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
					pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
					for (iX = 0; iX < iWidth; iX++) {

						/* Form discrete R, G, and B values for use as indices */
						pu8ThisColor = (PU8) (pbeCTable + *pu8Byte++);
						u8Bt = *(pu8ThisColor++);
						u8Gt = *(pu8ThisColor++);
						u8Rt = *(pu8ThisColor);

						/* Test to see if this pixel is opaque (outside the transparency range) */

						if ( u8Bt < u8BlueMin  || u8Bt > u8BlueMax  ||
							 u8Gt < u8GreenMin || u8Gt > u8GreenMax ||
							 u8Rt < u8RedMin   || u8Rt > u8RedMax ) 

							{
							/* If opaque, use the given values, and set the mask byte */
							*(pu8TransPixels++) = 1;
							/* Sum the translations of R, G, and B components to form a YVU24 value */
							uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];
							uYAvg += (U32)(uYVUval >> 16);
							uYCnt++;
							
						} else { /* If transparent, fill with grey */
							*(pu8TransPixels++) = 0;
							u8Bt = u8Rt = u8Gt = 128;	/* Grey, for now */
							/* Sum the translations of R, G, and B components to form a YVU24 value */
							uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];
						}

						/* Store Y-component and U-V components in their separate arrays. */
						*pu8UVPlace++ = (U8)uYVUval;
						*pu8UVPlace++ = (U8)(uYVUval >> 8);
						*pu8YPlace++  = (U8)(uYVUval >> 16);
					} /* for (iX... */
				} /* for (iLine ... */

				/* Now, convert the transparency array to a bitmask */

				pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;

				if (!BuildTransparencyBitmask(pInst, pInput, pu8TransPixels)) {
					prsRtn = PIA_S_ERROR;
					goto bail;
				}

			} else  {	 /* read the mask from the external input */

				/*  
				 *	Just convert each pixel from RGB to YVU...  Grey out transparent pixels.
				 */

				/* If the transparency mask is passed in from the encoding app, */
				/* then convert it from 1bpp to 8bpp for subsampling. */

				pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
				if (!BuildTransparencyBytemask(pInst, pInput, pu8TransPixels)) {
					prsRtn = PIA_S_ERROR;
					goto bail;
				}

				for (iLine = 0; iLine < iHeight; iLine++) {
					iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
					pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
					for (iX = 0; iX < iWidth; iX++) {

						/* Form discrete R, G, and B values for use as indices */
						pu8ThisColor = (PU8) (pbeCTable + *pu8Byte++);

						if (!*pu8TransPixels++) {  /* If pixel is transparent, make it grey */
							u8Gt = 128;
							u8Rt = 128;
							u8Bt = 128;
							/* Sum the translations of R, G, and B components to form a YVU24 value */
							uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];
						}
						else {
							u8Bt = *(pu8ThisColor++);
							u8Gt = *(pu8ThisColor++);
							u8Rt = *(pu8ThisColor);
							/* Sum the translations of R, G, and B components to form a YVU24 value */
							uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

							uYAvg += (U32)(uYVUval >> 16);
							uYCnt++;
						}

						/* Store Y-component and U-V components in their separate arrays. */
						*pu8UVPlace++ = (U8) uYVUval;
						*pu8UVPlace++ = (U8)(uYVUval >> 8);
						*pu8YPlace++  = (U8)(uYVUval >> 16);
					} /* for (iX... */
				} /* for (iLine ... */

				/* Reset the pointer to what it should be */
				pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
			}
		}	else {	/* If transparency is not on */

			/* No transparency processing:
			 *	Just convert each pixel from RGB to YVU...
			 */

			for (iLine = 0; iLine < iHeight; iLine++) {
				iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
				pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
				for (iX = 0; iX < iWidth; iX++) {

					/* Form discrete R, G, and B values for use as indices */
					pu8ThisColor = (PU8) (pbeCTable + *pu8Byte++);
					u8Bt = *(pu8ThisColor++);
					u8Gt = *(pu8ThisColor++);
					u8Rt = *(pu8ThisColor);

					/* Sum the translations of R, G, and B components to form a YVU24 value */
					uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

					/* Store Y-component and U-V components in their separate arrays. */
					*pu8UVPlace++ = (U8) uYVUval;
					*pu8UVPlace++ = (U8)(uYVUval >> 8);
					*pu8YPlace++  = (U8)(uYVUval >> 16);
				} /* for (iX... */
			} /* for (iLine ... */
		}

		break;	/* end of case: CF_CLUT_8 */

        case CF_BGR24:

			 if (ciscSeqCtlUsed & CIN_TRANSPARENCY) {

				 if (pParms->bBuildColorRange) {

               		uThreshold = (U32)iWidth*iHeight/TRANSPARENCY_CUTOFF;
                    
               		/* allocate memory for R, G, B buckets, each chunk size = 256*4 
              		 * total size = 3k 
               		 */
                     		 
               		puRbucket = HiveLocalAllocPtr(3*256*4, TRUE);
               		/* check if allocated memory is OK */
               		if(!puRbucket) {
                   		prsRtn = PIA_S_OUT_OF_MEMORY; 
                   		goto bail;
               		}        
					puGbucket = puRbucket+256;
               		puBbucket = puRbucket+512;

					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
                   		pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
                   		for (iX = 0; iX < iWidth; iX++) {
                  			/* build RGB histogram */
                       		puBbucket[*(pu8Byte++)]++;
                       		puGbucket[*(pu8Byte++)]++;
   	                   		puRbucket[*(pu8Byte++)]++;
       					} /* for (iX ... */
           			} /* for (iLine ... */

               		/* build RGB transparency range */
               		if (!BuildTransparencyRange(uThreshold, puRbucket, &pParms->crTransColorRange)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}

					pParms->bBuildColorRange = FALSE;	   /* turn it off for the subsequent frames */
					
        	   		/* free allocated memory */                     
               		HiveLocalFreePtr(puRbucket);
				}

				if (pParms->bBuildBitmask) {

					/* Calc ptr for transparency pixels temp space */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;	

          			u8BlueMax  = pParms->crTransColorRange.u8BlueHigh;
              		u8BlueMin  = pParms->crTransColorRange.u8BlueLow;
              		u8GreenMax = pParms->crTransColorRange.u8GreenHigh;
              		u8GreenMin = pParms->crTransColorRange.u8GreenLow;
              		u8RedMax   = pParms->crTransColorRange.u8RedHigh;
              		u8RedMin   = pParms->crTransColorRange.u8RedLow;

					pParms->bgrRepresentativeColor.u8R = 
    					(U8)(((U16)pParms->crTransColorRange.u8RedHigh +
 	 						  (U16)pParms->crTransColorRange.u8RedLow) >> 1);
   					pParms->bgrRepresentativeColor.u8G = 
    					(U8)(((U16)pParms->crTransColorRange.u8GreenHigh +
							  (U16)pParms->crTransColorRange.u8GreenLow) >> 1);
   					pParms->bgrRepresentativeColor.u8B = 
    					(U8)(((U16)pParms->crTransColorRange.u8BlueHigh +
							  (U16)pParms->crTransColorRange.u8BlueLow) >> 1);

					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {
							u8Bt = *(pu8Byte++);
							u8Gt = *(pu8Byte++);
							u8Rt = *(pu8Byte++);

							/* Test to see if this pixel is opaque */

                       		if (u8Bt < u8BlueMin  || u8Bt > u8BlueMax  ||
                       			u8Gt < u8GreenMin || u8Gt > u8GreenMax ||
								u8Rt < u8RedMin   || u8Rt > u8RedMax) {

								/* If opaque, use the given values, and set the mask byte */
								*(pu8TransPixels++) = 1;
								/* Sum the translations of R, G, and B components to form a YVU24 value */
								uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];
								uYAvg += (U32)(uYVUval >> 16);							
								uYCnt++;
							} else { /* If transparent, fill with grey */
								*(pu8TransPixels++) = 0;
					  			u8Bt = u8Rt = u8Gt = 128;
								/* Sum the translations of R, G, and B components to form a YVU24 value */
								uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];
							}

							/* Calculate and write out new Y U V values */

							/* Store Y-component and U-V components in their separate arrays. */
							*pu8UVPlace++ = (U8)uYVUval;
							*pu8UVPlace++ = (U8)(uYVUval >> 8);
							*pu8YPlace++  = (U8)(uYVUval >> 16);
				
						} /* for (iX ... */
					} /* for (iLine ... */

					/* Now, convert the transparency array to a bitmask */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;

					if (!BuildTransparencyBitmask(pInst, pInput, pu8TransPixels)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}


				}	else  {	   /* read the transparency mask  */

					/*  
					 *	Just convert each pixel from RGB to YVU...  Grey out transparent pixels.
					 */

					/* If the transparency mask is passed in from the encoding app, */
					/* then convert it from 1bpp to 8bpp for subsampling. */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
					if (!BuildTransparencyBytemask(pInst, pInput, pu8TransPixels)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}


					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {

							/* Form discrete R, G, and B values for use as indices */

								u8Bt = *(pu8Byte++);
								u8Gt = *(pu8Byte++);
								u8Rt = *(pu8Byte++);

							if (!*pu8TransPixels++) {  /* If pixel is transparent, make it grey */
								u8Gt = 128;
								u8Rt = 128;
								u8Bt = 128;
								/* Sum the translations of R, G, and B components to form a YVU24 value */
								uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];
							} else {
								/* Sum the translations of R, G, and B components to form a YVU24 value */
								uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];
								uYAvg += (U8)(uYVUval >> 16);
								uYCnt++;
							}

							/* Store Y-component and U-V components in their separate arrays. */
							*pu8UVPlace++ = (U8) uYVUval;
							*pu8UVPlace++ = (U8)(uYVUval >> 8);
							*pu8YPlace++  = (U8)(uYVUval >> 16);
						} /* for (iX... */
					} /* for (iLine ... */

					/* Reset the pointer to what it should be */
					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
				}

			} else { /* If transparency is not on */

				for (iLine = 0; iLine < iHeight; iLine++) {
					iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
					pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
					for (iX = 0; iX < iWidth; iX++) {
						uYVUval  = BtoYUV[*(pu8Byte++)];
						uYVUval += GtoYUV[*(pu8Byte++)];
						uYVUval += RtoYUV[*(pu8Byte++)];
						*pu8UVPlace++ = (U8)uYVUval;
						*pu8UVPlace++ = (U8)(uYVUval >> 8);
						*pu8YPlace++  = (U8)(uYVUval >> 16);
					} /* for (iX ... */
				} /* for (iLine ... */
			}
		break;

        case CF_XRGB32:

			 if (ciscSeqCtlUsed & CIN_TRANSPARENCY) {

				 if (pParms->bAlphaChannel) {
					/*  
					 *	Just convert each pixel from RGB to YVU...  Get Transparent Pixels from extra byte
					 */

					/* Calc ptr for transparency pixels temp space */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;	

					/* If the transparency mask is passed in from the encoding app */
					/* in the extra byte of the RGB32, then convert it from 8bpp alpha
					/* channel to 8bpp transparency for subsampling. */

					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY*uBytePerScan));
						for (iX=0; iX< iWidth; iX++) {

							/* Form discrete R, G, and B values for use as indices */

								u8Bt = *(pu8Byte++);
								u8Gt = *(pu8Byte++);
								u8Rt = *(pu8Byte++);

							/* Test to see if this pixel is opaque, do this by splitting the alpha */
							/* Channel down the center, 0-127 = Transparent, 128-255 = opaque. */

                       		if (*(pu8Byte++) > 127) {
 								/* If opaque, use the given values, and set the mask byte */
								*(pu8TransPixels++) = 1;
								/* Sum the translations of R, G, and B components to form a YVU24 value */
								uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];
								uYAvg += (U8)(uYVUval >> 16);
								uYCnt++;
							}
							else { /* If transparent, fill with grey */
								*(pu8TransPixels++) = 0;
					  			u8Bt = u8Rt = u8Gt = 128;
								/* Sum the translations of R, G, and B components to form a YVU24 value */
								uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];
							}

							/* Store Y-component and U-V components in their separate arrays. */
							*pu8UVPlace++ = (U8) uYVUval;
							*pu8UVPlace++ = (U8)(uYVUval >> 8);
							*pu8YPlace++  = (U8)(uYVUval >> 16);
						} /* for (iX... */
					} /* for (iLine ... */

					/* Now, convert the transparency array to a bitmask */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;

					if (!BuildTransparencyBitmask(pInst, pInput, pu8TransPixels)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}

				}	else {


					 if (pParms->bBuildColorRange) {


               			uThreshold = (U32)iWidth*iHeight/TRANSPARENCY_CUTOFF;
                    
               			/* allocate memory for R, G, B buckets, each chunk size = 256*4 
              			 * total size = 3k 
               			 */
                     			 
               			puRbucket = HiveLocalAllocPtr(3*256*4, TRUE);
               			/* check if allocated memory is OK */
               			if(!puRbucket) {
                   			prsRtn = PIA_S_OUT_OF_MEMORY; 
                   			goto bail;
               			}        
						puGbucket = puRbucket+256;
               			puBbucket = puRbucket+512;

						for (iLine = 0; iLine < iHeight; iLine++) {
							iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
                   			pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
                   			for (iX = 0; iX < iWidth; iX++) {
                  				/* build RGB histogram */
                       			puBbucket[*(pu8Byte++)]++;
                       			puGbucket[*(pu8Byte++)]++;
   	                   			puRbucket[*(pu8Byte++)]++;
								pu8Byte++;  /* Skip Over X byte */
       						} /* for (iX ... */
           				} /* for (iLine ... */

               			/* build RGB transparency range */
               			if (!BuildTransparencyRange(uThreshold, puRbucket, &pParms->crTransColorRange)) {
							prsRtn = PIA_S_ERROR;
							goto bail;
						}

						pParms->bBuildColorRange = FALSE;	   /* turn it off for the subsequent frames */

						/* free allocated memory */                     
               			HiveLocalFreePtr(puRbucket);
					}

					if (pParms->bBuildBitmask) {

						/* Calc ptr for transparency pixels temp space */

						pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;	

          				u8BlueMax  = pParms->crTransColorRange.u8BlueHigh;
              			u8BlueMin  = pParms->crTransColorRange.u8BlueLow;
              			u8GreenMax = pParms->crTransColorRange.u8GreenHigh;
              			u8GreenMin = pParms->crTransColorRange.u8GreenLow;
              			u8RedMax   = pParms->crTransColorRange.u8RedHigh;
              			u8RedMin   = pParms->crTransColorRange.u8RedLow;

						pParms->bgrRepresentativeColor.u8R = 
    						(U8)(((U16)pParms->crTransColorRange.u8RedHigh +
 	 							  (U16)pParms->crTransColorRange.u8RedLow) >> 1);
   						pParms->bgrRepresentativeColor.u8G = 
    						(U8)(((U16)pParms->crTransColorRange.u8GreenHigh +
								  (U16)pParms->crTransColorRange.u8GreenLow) >> 1);
   						pParms->bgrRepresentativeColor.u8B = 
    						(U8)(((U16)pParms->crTransColorRange.u8BlueHigh +
								  (U16)pParms->crTransColorRange.u8BlueLow) >> 1);

						for (iLine = 0; iLine < iHeight; iLine++) {
							iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
							pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
							for (iX = 0; iX < iWidth; iX++) {

								u8Bt = *(pu8Byte++);
								u8Gt = *(pu8Byte++);
								u8Rt = *(pu8Byte++);
								pu8Byte++;

								/* Test to see if this pixel is opaque */

                       			if (u8Bt < u8BlueMin  || u8Bt > u8BlueMax  ||
                       				u8Gt < u8GreenMin || u8Gt > u8GreenMax ||
									u8Rt < u8RedMin   || u8Rt > u8RedMax) {

									/* If opaque, use the given values, and set the mask byte */
									*(pu8TransPixels++) = 1;
									/* Sum the translations of R, G, and B components to form a YVU24 value */
									uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];
									uYAvg += (U32)(uYVUval >> 16);
									uYCnt++;
									
								} else { /* If transparent, fill with grey */
									*(pu8TransPixels++) = 0;
					  				u8Bt = u8Rt = u8Gt = 128;
									/* Sum the translations of R, G, and B components to form a YVU24 value */
									uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];
								}

								/* Calculate and write out new Y U V values */

								/* Store Y-component and U-V components in their separate arrays. */
								*pu8UVPlace++ = (U8)uYVUval;
								*pu8UVPlace++ = (U8)(uYVUval >> 8);
								*pu8YPlace++  = (U8)(uYVUval >> 16);
					
							} /* for (iX ... */
						} /* for (iLine ... */

						/* Now, convert the transparency array to a bitmask */

						pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;

						if (!BuildTransparencyBitmask(pInst, pInput, pu8TransPixels)) {
							prsRtn = PIA_S_ERROR;
							goto bail;
						}


				}else {	   /* read the mask in */

					/*  
					 *	Just convert each pixel from RGB to YVU...  grey out transparent pixels.
					 */

					/* If the transparency mask is passed in from the encoding app, */
					/* then convert it from 1bpp to 8bpp for subsampling. */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
					if (!BuildTransparencyBytemask(pInst, pInput, pu8TransPixels)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}


					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {

							/* Form discrete R, G, and B values for use as indices */

								u8Bt = *(pu8Byte++);
								u8Gt = *(pu8Byte++);
								u8Rt = *(pu8Byte++);
								pu8Byte++;

							if (!*pu8TransPixels++) {  /* If pixel is transparent, make it grey */
								u8Gt = 128;
								u8Rt = 128;
								u8Bt = 128;
								/* Sum the translations of R, G, and B components to form a YVU24 value */
								uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

							} else {
								/* Sum the translations of R, G, and B components to form a YVU24 value */
								uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];
								uYAvg += (U8)(uYVUval >> 16);
								uYCnt++;
							}

							/* Store Y-component and U-V components in their separate arrays. */
							*pu8UVPlace++ = (U8) uYVUval;
							*pu8UVPlace++ = (U8)(uYVUval >> 8);
							*pu8YPlace++  = (U8)(uYVUval >> 16);
						} /* for (iX... */
					} /* for (iLine... */

					/* Reset the pointer to what it should be */
					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
				}
			} /* end of if (alphachannel) ... else .... */	
		}
			
		else { /* If transparency is not on */

			for (iLine = 0; iLine < iHeight; iLine++) {
				iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
                pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
                for (iX = 0; iX < iWidth; iX++) {
                    uYVUval  = BtoYUV[*(pu8Byte++)];
                    uYVUval += GtoYUV[*(pu8Byte++)];
                    uYVUval += RtoYUV[*(pu8Byte++)];
					pu8Byte++;

                    *pu8UVPlace++ = (U8)uYVUval;
                    *pu8UVPlace++ = (U8)(uYVUval >> 8);
                    *pu8YPlace++  = (U8)(uYVUval >> 16);
                } /* for (iX... */
            } /* for (iLine... */
		}
		break;

    } /* end of switch (pInst->cfSrcFormat) */

	/* Now convert the non-subsampled YUV into sub-sampled YVU9 */
	uYAvg /= uYCnt + 1;  /* Never divide by zero */

	/* If the average has changed by less than 24, or the opaque pixels */
	/* currently make up less than 1/64th of the image, then stay with */
	/* the previous average. */

	if ((ABS((I32)pInst->pColorInPrivate->uPrevYAvg - (I32)uYAvg) < 24) || 
	    (uYCnt < (U32)((iHeight*iWidth)>>6))) {
		uYAvg = pInst->pColorInPrivate->uPrevYAvg;
	} else {
		pInst->pColorInPrivate->uPrevYAvg = uYAvg;
	}

#ifdef BETA_BANNER

	InsertBetaBanner(pInst, pOutput, pu8TransPixels);

#endif /* BETA_BANNER */

	SubSampling2YVU9(pInst, pInput, pOutput, pu8TransPixels, uYAvg);

bail:
    return(prsRtn);

}	/*** end of ConvertToYVU9() ***/


/***************************************************************************
 *
 * YUY2ToYVU9() is called from Vfw_Compress()
 *
 * Convert the input YUY2 (packed 4:2:2) image into YUV9 planar.
 *
 * returns status
 *
 ***************************************************************************/
PIA_RETURN_STATUS YUY2ToYVU9(PTR_CCIN_INST pInst,
                      PTR_COLORIN_INPUT_INFO pInput,
                      PTR_COLORIN_OUTPUT_INFO pOutput
                     )
{
    PIA_RETURN_STATUS  prsRtn = PIA_S_OK;
	PTR_PRIVATE_COLORIN_DATA   pPrivate = pInst->pColorInPrivate;
    U32                iWidth, iHeight;	 /* resolution of output frame */
    U32                uBytePerScan;     /* number of bytes in a scan line */
    PU8                pu8YPlace;        /* pointer to the place storing Y pixels */
    PU8                pu8UPlace;        /* pointer to the place storing U pixels */
    PU8                pu8VPlace;        /* pointer to the place storing V pixels */
    PU8                pu8InputPixels;   /* pointer to input data stream */
    PU8                pu8UVPlace;       /* byte-wide pointer to non-subsampled UV for both Endians */
	U32	               yrow, ycol;

    pu8InputPixels = pInput->pu8InputData;
    pu8YPlace = pOutput->pu8ExternalOutputData ? pOutput->pu8ExternalOutputData : pOutput->pu8OutputData;
    pu8UVPlace = pPrivate->pu8TempBuffer;

    uBytePerScan = (U32)pInput->iInputStride;               
    iWidth  = (int)pInst->dImageDim.w;
    iHeight = (int)pInst->dImageDim.h;

    /* determinate v, u subsampled plane starting address */
    pu8VPlace = pOutput->pu8OutputData + iWidth*iHeight;
    pu8UPlace = pu8VPlace + iWidth*iHeight/16;

	/* dimensions must be multiples of 4 for this to work */
	if ( iWidth % 4 || iHeight % 4)
	{
		prsRtn = PIA_S_BAD_IMAGE_DIMENSIONS;
		goto bail;
	}


#ifdef YUY2FILTER
	/* slower but higher quality. Upsample to YUV24 then call SubSampling function
	 * to convert to YUV9 with filtering.
	 */
	for ( yrow=0; yrow<iHeight; yrow++)
	{
		for ( ycol=0; ycol<iWidth; ycol+=2, pu8UVPlace+=4)
		{
			*pu8YPlace++ = *pu8InputPixels++;
			pu8UVPlace [1] = *pu8InputPixels;
			pu8UVPlace [3] = *pu8InputPixels++;
			*pu8YPlace++ = *pu8InputPixels++;
			pu8UVPlace [0] = *pu8InputPixels;
			pu8UVPlace [2] = *pu8InputPixels++;
		}
	}

	/* call function to subsample UV */
	SubSampling2YVU9(pInst, pInput, pu8TransPixels);

#else	/* YUY2FILTER */
	/* direct to YUV9 without using subsampling filter; faster, lower quality */
	for ( yrow=0; yrow<iHeight; yrow++)
	{
		for ( ycol=0; ycol<iWidth; ycol+=4, pu8InputPixels+=8)
		{
			*pu8YPlace++ = pu8InputPixels[0];
			*pu8YPlace++ = pu8InputPixels[2];
			*pu8YPlace++ = pu8InputPixels[4];
			*pu8YPlace++ = pu8InputPixels[6];
			if ((yrow%4) == 1 )	/* use second line for chroma sample */
			{
				*pu8UPlace++ = (pu8InputPixels[1] + pu8InputPixels[5]) >> 1;
				*pu8VPlace++ = (pu8InputPixels[3] + pu8InputPixels[7]) >> 1;
			}
		}
	}
#endif	/* ifdef YUY2FILTER */

bail:
	return(prsRtn);

}	/* YUY2ToYVU9() */


/***************************************************************************
 *
 * YVU12ToYVU9() is called from Vfw_Compress()
 *
 * Convert the input YVU12 (planar 4:2:0) image into YUV9 planar.
 *
 * returns status
 *
 ***************************************************************************/
PIA_RETURN_STATUS YVU12ToYVU9(PTR_CCIN_INST pInst,
                      PTR_COLORIN_INPUT_INFO pInput,
                      PTR_COLORIN_OUTPUT_INFO pOutput
                     )
{
    PIA_RETURN_STATUS  prsRtn = PIA_S_OK;
    U32                iWidth, iHeight;	 /* resolution of output frame */
    PU8                pu8UPlace;        /* pointer to the place storing U pixels */
    PU8                pu8VPlace;        /* pointer to the place storing V pixels */
    PU8                pu8InUPlace;      /* pointer to the input U pixels */
    PU8                pu8InVPlace;      /* pointer to the input V pixels */
	U32	               yrow, ycol;
	PU32			   pu32InY;			/* copy Y plane as 32-bit data, faster */
	PU32			   pu32OutY;

    pu32InY = (PU32)pInput->pu8InputData;
    pu32OutY = (PU32)pOutput->pu8ExternalOutputData ? (PU32)pOutput->pu8ExternalOutputData : (PU32) pOutput->pu8OutputData;

    iWidth  = (int)pInst->dImageDim.w;
    iHeight = (int)pInst->dImageDim.h;

	/* dimensions must be multiples of 4 for this to work */
	if ( iWidth % 4 || iHeight % 4)
	{
		prsRtn = PIA_S_BAD_IMAGE_DIMENSIONS;
		goto bail;
	}

    /* determinate v, u subsampled plane starting address */
    pu8VPlace = pOutput->pu8OutputData + iWidth*iHeight;
    pu8UPlace = pu8VPlace + iWidth*iHeight/16;
    pu8InUPlace = pInput->pu8InputData + iWidth*iHeight;
    pu8InVPlace = pu8InUPlace + iWidth*iHeight/4;

	for ( yrow=0; yrow<iHeight; yrow++)
	{
		for ( ycol=0; ycol<iWidth; ycol+=4)
		{
			*pu32OutY++ = *pu32InY++;
			if ((yrow%4) == 0 )	/* use first line for chroma sample */
			{
				*pu8UPlace++ = (*pu8InUPlace++ + *pu8InUPlace++) >> 1;
				*pu8VPlace++ = (*pu8InVPlace++ + *pu8InVPlace++) >> 1;
			}
		}
		if ((yrow%4) == 2)
		{
			/* move UV pointers to next row for row not used for samples */
			pu8InUPlace += iWidth/2;
			pu8InVPlace += iWidth/2;
		}
	}

bail:
	return(prsRtn);

}		/* YVU12ToYVU9() */

#ifdef SIMULATOR 

/***************************************************************************
 *
 * ConvertToYVU12() is called from ColorInFrame()
 *
 * Convert the passed bitmap into non-subsampled YUV
 * and place in the current context
 *
 * returns nothing
 *
 ***************************************************************************/

PIA_RETURN_STATUS ConvertToYVU12(PTR_CCIN_INST pInst,
                      PTR_COLORIN_INPUT_INFO pInput,
                      PTR_COLORIN_OUTPUT_INFO pOutput
                     )
{                                                
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
    PTR_PRIVATE_COLORIN_DATA   pPrivate = pInst->pColorInPrivate;                     
    PIA_RETURN_STATUS  prsRtn = PIA_S_OK;
    I32                iX, iY; 		 	 /* loop counters */	
    I32                iWidth, iHeight;	 /* resolution of output frame */
    U32                uBytePerScan;     /* number of bytes in a scan line */
    PU8                pu8Byte;        	 /* pointer to the bytes in the input bitmap */
    PU8                pu8YPlace;        /* pointer to the place storing Y pixels */
    PU8                pu8InputPixels;   /* pointer to input data stream */
	PU8				   pu8TransPixels;   /* pointer to transparency pixels */
    PU8                pu8UVPlace;       /* byte-wide pointer to non-subsampled UV for both Endians */
    U32                uYVUval;
    U8                 u8Rt,u8Gt,u8Bt;   /* temps while converting from rgb to yuv */
    U8                 u8BlueMin, u8BlueMax, u8GreenMin, u8GreenMax, u8RedMin, u8RedMax;
    U32                uThreshold;
    PU32               puRbucket, puGbucket, puBbucket;
	COLORIN_SEQUENCE_CONTROLS ciscSeqCtlUsed = pInst->ciscSequenceControlsUsed;
    PU8					pu8ThisColor;	/* pointer to individual color in color table */
    PTR_BGR_ENTRY		pbeCTable;		/* pointer to base of color table */
	PIA_Boolean			bUpsideDown;	/* Flag which signals an Upside down DIB. TRUE = Upside down */
	I32					iLine;			/* Looping variable that keeps track of image scanline number */

	/*------------------------- End of Declarations -------------------------*/

    pu8InputPixels = pInput->pu8InputData;
    pu8YPlace = pOutput->pu8ExternalOutputData ? pOutput->pu8ExternalOutputData : pOutput->pu8OutputData;
    pu8UVPlace = pPrivate->pu8TempBuffer;

    uBytePerScan = (U32)pInput->iInputStride;               
    iWidth  = (int)pInst->dImageDim.w;
    iHeight = (int)pInst->dImageDim.h;

	bUpsideDown = pInput->bFlipImage;  /*/ Set upside down flag here from passed struct.*/

/*/ TODO: IF YVU12 is implemented, it needs to be upgraded to fill */
/*/       transparent pixels with the Y average as the YVU9 code does.*/

/* First we do the non-subsampled Y and UV components
 * The DIBs are "upside down" (stored in rows beginning with the
 * the bottom one).  This code turns the information over so the
 * YUV information is stored with the orgin in the upper left.
 */
    switch (pInput->cfSrcFormat) {
		
		case CF_RGB16_565:
		/* Convert the inverted DIB to separate arrays, one containing the Y values in
		 * consecutive bytes, and one containing V and U in consecutive byte pairs:
		 *  Bit:   15 - 8   7 - 0	(assuming little-endian)
		 *       +-------+-------+
		 *       |   V   |   U   |
		 *       +-------+-------+
		 *
		 * These arrays are "rightside up" - that is, the upper left corner of the frame
		 * is in the lowest addresses of the arrays, and succeeding scan lines are stored
		 * left-to-right in successively higher addresses.
		 *
		 * This pass over all of the input pixels is the only one necessary, unless we are
		 * building the transparency range and building/using the mask in the same call. 
		 *
		 * There are, therefore, four modes for this conversion:
		 *	1. Building the transparency range, for which the YVU9 output will consist of every
		 *     pixel set to the YVU24 equivalent of "bgrRepresentativeColor", currently BLACK;
		 *     In this mode, CIN_BITMASK_TRANSPARENCY is off, and we won't build the mask.
		 *
		 *	2. Transparency processing, in which the transparency mask is created, and where the
		 *     output is the YUV24 equivalent of the input pixels, except where the mask indicates
		 *     a pixel is transparent, in which case the output pixel is set to the YVU24
		 *     equivalent of "bgrRepresentativeColor", currently BLACK;
		 *
		 *	3. A combination of (1) and (2), in which we build the range on one pass through the
		 *     input data, and then process that same input data on a second pass. This second
		 *     pass is provided by falling through to the (2) processing in the switch below.
		 *
		 *	4. No transparency processing, for which the output will simply be the YVU24
		 *     equivalent of the input image's RGB values.
		 */

			if (ciscSeqCtlUsed & CIN_TRANSPARENCY) {

				if (pParms->bBuildColorRange) {
					/* We're constructing the transparency range. This allocates memory for	a
					 * histogram of the R, G, and B components in this frame, and selects the
					 * threshold for noise rejection.
					 */

					uThreshold = (U32)iWidth*iHeight/TRANSPARENCY_CUTOFF;

					/* allocate memory for R, G, B buckets, each chunk size = 256*4, total=3K */
					puRbucket = HiveLocalAllocPtr(3*256*4, TRUE);

					if(!puRbucket) {
						prsRtn = PIA_S_OUT_OF_MEMORY; 
						goto bail;
					}        

					puGbucket = puRbucket+256;
					puBbucket = puRbucket+512;

					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {

							/* Form discrete R, G, and B values for use as indices */
							u8Bt = *pu8Byte++;
							u8Rt = *pu8Byte++;
							u8Gt = (u8Bt & 0xe0) >> 2;
							u8Gt |= (u8Rt & 0x03) << 6;
							u8Rt = (u8Rt & 0x7c) << 1;
							u8Bt = (u8Bt & 0x1f) << 3;

							/* build RGB histogram */
							puBbucket[u8Bt]++;
							puGbucket[u8Gt]++;
							puRbucket[u8Rt]++;

						} /* for (iX... */
					} /* for (iLine... */

					if (!BuildTransparencyRange(uThreshold, puRbucket, &pParms->crTransColorRange)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}

					pParms->bBuildColorRange = FALSE;	   /* turn it off for the subsequent frames */
					
					/* free the memory that was allocated for the histogram */                     
					HiveLocalFreePtr(puRbucket);
				}

				if (pParms->bBuildBitmask) {

					/* Transparency processing:
					 *	Build the transparency mask by analyzing the frame for colors lying within the
					 *  transparency range.  Wherever such is found, put grey in the output. Otherwise,
					 *  convert the input from RGB to YVU.
			 		*/

					/* Calc ptr for transparency pixels temp space */
					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;	

	       			/* Make local copies of range limits */
    	   			u8BlueMax  = pParms->crTransColorRange.u8BlueHigh;
        	   		u8BlueMin  = pParms->crTransColorRange.u8BlueLow;
	           		u8GreenMax = pParms->crTransColorRange.u8GreenHigh;
    	       		u8GreenMin = pParms->crTransColorRange.u8GreenLow;
        	   		u8RedMax   = pParms->crTransColorRange.u8RedHigh;
           			u8RedMin   = pParms->crTransColorRange.u8RedLow;

					/* Set the transparency-representative color in the output context */
					pParms->bgrRepresentativeColor.u8R = 
	   					(U8)(((U16)pParms->crTransColorRange.u8RedHigh +
 							  (U16)pParms->crTransColorRange.u8RedLow) >> 1);
					pParms->bgrRepresentativeColor.u8G = 
  						(U8)(((U16)pParms->crTransColorRange.u8GreenHigh +
							  (U16)pParms->crTransColorRange.u8GreenLow) >> 1);
					pParms->bgrRepresentativeColor.u8B = 
   						(U8)(((U16)pParms->crTransColorRange.u8BlueHigh +
							  (U16)pParms->crTransColorRange.u8BlueLow) >> 1);

					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {

							/* Form discrete R, G, and B values for use as indices */

							u8Bt = *pu8Byte++;
							u8Rt = *pu8Byte++;
							u8Gt = (u8Bt & 0xe0) >> 2;
							u8Gt |= (u8Rt & 0x03) << 6;
							u8Rt = (u8Rt & 0x7c) << 1;
							u8Bt = (u8Bt & 0x1f) << 3;

							/* Test to see if this pixel is opaque (outside the transparency range) */

							if ( u8Bt < u8BlueMin  || u8Bt > u8BlueMax  ||
								 u8Gt < u8GreenMin || u8Gt > u8GreenMax ||
								 u8Rt < u8RedMin   || u8Rt > u8RedMax ) 
					
							{
								/* If opaque, use the given values, and set the mask byte */
								*(pu8TransPixels++) = 1;
								
							} else { /* If transparent, fill with grey */
								*(pu8TransPixels++) = 0;
								u8Bt = u8Rt = u8Gt = 128;	
							}

							/* Sum the translations of R, G, and B components to form a YVU24 value */
							uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

							/* Store Y-component and U-V components in their separate arrays. */
							*pu8UVPlace++ = (U8)uYVUval;
							*pu8UVPlace++ = (U8)(uYVUval >> 8);
							*pu8YPlace++  = (U8)(uYVUval >> 16);
						} /* for (iX... */
					} /* for (iLine ... */

					/* Now, convert the transparency array to a bitmask */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;

					if (!BuildTransparencyBitmask(pInst, pInput, pu8TransPixels)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}


				} else  {

					/*  
					 *	Just convert each pixel from RGB to YVU...  grey out transparent pixels.
					 */

					/* If the transparency mask is passed in from the encoding app, */
					/* then convert it from 1bpp to 8bpp for subsampling. */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
					if (!BuildTransparencyBytemask(pInst, pInput, pu8TransPixels)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}


					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {

							/* Form discrete R, G, and B values for use as indices */
							u8Bt = *pu8Byte++;
							u8Rt = *pu8Byte++;

							if (!*pu8TransPixels++) {  /* If pixel is transparent, make it grey */
								u8Gt = 128;
								u8Rt = 128;
								u8Bt = 128;
							}
							else {
								u8Gt = (u8Bt & 0xe0) >> 2;	/* Else, complete getting the colors */
								u8Gt |= (u8Rt & 0x03) << 6;
								u8Rt = (u8Rt & 0x7c) << 1;
								u8Bt = (u8Bt & 0x1f) << 3;
							}

							/* Sum the translations of R, G, and B components to form a YVU24 value */
							uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

							/* Store Y-component and U-V components in their separate arrays. */
							*pu8UVPlace++ = (U8) uYVUval;
							*pu8UVPlace++ = (U8)(uYVUval >> 8);
							*pu8YPlace++  = (U8)(uYVUval >> 16);
						} /* for (iX... */
					} /* for (iLine ... */

					/* Reset the pointer to what it should be */
					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
				}
			} else {

				/* No transparency processing:
				 *	Just convert each pixel from RGB to YVU...
				 */

				for (iLine = 0; iLine < iHeight; iLine++) {
					iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
					pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
					for (iX = 0; iX < iWidth; iX++) {

						/* Form discrete R, G, and B values for use as indices */
						u8Bt = *pu8Byte++;
						u8Rt = *pu8Byte++;
						u8Gt = (u8Bt & 0xe0) >> 2;
						u8Gt |= (u8Rt & 0x03) << 6;
						u8Rt = (u8Rt & 0x7c) << 1;
						u8Bt = (u8Bt & 0x1f) << 3;

						/* Sum the translations of R, G, and B components to form a YVU24 value */
						uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

						/* Store Y-component and U-V components in their separate arrays. */
						*pu8UVPlace++ = (U8) uYVUval;
						*pu8UVPlace++ = (U8)(uYVUval >> 8);
						*pu8YPlace++  = (U8)(uYVUval >> 16);
					} /* for (iX... */
				} /* for (iLine ... */
			}

		break;	/* end of case: CF_RGB16_565 */

		case CF_CLUT_8:
			/* Other than the means by which the RGB value is developed, this conversion is
			 * identical to the RGB16_565 conversion above.
			 */

			/* get palette entry */
			if(!pInput->ppalCurrentPalette)	{
				prsRtn = PIA_S_BAD_SETUP_VALUE; 
				goto bail;
			}

			pbeCTable = &(pInput->ppalCurrentPalette->pbgrTable[0]);
			
			if (ciscSeqCtlUsed & CIN_TRANSPARENCY) {

				if (pParms->bBuildColorRange) {

					/* We're constructing the transparency range. This allocates memory for	a
					 * histogram of the R, G, and B components in this frame, and selects the
					 * threshold for noise rejection.
					 */

					uThreshold = (U32)iWidth*iHeight/TRANSPARENCY_CUTOFF;

					/* allocate memory for R, G, B buckets, each chunk size = 256*4, total=3K */
					puRbucket = HiveLocalAllocPtr(3*256*4, TRUE);

					if(!puRbucket) {
						prsRtn = PIA_S_OUT_OF_MEMORY; 
						goto bail;
					}

					puGbucket = puRbucket+256;
					puBbucket = puRbucket+512;
			
					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {
							/* Form discrete R, G, and B values for use as indices */
							pu8ThisColor = (PU8) (pbeCTable + *pu8Byte++);
							u8Bt = *(pu8ThisColor++);
							u8Gt = *(pu8ThisColor++);
							u8Rt = *(pu8ThisColor);

							/* build RGB histogram */
							puBbucket[u8Bt]++;
							puGbucket[u8Gt]++;
							puRbucket[u8Rt]++;
						
						} /* for (iX... */
					} /* for (iLine... */

					if (!BuildTransparencyRange(uThreshold, puRbucket, &pParms->crTransColorRange)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}

					pParms->bBuildColorRange = FALSE;	   /* turn it off for the subsequent frames */
					
					/* free the memory that was allocated for the histogram */                     
					HiveLocalFreePtr(puRbucket);
				
				}

				if (pParms->bBuildBitmask) {

					/* Transparency processing:
					 *	Build the transparency mask by analyzing the frame for colors lying within the
					 *  transparency range.  Wherever such is found, put grey in the output. Otherwise,
					 *  convert the input from RGB to YVU.
					 */

					/* Calc ptr for transparency pixels temp space */
					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;	

	       			/* Make local copies of range limits */
    	   			u8BlueMax  = pParms->crTransColorRange.u8BlueHigh;
        	   		u8BlueMin  = pParms->crTransColorRange.u8BlueLow;
           			u8GreenMax = pParms->crTransColorRange.u8GreenHigh;
	           		u8GreenMin = pParms->crTransColorRange.u8GreenLow;
    	       		u8RedMax   = pParms->crTransColorRange.u8RedHigh;
        	   		u8RedMin   = pParms->crTransColorRange.u8RedLow;
					
					/* Set the transparency-representative color  */
					pParms->bgrRepresentativeColor.u8R = 
   						(U8)(((U16)pParms->crTransColorRange.u8RedHigh +
 							  (U16)pParms->crTransColorRange.u8RedLow) >> 1);
					pParms->bgrRepresentativeColor.u8G = 
  						(U8)(((U16)pParms->crTransColorRange.u8GreenHigh +
							  (U16)pParms->crTransColorRange.u8GreenLow) >> 1);
					pParms->bgrRepresentativeColor.u8B = 
   						(U8)(((U16)pParms->crTransColorRange.u8BlueHigh +
							  (U16)pParms->crTransColorRange.u8BlueLow) >> 1);

					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {

							/* Form discrete R, G, and B values for use as indices */
							pu8ThisColor = (PU8) (pbeCTable + *pu8Byte++);
							u8Bt = *(pu8ThisColor++);
							u8Gt = *(pu8ThisColor++);
							u8Rt = *(pu8ThisColor);

							/* Test to see if this pixel is opaque (outside the transparency range) */

							if ( u8Bt < u8BlueMin  || u8Bt > u8BlueMax  ||
								 u8Gt < u8GreenMin || u8Gt > u8GreenMax ||
								 u8Rt < u8RedMin   || u8Rt > u8RedMax ) 

								{
								/* If opaque, use the given values, and set the mask byte */
								*(pu8TransPixels++) = 1;
								
							} else { /* If transparent, fill with grey */
								*(pu8TransPixels++) = 0;
								u8Bt = u8Rt = u8Gt = 128;	/* grey, for now */
							}

							/* Sum the translations of R, G, and B components to form a YVU24 value */
							uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

							/* Store Y-component and U-V components in their separate arrays. */
							*pu8UVPlace++ = (U8)uYVUval;
							*pu8UVPlace++ = (U8)(uYVUval >> 8);
							*pu8YPlace++  = (U8)(uYVUval>>16);
						} /* for (iX... */
					} /* for (iLine ... */

					/* Now, convert the transparency array to a bitmask */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;

					if (!BuildTransparencyBitmask(pInst, pInput, pu8TransPixels)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}

				} else  {

					/*  
					 *	Just convert each pixel from RGB to YVU...  grey out transparent pixels.
					 */

					/* If the transparency mask is passed in from the encoding app, */
					/* then convert it from 1bpp to 8bpp for subsampling. */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
					if (!BuildTransparencyBytemask(pInst, pInput, pu8TransPixels)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}


					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {

							/* Form discrete R, G, and B values for use as indices */
							pu8ThisColor = (PU8) (pbeCTable + *pu8Byte++);

							if (!*pu8TransPixels++) {  /* If pixel is transparent, make it grey */
								u8Gt = 128;
								u8Rt = 128;
								u8Bt = 128;
							}
							else {
								u8Bt = *(pu8ThisColor++);
								u8Gt = *(pu8ThisColor++);
								u8Rt = *(pu8ThisColor);
							}

							/* Sum the translations of R, G, and B components to form a YVU24 value */
							uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

							/* Store Y-component and U-V components in their separate arrays. */
							*pu8UVPlace++ = (U8) uYVUval;
							*pu8UVPlace++ = (U8)(uYVUval >> 8);
							*pu8YPlace++  = (U8)(uYVUval >> 16);
						} /* for (iX... */
					} /* for (iLine ... */

					/* Reset the pointer to what it should be */
					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
				}
			}	else {	/* If transparency is not on */

				/* No transparency processing:
				 *	Just convert each pixel from RGB to YVU...
				 */

				for (iLine = 0; iLine < iHeight; iLine++) {
					iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
					pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
					for (iX = 0; iX < iWidth; iX++) {

						/* Form discrete R, G, and B values for use as indices */
						pu8ThisColor = (PU8) (pbeCTable + *pu8Byte++);
						u8Bt = *(pu8ThisColor++);
						u8Gt = *(pu8ThisColor++);
						u8Rt = *(pu8ThisColor);

						/* Sum the translations of R, G, and B components to form a YVU24 value */
						uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

						/* Store Y-component and U-V components in their separate arrays. */
						*pu8UVPlace++ = (U8) uYVUval;
						*pu8UVPlace++ = (U8)(uYVUval >> 8);
						*pu8YPlace++  = (U8)(uYVUval >> 16);
					} /* for (iX... */
				} /* for (iLine ... */
			}

		break;	/* end of case: CF_CLUT_8 */

        case CF_BGR24:

			 if (ciscSeqCtlUsed & CIN_TRANSPARENCY) {

				 if (pParms->bBuildColorRange) {
               		uThreshold = (U32)iWidth*iHeight/TRANSPARENCY_CUTOFF;
                    
               		/* allocate memory for R, G, B buckets, each chunk size = 256*4 
              		 * total size = 3k 
               		 */
                     		 
               		puRbucket = HiveLocalAllocPtr(3*256*4, TRUE);
               		/* check if allocated memory is OK */
               		if(!puRbucket) {
                   		prsRtn = PIA_S_OUT_OF_MEMORY; 
                   		goto bail;
               		}        
					puGbucket = puRbucket+256;
               		puBbucket = puRbucket+512;

               		for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
                   		pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
                   		for (iX = 0; iX < iWidth; iX++) {
                  			/* build RGB histogram */
                       		puBbucket[*(pu8Byte++)]++;
                       		puGbucket[*(pu8Byte++)]++;
   	                   		puRbucket[*(pu8Byte++)]++;
       					} /* for (iX ... */
           			} /* for (iLine ... */

               		/* build RGB transparency range */
               		if (!BuildTransparencyRange(uThreshold, puRbucket, &pParms->crTransColorRange)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}

					pParms->bBuildColorRange = FALSE;	   /* turn it off for the subsequent frames */
					

        	   		/* free allocated memory */                     
               		HiveLocalFreePtr(puRbucket);
				}

				if (pParms->bBuildBitmask) {

					/* Calc ptr for transparency pixels temp space */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;	

          			u8BlueMax  = pParms->crTransColorRange.u8BlueHigh;
              		u8BlueMin  = pParms->crTransColorRange.u8BlueLow;
              		u8GreenMax = pParms->crTransColorRange.u8GreenHigh;
              		u8GreenMin = pParms->crTransColorRange.u8GreenLow;
              		u8RedMax   = pParms->crTransColorRange.u8RedHigh;
              		u8RedMin   = pParms->crTransColorRange.u8RedLow;

					pParms->bgrRepresentativeColor.u8R = 
    					(U8)(((U16)pParms->crTransColorRange.u8RedHigh +
 	 						  (U16)pParms->crTransColorRange.u8RedLow) >> 1);
   					pParms->bgrRepresentativeColor.u8G = 
    					(U8)(((U16)pParms->crTransColorRange.u8GreenHigh +
							  (U16)pParms->crTransColorRange.u8GreenLow) >> 1);
   					pParms->bgrRepresentativeColor.u8B = 
    					(U8)(((U16)pParms->crTransColorRange.u8BlueHigh +
							  (U16)pParms->crTransColorRange.u8BlueLow) >> 1);

					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {
							u8Bt = *(pu8Byte++);
							u8Gt = *(pu8Byte++);
							u8Rt = *(pu8Byte++);

							/* Test to see if this pixel is opaque */

                       		if (u8Bt < u8BlueMin  || u8Bt > u8BlueMax  ||
                       			u8Gt < u8GreenMin || u8Gt > u8GreenMax ||
								u8Rt < u8RedMin   || u8Rt > u8RedMax) 

								/* If opaque, use the given values, and set the mask byte */
								*(pu8TransPixels++) = 1;
								
							else { /* If transparent, fill with grey */
								*(pu8TransPixels++) = 0;
					  			u8Bt = u8Rt = u8Gt = 128;
							}

							/* Calculate and write out new Y U V values */

							/* Sum the translations of R, G, and B components to form a YVU24 value */
							uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

							/* Store Y-component and U-V components in their separate arrays. */
							*pu8UVPlace++ = (U8)uYVUval;
							*pu8UVPlace++ = (U8)(uYVUval >> 8);
							*pu8YPlace++  = (U8)(uYVUval >> 16);
				
						} /* for (iX ... */
					} /* for (iLine ... */

					/* Now, convert the transparency array to a bitmask */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;

					if (!BuildTransparencyBitmask(pInst, pInput, pu8TransPixels)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}


				}	else  {

					/*  
					 *	Just convert each pixel from RGB to YVU...  grey out transparent pixels.
					 */

					/* If the transparency mask is passed in from the encoding app, */
					/* then convert it from 1bpp to 8bpp for subsampling. */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
					if (!BuildTransparencyBytemask(pInst, pInput, pu8TransPixels)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}


					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
						for (iX = 0; iX < iWidth; iX++) {

							/* Form discrete R, G, and B values for use as indices */

								u8Bt = *(pu8Byte++);
								u8Gt = *(pu8Byte++);
								u8Rt = *(pu8Byte++);

							if (!*pu8TransPixels++) {  /* If pixel is transparent, make it grey */
								u8Gt = 128;
								u8Rt = 128;
								u8Bt = 128;
							}

							/* Sum the translations of R, G, and B components to form a YVU24 value */
							uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

							/* Store Y-component and U-V components in their separate arrays. */
							*pu8UVPlace++ = (U8) uYVUval;
							*pu8UVPlace++ = (U8)(uYVUval >> 8);
							*pu8YPlace++  = (U8)(uYVUval >> 16);
						} /* for (iX... */
					} /* for (iLine ... */

					/* Reset the pointer to what it should be */
					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
				}

			}	else { /* If transparency is not on */

                for (iLine = 0; iLine < iHeight; iLine++) {
					iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
                    pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
                    for (iX = 0; iX < iWidth; iX++) {
                        uYVUval  = BtoYUV[*(pu8Byte++)];
                        uYVUval += GtoYUV[*(pu8Byte++)];
                        uYVUval += RtoYUV[*(pu8Byte++)];
                        *pu8UVPlace++ = (U8)uYVUval;
                        *pu8UVPlace++ = (U8)(uYVUval >> 8);
                        *pu8YPlace++  = (U8)(uYVUval >> 16);
                    } /* for (iX ... */
                } /* for (iLine ... */
		   }
		break;

        case CF_XRGB32:

			 if (ciscSeqCtlUsed & CIN_TRANSPARENCY) {
				 
				 if (pParms->bAlphaChannel) {
					/*  
					 *	Just convert each pixel from RGB to YVU...  Get Transparent Pixels from extra byte
					 */

					/* Calc ptr for transparency pixels temp space */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;	

					/* If the transparency mask is passed in from the encoding app */
					/* in the extra byte of the RGB32, then convert it from 8bpp alpha
					/* channel to 8bpp transparency for subsampling. */

					for (iLine = 0; iLine < iHeight; iLine++) {
						iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
						pu8Byte = (PU8)(pu8InputPixels+((U32)iY*uBytePerScan));
						for (iX=0; iX< iWidth; iX++) {

							/* Form discrete R, G, and B values for use as indices */

								u8Bt = *(pu8Byte++);
								u8Gt = *(pu8Byte++);
								u8Rt = *(pu8Byte++);

							/* Test to see if this pixel is opaque, do this by splitting the alpha */
							/* Channel down the center, 0-127 = Transparent, 128-255 = opaque. */

                       		if (*(pu8Byte++) > 127) {
 								/* If opaque, use the given values, and set the mask byte */
								*(pu8TransPixels++) = 1;
							}
							else { /* If transparent, fill with grey */
								*(pu8TransPixels++) = 0;
					  			u8Bt = u8Rt = u8Gt = 128;
							}

							/* Sum the translations of R, G, and B components to form a YVU24 value */
							uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

							/* Store Y-component and U-V components in their separate arrays. */
							*pu8UVPlace++ = (U8) uYVUval;
							*pu8UVPlace++ = (U8)(uYVUval >> 8);
							*pu8YPlace++  = (U8)(uYVUval >> 16);
						} /* for (iX... */
					} /* for (iLine ... */

					/* Now, convert the transparency array to a bitmask */

					pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;

					if (!BuildTransparencyBitmask(pInst, pInput, pu8TransPixels)) {
						prsRtn = PIA_S_ERROR;
						goto bail;
					}

				 } else {

					 if (pParms->bBuildColorRange) {

               			uThreshold = (U32)iWidth*iHeight/TRANSPARENCY_CUTOFF;
                    
               			/* allocate memory for R, G, B buckets, each chunk size = 256*4 
              			 * total size = 3k 
               			 */
                     			 
               			puRbucket = HiveLocalAllocPtr(3*256*4, TRUE);
               			/* check if allocated memory is OK */
               			if(!puRbucket) {
                   			prsRtn = PIA_S_OUT_OF_MEMORY; 
                   			goto bail;
               			}        
						puGbucket = puRbucket+256;
               			puBbucket = puRbucket+512;

               			for (iLine = 0; iLine < iHeight; iLine++) {
							iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
                   			pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
                   			for (iX = 0; iX < iWidth; iX++) {
                  				/* build RGB histogram */
                       			puBbucket[*(pu8Byte++)]++;
                       			puGbucket[*(pu8Byte++)]++;
   	                   			puRbucket[*(pu8Byte++)]++;
								pu8Byte++;  /* Skip Over X byte */
       						} /* for (iX ... */
           				} /* for (iLine ... */

               			/* build RGB transparency range */
               			if (!BuildTransparencyRange(uThreshold, puRbucket, &pParms->crTransColorRange)) {
							prsRtn = PIA_S_ERROR;
							goto bail;
						}

						pParms->bBuildColorRange = FALSE;	   /* turn it off for the subsequent frames */
					
        	   			/* free allocated memory */                     
               			HiveLocalFreePtr(puRbucket);
					}

					if (pParms->bBuildBitmask) {

						/* Calc ptr for transparency pixels temp space */

						pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;	

          				u8BlueMax  = pParms->crTransColorRange.u8BlueHigh;
              			u8BlueMin  = pParms->crTransColorRange.u8BlueLow;
              			u8GreenMax = pParms->crTransColorRange.u8GreenHigh;
              			u8GreenMin = pParms->crTransColorRange.u8GreenLow;
              			u8RedMax   = pParms->crTransColorRange.u8RedHigh;
              			u8RedMin   = pParms->crTransColorRange.u8RedLow;

						pParms->bgrRepresentativeColor.u8R = 
    						(U8)(((U16)pParms->crTransColorRange.u8RedHigh +
 	 							  (U16)pParms->crTransColorRange.u8RedLow) >> 1);
   						pParms->bgrRepresentativeColor.u8G = 
    						(U8)(((U16)pParms->crTransColorRange.u8GreenHigh +
								  (U16)pParms->crTransColorRange.u8GreenLow) >> 1);
   						pParms->bgrRepresentativeColor.u8B = 
    						(U8)(((U16)pParms->crTransColorRange.u8BlueHigh +
								  (U16)pParms->crTransColorRange.u8BlueLow) >> 1);

						for (iLine = 0; iLine < iHeight; iLine++) {
							iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
							pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
							for (iX = 0; iX < iWidth; iX++) {

								u8Bt = *(pu8Byte++);
								u8Gt = *(pu8Byte++);
								u8Rt = *(pu8Byte++);
								pu8Byte++;

								/* Test to see if this pixel is opaque */

                       			if (u8Bt < u8BlueMin  || u8Bt > u8BlueMax  ||
                       				u8Gt < u8GreenMin || u8Gt > u8GreenMax ||
									u8Rt < u8RedMin   || u8Rt > u8RedMax) 

									/* If opaque, use the given values, and set the mask byte */
									*(pu8TransPixels++) = 1;
									
								else { /* If transparent, fill with grey */
									*(pu8TransPixels++) = 0;
					  				u8Bt = u8Rt = u8Gt = 128;
								}

								/* Calculate and write out new Y U V values */

								/* Sum the translations of R, G, and B components to form a YVU24 value */
								uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

								/* Store Y-component and U-V components in their separate arrays. */
								*pu8UVPlace++ = (U8)uYVUval;
								*pu8UVPlace++ = (U8)(uYVUval >> 8);
								*pu8YPlace++  = (U8)(uYVUval >> 16);
					
							} /* for (iX ... */
						} /* for (iLine ... */

						/* Now, convert the transparency array to a bitmask */

						pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;

						if (!BuildTransparencyBitmask(pInst, pInput, pu8TransPixels)) {
							prsRtn = PIA_S_ERROR;
							goto bail;
						}


					} else  {

						/*  
						 *	Just convert each pixel from RGB to YVU...  grey out transparent pixels.
						 */

						/* If the transparency mask is passed in from the encoding app, */
						/* then convert it from 1bpp to 8bpp for subsampling. */

						pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
						if (!BuildTransparencyBytemask(pInst, pInput, pu8TransPixels)) {
							prsRtn = PIA_S_ERROR;
							goto bail;
						}


						for (iLine = 0; iLine < iHeight; iLine++) {
							iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
							pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
							for (iX = 0; iX < iWidth; iX++) {

								/* Form discrete R, G, and B values for use as indices */

									u8Bt = *(pu8Byte++);
									u8Gt = *(pu8Byte++);
									u8Rt = *(pu8Byte++);
									pu8Byte++;

								if (!*pu8TransPixels++) {  /* If pixel is transparent, make it grey */
									u8Gt = 128;
									u8Rt = 128;
									u8Bt = 128;
								}

								/* Sum the translations of R, G, and B components to form a YVU24 value */
								uYVUval = RtoYUV[u8Rt] + GtoYUV[u8Gt] + BtoYUV[u8Bt];

								/* Store Y-component and U-V components in their separate arrays. */
								*pu8UVPlace++ = (U8) uYVUval;
								*pu8UVPlace++ = (U8)(uYVUval >> 8);
								*pu8YPlace++  = (U8)(uYVUval >> 16);
							} /* for (iX... */
						} /* for (iLine... */

						/* Reset the pointer to what it should be */
						pu8TransPixels = pPrivate->pu8TempBuffer + 2*iWidth*iHeight;
					}
				}  /* end of if (bAlphaChannel) ...else ... */
			}		else { /* If transparency is not on */

                for (iLine = 0; iLine < iHeight; iLine++) {
					iY = bUpsideDown ? iHeight - iLine - 1 : iLine;
                    pu8Byte = (PU8)(pu8InputPixels+((U32)iY * uBytePerScan));
                    for (iX = 0; iX < iWidth; iX++) {
                        uYVUval  = BtoYUV[*(pu8Byte++)];
                        uYVUval += GtoYUV[*(pu8Byte++)];
                        uYVUval += RtoYUV[*(pu8Byte++)];
						pu8Byte++;

                        *pu8UVPlace++ = (U8)uYVUval;
                        *pu8UVPlace++ = (U8)(uYVUval >> 8);
                        *pu8YPlace++  = (U8)(uYVUval >> 16);
                    } /* for (iX... */
                } /* for (iLine... */
		   }
		   break;

    } /* end of switch (pInst->cfSrcFormat) */

	/* Now convert the non-subsampled YUV into sub-sampled YVU12 */
	SubSampling2YVU12(pInst, pInput, pOutput, pu8TransPixels);

bail:
    return(prsRtn);

}	/*** end of ConvertToYVU12() ***/

#endif /* SIMULATOR */

/*************************************************************************
           PRIVATE ROUTINES
*************************************************************************/

/***************************************************************************
 *
 * SubSampling2YVU9 is called from ConvertToYVU9
 *
 * This routine will use 4x4 filter to convert the non-subsampled YUV 
 * into sub-sampled YVU9.  This routine also makes transparent pixels in 
 * 4x4 blocks with both transparent and non-transparent pixel match the 
 * average luma value of the non-transparent pixels.
 * 
 * returns nothing 
 *
 ***************************************************************************/

static void SubSampling2YVU9(PTR_CCIN_INST pInst, PTR_COLORIN_INPUT_INFO pInput,
							 PTR_COLORIN_OUTPUT_INFO pOutput,
							 PU8 pu8TransPixels, U32 YOverallAvg)
{
    PTR_PRIVATE_COLORIN_DATA   pPrivate = pInst->pColorInPrivate;                     
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
    I32 iFbias[16] = { 
                        3,  5,  5, 3,
                        5, 25, 25, 5, 
                        5, 25, 25, 5, 
                        3,  5,  5, 3 
                    };
    I32 *piFbp;					/* pointer point to bias table */
    I32 iXblk, iYblk;           /* Counters that refer to coordinates of 4x4 blocks */
    I32 iBlkX, iBlkY;			/* Counters that refer to pixel coordinates within 4x4 blocks*/
    I32 iWidth, iHeight;		/* resolution of output frame */
	I32 iX, iY;					/* Counters that refer to pixel coordinates */
    PU8 pu8UPlace, pu8VPlace; 	/* pointers to subsampled U and V arrays */
	PU8 pu8YPlace;				/* Pointer to Y plane */
    PU8 pu8Byte;			 	/* pointer to the bytes in the input bitmap */
	PU8 pu8TransByte;			/* Working pointer to transparency bitmask */
    U32 uLut, uLvt, uLyt;		/* Accumulate pixel value sums for a block */
    U32 uOffset;  				/* Pixel offsets within memory buffer */
	U32 uSumBias, uSumBias2;	/* Accumulate average bias based on transparency mask */

    iWidth  = (int)pInst->dImageDim.w;
    iHeight = (int)pInst->dImageDim.h;

    /* determinate v, u subsampled plane starting address */
    pu8VPlace = (pOutput->pu8ExternalOutputData ? pOutput->pu8ExternalOutputData : pOutput->pu8OutputData) + iWidth*iHeight;
    pu8UPlace = pu8VPlace + iWidth*iHeight/16;

	pu8YPlace = pOutput->pu8ExternalOutputData ? pOutput->pu8ExternalOutputData : pOutput->pu8OutputData;

	/* retrieve non-subsampled v, u plane starting address - input of subsampling process */
    pu8Byte  = pPrivate->pu8TempBuffer;

	/* If there is transparency...  */

	 if ((pInst->ciscSequenceControlsUsed & CIN_TRANSPARENCY) & !pParms->bAlphaChannel) {

		/* Walk through all 4x4 blocks in image */
	   	for (iYblk = 0; iYblk < iHeight; iYblk += 4) {
	        for (iXblk = 0; iXblk < iWidth; iXblk += 4) {

				/* Reset pixel sum variables and bias array pointer */
        	    uLut = uLvt = uLyt = 0;
				uSumBias = uSumBias2 = 0;
            	piFbp = iFbias;

				/* Get pointer into transparency buffer */
				pu8TransByte = pu8TransPixels + iYblk*iWidth + iXblk;
				
				/* Walk through all pixels in each 4x4 block */
            	for (iBlkY = 0, iY = iYblk; iBlkY < 4; iBlkY++, iY++) {
                	for (iBlkX = 0, iX = iXblk; iBlkX < 4; iBlkX++, iX++) {

						/* Calculate offset from beginning of buffer */
	                    uOffset = 2UL * ((iY * (U32)iWidth) + iX);

						/* If this pixel is not transparent */
						/* (transparent pixels do not contribute to block subsample) */
						if (*(pu8TransByte + iBlkY*iWidth + iBlkX))	{

							/* Accumulate biased pixel sums */
    	                   uLvt += *piFbp * pu8Byte[uOffset];
        	               uLut += *piFbp * pu8Byte[uOffset+1];
						   uLyt += pu8YPlace[uOffset>>1];

						   /* Accumulate total bias applied to block */
						   uSumBias += *piFbp;
						   uSumBias2++;
            	           piFbp++;
						}
	                } /* for (iBlkX ... */
    	        } /* for (iBlkY ... */

				/* If no bias applied, all pixels in block were transparent */
				if (uSumBias == 0) {
					/* Just write value of last pixel */
	            	*pu8VPlace++ = pu8Byte[uOffset];
    	        	*pu8UPlace++ = pu8Byte[uOffset+1];

					/* Loop through all of the Y pels in the 4x4, and set the Y values */
					/* of all pixels equal to the average of the non-transparent */
					/* pixels in the image.  This is done to minimize datarate. */

	            	for (iBlkY = 0, iY = iYblk; iBlkY < 4; iBlkY++, iY++) {
   		            	for (iBlkX = 0, iX = iXblk; iBlkX < 4; iBlkX++, iX++) {
    		                uOffset = (iY * (U32)iWidth) + iX;
							/* Write the overall Y average */
						    pu8YPlace[uOffset] = (U8) YOverallAvg;
    	    	        } /* for (iBlkX ... */
    		        } /* for (iBlkY ... */
				}
				else {
					/* Calculate U and V averages from accumulated sums and biases */
	            	*pu8UPlace++ = (U8)(((uLut+(uSumBias>>1)) / uSumBias) & 0xFF);
    	        	*pu8VPlace++ = (U8)(((uLvt+(uSumBias>>1)) / uSumBias) & 0xFF);

					/* If there were transparent pixels... */
					if (uSumBias2 != 16) {
						/* Calculate the average of the nontransparent Y pixels */
						U8 Yavg = (U8)(((uLyt+(uSumBias2>>1)) / uSumBias2) & 0xFF);

						/* Loop through all of the Y pels in the 4x4, and set the Y values */
						/* of all transparent pixels equal to the average of the */
						/* non-transparent pixels.  This is done so that if this frame */
						/* is decoded with only band 0 (half resolution), the averaged Y */
						/* pixels will not pick up values from the transparent pixels */

		            	for (iBlkY = 0, iY = iYblk; iBlkY < 4; iBlkY++, iY++) {
    		            	for (iBlkX = 0, iX = iXblk; iBlkX < 4; iBlkX++, iX++) {
	    		                uOffset = (iY * (U32)iWidth) + iX;
								/* If a transparent pixel, write the Y average */
								if (!(*(pu8TransByte + iBlkY*iWidth + iBlkX)))	{
								   pu8YPlace[uOffset] = Yavg;
								}
	    	    	        } /* for (iBlkX ... */
	    		        } /* for (iBlkY ... */
					}
				}
	        } /* for (iXblk ... */
	    } /* for (iYblk ... */
	}
	else {  /* Fast, no transparency subsample */

		/* Walk through all 4x4 blocks in image */
    	for (iYblk = 0; iYblk < iHeight; iYblk += 4) {
	        for (iXblk = 0; iXblk < iWidth; iXblk += 4) {

				/* Reset pixel sum variables and bias array pointer */
        	    uLut = uLvt = 0;
            	piFbp = iFbias;

				/* Walk through all pixels in each 4x4 block */
            	for (iBlkY = 0, iY = iYblk; iBlkY < 4; iBlkY++, iY++) {
                	for (iBlkX = 0, iX = iXblk; iBlkX < 4; iBlkX++, iX++) {

						/* Calculate offset from beginning of buffer */
	                    uOffset = 2UL * ((iY * (U32)iWidth) + iX);

						/* Accumulate biased pixel sums */
    	                uLvt += *piFbp * pu8Byte[uOffset];
        	            uLut += *piFbp * pu8Byte[uOffset+1];
						piFbp++;

	                } /* for (iBlkX ... */
    	        } /* for (iBlkX ... */

				/* Calculate and store biased U and V pixel averages */
	            *pu8UPlace++ = (U8)(((uLut+76) / 152) & 0xFF);
    	        *pu8VPlace++ = (U8)(((uLvt+76) / 152) & 0xFF);

	        } /* for (iXblk ... */
	    } /* for (iYblk ... */
	}  /* else no transparency */
	return;
}

#ifdef SIMULATOR

/***************************************************************************
 *
 * SubSampling2YVU12 is called from ConvertToYVU12
 *
 * This routine will use 2x2 filter to convert the non-subsampled YUV 
 * into sub-sampled YVU12.  This routine also makes transparent pixels in 
 * 2x2 blocks with both transparent and non-transparent pixel match the 
 * average luma value of the non-transparent pixels.
 * 
 * returns nothing 
 *
 ***************************************************************************/

static void SubSampling2YVU12(PTR_CCIN_INST pInst, PTR_COLORIN_INPUT_INFO pInput,
							  PTR_COLORIN_OUTPUT_INFO pOutput,
							 PU8 pu8TransPixels)
{
    PTR_PRIVATE_COLORIN_DATA   pPrivate = pInst->pColorInPrivate;                     
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
    I32 iFbias[4] = {  25, 25,
                       25, 25  };

    I32 *piFbp;					/* pointer point to bias table */
    I32 iXblk, iYblk;           /* Counters that refer to coordinates of 4x4 blocks */
    I32 iBlkX, iBlkY;			/* Counters that refer to pixel coordinates within 4x4 blocks*/
    I32 iWidth, iHeight;		/* resolution of output frame */
	I32 iX, iY;					/* Counters that refer to pixel coordinates */
    PU8 pu8UPlace, pu8VPlace; 	/* pointers to subsampled U and V arrays */
	PU8 pu8YPlace;				/* Pointer to Y plane */
    PU8 pu8Byte;			 	/* pointer to the bytes in the input bitmap */
	PU8 pu8TransByte;			/* Working pointer to transparency bitmask */
    U32 uLut, uLvt, uLyt;		/* Accumulate pixel value sums for a block */
    U32 uOffset;  				/* Pixel offsets within memory buffer */
	U32 uSumBias, uSumBias2;	/* Accumulate average bias based on transparency mask */

    iWidth  = (int)pInst->dImageDim.w;
    iHeight = (int)pInst->dImageDim.h;

    /* determinate v, u subsampled plane starting address */
    pu8VPlace = (pOutput->pu8ExternalOutputData ? pOutput->pu8ExternalOutputData : pOutput->pu8OutputData) + iWidth*iHeight;
    pu8UPlace = pu8VPlace + iWidth*iHeight/4;

	pu8YPlace = pOutput->pu8ExternalOutputData ? pOutput->pu8ExternalOutputData : pOutput->pu8OutputData;

	/* retrieve non-subsampled v, u plane starting address - input of subsampling process */
    pu8Byte  = pPrivate->pu8TempBuffer;

	/* If there is transparency...  */

	 if ((pInst->ciscSequenceControlsUsed & CIN_TRANSPARENCY) & !pParms->bAlphaChannel) {

		/* Walk through all 2x2 blocks in image */
	   	for (iYblk = 0; iYblk < iHeight; iYblk += 2) {
	        for (iXblk = 0; iXblk < iWidth; iXblk += 2) {

				/* Reset pixel sum variables and bias array pointer */
        	    uLut = uLvt = uLyt = 0;
				uSumBias = uSumBias2 = 0;
            	piFbp = iFbias;

				/* Get pointer into transparency buffer */
				pu8TransByte = pu8TransPixels + iYblk*iWidth + iXblk;
				
				/* Walk through all pixels in each 4x4 block */
            	for (iBlkY = 0, iY = iYblk; iBlkY < 2; iBlkY++, iY++) {
                	for (iBlkX = 0, iX = iXblk; iBlkX < 2; iBlkX++, iX++) {

						/* Calculate offset from beginning of buffer */
	                    uOffset = 2UL * ((iY * (U32)iWidth) + iX);

						/* If this pixel is not transparent */
						/* (transparent pixels do not contribute to block subsample) */
						if (*(pu8TransByte + iBlkY*iWidth + iBlkX))	{

							/* Accumulate biased pixel sums */
    	                   uLvt += *piFbp * pu8Byte[uOffset];
        	               uLut += *piFbp * pu8Byte[uOffset+1];
						   uLyt += pu8YPlace[uOffset>>1];

						   /* Accumulate total bias applied to block */
						   uSumBias += *piFbp;
						   uSumBias2++;
            	           piFbp++;
						}
	                } /* for (iBlkX ... */
    	        } /* for (iBlkY ... */

				/* If no bias applied, all pixels in block were transparent */
				if (uSumBias == 0) {
					/* Just write value of last pixel */
	            	*pu8VPlace++ = pu8Byte[uOffset];
    	        	*pu8UPlace++ = pu8Byte[uOffset+1];
				}
				else {
					/* Calculate U and V averages from accumulated sums and biases */
	            	*pu8VPlace++ = (U8)(((uLut+(uSumBias>>1)) / uSumBias) & 0xFF);
    	        	*pu8UPlace++ = (U8)(((uLvt+(uSumBias>>1)) / uSumBias) & 0xFF);

					/* If there were transparent pixels... */
					if (uSumBias2 != 4) {
						/* Calculate the average of the nontransparent Y pixels */
						U8 Yavg = (U8)(((uLyt+(uSumBias2>>1)) / uSumBias2) & 0xFF);

						/* Loop through all of the Y pels in the 4x4, and set the Y values */
						/* of all transparent pixels equal to the average of the */
						/* non-transparent pixels.  This is done so that if this frame */
						/* is decoded with only band 0 (half resolution), the averaged Y */
						/* pixels will not pick up values from the transparent pixels */

		            	for (iBlkY = 0, iY = iYblk; iBlkY < 2; iBlkY++, iY++) {
    		            	for (iBlkX = 0, iX = iXblk; iBlkX < 2; iBlkX++, iX++) {
	    		                uOffset = (iY * (U32)iWidth) + iX;
								/* If a transparent pixel, write the Y average */
								if (!(*(pu8TransByte + iBlkY*iWidth + iBlkX)))	{
								   pu8YPlace[uOffset] = Yavg;
								}
	    	    	        } /* for (iBlkX ... */
	    		        } /* for (iBlkY ... */
					}
				}
	        } /* for (iXblk ... */
	    } /* for (iYblk ... */
	}
	else {  /* Fast, no transparency subsample */

		/* Walk through all 2x2 blocks in image */
    	for (iYblk = 0; iYblk < iHeight; iYblk += 2) {
	        for (iXblk = 0; iXblk < iWidth; iXblk += 2) {

				/* Reset pixel sum variables and bias array pointer */
        	    uLut = uLvt = 0;
            	piFbp = iFbias;

				/* Walk through all pixels in each 4x4 block */
            	for (iBlkY = 0, iY = iYblk; iBlkY < 2; iBlkY++, iY++) {
                	for (iBlkX = 0, iX = iXblk; iBlkX < 2; iBlkX++, iX++) {

						/* Calculate offset from beginning of buffer */
	                    uOffset = 2UL * ((iY * (U32)iWidth) + iX);

						/* Accumulate biased pixel sums */
    	                uLvt += *piFbp * pu8Byte[uOffset];
        	            uLut += *piFbp * pu8Byte[uOffset+1];
						piFbp++;

	                } /* for (iBlkX ... */
    	        } /* for (iBlkX ... */

				/* Calculate and store biased U and V pixel averages */
	            *pu8VPlace++ = (U8)(((uLut+50) / 100) & 0xFF);
    	        *pu8UPlace++ = (U8)(((uLvt+50) / 100) & 0xFF);

	        } /* for (iXblk ... */
	    } /* for (iYblk ... */
	}  /* else no transparency */
	return;
}

#endif /* SIMULATOR */

/***************************************************************************
 *
 * BuildTransparencyRange is called from ConvertToYVU9
 *
 * Build R, G, B transparency range, 1st try to get histogram, then
 * cut off N = TRANSPARENCY_CUTOFF % from both end. 
 * Use these 2 values as transparency range.
 *
 * returns nothing but will update pOutput fields
 *
 ***************************************************************************/

static PIA_Boolean BuildTransparencyRange(U32 uThreshold, 
								   PU32 puBuckets, 
								   PTR_COLOR_RANGE pcrBuiltTransparencyRange
								   )
{
    I32 iCnt;  /* Counter for color ranges */
    PU32 puRbucket, puGbucket, puBbucket;
    U32 uLowR, uLowG, uLowB, uHighR, uHighG, uHighB;
    U32 uSumR = 0L, uSumG = 0L, uSumB = 0L;
	I32 iTemp;
    
    puRbucket = puBuckets;
    puGbucket = puBuckets + 256;
    puBbucket = puBuckets + 512;
    
    /* get the RGB range low bound */        
    for (iCnt = 0; iCnt < 256; iCnt++) {
        uSumB += puBbucket[iCnt];
        if (uSumB >= uThreshold)
           break;
    }
    uLowB = iCnt;

    for (iCnt = 0; iCnt < 256; iCnt++) {
        uSumG += puGbucket[iCnt];
        if (uSumG >= uThreshold)
           break;
    }
    uLowG = iCnt;

    for (iCnt = 0; iCnt < 256; iCnt++) {
        uSumR += puRbucket[iCnt];
        if (uSumR >= uThreshold)
           break;
    }
    uLowR = iCnt;
               
    /* get the RGB range high bound */        

	uSumB = 0L;
    for (iCnt = 255; iCnt >= 0; iCnt--) {
        uSumB += puBbucket[iCnt];
        if (uSumB >= uThreshold)
           break;
    }							 
    uHighB = iCnt;

	uSumG = 0L;
    for (iCnt = 255; iCnt >= 0; iCnt--) {
        uSumG += puGbucket[iCnt];
        if (uSumG >= uThreshold)
           break;
    }
    uHighG = iCnt;

	uSumR = 0L;
    for (iCnt = 255; iCnt >= 0; iCnt--) {
        uSumR += puRbucket[iCnt];
        if (uSumR >= uThreshold)
           break;
    }
    uHighR = iCnt;

	/* Now Add some Conservatism */

	iTemp = (uHighB + ((uHighB - uLowB) >> 1));
	uHighB = (iTemp > 255) ? 255 : (U32) iTemp;
	iTemp = (uLowB - ((uHighB - uLowB) >> 1));
	uLowB = (iTemp < 0) ? 0 : (U32) iTemp;
	
	iTemp = (uHighG + ((uHighG - uLowG) >> 1));
	uHighG = (iTemp > 255) ? 255 : (U32) iTemp;
	iTemp = (uLowG - ((uHighG - uLowG) >> 1));
	uLowG = (iTemp < 0) ? 0 : (U32) iTemp;

	iTemp = (uHighR + ((uHighR - uLowR) >> 1));
	uHighR = (iTemp > 255) ? 255 : (U32) iTemp;
	iTemp = (uLowR - ((uHighR - uLowR) >> 1));
	uLowR = (iTemp < 0) ? 0 : (U32) iTemp;
    
    /* update RGB range in pOutput data structure */
	pcrBuiltTransparencyRange->u16Tag = COLOR_RANGE_TAG;
    pcrBuiltTransparencyRange->u8BlueHigh = (U8)uHighB;
    pcrBuiltTransparencyRange->u8BlueLow  = (U8)uLowB;
    pcrBuiltTransparencyRange->u8GreenHigh= (U8)uHighG;
    pcrBuiltTransparencyRange->u8GreenLow = (U8)uLowG;
    pcrBuiltTransparencyRange->u8RedHigh  = (U8)uHighR;
    pcrBuiltTransparencyRange->u8RedLow   = (U8)uLowR;

	return(TRUE);

}

/***************************************************************************
 *
 * BuildTransparencyBitmask is called from ConvertToYVU9
 *
 * Accepts a transparency bitmask at 1 byte per pixel in pu8TransPixels.
 * It then packs this information down to 1 bit per pixel and puts it in
 * pInput->pu8TransparencyBitmask.
 *
 * returns nothing but will update pOutput fields
 *
 ***************************************************************************/

static PIA_Boolean BuildTransparencyBitmask(PTR_CCIN_INST pInst,
 									 PTR_COLORIN_INPUT_INFO pInput,
									 PU8 pu8TransPixels	/* input */
									 )
{
    PTR_PRIVATE_COLORIN_DATA   pPrivate = pInst->pColorInPrivate;                     
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
	I32 iX, iY;					/* Loop Counters */
    I32 iWidth, iHeight;		/* resolution of output frame */
    U32 uTempData=0L;			/* Hold Partial DWORDS while being built */
    U32 uMaskCnt=0L;			/* Keeps track of which bit in current DWORD */
	PTR_TRANSPARENCY_MASK	pTransMask =  pParms->pTransparencyMask;
    PU32 puBitMaskArray = (PU32) pTransMask->pu8TransBuffer;

    iWidth  = (I32)pInst->dImageDim.w;
    iHeight = (I32)pInst->dImageDim.h;

	for (iY = 0; iY < iHeight; iY++) {
        for (iX = 0; iX < iWidth; iX++) {

			/* Test to see if this pixel is opaque */

	      	if (*(pu8TransPixels++))
    	       	uTempData |= uMaskarray[uMaskCnt]; 
							
	       	uMaskCnt += 1;

			/* If end of line, fill remaining bits with the */
			/* same value as the last actual image bit. */

			if ((iX == iWidth - 1) && (uMaskCnt != 32)) {
    	       	if (*(pu8TransPixels - 1)) {
					while (uMaskCnt < 32) {
            	      	uTempData |= uMaskarray[uMaskCnt]; 
       					uMaskCnt += 1;
					}
				} 
				else {
					uMaskCnt = 32;
				}
			}

			/* Advance to the next DWORD in Trans Bitmask*/

	       	if (uMaskCnt == 32) {
        	  	*puBitMaskArray++ = uTempData; 
    	       	uMaskCnt = 0L;
           		uTempData = 0L;
	       	}
	    } /* for (iX ... */
	} /* for (iY ... */

	return(TRUE);
}


/***************************************************************************
 *
 * BuildTransparencyByteMask
 *
 * Accepts a transparency bitmask at 1 bit per pixel in
 * pInput->pu8TransparencyBitmask.  It then expands this information to
 * 1 byte per pixel and puts it in pu8TransPixels.
 *
 * returns nothing but will update pDirtyRect and pTransContext fields
 *
 ***************************************************************************/

static PIA_Boolean BuildTransparencyBytemask(PTR_CCIN_INST pInst,
 									  PTR_COLORIN_INPUT_INFO pInput,
									  PU8 pu8TransPixels	
										)
{
    PTR_PRIVATE_COLORIN_DATA   pPrivate = pInst->pColorInPrivate;                     
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
	I32 iX, iY;					/* Loop Counters */
    I32 iWidth, iHeight;		/* resolution of output frame */
    U32 uTempData = 0L;			/* Hold Partial DWORDS while being built */
    U32 uMaskCnt = 0L;			/* Keeps track of which bit in current DWORD */
	PTR_TRANSPARENCY_MASK pTransMask =  pParms->pTransparencyMask;
    PU8 puBitMaskArray;
	PU8 pu8ByteMaskArray = pu8TransPixels;

	if (pTransMask->cfPixelSize != CF_GRAY_1) {
		goto bail;
	}

	puBitMaskArray = pTransMask->pu8TransBuffer;

    iWidth  = (I32)pInst->dImageDim.w;
    iHeight = (I32)pInst->dImageDim.h;

	uTempData = *puBitMaskArray++;;      /* Get first DWORD of TransBitmask */

	for (iY = 0; iY < iHeight; iY++) {
        for (iX = 0; iX < iWidth; iX++) {

			/* Test to see if this pixel is opaque */

			if (uTempData & uMaskarray[uMaskCnt]) {
				*pu8ByteMaskArray++ = 1;
			} 
			else {
				*pu8ByteMaskArray++ = 0;
			}
										
	       	uMaskCnt += 1;

			/* If end of line, ignore remaining bits. */

			if (iX == iWidth - 1) {
				uMaskCnt = 32;
			}

			/* Advance to the next DWORD in Trans Bitmask*/

	       	if (uMaskCnt == 32) {
        	  	uTempData = *puBitMaskArray++;
    	       	uMaskCnt = 0;
	       	}
	    } /* for (iX ... */
	} /* for (iY ... */

	return(TRUE);

bail:
	return(FALSE);
}


#ifdef BETA_BANNER

/***************************************************************************
 *
 * InsertBetaBanner
 *
 * Accepts the YVU24 Image and inserts a transparent beta banner 
 * in the lower right corner.
 *
 ***************************************************************************/

static void InsertBetaBanner(PTR_CCIN_INST pInst,
                             PTR_COLORIN_OUTPUT_INFO pOutput,
							 PU8 pu8TransPixels)

{
    I32 iWidth, iHeight;		/* resolution of output frame */
	I32 iX, iY;					/* Loop Counters */
    PU8 pu8YPlace;              /* pointer to the place storing Y pixels */
    PU8 pu8UVPlace;             /* byte-wide pointer to non-subsampled UV for both Endians */
	PU8 pu8RGB24Logo;			/* This is a pointer to the logo banner to be inserted */
	U8  u8tR, u8tG, u8tB;		/* RGB transparent color for logo */
	U32 uYVUval;				/* Temp holder for YVU conversion of logo colors */
	I32 iLogoWidth, iLogoHeight;/* Logo banner sizes */ 
	I32 iLogoPitch;	
	PMatrixSt	pMat;

	pu8YPlace = pOutput->pu8ExternalOutputData ? pOutput->pu8ExternalOutputData : pOutput->pu8OutputData;
    pu8UVPlace = pInst->pColorInPrivate->pu8TempBuffer;

    iWidth  = (int)pInst->dImageDim.w;
    iHeight = (int)pInst->dImageDim.h;

	/* Get logo banner pointer and sizes */
	pMat = HiveGetCopyrightBits( pInst );

	pu8RGB24Logo = (PU8) pMat->pi16; 
	iLogoWidth = pMat->NumCols;
	iLogoHeight = pMat->NumRows;
	iLogoPitch = pMat->Pitch;


	/* Do different things depending on image resolution */

	if ((iHeight < 3*iLogoHeight) || (iWidth < 3*iLogoWidth)) {
		/* Reduce height of logo to just version information */
		iLogoHeight = 16;
		iLogoWidth -= 4;
		/* Adjust Pointers accordingly */
		pu8RGB24Logo += (iLogoPitch*(iLogoHeight-16));
	}

	/* Set Starting Point */
	/* Starting point is last line, since RGB Banner is Upside down */
	pu8YPlace = (pu8YPlace + (iWidth * (iHeight - 1)) + (iWidth - iLogoWidth));
	pu8UVPlace = (pu8UVPlace + 2*((iWidth * (iHeight - 1)) + (iWidth - iLogoWidth)));
	pu8TransPixels = (pu8TransPixels + (iWidth * (iHeight - 1)) + (iWidth - iLogoWidth));

	/* Bail out if the frame size is too small */

	if ((iHeight < iLogoHeight * 2) || (iWidth < iLogoWidth * 2))
		return;

	/* Find logo banner transparent color, first pixel upper left corner. */

	u8tR = *(pu8RGB24Logo+2);
	u8tG = *(pu8RGB24Logo+1);
	u8tB = *pu8RGB24Logo;

	/* Do different things depending on transparency setting */

	if ((pInst->ciscSequenceControlsUsed & CIN_TRANSPARENCY) & !(pInst->pColorInParms->bAlphaChannel)) {
		
		/* Transparency is on */

		/* Now loop through pixels, drawing logo into non-transparent regions */

		for (iY = 0; iY < iLogoHeight; iY++) {
			for (iX = 0; iX < iLogoWidth; iX++) {
				/* Is this a logo transparent color, or is video frame transparent here? */
				if (((*(pu8RGB24Logo+2) == u8tR) &&
					 (*(pu8RGB24Logo+1) == u8tG) &&
					 (*pu8RGB24Logo == u8tB)) ||
					 (*pu8TransPixels == 0)) {
					/* Don't change YVU in original image, transparent logo color */
					pu8YPlace++;
					pu8UVPlace += 2; /* Skip U and V */
				} else {
					/* Fill in Logo Color, start by converting to YVU */
					uYVUval = RtoYUV[*(pu8RGB24Logo+2)] + GtoYUV[*(pu8RGB24Logo+1)] + BtoYUV[*pu8RGB24Logo];
					*pu8UVPlace++ = (U8)(uYVUval);
					*pu8UVPlace++ = (U8)(uYVUval >> 8);
					*pu8YPlace++ = (U8)(uYVUval >> 16);
				}
				pu8TransPixels++;
				pu8RGB24Logo += 3;  /* Move to next pixel */
			}
			pu8RGB24Logo += (iLogoPitch - 3*iLogoWidth);
			pu8YPlace -= (iWidth+iLogoWidth);
			pu8UVPlace -= 2*(iWidth+iLogoWidth);
			pu8TransPixels -= (iWidth+iLogoWidth);
		}

	} else {

		/* Transparency is off */

		/* Now loop through pixels, drawing logo in */

		for (iY = 0; iY < iLogoHeight; iY++) {
			for (iX = 0; iX < iLogoWidth; iX++) {
				/* Is this a logo transparent color? */
				if ((*(pu8RGB24Logo+2) == u8tR) &&
					(*(pu8RGB24Logo+1) == u8tG) &&
					(*pu8RGB24Logo == u8tB)) {
					/* Don't change YVU in original image, transparent logo color */
					pu8YPlace++;
					pu8UVPlace += 2; /* Skip U and V */
				} else {
					/* Fill in Logo Color, start by converting to YVU */
					uYVUval = RtoYUV[*(pu8RGB24Logo+2)] + GtoYUV[*(pu8RGB24Logo+1)] + BtoYUV[*pu8RGB24Logo];
					*pu8UVPlace++ = (U8)(uYVUval);
					*pu8UVPlace++ = (U8)(uYVUval >> 8);
					*pu8YPlace++ = (U8)(uYVUval >> 16);
				}
				pu8RGB24Logo += 3;  /* Move to next pixel */
			}
			pu8RGB24Logo += (iLogoPitch - 3*iLogoWidth);
			pu8YPlace -= (iWidth+iLogoWidth);
			pu8UVPlace -= 2*(iWidth+iLogoWidth);
		}
	}

	return;
}

#endif /* BETA_BANNER */

