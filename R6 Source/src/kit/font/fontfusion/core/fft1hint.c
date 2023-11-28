/*
 * FFT1HINT.C
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

#include "syshead.h"
#include "config.h"

#ifdef ENABLE_NATIVE_T1_HINTS

#include "dtypes.h"
#include "tsimem.h"
#include "util.h"


#include "t2kstrm.h"
#include "truetype.h"
#include "fft1hint.h"
#include "util.h"

/* #define  DEBUG_PRINT_HINTS */


/* Local prototypes */
 
static void SetupStemWeights( FFT1HintClass *t, F16Dot16 scale, int32 *snapVals,
						int32 numSnapVals, int32 StdW , stemSnap_t *pSnapZones, int32 *numSnapZones);

static void SetInterpolation( FFT1HintClass *t);

static void DoVStrokes( FFT1HintClass *t, int32 firstIndex, int32 lastIndex, int16 *xOrus,
				F26Dot6 *xPix, int iParent);

static void DoHStrokes( FFT1HintClass *t, int32 firstIndex, int32 lastIndex, int16 *yOrus,
				F26Dot6 *yPix, int iParent);


static void SetupInput( FFT1HintClass *t );

static void DoExtraStrokes( FFT1HintClass *t,
				int16 *num_x, int16 *num_y, 
				int16 *xOrus, F26Dot6 *xPix,
				int16 *yOrus, F26Dot6 *yPix,
				int16 *extraXStoke, int16 *extraYStroke,
				int16 *numExtraX, int16 *numExtraY);

static void DoExtraEdges(FFT1HintClass *t,
					int16 *num_x, int16 *num_y, 
					int16 *xOrus, F26Dot6 *xPix,
					int16 *yOrus, F26Dot6 *yPix,
					int16	*extraXEdgeDelta,
					int16	*extraYEdgeDelta,
					int16	*extraXEdgeIndex,
					int16	*extraYEdgeIndex,
					int16	*extraXEdgeThresh,
					int16	*extraYEdgeThresh,
					int16 *numExtraX, int16 *numExtraY);



static void FlipContourDirection(GlyphClass *glyph, int16 group_flag)
{
	short	ctr, j;
	short	*oox;
	short	*ooy;
	F26Dot6 *x;
	F26Dot6 *y;


	if (group_flag == 0)
	{
		oox = glyph->oox;
		ooy = glyph->ooy;
	}
	else
	{
		x = glyph->x;
		y = glyph->y;
	}

	for ( ctr = 0; ctr < glyph->contourCount; ctr++ ) {
	 	short	flips, start, end;
	 	
	 	start	= glyph->sp[ctr];
	 	end		= glyph->ep[ctr];
	 	
	 	flips = (short)((end - start)/2);
	 	start++;
		if (group_flag == 0)
		{
			for ( j = 0; j < flips; j++ ) {
				int16	tempX, tempY;
	
				int16   indexA = (int16)(start + j);
				int16   indexB = (int16)(end   - j);
	 		
	 			tempX	= oox[indexA];
	 			tempY	= ooy[indexA];

	 		
	 			oox[indexA]	= oox[indexB];
	 			ooy[indexA]	= ooy[indexB];
	 

	 			oox[indexB]	= tempX;
	 			ooy[indexB]	= tempY;
	 
			}
		}
		else
		{
			for ( j = 0; j < flips; j++ ) {
				F26Dot6	tempX, tempY;
	
				int16   indexA = (int16)(start + j);
				int16   indexB = (int16)(end   - j);
	 		
	 			tempX	= x[indexA];
	 			tempY	= y[indexA];
	 
	 		
	 			x[indexA]	= x[indexB];
	 			y[indexA]	= y[indexB];
	 

	 			x[indexB]	= tempX;
	 			y[indexB]	= tempY;
	 
			}
		}
	}
}



/*
 * The FFT1HintClass constructor
 */
FFT1HintClass *New_FFT1HintClass( tsiMemObject *mem, int16 units_per_em )
{
	FFT1HintClass *t = (FFT1HintClass *) tsi_AllocMem( mem, sizeof( FFT1HintClass ) );
	
	
	t->mem			= mem;					/* Memory for the class */
	t->upem			= units_per_em;			/* UPEM					*/
	t->xPixelsPerEm = -1;					
	t->yPixelsPerEm	= -1;			
	t->xScale = -1;							/* X Scale factor		   */
	t->yScale = -1;							/* Y Scale factor		   */
	t->numBlueValues = -1;					/* Number of Blue Values */
	t->BlueFuzz = -1;					
	t->BlueShift = -1;
	t->BlueShiftPix = 0;
	t->StdHW = -1;							/* Main Horizontal stem weight	*/
	t->StdVW = -1;							/* Main Vertical stem weight    */
	t->numSnapV = 0;						/* Number of original snap values as retrieved from font */
	t->numSnapH = 0;						/* Number of original snap values as retrieved from font */
	t->numSnapVZones = 0;					/* Number of snap values after including the main stem */
	t->numSnapHZones = 0;					/* Number of snap values after including the main stem */

	t->numHintSets = 0;						/* Number of different groups of hints -- for hint substitusion */
	t->onepix = 1 << 6;						/* 1 pixel in 26.6 notation */
	t->pixrnd = 1 << 5;						/* Half a pixel in 26.6 notation */
	t->pixfix = 0xFFFFFFC0;					/* Mask only whole pixel value in 26.6 notation */
	t->xpos = 0;							/* Positioning for compound characters -- currently not used */
	t->ypos = 0;
	t->num_tcb = 0;
	t->suppressOvershoots = 1;				/* Flag for suppression of Overshoots */

	t->numextraXStroke = 0;
	t->numextraYStroke = 0;
	t->numextraXEdge = 0;
	t->numextraYEdge = 0;	

	t->xgcount_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->xgcount_ml = WIDTH_CHUNK;
	t->num_xgcount = 0;
	memset((void *)t->xgcount_ptr, 0, WIDTH_CHUNK * sizeof(int16));

	t->ygcount_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->ygcount_ml = WIDTH_CHUNK;
	t->num_ygcount = 0;
	memset((void *)t->ygcount_ptr, 0, WIDTH_CHUNK * sizeof(int16));

	t->xOrus_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->xOrus_num_ml = WIDTH_CHUNK;
	t->numxOrus = 0;

	t->yOrus_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->yOrus_num_ml = WIDTH_CHUNK;
	t->numyOrus = 0;

	t->xbgcount_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->xbgcount_ml = WIDTH_CHUNK;
	t->numxbgcount = 0;
	memset((void *)t->xbgcount_ptr, 0, WIDTH_CHUNK * sizeof(int16));


	t->ybgcount_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->ybgcount_ml = WIDTH_CHUNK;
	t->numybgcount = 0;
	memset((void *)t->ybgcount_ptr, 0, WIDTH_CHUNK * sizeof(int16));




	t->extraXStrokeOrus_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->extraXStrokeOrus_ml = WIDTH_CHUNK;
	t->numextraXStroke = 0;

	t->extraYStrokeOrus_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->extraYStrokeOrus_ml = WIDTH_CHUNK;
	t->numextraYStroke = 0;


	t->extraXStrokeGlyphCount_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->extraXStrokeGlyphCount_ml = WIDTH_CHUNK;
	t->numextraXStrokeGlyphCount = 0;


	t->extraYStrokeGlyphCount_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->extraYStrokeGlyphCount_ml = WIDTH_CHUNK;
	t->numextraYStrokeGlyphCount = 0;


	t->extraEdgeX_ml = WIDTH_CHUNK;
	t->extraEdgeY_ml = WIDTH_CHUNK;
	t->extraXEdgeThresh_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->extraYEdgeThresh_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->extraXEdgeDelta_ptr  = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->extraYEdgeDelta_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->extraXEdgeIndex_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->extraYEdgeIndex_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->extraXEdgeGlyphCount_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->extraYEdgeGlyphCount_ptr = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );



	return t; /******/
}


void SetupInput( FFT1HintClass *t)
{
	int i,index, k;
	int16 val;
	int	 j,jj, swap;
	F26Dot6 temp_p;
	int16   temp_hint;
	int		count;
	int		had_hints;



	t->hintmarkers_x_ptr = (int16 *)tsi_AllocMem( t->mem, DEPTH_CHUNK * sizeof( int16 ) );
	t->hintmarkers_x_ml = DEPTH_CHUNK;
	t->num_hintmarkers_x = 0;
	memset((void *)t->hintmarkers_x_ptr, 0, DEPTH_CHUNK * sizeof(int16));

	t->hintmarkers_y_ptr = (int16 *)tsi_AllocMem( t->mem, DEPTH_CHUNK * sizeof( int16 ) );
	t->hintmarkers_y_ml = DEPTH_CHUNK;
	t->num_hintmarkers_y = 0;
	memset((void *)t->hintmarkers_y_ptr, 0, DEPTH_CHUNK * sizeof( int16 ));

	t->x_hints = (hints_t *)tsi_AllocMem( t->mem, DEPTH_CHUNK * sizeof( hints_t ) );
	t->y_hints = (hints_t *)tsi_AllocMem( t->mem, DEPTH_CHUNK * sizeof( hints_t ) );
	t->num_x_hint_sets_ml = DEPTH_CHUNK;
	t->num_y_hint_sets_ml = DEPTH_CHUNK;

	for (count = 0; count < DEPTH_CHUNK; count++)
	{
		t->x_hints[count].hint_array = NULL;
		t->x_hints[count].num_hints = 0;
		t->x_hints[count].num_hints_ml = WIDTH_CHUNK;
		t->y_hints[count].hint_array = NULL;
		t->y_hints[count].num_hints = 0;
		t->y_hints[count].num_hints_ml = WIDTH_CHUNK;
	}


	t->nxIntOrus_ptr = (int16 *)tsi_AllocMem( t->mem, DEPTH_CHUNK * sizeof( int16 ) );
	t->xInt_num_ml = DEPTH_CHUNK;
	t->numxIntOrus = 0;					

	t->nyIntOrus_ptr = (int16 *)tsi_AllocMem( t->mem, DEPTH_CHUNK * sizeof( int16 ) );
	t->yInt_num_ml = DEPTH_CHUNK;
	t->numyIntOrus = 0;			


	t->x_strokes = (extraStroke_t *)tsi_AllocMem( t->mem, DEPTH_CHUNK * sizeof( extraStroke_t ) );
	t->y_strokes = (extraStroke_t *)tsi_AllocMem( t->mem, DEPTH_CHUNK * sizeof( extraStroke_t) );


	for (count = 0; count < DEPTH_CHUNK; count++)
	{
		t->x_strokes[count].hint_array = NULL;
		t->x_strokes[count].num_hints = 0;
		t->y_strokes[count].hint_array = NULL;
		t->y_strokes[count].num_hints = 0;
	}


	t->x_edges = (extraEdge_t *)tsi_AllocMem( t->mem, DEPTH_CHUNK * sizeof( extraEdge_t ) );
	t->y_edges = (extraEdge_t *)tsi_AllocMem( t->mem, DEPTH_CHUNK * sizeof( extraEdge_t ) );


	for (count = 0; count < DEPTH_CHUNK; count++)
	{
		t->x_edges[count].EdgeThresh = NULL;
		t->x_edges[count].EdgeDelta = NULL;
		t->x_edges[count].EdgeIndex = NULL;
		t->x_edges[count].numEdges = 0;

		t->y_edges[count].EdgeThresh = NULL;
		t->y_edges[count].EdgeDelta = NULL;
		t->y_edges[count].EdgeIndex = NULL;
		t->y_edges[count].numEdges = 0;
	}




	/***************** Get all X hints ********************************************/
	j=0;
	i=0;
	k=0;
	jj =0;
	index = 0;
	val = 0;


	/* Initialize the first hint structure */

	t->x_hints[0].num_hints = 0;
	t->x_hints[0].num_hints_ml = WIDTH_CHUNK;
	t->x_hints[0].hint_array = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->x_hints[0].hint_pix = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );

	t->x_hints[0].offset_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );		
	t->x_hints[0].IntOrus = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );			
	t->x_hints[0].mult_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );


	while (i < t->numxOrus)
	{
		had_hints = 0;
		while ((val == t->xgcount_ptr[jj]) && (j < t->numxOrus))
		{

			t->x_hints[index].hint_array[t->x_hints[index].num_hints] = t->xOrus_ptr[j];
			t->x_hints[index].num_hints++;

			if ((t->x_hints[index].num_hints) >= t->x_hints[index].num_hints_ml)
				{
					t->x_hints[index].num_hints_ml = (int16)(t->x_hints[index].num_hints_ml + WIDTH_CHUNK);
					t->x_hints[index].hint_array = (int16 *)tsi_ReAllocMem( t->mem, t->x_hints[index].hint_array , (t->x_hints[index].num_hints_ml) * sizeof(int16) );
					t->x_hints[index].hint_pix =   (F26Dot6 *)tsi_ReAllocMem( t->mem, t->x_hints[index].hint_pix , (t->x_hints[index].num_hints_ml) * sizeof(F26Dot6) );
					t->x_hints[index].offset_ptr = (F26Dot6 *)tsi_ReAllocMem( t->mem, t->x_hints[index].offset_ptr , (t->x_hints[index].num_hints_ml) * sizeof(F26Dot6) );
					t->x_hints[index].mult_ptr =   (F26Dot6 *)tsi_ReAllocMem( t->mem, t->x_hints[index].mult_ptr , (t->x_hints[index].num_hints_ml) * sizeof(F26Dot6) );
					t->x_hints[index].IntOrus =	   (int16 *)tsi_ReAllocMem( t->mem, t->x_hints[index].IntOrus , (t->x_hints[index].num_hints_ml) * sizeof(int16) );
				}

			j++;
			jj++;
			t->hintmarkers_x_ptr[index] = val;
		}

		t->num_hintmarkers_x++;

	
		if (t->xgcount_ptr[jj] == -999)
		{
		   i = j;
		   if (t->x_hints[index].num_hints > 0)
		   {
			had_hints = 1;
			index++;
		   }

		   if ((t->num_hintmarkers_x) >= t->hintmarkers_x_ml)
			{
				t->hintmarkers_x_ml = (int16)(t->hintmarkers_x_ml + DEPTH_CHUNK);
				t->hintmarkers_x_ptr = (int16 *)tsi_ReAllocMem( t->mem, t->hintmarkers_x_ptr , (t->hintmarkers_x_ml) * sizeof(int16) );
				
			}

		   t->hintmarkers_x_ptr[index] = t->xbgcount_ptr[k++];
		   jj++;
		   val = t->xgcount_ptr[jj];

		   	if ((index) >= t->num_x_hint_sets_ml)
			{
			/* extend NULLness here */
			int extCount, startIndex = t->num_x_hint_sets_ml;
				t->num_x_hint_sets_ml = (int16)(t->num_x_hint_sets_ml + DEPTH_CHUNK);
				t->x_hints = (hints_t *)tsi_ReAllocMem( t->mem, t->x_hints , (t->num_x_hint_sets_ml) * sizeof(hints_t) );
				t->x_strokes =
					(extraStroke_t *)tsi_ReAllocMem( t->mem, t->x_strokes , (t->num_x_hint_sets_ml) * sizeof(extraStroke_t) );
				t->x_edges = (extraEdge_t *)tsi_ReAllocMem( t->mem, t->x_edges , (t->num_x_hint_sets_ml) * sizeof(extraEdge_t) );
				for (extCount = startIndex; extCount < t->num_x_hint_sets_ml; extCount++)
				{
					t->x_hints[extCount].hint_array = NULL;
					t->x_strokes[extCount].num_hints = 0;
					t->x_edges[extCount].numEdges = 0;
				}
			}

			if ((index) >= t->xInt_num_ml )
			{
				t->xInt_num_ml = (int16)(t->xInt_num_ml + DEPTH_CHUNK);
				t->nxIntOrus_ptr = (int16 *)tsi_ReAllocMem( t->mem, t->nxIntOrus_ptr , (t->xInt_num_ml) * sizeof(int16) );
			}
			
			if (had_hints)
			{
				t->x_hints[index].num_hints = 0;
				t->x_hints[index].num_hints_ml = WIDTH_CHUNK;
				t->x_hints[index].hint_array = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
				t->x_hints[index].hint_pix = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );

				t->x_hints[index].offset_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );		
				t->x_hints[index].IntOrus = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );			
				t->x_hints[index].mult_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );
			}
		   index++; 
		   t->num_hintmarkers_x++;

		   if ((t->num_hintmarkers_x) >= t->hintmarkers_x_ml)
			{
			    t->hintmarkers_x_ml = (int16)(t->hintmarkers_x_ml + DEPTH_CHUNK);
				t->hintmarkers_x_ptr = (int16 *)tsi_ReAllocMem( t->mem, t->hintmarkers_x_ptr , (t->hintmarkers_x_ml) * sizeof(int16) );
				
			}

		    if ((index) >= t->num_x_hint_sets_ml)
			{
			/* extend NULLness here */
			int extCount, startIndex = t->num_x_hint_sets_ml;
				t->num_x_hint_sets_ml = (int16)(t->num_x_hint_sets_ml + DEPTH_CHUNK);
				t->x_hints = (hints_t *)tsi_ReAllocMem( t->mem, t->x_hints , (t->num_x_hint_sets_ml) * sizeof(hints_t) );
				t->x_strokes = (extraStroke_t *)tsi_ReAllocMem( t->mem, t->x_strokes , (t->num_x_hint_sets_ml) * sizeof(extraStroke_t) );
				t->x_edges = (extraEdge_t *)tsi_ReAllocMem( t->mem, t->x_edges , (t->num_x_hint_sets_ml) * sizeof(extraEdge_t) );
				for (extCount = startIndex; extCount < t->num_x_hint_sets_ml; extCount++)
				{
					t->x_hints[extCount].hint_array = NULL;
					t->x_strokes[extCount].num_hints = 0;
					t->x_edges[extCount].numEdges = 0;
				}
			}

			if ((index) >= t->xInt_num_ml )
			{
				t->xInt_num_ml = (int16)(t->xInt_num_ml + DEPTH_CHUNK);
				t->nxIntOrus_ptr = (int16 *)tsi_ReAllocMem( t->mem, t->nxIntOrus_ptr , (t->xInt_num_ml) * sizeof(int16) );	
			}

			t->x_hints[index].num_hints = 0;
			t->x_hints[index].num_hints_ml = WIDTH_CHUNK;
			t->x_hints[index].hint_array = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
			t->x_hints[index].hint_pix = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );

			t->x_hints[index].offset_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );		
			t->x_hints[index].IntOrus = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );			
			t->x_hints[index].mult_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );
		}
		else
		{
		i = j;
		val = t->xgcount_ptr[jj];
		index++;
		
		if ((t->num_hintmarkers_x) >= t->hintmarkers_x_ml)
			{
				t->hintmarkers_x_ml = (int16)(t->hintmarkers_x_ml + DEPTH_CHUNK);
				t->hintmarkers_x_ptr = (int16 *)tsi_ReAllocMem( t->mem, t->hintmarkers_x_ptr , (t->hintmarkers_x_ml) * sizeof(int16) );

			}

	
		if ((index) >= t->num_x_hint_sets_ml)
			{
			/* extend NULLness here */
			int extCount, startIndex = t->num_x_hint_sets_ml;
				t->num_x_hint_sets_ml = (int16)(t->num_x_hint_sets_ml + DEPTH_CHUNK);
				t->x_hints = (hints_t *)tsi_ReAllocMem( t->mem, t->x_hints , (t->num_x_hint_sets_ml) * sizeof(hints_t) );
				t->x_strokes = (extraStroke_t *)tsi_ReAllocMem( t->mem, t->x_strokes , (t->num_x_hint_sets_ml) * sizeof(extraStroke_t) );
				t->x_edges = (extraEdge_t *)tsi_ReAllocMem( t->mem, t->x_edges , (t->num_x_hint_sets_ml) * sizeof(extraEdge_t) );
				for (extCount = startIndex; extCount < t->num_x_hint_sets_ml; extCount++)
				{
					t->x_hints[extCount].hint_array = NULL;
					t->x_strokes[extCount].num_hints = 0;
					t->x_edges[extCount].numEdges = 0;
				}
			}

		if ((index) >= t->xInt_num_ml )
			{
				t->xInt_num_ml = (int16)(t->xInt_num_ml + DEPTH_CHUNK);
				t->nxIntOrus_ptr = (int16 *)tsi_ReAllocMem( t->mem, t->nxIntOrus_ptr , (t->xInt_num_ml) * sizeof(int16) );
			}

		t->x_hints[index].num_hints = 0;
		t->x_hints[index].num_hints_ml = WIDTH_CHUNK;
		t->x_hints[index].hint_array = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
		t->x_hints[index].hint_pix = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );

		t->x_hints[index].offset_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );		
		t->x_hints[index].IntOrus = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );			
		t->x_hints[index].mult_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );

		}
	}
	t->numHintSets = (int16)index;

	tsi_DeAllocMem( t->mem, t->x_hints[index].hint_array );
	tsi_DeAllocMem( t->mem, t->x_hints[index].hint_pix );
	tsi_DeAllocMem( t->mem, t->x_hints[index].offset_ptr );
	tsi_DeAllocMem( t->mem, t->x_hints[index].IntOrus );
	tsi_DeAllocMem( t->mem, t->x_hints[index].mult_ptr);

	t->x_hints[index].hint_array = NULL;

	if (t->numHintSets == 1)
	{
		t->hintmarkers_x_ptr[0] = -999;
		t->numHintSets = 1;
	}

	/*****************************************************************************/



	/***************** Get all Y hints *******************************************/
	j=0;
	i=0;
	k=0;
	jj =0;
	index = 0;
	val = 0;

	/* Initialize the first hint structure */
	t->y_hints[0].num_hints = 0;
	t->y_hints[0].num_hints_ml = WIDTH_CHUNK;
	t->y_hints[0].hint_array = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->y_hints[0].hint_pix = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );

	t->y_hints[0].offset_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );		
	t->y_hints[0].IntOrus = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );			
	t->y_hints[0].mult_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );

	while (i < t->numyOrus)
	{
		had_hints = 0;
		while ((val == t->ygcount_ptr[jj]) && (j < t->numyOrus))
		{
			t->y_hints[index].hint_array[t->y_hints[index].num_hints] = t->yOrus_ptr[j];
			t->y_hints[index].num_hints++;

			if ((t->y_hints[index].num_hints) >= t->y_hints[index].num_hints_ml)
				{
					t->y_hints[index].num_hints_ml = (int16)(t->y_hints[index].num_hints_ml + WIDTH_CHUNK);
					t->y_hints[index].hint_array = (int16 *)tsi_ReAllocMem( t->mem, t->y_hints[index].hint_array , (t->y_hints[index].num_hints_ml) * sizeof(int16) );
					t->y_hints[index].hint_pix =   (F26Dot6 *)tsi_ReAllocMem( t->mem, t->y_hints[index].hint_pix , (t->y_hints[index].num_hints_ml) * sizeof(F26Dot6) );
					t->y_hints[index].offset_ptr = (F26Dot6 *)tsi_ReAllocMem( t->mem, t->y_hints[index].offset_ptr , (t->y_hints[index].num_hints_ml) * sizeof(F26Dot6) );
					t->y_hints[index].mult_ptr =   (F26Dot6 *)tsi_ReAllocMem( t->mem, t->y_hints[index].mult_ptr , (t->y_hints[index].num_hints_ml) * sizeof(F26Dot6) );
					t->y_hints[index].IntOrus =	   (int16 *)tsi_ReAllocMem( t->mem, t->y_hints[index].IntOrus , (t->y_hints[index].num_hints_ml) * sizeof(int16) );
				}

			j++;
			jj++;
			t->hintmarkers_y_ptr[index] = val;
		}

		t->num_hintmarkers_y++;

	
		if (t->ygcount_ptr[jj] == -999)
		{
		   i = j;
		   if (t->y_hints[index].num_hints > 0)
		   {
			had_hints = 1;
		    index++;
		   }

		   if ((t->num_hintmarkers_y) >= t->hintmarkers_y_ml)
			{
			   	t->hintmarkers_y_ml = (int16)(t->hintmarkers_y_ml + DEPTH_CHUNK);
				t->hintmarkers_y_ptr = (int16 *)tsi_ReAllocMem( t->mem, t->hintmarkers_y_ptr , (t->hintmarkers_y_ml) * sizeof(int16) );
			}

		   t->hintmarkers_y_ptr[index] = t->ybgcount_ptr[k++];
		   jj++;
		   val = t->ygcount_ptr[jj];

		   	if ((index) >= t->num_y_hint_sets_ml)
			{
			/* extend NULLness here */
			int extCount, startIndex = t->num_y_hint_sets_ml;
				t->num_y_hint_sets_ml = (int16)(t->num_y_hint_sets_ml + DEPTH_CHUNK);
				t->y_hints = (hints_t *)tsi_ReAllocMem( t->mem, t->y_hints , (t->num_y_hint_sets_ml) * sizeof(hints_t) );
				t->y_strokes = (extraStroke_t *)tsi_ReAllocMem( t->mem, t->y_strokes , (t->num_y_hint_sets_ml) * sizeof(extraStroke_t) );
				t->y_edges = (extraEdge_t *)tsi_ReAllocMem( t->mem, t->y_edges , (t->num_y_hint_sets_ml) * sizeof(extraEdge_t) );
				for (extCount = startIndex; extCount < t->num_y_hint_sets_ml; extCount++)
				{
					t->y_hints[extCount].hint_array = NULL;
					t->y_strokes[extCount].num_hints = 0;
					t->y_edges[extCount].numEdges = 0;
				}
			}

			if ((index) >= t->yInt_num_ml )
			{
				t->yInt_num_ml = (int16)(t->yInt_num_ml + DEPTH_CHUNK);
				t->nyIntOrus_ptr = (int16 *)tsi_ReAllocMem( t->mem, t->nyIntOrus_ptr , (t->yInt_num_ml) * sizeof(int16) );
			}
			if (had_hints)
			{
				t->y_hints[index].num_hints = 0;
				t->y_hints[index].num_hints_ml = WIDTH_CHUNK;
				t->y_hints[index].hint_array = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
				t->y_hints[index].hint_pix = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );
				t->y_hints[index].offset_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );		
				t->y_hints[index].IntOrus = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );			
				t->y_hints[index].mult_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );
			}
			index++; 
		   t->num_hintmarkers_y++;


		   if ((t->num_hintmarkers_y) >= t->hintmarkers_y_ml)
			{
			   	t->hintmarkers_y_ml = (int16)(t->hintmarkers_y_ml + DEPTH_CHUNK);
				t->hintmarkers_y_ptr = (int16 *)tsi_ReAllocMem( t->mem, t->hintmarkers_y_ptr , (t->hintmarkers_y_ml) * sizeof(int16) );
			}

		    if ((index) >= t->num_y_hint_sets_ml)
			{
			/* extend NULLness here */
			int extCount, startIndex = t->num_y_hint_sets_ml;
				t->num_y_hint_sets_ml = (int16)(t->num_y_hint_sets_ml + DEPTH_CHUNK);
				t->y_hints = (hints_t *)tsi_ReAllocMem( t->mem, t->y_hints , (t->num_y_hint_sets_ml) * sizeof(hints_t) );
				t->y_strokes = (extraStroke_t *)tsi_ReAllocMem( t->mem, t->y_strokes , (t->num_y_hint_sets_ml) * sizeof(extraStroke_t) );
				t->y_edges = (extraEdge_t *)tsi_ReAllocMem( t->mem, t->y_edges , (t->num_y_hint_sets_ml) * sizeof(extraEdge_t) );
				for (extCount = startIndex; extCount < t->num_y_hint_sets_ml; extCount++)
				{
					t->y_hints[extCount].hint_array = NULL;
					t->y_strokes[extCount].num_hints = 0;
					t->y_edges[extCount].numEdges = 0;
				}
			}

			if ((index) >= t->yInt_num_ml )
			{
				t->yInt_num_ml = (int16)(t->yInt_num_ml + DEPTH_CHUNK);
				t->nyIntOrus_ptr = (int16 *)tsi_ReAllocMem( t->mem, t->nyIntOrus_ptr , (t->yInt_num_ml) * sizeof(int16) );
			}

			t->y_hints[index].num_hints = 0;
			t->y_hints[index].num_hints_ml = WIDTH_CHUNK;
			t->y_hints[index].hint_array = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
			t->y_hints[index].hint_pix = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );
			t->y_hints[index].offset_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );		
			t->y_hints[index].IntOrus = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );			
			t->y_hints[index].mult_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );
		}
		else
		{
		i = j;
		val = t->ygcount_ptr[jj];
		index++;
		
		if ((t->num_hintmarkers_y) >= t->hintmarkers_y_ml)
			{
				t->hintmarkers_y_ml = (int16)(t->hintmarkers_y_ml + DEPTH_CHUNK);
				t->hintmarkers_y_ptr = (int16 *)tsi_ReAllocMem( t->mem, t->hintmarkers_y_ptr , (t->hintmarkers_y_ml) * sizeof(int16) );
			}

	
		if ((index) >= t->num_y_hint_sets_ml)
			{
			/* extend NULLness here */
			int extCount, startIndex = t->num_y_hint_sets_ml;
				t->num_y_hint_sets_ml = (int16)(t->num_y_hint_sets_ml + DEPTH_CHUNK);
				t->y_hints = (hints_t *)tsi_ReAllocMem( t->mem, t->y_hints , (t->num_y_hint_sets_ml) * sizeof(hints_t) );
				t->y_strokes = (extraStroke_t *)tsi_ReAllocMem( t->mem, t->y_strokes , (t->num_y_hint_sets_ml) * sizeof(extraStroke_t) );
				t->y_edges = (extraEdge_t *)tsi_ReAllocMem( t->mem, t->y_edges , (t->num_y_hint_sets_ml) * sizeof(extraEdge_t) );
				for (extCount = startIndex; extCount < t->num_y_hint_sets_ml; extCount++)
				{
					t->y_hints[extCount].hint_array = NULL;
					t->y_strokes[extCount].num_hints = 0;
					t->y_edges[extCount].numEdges = 0;
				}
			}

		if ((index) >= t->yInt_num_ml )
			{
				t->yInt_num_ml = (int16)(t->yInt_num_ml + DEPTH_CHUNK);
				t->nyIntOrus_ptr = (int16 *)tsi_ReAllocMem( t->mem, t->nyIntOrus_ptr , (t->yInt_num_ml) * sizeof(int16) );
			}

		t->y_hints[index].num_hints = 0;
		t->y_hints[index].num_hints_ml = WIDTH_CHUNK;
		t->y_hints[index].hint_array = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
		t->y_hints[index].hint_pix = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );
		t->y_hints[index].offset_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );		
		t->y_hints[index].IntOrus = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );			
		t->y_hints[index].mult_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );

		}
	}

	if (index > t->numHintSets)
	t->numHintSets = (int16)index;

	
	tsi_DeAllocMem( t->mem, t->y_hints[index].hint_array );
	tsi_DeAllocMem( t->mem, t->y_hints[index].hint_pix );
	tsi_DeAllocMem( t->mem, t->y_hints[index].offset_ptr );
	tsi_DeAllocMem( t->mem, t->y_hints[index].IntOrus );
	tsi_DeAllocMem( t->mem, t->y_hints[index].mult_ptr);
	t->y_hints[index].hint_array = NULL;


	if (t->numHintSets == 1)
	{
		t->hintmarkers_y_ptr[0] = -999;
		t->numHintSets = 1;
	}


	for (count = 0; count < t->numHintSets; count++)
	{
#if 0
		if ((t->x_hints[count].num_hints == 0) && (t->numxOrus == 0))
#else
		if (t->x_hints[count].hint_array == NULL)
#endif
		{
			t->x_hints[count].hint_array = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
			t->x_hints[count].hint_pix = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );
			t->x_hints[count].offset_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );		
			t->x_hints[count].IntOrus = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );			
			t->x_hints[count].mult_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );
		}

#if 0
		if ((t->y_hints[count].num_hints == 0) && (t->numyOrus == 0))
#else
		if (t->y_hints[count].hint_array == NULL)
#endif
		{
		t->y_hints[count].hint_array = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
		t->y_hints[count].hint_pix = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );
		t->y_hints[count].offset_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );		
		t->y_hints[count].IntOrus = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );			
		t->y_hints[count].mult_ptr = (F26Dot6 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( F26Dot6) );
		}
	}
	/******************************************************************************/

	
	/***************** Get all X Extra Stroke hints *******************************/
	j=0;
	i=0;
	index = 0;
	val = 0;


	t->x_strokes[0].hint_array	= (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->x_strokes[0].num_hints = 0;
	t->x_strokes[0].num_hints_ml = WIDTH_CHUNK;


	while (i < t->numextraXStroke)
	{

		while ((val == t->extraXStrokeGlyphCount_ptr[j]) && (j < t->numextraXStroke))
		{
			t->x_strokes[index].hint_array[t->x_strokes[index].num_hints] = t->extraXStrokeOrus_ptr[j];
			t->x_strokes[index].num_hints++;
			
			if ((t->x_strokes[index].num_hints) >= t->x_strokes[index].num_hints_ml)
				{
					t->x_strokes[index].num_hints_ml = (int16)(t->x_strokes[index].num_hints_ml + WIDTH_CHUNK);
					t->x_strokes[index].hint_array = (int16 *)tsi_ReAllocMem( t->mem, t->x_strokes[index].hint_array , (t->x_strokes[index].num_hints_ml) * sizeof(int16) );
				}
			j++;
		}
		if (t->x_strokes[index].num_hints == 0)
		{
			tsi_DeAllocMem( t->mem, t->x_strokes[index].hint_array );
		}
	
		i = j;
		index++;
		val = t->hintmarkers_x_ptr[index];

		t->x_strokes[index].hint_array	= (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
		t->x_strokes[index].num_hints = 0;
		t->x_strokes[index].num_hints_ml = WIDTH_CHUNK;

	}
	tsi_DeAllocMem( t->mem, t->x_strokes[index].hint_array );

	/******************************************************************************/


	/***************** Get all Y Extra Stroke hints *******************************/
	j=0;
	i=0;
	index = 0;
	val = 0;


	t->y_strokes[0].hint_array	= (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->y_strokes[0].num_hints = 0;
	t->y_strokes[0].num_hints_ml = WIDTH_CHUNK;


	while (i < t->numextraYStroke)
	{

		while ((val == t->extraYStrokeGlyphCount_ptr[j]) && (j < t->numextraYStroke))
		{
			t->y_strokes[index].hint_array[t->y_strokes[index].num_hints] = t->extraYStrokeOrus_ptr[j];
			t->y_strokes[index].num_hints++;
			
			if ((t->y_strokes[index].num_hints) >= t->y_strokes[index].num_hints_ml)
				{
					t->y_strokes[index].num_hints_ml = (int16)(t->y_strokes[index].num_hints_ml + WIDTH_CHUNK);
					t->y_strokes[index].hint_array = (int16 *)tsi_ReAllocMem( t->mem, t->y_strokes[index].hint_array , (t->y_strokes[index].num_hints_ml) * sizeof(int16) );
				}
			j++;
		}
	
		if (t->y_strokes[index].num_hints == 0)
		{
			tsi_DeAllocMem( t->mem, t->y_strokes[index].hint_array );
		}
		i = j;
		index++;
		val = t->hintmarkers_y_ptr[index];

		t->y_strokes[index].hint_array	= (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
		t->y_strokes[index].num_hints = 0;
		t->y_strokes[index].num_hints_ml = WIDTH_CHUNK;

	}
	tsi_DeAllocMem( t->mem, t->y_strokes[index].hint_array );

	/******************************************************************************/


	/***************** Get all X Extra Edge hints *********************************/
	j=0;
	i=0;
	index = 0;
	val = 0;

	t->x_edges[0].EdgeThresh = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->x_edges[0].EdgeDelta	= (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->x_edges[0].EdgeIndex	= (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->x_edges[0].numEdges = 0;
	t->x_edges[0].numEdges_ml = WIDTH_CHUNK;


	while (i < t->numextraXEdge)
	{

		while ((val == t->extraXEdgeGlyphCount_ptr[j]) && (j < t->numextraXEdge))
		{
			t->x_edges[index].EdgeThresh[t->x_edges[index].numEdges ] = t->extraXEdgeThresh_ptr[j];
			t->x_edges[index].EdgeDelta[t->x_edges[index].numEdges ] = t->extraXEdgeDelta_ptr[j];
			t->x_edges[index].EdgeIndex[t->x_edges[index].numEdges ] = t->extraXEdgeIndex_ptr[j];
			t->x_edges[index].numEdges++;
		
			if ((t->x_edges[index].numEdges) >= t->x_edges[index].numEdges_ml)
				{
					t->x_edges[index].numEdges_ml = (int16)(t->x_edges[index].numEdges_ml + WIDTH_CHUNK);
					t->x_edges[index].EdgeThresh = (int16 *)tsi_ReAllocMem( t->mem, t->x_edges[index].EdgeThresh , (t->x_edges[index].numEdges_ml) * sizeof(int16) );
					t->x_edges[index].EdgeDelta = (int16 *)tsi_ReAllocMem( t->mem, t->x_edges[index].EdgeDelta , (t->x_edges[index].numEdges_ml) * sizeof(int16) );
					t->x_edges[index].EdgeIndex = (int16 *)tsi_ReAllocMem( t->mem, t->x_edges[index].EdgeIndex , (t->x_edges[index].numEdges_ml) * sizeof(int16) );
				}

			j++;
		}

		if (t->x_edges[index].numEdges == 0)
		{
			tsi_DeAllocMem( t->mem, t->x_edges[index].EdgeThresh );
			tsi_DeAllocMem( t->mem, t->x_edges[index].EdgeDelta );
			tsi_DeAllocMem( t->mem, t->x_edges[index].EdgeIndex );
		}
	
		i = j;
		index++;
		val = t->hintmarkers_x_ptr[index];

		t->x_edges[index].EdgeThresh = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
		t->x_edges[index].EdgeDelta	= (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
		t->x_edges[index].EdgeIndex	= (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
		t->x_edges[index].numEdges = 0;
		t->x_edges[index].numEdges_ml = WIDTH_CHUNK;
	
	}
	tsi_DeAllocMem( t->mem, t->x_edges[index].EdgeThresh );
	tsi_DeAllocMem( t->mem, t->x_edges[index].EdgeDelta );
	tsi_DeAllocMem( t->mem, t->x_edges[index].EdgeIndex );

	/******************************************************************************/


	/***************** Get all Y Extra Edge hints *********************************/

	j=0;
	i=0;
	index = 0;
	val = 0;

	t->y_edges[0].EdgeThresh = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->y_edges[0].EdgeDelta	= (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->y_edges[0].EdgeIndex	= (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
	t->y_edges[0].numEdges = 0;
	t->y_edges[0].numEdges_ml = WIDTH_CHUNK;


	while (i < t->numextraYEdge)
	{

		while ((val == t->extraYEdgeGlyphCount_ptr[j]) && (j < t->numextraYEdge))
		{
			t->y_edges[index].EdgeThresh[t->y_edges[index].numEdges ] = t->extraYEdgeThresh_ptr[j];
			t->y_edges[index].EdgeDelta[t->y_edges[index].numEdges ] = t->extraYEdgeDelta_ptr[j];
			t->y_edges[index].EdgeIndex[t->y_edges[index].numEdges ] = t->extraYEdgeIndex_ptr[j];
			t->y_edges[index].numEdges++;
		
			if ((t->y_edges[index].numEdges) >= t->y_edges[index].numEdges_ml)
				{
					t->y_edges[index].numEdges_ml = (int16)(t->y_edges[index].numEdges_ml + WIDTH_CHUNK);
					t->y_edges[index].EdgeThresh = (int16 *)tsi_ReAllocMem( t->mem, t->y_edges[index].EdgeThresh , (t->y_edges[index].numEdges_ml) * sizeof(int16) );
					t->y_edges[index].EdgeDelta = (int16 *)tsi_ReAllocMem( t->mem, t->y_edges[index].EdgeDelta , (t->y_edges[index].numEdges_ml) * sizeof(int16) );
					t->y_edges[index].EdgeIndex = (int16 *)tsi_ReAllocMem( t->mem, t->y_edges[index].EdgeIndex , (t->y_edges[index].numEdges_ml) * sizeof(int16) );

				}

			j++;
		}
	

		if (t->y_edges[index].numEdges == 0)
		{
			tsi_DeAllocMem( t->mem, t->y_edges[index].EdgeThresh );
			tsi_DeAllocMem( t->mem, t->y_edges[index].EdgeDelta );
			tsi_DeAllocMem( t->mem, t->y_edges[index].EdgeIndex );
		}
		i = j;
		index++;
		val = t->hintmarkers_y_ptr[index];

		t->y_edges[index].EdgeThresh = (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
		t->y_edges[index].EdgeDelta	= (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
		t->y_edges[index].EdgeIndex	= (int16 *)tsi_AllocMem( t->mem, WIDTH_CHUNK * sizeof( int16 ) );
		t->y_edges[index].numEdges = 0;
		t->y_edges[index].numEdges_ml = WIDTH_CHUNK;
	
	}
	tsi_DeAllocMem( t->mem, t->y_edges[index].EdgeThresh );
	tsi_DeAllocMem( t->mem, t->y_edges[index].EdgeDelta );
	tsi_DeAllocMem( t->mem, t->y_edges[index].EdgeIndex );


	/***********************************/

	for (i=0; i < t->numHintSets; i++)
	{

		if (t->x_hints[i].num_hints > 0)
		{
			for ( swap = true; swap; )
			{	/* Bubble-Sort it */
				swap = false;
				for ( j = (t->x_hints[i].num_hints -4); j >= 0; j=(j-2)) 
				{
					if ( (long)(t->x_hints[i].hint_array[j]) > 
						(long)(t->x_hints[i].hint_array[j+2]) )
					{
					swap = true;
					temp_hint = t->x_hints[i].hint_array[j];
					t->x_hints[i].hint_array[j] = t->x_hints[i].hint_array[j+2];
					t->x_hints[i].hint_array[j+2]  = temp_hint;


					temp_p = t->x_hints[i].hint_pix[j];
					t->x_hints[i].hint_pix[j] = t->x_hints[i].hint_pix[j+2];
					t->x_hints[i].hint_pix[j+2] = temp_p;

					temp_hint = t->x_hints[i].hint_array[j+1];
					t->x_hints[i].hint_array[j+1] = t->x_hints[i].hint_array[j+3];
					t->x_hints[i].hint_array[j+3]  = temp_hint;


					temp_p = t->x_hints[i].hint_pix[j+1];
					t->x_hints[i].hint_pix[j+1] = t->x_hints[i].hint_pix[j+3];
					t->x_hints[i].hint_pix[j+3] = temp_p;
					}
				}
			}
		}

		if (t->y_hints[i].num_hints > 0)
		{
			for ( swap = true; swap; )
			{	/* Bubble-Sort it */
				swap = false;
				for ( j = (t->y_hints[i].num_hints -4); j >= 0; j=(j-2)) 
				{
					if ( (long)(t->y_hints[i].hint_array[j]) > 
						(long)(t->y_hints[i].hint_array[j+2]) )
					{
						swap = true;
						temp_hint = t->y_hints[i].hint_array[j];
						t->y_hints[i].hint_array[j] = t->y_hints[i].hint_array[j+2];
						t->y_hints[i].hint_array[j+2]  = temp_hint;


						temp_p = t->y_hints[i].hint_pix[j];
						t->y_hints[i].hint_pix[j] = t->y_hints[i].hint_pix[j+2];
						t->y_hints[i].hint_pix[j+2] = temp_p;

						temp_hint = t->y_hints[i].hint_array[j+1];
						t->y_hints[i].hint_array[j+1] = t->y_hints[i].hint_array[j+3];
						t->y_hints[i].hint_array[j+3]  = temp_hint;


						temp_p = t->y_hints[i].hint_pix[j+1];
						t->y_hints[i].hint_pix[j+1] = t->y_hints[i].hint_pix[j+3];
						t->y_hints[i].hint_pix[j+3] = temp_p;
					}
				}
			}
		}

	}

	for (i=0; i < t->numHintSets; i++)
	{

		DoVStrokes( t, 0, t->x_hints[i].num_hints, &(t->x_hints[i].hint_array[0]), 
				&(t->x_hints[i].hint_pix[0]), -1);

		DoHStrokes( t, 0, t->y_hints[i].num_hints, &(t->y_hints[i].hint_array[0]), 
				&(t->y_hints[i].hint_pix[0]), -1);


		if ((t->x_strokes[i].num_hints > 0) || (t->y_strokes[i].num_hints > 0))
		{

			if ((t->x_strokes[i].num_hints + t->x_hints[i].num_hints) >= t->x_hints[i].num_hints_ml)
				{
					t->x_hints[i].num_hints_ml = (int16)(t->x_hints[i].num_hints_ml + WIDTH_CHUNK);
					t->x_hints[i].hint_array = (int16 *)tsi_ReAllocMem( t->mem, t->x_hints[i].hint_array , (t->x_hints[i].num_hints_ml) * sizeof(int16) );
					t->x_hints[i].hint_pix =   (F26Dot6 *)tsi_ReAllocMem( t->mem, t->x_hints[i].hint_pix , (t->x_hints[i].num_hints_ml) * sizeof(F26Dot6) );
					t->x_hints[i].offset_ptr = (F26Dot6 *)tsi_ReAllocMem( t->mem, t->x_hints[i].offset_ptr , (t->x_hints[i].num_hints_ml) * sizeof(F26Dot6) );
					t->x_hints[i].mult_ptr =   (F26Dot6 *)tsi_ReAllocMem( t->mem, t->x_hints[i].mult_ptr , (t->x_hints[i].num_hints_ml) * sizeof(F26Dot6) );
					t->x_hints[i].IntOrus =	   (int16 *)tsi_ReAllocMem( t->mem, t->x_hints[i].IntOrus , (t->x_hints[i].num_hints_ml) * sizeof(int16) );
				}

			if ((t->y_strokes[i].num_hints + t->y_hints[i].num_hints) >= t->y_hints[i].num_hints_ml)
				{
					t->y_hints[i].num_hints_ml = (int16)(t->y_hints[i].num_hints_ml + WIDTH_CHUNK);
					t->y_hints[i].hint_array = (int16 *)tsi_ReAllocMem( t->mem, t->y_hints[i].hint_array , (t->y_hints[i].num_hints_ml) * sizeof(int16) );
					t->y_hints[i].hint_pix =   (F26Dot6 *)tsi_ReAllocMem( t->mem, t->y_hints[i].hint_pix , (t->y_hints[i].num_hints_ml) * sizeof(F26Dot6) );
					t->y_hints[i].offset_ptr = (F26Dot6 *)tsi_ReAllocMem( t->mem, t->y_hints[i].offset_ptr , (t->y_hints[i].num_hints_ml) * sizeof(F26Dot6) );
					t->y_hints[i].mult_ptr =   (F26Dot6 *)tsi_ReAllocMem( t->mem, t->y_hints[i].mult_ptr , (t->y_hints[i].num_hints_ml) * sizeof(F26Dot6) );
					t->y_hints[i].IntOrus =	   (int16 *)tsi_ReAllocMem( t->mem, t->y_hints[i].IntOrus , (t->y_hints[i].num_hints_ml) * sizeof(int16) );
				}



			/* Now going to set up the Extra Edges, Extra Strokes when dealing with PFRs */
			DoExtraStrokes(t, &(t->x_hints[i].num_hints),
					&(t->y_hints[i].num_hints),
					&(t->x_hints[i].hint_array[0]), 
					&(t->x_hints[i].hint_pix[0]),
					&(t->y_hints[i].hint_array[0]), 
					&(t->y_hints[i].hint_pix[0]),
					&(t->x_strokes[i].hint_array[0]),
					&(t->y_strokes[i].hint_array[0]),
					&(t->x_strokes[i].num_hints),
					&(t->y_strokes[i].num_hints ));
		}




		if ((t->x_edges[i].numEdges > 0) || (t->y_edges[i].numEdges > 0))
		{


			if ((t->x_edges[i].numEdges + t->x_hints[i].num_hints) >= t->x_hints[i].num_hints_ml)
				{
					t->x_hints[i].num_hints_ml = (int16)(t->x_hints[i].num_hints_ml + WIDTH_CHUNK);
					t->x_hints[i].hint_array = (int16 *)tsi_ReAllocMem( t->mem, t->x_hints[i].hint_array , (t->x_hints[i].num_hints_ml) * sizeof(int16) );
					t->x_hints[i].hint_pix =   (F26Dot6 *)tsi_ReAllocMem( t->mem, t->x_hints[i].hint_pix , (t->x_hints[i].num_hints_ml) * sizeof(F26Dot6) );
					t->x_hints[i].offset_ptr = (F26Dot6 *)tsi_ReAllocMem( t->mem, t->x_hints[i].offset_ptr , (t->x_hints[i].num_hints_ml) * sizeof(F26Dot6) );
					t->x_hints[i].mult_ptr =   (F26Dot6 *)tsi_ReAllocMem( t->mem, t->x_hints[i].mult_ptr , (t->x_hints[i].num_hints_ml) * sizeof(F26Dot6) );
					t->x_hints[i].IntOrus =	   (int16 *)tsi_ReAllocMem( t->mem, t->x_hints[i].IntOrus , (t->x_hints[i].num_hints_ml) * sizeof(int16) );
				}

			if ((t->y_edges[i].numEdges + t->y_hints[i].num_hints) >= t->y_hints[i].num_hints_ml)
				{
					t->y_hints[i].num_hints_ml = (int16)(t->y_hints[i].num_hints_ml + WIDTH_CHUNK);
					t->y_hints[i].hint_array = (int16 *)tsi_ReAllocMem( t->mem, t->y_hints[i].hint_array , (t->y_hints[i].num_hints_ml) * sizeof(int16) );
					t->y_hints[i].hint_pix =   (F26Dot6 *)tsi_ReAllocMem( t->mem, t->y_hints[i].hint_pix , (t->y_hints[i].num_hints_ml) * sizeof(F26Dot6) );
					t->y_hints[i].offset_ptr = (F26Dot6 *)tsi_ReAllocMem( t->mem, t->y_hints[i].offset_ptr , (t->y_hints[i].num_hints_ml) * sizeof(F26Dot6) );
					t->y_hints[i].mult_ptr =   (F26Dot6 *)tsi_ReAllocMem( t->mem, t->y_hints[i].mult_ptr , (t->y_hints[i].num_hints_ml) * sizeof(F26Dot6) );
					t->y_hints[i].IntOrus =	   (int16 *)tsi_ReAllocMem( t->mem, t->y_hints[i].IntOrus , (t->y_hints[i].num_hints_ml) * sizeof(int16) );
				}


			DoExtraEdges(t, &(t->x_hints[i].num_hints),
					&(t->y_hints[i].num_hints),
					&(t->x_hints[i].hint_array[0]), 
					&(t->x_hints[i].hint_pix[0]),
					&(t->y_hints[i].hint_array[0]), 
					&(t->y_hints[i].hint_pix[0]),
					&(t->x_edges[i].EdgeDelta[0]),
					&(t->y_edges[i].EdgeDelta[0]),
					&(t->x_edges[i].EdgeIndex[0]),
					&(t->y_edges[i].EdgeIndex[0]),
					&(t->x_edges[i].EdgeThresh[0]),
					&(t->y_edges[i].EdgeThresh[0]),
					&(t->x_edges[i].numEdges),
					&(t->y_edges[i].numEdges));

		}


	}


	SetInterpolation( t);

}










void SetupGlobalHints( FFT1HintClass *t, int32 numBlues, int32 BlueFuzz, F16Dot16 BlueScale, int32 BlueShift, int32 *BlueArray,
						int32 StdVW, int32 StdHW, int32 *StemVArray, int32 *StemHArray,
						int32 numVArray, int32 numHArray)
{
	int i;
	int nBlueValues, nBlueZones;
	int ii, jj;
	int16 refOrus, minOrus, maxOrus;
	blueZone_t temp;
	int	 j, swap;

	/* Deal with Blue Zone Infomation */
	nBlueValues = t->numBlueValues = numBlues;
	t->BlueFuzz = BlueFuzz;
	t->BlueScale = BlueScale;
	t->BlueShift = BlueShift;

	t->BlueShiftPix = util_FixMul( t->BlueShift, t->yScale );

	for (i=0; i < numBlues; i++)
	{
	  t->bluevalues[i] = BlueArray[i];
	}

	nBlueZones = (nBlueValues >> 1);

	for (ii=0, jj=0; ii < nBlueValues; ii +=2, jj += 1)
	{
		refOrus = (int16)((ii == 0) ?
			0:
			t->bluevalues[ii]);

		t->pBlueZones[jj].refPix = util_FixMul( refOrus, t->yScale );
		minOrus = (int16)(t->bluevalues[ii] - t->BlueFuzz);

		t->pBlueZones[jj].minPix = util_FixMul( minOrus, t->yScale );
		maxOrus = (int16)(t->bluevalues[ii + 1] + t->BlueFuzz);

		t->pBlueZones[jj].maxPix = util_FixMul( maxOrus, t->yScale ); 
	}

	for ( swap = true; swap; )
	{	/* Bubble-Sort it */
		swap = false;
		for ( j = (nBlueZones - 2); j >= 0; j--) 
		{
			if ( (long)(t->pBlueZones[j].minPix) > 
				 (long)(t->pBlueZones[j+1].minPix) )
			{
				swap = true;
				temp = t->pBlueZones[j];
				t->pBlueZones[j] = t->pBlueZones[j+1];
				t->pBlueZones[j+1]  = temp;

			}
		}
	}
	
	if (t->num_tcb > 0)
	{
	/* Extend the zones to catch edges within 0.25 pixels of ref position */
	if (nBlueZones > 0)
	{
		F26Dot6 thresh, minPix, maxPix, limitPix;

		thresh = 0x10;

		minPix = t->pBlueZones[0].refPix - thresh;

		if (t->pBlueZones[0].minPix > minPix)
			t->pBlueZones[0].minPix = minPix;


		for (jj = 1; jj < nBlueZones; jj++)
		{
			limitPix = (t->pBlueZones[jj - 1].maxPix + t->pBlueZones[jj].minPix ) >> 1;
			maxPix = t->pBlueZones[jj - 1].refPix + thresh;
			if (maxPix > limitPix)
				maxPix = limitPix;

			if (t->pBlueZones[jj - 1].maxPix < maxPix)
				t->pBlueZones[jj - 1].maxPix = maxPix;

			minPix = t->pBlueZones[jj].refPix - thresh;
			if (minPix < limitPix)
				minPix = limitPix;

			if (t->pBlueZones[jj].minPix > minPix)
				t->pBlueZones[jj].minPix = minPix;
		}

		maxPix = t->pBlueZones[nBlueZones - 1].refPix + thresh;
		if (t->pBlueZones[nBlueZones - 1].maxPix < maxPix)
			t->pBlueZones[nBlueZones - 1].maxPix  = maxPix;
	}
	}

	for ( swap = true; swap; )
	{	/* Bubble-Sort it */
		swap = false;
		for ( j = (nBlueZones - 2); j >= 0; j--) 
		{
			if ( (long)(t->pBlueZones[j].minPix) > 
				 (long)(t->pBlueZones[j+1].minPix) )
			{
				swap = true;
				temp = t->pBlueZones[j];
				t->pBlueZones[j] = t->pBlueZones[j+1];
				t->pBlueZones[j+1]  = temp;

			}
		}
	}






	if (t->num_tcb > 0)
		t->suppressOvershoots = (t->yPixelsPerEm < t->BlueScale );
	else
		t->suppressOvershoots = (t->yPixelsPerEm < util_FixMul(t->BlueScale,t->upem) );



	/* Deal with Stem information */


	if (numVArray > 0)
	{
		for (i = 0; i < numVArray; i++)
		{
			t->snapVWVals[i] = StemVArray[i];
		}
		t->numSnapV = numVArray;
	}
	else
		t->numSnapV = 0;

	if (numHArray > 0)
	{
		for (i = 0; i < numHArray; i++)
		{
			t->snapHWVals[i] = StemHArray[i];
		}
		t->numSnapH = numHArray;
	}
	else
		t->numSnapH = 0;

	t->StdHW = StdHW;
	t->StdVW = StdVW;
	

	SetupStemWeights(t, t->xScale , t->snapVWVals , t->numSnapV,
						 t->StdVW  , t->pSnapV, &(t->numSnapVZones));

	SetupStemWeights( t, t->yScale , t->snapHWVals, t->numSnapH,
						t->StdHW  ,t->pSnapH, &(t->numSnapHZones));

	SetupInput( t );
}



void SetupStemWeights( FFT1HintClass *t, F16Dot16 scale, int32 *snapVals,
						int32 numSnapVals, int32 StdW ,stemSnap_t *pSnapZones,
						int32 *numSnapZones)
{

	F26Dot6 thresh, thresh1, thresh2;
	int ii, jj; 
	F26Dot6 refPix, refPixRnd, minPix, maxPix;

	
	/* Setup initial stem snap table */
	thresh = 0x10;

	for (ii=0; ii < numSnapVals; ii++)
	{
	  refPix = util_FixMul( snapVals[ii], scale );
	  minPix = (refPix - thresh);
	  maxPix = (refPix + thresh);

	  /* Ensure that there is no overlap between zones */
	  if ((ii > 0) && (pSnapZones[ii - 1].maxPix > minPix))
	  {
		if (pSnapZones[ii - 1].maxPix > refPix)
			pSnapZones[ii - 1].maxPix = refPix;
		if (minPix < pSnapZones[ii - 1].refPix)
			minPix = pSnapZones[ii - 1].refPix;

		minPix = pSnapZones[ii - 1].maxPix = 
		((pSnapZones[ii - 1].maxPix + minPix)) >> 1;
	  }

	/* Update the stem snap table */
	pSnapZones[ii].maxPix = maxPix;
	pSnapZones[ii].minPix = minPix;
	pSnapZones[ii].refPix = refPix;

	}


	/* Exit if no stem Snap zone */
	if (StdW <= 0)
	{
		*numSnapZones = numSnapVals;
		return;
	}

	/* Compute the main stem snap zones */

	refPix = util_FixMul( StdW, scale);
		
	refPixRnd = (refPix < t->onepix) ?	
				t->onepix :
				(refPix + t->pixrnd) & t->pixfix;

	thresh1 = refPix - 0x17 - 0x00;		
	thresh2 = refPixRnd - 0x2E - 0x00;

	minPix = (thresh1 > thresh2) ? thresh1 : thresh2;

	if (minPix > refPix)
		minPix = refPix;


	thresh1 = refPix + 0x26 + 0x20;		
	thresh2 = refPixRnd + 0x38 + 0x20;

	maxPix = (thresh1 < thresh2) ? thresh1 : thresh2;

	if (maxPix < refPix)
		maxPix = refPix;


	/* Remove the captured stem snap zones from table */
	jj = 0;
	for (ii=0; ii < numSnapVals; ii++)
	{
		if ((pSnapZones[ii].refPix >= minPix) &&
				(pSnapZones[ii].refPix <= maxPix))
		{
			/* Ensure main zone includes the captured zones */
			if (pSnapZones[ii].minPix < minPix)
					minPix = pSnapZones[ii].minPix;
			if (pSnapZones[ii].maxPix > maxPix)
					maxPix = pSnapZones[ii].maxPix;
		}
		else
			pSnapZones[jj++] = pSnapZones[ii];
	}
	numSnapVals = (int32)jj;
		
	/* Make room for main Stem Snap Zone */
	for (
		ii = (numSnapVals - 1); 
		(ii >= 0) && (pSnapZones[ii].refPix > refPix); 
		ii --)
		{
		 pSnapZones[ii + 1] = pSnapZones[ii];
		}

	/* Ensure that the main stem snap zone does not overlap the uncaptured zones */
	if ((ii >= 0) && (minPix < pSnapZones[ii].maxPix))
			minPix = pSnapZones[ii].maxPix;
	ii++;
	if ((ii < numSnapVals) && (maxPix > pSnapZones[ii].minPix))
			maxPix = pSnapZones[ii].maxPix;

	/* Insert the main stem snap zones */
	pSnapZones[ii].minPix = minPix;
	pSnapZones[ii].maxPix = maxPix;
	pSnapZones[ii].refPix = refPix;

	*numSnapZones = (int32)(numSnapVals + 1);

}


void SetInterpolation( FFT1HintClass *t)
{
 F16Dot16 scale;
 F26Dot6  pos; 

 int16 nBasicOrus;
 int16 nOrus;

 int16 *pOrus;
 F26Dot6 *pPix;
 int16 *pIntOrus;
 F16Dot16 *pIntMult;
 F26Dot6 *pIntOffset;

 int dimension;
 int16 nIntOrus;
 int ii, jj, kk, count;
 unsigned char extended;
 int16 deltaOrus;
 int16 orus;
 F26Dot6 pix;


	/* Set all pointers for one dimension */
	scale = t->xScale;
	pos = t->xpos;			


	for (count = 0; count < t->numHintSets; count++)
	{

		nBasicOrus = (int16)(t->x_hints[count].num_hints - t->x_strokes[count].num_hints - t->x_edges[count].numEdges);
		nOrus =		 t->x_hints[count].num_hints;
		pOrus =		 &(t->x_hints[count].hint_array[0]);
		pPix =		 &(t->x_hints[count].hint_pix[0]);
		pIntOrus =	 &(t->x_hints[count].IntOrus[0]);
		pIntMult =	 &(t->x_hints[count].mult_ptr[0]);
		pIntOffset = &(t->x_hints[count].offset_ptr[0]);



		for (dimension = 0;;)		/* For the X and Y .... */
		{
#ifdef DEBUG_PRINT_HINTS
			printf("\n%c  tables\n", (dimension == 0) ? 'X': 'Y');
			printf("   ORUS      PIX     POINTS\n");
	
			for (ii = 0; ii < nOrus; ii++)
			{
				printf("%6d   %9.3f    %6d\n", pOrus[ii], (double)pPix[ii], count);
			}
#endif
			if (nOrus == 0)
			{
				pIntMult[0] = scale;
				pIntOffset[0] = pos;
				nIntOrus = 0;
			}
			else
			{
				/* Merge secondary hints into pre-sorted basic oru list */
	
				/* NOTE:::  This cast was to silence warning!! */
				extended = (unsigned char)(nOrus > nBasicOrus);
				if (extended)
				{
					for (ii = 0; ii < nBasicOrus; ii++)
					{
						pIntOrus[ii] = (int16)ii;
					}
					for (ii = nBasicOrus; ii < nOrus; ii++)
					{
						orus = pOrus[ii];
						for (jj = ii; jj > 0; jj--)
						{
							if (orus >= pOrus[pIntOrus[jj - 1]])
							{
								break;
							}
							pIntOrus[jj] = pIntOrus[jj - 1];
						}
						pIntOrus[jj] = (int16)ii;
					}
				}
	
				/* First entry in interpolation table */
				if (extended)
				{
					kk = pIntOrus[0];
					orus = pIntOrus[0] = pOrus[kk];
					pix = pPix[kk];
				}
				else
				{
					orus = pIntOrus[0] = pOrus[0];
					pix = pPix[0];
				}
				pIntMult[0] = scale;
				pIntOffset[0] = (pix - util_FixMul(orus, scale )); /* Check if we need to round here! */
	
				/* Remaining entries except the last in int table */
	
				for (ii = jj = 1; ii < nOrus; ii++)
				{
					kk = extended ? pIntOrus[ii] : ii;
					deltaOrus = (int16)(pOrus[kk] - orus);
					if (deltaOrus > 0)
					{
						orus = pIntOrus[jj] = pOrus[kk];
						pIntMult[jj] = util_FixDiv((pPix[kk] - pix), deltaOrus);  
						pix = pPix[kk];
						pIntOffset[jj] = (pix - util_FixMul(orus, pIntMult[jj])); /* ??? */
						if ((pIntMult[jj] == pIntMult[jj - 1]) &&
							(pIntOffset[jj] == pIntOffset[jj - 1]))
						{
							pIntOrus[jj - 1] = orus;
						}
						else
						{
							jj++;
						}
					}
				}
	
				/* Last Entry in interpolation table */
				pIntMult[jj] = scale;
				pIntOffset[jj] = (pix - util_FixMul(orus, scale));  /* Do I have to round?? */
				nIntOrus = (int16)(((pIntMult[jj] != pIntMult[jj - 1]) || 
							(pIntOffset[jj] != pIntOffset[jj - 1])) ?
						 	jj :
						 	jj - 1);
			}
#ifdef DEBUG_PRINT_HINTS
			printf("\n%c interpolation table\n", (dimension == 0) ? 'X' : 'Y');
			printf("   ORUS     MULT     OFFSET    RESULT    RESULT2\n");
	
			for (ii = 0; ii < nIntOrus; ii++)
			{
				printf("%6d   %8.4f  %8.4f   %8.4f  %8.4f\n",
					pIntOrus[ii], (double)pIntMult[ii]/65536.0, (double)pIntOffset[ii],
					(double)(util_FixMul(pIntOrus[ii], pIntMult[ii]) +  (double)pIntOffset[ii]),
					(double)(util_FixMul(pIntOrus[ii], pIntMult[ii + 1]) + (double)pIntOffset[ii + 1]));
			}
	
		 		printf("        %8.4f  %8.4f\n",
				 	(double)pIntMult[nIntOrus]/65536.0, (double)pIntOffset[nIntOrus]);
#endif
	
	
			if (dimension == 0)
			{
				t->nxIntOrus_ptr[count] = nIntOrus;
				dimension = 1;
				scale = t->yScale;
				pos = t->ypos;
	
				nBasicOrus = (int16)(t->y_hints[count].num_hints - t->y_strokes[count].num_hints - t->y_edges[count].numEdges);
				nOrus =		 t->y_hints[count].num_hints;
				pOrus =		 &(t->y_hints[count].hint_array[0]);
				pPix =		 &(t->y_hints[count].hint_pix[0]);
				pIntOrus =	 &(t->y_hints[count].IntOrus[0]);
				pIntMult =	 &(t->y_hints[count].mult_ptr[0]);
				pIntOffset = &(t->y_hints[count].offset_ptr[0]);
	
				continue;
			}
			else
			{
				t->nyIntOrus_ptr[count] = nIntOrus;
				break;
			}
		}/* the dimension loop */
	}/* the count loop */
}




void DoVStrokes( FFT1HintClass *t, int32 firstIndex, int32 lastIndex, int16 *xOrus, 
				F26Dot6 *xPix, int iParent)
{
	int ii,jj;
	int16 leftOrus, rightOrus;
	F26Dot6 widthPix;
	F26Dot6 pix;
	int16 orus;


	for (ii = firstIndex; ii < lastIndex; ii+= 2)
	{
		leftOrus = xOrus[ii];
		rightOrus = xOrus[ii + 1];

		/* Compute stroke weight */
	
		widthPix = util_FixMul((rightOrus - leftOrus), t->xScale );
	
		/* Snap to standard stroke weight if within range */
	

		for (jj = 0; jj < t->numSnapVZones; jj++)
		{
			if (widthPix < t->pSnapV[jj].minPix)
			{
				break;
			}
			if (widthPix <= t->pSnapV[jj].maxPix)
			{
				widthPix = t->pSnapV[jj].refPix;
				break;
			}
		}

		/* Round to whole pixels; min 1 pix */
		widthPix = (widthPix < t->onepix) ?
				t->onepix:
				(widthPix + t->pixrnd) & t->pixfix;

		/* Position stroke for optimum centerline accuracy */

		if (iParent >= 0)
		{
			orus = (int16)((leftOrus + rightOrus - xOrus[iParent] - xOrus[iParent + 1]) >> 1);
			pix = (xPix[iParent] + xPix[iParent + 1] - widthPix) >> 1;
		}
		else
		{
			orus = (int16)((leftOrus + rightOrus + 1) >> 1);
			pix =  (0 - widthPix )  >> 1;
		}

		xPix[ii] = ((util_FixMul(orus, t->xScale) + pix + t->pixrnd)) & t->pixfix;
		xPix[ii + 1] = xPix[ii] + widthPix;
	}

}



void DoHStrokes( FFT1HintClass *t, int32 firstIndex, int32 lastIndex, 
				int16 *yOrus, F26Dot6 *yPix, int iParent)
{

	int ii, jj, kk, ll;
	int nBottomZones, nTopZones;
	int alignMode;
	int16 bottomOrus, topOrus;
	F26Dot6 bottomPix, topPix;
	F26Dot6 overshootPix;
	F26Dot6 alignBottomPix, alignTopPix;
	F26Dot6 heightPix;
	int16 orus;
	F26Dot6 pix;


	if ((t->numBlueValues>>1) <= 0)
	{
		nBottomZones = 0;
		nTopZones = 0;
		jj = kk = 0;
	}
	else
	{
		nBottomZones = 1;
		nTopZones = (t->numBlueValues>>1);
		jj = 0;
		kk = 1;
	}


	for (ii = firstIndex; ii < lastIndex; ii += 2)
	{
		bottomOrus = yOrus[ii];
		topOrus = yOrus[ii + 1];
		alignMode = FLOATING;

		/* Align bottom edge of stroke if within bottom zone */
		if (jj < nBottomZones)	/* At least one botton zone active ?? */
		{
			/* Calculate bottom of stroke in unrounded device coordinates */
		
			bottomPix = util_FixMul( bottomOrus , t->yScale );
		
			/* Align bottom edge of stroke if in bottom blue zone */
			while ((jj < nBottomZones) && 
				(bottomPix > t->pBlueZones[jj].maxPix))
			{
				jj++;
			}
			if ((jj < nBottomZones) && 
				(bottomPix >= t->pBlueZones[jj].minPix))
			{
				alignMode |= BOTTOM_ALIGNED;
				overshootPix = t->suppressOvershoots ?
					0:
					((t->pBlueZones[jj].refPix - bottomPix + t->pixrnd)) & t->pixfix;
				alignBottomPix = 
					(((t->pBlueZones[jj].refPix + t->pixrnd) & t->pixfix) - overshootPix);
			}
		}

		/* Align top edge of stroke if within top zone */
		if (kk < nTopZones)	/* At least one top zone active ? */
		{
			/* Calculate top of stroke in unrounded device coordinates */
		
			topPix = util_FixMul( topOrus , t->yScale );

			/* Align top edge of stroke if in the top blue zone */
			while ((kk < nTopZones) && 
				(topPix > t->pBlueZones[kk].maxPix))
			{
				kk++;
			}
			if ((kk < nTopZones) && 
				(topPix >= t->pBlueZones[kk].minPix))
			{
				alignMode |= TOP_ALIGNED;
				if (t->suppressOvershoots)
				{
					overshootPix = 0;
				}
				else
				{
					overshootPix = 
						topPix - t->pBlueZones[kk].refPix;

					overshootPix = 
						((overshootPix < t->pixrnd) &&
						 (overshootPix >= t->BlueShiftPix)) ?
						 t->onepix :
						 (overshootPix + t->pixrnd) & t->pixfix;

				}

				alignTopPix = 
				    (((t->pBlueZones[kk].refPix + t->pixrnd) & t->pixfix)  + overshootPix);
			}
		}

		if (topOrus == bottomOrus) /* Ghost stroke ?? */
		{
			heightPix = 0;
		}
		else
		{
			/* Compute the stroke weight if within range */
		
			heightPix = util_FixMul( (topOrus - bottomOrus) , t->yScale );

			/* Snap to standard stroke weight if within range */
			
			for (ll = 0 ; ll < t->numSnapHZones; ll++)
			{
				if (heightPix  < t->pSnapH[ll].minPix)
				{
					break;
				}
				if (heightPix  <= t->pSnapH[ll].maxPix)
				{
					heightPix = t->pSnapH[ll].refPix;
					break;
				}
			}
			/* Round to whole Pixels; min 1 pix */
			heightPix = (heightPix < t->onepix) ?
			t->onepix:
			((heightPix + t->pixrnd) & t->pixfix);
		}

		/* Position stroke as appropriate for alignment mode */
		switch (alignMode)
		{
		case FLOATING:
			if (iParent >= 0)
			{
				orus = (int16)((bottomOrus + topOrus - yOrus[iParent] - yOrus[iParent + 1]) >> 1);
				pix = (yPix[iParent] + yPix[iParent + 1] - heightPix) >> 1;
			}
			else
			{
				orus = (int16)((bottomOrus + topOrus + 1) >> 1);
				pix = (0 - heightPix + 1) >> 1;
			}

			yPix[ii] = (((util_FixMul( orus , t->yScale ) + pix) + t->pixrnd) & t->pixfix);
			yPix[ii + 1] = yPix[ii] + heightPix;
			break;

		case BOTTOM_ALIGNED:
			yPix[ii] = alignBottomPix;
			yPix[ii + 1] = (alignBottomPix + heightPix);
			break;

		case TOP_ALIGNED:
			yPix[ii] = (alignTopPix - heightPix);
			yPix[ii + 1] = (alignTopPix);
			break;

		case BOTTOM_ALIGNED + TOP_ALIGNED:
			yPix[ii] = alignBottomPix;
			yPix[ii + 1] = alignTopPix;
			break;
		}
	}
}



void DoExtraStrokes( FFT1HintClass *t,
				int16 *num_x, int16 *num_y, 
				int16 *xOrus, F26Dot6 *xPix,
				int16 *yOrus, F26Dot6 *yPix,
				int16 *extraXStroke, int16 *extraYStroke,
				int16 *numExtraX, int16 *numExtraY)
{

	int16 *pOrus;
	F26Dot6 *pPix;
	int16 *pOruCount;
	int16 *pExtraCount;
	int16 *pExtraStroke;
	int dimension;
	int nBasicOrus;
	int kk, jj, ii;


	pOrus = xOrus;
	pPix = xPix;
	pOruCount = num_x;
	pExtraCount = numExtraX;
	pExtraStroke = extraXStroke;

	nBasicOrus = jj = (int)*pOruCount;

	for (dimension = 0;;)
	{
	  kk = 0;
	  for (ii = 0; ii < *pExtraCount; ii+=2)
	  {
		  pOrus[jj] = pExtraStroke[ii];
		  pOrus[jj + 1] = pExtraStroke[ii + 1];

		  while ((kk < nBasicOrus) && (pOrus[jj] > pOrus[kk + 1]))
		  {
			  kk += 2;
		  }

		  if (dimension == 0)
			DoVStrokes( t, jj, jj+2, pOrus, pPix,
				((kk < nBasicOrus) && (pOrus[jj + 1] > pOrus[kk])) ? kk : 0);
		  else
			DoHStrokes( t, jj, jj+2, pOrus, pPix,
				((kk < nBasicOrus) && (pOrus[jj + 1] > pOrus[kk])) ? kk : 0);

		  jj += 2;
	  }

	  if (dimension == 0)
	  {
		    
		    *pOruCount = (int16)jj;
			dimension = 1;
			pOrus = yOrus;
			pPix = yPix;
			pOruCount = num_y;
			nBasicOrus = jj = *pOruCount;
			pExtraCount = numExtraY;
			pExtraStroke = extraYStroke;
			continue;

	  }
	  *pOruCount = (int16)jj;
	  break;
	}
}
			 


static void DoExtraEdges(FFT1HintClass *t,
					int16 *num_x, int16 *num_y, 
					int16 *xOrus, F26Dot6 *xPix,
					int16 *yOrus, F26Dot6 *yPix,
					int16	*extraXEdgeDelta,
					int16	*extraYEdgeDelta,
					int16	*extraXEdgeIndex,
					int16	*extraYEdgeIndex,
					int16	*extraXEdgeThresh,
					int16	*extraYEdgeThresh,
					int16 *numExtraX, int16 *numExtraY)

{
	int16 *pOrus;
	F26Dot6 *pPix;
	int16 *pOruCount;
	int16 *pExtraCount;
	int16 *pExtraThresh;
	int16 *pExtraDelta;
	int16 *pExtraIndex;
	int dimension;
	int jj, ii;
	F26Dot6 deltaPix;
	F26Dot6 threshPix;
	F26Dot6 parentPix;


	pOrus = xOrus;
	pPix = xPix;
	pOruCount = num_x;
	pExtraCount = numExtraX;
	pExtraThresh = extraXEdgeThresh;
	pExtraDelta = extraXEdgeDelta;
	pExtraIndex = extraXEdgeIndex;

	jj = *pOruCount;

	for (dimension = 0;;)
	{
	  for (ii = 0; ii < *pExtraCount; ii++)
	  {
		  pOrus[jj]  =  (int16)(pOrus[pExtraIndex[ii]] + pExtraDelta[ii]);

		  if (dimension == 0)
			deltaPix = util_FixMul(pExtraDelta[ii], t->xScale);
		  else
			deltaPix = util_FixMul(pExtraDelta[ii], t->yScale);

		  threshPix = pExtraThresh[ii] << 2;  
		  if ((deltaPix < threshPix) &&
			  (deltaPix > -threshPix))
		  {
			  deltaPix = 0;
		  }
		  else
		  {
			  deltaPix = 
				  (deltaPix + t->pixrnd) & t->pixfix;
		  }
		  parentPix = pPix[pExtraIndex[ii]];
		  pPix[jj] = (parentPix + deltaPix);
		  jj += 1;
	  }

	  if (dimension == 0)
	  {
		    
		    *pOruCount = (int16)jj;
			dimension = 1;
			pOrus = yOrus;
			pPix = yPix;
			pOruCount = num_y;
			pExtraCount = numExtraY;
			pExtraThresh = extraYEdgeThresh;
			pExtraDelta = extraYEdgeDelta;
			pExtraIndex = extraYEdgeIndex;
			jj = *pOruCount;

			continue;

	  }
	  *pOruCount = (int16)jj;
	  break;
	}
}





void SetScale_FFT1HintClass( FFT1HintClass *t, int32 xPixelsPerEm, int32 yPixelsPerEm )
{
	int32 nScale;
	int32 dScale;


	if ( t != NULL  && (t->xPixelsPerEm != xPixelsPerEm || t->yPixelsPerEm != yPixelsPerEm) )
	{
		t->xPixelsPerEm = xPixelsPerEm;
		t->yPixelsPerEm = yPixelsPerEm;
	
		nScale = xPixelsPerEm << 6;
		dScale = t->upem;
	
	
		t->xScale = util_FixDiv( nScale, dScale );
		nScale = yPixelsPerEm << 6;
		t->yScale = util_FixDiv( nScale, dScale );
	}

}





void ApplyHints_FFT1HintClass( FFT1HintClass *t, int16 count, int16 extracount, GlyphClass *glyph )
{
	if ( t != NULL)
	{
		int i, jj, hcount = 0;
		int16 n;
		int16 *oldx_gptr;
		F26Dot6	*newx_gptr;
		int16 *oldy_gptr;
		F26Dot6	*newy_gptr;
		int ncount;
#ifdef DEBUG_PRINT_HINTS
		F26Dot6 temp_newy_gptr;
		F26Dot6 temp_newx_gptr;
#endif


		FlipContourDirection( glyph, 0 );

		n = count;
	
		newx_gptr = glyph->x;
		newy_gptr = glyph->y;
	 
	    oldx_gptr = glyph->oox;
	    oldy_gptr = glyph->ooy;

#ifdef DEBUG_PRINT_HINTS
		printf("\n");
		printf("INDEX      ORUS     MULT     OFFSET    HINTED    UNHINTED\n");

#endif
		for (i=count; i < (count + extracount); i++)
		{
			newx_gptr[i] = util_FixMul( oldx_gptr[i], t->xScale );
			newy_gptr[i] = util_FixMul( oldy_gptr[i], t->yScale );
		}

		if (t->numHintSets == 0)
		{
			for ( i = 0; i < n; i++ ) 
			{
				newx_gptr[i] = util_FixMul( oldx_gptr[i], t->xScale );
				newy_gptr[i] = util_FixMul( oldy_gptr[i], t->yScale );
			}
		}
		else
		{
			if (t->hintmarkers_x_ptr[hcount] == -999)
			{
				t->hintmarkers_x_ptr[hcount] = n;
				t->hintmarkers_x_ptr[hcount+1] = n;
			}
	
			else
				t->hintmarkers_x_ptr[t->numHintSets] = n;
				
			for ( i = 0; i < n; i++ ) 
			{
	
#ifdef DEBUG_PRINT_HINTS
				newx_gptr[i] = util_FixMul( oldx_gptr[i], t->xScale );
				newy_gptr[i] = util_FixMul( oldy_gptr[i], t->yScale );
#endif
				if (i < t->hintmarkers_x_ptr[hcount+1])
				{
				  if (t->nxIntOrus_ptr[hcount] > 0)
				  {
					for (jj = 0; jj < t->nxIntOrus_ptr[hcount]; jj++)
	                {
						if (oldx_gptr[i] <= t->x_hints[hcount].IntOrus[jj])
	                    {
	                    break;
	                    }
	                }
	
	
					if (jj != t->nxIntOrus_ptr[hcount])
					{
#ifdef DEBUG_PRINT_HINTS
				 		temp_newx_gptr = newx_gptr[i]; 
#endif
						newx_gptr[i] = util_FixMul(oldx_gptr[i], t->x_hints[hcount].mult_ptr[jj]) + 
							t->x_hints[hcount].offset_ptr[jj];
#ifdef DEBUG_PRINT_HINTS
						printf("X - %ld) %6d   %8.4f  %8.4f   %8.4f  %8.4f\n",
							i,
							(int)oldx_gptr[i], (double)t->x_hints[hcount].mult_ptr[jj]/65536.0,
							(double)t->x_hints[hcount].offset_ptr[jj],
							(double)(newx_gptr[i]/64.0),  (double)temp_newx_gptr/64.0);
#endif
					}
					else
					{
#ifdef DEBUG_PRINT_HINTS
				 		temp_newx_gptr = newx_gptr[i] ;
#endif
						newx_gptr[i] = util_FixMul(oldx_gptr[i], t->x_hints[hcount].mult_ptr[t->nxIntOrus_ptr[hcount]]) + 
							t->x_hints[hcount].offset_ptr[t->nxIntOrus_ptr[hcount]];
#ifdef DEBUG_PRINT_HINTS
						printf("X - %ld) %6d   %8.4f  %8.4f   %8.4f   %8.4f\n",
							i,
							(int)oldx_gptr[i], (double)t->x_hints[hcount].mult_ptr[t->nxIntOrus_ptr[hcount]]/65536.0, 
							(double)t->x_hints[hcount].offset_ptr[t->nxIntOrus_ptr[hcount]],
							(double)(newx_gptr[i]/64.0),  (double)temp_newx_gptr/64.0);
#endif
					}
				  }
				  else	
				  {
					newx_gptr[i] = util_FixMul( oldx_gptr[i], t->xScale );
				  }
	
				}	/* the new world order */
				else
				{
					hcount++;
					i--;
				}
	
			}  /* the count to n */
	
	
			hcount = 0;
	
			if (t->hintmarkers_y_ptr[hcount] == -999)
			{
				t->hintmarkers_y_ptr[hcount] = n;
				t->hintmarkers_y_ptr[hcount+1] = n;
			}
	
			else
				t->hintmarkers_y_ptr[t->numHintSets] = n;
	
	
	
			for ( i = 0; i < n; i++ ) 
			{
	
				if (i < t->hintmarkers_y_ptr[hcount+1])
				{
				  if (t->nyIntOrus_ptr[hcount] > 0)
				  {
					for (jj = 0; jj < t->nyIntOrus_ptr[hcount]; jj++)
	                {
						if (oldy_gptr[i] <= t->y_hints[hcount].IntOrus[jj])
	                    {
	                    break;
	                    }
	                }
	
					if (jj != t->nyIntOrus_ptr[hcount])
					{
#ifdef DEBUG_PRINT_HINTS
				 		temp_newy_gptr = newy_gptr[i]; 
#endif
						newy_gptr[i] = util_FixMul(oldy_gptr[i], t->y_hints[hcount].mult_ptr[jj]) + 
							t->y_hints[hcount].offset_ptr[jj];
#ifdef DEBUG_PRINT_HINTS
						printf("Y - %ld) %6d   %8.4f  %8.4f   %8.4f  %8.4f\n",
							i,
							(int)oldy_gptr[i], (double)t->y_hints[hcount].mult_ptr[jj]/65536.0,
							(double)t->y_hints[hcount].offset_ptr[jj],
							(double)(newy_gptr[i]/64.0),
							(double)temp_newy_gptr/64.0);
#endif
					}
					else
					{
#ifdef DEBUG_PRINT_HINTS
				 		temp_newy_gptr = newy_gptr[i] ;
#endif
						newy_gptr[i] = util_FixMul(oldy_gptr[i], t->y_hints[hcount].mult_ptr[t->nyIntOrus_ptr[hcount]]) + 
							t->y_hints[hcount].offset_ptr[t->nyIntOrus_ptr[hcount]];
#ifdef DEBUG_PRINT_HINTS
						printf("Y - %ld) %6d   %8.4f  %8.4f   %8.4f   %8.4f\n",
							i,
							(int)oldy_gptr[i], (double)t->y_hints[hcount].mult_ptr[t->nyIntOrus_ptr[hcount]]/65536.0, 
							(double)t->y_hints[hcount].offset_ptr[t->nyIntOrus_ptr[hcount]],
							(double)(newy_gptr[i]/64.0),  (double)temp_newy_gptr/64.0);
#endif
					}
				  }
				  else	
				  {
					newy_gptr[i] = util_FixMul( oldy_gptr[i], t->yScale );
				  }
	
				}	/* the new world order */
				else
				{
					hcount++;
					i--;
				}
	
	
			} /* the one that goes with n */
  		}
    t->numSnapV = 0;
	t->numSnapH = 0;
	t->numSnapVZones = 0;
	t->numSnapHZones = 0;
	t->numxOrus = 0;
	t->numyOrus = 0;

	t->numxbgcount = 0;
	t->numybgcount = 0;

	t->num_tcb = 0;
	t->numextraXStroke = 0;
	t->numextraYStroke = 0;
	t->numextraXEdge = 0;
	t->numextraYEdge = 0;


	memset((void *)t->ybgcount_ptr, 0, WIDTH_CHUNK * sizeof(int16));
	memset((void *)t->xbgcount_ptr, 0, WIDTH_CHUNK * sizeof(int16));
	memset((void *)t->ygcount_ptr, 0, WIDTH_CHUNK * sizeof(int16));
	memset((void *)t->xgcount_ptr, 0, WIDTH_CHUNK * sizeof(int16));

	tsi_DeAllocMem( t->mem, t->hintmarkers_x_ptr );
	tsi_DeAllocMem( t->mem, t->hintmarkers_y_ptr );
	tsi_DeAllocMem( t->mem, t->nxIntOrus_ptr );
	tsi_DeAllocMem( t->mem, t->nyIntOrus_ptr );


	for (ncount = 0; ncount < t->numHintSets;  ncount++)
	{

		tsi_DeAllocMem( t->mem, t->x_hints[ncount].hint_array );
		tsi_DeAllocMem( t->mem, t->x_hints[ncount].hint_pix );
		tsi_DeAllocMem( t->mem, t->x_hints[ncount].offset_ptr );
		tsi_DeAllocMem( t->mem, t->x_hints[ncount].IntOrus );
		tsi_DeAllocMem( t->mem, t->x_hints[ncount].mult_ptr);

		tsi_DeAllocMem( t->mem, t->y_hints[ncount].hint_array );
		tsi_DeAllocMem( t->mem, t->y_hints[ncount].hint_pix );
		tsi_DeAllocMem( t->mem, t->y_hints[ncount].offset_ptr );
		tsi_DeAllocMem( t->mem, t->y_hints[ncount].IntOrus );
		tsi_DeAllocMem( t->mem, t->y_hints[ncount].mult_ptr);


		if (t->x_strokes[ncount].num_hints != 0)
		{
		tsi_DeAllocMem( t->mem, t->x_strokes[ncount].hint_array );
		}

		if (t->y_strokes[ncount].num_hints != 0)
		{
		tsi_DeAllocMem( t->mem, t->y_strokes[ncount].hint_array );
		}


		if (t->x_edges[ncount].numEdges != 0)
		{
			tsi_DeAllocMem( t->mem, t->x_edges[ncount].EdgeThresh );
			tsi_DeAllocMem( t->mem, t->x_edges[ncount].EdgeDelta );
			tsi_DeAllocMem( t->mem, t->x_edges[ncount].EdgeIndex );
		}

		if (t->y_edges[ncount].numEdges != 0)
		{
			tsi_DeAllocMem( t->mem, t->y_edges[ncount].EdgeThresh );
			tsi_DeAllocMem( t->mem, t->y_edges[ncount].EdgeDelta );
			tsi_DeAllocMem( t->mem, t->y_edges[ncount].EdgeIndex );
		}

	}
	t->numHintSets = 0;
	tsi_DeAllocMem( t->mem, t->x_hints);
	tsi_DeAllocMem( t->mem, t->y_hints);

	tsi_DeAllocMem( t->mem, t->x_strokes);
	tsi_DeAllocMem( t->mem, t->y_strokes);

	tsi_DeAllocMem( t->mem, t->x_edges);
	tsi_DeAllocMem( t->mem, t->y_edges);


	FlipContourDirection( glyph, 1 );

	}
}


void FFT1HintClass_releaseMem(FFT1HintClass *t)
{

	t->numSnapV = 0;
	t->numSnapH = 0;
	t->numSnapVZones = 0;
	t->numSnapHZones = 0;
	t->numxOrus = 0;
	t->numyOrus = 0;

	t->numxbgcount = 0;
	t->numybgcount = 0;

	t->num_tcb = 0;
	t->numextraXStroke = 0;
	t->numextraYStroke = 0;
	t->numextraXEdge = 0;
	t->numextraYEdge = 0;


	memset((void *)t->ybgcount_ptr, 0, WIDTH_CHUNK * sizeof(int16));
	memset((void *)t->xbgcount_ptr, 0, WIDTH_CHUNK * sizeof(int16));
	memset((void *)t->ygcount_ptr, 0, WIDTH_CHUNK * sizeof(int16));
	memset((void *)t->xgcount_ptr, 0, WIDTH_CHUNK * sizeof(int16));

	t->numHintSets = 0;

}

/*
 * The T2KTTClass destructor
 */
void Delete_FFT1HintClass( FFT1HintClass *t )
{

	if ( t != NULL ) 
	{
		tsi_DeAllocMem( t->mem, t->extraXStrokeOrus_ptr );
		tsi_DeAllocMem( t->mem, t->extraYStrokeOrus_ptr  );
		tsi_DeAllocMem( t->mem, t->extraXStrokeGlyphCount_ptr );
		tsi_DeAllocMem( t->mem, t->extraYStrokeGlyphCount_ptr );
	
		tsi_DeAllocMem( t->mem, t->extraXEdgeThresh_ptr );
		tsi_DeAllocMem( t->mem, t->extraYEdgeThresh_ptr  );
		tsi_DeAllocMem( t->mem, t->extraXEdgeDelta_ptr );
		tsi_DeAllocMem( t->mem, t->extraYEdgeDelta_ptr );
	
		tsi_DeAllocMem( t->mem, t->extraXEdgeIndex_ptr );
		tsi_DeAllocMem( t->mem, t->extraYEdgeIndex_ptr  );
		tsi_DeAllocMem( t->mem, t->extraXEdgeGlyphCount_ptr );
		tsi_DeAllocMem( t->mem, t->extraYEdgeGlyphCount_ptr);
	
	
		tsi_DeAllocMem( t->mem, t->xgcount_ptr );
		tsi_DeAllocMem( t->mem, t->ygcount_ptr );
		tsi_DeAllocMem( t->mem, t->xbgcount_ptr );
		tsi_DeAllocMem( t->mem, t->ybgcount_ptr );
		tsi_DeAllocMem( t->mem, t->xOrus_ptr);
		tsi_DeAllocMem( t->mem, t->yOrus_ptr);
	
	
		tsi_DeAllocMem( t->mem, t );
	}
}

#endif /*#ifdef ENABLE_NATIVE_T1_HINTS*/




/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/fft1hint.c 1.11 2000/06/15 19:28:05 reggers release reggers $
 *                                                                           *
 *     $Log: fft1hint.c $
 *     Revision 1.11  2000/06/15 19:28:05  reggers
 *     Added comment header.
 *     Revision 1.10  2000/06/14 21:26:17  reggers
 *     Only extend blue values by a 1/4 pixel for PFRs.
 *     Revision 1.9  2000/06/12 20:54:38  reggers
 *     Borland STRICT warning removal.
 *     Revision 1.8  2000/06/07 19:26:11  mdewsnap
 *     Fixed bad cut-paste in Y hint sort
 *     Revision 1.7  2000/06/07 16:48:26  reggers
 *     Removed some warnings.
 *     Revision 1.6  2000/06/07 15:18:40  mdewsnap
 *     Converted to dynamic storage
 *     Revision 1.5  2000/05/26 20:35:47  mdewsnap
 *     Made many changes for PFRs.
 *     Revision 1.4  2000/05/19 14:38:03  mdewsnap
 *     Repaired bugs in PFR hint code.  Expanded variables to longs.
 *     Revision 1.3  2000/05/17 20:44:06  mdewsnap
 *     Added PFR extraStrokes and ExtraEdge processing
 *     Revision 1.2  2000/05/16 20:01:55  mdewsnap
 *     Added in the PFR hints.
 *     Revision 1.1  2000/04/24 17:21:21  reggers
 *     Initial revision
 *     Revision 1.2  2000/04/24 17:18:15  reggers
 *     Corrected exposed test of ->T1 outside of ENABLE_T1 block.
 *     Revision 1.1  2000/04/19 19:01:45  mdewsnap
 *     Initial revision
 *     Revision 1.3  1999/09/30 15:12:28  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:58:36  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

