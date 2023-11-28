/*
 * pclread.h
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
#ifndef _PCLRead_H
#define _PCLRead_H
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/* Miscellaneous constants */
#define MASTER_RESOLUTION  8782 /* resolution of the following parameters */
#define BASELINE           5380 /* Baseline position in Design Window Coordinates */
#define LEFT_REFERENCE     2980 /* Left sidebearing position in Design Window Coordinates */

typedef struct
    { 
    uint8  structure;
    uint8  width;
    uint8  posture;
    } style_t;

typedef struct
    { 
    uint8  vendor;
    uint8  version;
    uint16  value;
    } typeface_t;

typedef struct
{
uint16  size;           /* 0   1  always 80 */
uint8   dformat,        /* 2      always 10 */
        font_type;      /* 3      0=7-bit,1=8-bit,2=PC-8 */
style_t style;          /* 4 23   */
uint16  baseline,       /* 6   7  bottom of em to baseline, PCPU */
        cell_width,     /* 8   9  PCPU */
        cell_height;    /* 10 11  PCPU */
uint8   orientation,    /* 12     always 0 */
        spacing;        /* 13     0 = fixed, 1 = proportional */
uint16  symbol_set,     /* 14 15  HP symbol set */
        pitch,          /* 16 17  default HMI for monospace, PCPU. 
                                  0 if proportional font */
        height,         /* 18 19  always 192 */
        x_height;       /* 20 21  height of lowercase x from baseline, PCPU */
int8    width_type;     /* 22     -2 condensed to +2 expanded */
int8    stroke_weight;  /* 24     -7 to 7, 0 being normal */
typeface_t typeface;    /* 26 25  Typeface Value(bits 0 - 8)  */ 
                        /*        Version(bits 9 -10) Vendor(bits 11-14) */ 
uint8   serif_style,    /* 27     same as cvthpf uses */
        quality;        /* 28     always letter quality (2) */
int8    placement,      /* 29     */
        uline_dist;     /* 30     always 0 */ 
uint8   uline_height;   /* 31     always 0 */
uint16  text_height,    /* 32 33  always 0 */
        text_width,     /* 34 35  always 0 */
        first_code,     /* 36 37  */
        last_code;      /* 38 39  */
uint8   pitch_extend,   /* 40     */
        height_extend;  /* 41     */
uint16  cap_height;     /* 42 43  */
uint8   font_vendor_code;  /* 44  */
uint32  font_number;    /* 45 46 47 */
                        /* 45 46 47 */ 
char    fontname[16];   /* 48-63    */
uint16  scale_factor,   /* 64 65    */
        x_resolution,   /* 66 67    */
        y_resolution;   /* 68 69    */
int16   mstr_uline_pos; /* 70 71    baseline to top of underline */
uint16  mstr_uline_hght,/* 72 73    thickness of underline */
        threshold;      /* 74 75    */
int16   italic_angle;   /* 76 77    */
uint16  scal_data_size; /* 78 79    */
int8   *scal_data;      /* 80 ...   */
uint16  nobtf;          /* 80 + .scal_data_size -- Copyright notice size */
char   *copyright;      /* 80 + .scal_data_size + 3 ...    */
uint8   checksum;       /* 80 + .scal_data_size + .nobtf
                           sigma bytes 64 through c.s. = 0 */
uint16  total_size;     /* 80 + .scal_data_size + .notbf + 2 */
} PC5HEAD;


#define GET_WORD(A)  ((uint16)((uint16)A[0] << 8) | ((uint8)A[1]))

typedef   unsigned char  eo_char_t;


#define  EEO_DOES_NOT_EXIST      2001     /* Nonexistent character */
#define  EEO_NESTED_COMP_CHAR    2002     /* Nested compound character */
#define  EEO_ILL_COMP_CHAR_REF   2003     /* Compound character ref's nonexistent character */
#define  EEO_ILL_COMP_SUBCHAR    2004     /* Illegal subcharacter index */
#define  EEO_NOT_A_PCL5_CHAR     2005     /* Illegal PCL5 character descriptor */
#define  EEO_BAD_OUTL_DATA       2006     /* 1) Unparsable data; 2) can't split arc */
#define  EEO_SPECS_OUT_OF_RANGE  2007     /* font specs out of range (eo_set_specs) */
#define  EEO_WIDTH_SPECS_NOTVAL  2008     /* font specs invalid while attempting call to
                                             eo_get_char_width(), eo_get_bitmap_width()
                                             or eo_get_char_bbox()   */

#if 0
typedef struct {
    uint16  gpsSize;
    long    gpsOffset;
} indexToLoc_t;

typedef struct {
	uint16	charCode;
	uint16	gIndex;
} codeToIndex_t;
#endif

typedef struct {
	/* private */
	PC5HEAD  pcl5head;
	tsiMemObject *mem;
	InputStream *in;
#if 0
	indexToLoc_t	*indexToLoc;
	codeToIndex_t	*codeToIndex;
#endif
	uint16	outlineRes;
	uint16	metricsRes;
	int8 verticalEscapement;
	long lsbx;
	uint8	contourOpen;
	long awx;
	long awy;
	int16 eo_baseline;				/* baseline adjustment vector */
	int16 eo_left_reference;		/* left side bearing adjustment vector */
	

	
	/* public */
	GlyphClass *glyph;

	
	short NumCharStrings;
	long upem;
	long maxPointCount;
	long ascent;
	long descent;
	long lineGap;
	long advanceWidthMax;
	F16Dot16 italicAngle;
	
	uint32	isFixedPitch;			/* 0 = proportional, non 0 = monospace */
	uint16	firstCharCode;			/* lowest code point, character code in font */
	uint16	lastCharCode;			/* highest code point, character code in font */
	
	
} PCLClass;


#define  ESC_FNDESC  1
#define  ESC_CCHARID  2
#define  ESC_CHDESC  3
#define  NO_ESCAPE   0
#define  ESC_UNKN   -1
#define  ESC_EOF    -2

PCLClass *tsi_NewPCLClass( tsiMemObject *mem, InputStream *in, int32 fontNum );
void tsi_DeletePCLClass( PCLClass *t );
GlyphClass *tsi_PCLGetGlyphByIndex( PCLClass *t, uint16 index, uint16 *aWidth, uint16 *aHeight );
uint16 tsi_PCLGetGlyphIndex( PCLClass *t, uint16 charCode );
uint8 *GetPCLNameProperty( PCLClass *t, uint16 languageID, uint16 nameID );

void BuildPCLMetricsEtc( PCLClass *t, uint16 NumCharStrings );

uint16 FF_GetAW_PCLClass( void *param1, register uint16 index );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* _PCLRead_H */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 * 12/13/99 R. Eggers Created                                                *
 *     $Header: R:/src/FontFusion/Source/Core/rcs/pclread.h 1.4 2000/05/26 16:44:16 reggers release $
 *                                                                           *
 *     $Log: pclread.h $
 *     Revision 1.4  2000/05/26 16:44:16  reggers
 *     Support for PCLeo GetAWFuncPtr.
 *     Revision 1.3  2000/01/18 20:52:45  reggers
 *     Changes to abstract the character directory and character string
 *     storage to the application environment.
 *     Revision 1.2  2000/01/06 20:53:26  reggers
 *     Corrections of data types and casts. Cleanup for configurations.
 *     Revision 1.1  1999/12/27 19:55:16  reggers
 *     Initial revision
 *                                                                           *
******************************************************************************/
/* EOF PCLRead.h */
