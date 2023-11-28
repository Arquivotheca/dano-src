/*
 * SPDREAD.c
 * Font Fusion Copyright (c) 1989-2000 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Robert Eggers
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

/************************** S P D R E A D . C ******************************
 *                                                                           *
 * Examine Speedo binary font file                                           *
 *                                                                           *
 ****************************************************************************/


#include <ctype.h>
#include "syshead.h"

#include "config.h"

#ifdef ENABLE_SPD
#include "dtypes.h"
#include "tsimem.h"
#include "t2kstrm.h"
#include "glyph.h"
#include "truetype.h"
#include "util.h"
#include "t2k.h"
#include "auxdata.h"
#include "spdread.h"

/***** MACRO DEFINITIONS *****/

#define DEBUG	0

#if DEBUG
static int debugOn = 1;
#endif

#define NEED_FLIP 1

#ifdef NEXT_BYTE
#undef NEXT_BYTE
#endif
#define NEXT_BYTE(A) (*(A)++)

#ifdef NEXT_WORD
#undef NEXT_WORD
#endif
#define NEXT_WORD(A) \
    ((int16)(key32 ^ ((A) += 2, ((int16)((A)[-1]) << 8) | (int16)((A)[-2]))))

#define NEXT_BYTES(A, B) \
    (((B = (uint16)(*(A)++) ^ key7) >= 248)? \
     ((uint16)(B & 0x07) << 8) + ((*(A)++) ^ key8) + 248: \
     B)


static  uint16   key32;            /* Decryption keys 3,2 combined */
static  uint8    key4;             /* Decryption key 4 */
static  uint8    key6;             /* Decryption key 6 */
static  uint8    key7;             /* Decryption key 7 */
static  uint8    key8;             /* Decryption key 8 */

static int16 read_2b_e(uint8 *pointer);
static int32 read_3b_e	(uint8 *pointer);
static void get_args(
						SPDClass *t,
						uint8   **pp1,          /* Pointer to next byte in char data */
						uint8     format,       /* Format specification of argument pair */
						point_t  *pP           /* Resulting point */
					);

static void ShellSortCmap(spdCharDir_t *pairs, int num_pair);
static uint8 BSearchCTbl ( SPDClass *t, uint16 *indexPtr, uint16 theValue);
#if NEED_FLIP
static void FlipContourDirection(GlyphClass *glyph);
#endif
static void TransformPoint(SPDClass *t, point_t *P/*input point*/, point_t *TP/*transformed point*/ );
static void SetupTrans(SPDClass *t,
				F16Dot16 xScale, 
				F16Dot16 yScale, 
				F16Dot16 xPosition, 
				F16Dot16 yPosition);
static void SetupTCB(SPDClass *t);
static short GetGlyphYMin( GlyphClass *glyph );
static short GetGlyphYMax( GlyphClass *glyph );
static void BuildSPDMetricsEtc( SPDClass *t );
#ifdef ENABLE_KERNING
static kernClass *New_spdkernClass( SPDClass *t, tsiMemObject *mem);
static kernSubTable *New_spdkernSubTable(SPDClass *t, tsiMemObject *mem);
static kernSubTable0Data *New_spdkernSubTable0Data(SPDClass *t, tsiMemObject *mem);
#endif


SPDClass *tsi_NewSPDClass( tsiMemObject *mem, InputStream *in, int32 fontNum )
{
SPDClass *t;
uint16  pub_hdr_size;         /* Public header size (bytes) */
uint16  pvt_hdr_size;         /* Private header size (bytes) */
uint8 isSPD;
uint32 tmpuint32;
uint8	tmpuint8, aBuffer[16];
int ii;
uint8   format/* , charFormat*/;
int32 offset, next_offset, size;

	UNUSED(fontNum);
	t = (SPDClass *)tsi_AllocMem( mem, sizeof( SPDClass ) );
	t->mem		= mem;
	t->fontName[0] = 0;
	t->in = in;
	t->maxPointCount = 0;
	t->advanceWidthMax = 0;
	t->ascent = t->descent = t->lineGap = 0;

	t->fontMatrix.m00 = 0x00010000;
	t->fontMatrix.m01 = 0x00000000;
	t->fontMatrix.m10 = 0x00000000;
	t->fontMatrix.m11 = 0x00010000;
	t->verticalEscapement = false;
	t->kern = NULL;
	
	/* Reset decryption keys */
	key32 = (KEY3 << 8) | KEY2;
	key4 = KEY4;
	key6 = KEY6;
	key7 = KEY7;
	key8 = KEY8;
	
	/* read any pertinent head information */
	Seek_InputStream( in, FH_FMVER );	/* seeks beginning of file for signature */
	
	Seek_InputStream( in, FH_HEDSZ );	/* seeks where 2 bytes are with public header size */
	pub_hdr_size = (uint16)ReadInt16(in);
	Seek_InputStream( in, FH_PVHSZ );
	pvt_hdr_size = 
  	  (pub_hdr_size >= (FH_PVHSZ + 2))?
 	   (uint16)ReadInt16(in):
 	   (uint16)(FH_NBYTE + 3);
	Seek_InputStream( in, FH_FMVER + 4 );
	tmpuint32 = (uint32)ReadInt32(in);
	isSPD = (uint8)(tmpuint32 == 0x0d0a0000);
	if (isSPD)
	{
		Seek_InputStream( in, FH_SFNTN);	/* find the short font name */
		ii = 0;
		do
		{
			t->fontName[ii] = ReadUnsignedByteMacro( in );
		} while (t->fontName[ii++]);
		Seek_InputStream( in, FH_NCHRF);	/* number of character indices in font */
		t->NumCharStrings = (short)ReadInt16(in);
		Seek_InputStream( in, FH_NCHRL);	/* number of characters in layout */
		t->NumLayoutChars = (uint16)ReadInt16(in);
		Seek_InputStream( in, FH_FCHRF);	/* first char index */
		t->firstCharIndex = (uint16)ReadInt16(in);
		Seek_InputStream( in, FH_NKPRS);	/* number of kerning pairs */
		t->nKernPairs = (uint16)ReadInt16(in);
		
#ifdef ENABLE_KERNING
		if (t->nKernPairs)
		{/* get 'em */
			Seek_InputStream( in, (uint32)pub_hdr_size + FH_OFFPK ); /* seek to where offset to pair data is */
			if (t->kern == NULL)
				t->kern = New_spdkernClass( t, t->mem);
		}
#else
		t->nKernPairs = 0;	/* forget 'em */
#endif

		Seek_InputStream( in, FH_CLFGS);	/* flags */
		tmpuint8 = ReadUnsignedByteMacro( in );
#if DEBUG
		if (tmpuint8 & BIT0)
			;	/* font is italic */
#endif
		t->isFixedPitch = (uint32)(tmpuint8 & BIT1);
		Seek_InputStream( in, FH_FRMCL);
		tmpuint8 = ReadUnsignedByteMacro( in );
		t->weight = (uint8)(tmpuint8 >> 4);
		Seek_InputStream( in, FH_ITANG);	/* italic angle * 256 */
		t->italicAngle = (F16Dot16)ReadInt16(in);
		t->italicAngle <<= 8;	/* now shifted to 16.16 */
		t->outlineRes = (uint16)ReadInt16(in);
		t->wordSpace = (uint16)ReadInt16(in);
		t->emSpace = (uint16)ReadInt16(in);
		t->enSpace = (uint16)ReadInt16(in);
		t->thinSpace = (uint16)ReadInt16(in);
		t->figureSpace = (uint16)ReadInt16(in);
		t->minX = (int16)ReadInt16(in);
		t->minY = (int16)ReadInt16(in);
		t->maxX = (int16)ReadInt16(in);
		t->maxY = (int16)ReadInt16(in);
		t->uline_pos = (int16)ReadInt16(in);
		t->uline_thickness = (int16)ReadInt16(in);
		if (pub_hdr_size >= (FH_METRS + 2))             /* Metrics resolution included in header? */
		{
			Seek_InputStream( in, FH_METRS);	/* metrics res */
			t->metricsRes = (uint16)ReadInt16(in);
		}
		else
			t->metricsRes = t->outlineRes;


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

		Seek_InputStream( in, pub_hdr_size );	/* seeks beginning of private header */
		t->charDirOffset = 0;
		if (pvt_hdr_size >= (FH_OFFCD + 3))
		    {
			Seek_InputStream( in, (uint32)pub_hdr_size + FH_OFFCD );
			ReadSegment(in, aBuffer, 3L);
		    t->charDirOffset = (uint32)read_3b_e(aBuffer);	/* character directory offset */
		    }
		t->pairKernOffset = 0;
		if (pvt_hdr_size >= (FH_OFFPK + 3))
		    {
			Seek_InputStream( in, (uint32)pub_hdr_size + FH_OFFPK );
			ReadSegment(in, aBuffer, 3L);
		    t->pairKernOffset = (uint32)read_3b_e(aBuffer);	/* character directory offset */
		    }
		t->charDataOffset = 0;
		if (pvt_hdr_size >= (FH_OCHRD + 3))
		    {
			Seek_InputStream( in, (uint32)pub_hdr_size + FH_OCHRD );
			ReadSegment(in, aBuffer, 3L);
		    t->charDataOffset = (uint32)read_3b_e(aBuffer);	/* character directory offset */
		    }
		/* loop through the characters, get maxPointCount, advanceWidthMax, ascent, descent, lineGap */
		Seek_InputStream( in, t->charDirOffset );
		format = ReadUnsignedByteMacro(in);		/* get the character directory format */
		/* allocate enough space for the codeToIndex table (NumLayoutChars) */
		t->spdCodeToIndex = (spdCharDir_t *)tsi_AllocMem(t->mem, t->NumLayoutChars * sizeof(spdCharDir_t));
		if (!t->spdCodeToIndex)
		{
			tsi_DeAllocMem( t->mem, (char *)t );
			return t = NULL;
		}

		/* allocate enough space for the indexToLoc table (more than for codeToIndex: NumCharStrings) */
		t->spdIndexToLoc = (spdLocDir_t *)tsi_AllocMem(t->mem, t->NumCharStrings * sizeof(spdLocDir_t));
		if (!t->spdIndexToLoc)
		{
			tsi_DeAllocMem( t->mem, (char *)t->spdCodeToIndex);
			t->spdCodeToIndex = NULL;
			tsi_DeAllocMem( t->mem, (char *)t );
			return t = NULL;
		}

/**/
		t->advanceWidthMax = 0;
		t->hmtx = New_hmtxEmptyClass( t->mem, t->NumCharStrings, t->NumCharStrings );
		for (ii = 0; ii <= t->NumCharStrings; ii++)
		{
		uint8 *p1;
		uint32 curPos;
		uint16 char_id, setWidth;
		    if (format)                         /* 3-byte character directory format? */
		    {
				ReadSegment(in, aBuffer, 3L);
		        offset = read_3b_e(aBuffer);
		        curPos = Tell_InputStream(in);
				ReadSegment(in, aBuffer, 3L);
		        next_offset = read_3b_e(aBuffer);
		    }
		    else                                /* 2-byte character directory format? */
		    {
				ReadSegment(in, aBuffer, 2L);
		        offset = (uint16)read_2b_e(aBuffer);
		        curPos = Tell_InputStream(in);
				ReadSegment(in, aBuffer, 2L);
		        next_offset = (uint16)read_2b_e(aBuffer);
		    }
			size = next_offset - offset;	/* character data size */
			if (ii < t->NumCharStrings)
			{
				if (size)
				{
					Seek_InputStream( in, (uint32)offset );
					ReadSegment(in, aBuffer, 5L);
					p1 = aBuffer;
					char_id = (uint16)NEXT_WORD(p1);
					setWidth = (uint16)NEXT_WORD(p1);               /* Read set width */
					t->hmtx->aw[ii] = setWidth;
					t->hmtx->lsb[ii] = 0;
					if (setWidth > t->advanceWidthMax)
						t->advanceWidthMax = setWidth;
					/* charFormat = NEXT_BYTE(p1) */;
					if (ii < t->NumLayoutChars)
					{
						t->spdCodeToIndex[ii].charID = char_id;
						t->spdCodeToIndex[ii].charIndex = (uint16)(ii /*+ t->firstCharIndex*/);
					}
					t->spdIndexToLoc[ii].gpsSize = (uint16)size;
					t->spdIndexToLoc[ii].gpsOffset = (uint32)offset;
					t->spdIndexToLoc[ii].charID = char_id;
				}
				else
				{/* no char data, make a nothing entry */
					char_id = 0xffff;	/* impossible BCID */
					t->hmtx->aw[ii] = 0;
					t->hmtx->lsb[ii] = 0;
					if (ii < t->NumLayoutChars)
					{
						t->spdCodeToIndex[ii].charID = char_id;
						t->spdCodeToIndex[ii].charIndex = (uint16)(ii /*+ t->firstCharIndex*/);
					}
					t->spdIndexToLoc[ii].gpsSize = (uint16)size;
					t->spdIndexToLoc[ii].gpsOffset = (uint32)offset;
					t->spdIndexToLoc[ii].charID = char_id;
				}
			}
			Seek_InputStream(in, curPos);	/* put self after chardir entry for this char */
		}
/**/

	}

	BuildSPDMetricsEtc( t );

	ShellSortCmap(t->spdCodeToIndex, (int)t->NumLayoutChars);

	return t;
}

void tsi_DeleteSPDClass( SPDClass *t )
{
	if ( t != NULL ) { /* delete any allocated space owned by us */
		if (t->spdIndexToLoc)
			tsi_DeAllocMem( t->mem, (char *)t->spdIndexToLoc );
		if (t->spdCodeToIndex)
			tsi_DeAllocMem( t->mem, (char *)t->spdCodeToIndex );
		tsi_DeAllocMem( t->mem, (char *)t );
	}
}

static void SPDBuildChar( SPDClass *t, uint16 index, int16 xpos, int16 ypos, F16Dot16 xscale, F16Dot16 yscale , int16 nesting);

GlyphClass *tsi_SPDGetGlyphByIndex( SPDClass *t, uint16 index, uint16 *aWidth, uint16 *aHeight )
{
GlyphClass *glyph;

	t->glyph = New_EmptyGlyph( t->mem, 0, 0, 0, 0 );
	t->glyph->curveType = 3;

	t->x_orus = 0;
	t->y_orus = 0;

	SPDBuildChar( t, index, 0, 0, 1L << 16, 1L << 16, 0);

	glyph = t->glyph;
	glyph->ooy[glyph->pointCount + 0] = 0;
	glyph->oox[glyph->pointCount + 0] = 0;
	
	glyph->ooy[glyph->pointCount + 1] = (short)t->awy;
	glyph->oox[glyph->pointCount + 1] = (short)t->awx;
	*aWidth = (uint16)t->awx;
	*aHeight = (uint16)t->awy;
	t->glyph = NULL;
#if NEED_FLIP
	FlipContourDirection( glyph );
#endif
	return glyph;
}

static void SPDBuildChar( SPDClass *t, uint16 index, int16 xpos, int16 ypos, F16Dot16 xscale, F16Dot16 yscale , int16 nesting)
{
GlyphClass *glyph;
InputStream *in = t->in;
uint8 *char_buffer, *p1;
size_t size;
#if DEBUG
uint16 char_id;
#endif
uint16 tmpuint16; 
int16 tmpint16;
uint8   format, tmpuint8;
int ii, jj, kk, ll, nn;
int16   count, oru, raw_oru;                /* adjusted, unadjusted oru value read from font */
uint8 zero_not_in;
uint8 zero_added;
uint8   edge;
uint8   format1, format2, format3, format_copy;
uint16  adj_factor;
int16   adj_orus;
point_t P0, P1, P2, P3;
point_t TP1, TP2, TP3;
uint16  sub_char_index;
int16 lxPos, lyPos;

	Seek_InputStream(in, t->spdIndexToLoc[index /*- t->firstCharIndex*/].gpsOffset);
	size = (size_t)t->spdIndexToLoc[index /*- t->firstCharIndex*/].gpsSize;
	char_buffer = CLIENT_MALLOC(size);
	if (char_buffer)
	{
		ReadSegment(in, char_buffer, (long)size);
		p1 = char_buffer;                       /* Point to start of character data */
#if DEBUG
		/* Read character ID */
		char_id = (uint16)NEXT_WORD(p1);
#else
		/* Skip character ID */
		p1 = p1 + 2;
#endif
		/* Read character set width */
		tmpint16 = NEXT_WORD(p1);               /* Read set width */

		if (!nesting)
		{/* outermost, parent glyph */
			if (t->verticalEscapement)
			{
				t->awx = 0;
				t->awy = t->upem;
			}
			else
			{
				t->awx = tmpint16;
				t->awy = 0;
			}
		}
		
		format = NEXT_BYTE(p1);
#if DEBUG
		if (debugOn && !nesting)
		{
			printf("\n\nCHARACTER DEFINITION\n");
			printf("--------------------\n\n");
			/* Print character index */
			printf("CHAR_INDEX = %d\n", index);
			/* Print character ID */
			printf("CHAR_ID = %d\n", char_id);
			/* Print character set width */
			printf("CHAR_SET_WIDTH = %d\n", tmpint16);
			printf("CHAR_TYPE = %s\n", (format & BIT0)? "COMPOUND": "SIMPLE");
		}
#endif

		/* Skip optional data if present */
		if (format & BIT1)                      /* Optional data in header? */
		    {
		    nn =(int)( (uint8)NEXT_BYTE(p1)); /* Read size of optional data */
		    if (nn >= 2)                        /* Size of main char data present? */
		        {
		        tmpint16 = NEXT_WORD(p1);
		        }
		    for (ii = 2; ii < nn; ii++)
		        {
#if DEBUG
		        tmpuint8 = (uint8)NEXT_BYTE(p1);
#else
				p1++;
#endif
		        }
		    }

#if DEBUG
		if (debugOn && !nesting)
		{
			printf("\n; CONTROLLED COORDINATE TABLE\n");
		}
#endif
		/* Read controlled coordinate table */
		t->no_X_orus = (format & BIT2)?  
		    (uint16)NEXT_BYTE(p1):
		    (uint16)0;

		t->no_Y_orus = (format & BIT3)?
		    (uint16)NEXT_BYTE(p1):
		    (uint16)0;

		nn = t->no_X_orus;
		ii = 0;
		for (jj = 0; ; jj++)                    /* For X and Y controlled coordinates... */
		    {
		    zero_not_in = true;
		    zero_added = false;
		    for (kk = 0; kk < nn; kk++)         /* For each coordinate in X or Y... */
		        {
		        raw_oru = NEXT_WORD(p1);        /* Read raw oru value */
		        oru = raw_oru;
		        if (zero_not_in && (raw_oru >= 0)) /* First positive oru value? */
		            {
		            if (raw_oru != 0)           /* Zero oru value omitted? */
		                {
		                t->orus[ii] = 0;           /* Insert zero value in oru array */
		                ii++;
		                zero_added = true;      /* Remember to increment size of array */
		                }
		            zero_not_in = false;        /* Inhibit further testing for zero ins */
		            }
		        /* raw_orus[ii] = raw_oru; */
		        t->orus[ii] = oru;
		        ii++;
		        }
		    if (zero_not_in)                    /* All specified oru values negative? */
		        {
		        /* raw_orus[ii] = 0; */
		        t->orus[ii] = 0;
		        ii++;
		        zero_added = true;              /* Remember to increment size of array */
		        }
		    if (jj)                             /* Both X and Y orus read? */
		        break;
		    if (zero_added)                                 
		        t->no_X_orus++;                    /* Increment X array size */
		    nn = t->no_Y_orus;                     /* Prepare to read Y oru values */
		    }
		if (zero_added)                         /* Zero Y oru value added to array? */
		    t->no_Y_orus++;                        /* Increment Y array size */
		
		t->Y_edge_org = (int16)t->no_X_orus;
#if DEBUG
		if (debugOn && !nesting)
		{
			printf("NO_X_ORUS = %d\n", t->no_X_orus);
			printf("NO_Y_ORUS = %d\n", t->no_Y_orus);
			for (ii = 0; ii < t->no_X_orus; ii++)
			    {
			    printf("%d ", t->orus[ii]);
			    }                  
			printf("\n");
			for (ii = 0; ii < t->no_Y_orus; ii++)
			    {
			    printf("%d ", t->orus[t->no_X_orus + ii]);
			    }
			printf("\n");
		/* Print control zone table */
		printf("\n; CONTROL ZONE TABLE\n");
		}
#endif


		/* Read control zone table */
		nn = t->no_X_orus - 1;
		count = 0;
		for (ii = 0; ; ii++)                    /* For X and Y control zones... */
		    {
		    for (jj = 0; jj < nn; jj++)         /* For each zone in X or Y... */
		        {
		        if (format & BIT4)              /* 1 byte from/to specification? */
		            {
		            edge = NEXT_BYTE(p1);       /* Read packed from/to spec */
		            t->controlZoneTbl[count].start_edge = (uint8)(edge & 0x0f);   /* Extract start edge */
		            t->controlZoneTbl[count].end_edge = (uint8)(edge >> 4);       /* Extract end edge */
		            }
		        else                            /* 2 byte from/to specification? */
		            {
		            t->controlZoneTbl[count].start_edge = NEXT_BYTE(p1); /* Read start edge */
		            t->controlZoneTbl[count].end_edge = NEXT_BYTE(p1);   /* read end edge */
		            }
		        t->controlZoneTbl[count++].constr_nr = (int16)NEXT_BYTES(p1, tmpuint16); /* Read constraint number */ 
#if DEBUG
				if (debugOn && !nesting)
				{
			        printf("%4d%4d%4d\n", t->controlZoneTbl[count-1].start_edge, t->controlZoneTbl[count-1].end_edge,
			        					t->controlZoneTbl[count-1].constr_nr);
				}
#endif
		        }
		    if (ii)                             /* Y pixels done? */
		        break;                                          
#if DEBUG
			if (debugOn && !nesting)
			{
			    printf("\n");
			}
#endif
		    nn = t->no_Y_orus - 1;
		    t->Y_zone_org = count;                      
		    }


		/* Read interpolation zone table */
		t->no_X_int_zones = (format & BIT6)?
		    (int16)NEXT_BYTE(p1):
		    (int16)0;
		t->no_Y_int_zones = (format & BIT7)?
		    (int16)NEXT_BYTE(p1):
		    (int16)0;
#if DEBUG
		if (debugOn && !nesting)
		{
			printf("\n; INTERPOLATION ZONE TABLE\n");
			printf("NO_X_INT_ZONES = %d\n", t->no_X_int_zones);
			printf("NO_Y_INT_ZONES = %d\n", t->no_Y_int_zones);
		}
#endif
		t->edge_org = 0;
		nn = t->no_X_int_zones;
		count = 0;
		for (jj = 0; ; jj++)                    /* Loop for X and Y dimensions */
		    {
		    for (kk = 0; kk < nn; kk++)         /* Loop for each zone in one dimension */
		        {
		        format1 = NEXT_BYTE(p1);
		        if (format1 & BIT7)             /* Short start/end point spec? */
		            {
		            tmpuint8 = (uint8)NEXT_BYTE(p1);
		            t->interpolationTable[count].start_edge = (uint8)(tmpuint8 & 0xf);
		            t->interpolationTable[count].start_adj = 0;
		            t->interpolationTable[count].end_edge = (uint8)(tmpuint8 >> 4);
		            t->interpolationTable[count].end_adj = 0;
#if DEBUG
					if (debugOn && !nesting)
					{
		            	printf("%4d    0 %4d    0\n", t->interpolationTable[count].start_edge,
		            									t->interpolationTable[count].end_edge);
					}
#endif
					count++;
		            }
		        else                            /* Standard start and end point spec? */
		            {
		            format_copy = format1;
		            for (ll = 0; ; ll++)        /* Loop for start and end point */
		                {
		                switch (format_copy & 0x7) /* Decode start/end point format */
		                    {
		                case 0:                 /* Index to control edge */
		                    edge = NEXT_BYTE(p1);
		                    if (ll == 0)
		                    {
					            t->interpolationTable[count].start_edge = edge;
					            t->interpolationTable[count].start_adj = 0;
		                    }
		                    else
		                    {
					            t->interpolationTable[count].end_edge = edge;
					            t->interpolationTable[count].end_adj = 0;
		                    }
#if DEBUG
							if (debugOn && !nesting)
							{
			                    printf("%4d    0 ", edge);
							}
#endif
		                    break;

		                case 1:                 /* 1 byte fractional distance to next edge */
		                    adj_factor = (uint16)((uint16)NEXT_BYTE(p1) << 8);
		                    goto L1;


		                case 2:                 /* 2 byte fractional distance to next edge */
		                    adj_factor = (uint16)NEXT_WORD(p1);
		                L1: edge = NEXT_BYTE(p1);
#if 1
	                        adj_orus = (int16)((((int32)t->orus[t->edge_org + edge + 1] - (int32)t->orus[t->edge_org + edge]) * 
    	                    			(uint32)adj_factor + (int32)32768) >> 16);
#else
		                    adj_orus = (int16)floor((double)(t->orus[t->edge_org + edge + 1] - t->orus[t->edge_org + edge]) * 
		                                     (double)adj_factor / 
		                                     65536.0 +
		                                     0.5);
#endif
		                    goto L4;

		                case 3:                 /* 1 byte delta orus before first edge */
		                    adj_orus = (int16)(-NEXT_BYTE(p1)); 
		                    edge = 0;
		                    goto L4;

		                case 4:                 /* 2 byte delta orus before first edge */
		                    adj_orus = (int16)-NEXT_WORD(p1);
		                    edge = 0;
		                    goto L4;

		                case 5:                 /* 1 byte delta orus after last edge */
		                    adj_orus = (int16)NEXT_BYTE(p1);
		                    edge = (uint8)(jj? t->no_Y_orus - 1: t->no_X_orus - 1);
		                    goto L4;

		                case 6:                 /* 2 byte delta orus after last edge */
		                    adj_orus = NEXT_WORD(p1);
		                    edge = (uint8)(jj? t->no_Y_orus - 1: t->no_X_orus - 1);
		                L4:
		                    if (ll == 0)
		                    {
					            t->interpolationTable[count].start_edge = edge;
					            t->interpolationTable[count].start_adj = adj_orus;
		                    }
		                    else
		                    {
					            t->interpolationTable[count].end_edge = edge;
					            t->interpolationTable[count].end_adj = adj_orus;
		                    }
#if DEBUG
							if (debugOn && !nesting)
							{
			                    printf("%4d %4d ", edge, adj_orus);
			                }
#endif
		                    break;
		                    }
		                if (ll)                 /* Second time round loop? */
		                	{
		                	count++;
		                    break;
		                	}
		                format_copy >>= 3;      /* Adj format to decode end point format */
		                }
#if DEBUG
					if (debugOn && !nesting)
					{
			            printf("\n");
					}
#endif
		            }
		   /*   ii++; */
		        }
		    if (jj)                             /* Finished? */
		        break;
#if DEBUG
			if (debugOn && !nesting)
			{
			    printf("\n");
			}
#endif
		    t->edge_org = (uint8)t->Y_edge_org;
		    nn = t->no_Y_int_zones;
		    }


		t->x_orus = 0;
		t->y_orus = 0;
		/* Print bounding box */
		format1 = NEXT_BYTE(p1);                /* Read bounding box format byte 1 */
		get_args(t, &p1, format1, &P0);
		format3 = (uint8)(((format1 >> 4) & 0x03) | 0x0c);
		get_args(t, &p1, format3, &P1);
		format2 = NEXT_BYTE(p1);                /* Read bounding box format byte 2 */
		get_args(t, &p1, format2, &P2);
		get_args(t, &p1, (uint8)(format2 >> 4), &P3);

		if ((P1.y != P0.y) ||
		    (P2.x != P1.x) ||
		    (P3.y != P2.y) ||
		    (P0.x != P3.x))
		    {
#if DEBUG
				if (debugOn && !nesting)
				{
				    printf("*** Bounding box inconsistency:\n");
				    printf("    (%d, %d), (%d, %d), (%d, %d), (%d, %d)\n",
				        P0.x, P0.y,
				        P1.x, P1.y,
				        P2.x, P2.y,
				        P3.x, P3.y);
				}
#endif
		    }
#if DEBUG
		if (debugOn && !nesting)
		{
			printf("\nBBOX %4d %4d %4d %4d\n",
			     P0.x, P0.y, P2.x, P2.y);
		}
#endif
		if (format & BIT0)
		{/* compound character */
#if DEBUG
			if (debugOn && !nesting)
			{
			    printf("\nCOMPOUND CHARACTER DATA\n"); 
			}
#endif
		    while(true)
		    {
		        format1 = NEXT_BYTE(p1);
		        if (format1 == 0)               /* End of compound character? */
		            break;

		        switch (format1 & 0x03)         /* Switch on format of X position */
		        {
		        case 1:
		            lxPos = NEXT_WORD(p1);
		            break;

		        case 2:
		            lxPos = (int16)((int8)NEXT_BYTE(p1));
		            break;

		        case 3:
		            lxPos = 0;
		            break;

		        default:
#if DEBUG
					if (debugOn && !nesting)
					{
		            	printf("*** Undefined Xpos format %2x\n", format1);
		            }
#endif
		            break;
		        }
		        switch ((format1 >> 2) & 0x03)  /* Switch on format of Y position */
		        {
		        case 1:
		            lyPos = NEXT_WORD(p1);
		            break;

		        case 2:
		            lyPos = (int16)((int8)NEXT_BYTE(p1));
		            break;

		        case 3:
		            lyPos = 0;
		            break;

		        default:
#if DEBUG
					if (debugOn && !nesting)
					{
		            	printf("*** Undefined Ypos format %2x\n", format1);
		            }
#endif
		            break;
		        }
		        xscale = (format1 & BIT4)?
		            NEXT_WORD(p1):
		            4096;

		        yscale = (format1 & BIT5)?
		            NEXT_WORD(p1):
		            4096;

		        sub_char_index = (uint16)(/*t->firstCharIndex + */
		            ((format1 & BIT6)?
		             (uint16)NEXT_WORD(p1):
		             (uint16)NEXT_BYTE(p1)));

#if DEBUG
		        if (debugOn) printf("DOCH %6d %6d %6d %4.2f %4.2f\n", 
		            sub_char_index, 
		            lxPos, 
		            lyPos, 
		            (double)xscale / 4096.0, 
		            (double)yscale / 4096.0);
#endif
				SPDBuildChar( t, sub_char_index, lxPos, lyPos, (long)xscale << 4, (long)yscale << 4, 1);
		    }
		}
		else
		{/* simple character */
/**/
	    spdtcb_t tcbSave;
		uint8   edge;
#if DEBUG
			if (debugOn && !nesting)
			{
				printf("\nCHARACTER CONTOURS\n");
			}
#endif
			t->x_orus = 0;
			t->y_orus = 0;
			glyph = t->glyph;
			tcbSave = t->tcb;					/* save the transformation control block */
			SetupTrans(t,
						xscale, 
						yscale, 
						(F16Dot16)xpos << 16, 
						(F16Dot16)ypos << 16);				/* alter t->tcb based on input params */
			t->contourOpen = false;
		    while(true)
		    {
		        format1 = NEXT_BYTE(p1);
		        switch(format1 >> 4)
		        {
		        case 0:                         /* LINE */
		            get_args(t, &p1, format1, &P1);
					TransformPoint(t, &P1, &TP1);
#if DEBUG
		            if (debugOn) printf("LINE %4d %4d\n", TP1.x, TP1.y);
#endif
		        	glyph_AddPoint( glyph, (long)TP1.x, (long)TP1.y, 1 );
		            continue;

		        case 1:                         /* Short XINT */
#if DEBUG
		            if (debugOn) printf("XINT %d\n", format1 & 0x0f);
#endif
		            continue;
		 
		        case 2:                         /* Short YINT */
#if DEBUG
		            if (debugOn) printf("YINT %d\n", format1 & 0x0f);
#endif
		            continue;
		         
		        case 3:                         /* Miscellaneous */
		            switch(format1 & 0x0f)
		            {
		            case 0:                     /* END */
#if DEBUG
		            if (debugOn) printf("END\n\n");
#endif
						if (t->contourOpen)
							{
							glyph_CloseContour( glyph );
							t->contourOpen = false;
							}
						t->tcb = tcbSave;					/* restore the transformation control block */
						CLIENT_FREE(char_buffer);
		                return;

		            case 1:                     /* Long XINT */
#if DEBUG
		                tmpuint8 = NEXT_BYTE(p1);
		            	if (debugOn) printf("XINT %d\n", (int)tmpuint8);
#else
						p1++;
#endif
		                continue;

		            case 2:                     /* Long YINT */
#if DEBUG
		                tmpuint8 = NEXT_BYTE(p1);
		            	if (debugOn) printf("YINT %d\n", (int)tmpuint8);
#else
						p1++;
#endif
		                continue;

		            default:                    /* Not used */
		                continue;
		            }    

		        case 4:                         /* MOVE Inside */
		        case 5:                         /* MOVE Outside */
					if (t->contourOpen)
						glyph_CloseContour( glyph );
		            get_args(t, &p1, format1, &P1);
					TransformPoint(t, &P1, &TP1);
#if DEBUG
		            if (debugOn) printf("\nMOVE %4d %4d\n", TP1.x, TP1.y);
#endif
		        	glyph_AddPoint( glyph, (long)TP1.x, (long)TP1.y, 1 );
					t->contourOpen = true;
		            continue;

		        case 6:                         /* VLINE or HLINE */
					if (format1 & 0x08)
					{/* VLINE */
						edge = (uint8)(t->Y_edge_org + (format1 & 0x07));
						P1.y = t->orus[edge];
					}
					else
					{/* HLINE */
						edge = (uint8)(format1 & 0x07);
						P1.x = t->orus[edge];
					}
					TransformPoint(t, &P1, &TP1);
#if DEBUG
					if (format1 & 0x08)
					{
		            	if (debugOn) printf("VLINE %4d %4d\n", TP1.x, TP1.y);
		            }
		            else
		            {
		            	if (debugOn) printf("HLINE %4d %4d\n", TP1.x, TP1.y);
		            }
#endif
		        	glyph_AddPoint( glyph, (long)TP1.x, (long)TP1.y, 1 );
		            continue;

		        case 7:                         /* 5 byte Quadrant Curve format */
					if (format1 & 0x08)
					{/* Vertical to Horizontal Quadrant */
						/* X1 unchanged */
						/* Y1 interpolated relative (type 2) */
						P1.y += (int16)((int8)NEXT_BYTE(p1));
						
						/* X2 interpolated relative (type 2) */
						/* Y2 controlled coord (TYPE 0) */
						P2.x += (int16)((int8)NEXT_BYTE(p1));
						edge = (uint8)(t->Y_edge_org + NEXT_BYTE(p1));
						P2.y = t->orus[edge];
						
						/* X3 interpolated relative (type 2) */
						/* Y3 unchanged */
						P3.x += (int16)((int8)NEXT_BYTE(p1));
					}
					else
					{/* Horizontal to Vertical Quadrant */
						/* X1 interpolated relative (type 2) */
						/* Y1 unchanged */
						P1.x += (int16)((int8)NEXT_BYTE(p1));
						
						/* X2 controlled coord (type 0) */
						/* Y2 interpolated relative (type 2) */
						edge = NEXT_BYTE(p1);
						P2.x = t->orus[edge];
						P2.y += (int16)((int8)NEXT_BYTE(p1));
						
						/* X3 unchanged */
						/* Y3 interpolated relative (type 2) */
						P3.y += (int16)((int8)NEXT_BYTE(p1));
					}
					TransformPoint(t, &P1, &TP1);
					TransformPoint(t, &P2, &TP2);
					TransformPoint(t, &P3, &TP3);
#if DEBUG
		            if (debugOn) printf("QCRVE %4d %4d %4d %4d %4d %4d\n", TP1.x, TP1.y, TP2.x, TP2.y, TP3.x, TP3.y);
#endif
		        	glyph_AddPoint( glyph, (long)TP1.x, (long)TP1.y, 0 );
		        	glyph_AddPoint( glyph, (long)TP2.x, (long)TP2.y, 0 );
		        	glyph_AddPoint( glyph, (long)TP3.x, (long)TP3.y, 1 );
		            continue;

		        default:                        /* CRVE */
		            format2 = NEXT_BYTE(p1);
		            get_args(t, &p1, format1, &P1);
					TransformPoint(t, &P1, &TP1);
		            get_args(t, &p1, format2, &P2);
					TransformPoint(t, &P2, &TP2);
		            get_args(t, &p1, (uint8)(format2 >> 4), &P3);
					TransformPoint(t, &P3, &TP3);
#if DEBUG
		            if (debugOn) printf("CRVE %4d %4d %4d %4d %4d %4d\n", TP1.x, TP1.y, TP2.x, TP2.y, TP3.x, TP3.y);
#endif
		        	glyph_AddPoint( glyph, (long)TP1.x, (long)TP1.y, 0 );
		        	glyph_AddPoint( glyph, (long)TP2.x, (long)TP2.y, 0 );
		        	glyph_AddPoint( glyph, (long)TP3.x, (long)TP3.y, 1 );
		            continue;
		        }
		    }
		}
		CLIENT_FREE(char_buffer);

	}
	return;
}

uint16 tsi_SPDGetGlyphIndex( SPDClass *t, uint16 charCode )
{
uint16 glyphIndex = 0;
uint8 found;
	found = BSearchCTbl ( t, &glyphIndex, charCode);
	t->glyphExists = (int)found;
	return glyphIndex;
}

uint8 *GetSPDNameProperty( SPDClass *t, uint16 languageID, uint16 nameID )
{
	uint8 *result, *p;
	unsigned long length;
	UNUSED(languageID);
	UNUSED(nameID);
	length = strlen((const char *)t->fontName);
	p = t->fontName;
	result = (uint8 *)tsi_AllocMem( t->mem, (unsigned long)(length + 1) );
	strcpy((char *)result, (const char *)p);
	return result;
}

static int32 read_3b_e	(
							uint8 *pointer    /* Pointer to first byte of encrypted 3-byte integer */
							)
/*
 * Reads a 3-byte encrypted integer from the byte string starting at 
 * the specified point.
 * Returns the decrypted value read as a signed integer.
 */
{
int32 tmpint32;

tmpint32 = (int32)((*pointer++) ^ key4) << 8;    /* Read middle byte */
tmpint32 += (int32)(*pointer++) << 16;           /* Read most significant byte */
tmpint32 += (int32)((*pointer) ^ key6);          /* Read least significant byte */
return tmpint32;
}

static int16 read_2b_e(uint8 *pointer)
/*
 * Reads encrypted 2-byte field from buffer 
 */
{
int32 temp;

temp = *pointer++;
temp += (*pointer) << 8;
temp ^= key32;
return (int16)temp;
}

static void get_args(
						SPDClass *t,
						uint8   **pp1,          /* Pointer to next byte in char data */
						uint8     format,       /* Format specification of argument pair */
						point_t  *pP           /* Resulting point */
					)
/*
 * The format is specified as follows:
 *     Bits 0-1: Type of X argument.
 *     Bits 2-3: Type of Y argument.
 * where the 4 possible argument types are:
 *     Type 0:   Controlled coordinate represented by one byte
 *               index into the X or Y controlled coordinate table.
 *     Type 1:   Interpolated coordinate represented by a two-byte
 *               signed integer.
 *     Type 2:   Interpolated coordinate represented by a one-byte
 *               signed increment/decrement relative to the 
 *               proceding X or Y coordinate.
 *     Type 3:   Repeat of preceding X or Y argument value and type.
 * Returns pointer to the byte following the
 * argument pair.
 */
{
uint8   edge;

/* Read X argument */
switch(format & 0x03)
    {
case 0:                                 /* Index to controlled oru */
    edge = NEXT_BYTE(*pp1);
    t->x_orus = t->orus[edge];
    break;

case 1:                                 /* 2 byte interpolated oru value */
    t->x_orus = NEXT_WORD(*pp1);
    break;

case 2:                                 /* 1 byte signed oru increment */
    t->x_orus += (int16)((int8)NEXT_BYTE(*pp1));
    break;

default:                                /* No change in X value */
    break;
    }
pP->x = t->x_orus;

/* Read Y argument */
switch((format >> 2) & 0x03)
    {
case 0:                                 /* Index to controlled oru */
    edge = NEXT_BYTE(*pp1);
    t->y_orus = t->orus[t->Y_edge_org + edge]; /* Unadjusted oru value */
    break;

case 1:                                 /* 2 byte interpolated oru value */
    t->y_orus = NEXT_WORD(*pp1);
    break;

case 2:                                 /* 1 byte signed oru increment */
    t->y_orus += (int16)((int8)NEXT_BYTE(*pp1));

default:                                /* No change in Y value */
    break;
    }
pP->y = t->y_orus;
}


static void ShellSortCmap(spdCharDir_t *pairs, int num_pair)
{
	int i, j, incr = num_pair/2;
	uint32 tempindex;
	uint16  tempvalue;

	while (incr > 0)
	{
		for (i = incr; i < num_pair; i++)
		{
			j = i - incr;
			while (j >= 0)
			{
				if (pairs[j].charID > pairs[j+incr].charID)
				{
					/* swap the two numbers */
					tempindex = pairs[j].charID;
					tempvalue = pairs[j].charIndex;
	
					pairs[j].charID = pairs[j+incr].charID;
					pairs[j].charIndex = pairs[j+incr].charIndex;
	
					pairs[j+incr].charID = (uint16)tempindex;
					pairs[j+incr].charIndex = tempvalue;
					j -= incr;
				}
				else
				{
					j = -1;
				}
			}
		}
		incr = incr/2;
	}
}

/*************************************************************************************
*	BSearchCTbl()
*	RETURNS:	true on success, false on failure.
*				If successful, *indexPtr contains index of where found.
*************************************************************************************/
static uint8 BSearchCTbl ( SPDClass *t, uint16 *indexPtr, uint16 theValue)
{
   signed long    left, right, middle;
   int16    result;
   int32 nElements = t->NumLayoutChars;

   left = 0;
   right = nElements - 1;

   while ( right >= left )
   {
      middle = (left + right)/2;
 
      if (theValue == t->spdCodeToIndex[middle].charID)
      	result = 0;
      else if (theValue < t->spdCodeToIndex[middle].charID)
      	result = -1;
      else
      	result = 1;
      	
      if ( result == -1 )
         right = middle - 1;
      else
         left = middle + 1;
      if ( result == 0 )
      {
         *indexPtr = (uint16)t->spdCodeToIndex[middle].charIndex;
         return ( true );
      }
   }
   return ( false );
}


static void SetupTCB(SPDClass *t)
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

static void SetupTrans(SPDClass *t,
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

static void TransformPoint(SPDClass *t, point_t *P/*input point*/, point_t *TP/*transformed point*/ )
{
F16Dot16 x,  y;
F16Dot16   xt, yt;
	x = (F16Dot16)P->x << 16;
	y = (F16Dot16)P->y << 16;
	xt =
			util_FixMul(x, t->tcb.m00) +
			util_FixMul(y, t->tcb.m10);
	yt =
			util_FixMul(x, t->tcb.m01) +
			util_FixMul(y, t->tcb.m11);
 
	TP->x = (int16)((xt + t->tcb.xOffset) >> 16);
	TP->y = (int16)((yt + t->tcb.yOffset) >> 16);
	return;
}

#if NEED_FLIP
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
#endif

static void BuildSPDMetricsEtc( SPDClass *t )
{
register uint16 gIndex;
	GlyphClass *glyph;
	uint16 aw, ah;
	
	t->maxPointCount = 0;
	t->ascent = 0x7fff;
	t->descent = -0x7fff;
	t->lineGap = 0x7fff;
	
	gIndex = tsi_SPDGetGlyphIndex( t, (uint16) 'f' );
	if (gIndex)
	{
		glyph = tsi_SPDGetGlyphByIndex( t, gIndex, &aw, &ah);
		t->ascent = GetGlyphYMax( glyph );
		Delete_GlyphClass( glyph );
	}
	gIndex = tsi_SPDGetGlyphIndex( t, (uint16) 'g' );
	if (gIndex)
	{
		glyph = tsi_SPDGetGlyphByIndex( t, gIndex, &aw, &ah);
		t->descent = GetGlyphYMin( glyph );
		Delete_GlyphClass( glyph );
	}
	t->maxPointCount = 32;

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

/**/
#ifdef ENABLE_KERNING

static kernSubTable0Data *New_spdkernSubTable0Data(SPDClass *t, tsiMemObject *mem)
{
int ii;
uint16  pair_count = t->nKernPairs;
InputStream *in = t->in;
int16   max_adj;                /* Maximum adjustment (1/256 points) */
uint16  char1_index;
uint16  char2_index;
int16   adj;
uint8	*p1;
uint8 format;
int32 offset;
uint8 aBuffer[16];

	kernSubTable0Data *ksub0 = (kernSubTable0Data *) tsi_AllocMem( mem, sizeof( kernSubTable0Data ) );


	ksub0->mem				= mem;

	ksub0->pairs			= (kernPair0Struct*) tsi_AllocMem( mem, pair_count * sizeof(kernPair0Struct) );
	ksub0->nPairs = pair_count;

	ksub0->searchRange		= 0;
	ksub0->entrySelector	= 0;
	ksub0->rangeShift		= 0;
	
#if DEBUG
	if (debugOn)
	{
		printf("\n\nPAIR KERNING DATA\n");
		printf("-----------------\n\n");
		printf ("NO_KERNING_PAIRS = %d\n", t->nKernPairs);

		printf ("\nCHAR 1   CHAR 2      ADJ\n");
	}
#endif

	ReadSegment(in, aBuffer, 3L);
    offset = read_3b_e(aBuffer);	/* kern data offset */
	Seek_InputStream( in, (uint32)offset ); /* seek to where pair data is */
	format = ReadUnsignedByteMacro(in);         /* Read pair kerning data format byte */
	if (format & BIT0)
		max_adj = 0;
	else
	{
		ReadSegment(in, aBuffer, 2L);
		p1 = aBuffer;
		max_adj = NEXT_WORD(p1);
	}

	for ( ii = 0; ii < pair_count; ii++ ) 
	{		
		if (format & BIT1)
		{
			ReadSegment(in, aBuffer, 4L);
			p1 = aBuffer;
			char1_index = (uint16)NEXT_WORD(p1);
			char2_index = (uint16)NEXT_WORD(p1);
		}
		else
		{
			ReadSegment(in, aBuffer, 2L);
			p1 = aBuffer;
			char1_index = NEXT_BYTE(p1);
			char2_index = NEXT_BYTE(p1);
		}
		ksub0->pairs[ii].leftRightIndex = (uint32)((((uint32)char1_index << 16) | (uint32)char2_index));

		if (format & BIT0)
	    {
			ReadSegment(in, aBuffer, 2L);
			p1 = aBuffer;
			adj = NEXT_WORD(p1);
	    }
	    else
	    {
			ReadSegment(in, aBuffer, 1L);
			p1 = aBuffer;
			adj = (int16)(max_adj + (int16)((int8)NEXT_BYTE(p1)));
	    }
	    ksub0->pairs[ii].value  =  adj;
#if DEBUG
	    if (debugOn) printf("%6d   %6d   %6d\n", char1_index, char2_index, adj);
#endif

	}

#if 0
/* this should never have to be sorted, Speedo has them in correct sort order! */
	ff_KernShellSort(ksub0->pairs, ksub0->nPairs);
#endif
	
	return ksub0; /*****/
}


static kernSubTable *New_spdkernSubTable(SPDClass *t, tsiMemObject *mem)
{
	kernSubTable *ksub = (kernSubTable *) tsi_AllocMem( mem, sizeof( kernSubTable ) );

	ksub->mem			= mem;
	
	ksub->version		= (uint16)0;
	ksub->length		= (uint16)sizeof(kernSubTable);
	ksub->coverage		= (uint16)1;	
	
	ksub->kernData			= NULL;
	if ( ksub->version == 0 && ksub->length > 0 ) {
		ksub->kernData			= New_spdkernSubTable0Data( t, mem);
	}
	
	return ksub; /*****/
}

static kernClass *New_spdkernClass( SPDClass *t, tsiMemObject *mem)
{
	
	kernClass *kClass = (kernClass *) tsi_AllocMem( mem, sizeof( kernClass ) );
	kClass->mem			= mem;
	kClass->version		= 0;
	kClass->nTables		= 1;
	
	kClass->table		= (kernSubTable **) tsi_AllocMem( mem, sizeof( kernSubTable * ) );

	kClass->table[0] = New_spdkernSubTable( t, mem);
	
	return kClass; /*****/
}
#endif /* ENABLE_KERNING */
/**/

void tsi_SPDListChars(void *userArg, SPDClass *t, void *ctxPtr, int ListCharsFn(void *userArg, void *p, uint16 code))
{
int ii, checkStop = 0;
	for (ii = 0; !checkStop && (ii < t->NumLayoutChars); ii++)
	{
		if (t->spdCodeToIndex[ii].charID != 0xffff)
			checkStop = ListCharsFn(userArg, ctxPtr, t->spdCodeToIndex[ii].charID);
	}
}

#endif /* ENABLE_SPD */


/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 * 01/21/00 R. Eggers Created                                                *
 *     $Header: R:/src/FontFusion/Source/Core/rcs/spdread.c 1.14 2001/05/01 18:29:12 reggers Exp reggers $
 *                                                                           *
 *     $Log: spdread.c $
 *     Revision 1.14  2001/05/01 18:29:12  reggers
 *     Added support for GlyphExists().
 *     Fixed SPD's with non-zero firstCharIndex.
 *     Revision 1.12  2000/06/16 18:38:14  reggers
 *     Warnings cleanup.
 *     Revision 1.11  2000/05/31 15:42:58  reggers
 *     Got rid of some comments embedded in comments.
 *     Revision 1.10  2000/05/25 20:35:14  reggers
 *     Fixes for 2 byte int builds.
 *     Revision 1.9  2000/05/17 19:33:03  reggers
 *     Removed dead, unreachable code.
 *     Revision 1.8  2000/05/09 17:39:13  reggers
 *     Create hmtx class, and initialize.
 *     Revision 1.7  2000/05/08 18:54:54  reggers
 *     Faster font opening: don't churn all chars for metrics.
 *     Revision 1.6  2000/04/13 18:14:05  reggers
 *     Updated list chars for user argument or context pass through.
 *     Revision 1.5  2000/03/27 20:54:34  reggers
 *     Bracketed whole file with ENABLE_SPD
 *     Revision 1.4  2000/03/10 19:18:12  reggers
 *     Enhanced for enumeration of character codes in font.
 *     Revision 1.3  2000/02/25 17:45:59  reggers
 *     STRICT warning cleanup.
 *     Revision 1.2  2000/02/18 19:52:10  reggers
 *     Warning cleanup- MS Studio
 *                                                                           *
******************************************************************************/

