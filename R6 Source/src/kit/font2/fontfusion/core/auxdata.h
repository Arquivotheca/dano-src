/*
 * Auxdata.h
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Sampo Kaasila
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



/*************************** A U X D A T A . H *******************************
 *                                                                           *
 *  AuxData.h - Define standard auxilliary data stored by High Level         *
 *  DLL interface.  This data must be sufficient to recreate LOGFONT         *
 *  and TEXTMETRICS data structures for Windows Apps.                        *
 *                                                                           *
 *  For flexibility, specific data is stored in a chain of blocks, so        *
 *  that new blocks can be added.  The chain format is as follows:           *
 *                                                                           *
 *      Offset	Size 	Contents                                             *
 *      ------	----	--------                                             *
 *         0	  2		len - Block lenght in bytes (always rounded to even) *
 *         2	  2		BlockID defines block contents                       *
 *         4	 len	                                                     *
 *       4+len	  2	 	repeat for subsequent blocks                         *
 *                                                                           *
 *   Chain is terminated by a block with length and ID of 0                  *
 *                                                                           *
 ****************************************************************************/

#ifndef auxdata_h
#define auxdata_h

typedef struct blockHead_tag
    {
    unsigned char len[2];
    unsigned char blockID[2];
    }  blockHead_t;

/*  Block IDS */

#define IDWINFACENAME 1  	/* Windows FaceName */
#define IDWINTYPEATT 2		/* Typographic attributes and 
                               metrics used by windows */
#define IDWINSTYLE 3        /* Windows style name */
#define IDPOSTNAME 4        /* PostScript name */
#define IDADDTYPEATT 5		/* Additional type attributes */
#define IDFULLFONTNAME 6	/* Full font name */
#define IDPFMTYPEATT 7		/* PFM type attributes */
#define IDPAIRKERNDATA 8	/* Pair kerning data */
#define IDTRACKKERNDATA 9	/* Track kerning data */


/* User defined data tags - 0x8000 to 0xffff */

#define IDUSERDATA 0x8000
                                       	

/*  Block ID = 2 ;  Windows attributes and metrics */

#define CS_WINDOWSANSI 0
#define CS_WINDOWSCUSTOM 1
#define CS_MACANSI 2
#define CS_MACCUSTOM 3
#define CS_UNICODE 4
#define CS_CUSTOM_TRANSLATE 5

typedef struct auxTypeAtt_tag
    {
    unsigned char panose[10];
    unsigned char ascent[2];
    unsigned char descent[2];
    unsigned char height[2];
    unsigned char internalLeading[2];
    unsigned char externalLeading[2];
    unsigned char aveCharWidth[2];
    unsigned char maxCharWidth[2];
    unsigned char weight[2];
    char italic;	/* boolean italic flag */                             
    char charSet;	/* character complement */
    char pitchAndFamily;	/* flags monospace and general style */
    char breakChar;
    unsigned char overhang[2];	/* width added to synthesize bold or italic fonts */
    }  auxTypeAtt_t;
                                       	
/*  Block ID = 5 ;  Additional type attributes and metrics */

typedef struct auxAddTypeAtt_tag
    {
	unsigned char underlinePos[2];
	unsigned char underlineThickness[2];
	unsigned char strikethroughPos[2];
	unsigned char strikethroughThickness[2];
	unsigned char subscriptXSize[2];
	unsigned char subscriptYSize[2];
	unsigned char subscriptXPos[2];
	unsigned char subscriptYPos[2];
	unsigned char superscriptXSize[2];
	unsigned char superscriptYSize[2];
	unsigned char superscriptXPos[2];
	unsigned char superscriptYPos[2];
    }  auxAddTypeAtt_t;
                                       	
/*  Block ID = 7 ;  PFM attributes and metrics */
typedef struct auxPfmTypeAtt_tag
	{
	unsigned char dfAscent[2];
	unsigned char dfMaxWidth[2];
	unsigned char dfWeight[2];
	unsigned char etmSlant[2];
	unsigned char etmLowerCaseDescent[2];
	unsigned char lineGap[2];
	unsigned char sTypoAscender[2];
	unsigned char sTypoDescender[2];
	unsigned char dfItalic;
	unsigned char fixedPitch;
	}  auxPfmTypeAtt_t;


/*  Block ID = 8 ;  Pair kerning data */
typedef struct auxPairKernHeader_tag
	{
	unsigned char nKernPairs[2];
	unsigned char baseAdjustment[2];
	unsigned char flags;
	} auxPairKernHeader_t;

/* Bit assignment for pair kerning header flags */
#define TWO_BYTE_KERN_PAIR_CHAR_CODES 1
#define TWO_BYTE_KERN_PAIR_ADJ_VALUES 2

/*  Block ID = 9 ;  Track kerning data */
typedef struct auxTrackKernHeader_tag
	{
	unsigned char nKernTracks[2];
	} auxTrackKernHeader_t;
	

/*  Data access macros */

#define PUTAUXINT(chararray,ival) chararray[0] = (unsigned char)(ival >> 8);chararray[1] = (unsigned char)(ival & 0xFF);

#define GETAUXINT(chararray) (((unsigned short)chararray[0] << 8) + (unsigned short)chararray[1])   


#endif /* #ifndef auxdata_h */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/auxdata.h 1.3 1999/09/30 15:11:06 jfatal release $
 *                                                                           *
 *     $Log: auxdata.h $
 *     Revision 1.3  1999/09/30 15:11:06  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:59:25  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
