/*
 * FFT1HINT.H
 * Font Fusion Copyright (c) 1989-2000 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Mike Dewsnap
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

#ifndef __FFHINT__
#define __FFHINT__


#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifdef ENABLE_NATIVE_T1_HINTS

#define MAXBLUEVALUES	14
#define MAXSNAPVALUES	12
#define FLOATING		0
#define BOTTOM_ALIGNED	1
#define TOP_ALIGNED		2

#define WIDTH_CHUNK		20
#define	DEPTH_CHUNK		10

typedef struct blueZone_tag
    {
    F26Dot6   minPix;             /* Lower limit of snapping zone */
    F26Dot6   maxPix;             /* Upper limit of snapping zone */
    F26Dot6   refPix;             /* Unrounded alignment value */
    } blueZone_t;


typedef struct stemSnap_tag
    {
    F26Dot6   minPix;             /* Lower limit of snapping zone */
    F26Dot6   maxPix;             /* Upper limit of snapping zone */
    F26Dot6   refPix;             /* Unrounded alignment value */
    } stemSnap_t;


typedef struct {
	int16   *hint_array;
	int16		num_hints_ml;
	int16		num_hints;
	F26Dot6	*hint_pix;
	F26Dot6	*offset_ptr;		
	int16	*IntOrus;			
	F16Dot16 *mult_ptr;
} hints_t;


typedef struct {
	int16   *hint_array;
	int16		num_hints_ml;
	int16		num_hints;
} extraStroke_t;



typedef struct {
	int16    *EdgeThresh;
	int16    *EdgeDelta;
	int16    *EdgeIndex;
	int16		numEdges_ml;
	int16		numEdges;
} extraEdge_t;



typedef struct {
	/* public */	
	

	/* semi private */
	
	/* private */
	tsiMemObject *mem;

	/* Scaling factors */
	int32		xPixelsPerEm;
	int32		yPixelsPerEm;
	F16Dot16	xScale;
	F16Dot16	yScale;
	F26Dot6		xpos;
	F26Dot6		ypos;
	int32		upem;

	/* Blue Zone info */

	blueZone_t	pBlueZones[MAXBLUEVALUES];	/* A structure of blue values in pixel space */
	int32		bluevalues[MAXBLUEVALUES];	/* A list of the blue values from the font in orus */
	int32		numBlueValues;				/* Number of blue values received from font */
	int32		BlueFuzz;
	F16Dot16	BlueScale;
	int32		BlueShift;
	F26Dot6		BlueShiftPix;

	/* Stem info */

	stemSnap_t	pSnapV[MAXSNAPVALUES];		/* A structure of stem snap values in pixel space */
	stemSnap_t	pSnapH[MAXSNAPVALUES];		/* A structure of stem snap values in pixel space */
	int32		snapHWVals[MAXSNAPVALUES];	/* Array of stem values retreived from font (orus) */
	int32		snapVWVals[MAXSNAPVALUES];	/* Array of stem values retreived from font (orus) */
	int32		numSnapV;					/* How many V stem snap values */
	int32		numSnapH;					/* How many H stem snap values */
	int32		numSnapVZones;				/* How many V snap zones */
	int32		numSnapHZones;				/* How many H snap zones */
	int32		StdHW;						/* Standard Horizontal Weight */
	int32		StdVW;						/* Standard Vertical Weight */
	int16		numHintSets;				/* The number of sets of hints */

	/***   New Stuff **/


	int16		*hintmarkers_x_ptr;
	int16			hintmarkers_x_ml;
	int16			num_hintmarkers_x;

	int16		*hintmarkers_y_ptr;
	int16			hintmarkers_y_ml;
	int16			num_hintmarkers_y;

	int16		*xgcount_ptr;
	int16			xgcount_ml;
	int16			num_xgcount;


	int16		*ygcount_ptr;
	int16			ygcount_ml;
	int16			num_ygcount;


	int16		*xbgcount_ptr;
	int16			xbgcount_ml;
	int16		numxbgcount;

	int16		*ybgcount_ptr;
	int16			ybgcount_ml;
	int16		numybgcount;


	int16		*xOrus_ptr;
	int16			xOrus_num_ml;
	int16		numxOrus;					/* Total number of vertical hints parsed */

	int16		*yOrus_ptr;
	int16			yOrus_num_ml;
	int16		numyOrus;					/* Total number of horizontal hints parsed */


	int16		*nxIntOrus_ptr;
	int16			xInt_num_ml;
	int16		numxIntOrus;					/* Total number of vertical hints parsed */

	int16		*nyIntOrus_ptr;
	int16			yInt_num_ml;
	int16		numyIntOrus;					/* Total number of horizontal hints parsed */

	/***  *********** **/


	hints_t		*x_hints;
	hints_t		*y_hints;

	extraStroke_t	*x_strokes;
	extraStroke_t	*y_strokes;

	int16			num_x_hint_sets_ml;
	int16			num_y_hint_sets_ml;

	extraEdge_t	*x_edges;
	extraEdge_t	*y_edges;


	F26Dot6		onepix;
	F26Dot6		pixrnd;
	F26Dot6		pixfix;
	int32		suppressOvershoots;


	int16		*extraXStrokeOrus_ptr;
	int16			extraXStrokeOrus_ml;
	int16			numextraXStroke;


	int16		*extraYStrokeOrus_ptr;
	int16			extraYStrokeOrus_ml;
	int16			numextraYStroke;


	int16		*extraXStrokeGlyphCount_ptr;
	int16			extraXStrokeGlyphCount_ml;
	int16			numextraXStrokeGlyphCount;

	int16		*extraYStrokeGlyphCount_ptr;
	int16			extraYStrokeGlyphCount_ml;
	int16			numextraYStrokeGlyphCount;

	int16			num_tcb;
	
	int16			numextraXEdge;
	int16			numextraYEdge;
	int16			extraEdgeX_ml;
	int16			extraEdgeY_ml;

	int16			*extraXEdgeThresh_ptr;
	int16			*extraYEdgeThresh_ptr;
	int16			*extraXEdgeDelta_ptr;
	int16			*extraYEdgeDelta_ptr;
	int16			*extraXEdgeIndex_ptr;
	int16			*extraYEdgeIndex_ptr;
	int16		*extraXEdgeGlyphCount_ptr;
	int16		*extraYEdgeGlyphCount_ptr;


} FFT1HintClass;

/*
 *
 */
FFT1HintClass *New_FFT1HintClass( tsiMemObject *mem, int16 units_per_em);


void SetScale_FFT1HintClass( FFT1HintClass *t, int32 xPixelsPerEm, int32 yPixelsPerEm );


void SetupGlobalHints( FFT1HintClass *t, int32 numBlues, int32 BlueFuzz, F16Dot16 BlueScale,
						int32 BlueShift, int32 *BlueArray,
						int32 StdVW, int32 StdHW, int32 *StemVArray, int32 *StemHArray,
						int32 numVArray, int32 numHArray);


void ApplyHints_FFT1HintClass( FFT1HintClass *t, int16 count, int16 extracount, GlyphClass *glyph );

void FFT1HintClass_releaseMem(FFT1HintClass *t);

void Delete_FFT1HintClass( FFT1HintClass *t );

#else /* !ENABLE_NATIVE_T1_HINTS: */
typedef void FFT1HintClass;
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __FFHINT__ */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/fft1hint.h 1.7 2000/06/16 17:17:57 reggers release $
 *                                                                           *
 *     $Log: fft1hint.h $
 *     Revision 1.7  2000/06/16 17:17:57  reggers
 *     Made ENABLE_NATIVE_T1_HINTS user config option.
 *     Revision 1.6  2000/06/15 19:28:17  reggers
 *     Added comment header and footer.
 *
******************************************************************************/
