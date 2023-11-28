#include <OS.h>
#include <image.h>
#include <opengl/GLDefines.h>
#include <add-ons/graphics/Accelerant.h>

#include <graphics_p/radeon/defines.h>
#include <graphics_p/radeon/main.h>
#include <graphics_p/radeon/regdef.h>
#include <graphics_p/radeon/CardState.h>
#include <graphics_p/radeon/radeon_ioctls.h>

#include "proto.h"

static image_id accelerator_image;
static int32 initCount;

status_t Radeon_Init3DEngine( char *device_path, void *context )
{
	status_t (*init)( void *gc, char *dp ) = 0;

dprintf(( "Radeon:  Init3DEngine \n" ));
	if( !initCount )
	{
		accelerator_image = load_add_on( "accelerants/radeon.3da" );
		if( accelerator_image < B_OK )
		{
dprintf(( "Radeon:  Error image load failed \n" ));
			return B_ERROR;
		}
		else
		{
			if( get_image_symbol( accelerator_image, "_AcceleratorInit", B_SYMBOL_TYPE_TEXT, (void **)&init ) <  B_OK )
			{
				unload_add_on( accelerator_image );
dprintf(( "Radeon:  Error image symbol load failed \n" ));
				return B_ERROR;
			}
		}
		initCount++;
	}

	if( (*init)( device_path, context ) >= B_OK )
	{
dprintf(( "Radeon:  Error device init OK \n" ));
		return B_OK;
	}

dprintf(( "Radeon:  Error device init FAILED!! \n" ));
	initCount--;
	if( !initCount )
		unload_add_on( accelerator_image );

	return B_ERROR;
}

status_t Radeon_Shutdown3DEngine( char *device_path, void *context )
{
dprintf(( "radeon_accel:  Radeon_Shutdown3DEngine \n" ));
	
	initCount--;
	if( !initCount )
		unload_add_on( accelerator_image );
	return B_OK;
}

void Radeon_get_device_info( char *device_path, const char **name, uint8 *depth, uint8 *stencil, uint8 *accum )
{
dprintf(( "radeon_accel:  Radeon_get_device_info \n" ));
	*name = "ATI radeon";
	*depth = 1;
	*stencil = 0;
	*accum = 0;
}

void Radeon_get_3d_modes( char *device_path, void (* callback)(video_mode *modes), uint32 min_color, uint32 min_depth, uint32 min_stencil, uint32 min_accum )
{
	static int32 widths[6] = {640, 800, 1024, 1152, 1280, 1600};
	static int32 heights[6] = {480, 600, 768, 864, 1024, 1200};
	int32 ct;
	video_mode m;

dprintf(( "radeon_accel:  Radeon_get_3d_modes \n" ));

	if( !((min_stencil < BGL_8_BIT) || (min_stencil == BGL_8_BIT) ))
		return;
	if( min_accum > BGL_NONE )
		return;
	if( !(((min_color & BGL_BIT_DEPTH_MASK) < BGL_8_BIT) ||
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_16_BIT) ||
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_32_BIT)) )
		return;
	if( !((min_depth < BGL_8_BIT) || (min_depth == BGL_16_BIT) || (min_depth == BGL_24_BIT)) )
		return;
	if( ((min_color & BGL_BIT_DEPTH_MASK) == BGL_32_BIT ) && (min_depth == BGL_16_BIT) )
		return;
	if( ((min_color & BGL_BIT_DEPTH_MASK) == BGL_16_BIT ) && (min_stencil != BGL_NONE) )
		return;
	
	//The user asked for a color and depth mode we can do.
	
	if( min_stencil ||
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_32_BIT) ||
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_HIGH_QUALITY) ||
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_ANY) ||
		(min_depth == BGL_24_BIT) ||
		(min_depth == BGL_HIGH_QUALITY) ||
		(min_depth == BGL_ANY) )
	{
		m.color = (min_color & BGL_COLOR_BUFFER_MASK) | BGL_32_BIT;
		if( (min_depth == BGL_NONE) && (min_stencil == BGL_NONE))
		{
			m.depth = BGL_NONE;
			m.stencil = BGL_NONE;
		}
		else
		{
			m.depth = BGL_24_BIT;
			m.stencil = BGL_8_BIT;
		}
	
		m.width = 0;
		m.height = 0;
		m.refresh = 0;
		m.accum = BGL_NONE;
		callback( &m );
		for( ct=0; ct<6; ct++ )
		{
			m.width = widths[ct];
			m.height = heights[ct];
			m.refresh = 60;
			callback( &m );
			m.refresh = 70;
			callback( &m );
			m.refresh = 75;
			callback( &m );
			m.refresh = 85;
			callback( &m );
		}
	}
	
	if( (min_stencil==BGL_NONE) && (
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_16_BIT) ||
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_FASTEST) ||
		((min_color & BGL_BIT_DEPTH_MASK) == BGL_ANY) ||
		(min_depth == BGL_16_BIT) ||
		(min_depth == BGL_FASTEST) ||
		(min_depth == BGL_ANY) ))
	{
		m.color = (min_color & BGL_COLOR_BUFFER_MASK) | BGL_16_BIT;
		if( min_depth == BGL_NONE )
		{
			m.depth = BGL_NONE;
		}
		else
		{
			m.depth = BGL_16_BIT;
		}
	
		m.width = 0;
		m.height = 0;
		m.refresh = 0;
		m.accum = BGL_NONE;
		m.stencil = BGL_NONE;
		callback( &m );
		for( ct=0; ct<6; ct++ )
		{
			m.width = widths[ct];
			m.height = heights[ct];
			m.refresh = 60;
			callback( &m );
			m.refresh = 70;
			callback( &m );
			m.refresh = 75;
			callback( &m );
			m.refresh = 85;
			callback( &m );
		}
	}
	
}

