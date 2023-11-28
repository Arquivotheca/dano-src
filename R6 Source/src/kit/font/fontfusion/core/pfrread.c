/*
 * PFRREAD.c
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Sampo Kaasila, Robert Eggers, Mike Dewsnap
 *
 * This software is the property of Bitstream Inc. and it is furnished
 * under a license and may be used and copied only in accordance with the
 * terms of such license and with the inclusion of the above copyright notice.
 * This software or any other copies thereof may not be provided or otherwise
 * made available to any other person or entity except as allowed under license.
 * No title to and no ownership of the software or intellectual property
 * contained herein is hereby transferred. This information in this software
 * is subject to change without notice
 */


/************************** p f r r e a d . c ******************************
 *                                                                           *
 * Migrated from PEEK_PFR.C (Portable font resource dump program)            *
 * to become a dynamic, low code footprint PFR parser for T2K                *
 *                                                                           *
 ****************************************************************************/


#include <ctype.h>
#include "syshead.h"

#include "config.h"

#ifdef ENABLE_PFR

#include "dtypes.h"
#include "tsimem.h"
#include "t2kstrm.h"
#include "glyph.h"
#include "truetype.h"
#include "util.h"
#include "t2k.h"
#include "auxdata.h"


#ifndef DEBUG
#define DEBUG  0
#endif


#if DEBUG
static int debugOn = 1;
#endif

#if DEBUG
#define SHOW(X) if (debugOn) printf(#X " = %ld\n", (long)(X))
#else
#define SHOW(X)
#endif

#if DEBUG
#define LOG_CMD( s, d ) if (debugOn) printf("%s %d\n", s, d )
#else
#define LOG_CMD( s, d )
#endif

/***** MACRO DEFINITIONS *****/
#define NEXT_BYTE(P) (*(P++))
#define NEXT_WORD(P) (P+=2, ((int16)P[-2] << 8) + P[-1])
#define NEXT_LONG(P) (P+=3, ((uint32)P[-3] << 16) + ((uint32)P[-2] << 8) + P[-1])
#define NEXT_EXT_LONG(P) (P+=4, ((uint32)P[-4] << 24) + ((uint32)P[-3] << 16) + ((uint32)P[-2] << 8) + P[-1])

#if EXPLICIT_SIGN_EXTENSION
#define NEXT_SBYTE(P) ((int8)(P+=1, (P[-1] >= 128)? P[-1] - 256: P[-1]))
#define NEXT_SWORD(P) ((int16)(P+=2, (P[-2] >= 128)? \
	(((int16)P[-2] << 8) + P[-1] - 65536): \
	(((int16)P[-2] << 8) + P[-1])))
#else
#define NEXT_SBYTE(P) ((int8)(*(P++)))
#define NEXT_SWORD(P) (int16)((P+=2, ((int16)P[-2] << 8) + P[-1]))
#endif

#define NEXT_UBYTE(P) ((uint8)(*(P++)))

#define NEXT_UWORD(P) (uint16)((P+=2, ((int16)P[-2] << 8) + P[-1]))

#define NEXT_SLONG(P) ((int32)(P+=3, ((uint32)P[-3] << 16) + ((uint32)P[-2] << 8) + P[-1]))

#define NEXT_ULONG(P) ((uint32)(P+=3, ((uint32)P[-3] << 16) + ((uint32)P[-2] << 8) + P[-1]))

#define IDWININSTINFO   0x8001  /* Installation Info tag value */
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#define ABS(x)		(((x)<0)?-(x):(x))
#define BLUE_SHIFT  459

typedef struct state_tag
    {
    uint8   mask;
    uint8   black;
    uint8  *pByte;
    int32   bitsLeft;	/* used for both debug and double time processing */
    int32   whiteLeft;
    int32   blackLeft;
    void (*GetPixRun)(struct state_tag *pState);
    int16   xSize, ySize;
    int16   x0, x1;
    int16   y;
    int16   yIncrement;
#if DEBUG
    int32 rowsLeft;		/* used only for debug printing */
    int32 bitsPerRow;	/* used only for debug printing */
#endif
    } state_t;


#ifdef ENABLE_SBIT

typedef void (*GetPixRun_t)(struct state_tag *pState);

static char GetPixRunInRow(
    state_t *pState);

static void GetPixRun0(
    state_t *pState);

static void GetPixRun1(
    state_t *pState);

static void GetPixRun2(
    state_t *pState);

static GetPixRun_t GetPixRun[] =
    {
    GetPixRun0,
    GetPixRun1,
    GetPixRun2
    };
#endif	/* ENABLE_SBIT */

typedef struct nibble_tag
	{
	uint8 *pByte;
	int16	phase;
	} nibble_t;
	
/* Instruction opcodes */
#define LINE1	0
#define HLINE2	1
#define VLINE2	2
#define HLINE3	3
#define VLINE3	4
#define GLINE	5
#define GMOVE	6
#define HVCRV1	7
#define VHCRV1	8
#define HVCRV2	9
#define VHCRV2	10
#define HVCRV3	11
#define VHCRV3	12
#define GCRV2	13
#define GCRV3	14
#define GCRV4	15

#if CSR_DEBUG >= 3
char *opcodeTable[] =
	{
	"LINE1",
	"HLINE2",
	"VLINE2",
	"HLINE3",
	"VLINE3",
	"GLINE",
	"GMOVE",
	"HVCRV1",
	"VHCRV1",
	"HVCRV2",
	"VHCRV2",
	"HVCRV3",
	"VHCRV3",
	"CRV2",
	"GCRV3",
	"GCRV4"
	};
#endif

/* Encoding/Decoding table for GCRV2 argument format */
static const uint16 gcrv2FormatTable[] =
	{
	(3 << 10) + (3 << 8) + (3 << 6) + (3 << 4) + (3 << 2) + (3 << 0),
	(0 << 10) + (3 << 8) + (2 << 6) + (2 << 4) + (2 << 2) + (2 << 0),
	(3 << 10) + (0 << 8) + (2 << 6) + (2 << 4) + (2 << 2) + (2 << 0),
	(2 << 10) + (2 << 8) + (2 << 6) + (2 << 4) + (0 << 2) + (3 << 0),
	(2 << 10) + (2 << 8) + (2 << 6) + (2 << 4) + (3 << 2) + (0 << 0),
	(2 << 10) + (2 << 8) + (2 << 6) + (2 << 4) + (2 << 2) + (2 << 0),
	(0 << 10) + (2 << 8) + (2 << 6) + (2 << 4) + (2 << 2) + (2 << 0),
	(2 << 10) + (0 << 8) + (2 << 6) + (2 << 4) + (2 << 2) + (2 << 0),
	(2 << 10) + (2 << 8) + (2 << 6) + (2 << 4) + (0 << 2) + (2 << 0),
	(2 << 10) + (2 << 8) + (2 << 6) + (2 << 4) + (2 << 2) + (0 << 0),
	(0 << 10) + (0 << 8) + (2 << 6) + (2 << 4) + (2 << 2) + (2 << 0),
	(1 << 10) + (1 << 8) + (1 << 6) + (1 << 4) + (1 << 2) + (1 << 0),
	(0 << 10) + (1 << 8) + (1 << 6) + (1 << 4) + (1 << 2) + (1 << 0),
	(1 << 10) + (0 << 8) + (1 << 6) + (1 << 4) + (1 << 2) + (1 << 0),
	(1 << 10) + (1 << 8) + (1 << 6) + (1 << 4) + (0 << 2) + (1 << 0),
	(1 << 10) + (1 << 8) + (1 << 6) + (1 << 4) + (1 << 2) + (0 << 0),	
	};
	
#if 0
/* Encoding table for arg formats of 0-2 except 0, 0 */
static const uint16 stdCurveFormatTable2[] =
	{
	8, 0, 1, 8, 2, 3, 4, 8, 5, 6, 7, 8, 8, 8, 8, 8
	};
	
/* Encoding table for arg format of 1-2 */
static const uint16 stdCurveFormatTable3[] =
	{
	4, 4, 4, 4, 4, 0, 1, 4, 4, 2, 3, 4, 4, 4, 4, 4
	};
#endif /* #if 0 */

/* Decoding table for HVCRV2 argument format */
static const uint16 hvcrv2Table[] =
	{
	0x0451, 0x0452, 0x0461, 0x0462, 0x0491, 0x0492, 0x04a1, 0x04a2,	
	0x0851, 0x0852, 0x0861, 0x0862, 0x0891, 0x0892, 0x08a1, 0x08a2
	};

/* Decoding table for VHCRV2 argument format */
static const uint16 vhcrv2Table[] =
	{
	0x0154, 0x0158, 0x0164, 0x0168, 0x0194, 0x0198, 0x01a4, 0x01a8,
	0x0254, 0x0258, 0x0264, 0x0268, 0x0294, 0x0298, 0x02a4, 0x02a8
	};

/* Decoding table for arg formats of 0-2 except 0, 0 */
static const uint16 gcrv3Table2[] =
	{
	0x0001, 0x0002, 0x0004, 0x0005, 0x0006, 0x0008, 0x0009, 0x000a
	};
	
/* Decoding table for arg formats of 1-2 except 0, 0 */
static const uint16 gcrv3Table3[] =
	{
	0x0005, 0x0006, 0x0009, 0x000a
	};
	
	
/***** GLOBAL VARIABLES *****/

/***** Function prototypes *****/
static kernClass *New_pfrkernClass( tsiMemObject *mem, uint8 *in , int16 extraItemSize);
static kernSubTable *New_pfrkernSubTable( tsiMemObject *mem, int appleFormat, uint8 *in, int16 extraItemSize );
static kernSubTable0Data *New_pfrkernSubTable0Data( tsiMemObject *mem, uint8 *in, int16 extraItemSize );

static void ReadGpsSegment(
    gpsExecContext_t *pGpsExecContext,
    nibble_t *pNibble,
    int16	*pType,
    int16	values[]);
    
static uint8 ReadNibble(
	nibble_t *pNibble);

static uint8 Read2Nibbles(
	nibble_t *pNibble);

static int16 Read3Nibbles(
	nibble_t *pNibble);
	
static int16 GetControlledCoord(
    gpsExecContext_t *pGpsExecContext,
    int16	dimension,
    int16	n);
    
static void ReadGpsPoint(
    gpsExecContext_t *pGpsExecContext,
    nibble_t *pNibble,
    uint16	argFormat,
    int16	values[]);
    
#if 0
static void ShowAdjustmentVector(
	uint8 *pByte,
	int16 nBytes);
#endif

#if 0
static uint8 ReadNibbleBack(
	nibble_t *pNibble);
#endif

static int8 Read2NibblesSigned(
	nibble_t *pNibble);

/* T2K internal functions */
static void BuildPFRCMAP( PFRClass *t , uint8 **pPhysicalFont, uint8 format, int16 standardSetWidth);
static void BuildPFRMetricsEtc( PFRClass *t );
static int GetAuxDataMetrics(
	PFRClass *t ,
	uint8 *pAuxData,
    uint32 nAuxBytes,
    unsigned short requestedBlockID,
    short *ascent, short *descent, short *lineGap, short *maxAW,
    uint32 *isFixedPitch
    );
static void PFRBuildChar( PFRClass *t, uint8 *p, long gpsOffset, uint16 gpsSize,
	F16Dot16 xScale, F16Dot16 yScale, F16Dot16 xPos, F16Dot16 yPos , FFT1HintClass *ffhint);
static uint8 BSearchCTbl ( PFRClass *t, uint16 *indexPtr, uint16 theValue);

static short GetGlyphYMax( GlyphClass *glyph );
static short GetGlyphYMin( GlyphClass *glyph );

static void FlipContourDirection(GlyphClass *glyph);

static void ReadOruTable(
	PFRClass *t,
    uint8   format,
    uint8 **ppBuff, 
    gpsExecContext_t *pSourceExecContext);

static void ReadGlyphElement(
	PFRClass *t,
    uint8 **ppBuff,
    charElement_t *pGlyphElement,
    long *pGpsOffset);

static void SetupTCB(PFRClass *t);

static void SetupTrans(PFRClass *t,
				F16Dot16 xScale, 
				F16Dot16 yScale, 
				F16Dot16 xPos, 
				F16Dot16 yPos);

static void TransformPoint(PFRClass *t, F16Dot16 x, F16Dot16 y, F16Dot16 *pXt, F16Dot16 *pYt);

static void SkipExtraItems(
    uint8 **ppByte);
	
static void SkipExtraStreamItems(
    InputStream *in);



void InitGpsExecContext(
    gpsExecContext_t *pGpsExecContext)
{
pGpsExecContext->nXorus = 0;
pGpsExecContext->nYorus = 0;
pGpsExecContext->xPrevValue = 0;
pGpsExecContext->yPrevValue = 0;
}

 
static void ReadOruTable(
	PFRClass *t,
    uint8 format,
    uint8 **ppBuff, 
    gpsExecContext_t *pGpsExecContext)
{
uint8  *pBuff;
int16   nXorus = 0, nYorus = 0;
int16   nXorusRead, nYorusRead;
uint8   oruFormat = 0;
int16   prevValue = 0;
int   ii;

pBuff = *ppBuff;

if (t->pfrType > 0)
	{
	unsigned char	extraLongFormat;
	int16	formatPhase;
	int16	nibblePhase;
	int16	delta;
	int16  *pOruTable;
	uint8	variableFormat = 0;

	/* Read X and Y oru counts; set extra long format flag */
	extraLongFormat = false;
	switch(format & 3)
		{
	case 0:
		nXorus = nYorus = 0;
		break;
		
	case 1:
	    nXorus = (int16)(*pBuff & 0x0f);
	    nYorus = (int16)((*pBuff >> 4) & 0x0f);
	    pBuff++;
	    break;
	    
	case 3:
		extraLongFormat = true;
	case 2:
	    nXorus = *(pBuff++);
	    nYorus = *(pBuff++);
	    break;
	    }

	/* Read and save the table values */
	nXorusRead = nYorusRead = 0;
	pOruTable = pGpsExecContext->xOruTable;
	nibblePhase = 0;
	formatPhase = 0;
/*	delta = 0; */
	while(true)
		{
		if (nXorusRead < nXorus)
			{
			ii = nXorusRead++;
			if (ii == 0)
				{
				pOruTable = pGpsExecContext->xOruTable;
				oruFormat = (uint8)((format & BIT_4)? BIT_0: 0);
				prevValue = 0;
				}
			}
		else if (nYorusRead < nYorus)
			{
			ii = nYorusRead++;
			if (ii == 0)
				{
				pOruTable = pGpsExecContext->yOruTable;
				oruFormat = (uint8)((format & BIT_5)? BIT_0: 0);
				prevValue = 0;
				}
			}
		else
			{
			break;
			}
		
		/* Set oru value format for values other than first in each dimension */
		if (ii > 0)
			{
			if (format & BIT_6)		/* Variable formatting? */
				{
				if (formatPhase == 0)
					{
				    /* Read format nibble if variable format values */
			    	if (nibblePhase == 0)
			    		{
			    		variableFormat = (uint8)(*pBuff >> 4);
			    		nibblePhase = 1;
			    		}
			    	else
			    		{
			    		variableFormat = (uint8)(*(pBuff++) & 0x0f);
			    		nibblePhase = 0;
			    		}
					formatPhase = 3;
					}
				else
					{
					variableFormat >>= 1;
					formatPhase--;
					}
				oruFormat = variableFormat;
				}
			else					/* Fixed formatting? */
				{
				oruFormat = 0;
				}
			}
		
	    /* Read and save the relative oru value */
	    if ((oruFormat & BIT_0) == 0)
	    	{
	    	/* Read unsigned value from next two nibbles */
	    	if (nibblePhase == 0)
	    		{
	    		delta = (int16)(*(pBuff++));
	    		}
	    	else
	    		{
	    		delta = (int16)((uint8)(*(pBuff++) << 4));
	    		delta = (int16)(delta + ((uint8)(*pBuff >> 4)));
	    		}
	    	}
	    else
	    	{
	    	if (extraLongFormat)
	    		{
		    	/* Read signed value from next four nibbles */
		    	if (nibblePhase == 0)
		    		{
		    		delta = (int16)((int8)(*(pBuff++)) << 8);
		    		delta = (int16)(delta + (int16)(*(pBuff++)));
		    		}
		    	else
		    		{
		    		delta = (int16)((int8)((*(pBuff++) & 0x0f) << 4) << 8);
		    		delta = (int16)(delta + (int16)(*(pBuff++) << 4));
		    		delta = (int16)(delta + (int16)((uint8)(*pBuff >> 4)));
		    		}
	    		}
	    	else
	    		{
		    	/* Read signed value from next three nibbles */
		    	if (nibblePhase == 0)
		    		{
		    		delta = (int16)((int8)(*(pBuff++)) << 4);
		    		delta = (int16)(delta + (int16)(*pBuff >> 4));
		    		nibblePhase = 1;
		    		}
		    	else
		    		{
		    		delta = (int16)((int8)(*(pBuff++) << 4) << 4);
		    		delta = (int16)(delta + (int16)(*(pBuff++)));
		    		nibblePhase = 0;
		    		}
				}
			}
	    pOruTable[ii] = prevValue = (int16)(prevValue + delta);
	    }

	/* Select Y oru table */
	pOruTable = pGpsExecContext->yOruTable;

	/* Insert leading ghost edge if required */
	if (format & BIT_2)
		{
		for (ii = nYorus - 1; ii >= 0; ii--)
			{
			pOruTable[ii + 1] = pOruTable[ii];
			}
		nYorus++;
		}

	/* Insert trailing ghost edge if required */
	if (nYorus & 1)
		{
		pOruTable[nYorus] = pOruTable[nYorus - 1];
		nYorus++;
		}
		    
	if (nibblePhase)
		pBuff++;				/* Round to next byte boundary */

	}
else
	{
	nXorus = nYorus = 0;
	if (format & BIT_2)
	    {
	    nXorus = (int16)(*pBuff & 0x0f);
	    nYorus = (int16)((*pBuff >> 4) & 0x0f);
	    pBuff++;
	    }
	else
	    {
	    if (format & BIT_0)
	        nXorus = *(pBuff++);

	    if (format & BIT_1)
	        nYorus = *(pBuff++);
	    }

	nXorusRead = nYorusRead = 0;
	prevValue = 0;
	while 
	    ((nXorusRead + nYorusRead) <
	     (nXorus + nYorus))
	    {
	    oruFormat = *(pBuff++);
	    for (ii = 0; ii < 8; ii++)
	        {
	        if (nXorusRead < nXorus)
	            {
	            pGpsExecContext->xOruTable[nXorusRead++] = prevValue = (int16)((oruFormat & BIT_0)?
							NEXT_WORD(pBuff):
							prevValue + (int16)(*(pBuff++)));
	            }
	        else if (nYorusRead < nYorus)
	            {
	            pGpsExecContext->yOruTable[nYorusRead++] = prevValue = (int16)((oruFormat & BIT_0)?
							NEXT_WORD(pBuff):
							prevValue + (int16)(*(pBuff++)));
	            }
	        else
	            {
	            break;
	            }
	        oruFormat >>= 1;
	        }
	    }
	}
	

pGpsExecContext->nXorus = nXorus;
pGpsExecContext->nYorus = nYorus;

*ppBuff = pBuff;
}


static void ReadGlyphElement(
	PFRClass *t,
    uint8 **ppBuff,
    charElement_t *pGlyphElement,
    long *pGpsOffset)
/*
 *  Reads one glyph element of a compound glyph program string starting
 *  at the specified point, puts the glyph element specs into the glyph
 *  element structure and updates the buffer pointer.
 */
{
uint8  *pBuff;
uint8   format;
int16	ii;
gpsExecContext_t gpsExecContext;
int16   pos = 0, xPos, yPos;
int16   scale = 0, xScale, yScale;
uint16  gpsSize = 0;
int32	gpsOffset = 0;
uint32  tmpuint32;

pBuff = *ppBuff;

if (t->pfrType > 0)			/* Compressed PFR format? */
	{
	format = *(pBuff++);    /* Read compound glyph format */
	
	/* Read scale and position data for X and Y dimensions */
	for (ii = 0;;)
		{
		switch (format % 6)
			{
		case 0:
			scale = 1 << 12;
			pos = 0;
			break;
			
		case 1:
			scale = 1 << 12;
			pos = (int16)((int8)(NEXT_BYTE(pBuff)));
			break;
			
		case 2:
			scale = 1 << 12;
			pos = (int16)NEXT_WORD(pBuff);
			break;

		case 3:
			if (t->shortScaleFactors)
				{
				scale = (int16)(((int16)NEXT_BYTE(pBuff) + 1) << 5);
				}
			else
				{
				scale = (int16)NEXT_WORD(pBuff);
				}
			pos = (int16)((int8)(NEXT_BYTE(pBuff)));
			break;
			
		case 4:
			if (t->shortScaleFactors)
				{
				scale = (int16)(((int16)NEXT_BYTE(pBuff) + 1) << 5);
				}
			else
				{
				scale = (int16)NEXT_WORD(pBuff);
				}
			pos = (int16)NEXT_WORD(pBuff);
			break;
			
		case 5:
			scale = 0;
			pos = 0;
			break;
			}
		format = (uint8)(format / 6);
		if (ii != 0)
			{
			pGlyphElement->yScale = (int32)scale << 4;
			pGlyphElement->yPosition = (int32)pos << 16;
			break;
			}
		ii++;
		pGlyphElement->xScale = (int32)scale << 4;
		pGlyphElement->xPosition = (int32)pos << 16;
		}

	/* Read glyph prog string size/offset info */
	switch(format)
		{
	case 0:
		gpsSize = (uint16)NEXT_BYTE(pBuff);
		gpsOffset = *pGpsOffset - (int32)gpsSize;
		*pGpsOffset = gpsOffset;
		break;
		
	case 1:
		gpsSize = (uint16)(NEXT_BYTE(pBuff) + 256);
		gpsOffset = *pGpsOffset - (int32)gpsSize;
		*pGpsOffset = gpsOffset;
		break;
	
	case 2:
		gpsSize = (uint16)NEXT_WORD(pBuff);
		gpsOffset = *pGpsOffset - (int32)gpsSize;
		*pGpsOffset = gpsOffset;
		break;

	case 3:
		tmpuint32 = (uint32)NEXT_LONG(pBuff);
		gpsSize = (uint16)(tmpuint32 >> 15);
		gpsOffset = *pGpsOffset - ((int32)tmpuint32 & 0x7fff);
		break;
	
	case 4:
		tmpuint32 = (uint32)NEXT_LONG(pBuff);
		gpsSize = (uint16)(tmpuint32 >> 15);
		gpsOffset = ((int32)tmpuint32 & 0x7fff);
		break;
	
	case 5:
		tmpuint32 = (uint32)NEXT_EXT_LONG(pBuff);
		gpsSize = (uint16)(tmpuint32 >> 23);
		gpsOffset = ((int32)tmpuint32 & 0x7fffff);
		break;
	
	case 6:
		gpsSize = (uint16)NEXT_WORD(pBuff);
		gpsOffset = (int32)NEXT_LONG(pBuff);
		break;
		}
	pGlyphElement->glyphProgStringSize = gpsSize;		
	pGlyphElement->glyphProgStringOffset = (uint32)gpsOffset;
	}
else						/* Standard PFR format? */
	{
	format = *(pBuff++);    /* Read compound glyph format */

	xScale = (int16)((format & BIT_4)?
	    NEXT_WORD(pBuff):
	    1 << 12);

	yScale = (int16)((format & BIT_5)?
	    NEXT_WORD(pBuff):
	    1 << 12);

	pGlyphElement->xScale = ((int32)xScale) << 4;
	pGlyphElement->yScale = ((int32)yScale) << 4;

	InitGpsExecContext(&gpsExecContext);
	ReadGpsArgs(
	    &pBuff, 
	    format, 
	    &gpsExecContext, 
	    &xPos,
	    &yPos);
	pGlyphElement->xPosition = ((int32)xPos) << 16; 
	pGlyphElement->yPosition = ((int32)yPos) << 16; 

	pGlyphElement->glyphProgStringSize = (uint16)((format & BIT_6)?
	    NEXT_WORD(pBuff):
	    (*(pBuff++)));

	pGlyphElement->glyphProgStringOffset = (format & BIT_7)?
	    (uint32)NEXT_LONG(pBuff):
	    (uint32)((uint16)NEXT_WORD(pBuff));
	}
	
*ppBuff = pBuff;
}


void ReadGpsArgs(
    uint8 **ppByte,
    uint8 format,
    gpsExecContext_t *pGpsExecContext,
    int16 *pX,
    int16 *pY)
{
uint8 *pByte;

pByte = *ppByte;

switch (format & 0x03)
    {
case 0: /* 1-byte index into controlled oru table */
    *pX = pGpsExecContext->xOruTable[NEXT_BYTE(pByte)];
    break;

case 1: /* 2-byte signed integer orus */
    *pX = (int16)(NEXT_WORD(pByte));
    break;

case 2:
    *pX = (int16)(pGpsExecContext->xPrevValue + (int16)((int8)(NEXT_BYTE(pByte))));
    break;

case 3:
    *pX = pGpsExecContext->xPrevValue;
    break;
    }

pGpsExecContext->xPrevValue = *pX;

switch ((format >> 2) & 0x03)
    {
case 0: /* 1-byte index into controlled oru table */
    *pY = pGpsExecContext->yOruTable[NEXT_BYTE(pByte)];
    break;

case 1: /* 2-byte signed integer orus */
    *pY = (int16)(NEXT_WORD(pByte));
    break;

case 2:
    *pY = (int16)(pGpsExecContext->yPrevValue + (int16)((int8)(NEXT_BYTE(pByte))));
    break;

case 3:
    *pY = pGpsExecContext->yPrevValue;
    break;
    }

pGpsExecContext->yPrevValue = *pY;

*ppByte = pByte;
}


static void ReadGpsSegment(
    gpsExecContext_t *pGpsExecContext,
    nibble_t *pNibble,
    int16	*pType,
    int16	values[])
/*
 *	Reads one opcode and its associated arguments.
 * 	Sets *pType to the type of instruction:
 *		0: Move
 *		1: Line
 *		2: Curve
 *  Updates the values array with the values of the associated arguments
 *	in order x1, y1, x2, y2, x3, y3.
 */
{
uint8	opcode;
uint16	argFormat;
uint16	format1, format2, format3;
int16	nTabs;

values[0] = pGpsExecContext->xPrevValue;
values[1] = pGpsExecContext->yPrevValue;

opcode = (uint8)((*pType >= 0)? ReadNibble(pNibble): GMOVE);
#if CSR_DEBUG >= 3
printf("%s\n", opcodeTable[opcode]);
#endif
switch (opcode)
    {
case LINE1:	/* Horiz or vert line to controlled coordinate */
	*pType = 1;
	argFormat =  (uint16)ReadNibble(pNibble);
	nTabs = (int16)((argFormat & BIT_2)?
		(argFormat & 0x07) - 8:
		(argFormat & 0x07) + 1);
	if (argFormat & BIT_3)	/* Vertical line? */
		{
		values[1] = GetControlledCoord(pGpsExecContext, 1, nTabs);
		}
	else					/* Horizontal line? */
		{
		values[0] = GetControlledCoord(pGpsExecContext, 0, nTabs);
		}
	goto L0;
		
case HLINE2: 	/* Horizontal line to signed 1-byte relative orus */
	*pType = 1;
	values[0] = 
		(int16)(pGpsExecContext->xPrevValue + 
			(int16)((int8)Read2NibblesSigned(pNibble)));
	goto L0;
	
case VLINE2:	/* Vertical line to signed 1-byte relative orus */
	*pType = 1;
	values[1] = 
		(int16)(pGpsExecContext->yPrevValue + 
			(int16)((int8)Read2NibblesSigned(pNibble)));
	goto L0;
	
case HLINE3:	/* General horizontal line */
	*pType = 1;
	values[0] = 
		(int16)(pGpsExecContext->xPrevValue +
			Read3Nibbles(pNibble));
	goto L0;
	
case VLINE3:	/* General vertical line */
	*pType = 1;
	values[1] = 
		(int16)(pGpsExecContext->yPrevValue +
			Read3Nibbles(pNibble));
L0:	
	pGpsExecContext->xPrev2Value = pGpsExecContext->xPrevValue;
	pGpsExecContext->yPrev2Value = pGpsExecContext->yPrevValue;
	pGpsExecContext->xPrevValue = values[0];
	pGpsExecContext->yPrevValue = values[1];
	break;
		
case GLINE:		/* General line */
	*pType = 1;
	argFormat = ReadNibble(pNibble);
	ReadGpsPoint(pGpsExecContext, pNibble, argFormat, values);
	break;
	
case GMOVE:		/* Move instruction */
	*pType = 0;
	argFormat = ReadNibble(pNibble);
	ReadGpsPoint(pGpsExecContext, pNibble, argFormat, values);
	break;
	
case HVCRV1:	/* Horizontal-to-vertical quadrant */
	argFormat = 0x08a2;
	goto L1;

case VHCRV1:
	argFormat = 0x02a8;
	goto L2;
	
case HVCRV2:
	argFormat = ReadNibble(pNibble);
	argFormat = hvcrv2Table[argFormat];
	goto L1;		
	
case VHCRV2:
	argFormat = ReadNibble(pNibble);
	argFormat = vhcrv2Table[argFormat];
	goto L2;
	
case HVCRV3:
	argFormat = Read2Nibbles(pNibble);
	argFormat = 
		(uint16)(((argFormat & 0xc0) << 4) +
		((argFormat & 0x3c) << 2) +
		(argFormat & 0x03));
L1:	/* Horizontal-to-vertical curve quadrant */
	*pType = 2;
	ReadGpsPoint(pGpsExecContext, pNibble, argFormat, values);
	values[2] = GetControlledCoord(pGpsExecContext, 0, 0);
	values[3] = values[1];
	ReadGpsPoint(pGpsExecContext, pNibble, (uint16)(argFormat >> 4), values + 2);
	values[4] = values[2];
	values[5] = GetControlledCoord(pGpsExecContext, 1, 0);
	ReadGpsPoint(pGpsExecContext, pNibble, (uint16)(argFormat >> 8), values + 4);
	break;
	
case VHCRV3:
	argFormat = Read2Nibbles(pNibble);
	argFormat = (uint16)(argFormat << 2);
L2:	/* Vertical-to-horizontal curve quadrant */
	*pType = 2;
	ReadGpsPoint(pGpsExecContext, pNibble, (uint16)argFormat, values);
	values[2] = values[0];
	values[3] = GetControlledCoord(pGpsExecContext, 1, 0);
	ReadGpsPoint(pGpsExecContext, pNibble, (uint16)(argFormat >> 4), values + 2);
	values[4] = GetControlledCoord(pGpsExecContext, 0, 0);
	values[5] = values[3];
	ReadGpsPoint(pGpsExecContext, pNibble, (uint16)(argFormat >> 8), values + 4);
	break;
	
case GCRV2:
	argFormat = ReadNibble(pNibble);
	argFormat = gcrv2FormatTable[argFormat];
	goto L3;
	
case GCRV3:
	argFormat = Read2Nibbles(pNibble);
	format1 = gcrv3Table2[argFormat & 0x07];
	format2 = gcrv3Table3[(argFormat >> 3) & 0x03];
	format3 = gcrv3Table2[(argFormat >> 5) & 0x07];
	argFormat = (uint16)((format3 << 8) + (format2 << 4) + format1);
	goto L3;
	
case GCRV4:
	argFormat = ReadNibble(pNibble);
	argFormat = (uint16)((argFormat << 8) + Read2Nibbles(pNibble));
	
L3:	/* General curve */
	*pType = 2;
	values[0] = (int16)(values[0] + (int16)(pGpsExecContext->xPrevValue - pGpsExecContext->xPrev2Value));
	values[1] = (int16)(values[1] + (int16)(pGpsExecContext->yPrevValue - pGpsExecContext->yPrev2Value));
	ReadGpsPoint(pGpsExecContext, pNibble, (uint16)argFormat, values);
	values[2] = values[0];
	values[3] = values[1];
	ReadGpsPoint(pGpsExecContext, pNibble, (uint16)(argFormat >> 4), values + 2);
	values[4] = values[2];
	values[5] = values[3];
	ReadGpsPoint(pGpsExecContext, pNibble, (uint16)(argFormat >> 8), values + 4);
	break;
	}
}


static uint8 ReadNibble(
	nibble_t *pNibble)
{
if (pNibble->phase == 0)
	{
	pNibble->phase = 1;
	return (uint8)(*(pNibble->pByte) >> 4);
	}
else
	{
	pNibble->phase = 0;
	return (uint8)(*(pNibble->pByte++) & 0x0f);
	}
}


static uint8 Read2Nibbles(
	nibble_t *pNibble)
{
uint8	byte;

if (pNibble->phase == 0)
	{
	return *(pNibble->pByte++);
	}
else
	{
	byte = (uint8)(*(pNibble->pByte++) << 4);
	return (uint8)((*(pNibble->pByte) >> 4) + byte);
	}
}


static int16 Read3Nibbles(
	nibble_t *pNibble)
{
int16	word;

word = (int16)((int8)Read2NibblesSigned(pNibble));
word = (int16)((word << 4) + ReadNibble(pNibble));
if ((word >= -128) && (word < 128))
	{
	word = (int16)((word << 8) + (int16)Read2NibblesSigned(pNibble));
	}

return word;
}


static int16 GetControlledCoord(
    gpsExecContext_t *pGpsExecContext,
    int16	dimension,
    int16	n)
/*
 *	Returns the nth controlled coordinate in the specified diminsion 
 *  relative to the current position.
 *	If n = 0, returns the next controlled coordinate in the
 *	current direction. If the current direction is undefined, returns
 *	the current point.
 */
{
int16  *pOruTable;
int16	nOrus;
int16	z0;
int16	z;
int	ii;

if (dimension == 0)			/* X dimension? */
	{
	pOruTable = pGpsExecContext->xOruTable;
	nOrus = pGpsExecContext->nXorus;
	z0 = pGpsExecContext->xPrev2Value;
	z = pGpsExecContext->xPrevValue;
	}
else						/* Y dimension? */
	{
	pOruTable = pGpsExecContext->yOruTable;
	nOrus = pGpsExecContext->nYorus;
	z0 = pGpsExecContext->yPrev2Value;
	z = pGpsExecContext->yPrevValue;
	}

if (n == 0)
	{
	if (z > z0)				/* Current direction is right or up? */
		n = 1;
	else if (z < z0)		/* Current direction is left or down? */
		n = -1;
	else					/* Current direction is undefined? */
		return z;
	}

if (n > 0)					/* Tab right/up? */
	{
	for (ii = 0; ii < nOrus; ii++)
		{
		if (pOruTable[ii] > z)
			{
			ii = ii + n - 1;
			if (ii >= nOrus)
				ii = nOrus - 1;
			return pOruTable[ii];
			}
		}
	}
else						/* Tab left/down? */
	{
	for (ii = nOrus - 1; ii >= 0; ii--)
		{
		if (pOruTable[ii] < z)
			{
			ii = ii + n + 1;
			if (ii < 0)
				ii = 0;
			return pOruTable[ii];
			}
		}
	}
return z;
}


static void ReadGpsPoint(
    gpsExecContext_t *pGpsExecContext,
    nibble_t *pNibble,
    uint16	argFormat,
    int16	values[])
/*
 *	Reads one point from the glyph program string using the specified format.
 */
{
int16	delta;

switch (argFormat & 0x03)
    {
case 0: /* Default value */
    break;

case 1: /* Delta orus in signed nibble */
    values[0] = 
    	(int16)(pGpsExecContext->xPrevValue + 
    		(int16)ReadNibble(pNibble) - 8);
    break;

case 2:	/* Delta orus in signed byte */
	delta = (int16)((int8)Read2NibblesSigned(pNibble));
	if ((delta >= -8) && (delta < 8))
		{
		values[0] = GetControlledCoord(
			pGpsExecContext,
			0,
			(int16)((delta >= 0)? delta + 1: delta));
		}
	else
		{
    	values[0] = 
    		(int16)(pGpsExecContext->xPrevValue + delta);
    	}
    break;

case 3:	/* Delta orus in long word */
    values[0] = 
    	(int16)(pGpsExecContext->xPrevValue +
    		Read3Nibbles(pNibble));
    break;
    }

pGpsExecContext->xPrev2Value = pGpsExecContext->xPrevValue;
pGpsExecContext->xPrevValue = values[0];

switch ((argFormat >> 2) & 0x03)
    {
case 0: /* Default value */
    break;

case 1: /* Delta orus in signed nibble */
    values[1] = 
    	(int16)(pGpsExecContext->yPrevValue + 
    		(int16)ReadNibble(pNibble) - 8);
    break;

case 2:	/* Delta orus in signed byte */
	delta = (int16)((int8)Read2NibblesSigned(pNibble));
	if ((delta >= -8) && (delta < 8))
		{
		values[1] = GetControlledCoord(
			pGpsExecContext,
			1,
			(int16)((delta >= 0)? delta + 1: delta));
		}
	else
		{
    	values[1] = 
    		(int16)(pGpsExecContext->yPrevValue + delta);
    	}
    break;

case 3:	/* Delta orus in long word */
    values[1] = 
    	(int16)(pGpsExecContext->yPrevValue +
    		(int16)Read3Nibbles(pNibble));
    break;
    }

pGpsExecContext->yPrev2Value = pGpsExecContext->yPrevValue;
pGpsExecContext->yPrevValue = values[1];
}


int16 ReadByte(
    uint8 *pBuff)
{
int16 result;

result = (int16)(*(pBuff));

return result;
}


int16 ReadWord(
    uint8 *pBuff)
{
uint16 result;

result = (uint16)(*(pBuff++));
result = (uint16)((result << 8) + (uint16)(*(pBuff)));

return (int16)result;
}


int32 ReadLongSigned(
    uint8 *pBuff)
{
int32 result;

result = (int32)((int8)(*(pBuff++)));
result = (result << 8) + (*(pBuff++));
result = (result << 8) + (*(pBuff));

return result;
}

int32 ReadLongUnsigned(
    uint8 *pBuff)
{
int32 result;

result = (int32)((uint8)(*(pBuff++)));
result = (result << 8) + (*(pBuff++));
result = (result << 8) + (*(pBuff));

return result;
}

static void SkipExtraItems(
    uint8 **ppByte)
/*
 *  Skips the extra item data
 */
{
uint8 *pByte;
int16  nExtraItems;
int16  ii;
int16  extraItemSize;

pByte = *ppByte;
nExtraItems = NEXT_BYTE(pByte);
for (ii = 0; ii < nExtraItems; ii++)
    {
    extraItemSize = (int16)NEXT_BYTE(pByte);
	pByte += 1;
    pByte += extraItemSize;
    }
*ppByte = pByte;
}

static void SkipExtraStreamItems(
    InputStream *in)
/*
 *  Skips the extra item data
 */
{
int16  nExtraItems;
int16  ii;
int16  extraItemSize;

nExtraItems = ReadUnsignedByteMacro(in);
for (ii = 0; ii < nExtraItems; ii++)
    {
    extraItemSize = (int16)ReadUnsignedByteMacro(in);
    Seek_InputStream( in, Tell_InputStream( in ) + extraItemSize + 1 );
    }
}

#ifdef ENABLE_NATIVE_T1_HINTS
static void ProcessExtraStrokes(
	PFRClass *t,
	FFT1HintClass *ffhint,
    uint8 *pByte)
{
 
 int16 *pOrus;
 int ii, kk;
 int nStrokes;
 int  dimension;
 int16 *pGlyphcount;
 F16Dot16 tempx, tempy;



 pOrus = &(ffhint->extraXStrokeOrus_ptr[ffhint->numextraXStroke]);
 pGlyphcount = &(ffhint->extraXStrokeGlyphCount_ptr[ffhint->numextraXStroke]);

 for (dimension = 0;;)
 {
	 kk = 0;
	 nStrokes = NEXT_UBYTE(pByte);
	 if (pOrus != NULL)
	 {
		if (dimension == 0)
		{
			if ((ffhint->numextraXStroke + nStrokes) >= ffhint->extraXStrokeOrus_ml)
			{
				ffhint->extraXStrokeOrus_ml = (int16)(ffhint->extraXStrokeOrus_ml + WIDTH_CHUNK);
				ffhint->extraXStrokeOrus_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->extraXStrokeOrus_ptr , (ffhint->extraXStrokeOrus_ml) * sizeof(int16) );
				ffhint->extraXStrokeGlyphCount_ml = (int16)(ffhint->extraXStrokeGlyphCount_ml + WIDTH_CHUNK);
				ffhint->extraXStrokeGlyphCount_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->extraXStrokeGlyphCount_ptr , (ffhint->extraXStrokeGlyphCount_ml) * sizeof(int16) );
			}
		}
		else
		{
			if ((ffhint->numextraYStroke + nStrokes) >= ffhint->extraYStrokeOrus_ml)
			{
				ffhint->extraYStrokeOrus_ml = (int16)(ffhint->extraYStrokeOrus_ml + WIDTH_CHUNK);
				ffhint->extraYStrokeOrus_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->extraYStrokeOrus_ptr , (ffhint->extraYStrokeOrus_ml) * sizeof(int16) );
				ffhint->extraYStrokeGlyphCount_ml = (int16)(ffhint->extraYStrokeGlyphCount_ml + WIDTH_CHUNK);
				ffhint->extraYStrokeGlyphCount_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->extraYStrokeGlyphCount_ptr , (ffhint->extraYStrokeGlyphCount_ml) * sizeof(int16) );
			}
		}

		 for (ii = 0; ii < nStrokes; ii++)
		 {
			 pOrus[kk] = NEXT_SWORD(pByte);
			 if (dimension == 0)
			 {
				TransformPoint(t, ((F16Dot16)pOrus[kk] << 16), 1, &tempx, &tempy);
				pOrus[kk] = (int16)(tempx >> 16);
			 }
			 else
			 {
				TransformPoint(t, 1, ((F16Dot16)pOrus[kk] << 16), &tempx, &tempy);
				pOrus[kk] = (int16)(tempy >> 16);
			 }

			 pGlyphcount[kk] = t->glyph->pointCount;

			 pOrus[kk + 1] = NEXT_SWORD(pByte);
			 if (dimension == 0)
			 {
				TransformPoint(t, ((F16Dot16)pOrus[kk + 1] << 16), 1, &tempx, &tempy);
				pOrus[kk + 1] = (int16)(tempx >> 16);
			 }
			 else
			 {
				 TransformPoint(t, 1, ((F16Dot16)pOrus[kk + 1] << 16), &tempx, &tempy);
				 pOrus[kk + 1] = (int16)(tempy >> 16);
			 }

			 pGlyphcount[kk + 1] = t->glyph->pointCount;
			 kk += 2;
		 }
	 }
	 else
	 {
		pByte += nStrokes << 2;
	 }
	
	if (dimension == 0)
	{
	  ffhint->numextraXStroke = (int16)(ffhint->numextraXStroke + kk);
	  dimension = 1;
	  pOrus = &(ffhint->extraYStrokeOrus_ptr[ffhint->numextraYStroke]);
	  pGlyphcount = &(ffhint->extraYStrokeGlyphCount_ptr[ffhint->numextraYStroke]);
	  continue;
	}
 ffhint->numextraYStroke = (int16)(ffhint->numextraYStroke + kk);
 break;
 }
}


static void ProcessExtraEdges(
	PFRClass *t,
	FFT1HintClass *ffhint,
    uint8 *pByte)
{
 int ii;
 int kk;
 int jj = 0;
 int	nEdges;
 uint8	format;
 int16	*pThresh;
 int16	*pDeltaOrus;
 int16	*pIndex;
 int16	*pGlyphcount;
 int	dimension;
 F16Dot16  tempx, tempy;


 pThresh = &(ffhint->extraXEdgeThresh_ptr[ffhint->numextraXEdge]);
 pDeltaOrus = &(ffhint->extraXEdgeDelta_ptr[ffhint->numextraXEdge]);
 pIndex = &(ffhint->extraXEdgeIndex_ptr[ffhint->numextraXEdge]);
 pGlyphcount = &(ffhint->extraXEdgeGlyphCount_ptr[ffhint->numextraXEdge]);


 for (dimension = 0;;)
 {
	kk = 0;

	nEdges = NEXT_UBYTE(pByte);


	if (dimension == 0)
		{
			if ((ffhint->numextraXEdge + nEdges) >= ffhint->extraEdgeX_ml)
			{
				ffhint->extraEdgeX_ml = (int16)(ffhint->extraEdgeX_ml + WIDTH_CHUNK);
				ffhint->extraXEdgeThresh_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->extraXEdgeThresh_ptr , (ffhint->extraEdgeX_ml) * sizeof(int16) );
				ffhint->extraXEdgeDelta_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->extraXEdgeDelta_ptr, (ffhint->extraEdgeX_ml) * sizeof(int16) );
				ffhint->extraXEdgeIndex_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->extraXEdgeIndex_ptr , (ffhint->extraEdgeX_ml) * sizeof(int16) );
				ffhint->extraXEdgeGlyphCount_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->extraXEdgeGlyphCount_ptr , (ffhint->extraEdgeX_ml) * sizeof(int16) );
			}
		}
		else
		{
			if  ((ffhint->numextraYEdge + nEdges) >= ffhint->extraEdgeY_ml)
			{
				ffhint->extraEdgeY_ml = (int16)(ffhint->extraEdgeY_ml + WIDTH_CHUNK);
				ffhint->extraYEdgeThresh_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->extraYEdgeThresh_ptr , (ffhint->extraEdgeY_ml) * sizeof(int16) );
				ffhint->extraYEdgeDelta_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->extraYEdgeDelta_ptr, (ffhint->extraEdgeY_ml) * sizeof(int16) );
				ffhint->extraYEdgeIndex_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->extraYEdgeIndex_ptr , (ffhint->extraEdgeY_ml) * sizeof(int16) );
				ffhint->extraYEdgeGlyphCount_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->extraYEdgeGlyphCount_ptr , (ffhint->extraEdgeY_ml) * sizeof(int16) );
			}
		}
	
	for (ii = 0; ii < nEdges; ii++)
		{
		/* Read in the edges */
		format = NEXT_UBYTE(pByte);
			 
		if ((format & BIT_7) == 0)
		{
			kk += format >> 4;
			pThresh[ii] = 1 << 4;
			pDeltaOrus[ii] = (int16)(((int8)(format << 4)) >> 4);
		

		}
		else
		{
			kk = (format & 0x3f) - 1;
			if (kk < 0)
			{
				kk = NEXT_UBYTE(pByte);
			}
			if (format & BIT_6)
			{
				pThresh[ii] = 1 << 4;
			}
			else
			{
				pThresh[ii] = NEXT_UBYTE(pByte);
			}
			pDeltaOrus[ii] = NEXT_SBYTE(pByte);
			if (pDeltaOrus[ii] == 0)
			{
				pDeltaOrus[ii] = NEXT_SWORD(pByte);
			}
		}

		if (dimension == 0)
		{
			TransformPoint(t, ((F16Dot16)pDeltaOrus[ii] << 16), 1, &tempx, &tempy);
			pDeltaOrus[ii] = (int16)(tempx >> 16);
		}
		else
		{
			TransformPoint(t, 1, ((F16Dot16)pDeltaOrus[ii] << 16), &tempx, &tempy);
			pDeltaOrus[ii] = (int16)(tempy >> 16);
		}

		pIndex[ii] = (int16)kk;
		pGlyphcount[ii] = t->glyph->pointCount;
		jj += 1;
		
		}

	
	if (dimension == 0)
	{
	  ffhint->numextraXEdge = (int16)(ffhint->numextraXEdge + jj);
	  dimension = 1;
	  pThresh = &(ffhint->extraYEdgeThresh_ptr[ffhint->numextraYEdge]);
	  pDeltaOrus = &(ffhint->extraYEdgeDelta_ptr[ffhint->numextraYEdge]);
	  pIndex = &(ffhint->extraYEdgeIndex_ptr[ffhint->numextraYEdge]);
	  pGlyphcount = &(ffhint->extraYEdgeGlyphCount_ptr[ffhint->numextraYEdge]);
	  jj = 0;
	  continue;
	}

 ffhint->numextraYEdge = (int16)(ffhint->numextraYEdge + jj);
 break;
 }
}


static void ProcessExtraHints(
	PFRClass *t,
	FFT1HintClass *ffhint,
    uint8 **ppByte)
{
uint8 *pByte;
int16  nExtraItems;
int16  ii;
int16  extraItemSize;


pByte = *ppByte;
nExtraItems = NEXT_UBYTE(pByte);
for (ii = 0; ii < nExtraItems; ii++)
    {
		extraItemSize = (int16)NEXT_UBYTE(pByte);
		switch(NEXT_UBYTE(pByte))
		{
		case 1:
		ProcessExtraStrokes(t, ffhint, pByte);
		break;
		case 2:
		ProcessExtraEdges(t, ffhint, pByte);
		break;
		}
    pByte += extraItemSize;
    }
*ppByte = pByte;
}
#endif /* ENABLE_NATIVE_T1_HINTS */


#if 0

static void ShowAdjustmentVector(
	uint8 *pByte,
	int16 nBytes)
/*
 *	Show nBytes bytes of the adjustment vector at pByte - 1, pByte - 2, ...
 */
{
nibble_t nibble;
int16	val;
int16	ii;
uint8 *pLastByte;

printf("    Adjustment vector (nibbles):\n");
nibble.pByte = pByte - 1;
nibble.phase = 1;
pLastByte = pByte - nBytes;
while (nibble.pByte >= pLastByte)
	{
	val = ReadNibbleBack(&nibble);
	printf("%5d\n", (int)val);
	}
	
printf("\n    Adjustment vector (after nibble interpretation):\n");
nibble.pByte = pByte - 1;
nibble.phase = 1;
pLastByte = pByte - nBytes;
while (nibble.pByte >= pLastByte)
	{
	val = ReadNibbleBack(&nibble);
	switch (val)
		{
	case 0:
		printf("    0 ");
		break;

	case 1:
		val = ReadNibbleBack(&nibble);
		val += 3;
		for (ii = 0; ii < val; ii++)
			printf("    0 ");
		break;
		
	case 2:
	case 3:
	case 4:
		printf("%5d ", (int)(val - 5));
		break;
		
	case 5:
	case 6:
	case 7:
		printf("%5d ", (int)(val - 4));
		break;

	case 8:
		val = ReadNibbleBack(&nibble);
		printf("%5d ", (int)(val - 35));
		break;
		
	case 9:
		val = ReadNibbleBack(&nibble);
		printf("%5d ", (int)(val - 19));
		break;

	case 10:
		val = ReadNibbleBack(&nibble);
		printf("%5d ", (int)(val + 4));
		break;

	case 11:
		val = ReadNibbleBack(&nibble);
		printf("%5d ", (int)(val + 20));
		break;

	case 12:
		val = ReadNibbleBack(&nibble);
		val = (val << 4) + ReadNibbleBack(&nibble);
		printf("%5d ", (int)(val - 291));
		break;
		
	case 13:
		val = ReadNibbleBack(&nibble);
		val = (val << 4) + ReadNibbleBack(&nibble);
		printf("%5d ", (int)(val + 36));
		break;
		
	case 14:
		val = ReadNibbleBack(&nibble);
		val = (val << 4) + ReadNibbleBack(&nibble);
		val = (val << 4) + ReadNibbleBack(&nibble);
		val = (int16)(val << 4) >> 4;
		printf("%5d ", (int)(val));
		break;
		
	case 15:
		val = ReadNibbleBack(&nibble);
		val = (val << 4) + ReadNibbleBack(&nibble);
		val = (val << 4) + ReadNibbleBack(&nibble);
		val = (val << 4) + ReadNibbleBack(&nibble);
		printf("%5d ", (int)(val));
		break;
		}
	}
printf("\n");
}
#endif /* #if 0 */

#if 0

static uint8 ReadNibbleBack(
	nibble_t *pNibble)
{
uint8 nibble;

if (pNibble->phase == 1)
	{
	pNibble->phase = 0;
	nibble = *(pNibble->pByte) & 0x0f;
	}
else
	{
	pNibble->phase = 1;
	nibble = *(pNibble->pByte--) >> 4;
	}
return nibble;
}
#endif /* #if 0 */


static int8 Read2NibblesSigned(
	nibble_t *pNibble)
{
uint8	byte;

if (pNibble->phase == 0)
	{
	return (int8)*(pNibble->pByte++);
	}
else
	{
	byte = (uint8)((*pNibble->pByte & 0x0f) << 4);
	pNibble->pByte++;
	return (int8)((*(pNibble->pByte) >> 4) + byte);
	}
}

#if DEBUG
static void PrintBit(
    state_t *pState,
    char x)
{
if (pState->bitsLeft == pState->bitsPerRow)
    {
    printf("    ");
    }
printf("%c ", x);
if (--pState->bitsLeft == 0)
    {
    printf("\n");
    if (--pState->rowsLeft == 0)
        {
        return;
        }
    pState->bitsLeft = pState->bitsPerRow;
    }
}

static void ShowBitmapGps(
    PFRClass *t,
	uint16 gpsSize,
    int32  gpsOffset)
{
uint8  *pGps;
uint8  *pByte;
uint8   format;
uint8   byte;
int32   xPos, yPos;
int16   xSize, ySize;
int32   escapement;
uint8   mask;
int16   whiteCount, blackCount;
int16   ii;
state_t state;

pGps = (uint8 *)tsi_AllocMem(t->mem, gpsSize);
if (pGps == NULL)
    {
	printf("Insufficient memory (ShowBitmapGps)\n");
    return;
    }

Seek_InputStream( t->in, t->firstGpsOffset + gpsOffset );

ReadSegment( t->in, (uint8 *)pGps, gpsSize );

pByte = pGps;
format = NEXT_BYTE(pByte);      /* Read format byte */

/* Read X and Y positions */
switch (format & 3)
    {
case 0:
    byte = NEXT_BYTE(pByte);
    xPos = (int32)((int8)byte & 0xf0) << 12;
    yPos = (int32)((int8)((byte << 4) & 0xf0)) << 12;
    break;

case 1:
    xPos = (int32)((int8)NEXT_BYTE(pByte)) << 16;
    yPos = (int32)((int8)NEXT_BYTE(pByte)) << 16;
    break;

case 2:
    xPos = (int32)((int16)NEXT_WORD(pByte)) << 8;
    yPos = (int32)((int16)NEXT_WORD(pByte)) << 8;
    break;

case 3:
    xPos = NEXT_LONG(pByte) << 8;
    yPos = NEXT_LONG(pByte) << 8;
    break;
    }

/* Read X and Y dimensions */
switch ((format >> 2) & 3)
    {
case 1:
    byte = NEXT_BYTE(pByte);
    xSize = ((int16)byte) >> 4;
    ySize = (int16)byte & 0x0f;
    break;

case 2:
    xSize = (int16)NEXT_BYTE(pByte);
    ySize = (int16)NEXT_BYTE(pByte);
    break;

case 3:
    xSize = NEXT_WORD(pByte);
    ySize = NEXT_WORD(pByte);
    break;

default:
    xSize = ySize = 0;
    break;
    }

if (t->bmapFlags & BIT_1)          /* Rows are top to bottom order? */
    {
    printf("    Top left corner is at (%3.1f, %3.1f)\n", 
        (double)xPos / 65536.0, 
        (double)yPos / 65536.0);
    }
else                            /* Rows are bottom to top order? */
    {
    printf("    Bottom left corner is at (%3.1f, %3.1f)\n",
        (double)xPos / 65536.0, 
        (double)yPos / 65536.0);
    }

/* Read escapement */
switch((format >> 4) & 3)
    {
case 0:
    printf("    Escapement is unrounded linear width\n");
    break;

case 1:
    escapement = (int32)((int8)NEXT_BYTE(pByte)) << 8;
    printf("    Escapement is %d pixels\n", (escapement >> 8));
    break;

case 2:
    escapement = (int32)((int16)NEXT_WORD(pByte));
    printf("    Escapement is %3.1f pixels\n", (double)escapement / 256.0);
    break;

case 3:
    escapement = NEXT_LONG(pByte);
    printf("    Escapement is %3.1f pixels\n", (double)escapement / 256.0);
    break;
    }

switch((format >> 6) & 3)
    {
case 0:
    printf("    Bitmap image is directly encoded\n");
    break;

case 1:
    printf("    Bitmap image is run-length encoded in nibble mode\n");
    break;

case 2:
    printf("    Bitmap image is run-length encoded in byte mode\n");
    break;

case 3:
    printf("    Bitmap image has undefined encoding\n");
    break;
    }

state.bitsPerRow = xSize;
state.bitsLeft = xSize;
state.rowsLeft = ySize;
switch((format >> 6) & 3)
    {
case 0:
    mask = 0x80;
    while((state.rowsLeft > 0) && (state.bitsLeft > 0))
        {
        byte = *pByte;
        if (byte & mask)
            {
            PrintBit(&state, 'X');
            }
        else
            {
            PrintBit(&state, '.');
            }
        mask >>= 1;
        if (mask == 0)
            {
            pByte++;
            mask = 0x80;
            }
        }
    break;

case 1:
    while((state.rowsLeft > 0) && (state.bitsLeft > 0))
        {
        byte = *(pByte++);
        whiteCount = byte >> 4;
        blackCount = byte & 0x0f;
        for (ii = 0; ii < whiteCount; ii++)
            {
            PrintBit(&state, '.');
            }
        for (ii = 0; ii < blackCount; ii++)
            {
            PrintBit(&state, 'X');
            }
        }
    break;

case 2:
    state.bitsPerRow = xSize;
    state.bitsLeft = xSize;
    state.rowsLeft = ySize;
    while((state.rowsLeft > 0) && (state.bitsLeft > 0))
        {
        whiteCount = *(pByte++);
        for (ii = 0; ii < whiteCount; ii++)
            {
            PrintBit(&state, '.');
            }
        blackCount = *(pByte++);
        for (ii = 0; ii < blackCount; ii++)
            {
            PrintBit(&state, 'X');
            }
        }
    break;
    }

tsi_DeAllocMem (t->mem, pGps);
}
#endif	/* DEBUG */

#ifdef ENABLE_KERNING
static kernSubTable0Data *New_pfrkernSubTable0Data( tsiMemObject *mem, uint8 *in, int16 extraItemSize )
{
	int i;
	int16	baseAdjustment;
	uint16	leftCharCode, rightCharCode;
	int16	delta;
	uint8  	format;
	uint8   *begin_pt;
	uint8   *start_pt;
	uint16  pair_count = 0;
	uint16  pair_count_start = 0;
	int16	extraSize;
	int16	extraItemType = 4;


	kernSubTable0Data *t = (kernSubTable0Data *) tsi_AllocMem( mem, sizeof( kernSubTable0Data ) );


	start_pt = begin_pt = in;
	extraSize = extraItemSize;

	while (extraItemType == 4)
	{
	 pair_count = (uint16)(pair_count + (uint16)(*in));
	 in = begin_pt + extraSize;


	 extraSize = (int16)NEXT_BYTE(in);
	 extraItemType = (int16)NEXT_BYTE(in);
	 begin_pt = in;
	}

	in = start_pt;

	t->mem				= mem;

	t->pairs			= (kernPair0Struct*) tsi_AllocMem( mem, pair_count * sizeof(kernPair0Struct) );
	t->nPairs = pair_count;

	extraItemType = 4;
	extraSize = extraItemSize;

	while (extraItemType == 4)
	{

	pair_count			= (uint16)NEXT_BYTE(in);

	t->searchRange		= 0;
	t->entrySelector	= 0;
	t->rangeShift		= 0;
	
	baseAdjustment		= (int16)NEXT_WORD(in);
	format				= NEXT_BYTE(in);


	for ( i = pair_count_start; i < (pair_count_start + pair_count); i++ ) 
	{		
		if (format & BIT_0)
			{
			leftCharCode = (uint16)NEXT_WORD(in);
			rightCharCode = (uint16)NEXT_WORD(in);
			t->pairs[i].leftRightIndex = (uint32)((((uint32)leftCharCode << 16) | rightCharCode));
			}
		else
			{
			leftCharCode = (uint16)(*(in++));
			rightCharCode = (uint16)(*(in++));
			t->pairs[i].leftRightIndex = (uint32)((((uint32)leftCharCode << 16) | rightCharCode));
			}
		if (format & BIT_1)
			{
			delta = (int16)NEXT_WORD(in);
			t->pairs[i].value  = (int16)(baseAdjustment + delta);
			}
		else
			{
			delta = (int16)(*(in++));
			t->pairs[i].value  = (int16)(baseAdjustment + delta);
			}
	}
	pair_count_start = (uint16)(pair_count + pair_count);

	in = start_pt + extraSize;
	
	extraSize = (int16)NEXT_BYTE(in);
	extraItemType = (int16)NEXT_BYTE(in);
	start_pt = in;
	}
	
	return t; /*****/
}


static kernSubTable *New_pfrkernSubTable( tsiMemObject *mem, int appleFormat, uint8 *in , 
										 int16 extraItemSize)
{
	kernSubTable *t = (kernSubTable *) tsi_AllocMem( mem, sizeof( kernSubTable ) );

	UNUSED(appleFormat);
	t->mem			= mem;
	
	t->version		= (uint16)0;
	t->length		= (uint16)sizeof(kernSubTable);
	t->coverage		= (uint16)1;	
	
	t->kernData			= NULL;
	if ( t->version == 0 && t->length > 0 ) {
		t->kernData			= New_pfrkernSubTable0Data( mem, in , extraItemSize);
	}
	
	return t; /*****/
}

static kernClass *New_pfrkernClass( tsiMemObject *mem, uint8 *in, int16 extraItemSize )
{
	int appleFormat = 0;
	
	kernClass *t = (kernClass *) tsi_AllocMem( mem, sizeof( kernClass ) );
	t->mem			= mem;
	t->version		= 0;
	t->nTables		= 1;
	
	t->table		= (kernSubTable **) tsi_AllocMem( mem, sizeof( kernSubTable * ) );

	t->table[0] = New_pfrkernSubTable( mem, appleFormat, in , extraItemSize);
	
	return t; /*****/
}
#endif /* ENABLE_KERNING */


PFRClass *tsi_NewPFRClass( tsiMemObject *mem, InputStream *in, int32 fontNum )
{
	register PFRClass *t;
	uint8   versSizeBuff[10];
	uint8 isPFR;
	uint8   pfrHeader[PFR_HEADER_SIZE];
	long    physFontMaxSize;
	long    fontDirOffset;
	uint8	tmpBuff[5];
	uint16  nLogicalFonts;
#if 0
	uint16  fontDirSize;
	uint16  logicalFontSize;
#endif
	uint32  logicalFontOffset;
	int16   boldThickness = 0;
	uint8   format;
	uint8  *pByte;
	long    physicalFontSize;
	long    physicalFontOffset;
	uint8  *pPhysicalFont;
	int16   standardSetWidth = 0;
	uint8  *pExtraItems;
	int16   nBlueValues;
	uint16  nCharacters;
	int     ii, jj;
	uint8  *ptempByte;
	F16Dot16 xSize, ySize, aSize;
	F16Dot16 tempx, tempy;

	t = (PFRClass *)tsi_AllocMem( mem, sizeof( PFRClass ) );
	t->mem		= mem;
	t->fontNumber = (uint16)fontNum;
	t->rendering = false;
	t->pluggedIn = false;
	t->fontID = NULL;
	t->in = in;
	t->totalGpsSize = 0;
	t->advanceWidthMax = 0;
	/* read any pertinent head information */
	Seek_InputStream( in, 0 );
	ReadSegment( in, (uint8 *)&versSizeBuff[0], 10 );
	isPFR = (uint8)(((versSizeBuff[0] == 'P') && (versSizeBuff[1] == 'F') && (versSizeBuff[2] == 'R') ));
	if (isPFR)
		{
		t->headerSize = (uint16)ReadWord(versSizeBuff + PFRHEDSIZ);
		t->version = (uint16)ReadWord(versSizeBuff + PFRHEDFMT + 4);
		t->pfrType = (uint8)(versSizeBuff[3] - '0');
		Seek_InputStream( in, 0 );
		ReadSegment( in, (uint8 *)&pfrHeader[0], PFR_HEADER_SIZE );
		t->bmapFlags = (uint8)ReadByte(pfrHeader + PFRRSRVD1);
		physFontMaxSize = 
			    (long)((uint16)ReadWord(pfrHeader + PFRPFTSZM)) +
			    ((long)pfrHeader[PFRPFTSZX] << 16);
#if 0
		fontDirSize = (uint16)ReadWord(pfrHeader + PFRLFDSIZ);
#endif
		fontDirOffset = (long)((uint16)ReadWord(pfrHeader + PFRLFDOFF));
		t->firstGpsOffset = ReadLongUnsigned(pfrHeader + PFRGPSFOF);
		t->totalGpsSize = ReadLongUnsigned(pfrHeader + PFRGPSSZT);
		Seek_InputStream( in, (uint32)fontDirOffset );
		nLogicalFonts = (uint16)ReadInt16(in);
		Seek_InputStream( in, Tell_InputStream( in ) + fontNum * 5);
#if 1
		Seek_InputStream( in, Tell_InputStream( in ) + 2);
#else
		logicalFontSize = (uint16)ReadInt16(in);
#endif
		ReadSegment ( in, (uint8 *) tmpBuff, 3);
		logicalFontOffset = (uint32)ReadLongUnsigned(tmpBuff);
		t->directoryCount =(uint32) nLogicalFonts;
		Seek_InputStream( in, logicalFontOffset );
		ReadSegment ( in, (uint8 *) tmpBuff, 3);
		t->fontMatrix.m00 = ReadLongSigned(tmpBuff) << 8;
		ReadSegment ( in, (uint8 *) tmpBuff, 3);
		t->fontMatrix.m01 = ReadLongSigned(tmpBuff) << 8;
		ReadSegment ( in, (uint8 *) tmpBuff, 3);
		t->fontMatrix.m10 = ReadLongSigned(tmpBuff) << 8;
		ReadSegment ( in, (uint8 *) tmpBuff, 3);
		t->fontMatrix.m11 = ReadLongSigned(tmpBuff) << 8;
		if (t->fontMatrix.m11 < 0)
			t->fontMatrix.m11 = -t->fontMatrix.m11;
		t->fontMatrix.xOffset = 0;
		t->fontMatrix.yOffset = 0;

#if DEBUG
	    if (debugOn)
		    printf("    Font matrix = [%3.1f %3.1f %3.1f %3.1f]\n",
		        (double)t->fontMatrix.m00 / 256.0,
		        (double)t->fontMatrix.m01 / 256.0,
		        (double)t->fontMatrix.m10 / 256.0,
		        (double)t->fontMatrix.m11 / 256.0);
#endif
		xSize = MAX(ABS(t->fontMatrix.m00), ABS(t->fontMatrix.m01));
		ySize = MAX(ABS(t->fontMatrix.m10), ABS(t->fontMatrix.m11));
		aSize = (xSize < ySize) ? xSize : ySize;
		t->fontMatrix.m00 = util_FixDiv(t->fontMatrix.m00, aSize); /* to unit */
		t->fontMatrix.m01 = util_FixDiv(t->fontMatrix.m01, aSize); /* to unit */
		t->fontMatrix.m10 = util_FixDiv(t->fontMatrix.m10, aSize); /* to unit */
		t->fontMatrix.m11 = util_FixDiv(t->fontMatrix.m11, aSize ); /* to unit */

	    /* pByte = pLogicalFont + PFRLFTSFX; */
	    Seek_InputStream(in, logicalFontOffset + (uint32)PFRLFTSFX);
	    format = ReadUnsignedByteMacro(in);
	    if (format & BIT_2)         /* Stroked style? */
	        {
#if DEBUG
	        if (debugOn) printf("    Stroked style\n");
#endif
	        t->strokeThickness = (int16)((format & BIT_3)?
	            ReadInt16(in):
	            ReadUnsignedByteMacro(in));
#if DEBUG
 	        if (debugOn) printf("    Stroke thickness = %d\n", (int)t->strokeThickness);
#endif
	        }
	    else if (format & BIT_4)    /* Bold style? */
	        {
#if DEBUG
	        if (debugOn) printf("    Bold style\n");
#endif
	        boldThickness = (int16)((format & BIT_5)?
	            ReadInt16(in):
	            ReadUnsignedByteMacro(in));
#if DEBUG
	        if (debugOn) printf("    Bold thickness = %d\n", (int)boldThickness);
#endif
	        }

	    t->boldThickness = boldThickness;
	    t->pluggedIn = false;
	    t->rendering = false;
	    /* Skip extra items if present */
	    if (format & BIT_6)
	        {
	        SkipExtraStreamItems(in);
	        }

	    physicalFontSize = (long)((uint16)ReadInt16(in));
		ReadSegment ( in, (uint8 *) tmpBuff, 3);
		physicalFontOffset = ReadLongSigned(tmpBuff);
	    if (physFontMaxSize >= 65536L)
	        physicalFontSize += (long)((uint8)ReadUnsignedByteMacro(in)) << 16;

		pPhysicalFont = (uint8 *)tsi_AllocMem(t->mem, (size_t)physicalFontSize);
		if (pPhysicalFont == NULL)
		    {
#if DEBUG
			if (debugOn) printf("Insufficient memory (tsi_NewPFRClass)\n");
#endif
		    return t = NULL;
		    }
		Seek_InputStream( in, (uint32)physicalFontOffset );
		ReadSegment( in, (uint8 *)pPhysicalFont, physicalFontSize );

		pByte = pPhysicalFont;
		t->physFontNumber = (uint16)NEXT_WORD(pByte);
		t->outlineRes = (uint16)NEXT_WORD(pByte);
		t->metricsRes = (uint16)NEXT_WORD(pByte);
#if DEBUG
		if (debugOn) printf("    Physical font number = %d\n", (int)t->physFontNumber);
		if (debugOn) printf("    Outline resolution = %d\n", (int)t->outlineRes);
		if (debugOn) printf("    Metrics resolution = %d\n", (int)t->metricsRes);
#endif

		t->shortScaleFactors = (unsigned char)(((t->pfrType > 1) && (t->outlineRes < 256)));
		t->xyScale = util_FixDiv( (F16Dot16)t->metricsRes << 16, (F16Dot16)t->outlineRes << 16 );

		t->outputMatrix.m00 = t->xyScale;
		t->outputMatrix.m01 = 0;
		t->outputMatrix.m10 = 0;
		t->outputMatrix.m11 = t->xyScale;
		t->outputMatrix.xOffset = 0;
		t->outputMatrix.yOffset = 0;

		/* got fontMatrix and outputMatrix: setup the transformation control block */
		SetupTCB(t);

		/* set t->upem to metrics res: */

		t->upem = t->metricsRes;

		
		/* Show font bounding box */
		t->xmin =  (int16)NEXT_WORD(pByte);
		t->ymin =  (int16)NEXT_WORD(pByte);
		t->xmax =  (int16)NEXT_WORD(pByte);
		t->ymax =  (int16)NEXT_WORD(pByte);
#if DEBUG
		if (debugOn) printf("    Font bounding box = [%d %d %d %d]\n",
    			(int)t->xmin, (int)t->ymin, (int)t->xmax, (int)t->ymax);
#endif

		/* Show font properties */
		format = NEXT_BYTE(pByte);
		t->verticalEscapement = (int8)(((format & BIT_0) != 0));
#if DEBUG
		if (format & BIT_0)
		    {
		    if (debugOn) printf("    Vertical Escapement direction\n");
		    }
		else
		    {
		    if (debugOn) printf("    Horizontal Escapement direction\n");
		    }
#endif

		if ((format & BIT_2) == 0)
		    {
		    standardSetWidth = (int16)(NEXT_WORD(pByte));
#if DEBUG
			if (debugOn) printf("    Standard set width = %d\n", (int)standardSetWidth);
#endif
		    }
#if DEBUG
		if (format & BIT_4)
		    {
		    if (debugOn) printf("    2-byte gps size field\n");
		    }
		if (format & BIT_5)
		    {
		    if (debugOn) printf("    3-byte gps offset field\n");
		    }
#endif
		/* Skip extra data items if present */
		t->kern = NULL;
		t->nBmapStrikes = 0;
		if (format & BIT_7)
		    {
			int nExtraItems;
			int16 extraItemSize, extraItemType; 
#if DEBUG
			uint8  *pByteOrg;
#endif
			int		nSuccessfulSizes = 0;
			nExtraItems = NEXT_BYTE(pByte);
			for (ii = 0; ii < nExtraItems; ii++)
				{
				extraItemSize = (int16)NEXT_BYTE(pByte);
				extraItemType = (int16)NEXT_BYTE(pByte);
#if DEBUG
				pByteOrg = pExtraItems = ptempByte = pByte;
#else
				pExtraItems = ptempByte = pByte;
#endif
				if (extraItemType == 2)
					{/* unique fontID */
					size_t len;
					len = strlen((const char *)pExtraItems);
					t->fontID = (uint8 *)tsi_AllocMem( t->mem, (unsigned long)(len + 1) );
					if (t->fontID)
						{
						strcpy((char *)t->fontID, (const char *)pExtraItems);
#if DEBUG
						if (debugOn)
							{
							printf("        Font ID string = %s", t->fontID);
							printf("\"\n");
							}
#endif
						}
					}


				else if (extraItemType == 3)
					{

					uint8  format;
					int16  ii;

						/* Get the StemSnap arrays! */
						format = NEXT_BYTE(pExtraItems);
						t->nStemSnapV = ((format >> 4) & 0x0f);
						t->nStemSnapH = (format & 0x0f);
						if (t->nStemSnapV > 0)
						{
						/*	t->StemSnapV = (int16 *)tsi_AllocMem( t->mem, sizeof(int16)*t->nStemSnapsV ); */
			
							for (ii = 0; ii < t->nStemSnapV; ii++)
							{
							t->StemSnapV[ii] = NEXT_WORD(pExtraItems);
							TransformPoint(t, ((F16Dot16)t->StemSnapV[ii] << 16), 1, &tempx, &tempy);
							t->StemSnapV[ii] = (tempx >> 16);
							}
					
						}
						if (t->nStemSnapH > 0)
						{
						/*	t->StemSnapH = (int16 *)tsi_AllocMem( t->mem, sizeof(int16)*t->nStemSnapsH ); */
							
							for (ii = 0; ii < t->nStemSnapH; ii++)
							{
							t->StemSnapH[ii] = NEXT_WORD(pExtraItems);
							TransformPoint(t, 1, ((F16Dot16)t->StemSnapH[ii] << 16), &tempx, &tempy);
							t->StemSnapH[ii] = (tempy >> 16);
							}
						}
					}
#ifdef ENABLE_KERNING
				else if (extraItemType == 4)
					{/* kerning data */
					if (t->kern == NULL)
						t->kern = New_pfrkernClass( t->mem , ptempByte , extraItemSize);
					}
#endif /* ENABLE_KERNING */
#ifdef ENABLE_SBIT
				else if (extraItemType == 1)
					{/* bitmap data */
#if DEBUG
					int32   fontBctSize;
#endif
					uint8   sizeFormat;
					int16   nBmapSizes;
#if DEBUG
					int32   totalSize = 0;
#endif
					uint16  xppm, yppm;
					uint8   charFormat;
					int32   bctSize;
					int32   bctOffset;
					int32   nBmapChars;
					uint8	*pBct;
					uint8	*pBmapChar;
					long	firstBctOffset = physicalFontOffset + physicalFontSize;
					uint16  charCode;
					uint16  gpsSize;
					int32   gpsOffset;
					int		kk;
					
#if DEBUG
					fontBctSize = (int32)NEXT_LONG(pExtraItems);
#else
					pExtraItems += 3;
#endif
					sizeFormat = NEXT_BYTE(pExtraItems);
					nBmapSizes = (int16)NEXT_BYTE(pExtraItems);
					t->bmapStrikes = (bmapStrike_t	*)tsi_AllocMem(t->mem,
										nBmapSizes * sizeof(bmapStrike_t));
					if (t->bmapStrikes)
						{
						for (jj = 0; jj < nBmapSizes; jj++)
						    {
	    					xppm = (uint16)((sizeFormat & BIT_0)?
	        					(uint16)NEXT_WORD(pExtraItems):
	        					(uint16)NEXT_BYTE(pExtraItems));
	    					yppm = (uint16)((sizeFormat & BIT_1)?
	        					(uint16)NEXT_WORD(pExtraItems):
	        					(uint16)NEXT_BYTE(pExtraItems));
	    					charFormat = NEXT_BYTE(pExtraItems);
	    					bctSize = (sizeFormat & BIT_2)?
	        					(int32)NEXT_LONG(pExtraItems):
	        					(int32)((uint16)NEXT_WORD(pExtraItems));
	    					bctOffset = (sizeFormat & BIT_3)?
	        					(int32)NEXT_LONG(pExtraItems):
	        					(int32)((uint16)NEXT_WORD(pExtraItems));
	    					nBmapChars = (sizeFormat & BIT_4)?
	        					(int32)((uint16)NEXT_WORD(pExtraItems)):
	        					(int32)NEXT_BYTE(pExtraItems);
#if DEBUG
							if (debugOn)
	    						printf("        %d bitmaps for %ld x %ld pix per em at offset %ld (%ld bytes)\n",
		        					(int)nBmapChars,
		        					(long)xppm,
		        					(long)yppm,
		        					(long)bctOffset,
		        					(long)bctSize);
#endif	        					
	        				t->bmapStrikes[nSuccessfulSizes].xppm = xppm;
	        				t->bmapStrikes[nSuccessfulSizes].yppm = yppm;
	        				t->bmapStrikes[nSuccessfulSizes].nBmapChars = nBmapChars;
	        				t->bmapStrikes[nSuccessfulSizes].bmapDir = (bmapCharDir_t	*)tsi_AllocMem(t->mem,
	        										nBmapChars * sizeof(bmapCharDir_t));
	        				
#if DEBUG
	    					totalSize += bctSize;
#endif
				            /* Allocate and load bitmap character table */
							pBct = (uint8 *)tsi_AllocMem( t->mem, (size_t)bctSize);
							if (pBct == NULL)
						       {
#if DEBUG
								if (debugOn) printf("Insufficient memory (ShowBitmapChar)\n");
#endif
							    return t = NULL;
								}
							if (pBct)
								{
								Seek_InputStream( in, (uint32)(firstBctOffset + bctOffset) );
								ReadSegment( in, (uint8 *)pBct, bctSize );
			
								pBmapChar = pBct;
					            for (kk = 0; kk < nBmapChars; kk++)
							        {
									charCode = (uint16)((charFormat & BIT_0)?
										(uint16)NEXT_WORD(pBmapChar):
										(uint16)NEXT_BYTE(pBmapChar));
									gpsSize = (uint16)((charFormat & BIT_1)?
										(uint16)NEXT_WORD(pBmapChar):
										(uint16)NEXT_BYTE(pBmapChar));
			                		gpsOffset = (int32)((charFormat & BIT_2)?
			                    		(int32)NEXT_LONG(pBmapChar):
			                    		(int32)NEXT_WORD(pBmapChar));
#if DEBUG
			                   		if (debugOn)
										{
										printf("\nBitmap image for %d x %d pixels per em\n",
			                        		(int)xppm, (int)yppm);
			                   			ShowBitmapGps(t, gpsSize, gpsOffset);
										}
#endif
									/* BOB: must remember to correct charIndex fields later */
									t->bmapStrikes[nSuccessfulSizes].bmapDir[kk].charIndex = 0;
									t->bmapStrikes[nSuccessfulSizes].bmapDir[kk].charCode = charCode;
									t->bmapStrikes[nSuccessfulSizes].bmapDir[kk].gpsSize = gpsSize;
									t->bmapStrikes[nSuccessfulSizes].bmapDir[kk].gpsOffset = (uint32)gpsOffset;
			                		}
								tsi_DeAllocMem( t->mem, (void *)pBct );
								nSuccessfulSizes++;
								}
#if DEBUG
							else
								{
								if (debugOn) printf("Insufficient memory for pBct: skipping strike.\n");
								tsi_DeAllocMem(t->mem, t->bmapStrikes[nSuccessfulSizes].bmapDir);
								}
#endif
	    					}
	
#if DEBUG
						if (debugOn)
							{
							if (totalSize != fontBctSize)
		    					{
		    					printf("*** Bitmap character table size mismatch:\n"); 
		    					printf("        Specified total = %ld\n", (long)fontBctSize); 
		    					printf("        Actual total = %ld\n", (long)totalSize); 
		    					}
							/* Check size allocated versus size used */
							if ((pExtraItems - pByteOrg) != extraItemSize)
		    					{
		    					printf("*** Extra item size mismatch: %d allocated, %d used\n",
		        					(int)extraItemSize,
		        					(int)(pExtraItems - pByteOrg));
		    					}
	    					}
#endif	
						}
#if DEBUG
					else
						{
						if (debugOn) printf("Insufficient memory for t->bmapStrikes\n");
						}
#endif
					t->nBmapStrikes = (uint16)nSuccessfulSizes;
					}
#endif /* ENABLE_SBIT */
					pByte += extraItemSize;
				}
		    }

		/* Get auxiliary data size and copy contents */
		t->nAuxBytes = (long)NEXT_LONG(pByte);
		if (t->nAuxBytes)
			{
			t->pAuxData		= (uint8 *)tsi_AllocMem( t->mem, sizeof(uint8) * t->nAuxBytes );
			/* copy the auxdata into the buffer: */
			memcpy((void *)t->pAuxData, (void *)pByte, (size_t)t->nAuxBytes);
			}
		else
			t->pAuxData = NULL;
		
		/* skip over already copied auxdata: */
		pByte += t->nAuxBytes;

		/* Get blue values */
		t->numBlueValues = nBlueValues = NEXT_BYTE(pByte);

#if DEBUG
		if (debugOn) printf("    Blue values = [");
#endif
		for (ii = 0; ii < nBlueValues; ii++)
		    {
			t->BlueValues[ii] = NEXT_WORD(pByte);

			TransformPoint(t, 1, ((F16Dot16)t->BlueValues[ii] << 16), &tempx, &tempy);
			t->BlueValues[ii] = (tempy >> 16);


#if DEBUG
		    if (debugOn) printf("%d ", (int)t->BlueValues[ii]);
#endif
		    }
#if DEBUG
		if (debugOn) printf("]\n");
#endif

		/* Get blue fuzz value */
		t->BlueFuzz = NEXT_BYTE(pByte);
		TransformPoint(t, 1, ((F16Dot16)t->BlueFuzz<< 16), &tempx, &tempy);
		t->BlueFuzz = (tempy >> 16);
#if DEBUG
		if (debugOn) printf("    Blue fuzz = %d orus\n", (int)t->BlueFuzz);
#endif
		/* Get blue scale value */
		t->BlueScale = NEXT_BYTE(pByte);
		TransformPoint(t, 1, ((F16Dot16)t->BlueScale<< 16), &tempx, &tempy);
		t->BlueScale = (tempy >> 16);
#if DEBUG
		if (debugOn) printf("    Blue scale = %d pixels per em\n", (int)t->BlueScale);
#endif

		/* Set default value for blue shift */
		t->BlueShift = (
    (((int32)BLUE_SHIFT * t->outlineRes) + 
			0x8000L) >>
				16);

		/* Get standard vertical stroke weight */
		t->StdVW = NEXT_WORD(pByte);
		TransformPoint(t, ((F16Dot16)t->StdVW << 16), 1, &tempx, &tempy);
		t->StdVW = (tempx >> 16);

#if DEBUG
		if (debugOn) printf("    Standard vertical stroke weight = %d\n", (int)t->StdVW);
#endif

		/* Get standard horizontal stroke weight */
		t->StdHW  = NEXT_WORD(pByte);
		TransformPoint(t, 1,((F16Dot16)t->StdHW << 16), &tempx, &tempy);
		t->StdHW = (tempy >> 16);
#if DEBUG
		if (debugOn) printf("    Standard horizontal stroke weight = %d\n", (int)t->StdHW);
#endif

		/* Show character set */
		nCharacters = (uint16)NEXT_WORD(pByte);
		t->NumCharStrings = (short)nCharacters;
#if DEBUG
		if (debugOn) printf("    Number of characters available = %d\n", (int)nCharacters);
#endif
		/*** Done with the header ***/
			
		BuildPFRCMAP( t, (uint8 **)&pByte, format, standardSetWidth);
		for (ii = 0; ii < t->nBmapStrikes; ii++)
			{/* translate all charCodes in bitmap strike directories to charIndexes: */
			for (jj = 0; jj < t->bmapStrikes[ii].nBmapChars; jj++)
				{
				t->bmapStrikes[ii].bmapDir[jj].charIndex = 
					tsi_PFRGetGlyphIndex( t, t->bmapStrikes[ii].bmapDir[jj].charCode );
				}
			}
#ifdef ENABLE_KERNING
		{
		kernClass *p;
		kernSubTable *kSubTblPtr;
		void *kernData;
		kernSubTable0Data *q;
		uint16 leftCharCode, rightCharCode;
		uint16 leftCharIndex, rightCharIndex;

			if (t->kern)
			{
				/* translate all charCodes in pfrKern array to glyph indices, then sort: */
				p = t->kern;
				kSubTblPtr = p->table[0];
				kernData = kSubTblPtr->kernData;
				q = (kernSubTable0Data *)kernData;
				for (ii = 0; ii < q->nPairs; ii++)
				{
					leftCharCode = (uint16)(q->pairs[ii].leftRightIndex >> 16);
					rightCharCode = (uint16)(q->pairs[ii].leftRightIndex & 0xffff);
					leftCharIndex = 
						tsi_PFRGetGlyphIndex( t, leftCharCode );
					rightCharIndex = 
						tsi_PFRGetGlyphIndex( t, rightCharCode );
					q->pairs[ii].leftRightIndex = (uint32)((((uint32)leftCharIndex << 16) | rightCharIndex));
				}
				ff_KernShellSort(q->pairs, q->nPairs);
			}
		}
#endif

		BuildPFRMetricsEtc( t );
		tsi_DeAllocMem( t->mem, (char *)t->pAuxData );	/* don't need anymore */
		
		tsi_DeAllocMem( t->mem, (void *)pPhysicalFont );
		}
	t->rendering = true;	/* from now on, when tsi_PFRGetGlyphByIndex() is called, we're rendering */
	return t; /*****/
}

void tsi_DeletePFRClass( PFRClass *t )
{
int ii;
	if ( t != NULL ) {
		tsi_DeAllocMem( t->mem, (char *)t->charMap );
		for (ii = t->nBmapStrikes; ii > 0; ii--)
			tsi_DeAllocMem( t->mem, (char *)t->bmapStrikes[ii-1].bmapDir);
		if (t->nBmapStrikes && t->bmapStrikes)
			tsi_DeAllocMem( t->mem, t->bmapStrikes);
		tsi_DeAllocMem( t->mem, (char *)t->fontID);
		tsi_DeAllocMem( t->mem, (char *)t );
	}
}

static void BuildPFRCMAP( PFRClass *t , uint8 **pPhysicalFont, uint8 format, int16 standardSetWidth)
{
register int ii;
uint8  *pByte = *pPhysicalFont;
#if DEBUG
uint16  asciiCode;
#endif
uint16  defaultCharCode;
int16   defaultSetWidth;
long    defaultGpsOffset;

	t->charMap		= (physCharMap *)tsi_AllocMem( t->mem, sizeof(physCharMap) * t->NumCharStrings );
	/* Initialize */
	for ( ii = 0; ii < t->NumCharStrings; ii++ )
		{
		t->charMap[ii].charCode = 0xffff;
		t->charMap[ii].gpsSize = 0x0;
		t->charMap[ii].gpsOffset = 0x0;
		}
	t->firstCharCode = 0xffff;
	t->lastCharCode = 0x0000;
	t->hmtx = New_hmtxEmptyClass( t->mem, t->NumCharStrings, t->NumCharStrings );
	t->noDelete_hmtx = t->hmtx; /* Initialize our no delete reference */
	defaultCharCode = 0;
	defaultSetWidth = 0;
	defaultGpsOffset = 0L;
    for (ii = 0; ii < t->NumCharStrings; ii++)
        {
	    if (t->pfrType > 0)
	    	{
	        uint8	charFormat;
	        
	        /* Character record format byte */
	        charFormat = NEXT_BYTE(pByte);
	        
	        /* Character code */
	        switch(charFormat & 0x03)
	        	{
	        case 0:
	        	t->charMap[ii].charCode = defaultCharCode;
	        	break;
	        	
	        case 1:
	        	t->charMap[ii].charCode = (uint16)(defaultCharCode + (uint16)NEXT_BYTE(pByte));
	        	break;
	        	
	        case 2:
	        	t->charMap[ii].charCode = (uint16)(defaultCharCode + (uint16)NEXT_WORD(pByte));
	        	break;
	        	}
	        defaultCharCode = (uint16)(t->charMap[ii].charCode + 1);
	        
	        /* Character escapement */
	        switch((charFormat >> 2) & 0x03)
	        	{
	        case 0:
				t->hmtx->aw[ii]  = (uint16)defaultSetWidth;
	        	break;
	        	
	        case 1:
				t->hmtx->aw[ii]  = (uint16)(defaultSetWidth + (int16)NEXT_BYTE(pByte));
	        	break;
	        	
	        case 2:
				t->hmtx->aw[ii]  = (uint16)(defaultSetWidth - (int16)NEXT_BYTE(pByte));
	        	break;
	        	
	        case 3:
				t->hmtx->aw[ii]  = (uint16)(NEXT_WORD(pByte));
	        	break;
	        	}
	        defaultSetWidth = (int16)t->hmtx->aw[ii];
	        if (defaultSetWidth > t->advanceWidthMax)
	        	t->advanceWidthMax = defaultSetWidth;

#if DEBUG
			/* Ascii code */
			asciiCode =
	            (uint16)(((format & BIT_6)? t->charMap[ii].charCode: 0));
#endif
	            
	        /* Glyph prog string size */
	        switch((charFormat >> 4) & 0x03)
	        	{
	        case 0:
	        	t->charMap[ii].gpsSize = (uint16)NEXT_BYTE(pByte);
	        	break;
	        	
	        case 1:
	        	t->charMap[ii].gpsSize = (uint16)(NEXT_BYTE(pByte) + 256);
	        	break;
	        	
	        case 2:
	        	t->charMap[ii].gpsSize = (uint16)(NEXT_BYTE(pByte) + 512);
	        	break;
	        	
	        case 3:
	        	t->charMap[ii].gpsSize = (uint16)NEXT_WORD(pByte);
	        	break;
	        	}

	        /* Glyph prog string offset */
	        switch((charFormat >> 6) & 0x03)
	        	{
	        case 0:
	        	t->charMap[ii].gpsOffset = defaultGpsOffset;
	        	break;
	        	
	        case 1:
	        	t->charMap[ii].gpsOffset = defaultGpsOffset + (int32)NEXT_BYTE(pByte);
	        	break;
	        	
	        case 2:
	        	t->charMap[ii].gpsOffset = (int32)NEXT_WORD(pByte);
	        	break;
	        	
	        case 3:
	        	t->charMap[ii].gpsOffset = (int32)NEXT_LONG(pByte);
	        	break;
	        	}
	        defaultGpsOffset = t->charMap[ii].gpsOffset + t->charMap[ii].gpsSize;
	        }
	    else
	        {
	        t->charMap[ii].charCode = (uint16)((format & BIT_1)? 
					NEXT_WORD(pByte): 
					NEXT_BYTE(pByte));
			t->hmtx->aw[ii]  = (uint16)((format & BIT_2)? 
									            NEXT_WORD(pByte): 
									            standardSetWidth);
#if DEBUG
	        asciiCode = (uint16)((format & BIT_3)? 
					NEXT_BYTE(pByte): 
					((format & BIT_6)? t->charMap[ii].charCode: 0));
#else
			if (format & BIT_3)
				pByte++;
#endif
	        t->charMap[ii].gpsSize = (uint16)((format & BIT_4)? 
					NEXT_WORD(pByte): 
					NEXT_BYTE(pByte));
	        t->charMap[ii].gpsOffset = (format & BIT_5)? 
	            (long)NEXT_LONG(pByte): 
	            (long)((uint16)NEXT_WORD(pByte));
	        }
#if DEBUG
        if (debugOn) printf("   %5ld  %5d    %5d  %6ld  \n",
            (long)t->charMap[ii].charCode, (int)t->hmtx->aw[ii], (int)t->charMap[ii].gpsSize, (long)t->charMap[ii].gpsOffset);
#endif
		if (t->charMap[ii].charCode < t->firstCharCode)
			t->firstCharCode = t->charMap[ii].charCode;
		if (t->charMap[ii].charCode > t->lastCharCode)
			t->lastCharCode = t->charMap[ii].charCode;
	    }
*pPhysicalFont = pByte;
}

static void BuildPFRMetricsEtc( PFRClass *t )
{
register uint16 gIndex;
	GlyphClass *glyph;
	short ascent, descent, lineGap, maxSW;
	uint16 aw, ah;
	int err;
	
	t->maxPointCount = 0;
	t->ascent = 0x7fff;
	t->descent = -0x7fff;
	t->lineGap = 0x7fff;
	
	err = GetAuxDataMetrics(
							t,
						    t->pAuxData,
						    (uint32)t->nAuxBytes,
						    IDWINTYPEATT,
						    (short *)&ascent,
						    (short *)&descent,
						    (short *)&lineGap,
						    (short *)&maxSW,
						    &t->isFixedPitch);
	if (err)
		{/* try IDPFMTYPEATTR: */
		err = GetAuxDataMetrics(
								t,
							    t->pAuxData,
							    (uint32)t->nAuxBytes,
							    IDPFMTYPEATT,
							    (short *)&ascent,
							    (short *)&descent,
							    (short *)&lineGap,
							    (short *)&maxSW,
							    &t->isFixedPitch);
		}
	if (!err)
		{
		t->ascent = ascent;
		t->descent = -descent;
		t->lineGap = lineGap;
		t->advanceWidthMax = maxSW;
		}
	t->italicAngle = t->fontMatrix.m10; /* get from font matrix (t->m01)  */
	
	for ( gIndex = 0; gIndex < t->NumCharStrings; gIndex++ ) {
		t->hmtx->lsb[gIndex] = 0;
	}
	if ((t->ascent == 0x7fff) && ((gIndex = tsi_PFRGetGlyphIndex( t, 'f')) > 0) )
	{
		glyph = tsi_PFRGetGlyphByIndex( t, gIndex, &aw, &ah, NULL);
		if (glyph)
		{
			t->ascent = GetGlyphYMax( glyph );
			Delete_GlyphClass( glyph );
		}
	}
	if ((t->descent == -0x7fff) && ((gIndex = tsi_PFRGetGlyphIndex( t, 'g')) > 0) )
	{
		glyph = tsi_PFRGetGlyphByIndex( t, gIndex, &aw, &ah, NULL);
		if (glyph)
		{
			t->descent = GetGlyphYMin( glyph );
			Delete_GlyphClass( glyph );
		}
	}
	t->maxPointCount = 32;	/* until we learn better */

	if ( t->ascent == 0x7fff )  t->ascent  =  t->upem * 3 / 4;
	if ( t->descent == -0x7fff ) t->descent = -(t->upem / 4);
	if ( t->lineGap == 0x7fff ) t->lineGap = t->upem - (t->ascent - t->descent);
	if ( t->lineGap < 0 ) t->lineGap = 0;
	
}


static short GetGlyphYMax( GlyphClass *glyph )
{
	register int i, limit = glyph->pointCount;
	register short *ooy = glyph->ooy;
	register short ymax = ooy[0];
	
	for ( i = 1; i < limit; i++ ) {
		if ( ooy[i] > ymax ) {
			ymax = ooy[i];
		}
	}
	return ymax; /*****/
}

static short GetGlyphYMin( GlyphClass *glyph )
{
	register int i, limit = glyph->pointCount;
	register short *ooy = glyph->ooy;
	register short ymin = ooy[0];
	
	for ( i = 1; i < limit; i++ ) {
		if ( ooy[i] < ymin ) {
			ymin = ooy[i];
		}
	}
	return ymin; /*****/
}


uint16 tsi_PFRGetGlyphIndex( PFRClass *t, uint16 charCode)
{
uint16 gIndex = 0;
uint8 found;	
	found = BSearchCTbl (t, &gIndex, charCode);
	t->glyphExists = (int)found;
	return gIndex; /*****/
}

GlyphClass *tsi_PFRGetGlyphByIndex( PFRClass *t, uint16 index, uint16 *aWidth, 
								   uint16 *aHeight, FFT1HintClass *ffhint )
{
	int byteCount;
	uint8 *p;
	GlyphClass *glyph;
	uint32 offset;
	short upper;
	
	upper = t->NumCharStrings;
	if (t->boldThickness && t->rendering && !t->pluggedIn)
		{
#ifdef ALGORITHMIC_STYLES
		/* This works if SetStyling is made public,
			but is unsupported in this architecture at this time
			rje 05-14-99

			sfntClass *sfc = (sfntClass *)t->sfntClassPtr;
			T2K_AlgStyleDescriptor	style;
			style.StyleFunc			= 	tsi_SHAPET_BOLD_GLYPH;
			style.StyleMetricsFunc	=	tsi_SHAPET_BOLD_METRICS;
			style.params[0] = util_FixDiv((uint32)t->boldThickness << 16,(uint32)t->outlineRes << 16);
			SetStyling( sfc, &style );
			style.StyleMetricsFunc( sfc->hmtx, sfc->mem, (short)GetUPEM( sfc ), sfc->params );
		*/
#endif			
			t->pluggedIn = true;
		}
	t->glyph = New_EmptyGlyph( t->mem, 0, 0, 0, 0 );
	t->glyph->curveType = 3;

	if ( index < upper )
		{
		byteCount = t->charMap[index].gpsSize;
		offset = (uint32)(t->firstGpsOffset + t->charMap[index].gpsOffset);
		Seek_InputStream( t->in, offset );
		p = (uint8 *)tsi_AllocMem(t->mem, (size_t)byteCount);
		if ( p != NULL )
			{
			ReadSegment( t->in, p, byteCount );
			LOG_CMD( "tsi_PFRGetGlyphByIndex:", index );
			PFRBuildChar( t, p, (long)t->charMap[index].gpsOffset, (uint16)byteCount, 
						0x00010000, 0x00010000, 0x00000000, 0x00000000, ffhint );
			if (t->verticalEscapement)
			{
				t->awx = 0;
				t->awy = t->noDelete_hmtx->aw[index];
			}
			else
			{
				t->awx = t->noDelete_hmtx->aw[index];
				t->awy = 0;
			}
			if ( t->glyph->contourCount == 0 || t->contourOpen)
				{
				glyph_CloseContour( t->glyph );
				t->contourOpen = false;
				}
			tsi_DeAllocMem( t->mem, (void *)p );
			}
		}
	glyph = t->glyph;
	
	glyph->ooy[glyph->pointCount + 0] = 0;
	glyph->oox[glyph->pointCount + 0] = 0;
	
	glyph->ooy[glyph->pointCount + 1] = (short)t->awy;
	glyph->oox[glyph->pointCount + 1] = (short)t->awx;
	
	*aWidth = (uint16)t->awx;
	
	{
		uint16 ah;

#		if SbPtCount >= 4
		{
			long xMid = (glyph->oox[glyph->pointCount + 0] + glyph->oox[glyph->pointCount + 1]) >> 1;
			int16 tsb, ymax, *ooy = glyph->ooy;
			int i, limit = glyph->pointCount;
			
			ymax = ooy[0];
			for ( i = 1; i < limit; i++ ) {
				if ( ooy[i] > ymax ) ymax = ooy[i];
			}
			/* Simulate */
			ah 	= (uint16)t->upem;
			tsb = (int16)(ah/10);
			
			ooy[glyph->pointCount + 2] 			= (short)(ymax + tsb);
			glyph->oox[glyph->pointCount + 2]	= (short)xMid;
			
			ooy[glyph->pointCount + 3]			= (short)(ooy[glyph->pointCount + 2] - ah);
			glyph->oox[glyph->pointCount + 3]	= (short)xMid;
		}
#		else
		ah = 0;
#		endif
		*aHeight = ah;
	}	
	t->glyph = NULL;
	FlipContourDirection( glyph );
	return glyph; /*****/
}


static void PFRBuildChar( PFRClass *t, uint8 *p, long gpsOffset, uint16  gpsSize,
	F16Dot16 xScale, F16Dot16 yScale, F16Dot16 xPos, F16Dot16 yPos , FFT1HintClass *ffhint )
{
uint8	*pSubChar;	/* buffer pointer for sub-character element */
uint8   format;
uint8   index;
uint8  *pGps = p;
uint8  *pByte;
int16   nElements;
int16   ii;
int16   x1, y1, x2, y2, x3, y3;
F16Dot16   fx1, fy1, fx2, fy2, fx3, fy3;
#ifdef ENABLE_NATIVE_T1_HINTS
uint8  *pExtraItems;
F16Dot16 tempx;
F16Dot16 tempy;
int chunkSize;
#endif

#if DEBUG
int16   depth;
#endif
gpsExecContext_t gpsExecContext;
charElement_t glyphElement;
uint32 offset;
int byteCount;

	pByte = pGps;
	format = NEXT_BYTE(pByte);      /* Read format byte */
	if (format & BIT_7)             /* Compound glyph? */
	{
		if (format & BIT_6)
		{/* skip extra items */
			SkipExtraItems(&pByte);
		}
		nElements = (int16)(format & 0x3f);
		for (ii = 0; ii < nElements; ii++)
	    {
			ReadGlyphElement(t, &pByte, &glyphElement, &gpsOffset);
#if DEBUG
			if (debugOn) printf("    DOCH: offset %4ld, size %2d, xScale %4.2f, yScale %4.2f, xPos %4.2f, yPos %4.2f\n",
				(long)glyphElement.glyphProgStringOffset, 
				(int)glyphElement.glyphProgStringSize, 
				(double)glyphElement.xScale / 65536.0, 
				(double)glyphElement.yScale / 65536.0, 
				(double)glyphElement.xPosition / 65536.0, 
				(double)glyphElement.yPosition / 65536.0);
#endif
#if 0		/* BEGIN: J.C. says this has never been used! */
			if (gpsSize > (pByte - pGps))
	    	{
				/* we'll have to acquire the adjustment vectors and apply them */
		    	ShowAdjustmentVector(
		    		pGps + gpsSize,
	    			gpsSize - (pByte - pGps));
	    	}
#endif	/* END: J.C. says this has never been used! */
	    	
			offset = t->firstGpsOffset + glyphElement.glyphProgStringOffset;
			Seek_InputStream( t->in, offset );
			byteCount = glyphElement.glyphProgStringSize;
			pSubChar = (uint8 *)tsi_AllocMem(t->mem, (size_t)byteCount);
			ReadSegment( t->in, pSubChar, byteCount );
			PFRBuildChar( t, pSubChar, (long)glyphElement.glyphProgStringOffset, glyphElement.glyphProgStringSize,
						glyphElement.xScale, glyphElement.yScale, glyphElement.xPosition, glyphElement.yPosition, ffhint);
			tsi_DeAllocMem(t->mem, (void *)pSubChar);
		}
	}                      
	else                          	/* Simple glyph? */
	{
		GlyphClass *glyph = t->glyph;
	    tcb_t tcbSave;
	    
	    InitGpsExecContext(&gpsExecContext);

	    ReadOruTable(t, format, &pByte, &gpsExecContext);
#if DEBUG
	    if (debugOn) printf("    Controlled X coordinates = [");
	    for (ii = 0; debugOn && ii < gpsExecContext.nXorus; ii++)
	    {
	        printf("%d ", (int)gpsExecContext.xOruTable[ii]);
	    }
	    if (debugOn) printf("]\n");
#endif
		


#if DEBUG
	    if (debugOn) printf("    Controlled Y coordinates = [");
	    for (ii = 0; debugOn && ii < gpsExecContext.nYorus; ii++)
	    {
	        printf("%d ", (int)gpsExecContext.yOruTable[ii]);
	    }
	    if (debugOn) printf("]\n");
#endif


#ifdef ENABLE_NATIVE_T1_HINTS
		pExtraItems = NULL;
#endif
	    if (format & BIT_3)
	    {
#ifdef ENABLE_NATIVE_T1_HINTS
			pExtraItems = pByte;
#endif
	        SkipExtraItems(&pByte);
	    }

		tcbSave = t->tcb;					/* save the transformation control block */
		SetupTrans(t,
					xScale, 
					yScale, 
					xPos, 
					yPos);				/* alter t->tcb based on input params */

#ifdef ENABLE_NATIVE_T1_HINTS
		/* Have to parse out the extra hints for this character */
		if ((ffhint != NULL) && (pExtraItems != NULL))
			ProcessExtraHints(t, ffhint, &pExtraItems);

		/* Store the X controlled coordinates */
		if (ffhint != NULL)
		{

			if (gpsExecContext.nXorus > 0)
			{
				
				if ((ffhint->numxOrus + gpsExecContext.nXorus) >= ffhint->xOrus_num_ml)
				{
					chunkSize = (gpsExecContext.nXorus < WIDTH_CHUNK) ? WIDTH_CHUNK : gpsExecContext.nXorus;
					ffhint->xOrus_num_ml = (int16)(ffhint->xOrus_num_ml + chunkSize);
					ffhint->xOrus_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->xOrus_ptr , (ffhint->xOrus_num_ml) * sizeof(int16) );
				}
				if ((ffhint->numxOrus + ffhint->numxbgcount + gpsExecContext.nXorus) >= ffhint->xgcount_ml)
				{
					chunkSize = (gpsExecContext.nXorus < WIDTH_CHUNK) ? WIDTH_CHUNK : gpsExecContext.nXorus;
					chunkSize += ffhint->numxbgcount;
					ffhint->xgcount_ml = (int16)(ffhint->xgcount_ml + chunkSize);
					ffhint->xgcount_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->xgcount_ptr , (ffhint->xgcount_ml) * sizeof(int16) );
				}

				for (ii = 0; ii < gpsExecContext.nXorus; ii++)
				{
					TransformPoint(t, ((F16Dot16)gpsExecContext.xOruTable[ii] << 16), 1, &tempx, &tempy);
					ffhint->xOrus_ptr[ffhint->numxOrus + ii] = (int16)(tempx >> 16);
					ffhint->xgcount_ptr[ffhint->numxOrus + ffhint->numxbgcount + ii] = t->glyph->pointCount;
				}
			}
			else
			{
				ffhint->xgcount_ptr[ffhint->numxOrus] = -999;

				if ((ffhint->numxbgcount + 1) >= ffhint->xbgcount_ml)
				{
					ffhint->xbgcount_ml = (int16)(ffhint->xbgcount_ml + WIDTH_CHUNK);
					ffhint->xbgcount_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->xbgcount_ptr , (ffhint->xbgcount_ml) * sizeof(int16) );
				}
				ffhint->xbgcount_ptr[ffhint->numxbgcount++] = t->glyph->pointCount;
			}

		ffhint->numxOrus += gpsExecContext.nXorus;
		}

		/* Store the Y controlled coordinates */
		if (ffhint != NULL)
		{
			if (gpsExecContext.nYorus > 0)
			{
				if ((ffhint->numyOrus + gpsExecContext.nYorus) >= ffhint->yOrus_num_ml)
				{
					chunkSize = (gpsExecContext.nYorus < WIDTH_CHUNK) ? WIDTH_CHUNK : gpsExecContext.nYorus;
					ffhint->yOrus_num_ml = (int16)(ffhint->yOrus_num_ml + chunkSize);
					ffhint->yOrus_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->yOrus_ptr , (ffhint->yOrus_num_ml) * sizeof(int16) );
				}
				if ((ffhint->numyOrus + gpsExecContext.nYorus) >= ffhint->ygcount_ml)
				{
					chunkSize = (gpsExecContext.nYorus < WIDTH_CHUNK) ? WIDTH_CHUNK : gpsExecContext.nYorus;
					chunkSize += ffhint->numybgcount;
					ffhint->ygcount_ml = (int16)(ffhint->ygcount_ml + chunkSize);
					ffhint->ygcount_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->ygcount_ptr , (ffhint->ygcount_ml) * sizeof(int16) );
				}
				for (ii = 0; ii < gpsExecContext.nYorus; ii++)
				{
					TransformPoint(t, 1, ((F16Dot16)gpsExecContext.yOruTable[ii] << 16), &tempx , &tempy);
					ffhint->yOrus_ptr[ffhint->numyOrus + ii] = (int16)(tempy >> 16);
					ffhint->ygcount_ptr[ffhint->numyOrus + ffhint->numybgcount +  ii] = t->glyph->pointCount;
				}
			}
			else
			{
				ffhint->ygcount_ptr[ffhint->numyOrus] = -999;

				if ((ffhint->numybgcount + 1) >= ffhint->ybgcount_ml)
				{
					ffhint->ybgcount_ml = (int16)(ffhint->ybgcount_ml + WIDTH_CHUNK);
					ffhint->ybgcount_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->ybgcount_ptr , (ffhint->ybgcount_ml) * sizeof(int16) );
				}
				ffhint->ybgcount_ptr[ffhint->numybgcount++] = t->glyph->pointCount;
			}
			ffhint->numyOrus += gpsExecContext.nYorus;
		}

		/* Make note that a tcb was used */
		if (ffhint != NULL)
		{
			ffhint->num_tcb++;
		}
#endif


		if (t->pfrType > 0)				/* Compressed glyph prog string? */
	   	{
	   		uint8	*pLastByte;
	   		nibble_t nibble;
	   		int16	type;
	   		int16	values[6];
			   		
		    pLastByte = pGps + gpsSize - 1;
		    gpsExecContext.xPrev2Value = 0;
		    gpsExecContext.yPrev2Value = 0;
		    gpsExecContext.xPrevValue = 0;
		    gpsExecContext.yPrevValue = 0;
		    nibble.pByte = pByte;
		    nibble.phase = 0;
		    type = -1;
		    
		   	t->contourOpen = false;
		    /* Cycle through rest of glyph program string */
		    while ((nibble.pByte < pLastByte) ||
		    	((nibble.pByte == pLastByte) && (nibble.phase == 0)))
		    {
		        ReadGpsSegment(&gpsExecContext, &nibble, &type, values);
		        switch (type)
		        {
		        case 0:	/* Move */
#if DEBUG
		            if (debugOn)
		            	printf("\n    MOVE %4d %4d\n", 
		            		values[0], values[1]);
#endif
					if (t->contourOpen)
						glyph_CloseContour( glyph );
					TransformPoint(t, (F16Dot16)values[0] << 16, (F16Dot16)values[1] << 16, &fx1, &fy1);
					x1 = (int16)(fx1 >> 16);
					y1 = (int16)(fy1 >> 16);

#if DEBUG
		            if (debugOn) printf("\n    	**MOVE %4d %4d\n", 
		            	x1, y1);
#endif
		        	glyph_AddPoint( glyph, (long)x1, (long)y1, 1 );

				   	t->contourOpen = true;
		            break;
		            
		        case 1: /* Line */
#if DEBUG
		            if (debugOn) printf("    LINE %4d %4d\n", 
		            	values[0], values[1]);
#endif
					fx1 = util_FixMul(xScale, ((F16Dot16)values[0] << 16));	/* first scale,  */
					fy1 = util_FixMul(yScale, ((F16Dot16)values[1] << 16));	/* first scale,  */
					fx1 += xPos;									/* then shift */
					fy1 += yPos;									/* then shift */
					fx1 += util_FixMul(fy1, t->fontMatrix.m01);				/* oblique, */ 
					x1 = (int16)((util_FixMul( fx1, t->xyScale )) >> 16);
					y1 = (int16)((util_FixMul( fy1, t->xyScale )) >> 16);
					TransformPoint(t, (F16Dot16)values[0] << 16, (F16Dot16)values[1] << 16, &fx1, &fy1);
					x1 = (int16)(fx1 >> 16);
					y1 = (int16)(fy1 >> 16);

#if DEBUG
		            if (debugOn) printf("    	**LINE %4d %4d\n", 
		            	x1, y1);
#endif
		        	glyph_AddPoint( glyph, (long)x1, (long)y1, 1 );

		        	break;
		        	
		        case 2:	/* Curve */
#if DEBUG
		        	if (debugOn) printf("    CRVE %4d %4d %4d %4d %4d %4d\n", 
		        		values[0], values[1],
		        		values[2], values[3],
		        		values[4], values[5]);
#endif
					TransformPoint(t, (F16Dot16)values[0] << 16, (F16Dot16)values[1] << 16, &fx1, &fy1);
					x1 = (int16)(fx1 >> 16);
					y1 = (int16)(fy1 >> 16);
					TransformPoint(t, (F16Dot16)values[2] << 16, (F16Dot16)values[3] << 16, &fx2, &fy2);
					x2 = (int16)(fx2 >> 16);
					y2 = (int16)(fy2 >> 16);
					TransformPoint(t, (F16Dot16)values[4] << 16, (F16Dot16)values[5] << 16, &fx3, &fy3);
					x3 = (int16)(fx3 >> 16);
					y3 = (int16)(fy3 >> 16);
#if DEBUG
		        	if (debugOn) printf("    	**CRVE %4d %4d %4d %4d %4d %4d\n", 
		        		x1, y1,
		        		x2, y2,
		        		x3,y3);
#endif
		        	glyph_AddPoint( glyph, (long)x1, (long)y1, 0 );
		        	glyph_AddPoint( glyph, (long)x2, (long)y2, 0 );
		        	glyph_AddPoint( glyph, (long)x3, (long)y3, 1 );
				}
			}
	   	}
	   	else							/* Standard glyph program string? */
	   	{
		   	t->contourOpen = false;
	   		while (true)
		    {
		        format = NEXT_BYTE(pByte);
		        switch (format >> 4)
		        {
		        case 0:                 /* End instruction */
#if DEBUG
		            if (debugOn) printf("    END\n");
#endif
					if (t->contourOpen)
					{
						glyph_CloseContour( glyph );
						t->contourOpen = false;
					}
					t->tcb = tcbSave;					/* restore the transformation control block */
		            return;

		        case 1:                 /* Line instruction */
		            ReadGpsArgs(&pByte, format, &gpsExecContext, &x1, &y1);
					TransformPoint(t, (F16Dot16)x1 << 16, (F16Dot16)y1 << 16, &fx1, &fy1);
					x1 = (int16)(fx1 >> 16);
					y1 = (int16)(fy1 >> 16);
#if DEBUG
		            if (debugOn)
		            {
		            	printf("    	 **LINE %4d %4d\n", (int)x1, (int)y1);
		            }
#endif
		        	glyph_AddPoint( glyph, (long)x1, (long)y1, 1 );
		            break;

		        case 2:                 /* Horizontal line instruction */
		            index = (uint8)(format & 0x0f);
		            x1 = gpsExecContext.xOruTable[index];
		            gpsExecContext.xPrevValue = x1;
		            y1 = gpsExecContext.yPrevValue;
					TransformPoint(t, (F16Dot16)x1 << 16, (F16Dot16)y1 << 16, &fx1, &fy1);
					x1 = (int16)(fx1 >> 16);
					y1 = (int16)(fy1 >> 16);
#if DEBUG
		            if (debugOn)
		            {
		            	printf("    	 **HLIN %4d %4d\n", (int)x1, (int)y1);
		            }
#endif
		        	glyph_AddPoint( glyph, (long)x1, (long)y1, 1 );
		            break;

		        case 3:                 /* Vertical line instruction */
		            x1 = gpsExecContext.xPrevValue;
		            index = (uint8)(format & 0x0f);
		            y1 = gpsExecContext.yOruTable[index];
		            gpsExecContext.yPrevValue = y1;
					TransformPoint(t, (F16Dot16)x1 << 16, (F16Dot16)y1 << 16, &fx1, &fy1);
					x1 = (int16)(fx1 >> 16);
					y1 = (int16)(fy1 >> 16);
#if DEBUG
		            if (debugOn)
		            {
		            	printf("    	 **VLIN %4d %4d\n", (int)x1, (int)y1);
		            }
#endif
		        	glyph_AddPoint( glyph, (long)x1, (long)y1, 1 );
		            break;

		        case 4:                 /* Move inside instruction */
		            ReadGpsArgs(&pByte, format, &gpsExecContext, &x1, &y1);
					if (t->contourOpen)
						glyph_CloseContour( glyph );
					TransformPoint(t, (F16Dot16)x1 << 16, (F16Dot16)y1 << 16, &fx1, &fy1);
					x1 = (int16)(fx1 >> 16);
					y1 = (int16)(fy1 >> 16);
		        	glyph_AddPoint( glyph, (long)x1, (long)y1, 1 );
#if DEBUG
		            if (debugOn)
		            {
		            	printf("\n    	 **MOVI %4d %4d\n", (int)x1, (int)y1);
		            }
#endif
				   	t->contourOpen = true;
		            break;

		        case 5:                 /* Move outside instruction */
		            ReadGpsArgs(&pByte, format, &gpsExecContext, &x1, &y1);
					if (t->contourOpen)
						glyph_CloseContour( glyph );
					TransformPoint(t, (F16Dot16)x1 << 16, (F16Dot16)y1 << 16, &fx1, &fy1);
					x1 = (int16)(fx1 >> 16);
					y1 = (int16)(fy1 >> 16);
#if DEBUG
		            if (debugOn)
		            {
		            	printf("\n    	 **MOVO %4d %4d\n", (int)x1, (int)y1);
		            }
#endif
		        	glyph_AddPoint( glyph, (long)x1, (long)y1, 1 );
				   	t->contourOpen = true;
		            break;

		        case 6:                 /* Horiz-vert curve quadrant */
#if DEBUG
		            depth = (int16)(format & 0x07);
#endif
		            ReadGpsArgs(&pByte, 14, &gpsExecContext, &x1, &y1);
		            ReadGpsArgs(&pByte,  8, &gpsExecContext, &x2, &y2);
		            ReadGpsArgs(&pByte, 11, &gpsExecContext, &x3, &y3);
					TransformPoint(t, (F16Dot16)x1 << 16, (F16Dot16)y1 << 16, &fx1, &fy1);
					x1 = (int16)(fx1 >> 16);
					y1 = (int16)(fy1 >> 16);
					TransformPoint(t, (F16Dot16)x2 << 16, (F16Dot16)y2 << 16, &fx2, &fy2);
					x2 = (int16)(fx2 >> 16);
					y2 = (int16)(fy2 >> 16);
					TransformPoint(t, (F16Dot16)x3 << 16, (F16Dot16)y3 << 16, &fx3, &fy3);
					x3 = (int16)(fx3 >> 16);
					y3 = (int16)(fy3 >> 16);
#if DEBUG
		            if (debugOn)
		            {
		            	printf("    	 **HVC%1d %4d %4d %4d %4d %4d %4d\n", 
		                				(int)depth, (int)x1, (int)y1, (int)x2, (int)y2, (int)x3, (int)y3);
		            }
#endif
		        	glyph_AddPoint( glyph, (long)x1, (long)y1, 0 );
		        	glyph_AddPoint( glyph, (long)x2, (long)y2, 0 );
		        	glyph_AddPoint( glyph, (long)x3, (long)y3, 1 );
		            break;

		        case 7:                 /* Vert-horiz curve quadrant */
#if DEBUG
		            depth = (int16)(format & 0x07);
#endif
		            ReadGpsArgs(&pByte, 11, &gpsExecContext, &x1, &y1);
		            ReadGpsArgs(&pByte,  2, &gpsExecContext, &x2, &y2);
		            ReadGpsArgs(&pByte, 14, &gpsExecContext, &x3, &y3);
					TransformPoint(t, (F16Dot16)x1 << 16, (F16Dot16)y1 << 16, &fx1, &fy1);
					x1 = (int16)(fx1 >> 16);
					y1 = (int16)(fy1 >> 16);
					TransformPoint(t, (F16Dot16)x2 << 16, (F16Dot16)y2 << 16, &fx2, &fy2);
					x2 = (int16)(fx2 >> 16);
					y2 = (int16)(fy2 >> 16);
					TransformPoint(t, (F16Dot16)x3 << 16, (F16Dot16)y3 << 16, &fx3, &fy3);
					x3 = (int16)(fx3 >> 16);
					y3 = (int16)(fy3 >> 16);
#if DEBUG
		            if (debugOn)
		            {
		            	printf("    	 **VHC%1d %4d %4d %4d %4d %4d %4d\n", 
		                		(int)depth, (int)x1, (int)y1, (int)x2, (int)y2, (int)x3, (int)y3);
		            }
#endif
		        	glyph_AddPoint( glyph, (long)x1, (long)y1, 0 );
		        	glyph_AddPoint( glyph, (long)x2, (long)y2, 0 );
		        	glyph_AddPoint( glyph, (long)x3, (long)y3, 1 );
		            break;

		        default:                /* Curve instruction */
#if DEBUG
		            depth = (int16)((format & 0x70) >> 4);

#endif
			        ReadGpsArgs(&pByte, format, &gpsExecContext, &x1, &y1);
		            format = NEXT_BYTE(pByte);
		            ReadGpsArgs(&pByte, format, &gpsExecContext, &x2, &y2);
		            format >>= 4;
		            ReadGpsArgs(&pByte, format, &gpsExecContext, &x3, &y3);
					TransformPoint(t, (F16Dot16)x1 << 16, (F16Dot16)y1 << 16, &fx1, &fy1);
					x1 = (int16)(fx1 >> 16);
					y1 = (int16)(fy1 >> 16);
					TransformPoint(t, (F16Dot16)x2 << 16, (F16Dot16)y2 << 16, &fx2, &fy2);
					x2 = (int16)(fx2 >> 16);
					y2 = (int16)(fy2 >> 16);
					TransformPoint(t, (F16Dot16)x3 << 16, (F16Dot16)y3 << 16, &fx3, &fy3);
					x3 = (int16)(fx3 >> 16);
					y3 = (int16)(fy3 >> 16);
#if DEBUG
		            if (debugOn)
		            {
		            	printf("    	 **CRV%1d %4d %4d %4d %4d %4d %4d\n", 
		                		(int)depth, (int)x1, (int)y1, (int)x2, (int)y2, (int)x3, (int)y3);
		            }
#endif
		        	glyph_AddPoint( glyph, (long)x1, (long)y1, 0 );
		        	glyph_AddPoint( glyph, (long)x2, (long)y2, 0 );
		        	glyph_AddPoint( glyph, (long)x3, (long)y3, 1 );
		            break;
		        }
		    }
	    }

		t->tcb = tcbSave;					/* restore the transformation control block */
		if (t->contourOpen)
		{
			glyph_CloseContour( glyph );
			t->contourOpen = false;
		}
	}
}


uint8 *GetPFRNameProperty( PFRClass *t, uint16 languageID, uint16 nameID )
{
	uint8 *result, *p;
	unsigned long length;
	UNUSED(languageID);
	UNUSED(nameID);
	length = strlen((const char *)t->fontID);
	p = t->fontID;
	result = (uint8 *)tsi_AllocMem( t->mem, (unsigned long)(length + 1) );
	strcpy((char *)result, (const char *)p);
	return result;
}



static int GetAuxDataMetrics(
	PFRClass *t ,
    uint8 *pAuxData,
    uint32 nAuxBytes,
    unsigned short requestedBlockID,
    short *ascent, short *descent, short *lineGap, short *maxAW,
    uint32 *isFixedPitch
    )
/*
 *  Walks through auxiliary data assuming format used by high-level
 *  TrueDoc library functions. Looks for specific requested blockID. If
 *	the requested ID is either IDWINTYPEATTR or IDPFMTYPEATTR, it can
 *	return typeface metrics for the font like ascent, descent, lineGap
 *	and maxAW.
 *	Returns 0 on success, 1 on error (no aux data, or metrics type not found).
 */
{
int errCode = 1;
blockHead_t *pAux = (blockHead_t *)pAuxData;
unsigned short   len;
unsigned short blockID;
uint8 match;

UNUSED(t);
if (nAuxBytes <= 0)
    {
#if DEBUG
	if (debugOn) printf("    No auxiliary data\n");
#endif
    return errCode;
    }

#if DEBUG
if (debugOn) printf("    Auxiliary data (size = %ld)\n", (long)nAuxBytes);
#endif
    match = false;
    while (!match && ((len = (unsigned short)GETAUXINT(pAux->len)) != 0))
        {
        blockID = (unsigned short)GETAUXINT(pAux->blockID);
		match = (uint8)(blockID == requestedBlockID);
        if (blockID == IDWINFACENAME)           /* Face name block */
            {
#if DEBUG
            if (debugOn) printf("        Face name = \"%s\"\n", (char *)(pAux + 1));
#endif
            }
        else if (blockID == IDWINTYPEATT)       /* Typographic attribute block */
            {
            auxTypeAtt_t *pWinAtt;

            pWinAtt = (auxTypeAtt_t *)(pAux+1); 
			if (match)
				{
				char pitchAndFamily;
	            *ascent = (short)((int16)GETAUXINT(pWinAtt->ascent));
	            *descent = (short)((int16)GETAUXINT(pWinAtt->descent));
	            *lineGap = (short)((int16)GETAUXINT(pWinAtt->internalLeading) + (int16)GETAUXINT(pWinAtt->externalLeading));
	            *maxAW = (short)((int16)GETAUXINT(pWinAtt->maxCharWidth));
	            pitchAndFamily = pWinAtt->pitchAndFamily;
	            *isFixedPitch = ((pitchAndFamily & 0x01) == 0);
				errCode = 0;
            	}
            }
        else if (blockID == IDWINSTYLE)      /* Style name block */
            {
#if DEBUG
            if (debugOn) printf("        Style name = %s\n", (char *)(pAux + 1));
#endif
            }
        else if (blockID == IDPOSTNAME)      /* PostScript name block */
            {
#if DEBUG
            if (debugOn) printf("        PostScript name = %s\n", (char *)(pAux + 1));
#endif
            }
        else if (blockID == IDADDTYPEATT)    /* Add'l type attributes block */
            {
            }
        else if (blockID == IDFULLFONTNAME)  /* Full font name block */
            {
#if DEBUG
            if (debugOn) printf("        Full font name = %s\n", (char *)(pAux + 1));
#endif
			}
        else if (blockID == IDPFMTYPEATT)  	/* PFM type attributes block */
            {
            auxPfmTypeAtt_t *pPfmAtt;

            pPfmAtt = (auxPfmTypeAtt_t *)(pAux + 1); 
			if (match)
				{
	            *ascent = (short)((int16)GETAUXINT(pPfmAtt->dfAscent));
	            *descent = (short)((int16)GETAUXINT(pPfmAtt->etmLowerCaseDescent));
	            *lineGap = (short)((int16)GETAUXINT(pPfmAtt->lineGap));
	            *maxAW = (short)((int16)GETAUXINT(pPfmAtt->dfMaxWidth));
	            *isFixedPitch = pPfmAtt->fixedPitch;
	            errCode = 0;
				}
			}
        else if (blockID == IDPAIRKERNDATA)  /* Pair kerning data block */
            {
            }
        else if (blockID == IDTRACKKERNDATA)  /* Track kerning data block */
			{
            }
        else if (blockID == 0x8002)	/* DocLock data (single CRC version) */
        	{
        	}
        else if (blockID == 0x8003)	/* DocLock data (multiple CRC version) */
        	{
        	}
        else
        	{
#if DEBUG
        	if (debugOn) printf("        AuxData blockID = 0x%04x\n", blockID);
#endif
        	}   
        pAux = (blockHead_t *)((char *)pAux + len);
        }
return errCode;
}

/*
 *
 */
static void FlipContourDirection(GlyphClass *glyph)
{
	short	ctr, j;
	short	*oox = 	glyph->oox;
	short	*ooy = 	glyph->ooy;
	uint8 	*onCurve = glyph->onCurve;

	for ( ctr = 0; ctr < glyph->contourCount; ctr++ ) {
	 	short	flips, start, end;
	 	
	 	start	= glyph->sp[ctr];
	 	end		= glyph->ep[ctr];
	 	
	 	flips = (short)((end - start)/2);
	 	start++;
		for ( j = 0; j < flips; j++ ) {
			int16	tempX, tempY;
			uint8	pointType;
			int16   indexA = (int16)(start + j);
			int16   indexB = (int16)(end   - j);
	 		
	 		tempX				= oox[indexA];
	 		tempY				= ooy[indexA];
	 		pointType			= onCurve[indexA];
	 		
	 		oox[indexA]			= oox[indexB];
	 		ooy[indexA]			= ooy[indexB];
	 		onCurve[indexA]		= onCurve[indexB];

	 		oox[indexB]			= tempX;
	 		ooy[indexB]			= tempY;
	 		onCurve[indexB]		= pointType;
		}
	}
}

/*************************************************************************************
*	BSearchCTbl()
*	RETURNS:	true on success, FAlSE on failure.
*				If successful, *indexPtr contains index of where found.
*************************************************************************************/

static uint8 BSearchCTbl ( PFRClass *t, uint16 *indexPtr, uint16 theValue)
{
   signed long    left, right, middle;
   int16    result;
   int32 nElements = t->NumCharStrings;

   left = 0;
   right = nElements - 1;

   while ( right >= left )
   {
      middle = (left + right)/2;
 
      if (theValue == t->charMap[middle].charCode)
      	result = 0;
      else if (theValue < t->charMap[middle].charCode)
      	result = -1;
      else
      	result = 1;
      	
      if ( result == -1 )
         right = middle - 1;
      else
         left = middle + 1;
      if ( result == 0 )
      {
         *indexPtr = (uint16)middle;
         return ( true );
      }
   }
#if DEBUG
if (debugOn)
	{
	int idx, offset = -5;
	printf("Failed to find 0x%4x, here's the neighboorhood:\n", (int)theValue);
	while (offset < 6)
		{
		idx = left + offset;
		printf("t->charMap[%d].charCode = 0x%4x\n", (int)idx, (int)t->charMap[idx].charCode);
		offset++;
		}
	}
#endif
   return ( false );
}


static void SetupTCB(PFRClass *t)
{
	t->tcb.m00 =
				util_FixMul(t->outputMatrix.m00, t->fontMatrix.m00) +
				util_FixMul(t->outputMatrix.m10, t->fontMatrix.m01);

	t->tcb.m01 =
				util_FixMul(t->outputMatrix.m01, t->fontMatrix.m00) +
				util_FixMul(t->outputMatrix.m11, t->fontMatrix.m01);

	t->tcb.m10 =
				util_FixMul(t->outputMatrix.m00, t->fontMatrix.m10) +
				util_FixMul(t->outputMatrix.m10, t->fontMatrix.m11);


	t->tcb.m11 =
				util_FixMul(t->outputMatrix.m01, t->fontMatrix.m10) +
				util_FixMul(t->outputMatrix.m11, t->fontMatrix.m11);
	t->tcb.xOffset = 0;
	t->tcb.yOffset = 0;
	return;
}

static void SetupTrans(PFRClass *t,
				F16Dot16 xScale, 
				F16Dot16 yScale, 
				F16Dot16 xPosition, 
				F16Dot16 yPosition)
{
	t->tcb.xOffset += 
	    util_FixMul(t->tcb.m00, xPosition) +
	    util_FixMul(t->tcb.m10, yPosition);

	t->tcb.yOffset += 
	    util_FixMul(t->tcb.m01, xPosition) +
	    util_FixMul(t->tcb.m11, yPosition);

	t->tcb.m00 = util_FixMul(t->tcb.m00, xScale);

	t->tcb.m10 = util_FixMul(t->tcb.m10, yScale);

	t->tcb.m01 = util_FixMul(t->tcb.m01, xScale);

	t->tcb.m11 = util_FixMul(t->tcb.m11, yScale);
	return;
}

static void TransformPoint(PFRClass *t, F16Dot16 x, F16Dot16 y, F16Dot16 *pXt, F16Dot16 *pYt)
{
F16Dot16   xt, yt;
	xt =
			util_FixMul(x, t->tcb.m00) +
			util_FixMul(y, t->tcb.m10);
	yt =
			util_FixMul(x, t->tcb.m01) +
			util_FixMul(y, t->tcb.m11);
 
	*pXt = xt + t->tcb.xOffset;
	*pYt = yt + t->tcb.yOffset;
	return;
}

#ifdef ENABLE_SBIT
static char GetPixRunInRow(
    state_t *pState)
/*
 *  Collects a run of pixels from the current bitmap image.
 *  A pixel run consists of zero or more white pixels followed 
 *  zero or more black pixels with at least one pixel total.
 *  Pixel runs that extend over multiple rows are divided at the
 *  row boundaries.
 *  Returns:
 *      true:   Black run from (state.x1) to (state.x2) in row (state.y).
 *      false:  No more black pixels in bitmap image.
 */
{
int32   blackLeft;
int32   x;
int16   xSize;

if ((pState->blackLeft == 0))   /* Current run completed? */
    {
    pState->GetPixRun(pState);  /* Get another pixel run */
    }

/* Move to start of black and adjust row */
x = (int32)pState->x1 + pState->whiteLeft;
pState->whiteLeft = 0;
xSize = pState->xSize;
while(x >= (int32)xSize)		/* Remainder of row is white? */
    {
    x -= (int32)xSize;
    pState->y = (int16)(pState->y + pState->yIncrement);
    }

pState->x0 = (int16)x;
blackLeft = pState->blackLeft;
if (x + blackLeft >= (int32)xSize) /* Remainder of row is  black? */
    {
    pState->x1 = xSize;
    pState->blackLeft -= (int32)xSize - x;
    return true;
    }

if (blackLeft > 0)              /* Remaining black fits within row? */
    {
    pState->x1 = (int16)(pState->x0 + blackLeft);
    pState->blackLeft = 0;
    return true;
    }

return false;
}

static void GetPixRun0(
    state_t *pState)
/*
 *  Collects a run of pixels from a directly encoded bitmap image.
 *  A pixel run consists of zero or more white pixels followed 
 *  zero or more black pixels with at least one pixel total.
 */
{
int32   whiteLeft;
int32   blackLeft;
int32   bitsLeft;
uint8   mask;
uint8   black;
uint8   byte;

whiteLeft = 0;
blackLeft = 0;
mask = pState->mask;
black = pState->black;
bitsLeft = pState->bitsLeft;
if (bitsLeft <= 0)              /* End of bitmap image? */
    {
    goto L1;
    }

/* Count run of white pixels */
byte = (uint8)(*(pState->pByte) ^ black);
while (byte & mask)             /* White bit? */
    {
    whiteLeft++;                /* Increment white bit count */
    if (--bitsLeft <= 0)        /* End of bitmap image? */
        {
        goto L1;
        }
    mask >>= 1;
    if (mask == 0)
        {
        mask = 0x80;
        byte = (uint8)(*(++pState->pByte) ^ black);
        }
    }

/* Count run of black pixels */
byte = (uint8)(*(pState->pByte) ^ ~black);
while (byte & mask)             /* Black bit? */
    {
    blackLeft++;                /* Increment black bit count */
    if (--bitsLeft <= 0)        /* End of bitmap image? */
        {
        goto L1;
        }
    mask >>= 1;
    if (mask == 0)
        {
        mask = 0x80;
        byte = (uint8)(*(++pState->pByte) ^ ~black);
        }
    }

L1:
pState->whiteLeft = whiteLeft;
pState->blackLeft = blackLeft;
pState->mask = mask;
pState->bitsLeft = bitsLeft;
}

static void GetPixRun1(
    state_t *pState)
/*
 *  Collects a run of pixels from a nibble run-length encoded bitmap image.
 *  A pixel run consists of zero or more white pixels followed 
 *  zero or more black pixels with at least one pixel total.
 */
{
uint8   byte;
uint8   newWhite;
uint8   newBlack;
char joinRequired;

pState->whiteLeft = 0;
pState->blackLeft = 0;

if (pState->bitsLeft <= 0)
    {
    return;
    }

do
    {
    byte = *(pState->pByte++);
    newWhite = (uint8)(byte >> 4);
    newBlack = (uint8)(byte & 0x0f);
    pState->bitsLeft -= (int32)newWhite + (int32)newBlack;
    pState->whiteLeft += (int32)newWhite;
    pState->blackLeft += (int32)newBlack;
#if 0
    joinRequired = 
        (pState->bitsLeft > 0) &&
        ((newBlack == 0) || (*(pState->pByte) == 0));
#else
	joinRequired = 0;
	if ((pState->bitsLeft > 0) &&
        ((newBlack == 0) || (*(pState->pByte) == 0)))
		joinRequired = 1;
#endif
    } while (joinRequired);
}

static void GetPixRun2(
    state_t *pState)
/*
 *  Collects a run of pixels from a byte run-length encoded bitmap image.
 *  A pixel run consists of zero or more white pixels followed 
 *  zero or more black pixels with at least one pixel total.
 *  Adjacent runs are combined if the first one has zero black pixels or
 *  the second one has zero white pixels.
 */
{
uint8   newWhite;
uint8   newBlack;
char joinRequired;

pState->whiteLeft = 0;
pState->blackLeft = 0;

if (pState->bitsLeft <= 0)
    {
    return;
    }

do
    {
    newWhite = *(pState->pByte++);
    pState->whiteLeft += (int32)newWhite;
    newBlack = *(pState->pByte++);
    pState->blackLeft += (int32)newBlack;
    pState->bitsLeft -= (int32)newWhite + (int32)newBlack;
#if 0
    joinRequired = 
        (pState->bitsLeft > 0) &&
        ((newBlack == 0) || (*(pState->pByte) == 0));
#else
	joinRequired = 0;
	if ((pState->bitsLeft > 0) &&
        ((newBlack == 0) || (*(pState->pByte) == 0)))
		joinRequired = 1;
#endif
    } while (joinRequired);
}
#endif	/* ENABLE_SBIT */

#ifdef ENABLE_SBIT
static int	PFR_FindGlyph( PFRClass *t, uint16 glyphIndex, uint8 greyScaleLevel, uint16 ppemX, uint16 ppemY, pfrGlyphInfoData *gInfo );
static void PFR_ExtractBitMap( T2K *scaler, pfrGlyphInfoData *gInfo, PFRClass *t, uint8 greyScaleLevel,
								int callLevel);

static int	PFR_FindGlyph( PFRClass *t, uint16 glyphIndex, uint8 greyScaleLevel, uint16 ppemX, uint16 ppemY, pfrGlyphInfoData *gInfo )
{
int ii;
bmapStrike_t *strikePtr = NULL;
int result = false;
char found;
uint32 xppm = (uint32)ppemX, yppm = (uint32)ppemY;	
found = false;
if (greyScaleLevel != BLACK_AND_WHITE_BITMAP)
	{
#ifdef PFR_SUBSCALE_BITMAPS	
	/* this is not ready for prime time: */
	xppm = (uint32)ppemX * 4;
	yppm = (uint32)ppemY * 4;
#else
	return false;
#endif
	}
for (ii = 0; !found && (ii < t->nBmapStrikes); ii++)
	{/* linear search for ppemX, ppemY */
	found = false;
	if (((uint32)t->bmapStrikes[ii].xppm == xppm) && ((uint32)t->bmapStrikes[ii].yppm == yppm))
		found = true;
	if (found)
		strikePtr = (bmapStrike_t *)&t->bmapStrikes[ii];
	}
if (found)
	{/* linear search for glyphIndex */
	found = false;
	for (ii = 0; !found && (ii < strikePtr->nBmapChars); ii++)
		{
		found = false;
		if (strikePtr->bmapDir[ii].charIndex == glyphIndex)
			found = true;
		if (found)
			{
			gInfo->glyphIndex = glyphIndex;
			gInfo->greyScaleLevel = greyScaleLevel;
			gInfo->ppemX = ppemX;
			gInfo->ppemY = ppemY;
			gInfo->gpsSize = strikePtr->bmapDir[ii].gpsSize;
			gInfo->gpsOffset = strikePtr->bmapDir[ii].gpsOffset;
			gInfo->gpsPtr = NULL;
			result = true;
			}
		}
	}
return result;
}


#if MSB_FIRST
static uint8 msBits[] = {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};
static uint8 lsBits[] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00};
#else
static uint8 msBits[] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};
static uint8 lsBits[] = {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};
#endif

static void SetBitmapBits(pfrGlyphInfoData *gInfo, int16 y, int16 x1, int16 x2);
static void SetBitmapBits(pfrGlyphInfoData *gInfo, int16 y, int16 x1, int16 x2)
{
int 	start_byte;
int 	end_byte;
int 	i;
int32 	curr_index;
uint8	mask;
uint8	fgdByte = 0xff;
uint8  *pByte;

#if DEBUG >= 2
printf("SetBitmapBits(%d, %d, %d)\n",
    (int)y,
    (int)x1,
    (int)x2);
#endif

#if DEBUG
if (x1 < 0)
    printf("*** SetBitmapBits: x1 = %d\n", 
    (int)x1); 

if (x2 <= x1)
    printf("*** SetBitmapBits: x1 = %d; x2 = %d\n", 
    (int)x1, 
    (int)x2);
#endif
/* Return if zero- or negative-length run */
if (x1 >= x2)
    return;

start_byte = x1 >> 3;
end_byte = x2 >> 3;

/* Fill run of pixels with foreground value */
curr_index = (gInfo->invertBitmap)?
    (int32)y * gInfo->bytesPerRow:
    (int32)(gInfo->height - y - 1) *
        (gInfo->bytesPerRow);
pByte = gInfo->dst + curr_index + start_byte;
if (start_byte == end_byte)
    {
	mask = (uint8)(lsBits[x1 & 7] & msBits[x2 & 7]);
	*pByte = (uint8)((*pByte & ~mask) + (fgdByte & mask));
    }
else
    {
	mask = lsBits[x1 & 7];
	*pByte = (uint8)((*pByte & ~mask) + (fgdByte & mask));
	pByte++;
    for (i = start_byte + 1; i < end_byte; i++)
        {
        *(pByte++) = fgdByte;
        }
	mask = msBits[x2 & 7];
	*pByte = (uint8)((*pByte & ~mask) + (fgdByte & mask));
    }

}

#define bitmapAlignment	4
#define FOUR16DOT16		0x00040000

static void PFR_ExtractBitMap( T2K *scaler, pfrGlyphInfoData *gInfo, PFRClass *t, uint8 greyScaleLevel, int callLevel)
{
uint8 *pByte;
size_t bmapSize;
uint16 xSize = 0, ySize = 0, srcRowBytes, srcHeight;
#ifdef PFR_SUBSCALE_BITMAPS	
uint8 *src;
int ii, jj, kk;
#endif
uint8   bmapGpsFormat;
int32   xPos = 0, yPos = 0;
int32   escapement = 0;
int32   xEscapement;
int32   yEscapement;
uint8   byte;

	pByte = gInfo->gpsPtr;
	bmapGpsFormat = NEXT_UBYTE(pByte); /* Read GPS format byte */
	/* Read X and Y positions */
	switch (bmapGpsFormat & 3)
	    {
	case 0:
	    byte = NEXT_UBYTE(pByte);
#if EXPLICIT_SIGN_EXTENSION
	    xPos = (int32)(byte & 0xf0) << 12;
	    if (xPos >= 0x00080000)
	    	xPos -= 0x00100000;
	    yPos = (int32)(byte & 0x0f) << 16;
	    if (yPos >= 0x00080000)
	    	yPos -= 0x00100000;
#else
	    xPos = (int32)((int8)(byte & 0xf0)) << 12;
	    yPos = (int32)((int8)((byte << 4) & 0xf0)) << 12;
#endif
	    break;
	
	case 1:
	    xPos = (int32)NEXT_SBYTE(pByte) << 16;
	    yPos = (int32)NEXT_SBYTE(pByte) << 16;
	    break;
	
	case 2:
	    xPos = (int32)NEXT_SWORD(pByte) << 8;
	    yPos = (int32)NEXT_SWORD(pByte) << 8;
	    break;
	
	case 3:
	    xPos = NEXT_SLONG(pByte) << 16;
	    yPos = NEXT_SLONG(pByte) << 16;
	    break;
	    }
	
	/* Read X and Y dimensions */
	switch ((bmapGpsFormat >> 2) & 3)
	    {
	case 0:
	    xSize = ySize = 0;
	    break;
	
	case 1:
	    byte = NEXT_UBYTE(pByte);
	    xSize = (uint16)(byte >> 4);
	    ySize = (uint16)(byte & 0x0f);
	    break;
	
	case 2:
	    xSize = (uint16)NEXT_UBYTE(pByte);
	    ySize = (uint16)NEXT_UBYTE(pByte);
	    break;
	
	case 3:
	    xSize = (uint16)NEXT_SWORD(pByte);
	    ySize = (uint16)NEXT_SWORD(pByte);
	    break;
	    }
	
	/* Read escapement if present */
	xEscapement = 0;
	yEscapement = 0;
	switch((bmapGpsFormat >> 4) & 3)
	    {
	case 0:			/* Escapement is unrounded linear width */
		{
		int16 SW;
		F26Dot6 result26;
			SW = (int16)scaler->font->hmtx->aw[gInfo->glyphIndex];
		
			if (t->verticalEscapement)
		    {
				result26 = util_FixMul( scaler->yScale.fixedScale, SW );
			}
			else
			{
				result26 = util_FixMul( scaler->xScale.fixedScale, SW );
			}
			escapement = result26 << 10; /* 16.16 target */
			if (callLevel)
				escapement = util_FixMul(escapement, 0x00040000);
		}
		break;
	case 1:         /* Escapement is specified in whole pixels */
	    escapement = (int32)NEXT_SBYTE(pByte) << 16;
	    break;
	
	case 2:         /* Escapement is specified in 8.8 units */
	    escapement = (int32)NEXT_SWORD(pByte) << 8;
	    break;
	
	case 3:         /* Escapement is specified in 16.8 units */
	    escapement = NEXT_SLONG(pByte) << 8;
	    break;
	    }
	if (t->verticalEscapement)
	    {
	    yEscapement = escapement;
	    }
	else
	    {
	    xEscapement = escapement;
	    }
	gInfo->width = xSize;
	gInfo->height = ySize;
	if (greyScaleLevel == BLACK_AND_WHITE_BITMAP)
	{
	state_t state;
			
		if (callLevel == 1)
		{/* target buffer is already allocated: gInfo->tempBuffer */
			srcRowBytes = (uint16)((gInfo->width + 7) / 8);
		}
		else
		{
#ifdef MAKE_SC_ROWBYTES_A_4BYTE_MULTIPLE
			srcRowBytes = (uint16)((gInfo->width+31) / 32);
			srcRowBytes = (uint16)(srcRowBytes << 2);
#else
			srcRowBytes = (uint16)((gInfo->width+7) / 8);
#endif
		}
		gInfo->bytesPerRow = srcRowBytes;
		gInfo->greyScaleLevel = greyScaleLevel;
		gInfo->horiAdvance = xEscapement;
		gInfo->vertAdvance = yEscapement;
		gInfo->horiBearingX = gInfo->vertBearingX = xPos;
		{
		uint32 pos; /* 16.16 */
			pos = (uint32)(((long)ySize << 16) + yPos);
			gInfo->horiBearingY = gInfo->vertBearingY = (F26Dot6)pos;
		}
	
		srcHeight = (uint16)gInfo->height;
		bmapSize = (size_t)(srcRowBytes * srcHeight);
	
		if (callLevel == 1)
			{/* target buffer is already allocated: gInfo->tempBuffer */
			gInfo->dst = gInfo->tempBuffer;
			}
		else
			{/* target buffer is double scale->baseAddr: acquire in standard way */
			gInfo->baseAddr = NULL;
			scaler->internal_baseAddr = false;
			if (scaler->okForBitCreationToTalkToCache)
				{
					gInfo->baseAddr  = (uint8 *) scaler->GetCacheMemory( scaler->theCache,  bmapSize * sizeof( uint8 ) ); /* can return NULL */
				}
			if ( gInfo->baseAddr == NULL )
				{
					gInfo->baseAddr = (uint8 *) tsi_AllocMem( scaler->mem, bmapSize * sizeof( uint8 ) );
					assert( gInfo->baseAddr != NULL );
					scaler->internal_baseAddr = true;
				}
			gInfo->dst = gInfo->baseAddr;
			}
		memset((void *)gInfo->dst, 0, bmapSize); /* clear all bytes */
	
		/* now populate the 1 bit depth bitmap from source bitmap info: */
		state.xSize = (int16)gInfo->width;
		state.ySize = (int16)gInfo->height;
		/* Output bitmap data as series of horizontal runs */
		state.bitsLeft = (int32)state.xSize * state.ySize;
		if (state.bitsLeft > 0)         /* At least 1 pixel in image? */
		{
		    int16 y, x0, x1;
		    state.mask = 0x80;
		    state.pByte = pByte;
		    state.whiteLeft = 0;
		    state.blackLeft = 0;
		    state.GetPixRun = GetPixRun[(bmapGpsFormat >> 6) & 3];
		    state.x0 = 0;
		    state.x1 = 0;
		    if (t->bmapFlags & PFR_INVERT_BMAP) /* Read rows in descending Y order? */
		    {
		        state.y = (int16)(state.ySize - 1);
		        state.yIncrement = -1;
		        gInfo->invertBitmap = true;
		    }
		    else                        /* Read rows in ascending row order? */
		    {
		        state.y = 0;
		        state.yIncrement = 1;
		        gInfo->invertBitmap = false;
		    }
		    state.black = (uint8)((t->bmapFlags & PFR_BLACK_PIXEL)? 0xff: 0x00);
		    while (GetPixRunInRow(&state))
		    {
		        x0 = state.x0;
		        x1 = state.x1;
		        y = state.y;
		    	SetBitmapBits(gInfo, y, x0, x1);
		    }
		}
	}
#ifdef PFR_SUBSCALE_BITMAPS	
	/* this is not ready for prime time: */
	else
	{/* get super-sampled bitmap at one bit depth, then sub-sample: */
		uint16 dstRowBytes = 0, dstHeight = 0, dstRow, dstCol;
		uint16	srcBytesPerRow, srcLineHeight;
		xSize = (uint16)((gInfo->width + 3) / 4 * 4); /* multiple of 4 */
		srcLineHeight = ySize = (uint16)((gInfo->height + 3) / 4 * 4); /* multiple of 4 */
		srcBytesPerRow = (uint16)((xSize + 7) / 8);
		bmapSize = (size_t)(srcBytesPerRow * srcLineHeight);
		gInfo->tempBuffer = (uint8 *)tsi_AllocMem(t->mem, bmapSize);
		if (gInfo->tempBuffer)
			{
			uint8 sum1, sum2;
			uint8 *dst;
			PFR_ExtractBitMap( scaler, gInfo, t, BLACK_AND_WHITE_BITMAP, 1);
			/* sub-sample gInfo->tempBuffer into gInfo->baseAddr target */
			dstRowBytes = (uint16)(
						(((xSize * 8/*pixelSize*/ / 4/*subsampleRate*/) + 
						(bitmapAlignment << 3) - 1) / 
						(bitmapAlignment << 3)) * 
						bitmapAlignment);
			dstHeight = (uint16)(srcLineHeight / 4);
			bmapSize = (size_t)(dstRowBytes * dstHeight);
			gInfo->baseAddr = NULL;
			scaler->internal_baseAddr = false;
			if (scaler->->okForBitCreationToTalkToCache) {
				gInfo->baseAddr  = (uint8 *) scaler->GetCacheMemory( scaler->theCache,  bmapSize * sizeof( uint8 ) ); /* can return NULL */
			}
			if ( gInfo->baseAddr == NULL )
			{
				gInfo->baseAddr = (uint8 *) tsi_AllocMem( scaler->mem, bmapSize * sizeof( uint8 ) );
				assert( gInfo->baseAddr != NULL );
				scaler->internal_baseAddr = true;
			}
				
			assert( T2K_BLACK_VALUE == 126 );
	
			src = gInfo->tempBuffer;
			dst = gInfo->baseAddr;
			/* sub-sample from src to dst */
			for (ii = 0; ii < srcLineHeight; ii+=4)
			{
				dstRow = (uint16)(ii / 4);
				for (jj = 0, dstCol = 0; jj < srcBytesPerRow; jj++)
				{
					pByte = (uint8 *)&src[ii*srcBytesPerRow+jj];
					/* walk down 2 nibble columns, 4 rows */
					for (kk = 0, sum1 = 0, sum2 = 0; kk < 4; kk++)
					{
						sum1 += (uint8)(((*(pByte + kk * srcBytesPerRow) & 0xf0) >> 4) * 2);
						sum2 += (uint8)((*(pByte + kk * srcBytesPerRow) & 0x0f) * 2);
					}
					/* each sum is double sum of column, range 0-120, then value from 0 to 6 is added */	
					sum1 += (uint8)(sum1/20);
					sum2 += (uint8)(sum2/20);
					dst[dstRow * dstRowBytes + dstCol++] = sum1;
					dst[dstRow * dstRowBytes + dstCol++] = sum2;
				}
			}
		}
		/* clean up */
		if (gInfo->tempBuffer)
		{
			tsi_DeAllocMem(t->mem, gInfo->tempBuffer);
			gInfo->tempBuffer = NULL;
		}
		gInfo->width = gInfo->bytesPerRow = dstRowBytes;
		gInfo->height = dstHeight;
		gInfo->greyScaleLevel = greyScaleLevel;
	
		gInfo->horiAdvance = util_FixDiv( gInfo->horiAdvance, FOUR16DOT16 );
		gInfo->vertAdvance = util_FixDiv( gInfo->vertAdvance, FOUR16DOT16 );
		gInfo->horiBearingX = util_FixDiv( xPos, FOUR16DOT16 );
		gInfo->horiBearingY = util_FixDiv( yPos, FOUR16DOT16 );
	}
#endif /* #ifdef PFR_SUBSCALE_BITMAPS*/

}
/*
 * Query method to see if a particular glyph exists in sbit format for the current size.
 * If you need to use characterCode then map it to glyphIndex by using T2K_GetGlyphIndex() first.
 */
int PFR_GlyphSbitsExists( void *p, uint16 glyphIndex, uint8 greyScaleLevel, int *errCode  )
{
	T2K *scaler = (T2K *)p;
	pfrGlyphInfoData *gInfo = &scaler->font->PFR->gInfo;
	
	uint16 ppemX = (uint16)scaler->xPixelsPerEm;
	uint16 ppemY = (uint16)scaler->xPixelsPerEm;
	assert( errCode != NULL );
	if ( (*errCode = setjmp(scaler->mem->env)) == 0 ) {
		/* try */
		tsi_Assert( scaler->mem, scaler->mem->state == T2K_STATE_ALIVE, T2K_ERR_USE_PAST_DEATH );
		/* See if we have an indentity transformation */
		if ( scaler->is_Identity ) {
			PFR_FindGlyph( scaler->font->PFR, glyphIndex, greyScaleLevel, ppemX, ppemY, gInfo );
			return (gInfo->glyphIndex == glyphIndex && ppemX == gInfo->ppemX && ppemY == gInfo->ppemY);
		}
	} else {
		/* catch */
		tsi_EmergencyShutDown( scaler->mem );
		return false;
	}	
	return false; /*****/
}



/*
 * Gets the embeded bitmap
 */
int PFR_GetSbits(void *p, long code, uint8 greyScaleLevel, uint16 cmd)
{
	T2K *scaler = (T2K *)p;
	int result = false;	/* Initialize */

#ifndef PFR_SUBSCALE_BITMAPS	
	/* greyscale subsampling is not ready for prime time: */
	if (greyScaleLevel != BLACK_AND_WHITE_BITMAP)
		return false;
#endif
	
	/* See if we have an indentity transformation  */
	if ( scaler->is_Identity /* && ?? */ ) {
		uint16 ppemX = (uint16)scaler->xPixelsPerEm;
		uint16 ppemY = (uint16)scaler->yPixelsPerEm;
		pfrGlyphInfoData *gInfo = &scaler->font->PFR->gInfo;
		uint16 glyphIndex;
		
		glyphIndex = (uint16)((cmd & T2K_CODE_IS_GINDEX) ? code : tsi_PFRGetGlyphIndex( scaler->font->PFR, (uint16) code ));
		
		result = gInfo->glyphIndex == glyphIndex && ppemX == gInfo->ppemX && ppemY == gInfo->ppemY;
		if ( !result ) {
			PFR_FindGlyph( scaler->font->PFR, glyphIndex, greyScaleLevel, ppemX, ppemY, gInfo );
			result = gInfo->glyphIndex == glyphIndex && ppemX == gInfo->ppemX && ppemY == gInfo->ppemY;
		}
		if ( result ) {
			/* allocate and read up the gps for this guy */
			gInfo->gpsPtr = (uint8 *)tsi_AllocMem(scaler->font->PFR->mem, gInfo->gpsSize);
			if (!gInfo->gpsPtr)
				return result = false; /* early exit! */
			else
				{
				Seek_InputStream( scaler->font->PFR->in, scaler->font->PFR->firstGpsOffset + gInfo->gpsOffset );
				ReadSegment( scaler->font->PFR->in, (uint8 *)gInfo->gpsPtr, gInfo->gpsSize );
				}
			PFR_ExtractBitMap( scaler, gInfo, scaler->font->PFR, greyScaleLevel, 0);
			tsi_DeAllocMem(scaler->font->PFR->mem, gInfo->gpsPtr);
			gInfo->gpsPtr = NULL;
					
			scaler->baseAddr	= gInfo->baseAddr;	gInfo->baseAddr	= NULL; /* Hand over the pointer. */
			if ( scaler->baseAddr != NULL ) {
				scaler->xAdvanceWidth16Dot16 = gInfo->horiAdvance;
				/* scaler->yAdvanceWidth16Dot16 = gInfo->vertAdvance; */
				scaler->yAdvanceWidth16Dot16 = 0;
				
				scaler->rowBytes		= gInfo->bytesPerRow;	gInfo->bytesPerRow	= 0;
				scaler->width			= gInfo->width;	
				scaler->height			= gInfo->height;
				
				/* Set horizontal metrics. */
				scaler->horizontalMetricsAreValid = true;
				scaler->fLeft26Dot6		= gInfo->horiBearingX;	scaler->fLeft26Dot6		>>= 10;
				scaler->fTop26Dot6		= gInfo->horiBearingY; scaler->fTop26Dot6		>>= 10;
				scaler->xAdvanceWidth16Dot16		= gInfo->horiAdvance;
				scaler->xLinearAdvanceWidth16Dot16	= scaler->xAdvanceWidth16Dot16;
				scaler->yAdvanceWidth16Dot16 		= gInfo->vertAdvance;
				scaler->yLinearAdvanceWidth16Dot16	= scaler->yAdvanceWidth16Dot16;
				/* Set vertical metrics. */
				scaler->verticalMetricsAreValid = true;
				scaler->vert_fLeft26Dot6	= gInfo->vertBearingX;	scaler->vert_fLeft26Dot6	>>= 10;
				scaler->vert_fTop26Dot6		= gInfo->vertBearingY; scaler->vert_fTop26Dot6		>>= 10;
				scaler->vert_yAdvanceWidth16Dot16		= gInfo->vertAdvance;
				scaler->vert_yLinearAdvanceWidth16Dot16	= scaler->vert_yAdvanceWidth16Dot16;
				scaler->vert_xAdvanceWidth16Dot16 		= gInfo->horiAdvance;
				scaler->vert_xLinearAdvanceWidth16Dot16	= scaler->vert_xAdvanceWidth16Dot16;
				if (scaler->font->PFR->verticalEscapement)
					scaler->horizontalMetricsAreValid = false;
				else
					scaler->verticalMetricsAreValid = false;
			} else {
				result = false;
			}
		}
	}
	return result; /*****/
}
#endif /* ENABLE_SBIT */

void tsi_PFRListChars(void *userArg, PFRClass *t, void *ctxPtr, int ListCharsFn(void *userArg, void *p, uint16 code))
{
int ii, checkStop = 0;
	for (ii = 0; !checkStop && (ii < t->NumCharStrings); ii++)
	{
		checkStop = ListCharsFn(userArg, ctxPtr, t->charMap[ii].charCode);
	}
}

/* eof PFREAD.c */

#endif /* ENABLE_PFR */
/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 * 12/14/98 R. Eggers Created                                                *
 *     $Header: R:/src/FontFusion/Source/Core/rcs/pfrread.c 1.58 2001/05/07 16:52:26 reggers Exp $
 *                                                                           *
 *     $Log: pfrread.c $
 *     Revision 1.58  2001/05/07 16:52:26  reggers
 *     Fixed a memory overwrite of xgcount_ptr[] and ygcount_ptr[] in 
 *     the TYPE1 hint code.
 *     Revision 1.57  2001/05/04 21:44:09  reggers
 *     Warning cleanup.
 *     Revision 1.56  2001/05/02 21:40:23  reggers
 *     Warnings, warnings, warnings!
 *     Revision 1.55  2001/05/01 18:30:53  reggers
 *     Added support for GlyphExists()
 *     Revision 1.54  2001/04/26 19:28:13  reggers
 *     Made PFR kerning index based rather than charCode based.
 *     Revision 1.53  2001/04/23 19:30:21  reggers
 *     Fixed getting fontwide descent to get a negative value. Fixed incorrect
 *     setting of ascent to the descent when descent was not found
 *     in the aux data.
 *     Revision 1.52  2000/10/26 16:59:22  reggers
 *     Changes for SBIT: use cache based on setting of
 *     scaler->okForBitCreationToTalkToCache
 *     Revision 1.51  2000/07/11 17:32:08  reggers
 *     Borland STRICK warning fixes
 *     Revision 1.50  2000/06/16 22:05:03  reggers
 *     Disabled sub-sampling of bitmaps for time being.
 *     Revision 1.49  2000/06/16 17:18:01  reggers
 *     Made ENABLE_NATIVE_T1_HINTS user config option.
 *     Revision 1.48  2000/06/16 15:27:00  reggers
 *     Fixed warnings for Borland STRICT.
 *     Revision 1.47  2000/06/15 18:56:23  reggers
 *     PFR Bitmap proper setting of horiBearingY, horiAdvance.
 *     Revision 1.46  2000/06/14 21:26:39  reggers
 *     Casts for Borland STRICT.
 *     Revision 1.45  2000/06/12 20:53:51  reggers
 *     Borland STRICT warning removal.
 *     Revision 1.44  2000/06/07 15:19:30  mdewsnap
 *     Changes made for dynamic storage
 *     Revision 1.43  2000/05/30 20:43:30  reggers
 *     gcc warning removal
 *     Revision 1.42  2000/05/26 20:36:03  mdewsnap
 *     Fixed up ExtraEdges
 *     Revision 1.41  2000/05/26 18:56:19  reggers
 *     Set upem base on outline rather than metrics res. Set up initial state
 *     tcb as unary. Set width unit will be a problem.
 *     Revision 1.40  2000/05/26 17:08:49  mdewsnap
 *     Scale up the hint information.
 *     Revision 1.39  2000/05/25 17:34:01  reggers
 *     Remove point transformation of ORU tables.
 *     Revision 1.38  2000/05/24 23:02:29  reggers
 *     ReadOruTable: Transform orus as they are read, rather than passing
 *     through the ORU table a second time.
 *     Revision 1.37  2000/05/24 22:17:43  reggers
 *     Casts for 2 byte integer support.
 *     ReadOruTable: normalize all points as they are read.
 *     Revision 1.36  2000/05/24 20:57:12  mdewsnap
 *     Transformed global hint information.
 *     Revision 1.35  2000/05/23 21:02:09  mdewsnap
 *     Made sub char buffer dynamic.
 *     Revision 1.34  2000/05/19 14:39:31  mdewsnap
 *     Got rid of casts to short.
 *     Revision 1.33  2000/05/18 16:35:44  reggers
 *     Silence STRICT Borland warnings.
 *     Revision 1.32  2000/05/17 14:07:10  reggers
 *     Fast font open, bitmap option support for SBITS. 4 byte align.
 *     Revision 1.31  2000/05/16 18:44:40  mdewsnap
 *     Storing hint related fields.
 *     Revision 1.30  2000/04/13 18:13:58  reggers
 *     Updated list chars for user argument or context pass through.
 *     Revision 1.29  2000/04/06 16:36:13  reggers
 *     Disable full gps buffering. Slated for total removal: it was a bad idea!
 *     Revision 1.28  2000/03/27 20:54:12  reggers
 *     Bracketed intended debug with DEBUG
 *     Revision 1.27  2000/03/10 19:18:07  reggers
 *     Enhanced for enumeration of character codes in font.
 *     Revision 1.26  2000/02/25 17:45:46  reggers
 *     STRICT warning cleanup.
 *     Revision 1.25  2000/02/18 18:56:06  reggers
 *     Added Speedo processor capability.
 *     Revision 1.24  2000/01/06 21:58:14  reggers
 *     Some removal of legacy bits types. Corrected prototypes.
 *     Revision 1.23  1999/12/23 22:02:55  reggers
 *     New ENABLE_PCL branches. Rename any 'code' and 'data' symbols.
 *     Revision 1.22  1999/11/04 20:20:23  reggers
 *     Added code for getting fixed/proportional setting, firstCharCode and
 *     lastCharCode.
 *     Revision 1.21  1999/10/18 16:57:13  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.20  1999/10/15 17:54:01  reggers
 *     Purged use of btsbool data type -> char
 *     Revision 1.19  1999/09/30 15:10:28  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.18  1999/09/20 18:18:34  reggers
 *     Made debugOn a static variable
 *     Revision 1.17  1999/09/10 20:14:11  reggers
 *     Corrected setting of glyph->ooy and glyph->oox values later used
 *     for x and y advanceWidth. 
 *     Revision 1.16  1999/08/31 19:02:41  reggers
 *     Suppress a warning in binary search of CTable
 *     Revision 1.15  1999/08/31 18:50:50  reggers
 *     Fixed underflow in binary search of Character table.
 *     Revision 1.14  1999/08/09 21:18:01  reggers
 *     Made PFRGetNameProperty not picky about nameID.
 *     Revision 1.13  1999/07/19 20:32:30  sampo
 *     Error/warning cleanup
 *     Revision 1.12  1999/07/19 19:51:34  sampo
 *     Error/warning cleanup.
 *     Revision 1.11  1999/07/19 16:58:59  sampo
 *     Change shellsort call to new name.
 *     Revision 1.10  1999/07/16 19:31:34  sampo
 *     Restructured if/else on SBIT and KERN so conditionals work.
 *     Revision 1.9  1999/07/16 15:56:38  mdewsnap
 *     Added in call to shell sort
 *     Revision 1.8  1999/07/13 21:01:58  sampo
 *     Changed declarations of Sbits functions to match prototypes, added
 *     locals to receive void *'s which are T2K *'s,
 *     Revision 1.7  1999/07/09 21:29:24  sampo
 *     For embedded bitmaps, respect the scaler->BitmapFilter setting. That
 *     is, if it is non NULL, always allocate an internal_baseAddr pointer
 *     knowing that the BitmapFilter would be the one to image into the cache.
 *     Revision 1.6  1999/07/09 21:17:28  sampo
 *     Improved bitmap metrics precision and corrected failure to scale
 *     bitmap metrics when down-sampling to grey scale from hi-res
 *     monochrome bitmap.
 *     Some performance enhancement.
 *     Revision 1.5  1999/07/02 19:04:49  sampo
 *     First pass on processing embedded bitmaps
 *     Revision 1.4  1999/06/22 15:04:23  mdewsnap
 *     Added in Kern data parsing and storage
 *     Revision 1.3  1999/06/15 20:07:36  sampo
 *     Got rid of reading winFaceName from auxdata. Only use fontID from
 *     straight PFR extra data item.
 *     Revision 1.2  1999/06/02 16:58:12  sampo
 *     Added GetPFRNameProptery
 *     Revision 1.1  1999/04/28 15:47:02  reggers
 *     Initial revision
 *                                                                           *
******************************************************************************/
