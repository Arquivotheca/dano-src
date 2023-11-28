/*
 * cachemgr.h
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
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


#ifndef cachemgr_h
#define cachemgr_h
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


#ifdef FF_CM_DOCUMENTATION
-------------------------

Q&A Section:

Q1) What files should I look at for the Cache Manager ?
A1) There are only two files that involve the cache :: "cachemgr.c" and
"cachemgr.h".  "cachemgr.h" contains documentation, a coding example 
and the actual Cache Manager API.  Source code is contained in "cachemgr.c".
-------------------------
Q2) What are the benefits to using a the Cache Manager in Font Fusion ?
A2) Using a cache could speed up the performance of your application.  If
a character exists in the cache then the Cache Manager will deliver the 
bitmap to the calling application instead of Font Fusion having to create
the character each time it is needed.  The trade off is memory.  The more
memory allicated for cache purposes, the more characters can be stored in
the cache.
-------------------------
Q3) How much memory should I allocate for the cache ?
A3) The amount of memory that is given to the Cache Manager at creation time
is all it will use.  It will never allocate more memory on its own.  Keep in 
mind that the amount of memory that is set at creation time will hold all the
characters in the cache as well as the cache framework itself.  Therefore the 
final amount of usable memory for the cache is the total declared at creation
minus the size of the cache management structures.  The amount of memory desired
depends on the purposed uses of the application.  If large 500 line bitmaps will
be created than allocate plenty of memory for the cache manager.  If large grey
scale images are needed than again, allocate over 100K for the cache to use.
The more memory the cache is given, the more characters it can hold.  As it
runs out of space the cache will get rid of the oldest characters that is was 
holding in order to make room for the newly created characters.
-------------------------
Q4) How do I use the Cache Manager ?
A4) There is not much for the application to do with the cache.  It can
create the cache, make render glyph calls through it, flush and delete.
Filter functions are attached to Font Fusion through the cache.
The function calls are:: 

FF_CM_New, FF_CM_Delete, FF_CM_Flush, FF_CM_RenderGlyph and 
FF_CM_SetFilter.  Please see the description of each of these below.
Basically, just call the Cache Manager constructor FF_CM_New specifying
the amount of memeory to use, set a filter tag with FF_CM_SetFilter and
then create characters with FF_CM_RenderGlyph.  That is it!!
-------------------------
Q5) I noticed that T2K, the Font Manager AND the Cache Manager each have
RenderGlyph functions. What is the story?
A5) They were designed that way so they could be independent of each
other and work together.  The real RenderGlyph work is always done in 
T2K Core. If you are using the Cache Manager, the FF_CM_RenderGlyph() 
will first check the cache for the glyph or call another module to render
the glyph into the cache. It will use either the Font Manager RenderGlyph
function or call the T2K core. The Font Manager Render Glyph function will 
look for the requested glyph from among the font fragments of the font, 
and then call the T2K_RenderGlyph function.
-------------------------
Q6) It sounds like a lot of magic! How does the Cache Manager know if
the Font Manager should render a glyph? What is the configuration requirement
for me to make these work together?
A6) There is no configuration requirement! You just build the Font or
Cache Managers and use them at run time. If you "register" a font with 
the Manager, when you create and select a strike, the Font Manager stamps
or marks itself in the T2K Class to let the Cache Manager know it is present. 
If you are using the Cache Manager, it will respect this little stamp, which
consists of enough information for the Cache Manager to use the Font Managers API.
-------------------------
Q7) Tell me more about the setting up of a post-processing filter and why
does this involve the cache manager ?
A7) Font Fusion allows filters to be registered with the core.  Each filter
has a seperate ID.  By setting that filter ID with the cache the ID
will be stored with the created bitmap in the cache.  This is used as further
search criteria when retriving characters.  Filter functions are registered
through the cache manager to the core.  If a filter function has been set up
the core will call this function.


#endif /* FF_CM_DOCUMENTATION */


#ifdef CM_DOCUMENTATION_CODING_EXAMPLE
/* a simple example program using the Cache Manager and T2K Core! */
/* To see Cache Manager instrumentation, add this line to CONFIG.H or cachemgr.h: */
/* #define CM_DEBUG		1	*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "T2K.H"
#include "cachemgr.h"

/*
 *  Constants
 */
#define CACHE_SIZE	20000

/*
 *  Prototypes
 */
int main(void);
static void PrintChar( T2K *scaler );


/*
 *  main: where it all happens, mainly!
 */
int main(void)
{
	/* locals needed for the Cache Manager */
	FF_CM_Class 	*theCache;
	int 			errCode;
	uint32 			font_code = 0; /* only one font, always fontCode 0 */
	uint8 			filterTag = 0;  /* only one filter: none */
	/* locals needed for general T2K Core usage */
	FILE 			*fpID = NULL;
	unsigned long 	length, count;
	unsigned char 	*data;
	T2K_TRANS_MATRIX trans;
	unsigned short 	charCode;
	char 			*string = "AABBCCabcdefghijklmnopqrsabcdefghijklmnopqrsabcdefghijklmnopqrs";
	int				i;
	tsiMemObject	*mem 	= NULL;
	InputStream 	*in 	= NULL;
	sfntClass 		*font = NULL;
	T2K 			*scaler = NULL;
	char			*fName = "TT0003M_.TTF";
	short			fontType = FONT_TYPE_TT_OR_T2K;
	int				fontSize = 24;
	uint8			cmd = T2K_RETURN_OUTLINES | T2K_NAT_GRID_FIT  | T2K_SCAN_CONVERT;
	uint8			greyScaleLevel = BLACK_AND_WHITE_BITMAP;

	printf("\n\n\n");
	printf("**********    ******     **       **\n");
	printf("    **      **     **    **     **\n");
	printf("    **              **   **   **\n");
	printf("    **              **   ** **\n");
	printf("    **             **    ****\n");
	printf("    **            **     ** **\n");
	printf("    **          **       **   **\n");
	printf("    **        **         **     **\n");
	printf("    **      **********   **       **\n\n\n");
	printf ("Hello World, this is a simple Font Fusion Example,\n\n");
	printf ("showing use of the Cache Manager with T2K Core,\n\n");
	printf ("with just printf statements for output,\n\n");
	printf ("from www.bitstream.com !\n\n");

	// Create a new Cache Manager to play around with.
	theCache = FF_CM_New(CACHE_SIZE, &errCode);
	assert( errCode == 0 );
	
		/* configure Cache filterTag for all the characters we will make */
		FF_CM_SetFilter(theCache, 
						filterTag,
						NULL, 
						NULL);
		/* Create the Memhandler object. */
		mem	= tsi_NewMemhandler( &errCode );
		assert( errCode == 0 );
	
		/* Open the font. */
		fpID	= fopen(fName, "rb"); assert( fpID != NULL );
		errCode	= fseek( fpID, 0L, SEEK_END ); assert( errCode == 0 );
		length	= (unsigned long)ftell( fpID ); assert( ferror(fpID) == 0 );
		errCode	= fseek( fpID, 0L, SEEK_SET ); assert( errCode == 0 ); /* rewind */
			
		/* Read the font into memory. */	
		data	= (unsigned char *)malloc( sizeof( char ) * length ); assert( data != NULL );
		count	= fread( data, sizeof( char ), length, fpID ); assert( ferror(fpID) == 0 && count == length );
		errCode	= fclose( fpID ); assert( errCode == 0 );
		/* in = New_NonRamInputStream( mem, fpID, ReadFileDataFunc, length, &errCode  ); */
		
		
			/* Create the InputStream object, with data already in memory */
			in 	= New_InputStream3( mem, data, length, &errCode ); /* */
			assert( errCode == 0 );
	
				/* Create an sfntClass object*/
				font = New_sfntClass( mem, fontType, in, NULL, &errCode );
				assert( errCode == 0 );
				
					/* Create a T2K font scaler object.  */
					scaler = NewT2K( font->mem, font, &errCode );
					assert( errCode == 0 );
						/* 12 point */
						trans.t00 = ONE16Dot16 * fontSize;
						trans.t01 = 0;
						trans.t10 = 0;
						trans.t11 = ONE16Dot16 * fontSize;
						/* Set the transformation */
						T2K_NewTransformation( scaler, true, 72, 72, &trans, true, &errCode );
						assert( errCode == 0 );
						
						for ( i = 0; (charCode = string[i]) != 0; i++ ) {
							/* Create a character */
							printf("\n\n***Here comes the %c ****\n\n", (char)charCode);
							FF_CM_RenderGlyph(theCache,font_code,
											&scaler, charCode, 
											0, 0,
											greyScaleLevel, cmd, &errCode);
							assert( errCode == 0 );
							/* Now draw the char */
							PrintChar( scaler );
							/* Free up memory */
							T2K_PurgeMemory( scaler, 1, &errCode );
							assert( errCode == 0 );
						}
	
	
					/* Destroy the T2K font scaler object. */
					DeleteT2K( scaler, &errCode );
					assert( errCode == 0 );
				
		
				/* Destroy the sfntClass object. */
				Delete_sfntClass( font, &errCode );
				
			/* Destroy the InputStream object. */
			Delete_InputStream( in, &errCode  );
		
		free( data );
		/* Destroy the Memhandler object. */
		tsi_DeleteMemhandler( mem );
	
	FF_CM_Delete(theCache, &errCode);
	
	return 0;
}

/*
 * Print/Display Character Function
 */
static void PrintChar( T2K *scaler )
{
int y, x, k, w, h;
char c;
	w = scaler->width;
	assert( w <= scaler->rowBytes * 8 );
	h = scaler->height;
	
	/* printf("w = %d, h = %d\n", w, h ); */
	k = 0;
	for ( y = 0; y < h; y++ )
	{
		for ( x = 0; x < w; x++ )
		{
			if (scaler->rowBytes == w)
			{		/* greyscale, byte walk, divide values by 12, map to digits and clamp > '9' to '@' */
				c = 
					(char)((scaler->baseAddr[ k + x ] ) ?
							scaler->baseAddr[ k + x ]/12 + '0' : '.');
				if (c > '9')
					c = '@';
			}
			else	/* BLACK_AND_WHITE, fancy bit walk, off = '.' and on = '@' */
				c = 
					(char)((scaler->baseAddr[ k + (x>>3) ] & (0x80 >> (x&7)) ) ?
							'@' : '.');
			printf("%c", c );
		}
		printf("\n");
		k += scaler->rowBytes;
	}
}

#endif	/* CM_DOCUMENTATION_CODING_EXAMPLE */

/************************************************************/
/************************************************************/
/************************************************************/
/************************************************************/
/************************************************************/
/************************************************************/
/***** HERE THE ACTUAL NON-DOCUMENTATION CONTENTS BEGIN *****/
/************************************************************/
/************************************************************/
/************************************************************/
/************************************************************/
/************************************************************/

#include "syshead.h"
#include "t2k.h"


/* Do not change this value!!! */
#define HASHSZ 128


/* Default value for structure alignment */
#ifndef STRUCTALIGN
#define STRUCTALIGN           4
#endif



typedef void *(*FF_FilterSizeFunc)(long width, long height, long *newWidth, long *newHeight);
typedef void *(*FF_FilterFunc)( );

/* Cached Bitmap specifications */

typedef struct cacheSpecs_tag
    {
    int	horizontalMetricsAreValid;
    F16Dot16 xAdvanceWidth16Dot16;
	F16Dot16 yAdvanceWidth16Dot16;
	F16Dot16 xLinearAdvanceWidth16Dot16;
	F16Dot16 yLinearAdvanceWidth16Dot16;
	F26Dot6	 fTop26Dot6;
	F26Dot6	 fLeft26Dot6;
    int	verticalMetricsAreValid;
    F16Dot16 vert_xAdvanceWidth16Dot16;
	F16Dot16 vert_yAdvanceWidth16Dot16;
	F16Dot16 vert_xLinearAdvanceWidth16Dot16;
	F16Dot16 vert_yLinearAdvanceWidth16Dot16;
	F26Dot6	 vert_fTop26Dot6;
	F26Dot6  vert_fLeft26Dot6;
	long width;
	long height;
	long rowBytes;
    int embeddedBitmapWasUsed;
	int longbit;
    } cacheSpecs_t;





typedef struct chardata_hdr
	{
	long len;
	struct chardata_hdr *next, *prev;
	struct chardata_hdr *lruprev, *lrunext;
	cacheSpecs_t cacheSpecs;
	} chardata_hdr;

typedef struct memory_seg
	{
	long len;
	struct memory_seg *next, *prev;
	} memory_hdr;

typedef struct char_desc_tag
	{
	uint32 fontCode;
	uint32 instCode;
	uint16 charCode;
	} char_desc_t;
	
typedef struct cmGlobals_tag
    {
	long cacheSize;
    uint8 *imagedata;
    chardata_hdr *current_char;
    chardata_hdr **hashtable;
    memory_hdr *freelist;
    chardata_hdr *lruhead, *lrutail;
	uint16 FilterTag;
	FF_FilterSizeFunc SetFilterSizeFunc;
	FF_FilterFunc SetFilterFunc;
	tsiMemObject *mem;
	FF_T2K_FilterFuncPtr BitmapFilter;			/* Private reference to bitmap filter function */
	void *filterParamsPtr;						/* Private reference to bitmap filter 2nd parameter */
    } FF_CM_Class; 


/* Cache Manager "Public" function prototypes: */
/*
 *	Cache Manager New Class function
 *		Instantiates the Font Fusion Cache Manager.
 *		Returns the context pointer for the cache or a NULL if not successful.
 *		Possible error codes:
 *			T2K_ERR_MEM_MALLOC_FAILED
 *			T2K_ERR_NULL_MEM
 *			T2K_ERR_MEM_TOO_MANY_PTRS
 *			T2K_ERR_MEM_BAD_LOGIC
 */
FF_CM_Class *FF_CM_New(
						long sizeofCache,			/* size of cache, including FF_CM_Class */
						int *errCode);				/* to return possible errors */

/*
 *	Cache Manager Delete Class function
 *		Destroys the cache manager context and frees the memory used to hold the cache
 *		Currently does not return any error code
 */
void FF_CM_Delete(
						FF_CM_Class *theCache,		/* Cache class pointer returned from FF_CM_New() */
						int *errCode);				/* to return possible errors */

/*
 *	Cache Manager Cache Flush function
 *		Reinitializes the cache, emptying its contents
 *		Possible error codes:
 *			T2K_ERR_NULL_MEM
 */
void FF_CM_Flush(
    					FF_CM_Class *cache,			/* Cache class pointer returned from FF_CM_New() */
    					int *errCode);				/* to return possible errors */


/*
 *	Cache Manager Set Filter Tag Code and filter function pointers
 *		Set parameters related to filtering. These parameters will be stored and applied
 *		to new characters that are being created. The FilterTag is a component of the search
 *		criteria for finding chracters in the cache. The BitmapFilter and the parameter
 *		block are responsibility of the application. The BitmapFilter interface is specified
 *		by the T2K core and takes a void pointer to a T2K scaler, and a void pointer to
 *		some arbitrary parameter block used by the filter function itself.
 *		The filter function will be called by the core after preparing a source image
 *		which it *will not* put into the cache. The filter function will find this image
 *		in the T2K scaler, and can request cache memory for the target image just as the
 *		T2K Core code does. 
 */
void FF_CM_SetFilter(
						FF_CM_Class *theCache,		/* Cache class pointer returned from FF_CM_New() */
						uint16 theFilterTag,		/* numeric tag for identifying filtering done
														by BitmapFilter() using the filterParams block */
						FF_T2K_FilterFuncPtr BitmapFilter,	/* function pointer to T2K filter spec */
						void *filterParamsPtr);				/* Pointer to optional parameter block the BitmapFilter */

/*
 *	Cache Manager Render Glyph function
 *		Searches the cache based on the input parameters. If the character is found in the cache
 *		then the appropriate fields in the T2K scaler structure are updated.  If the character
 *		is not in the cache then the cache manager begins the sequence to have the character created
 *		and placed in the cache. The scaler is then updated. A nonzero result is returned if everything
 *		was successful.  Parameters passed into the function include the scaler that is to be updated
 *		as well as a pointer to the cache.  The fontCode and charCode refer to the desired character
 *		in a particular typeface.  The FracPenDelta fields are required by the core. They have valid
 *		values from 0 to 63 and refer to subpixel resolution.  The greyScaleLevel is another field
 *		needed by the core.  It refers to what output mode the core should operate in.
 *		If the Font Fusion Font Manager is active, the Cache Manager calls for the Font Manager
 *		to generate any characters not in the cache. Otherwise, it calls directly to the T2K Core.
 *		The cmd is used to set the desired level of hinting in the core.
 *		Possible error codes:
 *			FF_FM_ERR_FONT_CODE_ERR			( if Font Fusion Font Manager is active  )
 *			Error set by setjmp()
 */
void FF_CM_RenderGlyph(
						FF_CM_Class *theCache,		/* Cache class pointer returned from FF_CM_New() */
						uint32 font_code,			/* integer code identifying the font	*/
						T2K **theScaler,			/* a T2K scaler context in which the font is current */
						long char_code, 			/* the character code to render */
						int8 xFracPenDelta, 		/* sub-pixel position in x (0...63) */
						int8 yFracPenDelta, 		/* sub-pixel position in y (0...63) */
						uint8 greyScaleLevel,		/* BLACK_AND_WHITE_BITMAP, GREY_SCALE_BITMAP_HIGH_QUALITY ... */
						uint16 cmd,					/* T2K_NAT_GRID_FIT, T2K_GRID_FIT, T2K_TV_MODE, T2K_SCAN_CONVERT ... */  
						int *errCode);				/* to return possible errors */


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* ifndef cachemgr_h */
/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/CacheManager/rcs/cachemgr.h 1.15 1999/10/18 16:44:24 mdewsnap release $
 *                                                                           *
 *     $Log: cachemgr.h $
 *     Revision 1.15  1999/10/18 16:44:24  mdewsnap
 *     Changed include files to lower case.
 *     Revision 1.14  1999/09/30 13:15:54  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.13  1999/09/20 16:40:09  reggers
 *     Removed single quote items in comment blocks that bothered both
 *     compilers (cc and gcc) on Linux.
 *     Revision 1.12  1999/07/30 19:44:21  mdewsnap
 *     Remover returned error tag regarding no generated character.
 *     Revision 1.11  1999/07/23 17:51:26  sampo
 *     newline added to end of file.
 *     Revision 1.10  1999/07/19 19:40:32  sampo
 *     Error/warning cleanup
 *     Revision 1.9  1999/07/16 14:50:17  mdewsnap
 *     Added warning to the HASHSZ variable
 *     Revision 1.8  1999/07/15 13:20:46  mdewsnap
 *     Added in a Q and A section....
 *     Revision 1.7  1999/07/09 21:02:15  sampo
 *     Corrected the filter spec for FF_CM_SetfilterTag
 *     Revision 1.6  1999/07/08 21:09:30  sampo
 *     Cleanup. Moved static prototypes to cachemgr.c. Added lots
 *     of comments. Renamed FF_FM_Destroy to FF_CM_Delete.
 *     Revision 1.5  1999/07/08 16:12:24  sampo
 *     Added header and footer comment blocks.
 *     Inserted demonstration code,
 *                                                                           *
 *     AUTHOR: Mike Dewsnap                                                 *
******************************************************************************/
/* EOF cachemgr.h */
