/*
 * pclread.c
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
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
#include <ctype.h>
#include "syshead.h"
#include "config.h"

#ifdef ENABLE_PCL
#include "dtypes.h"
#include "tsimem.h"
#include "t2kstrm.h"
#include "glyph.h"
#include "truetype.h"
#include "util.h"
#include "t2k.h"

#define NEED_FLIP	1

#if DEBUG
static int debugOn = 1;
#endif

#if DEBUG
#define LOG_CMD( s, d ) if (debugOn) printf("%s %d\n", s, d )
#else
#define LOG_CMD( s, d ) NULL
#endif

static int16 escape_seq(InputStream *in, F16Dot16 *arg);
static char load_header(InputStream *in, PC5HEAD *pcl5head);
static char PCLBuildChar(PCLClass *t, uint8 *p,
					int16   comp_xoff     /* X origin of compound character component */,
					int16   comp_yoff     /* Y origin of compound character component */);

static short GetGlyphYMax( GlyphClass *glyph );
static short GetGlyphYMin( GlyphClass *glyph );

static int16 eo_inq_compchar(uint8 *pchar);
static int16 eo_get_swidth(uint8 *pchar);
static char eo_proc_outl_data(PCLClass *t, 
					uint8 *pChar,
					int16   comp_xoff     /* X origin of compound character component */,
					int16   comp_yoff     /* Y origin of compound character component */);
static char eo_get_component(uint8  *pchar, int16 nn, int16 *index, int16 *xoff, int16 *yoff);
static void mbyte(char *source, char *dest, int32 count);

#if NEED_FLIP
static void FlipContourDirection(GlyphClass *glyph);
#endif


PCLClass *tsi_NewPCLClass( tsiMemObject *mem, InputStream *in, int32 fontNum )
{
char isPCL;
PCLClass *t = NULL;
char ok;
F16Dot16 arg;
/* char    buffer[4096]; */
int32 curpos;
/* int16   charID; */
int16   chsize;
int16	code;
uint8   aBuffer[10];
int16 temp16;
	UNUSED(fontNum);
	/* read any pertinent head information */
	Seek_InputStream( in, 0 );
	ReadSegment( in, (uint8 *)&aBuffer[0], 3 );
	isPCL = (char)(((aBuffer[0] == 0x1b) && (aBuffer[1] == 0x29) && (aBuffer[2] == 0x73) ));
	if (isPCL)
	{/* ensure font is *not* pcletto font: */
		do
		{
			aBuffer[0] = ReadUnsignedByteMacro(in);
		} while (aBuffer[0] != 'W');
		temp16 = ReadInt16(in);
		isPCL = (char)(temp16 != 72);	/* if (temp16 == 72) then it is pcletto */
	}
	Seek_InputStream( in, 0 );

	if (isPCL)
	{
		Seek_InputStream( in, 0 );
		t = (PCLClass *)tsi_AllocMem( mem, sizeof( PCLClass ) );
		if (t)
		{
			t->mem		= mem;
			t->in = in;
			t->NumCharStrings = 0;		/* accrued OK */
			t->maxPointCount = 0;		/* BOB!!! MUST accrue!!! */
			t->verticalEscapement = false;	/* until further notice */
			ok = true;
			while (ok)
		    {
				code = escape_seq(in, &arg);
				switch (code)
		        {
		        case ESC_FNDESC:
					curpos = Tell_InputStream( in );
					load_header(in, &t->pcl5head);
		        	Seek_InputStream( in, curpos + (arg >> 16));
		            break;
		        case ESC_CCHARID:
					/* charID = (int16)(arg >> 16); */
		            break;
		        case ESC_CHDESC:
					chsize = (int16)(arg >> 16);
					Seek_InputStream( in, in->pos + chsize );
		            break;
		        case NO_ESCAPE:
		            printf("Escape character not found\n");
		            goto abort;
				/*  break;	 */
		        case ESC_EOF:
		            ok = false;
		            break;
		        case ESC_UNKN:
		        default:
		            printf("Unrecognized escape sequence\n");
		            goto abort;
		        /*	break; */
		        }
			}
		}
	}
	t->firstCharCode = t->pcl5head.first_code;
	t->lastCharCode = t->pcl5head.last_code;
	t->NumCharStrings = (short)(t->lastCharCode - t->firstCharCode + 1); /* until we're told different */
	t->upem = t->outlineRes = t->metricsRes = t->pcl5head.scale_factor;
	t->italicAngle = t->pcl5head.italic_angle;
	t->eo_baseline =       (int16)(((int32)BASELINE * (int32)t->upem) / (int32)MASTER_RESOLUTION);
	t->eo_left_reference = (int16)(((int32)LEFT_REFERENCE * (int32)t->upem) / (int32)MASTER_RESOLUTION);

	t->maxPointCount = 0;
	t->ascent = 0x7fff;
	t->descent = -0x7fff;
	t->lineGap = 0x7fff;
	t->advanceWidthMax = 0x7fff;
	return t;

abort:
	tsi_DeAllocMem( t->mem, (char *)t );
	return(NULL);
}

void tsi_DeletePCLClass( PCLClass *t )
{
	if ( t != NULL )
	{
		tsi_DeAllocMem( t->mem, (char *)t );
	}
}

static int16 eo_get_swidth(uint8 *pchar)
{
	int16  nn;

	if (eo_inq_compchar(pchar))             /* Compound character? */
    {
		nn = (int16)(GET_WORD((pchar + 4)));
		return(nn);
    }
	else                                    /* Simple character? */
    {
		nn = (int16)(GET_WORD((pchar + 6)));
		nn = (int16)(GET_WORD((pchar + nn + 16)) - GET_WORD((pchar + nn + 12)));
		return(nn);
    }
}

GlyphClass *tsi_PCLGetGlyphByIndex( PCLClass *t, uint16 index, uint16 *aWidth, uint16 *aHeight )
{
	uint16 byteCount, charCode, nothing, setWidth;
	short limit;
	uint8 *p;
	GlyphClass *glyph;
	limit = t->NumCharStrings;
	t->glyph = New_EmptyGlyph( t->mem, 0, 0, 0, 0 );
	t->glyph->curveType = 3;
	if ( index < limit )
	{
		eo_get_char_data(	(long) index,
						0x0008/* T2K_CODE_IS_GINDEX */,
						(uint8 **)&p,
						(uint16 *)&byteCount,
						(uint16 *)&charCode,
						(uint16 *)&nothing);
		if ( p != NULL )
		{
			/* LOG_CMD( "tsi_PCLGetGlyphByIndex:", index ); */
			PCLBuildChar( t, p, 0, 0 );
			setWidth = eo_get_swidth(p);
			if (t->verticalEscapement)
			{
				t->awx = 0;
				t->awy = setWidth;
			}
			else
			{
				t->awx = setWidth;
				t->awy = 0;
			}
			
			if ( t->glyph->contourCount == 0 || t->contourOpen)
			{
				glyph_CloseContour( t->glyph );
				t->contourOpen = false;
			}
		}
	}
	else
	{/* manufacture a space character: */
	uint16 spaceWidth;
		if (t->pcl5head.pitch)
			spaceWidth = t->pcl5head.pitch;
		else
			spaceWidth = (uint16)(t->upem/4);
		if (t->verticalEscapement)
		{
			t->awx = 0;
			t->awy = spaceWidth;
		}
		else
		{
			t->awx = spaceWidth;
			t->awy = 0;
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
#if NEED_FLIP
	FlipContourDirection( glyph );
#endif
	return glyph; /*****/
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

uint16 tsi_PCLGetGlyphIndex( PCLClass *t, uint16 charCode )
{
uint16 nothing, gIndex;
uint16 byteCount;
uint8 *p;
	UNUSED (t);
	eo_get_char_data(	(long) charCode,
						0x0000/* ! T2K_CODE_IS_GINDEX */,
						(uint8 **)&p,
						(uint16 *)&byteCount,
						(uint16 *)&nothing,
						(uint16 *)&gIndex);
	return gIndex;
}

#if 0
char *styleNames[] =
{
	{" Ultra-Thin"},		/* 0 */
	{" Extra-Thin"},		/* 1 */
	{" Thin"},			/* 2 */
	{" Extra-Light"},	/* 3 */
	{" Light"},			/* 4 */
	{" Demi-Light"},		/* 5 */
	{" Semi-Light"},		/* 6 */
	{""},				/* 7 */
	{" Semi-Bold"},		/* 8 */
	{" Demi-Bold"},		/* 9 */
	{" Bold"},			/* 10 */
	{" Extra-Bold"},		/* 11 */
	{" Black"},			/* 12 */
	{" Extra-Black"},	/* 13 */
	{" Ultra-Black"},	/* 14 */
};
#endif

uint8 *GetPCLNameProperty( PCLClass *t, uint16 languageID, uint16 nameID )
{
	uint8 *result, *p;
	unsigned long length;
	UNUSED(languageID);
	UNUSED(nameID);
	length = strlen((const char *)t->pcl5head.fontname);
	p = (uint8 *)&t->pcl5head.fontname[0];
#if 0
	length += strlen(styleNames[t->pcl5head.stroke_weight + 7]);
#endif
	result = (uint8 *)tsi_AllocMem( t->mem, (unsigned long)(length + 1) );
	strcpy((char *)result, (const char *)p);
#if 0
	strcat((char *)result, (const char *)styleNames[t->pcl5head.stroke_weight + 7]);
#endif
	return result;
}

static char is_num(char c);

static char is_num(char c)
{
if (c >= '0'  &&  c <= '9')
    return(true);
else if (c == '-')
    return(true);
else if (c == '.')
    return(true);

return(false);
}

static void mbyte(char *source, char *dest, int32 count)

/*  MBYTE (SOURCE, DEST, COUNT)
 *
 *  Arguments:
 *    source - address of source array
 *    dest - address of destination array
 *    count - number of bytes to be moved
 *  Returns:  nothing
 *
 *  Moves 'count' bytes from the source array to the destination array
 *  Error handling:
 *    If 'count' less than zero, does nothing
 *    Will crash if 'source', or 'dest' don't point to valid memory location
 */

{
int32   i;
char   *sarr, *darr;

    
if (count <= 0)
    return;

if (source > dest)
    {
    for (i=0; i<count; i++)
        *dest++ = *source++;
    }
else
    {
    sarr = (char *) (source + count);
    darr = (char *) (dest + count);
    for (i=0; i<count; i++)
        *(--darr) = *(--sarr);
    }

return;
}

#define TEN16DOT16	0x000a0000

static int16 escape_seq(InputStream *in, F16Dot16 *arg)
/* Moves file pointer beyond the escape sequence, if there is one.
   If there is no escape code in the first character, or if the escape
   sequence is unrecognized, the file pointer is left unchanged. */

{
char    buf[4];
int16     code;
uint32   curpos;
int16     dpt;
F16Dot16    ftemp;
char    minus;


/* curpos = fseek(fd, 0L, 1); */
curpos = Tell_InputStream( in );
/*if (fread (buf, 1, 1, fd) != 1) */
/*    return (ESC_EOF); */
if (in->pos >= in->maxPos)
    return (ESC_EOF);
	
ReadSegment( in, (uint8 *)buf, (long)1 );
if (*buf != 0x1B)
    {
/*    fseek(fd, curpos, 0); */
	Seek_InputStream( in, curpos );
    return (NO_ESCAPE);
    }

/*fread (buf, 1, 2, fd);   */                   /* look for "*c", "(s", or ")s" */
ReadSegment( in, (uint8 *)buf, (long)2 );     /* look for "*c", "(s", or ")s" */
if (strncmp( "*c", buf, 2 ) == 0)
    code = ESC_CCHARID;
else if (strncmp( ")s", buf, 2 ) == 0)
    code = ESC_FNDESC;
else if (strncmp( "(s", buf, 2 ) == 0)
    code = ESC_CHDESC;
else
    {
/*    fseek(fd, curpos, 0); */
	Seek_InputStream( in, curpos );
    return (ESC_UNKN);
    }

/*fread (buf, 1, 1, fd); */
ReadSegment( in, (uint8 *)buf, (long)1 );
for (*arg=0x00000000, minus=false, dpt=0; is_num(*buf); ReadSegment( in, (uint8 *)buf, (long)1 ))
    {
    if (*buf == '-')
        minus = true;
    else if (*buf == '.')
        dpt = 1;
    else
        {
        if (!dpt)
		{
			*arg = util_FixMul(*arg, TEN16DOT16) + (((F16Dot16)*buf - '0') << 16);
		}
        else
            {
			ftemp = util_FixDiv(((F16Dot16)*buf << 16), TEN16DOT16 * dpt);
            *arg |= ftemp;
            dpt++;
            }
        }
    }
if (minus)
	*arg = -*arg;
if (code == ESC_CCHARID  &&  *buf == 'E')
    return (code);
else if ((code == ESC_FNDESC  ||  code == ESC_CHDESC)  &&  *buf == 'W')
    return (code);
else
    {
/*    fseek(fd, curpos, 0); */
	Seek_InputStream( in, curpos );
    return (ESC_UNKN);
    }
}

static char load_header(InputStream *in, PC5HEAD *pcl5head)
{
uint16  temp16, temp16_1, temp16_2;
uint8	buffer[32];
int32 basePos = Tell_InputStream(in);

pcl5head->size = ReadInt16( in );
Seek_InputStream(in, basePos + 2);
pcl5head->dformat = ReadUnsignedByteMacro(in);
pcl5head->font_type = ReadUnsignedByteMacro(in);

temp16_1 = ReadUnsignedByteMacro(in);
Seek_InputStream(in, basePos + 23);
temp16_2 = ReadUnsignedByteMacro(in);

temp16 = (uint16)((temp16_1 << 8) | temp16_2);

pcl5head->style.structure = (uint8)((temp16 & 0x3E0) >> 5);
pcl5head->style.width = (uint8)((temp16 & 0x1C) >> 2);
pcl5head->style.posture = (uint8)(temp16 & 3);

Seek_InputStream(in, basePos + 6);
pcl5head->baseline = ReadInt16(in);
Seek_InputStream(in, basePos + 8);
pcl5head->cell_width = ReadInt16(in);
Seek_InputStream(in, basePos + 10);
pcl5head->cell_height = ReadInt16(in);

Seek_InputStream(in, basePos + 12);
pcl5head->orientation = ReadUnsignedByteMacro(in);
Seek_InputStream(in, basePos + 13);
pcl5head->spacing = ReadUnsignedByteMacro(in);

Seek_InputStream(in, basePos + 14);
pcl5head->symbol_set = ReadInt16(in);
Seek_InputStream(in, basePos + 16);
pcl5head->pitch = ReadInt16(in);
Seek_InputStream(in, basePos + 18);
pcl5head->height = ReadInt16(in);
Seek_InputStream(in, basePos + 20);
pcl5head->x_height = ReadInt16(in);

Seek_InputStream(in, basePos + 22);
pcl5head->width_type = ReadUnsignedByteMacro(in);
Seek_InputStream(in, basePos + 24);
pcl5head->stroke_weight = ReadUnsignedByteMacro(in);

Seek_InputStream(in, basePos + 25);
temp16_1 = ReadUnsignedByteMacro(in);
temp16_2 = ReadUnsignedByteMacro(in);
temp16 = (uint16)((temp16_2 << 8) | temp16_1);
pcl5head->typeface.vendor = (uint8)((temp16 & 0x7800) >> 11);
pcl5head->typeface.version = (uint8)((temp16 & 0x0600) >> 9);
pcl5head->typeface.value = (uint16)(temp16 & 0x01FF);

Seek_InputStream(in, basePos + 27);
pcl5head->serif_style = ReadUnsignedByteMacro(in);

/*Seek_InputStream(in, basePos + 28);*/
pcl5head->quality = ReadUnsignedByteMacro(in);

/*Seek_InputStream(in, basePos + 29);*/
pcl5head->placement = ReadUnsignedByteMacro(in);

/*Seek_InputStream(in, basePos + 30);*/
pcl5head->uline_dist = ReadUnsignedByteMacro(in);

/*Seek_InputStream(in, basePos + 31);*/
pcl5head->uline_height = ReadUnsignedByteMacro(in);

/*Seek_InputStream(in, basePos + 32);*/
pcl5head->text_height = ReadInt16(in);
/*Seek_InputStream(in, basePos + 34);*/
pcl5head->text_width = ReadInt16(in);
/*Seek_InputStream(in, basePos + 36);*/
pcl5head->first_code = ReadInt16(in);
/*Seek_InputStream(in, basePos + 38);*/
pcl5head->last_code = ReadInt16(in);
/*Seek_InputStream(in, basePos + 40);*/
pcl5head->pitch_extend = ReadUnsignedByteMacro(in);
/*Seek_InputStream(in, basePos + 41);*/
pcl5head->height_extend = ReadUnsignedByteMacro(in);
/*Seek_InputStream(in, basePos + 42);*/
pcl5head->cap_height = ReadInt16(in);
/*Seek_InputStream(in, basePos + 44);*/
pcl5head->font_vendor_code = ReadUnsignedByteMacro(in);

/*Seek_InputStream(in, basePos + 45);*/
temp16_1 = ReadUnsignedByteMacro(in);
temp16_2 = ReadInt16(in);
pcl5head->font_number = temp16_2 + (temp16_1 << 16);

/**/
ReadSegment( in, (uint8 *)&buffer[0], (long)16 );
mbyte ((char *)&buffer[0], pcl5head->fontname, (int32)16);

Seek_InputStream(in, basePos + 64);
pcl5head->scale_factor = ReadInt16(in);
/*Seek_InputStream(in, basePos + 66);*/
pcl5head->x_resolution = ReadInt16(in);
/*Seek_InputStream(in, basePos + 68);*/
pcl5head->y_resolution = ReadInt16(in);
/*Seek_InputStream(in, basePos + 70);*/
pcl5head->mstr_uline_pos = (int16)ReadInt16(in);
/*Seek_InputStream(in, basePos + 72);*/
pcl5head->mstr_uline_hght = ReadInt16(in);
/*Seek_InputStream(in, basePos + 74);*/
pcl5head->threshold = ReadInt16(in);
/*Seek_InputStream(in, basePos + 76);*/
pcl5head->italic_angle = (int16)ReadInt16(in);
/*Seek_InputStream(in, basePos + 78);*/
pcl5head->scal_data_size = ReadInt16(in);

Seek_InputStream(in, basePos + 80 + pcl5head->scal_data_size);
pcl5head->nobtf = ReadInt16(in);
/* pcl5head->copyright = (char *)CLIENT_MALLOC(pcl5head->nobtf - 2); */
Seek_InputStream(in, basePos + 80 + pcl5head->scal_data_size + pcl5head->nobtf);
pcl5head->checksum = (uint8)ReadInt16(in);
return(true);
}

void BuildPCLMetricsEtc( PCLClass *t, uint16 NumCharStrings )
{
register uint16 gIndex;
	uint16 byteCount, charCode, nothing;
	long maxAW;
	int16 setWidth;
	uint8 *p;
	
	t->NumCharStrings = NumCharStrings;
		
	maxAW = 0;
	for ( gIndex = 0; gIndex < t->NumCharStrings; gIndex++ )
		{
		eo_get_char_data(	(long) gIndex,
										0x0008/* T2K_CODE_IS_GINDEX */,
										(uint8 **)&p,
										(uint16 *)&byteCount,
										(uint16 *)&charCode,
										(uint16 *)&nothing);
		t->glyph = New_EmptyGlyph( t->mem, 0, 0, 0, 0 );
		t->glyph->curveType = 3;
		PCLBuildChar( t, p, 0, 0 );
		if ( t->glyph->contourCount == 0 || t->contourOpen)
		{
			glyph_CloseContour( t->glyph );
			t->contourOpen = false;
		}
		/* get the set width: */
		setWidth = eo_get_swidth(p);
		t->lsbx = 0;
		if (t->verticalEscapement)
		{
			t->awx = 0;
			t->awy = setWidth;
		}
		else
		{
			t->awx = setWidth;
			t->awy = 0;
		}
		if ( t->ascent == 0x7fff && charCode == 'f' )
			{
			t->ascent = GetGlyphYMax( t->glyph );
			}
		if ( t->descent == -0x7fff && charCode == 'g' )
			{
			t->descent = GetGlyphYMin( t->glyph );
			}
		if ( t->advanceWidthMax == 0x7fff && setWidth > maxAW )
			{
			maxAW = setWidth;
			}
		if ( t->glyph->pointCount > t->maxPointCount )
			{
			t->maxPointCount = t->glyph->pointCount;
			}
		Delete_GlyphClass( t->glyph );
		}
	if (t->advanceWidthMax == 0x7fff)
		t->advanceWidthMax = maxAW;

	if ( t->ascent == 0x7fff )  t->ascent  =  t->upem * 3 / 4;
	if ( t->descent == -0x7fff ) t->descent = -(t->upem / 4);
	if ( t->lineGap == 0x7fff ) t->lineGap = t->upem - (t->ascent - t->descent);
	if ( t->lineGap < 0 ) t->lineGap = 0;
	

}



static char eo_get_component(uint8  *pchar, int16 nn, int16 *index, int16 *xoff, int16 *yoff)

/*  Returns: false if error (i is out of bounds, or pchar != compound)
    Output:  index, x- and y-offsets */

{
	if (pchar[0] != 10  ||  pchar[1] != 0  ||  pchar[2] != 2  ||  pchar[3] != 4)
		return(false);

	if (nn >= pchar[6])
		return(false);

	*index = (int16)GET_WORD((pchar + 6*nn + 8));
	*xoff = (int16)GET_WORD((pchar + 6*nn + 10));
	*yoff = (int16)GET_WORD((pchar + 6*nn + 12));

	return(true);

}

static int16 eo_inq_compchar(uint8 *pchar)
{
	if (pchar == NULL)
		return(-1);

	if (pchar[0] != 10  ||  pchar[1] != 0  ||  pchar[2] != 2)
		return(-1);

	if (pchar[3] == 3)
		return(0);
	else
	{
		if (pchar[3] == 4)
			return((int16) pchar[6]);
		else
			return(-1);
	}
}

static char PCLBuildChar(PCLClass *t, uint8 *p,
					int16   comp_xoff     /* X origin of compound character component */,
					int16   comp_yoff     /* Y origin of compound character component */)

/*  Returns: nothing
    This function generates a character from a PCL5 downloaded font. */

{
	int16   ii, nn;
	int16   comp_index;    /* Index of compound character component */
/*	int16   errNum; */

	eo_char_t  *pchar;
	eo_char_t  *pchar_sub;
	uint16 byteCount;
	uint16 nada, nothing;
/* get address of character */
	pchar = (eo_char_t *)p;
	if (!pchar)
    {
		/* errNum = EEO_DOES_NOT_EXIST; */
		goto mcerror;
    }

	nn = eo_inq_compchar(pchar);

	if (nn == 0)                            /* Simple character? */
    {
		if (!eo_proc_outl_data(t, pchar, comp_xoff, comp_yoff)) /* Process outline data; error? */
			goto mcerror;
		return true;
	}
	else
	{
		if (nn > 0)                        /* Compound character? */
		{
			pchar_sub = NULL;			
			for (ii = 0; ii < nn; ii++)     /* For each component of character... */
			{
				if (eo_get_component(pchar, ii, &comp_index, &comp_xoff, &comp_yoff)) /* Get component origin */
				{
					/* comp_index is a high level glyph code */
					comp_index = tsi_PCLGetGlyphIndex( t, comp_index );
					eo_get_char_data(	(long) comp_index,
										0x0008/* T2K_CODE_IS_GINDEX */,
										(uint8 **)&pchar_sub,
										(uint16 *)&byteCount,
										(uint16 *)&nada,
										(uint16 *)&nothing);
					if (pchar_sub != NULL)  /* Valid component? */
					{
						/* LOG_CMD( "PCLBuild(Sub)Char:", comp_index ); */
						if (eo_inq_compchar(pchar_sub) == 0) /* Component is simple character? */
						{
							PCLBuildChar( t, pchar_sub, comp_xoff, comp_yoff );
						}
						else                /* Component is compound character? */
						{
							/* errNum = EEO_NESTED_COMP_CHAR; */
							goto mcerror;
						}
					}
					else                    /* Invalid component character? */
					{
						/* errNum = EEO_ILL_COMP_CHAR_REF; */
						goto mcerror;
					}
				}
				else                        /* Cannot access component character? */
				{
					/* errNum = EEO_ILL_COMP_SUBCHAR; */
					goto mcerror;
				}
			}
			return(true);                       /*   the whole process */
		}
		else
		{
			/* errNum = EEO_NOT_A_PCL5_CHAR; */
			goto mcerror;
		}
	}

mcerror:

/* sp_report_error (PARAMS2 (int16)errNum); */

	return(false);

}


static char eo_proc_outl_data(PCLClass *t, 
					uint8 *pChar,
					int16   comp_xoff     /* X origin of compound character component */,
					int16   comp_yoff     /* Y origin of compound character component */)
/*  
 * Processes all contours in the specified character.
 * Returns true if no error, else false
 * Outside curves are processed counterclockwise
 * Inside curves are processed clocksise
 */
{
	int16    ii, jj;
	uint8   *pointer;
	uint16   ContourTreeOffset;     /* Offset to char outline data contour tree */
	uint8   *pCharOutlineData;      /* Pointer to origin of char outl data contour tree */
	uint16   ContourFlag;           /* Contour flag indicating alt format */
	int16    nContour;              /* Number of contours in character */
	uint8   *pContourTreeData;      /* Pointer to node data for current contour */
	uint16   XYCoordDataOffset;     /* Offset to XY coordinate data for current contour */
	int16    nPoint;                /* Number of points in current contour */
	int16    nAuxPt;                /* Number of aux points in current contour */
	uint8   *pXOrg;                 /* Pointer to X coord of first point */
	uint8   *pYOrg;                 /* Pointer to Y coord of first point */
	uint8   *pAuxXOrg;              /* Pointer to X coord of first aux point */
	uint8   *pAuxYOrg;              /* Pointer to Y coord of first aux point */
	uint8   *pX;                    /* Pointer to X coord of current point */
	uint8   *pY;                    /* Pointer to Y coord of current point */
	uint8   *pAuxX;                 /* Pointer to X coord of current aux point */
	uint8   *pAuxY;                 /* Pointer to Y coord of current aux point */
	int16    incr;                  /* Increment for point pointers */
	int16    aux_incr;              /* Increment for auxiliary point pointers */
	uint16   curve_mask;			/* you have to check a different bit, working backwards */
	uint16   rawX;                  /* Raw X coord data for current point */
	uint16   rawY;                  /* Raw Y coord data for current point */
	int16    X = 0;                 /* X coord of current point */
	int16    Y = 0;                 /* Y coord of current point */
	int16    dX;                    /* X coord increment for current aux point */
	int16    dY;                    /* Y coord increment for current aux point */
	int16    auxX;                  /* X coord of current auxiliary point */
	int16    auxY;                  /* Y coord of current auxiliary point */
	int16    oldX;                  /* X coord of preceding point */
	int16    oldY;                  /* Y coord of preceding point */

	char  outside;
	GlyphClass *glyph = t->glyph;


	t->contourOpen = false;
	pointer = pChar + 10;
	ContourTreeOffset = (uint16)GET_WORD(pointer);  /* Read offset to character outline data */
	if (ContourTreeOffset == 0xffff)        /* No contours in this character? */
		return true;
	pCharOutlineData = pChar + 4 + ContourTreeOffset;
	pointer = pCharOutlineData + 2;
	ContourFlag = (uint16)GET_WORD(pointer);

	if (ContourFlag == 3)
	{	/* undocumented format? */
		
		if (GET_WORD((pChar + 10)) == 0xFFFF)
			return (false);         /* a no-op character! */
		pCharOutlineData = pChar + GET_WORD((pChar + 10)) + 4;
	}
	pointer = pCharOutlineData;
	nContour = (int16)GET_WORD(pointer);       /* Number of contours */
	pContourTreeData = pCharOutlineData + 4;
	for (ii = 0; ii < nContour; ii++, pContourTreeData += 8) /* For each contour... */
    {
		pointer = pContourTreeData;
		outside = (char)pContourTreeData[3]; /* Read polarity of contour data */
		if (ContourFlag == 3)
		{	/* undocumented format? */
			XYCoordDataOffset = 6;
			pointer = pChar + GET_WORD((pCharOutlineData + 8*ii + 4)) + 4;
			nPoint = (int16)GET_WORD(pointer);    /* total nr of contour + aux points + 1 */
			pointer += XYCoordDataOffset;
		
			for (jj = 1; jj < nPoint; jj++)
			{
				if (GET_WORD((pointer + 2*jj)) == (nPoint - jj - 1))
					break;
			}
			nAuxPt = (int16)(nPoint - jj - 1);
			if (nAuxPt < 0 || nAuxPt > jj)
			{

#ifdef DEBUG_ALT
				printf("          *** Unable to find auxiliary point count data! ***\n");
#endif
				return(false);
			}
    	
			pXOrg = pointer;  /* Pointer to X coord of first point */
			pYOrg = pXOrg + (nPoint << 1);  /* Pointer to Y coord of first point */
			nPoint = jj;	 /* number of contour points only */
		}
		else
		{	/* standard format */
			XYCoordDataOffset = (uint16)GET_WORD(pointer); /* Read offset to coordinate data */
			pointer = pChar + 4 + XYCoordDataOffset;
			nPoint = (int16)GET_WORD(pointer);     /* Number of points in contour */
			pointer += 2;
			nAuxPt = (int16)GET_WORD(pointer);     /* Number of auxiliary points in contour */
			pXOrg = pointer + 2;            /* Pointer to X coord of first point */
			pYOrg = pXOrg + (nPoint << 1);  /* Pointer to Y coord of first point */
		}
		pAuxXOrg = pYOrg + (nPoint << 1); /* Pointer to X coord of first aux point */
		pAuxYOrg = pAuxXOrg + nAuxPt;   /* Pointer to Y coord of first aux point */
		pX = pXOrg;
		pY = pYOrg;                     /* Current point is first point */
		if (outside)                    /* Outside (counter-clockwise) contour? */
        {
			pAuxX = pAuxXOrg; 
			pAuxY = pAuxYOrg;           /* Current aux point is first aux point */
			incr = 2;                   
			aux_incr = 1;               /* Processing direction is first to last */
			curve_mask = 0x8000;
        }
		else                            /* Inside (clockwise) contour? */
        {
			pAuxX = pAuxXOrg + nAuxPt - 1;
			pAuxY = pAuxYOrg + nAuxPt - 1; /* Current aux point is last aux point */ 
			incr = -2;
			aux_incr = -1;              /* Processing direction is last to first */
			curve_mask = 0x4000;
        }
		for (jj = 0; jj <= nPoint; jj++, pX += incr, pY += incr) /* For each point... */
        {
			if ((jj == nPoint) &&       /* Last point? */
				(outside))              /* Outside contour? */
            {
				pX = pXOrg;
				pY = pYOrg;             /* Current point is first point */
            }
			oldX = X;
			oldY = Y;                   /* Save preceding point */
			rawX = (uint16)GET_WORD(pX);        /* Read raw X data for current point */
			rawY = (uint16)GET_WORD(pY);        /* Read raw Y data for current point */
			X = (int16)(rawX & 0x3FFF);
			Y = (int16)(rawY & 0x3FFF);        /* Extract coordinate data */
			if (jj == 0)                /* First point in contour? */
            {
				if (t->contourOpen)
					glyph_CloseContour( glyph );
	        	glyph_AddPoint( glyph, (long)X + comp_xoff - t->eo_left_reference, (long)Y + comp_yoff - t->eo_baseline, true /*onCurve*/ );
			   	t->contourOpen = true;
					
				if (!outside)           /* Inside contour? */
                {
					pX = pXOrg + (nPoint << 1);
					pY = pYOrg + (nPoint << 1); /* Wrap to end of contour */
                }
				continue;
            }
			if ((X == oldX) &&
				(Y == oldY))            /* Duplicate point? */
            {
            
				if ( ContourFlag != 3 && ((rawX & curve_mask) == 0)) /* Curve ends at current point? */
                {
					pAuxX += aux_incr;  /* Update pointer to X coord of current aux point */
					pAuxY += aux_incr;  /* Update pointer to Y coord of current aux point */
                }
            
				continue;
            }
			if ( ContourFlag != 3 && ((rawX & curve_mask) == 0))   /* Curve ends at current point? */
            {
				dX = (int16)((int8 )(*pAuxX)); /* Read dX for aux point */
				pAuxX += aux_incr;      /* Update pointer to X coord of current aux point */
				dY = (int16)((int8 )(*pAuxY)); /* Read dY for aux point */
				pAuxY += aux_incr;      /* Update pointer to Y coord of current aux point */
/*				if (true) */ /* Aux point is significant? */
				{
					auxX = (int16)(((oldX + X) >> 1) + dX);
					auxY = (int16)(((oldY + Y) >> 1) + dY);
		        	glyph_AddPoint( glyph, (long)oldX + comp_xoff - t->eo_left_reference, (long)oldY + comp_yoff - t->eo_baseline, false );
		        	glyph_AddPoint( glyph, (long)auxX + comp_xoff - t->eo_left_reference, (long)auxY + comp_yoff - t->eo_baseline, false );
		        	glyph_AddPoint( glyph, (long)X + comp_xoff - t->eo_left_reference, (long)Y + comp_yoff - t->eo_baseline, true );
					continue;
				}
			}
	        glyph_AddPoint( glyph, (long)X + comp_xoff - t->eo_left_reference, (long)Y + comp_yoff - t->eo_baseline, true );
		}
		glyph_CloseContour( glyph );
		t->contourOpen = false;
	}
	return (true);                      /* End of character */
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
#endif /* NEED_FLIP */


uint16 FF_GetAW_PCLClass( void *param1, register uint16 index )
{
	register PCLClass *t = (PCLClass *)param1;
	int16 setWidth = 0;
	uint16 byteCount, charCode, nothing;
	uint8 *p;
	
	if (index < t->NumCharStrings)
	{
		eo_get_char_data(	(long) index,
										0x0008/* T2K_CODE_IS_GINDEX */,
										(uint8 **)&p,
										(uint16 *)&byteCount,
										(uint16 *)&charCode,
										(uint16 *)&nothing);
		if (p)
			setWidth = eo_get_swidth(p);
	}
	return (uint16)setWidth; /*****/
}

#endif /* ENABLE_PCL */
/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 * 12/13/99 R. Eggers Created                                                *
 *     $Header: R:/src/FontFusion/Source/Core/rcs/pclread.c 1.12 2001/05/03 15:56:36 reggers Exp $
 *                                                                           *
 *     $Log: pclread.c $
 *     Revision 1.12  2001/05/03 15:56:36  reggers
 *     Warning cleanup.
 *     Revision 1.11  2000/10/18 17:43:18  reggers
 *     Removed dependence on FF_FontTypeFromStream() and in-lined
 *     the isPCL checking to satisfy error handling requirements.
 *     Revision 1.10  2000/07/11 17:32:05  reggers
 *     Borland STRICK warning fixes
 *     Revision 1.9  2000/06/16 18:38:08  reggers
 *     Warnings cleanup.
 *     Revision 1.8  2000/06/02 19:41:10  reggers
 *     Removed last 'float'
 *     Revision 1.7  2000/05/26 16:44:10  reggers
 *     Support for PCLeo GetAWFuncPtr.
 *     Revision 1.6  2000/01/20 16:14:41  reggers
 *     fixed prototype of mbyte
 *     Revision 1.5  2000/01/19 21:34:27  reggers
 *     Flip the contour direction else auto-hinting is horrible!
 *     Revision 1.4  2000/01/18 20:52:39  reggers
 *     Changes to abstract the character directory and character string
 *     storage to the application environment.
 *     Revision 1.3  2000/01/07 19:44:46  reggers
 *     Get rid of FALSE and TRUE
 *     Revision 1.2  2000/01/06 20:53:23  reggers
 *     Corrections of data types and casts. Cleanup for configurations.
 *     Revision 1.1  1999/12/27 19:55:05  reggers
 *     Initial revision
 *                                                                           *
******************************************************************************/
/* EOF pclread.c */

