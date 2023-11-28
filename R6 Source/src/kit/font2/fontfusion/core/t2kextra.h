/*
 * T2kextra.h
 * Font Fusion Copyright (c) 1989-2000 all rights reserved by Bitstream Inc.
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

 
#ifndef __T2K_EXTRA__
#define __T2K_EXTRA__

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */



/* List of extra T2K capabilities that can be enabled or disabled */
/* # 0 */
/* #define ENABLE_COLORBORDERS */

/* End of list */

#ifdef ENABLE_COLORBORDERS
typedef struct {
	uint8 greyScaleLevel;	/* greyScaleLevel used */
	int32 dX, dY;			/* dX,dY is the thickhness of the border. (Should be 1 or 2 ) */
	uint32 R,G,B;			/* R,G,B, is the color of the character. (All values should be between 0 and 255) */
	uint32 borderR, borderG, borderB;	/* borderR, borderG, borderB is the color of the border.  (All values should be between 0 and 255) */
} T2K_BorderFilerParams;

/*
 * Creates a bordered colored antialiased character.
 *
 * This function can be invoked right after T2K_RenderGlyph().
 * You probably should use T2K_RenderGlyph with GrayScale and in T2K_TV_MODE for best results.
 *
 * t:       is a pointer to the T2K scaler object;
 * params:  is a void pointer that poins at T2K_BorderFilerParams
 *
 */
void T2K_CreateBorderedCharacter( T2K *t, void *params );
#endif /* ENABLE_COLORBORDERS */


#ifdef ENABLE_LCD_OPTION

#define QUANT_VALUE 4
#define LEVEL_COUNT (QUANT_VALUE*3 +1)

/*
 * This trivial routine just drives the implicit gray-pixels
 * on RGB like displays, where the pixels alternate between
 * the different RGB colors. The gray-pixel this routine is
 * driving is 5 pixels wide (shape 1,2,3,2,1) and 1 tall.
 * This implements the Gibson LCD Option for T2K
 */
void T2K_WriteToGrayPixels( T2K *t );
#endif /* ENABLE_LCD_OPTION */

/* #define ENABLE_FLICKER_FILTER */
#ifdef ENABLE_FLICKER_FILTER 
void T2K_FlickerFilterExample( T2K *t, void *params );
#endif


#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __T2K_EXTRA__ */
/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/t2kextra.h 1.6 2000/06/14 21:31:25 reggers release $
 *                                                                           *
 *     $Log: t2kextra.h $
 *     Revision 1.6  2000/06/14 21:31:25  reggers
 *     Removed extraneous LCD_OPTION setting.
 *     Revision 1.5  2000/03/27 22:17:15  reggers
 *     Updates for new LCD mode and functionality
 *     Revision 1.4  1999/10/19 16:17:14  shawn
 *     Added a manifest for a Flicker Filter.
 *     
 *     Revision 1.3  1999/09/30 15:12:04  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:58:16  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
