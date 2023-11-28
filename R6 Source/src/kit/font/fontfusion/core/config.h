/*
 * Config.h
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

/***********************************************/
#ifndef __T2K_CONFIG__
#define __T2K_CONFIG__

#ifdef UNUSED
#undef UNUSED
#endif
#define UNUSED(x) ((void)(x))


/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/**** **** **** BEGIN configuration defines  #1 --- #34  **** **** ****/
/* The T2K client has to define the meaning of these 3 functions */
/*** #1 ***/
#define CLIENT_MALLOC( size )			malloc( size )
/* #define CLIENT_MALLOC( size )			AllocateTaggedMemoryNilAllowed(n,"t2k") */
/*** #2 ***/
#define CLIENT_FREE( ptr )				free( ptr )
/* #define CLIENT_FREE( ptr )				FreeTaggedMemory(p,"t2k") */
/*** #3 ***/
#define CLIENT_REALLOC( ptr, newSize )	realloc( ptr, newSize )
/* #define CLIENT_REALLOC( ptr, newSize )	ReallocateTaggedMemoryNilAllowed(ptr, size, "t2k") */

/* #define CLIENT_REALLOC( ptr, newSize )	ReallocateTaggedMemoryNilAllowed(ptr, size, "t2k") */

/*** #4 ***/
/* Here the client can optionally redefine assert, by adding two lines according to the below example  */
/* #undef assert  (line1) */
/* Just leave it for some clients, OR	*/
/* #define assert(cond) 				CLIENT_ASSERT( cond ), OR for a _FINAL_ build _ALWAYS_ define as NULL for maximum speed 	*/
/* #define assert(cond) 				NULL					*/

#undef assert
#if SUPPORTS_FONTFUSION_SEAT_BELTS
int _debuggerAssert(const char *, int, char *);
#define assert(cond) (!(cond) ? _debuggerAssert(__FILE__,__LINE__, #cond) : (int)0)
#else
#define assert(cond) 				NULL
#endif

/*
#undef assert
#define assert(cond) 				NULL	
*/

/*** Start of optional features #5 --- #29 ***/
/* The optional features increase ROM/RAM needs, so only enable them if you are using them */
/*
 * This enables the following T2K functions/methods:
 * T2K_GetGlyphIndex(),T2K_MeasureTextInX(),T2K_GetIdealLineWidth,T2K_LayoutString()
 */
/*** #5 ***/
/* #define 								ENABLE_LINE_LAYOUT */

/*** #6 ***/
/* #define 								ENABLE_KERNING */

/*** #7 we consume 8 * somesize bytes for the cache ***/
/* #define 								LAYOUT_CACHE_SIZE somesize */
/*
 * This just speeds up T2K_MeasureTextInX()
 * It only makes sense to enable if ENABLE_LINE_LAYOUT and ENABLE_KERNING is enabled
 * and you are using T2K_MeasureTextInX().
 */
/* #define 								LAYOUT_CACHE_SIZE 149 */

/*** #8 ***/
/* See more info in T2K.H */
/* #define 								ALGORITHMIC_STYLES */

/*** #9 Always enable if you need Type 1 font support ***/
#define 								ENABLE_T1 

/*** #10 If you have enabled Type 1 support and also need Mac specific Type 1 then also enable this ***/
/* #define 								ENABLE_MAC_T1 */

/*** #11 Always enable if you need CFF font support ***/
/* #define 								ENABLE_CFF */
#define 								ENABLE_CFF

/*** #12 Always enable if you need to be able to read entropy encoded T2K fonts (for compact Kanji fonts) ***/
/* #define 								ENABLE_ORION */

/*** #13 enable if you need non RAM/ROM resident fonts. Allows you to leave the fonts on the disk/server etc. ***/
/* #define 								ENABLE_NON_RAM_STREAM */
#define 								ENABLE_NON_RAM_STREAM

/* 3 Scan-converter bitmap compile time configuration options (14,15, and 16): */

/*** #14 enable if you want to use a non-zero winding rule in the scan-converter instead of even-odd fill ***/
/* The strongly recommended setting is to leave it ON (defined)*/
/* See info in T2K.H */
/* #define USE_NON_ZERO_WINDING_RULE */
#define USE_NON_ZERO_WINDING_RULE 

/*** #15 ***/
/* If defined the scan-converter rowbytes will be a 4 byte multiple, otherwise it is just byte aligned */
/* The normal setting is to leave it OFF (not defined) */
/* Defining it consumes more memory for each bitmap */
/* #define MAKE_SC_ROWBYTES_A_4BYTE_MULTIPLE */

/*** #16 ***/
/* The normal setting is to leave it OFF (not defined) */
/* When defined the y-axis goes up and not down as is normal in graphics system with a top left 0,0 origin */
/* #define REVERSE_SC_Y_ORDER */

/*** #17 Only enable if you need embedded bitmap font support ***/
#define 								ENABLE_SBIT

/*** #18 Only enable if you need native TT hint support format ***/
/* #define								ENABLE_NATIVE_TT_HINTS */
#define								ENABLE_NATIVE_TT_HINTS

/*** #19 Only enable if you want run time auto-hinting/gridding ability ***/
/*** If you only need for instace TV_MODE then you do not need to enable this ***/
/* #define 								ENABLE_AUTO_GRIDDING */
#define 								ENABLE_AUTO_GRIDDING

/*** #20 Only enable if you need Font Fusion stroke font support ***/
/* #define								ENABLE_T2KS */
#define									ENABLE_T2KS

/*** #21 Only enable if you intend to use the T2K_LCD_MODE_2 or T2K_TV_MODE_2 mode ***/
/* #define 								ENABLE_AUTO_GRIDDING_CORE */
#define 								ENABLE_AUTO_GRIDDING_CORE

/*** #22 Only enable if you need Webfont/TrueDoc PFR font support ***/
/* #define 								ENABLE_PFR */

/*** #23 
 * Define IF 
 * *ptr16Bit++ = (uint6)data16;
 * is slower than 
 * *ptr8Bit++ = (uint8)data1; *ptr8Bit++ = (uint8)data2;
 * 
 * This should be define on for instance the MIPS CPU
 * where word writes are Really BAD. Basically, it can only write bytes or longs
 * so word writes compile into consecutive byte writes, PLUS the shifting and masking
 * to make them work!
 ****/
/* #define 								MY_CPU_WRITES_WORDS_SLOW */

/*** #24 
 *
 * Define IF you need increased TrueType black & white low resolution bitmap compatibility.
 *
 * If you do not need LOW resolution TrueType black & white bitmap compatability then do not define this.
 * If you are only using gray scale output then there is no need to define this.
 * Defining this will make FontFusion run somewhat slower as well as slightly increase code size.
 * The performance loss from defining this is about 4% - 5% at 12 ppem and the code size increase is about 2 Kbytes.
 *
 ***/
#define 								ENABLE_MORE_TT_COMPATIBILITY

/*** #25 Only enable if you intend to use the LCD modes ***/
/* #define 								ENABLE_LCD_OPTION */

/*** #26 ENABLE_PCL for PCLeo support */
/* #define ENABLE_PCL */

/*** #27 ENABLE_PCLETTO for PCLetto support */
/* #define ENABLE_PCLETTO */

/*** #28 ENABLE_SPD for Speedo support */
/* #define ENABLE_SPD */

/* This enables the direct stroke to bitmap converter code. It provides a 3X
   speed boost at low sizes for stroke fonts!!!. This makes stroke fonts
   render faster than outline fonts. It however uses approximations that 
   become visible at higher sizes. The define actives the code and defines the
   largest size that is allowed to use the fast stroke conversion process.
   We do not recommend that it is used above 32 ppem.
   The code size increase from this is about 4.5 Kbytes.
*/
/* #29 #define ENABLE_STRKCONV 32 */
#define ENABLE_STRKCONV 32

/***
 * Enables the T2K_ConvertGlyphSplineType() method.
 * The code size increase from this is about 7 Kbytes.
 * Only enable if you need outline curve conversion capability!
 ***/
/* #30 #define ENABLE_FF_CURVE_CONVERSION */
/* #define ENABLE_FF_CURVE_CONVERSION */

/***
 * Enables the native Type 1 and PFR hint interpreter.
 * Only enable this if you intend to use Type 1 or PFR fonts with native hints.
 * Native hints are only used by T2K_NAT_GRID_FIT and the LCD4 mode for T2K_RenderGlyph.
 * For instance if you only intend to use PFR fonts using TV_MODES for T2K_RenderGlyph
 * then you do not need to enable it. Enabling this means an increase
 * in code size, RAM usage as well as a slower execution.
 ***/
/* #31 #define ENABLE_NATIVE_T1_HINTS */

/***
 * Enables optional support the TrueType gasp table.
 * When enabled the T2K_GaspifyTheCmds() method lloks at the gasp table.
 ***/
/* #32 #define ENABLE_GASP_TABLE_SUPPORT */
#define ENABLE_GASP_TABLE_SUPPORT

/* #XX Research on a future T2K format, DO NOT ENABLE!!! */
/* #define								ENABLE_T2KE */


/***
  * Enabling this enables FontFusion to deal better with corrupt font.
  * The downside of this is that it produces some more code and FontFusion will run a little slower.
  * On an embedded system with a known to be good font set, do not enable this.
  * In a desk-top like situation when FontFusion will encounter random fonts -- some of them corrupt --
  * please do enable this.
  ***/
/* 33 #define USE_SEAT_BELTS */
#if SUPPORTS_FONTFUSION_SEAT_BELTS
#define USE_SEAT_BELTS
#endif

/***
 * Enables optional support for Type1 fonts in ROM.
 * If your application will never have a ROM based Type1 font,
 * you may save some run time memory allocation by
 * disabling this option.
 ***/
/* #34 #define ROM_BASED_T1 */
/* #define ROM_BASED_T1 */

/*** End of optional features ***/
/**** **** **** END configuration defines  #1 --- #34    **** **** ****/
/* The T2K client is not supposed to change anything else in here beside items #1 -- #20 */
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

#ifdef ENABLE_LCD_OPTION
/* #define DRIVE_8BIT_LCD */
#define DRIVE_8BIT_LCD
#endif

/* Below we just have internal, non-user-configurable stuff */
#define ENABLE_WRITE
#define ENABLE_PRINTF


/* Should ALWAYS be define when T2K is used as a font engine */
/* #define T2K_SCALER */
#define T2K_SCALER

/* #define ENABLE_T2KE_EDITING */



#ifdef T2K_SCALER
#	ifdef ENABLE_AUTO_GRIDDING
#		define ENABLE_ALL_AUTO_HANDG_CODE
#	endif
	
#	undef ENABLE_WRITE
#	undef ENABLE_PRINTF
#else
	/* Can not be defined at the same time as ENABLE_AUTO_GRIDDING */
#	define ENABLE_AUTO_HINTING 
#	define ENABLE_ALL_AUTO_HANDG_CODE
#	ifdef ENABLE_AUTO_GRIDDING
#		undef ENABLE_AUTO_GRIDDING
#	endif
#	ifdef ENABLE_NATIVE_TT_HINTS
#		undef ENABLE_NATIVE_TT_HINTS
#	endif
#endif /* T2K_SCALER */

#ifdef ENABLE_ALL_AUTO_HANDG_CODE
#	ifndef ENABLE_AUTO_GRIDDING_CORE
#		define ENABLE_AUTO_GRIDDING_CORE
#	endif
#endif


/* #define SAMPO_TESTING_T2K , should not be defined in a release going out from Type Solutions, Inc. */
#ifdef SAMPO_TESTING_T2K
#define ENABLE_WRITE
#define ENABLE_PRINTF
#endif /* SAMPO_TESTING_T2K */


#ifdef ENABLE_T1
#	define T1_OR_T2_IS_ENABLED	
#endif

#ifdef ENABLE_CFF
#	ifndef T1_OR_T2_IS_ENABLED
#		define T1_OR_T2_IS_ENABLED
#	endif
#endif

#ifdef ENABLE_PFR
#	ifndef T1_OR_T2_IS_ENABLED
#		define T1_OR_T2_IS_ENABLED
#	endif
#endif

#ifdef ENABLE_PCL
#	ifndef T1_OR_T2_IS_ENABLED
#		define T1_OR_T2_IS_ENABLED
#	endif
#endif

#ifdef ENABLE_SPD
#	ifndef T1_OR_T2_IS_ENABLED
#		define T1_OR_T2_IS_ENABLED
#	endif
#endif

#if defined (ENABLE_STRKCONV) && !defined (ENABLE_T2KS)
#	undef ENABLE_STRKCONV
#endif

#ifdef ENABLE_T1
#	define ENABLE_HASH_CLASS
#endif
#ifdef ENABLE_CFF
#	ifndef ENABLE_HASH_CLASS
#		define ENABLE_HASH_CLASS
#	endif
#endif


/****      End of configuration defines     ****/
/***********************************************/

/**********************************************************************/
/**********************************************************************/
/*            Error Checking for illegal cofigurations                */
/**********************************************************************/
/**********************************************************************/
#if defined(ENABLE_MAC_T1) && (!defined(ENABLE_T1))
#error ILLEGAL_CONFIGURATION_OPTION_IN_CONFIG_DOT_H
#endif
#ifdef ENABLE_T2KE
#error CONFIGURATION_NOT_SUPPORTED_IN_CONFIG_DOT_H
#endif
#endif /* __T2K_CONFIG__ */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                            *
 *     $Header: R:/src/FontFusion/Source/Core/rcs/config.h 1.31 2001/05/04 21:04:34 reggers Exp $
 *                                                                            *
 *     $Log: config.h $
 *     Revision 1.31  2001/05/04 21:04:34  reggers
 *     Cosmetic correction only.
 *     Revision 1.28  2001/04/26 19:27:29  reggers
 *     Fixed a missing #endif
 *     Revision 1.27  2001/04/26 15:23:50  reggers
 *     Enable ENABLE_HASH_CLASS if T1 of CFF enabled.
 *     Revision 1.26  2001/04/25 21:57:09  reggers
 *     Moved ROM_BASED_T1 option here from t1.c
 *     Revision 1.25  2001/04/24 21:56:26  reggers
 *     Added option #32 for GASP table support. (Sampo)
 *     Revision 1.24  2000/07/11 17:31:20  reggers
 *     Set STRKCONV to 32 lines as default.
 *     Automatically set T1_OR_T2_IS_ENABLED on ENABLE_SPD
 *     Automatically unset ENABLE_STRKCONV if ENABLE_T2KS not set.
 *     Revision 1.23  2000/06/16 21:39:14  reggers
 *     Fixed a comment.
 *     Revision 1.22  2000/06/16 17:17:54  reggers
 *     Made ENABLE_NATIVE_T1_HINTS user config option.
 *     Revision 1.21  2000/05/17 17:05:56  reggers
 *     Cleanup extra space before #define
 *     Revision 1.20  2000/05/12 19:56:17  reggers
 *     Made legacy settings consistent with last release.
 *     Revision 1.19  2000/05/11 13:36:38  reggers
 *     Added the outline curve conversion as an option. (Sampo)
 *     Revision 1.18  2000/04/24 17:10:50  reggers
 *     Added new stroke converter size threshold option (#29).
 *     Revision 1.17  2000/04/19 18:59:28  mdewsnap
 *     Added in ENABLE_T1_NATIVE_HINTS
 *     Revision 1.16  2000/03/27 22:16:45  reggers
 *     Updates for new LCD mode and functionality
 *     Revision 1.14  2000/01/20 16:19:41  reggers
 *     Set T1_OR_T2_IS_ENABLED when ENABLE_PCL is enabled.
 *     Revision 1.13  2000/01/20 15:48:47  reggers
 *     Got rid of ENABLE_HP_PRINTER: unneedeed.
 *     Revision 1.12  2000/01/18 20:51:26  reggers
 *     Improve indenting of #'s
 *     Added options for PCL and PCLETTO.
 *     Revision 1.11  2000/01/17 14:31:27  reggers
 *     Leave ENABLE_PCL and ENABLE_PCLETTO defaulted to off.
 *     Revision 1.10  2000/01/14 18:19:46  reggers
 *     Added new #define for ENABLE_PCLETTO.
 *     Revision 1.9  1999/12/23 22:02:51  reggers
 *     New ENABLE_PCL branches. Rename any 'code' and 'data' symbols.
 *     Revision 1.8  1999/12/09 22:14:04  reggers
 *     Added new ENABLE_MORE_TT_COMPATIBILITY option.
 *     Revision 1.7  1999/10/29 15:04:59  jfatal
 *     Check for a couple of bad (or unsupported) configuration and 
 *     return a compiler error.
 *     Revision 1.6  1999/10/19 16:20:08  shawn
 *     Changed UNUSED(x) macro to '((void)(x))'.
 *     
 *     Revision 1.5  1999/09/30 15:10:23  jfatal
 *     Added correct Copyright notice.
 *                                                                            *
 ******************************************************************************/

