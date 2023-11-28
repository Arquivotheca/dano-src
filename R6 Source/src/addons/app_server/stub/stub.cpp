/* ++++++++++
	FILE:	stub.cpp
	REVS:	$Revision$
	NAME:	bronson
	DATE:	Wed Oct 30 15:26:04 PST 1996
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

#define DEBUG 1
#include <BeBuild.h>
#include <GraphicsCard.h>
#include <Debug.h>
#include "bootscreen.h"
//#include "priv_syscalls.h"
extern "C" uint32 _kget_default_screen_info_(screen *);
extern "C" void _kdprintf_(const char *, ...);
//#define dprintf _kdprintf_
#define dprintf 

screen gScreen;
typedef screen clone_info;


static long UnsupportedCall( char *name )
{
	dprintf( "%s: ignored, returning B_ERROR\n", name );
	return B_ERROR;
}


static long OpenGraphicsCard( graphics_card_spec *spec )
{
	status_t result;
	result = _kget_default_screen_info_( &gScreen );
	gScreen.base = spec->screen_base;
	return result;
}


static void CloseGraphicsCard( void )
{
	// nothing to do
}


static long SetIndexedColor( indexed_color *color )
{
	// This would probably generate too much confusion
	// return UnsupportedCall( "B_SET_INDEXED_COLOR" );
	
	// I wonder if I should check to see if the color
	// matches the color already in the palette.
	
	return B_NO_ERROR;
}


static long GetGraphicsCardHooks( graphics_card_hook *hook )
{
	int i;
	
	//
	//	We don't support any hooks.
	//
	
	for( i=0; i<B_HOOK_COUNT; i++ ) {
		hook[i] = NULL;
	}
	
	return B_NO_ERROR;
}


static long GetGraphicsCardInfo( graphics_card_info *info )
{
	info->version	= 2;
	info->id		= 0;
	
	info->height		= gScreen.height;
	info->width			= gScreen.width;
	info->bits_per_pixel= gScreen.depth;
	info->frame_buffer	= gScreen.base;
	info->bytes_per_row = gScreen.rowbyte;
	info->flags			= 0;
	*(long*)&info->rgba_order = 'argb';
	
	return B_NO_ERROR;
}


static long GetRefreshRates( refresh_rate_info *refresh )
{
	refresh->min = gScreen.refresh;
	refresh->max = gScreen.refresh;
	refresh->current = gScreen.refresh;
	
	return B_NO_ERROR;
}


static ulong GetOurSpaces( void )
{
	//
	//	This function returns the screen space that most
	//	closeley matches what we are currently displaying.
	//  So, if the screen is displaying 832x624, it returns
	//  800x600.  If it can't find a match, it returns zero.
	//
	
	struct {
		int depth;
		int width;
		int height;
		ulong space;
	} const static space_lut[] = {
		8, 640, 480, B_8_BIT_640x480,
		8, 800, 600, B_8_BIT_800x600,
		8, 1024, 768, B_8_BIT_1024x768,
		8, 1280, 1024, B_8_BIT_1280x1024,
		8, 1600, 1200, B_8_BIT_1600x1200 ,
		16, 640, 480, B_16_BIT_640x480,
		16, 800, 600, B_16_BIT_800x600,
		16, 1024, 768, B_16_BIT_1024x768,
		16, 1280, 1024, B_16_BIT_1280x1024,
		16, 1600, 1200, B_16_BIT_1600x1200,
		32, 640, 480, B_32_BIT_640x480,
		32, 800, 600, B_32_BIT_800x600,
		32, 1024, 768, B_32_BIT_1024x768,
		32, 1280, 1024, B_32_BIT_1280x1024,
		32, 1600, 1200, B_32_BIT_1600x1200,
    	8, 1152, 900, B_8_BIT_1152x900,
    	16, 1152, 900, B_16_BIT_1152x900,
    	32, 1152, 900, B_32_BIT_1152x900,
		// not supporting B_8_BIT_640x400
	};
	
	int i, besti;
	ulong diff, bestdiff;
	ulong screen_area;
	long err;
	
		
	screen_area = (ulong)gScreen.width * (ulong)gScreen.height;
	
	dprintf( "Finding nearest match to %ld x %ld (area: %ld)\n",
		(ulong)gScreen.width, (ulong)gScreen.height, screen_area );
	
	bestdiff = 0xFFFFFFFF;
	besti = -1;
	for( i=0; i<sizeof(space_lut)/sizeof(space_lut[0]); i++ ) {
		if( space_lut[i].depth == gScreen.depth ) {
		
			// Figure out the difference of the areas...
			long diff = screen_area -
				((ulong)space_lut[i].width * (ulong)space_lut[i].height);
			if( diff < 0 ) diff = -diff;
			
			if( diff < bestdiff ) {
				dprintf( "%08lX (%dx%d) is better, its diff is %ld\n",
					(long)space_lut[i].space, (int)space_lut[i].width,
					(int)space_lut[i].height, diff );
				bestdiff = diff;
				besti = i;
			}
		}
	}
	
	if( besti > -1 ) dprintf( "WE LIKE %08lX\n", space_lut[besti].space );

	return  besti > -1  ?  space_lut[besti].space  :  0;
}


static long GetScreenSpaces( graphics_card_config *config )
{
	config->space = GetOurSpaces();
	dprintf( "GetScreenSpaces: returning 0x%08lX\n", config->space );
	return B_NO_ERROR;
}


static long ConfigGraphicsCard( graphics_card_config *config )
{
	long err = B_ERROR;
	
	ulong ourspaces = GetOurSpaces();
	if( ourspaces == 0 ) {
		// This should be impossible...
		dprintf( "ConfigGraphicsCard: We could not find a matching space!\n" );
		goto bail;
	}
	
	if( (config->space & ourspaces) == 0 ) {
		// This should be impossible as well, as we told the appserver
		// what spaces we support.  The appserver should only ask us
		// to set to one of those spaces.
		dprintf( "ConfigGraphicsCard: We didn't set to a space we do not support\n" );
		goto bail;
	}
	
	err = B_NO_ERROR;
	
bail:	
	return err;
}


static long SetScreenGamma( screen_gamma *gamma )
{
	return UnsupportedCall( "SET_SCREEN_GAMMA" );
}


static long GetInfoForClone( clone_info *clone )
{
	// Do we _really_ want to clone a stub video driver?
	// What possible use could the game kit have for this?
	
	*clone = gScreen;
	return B_NO_ERROR;
}


static long SetClonedGraphicsCard( clone_info *clone )
{
	gScreen = *clone;
	return B_NO_ERROR;
}


static void CloseClonedGraphicsCard( void )
{
}


static long ProposeFrameBuffer( frame_buffer_info *fb )
{
	return UnsupportedCall( "PROPOSE_FRAME_BUFFER" );
}


static long SetFrameBuffer( frame_buffer_info *fb )
{
	return UnsupportedCall( "SET_FRAME_BUFFER" );
}


static long SetDisplayArea( frame_buffer_info *fb )
{
	return UnsupportedCall( "SET_DISPLAY_AREA" );
}


static long MoveDisplayArea( frame_buffer_info *fb )
{
	return UnsupportedCall( "MOVE_DISPLAY_AREA" );
}


_EXPORT long control_onboard_graphics_card( uint32 msg, void *data )
{
	long err = B_NO_ERROR;

	//dprintf ("macstub: control call, msg=%.8x\n", msg);
	switch( msg ) {
		
		case B_OPEN_GRAPHICS_CARD:
			err = OpenGraphicsCard( (graphics_card_spec*)data );
			break;
			
		case B_CLOSE_GRAPHICS_CARD:
			// for this message, return value is ignored
			CloseGraphicsCard();
			break;
		
		case B_SET_INDEXED_COLOR:
			err = SetIndexedColor( (indexed_color*)data );
			break;
			
		case B_GET_GRAPHICS_CARD_HOOKS:
			err = GetGraphicsCardHooks( (graphics_card_hook*)data );
			break;
			
		case B_GET_GRAPHICS_CARD_INFO :
			err = GetGraphicsCardInfo( (graphics_card_info*)data );
			break;
		
		case B_GET_REFRESH_RATES:
			err = GetRefreshRates( (refresh_rate_info*)data );
			break;
		
		case B_GET_SCREEN_SPACES:
			err = GetScreenSpaces( (graphics_card_config*)data );
			break;
		
		case B_CONFIG_GRAPHICS_CARD:
			err = ConfigGraphicsCard( (graphics_card_config*)data );
			break;
		
		case B_SET_SCREEN_GAMMA:
			err = SetScreenGamma( (screen_gamma*)data );
			break;
			
		case B_GET_INFO_FOR_CLONE:
			err = GetInfoForClone( (clone_info*)data );
			break;
		
		case B_GET_INFO_FOR_CLONE_SIZE:
			*((long*)data) = sizeof(clone_info);
			break;
		
		case B_SET_CLONED_GRAPHICS_CARD:
			err = SetClonedGraphicsCard( (clone_info*)data );
			break;
			
		case B_CLOSE_CLONED_GRAPHICS_CARD:
			CloseClonedGraphicsCard();
			break;

		case B_PROPOSE_FRAME_BUFFER:
			err = ProposeFrameBuffer( (frame_buffer_info*)data );
			break;
		
		case B_SET_FRAME_BUFFER:
			err = SetFrameBuffer( (frame_buffer_info*)data );
			break;
		
		case B_SET_DISPLAY_AREA:
			err = SetDisplayArea( (frame_buffer_info*)data );
			break;
			
		case B_MOVE_DISPLAY_AREA:
			err = MoveDisplayArea( (frame_buffer_info*)data );
			break;
			
		default:
			err = B_ERROR;
	}

	//dprintf ("macstub: control call, msg=%.8x, returning %.8x\n", msg, err);
	return err;
}
