#include <image.h>
#include <opengl/GLDefines.h>
#include "accelerant.h"

static image_id accelerator_image;
static int32 initCount = 0;

status_t	sis630_init3Dengine		( char *device_path, void *glContext );
status_t	sis630_shutdown3Dengine	( char *device_path, void *glContext );
void		sis630_get3DdeviceInfo	( char *device_path, const char **name, uint8 *depth, uint8 *stencil, uint8 *accum );
void		sis630_get3Dmodes		( char *device_path, void (* callback)(video_mode *modes), uint32 min_color, uint32 min_depth, uint32 min_stencil, uint32 min_accum );
//uint32		sis630_get3DbuildID		( char *device_path );

status_t sis630_init3Dengine ( char *device_path, void *context ) {
	uint8 (*init)( void *gc , char *dp) = 0;

	vddprintf(("sis630: Init 3D Engine\n"));

	if( !initCount ) {
		accelerator_image = load_add_on( "accelerants/libSiS630.3da" );
		if( accelerator_image < B_OK ) {
			return B_ERROR;
			}
		else {
			if( get_image_symbol( accelerator_image, "_AcceleratorInit", B_SYMBOL_TYPE_TEXT, (void **)&init ) <  B_OK ) {
				unload_add_on( accelerator_image );
				return B_ERROR;
				}
			}
		}

	if( (*init)( device_path, context ) >= B_OK ) {
		initCount++;
		return B_OK;
		}

	return B_ERROR;
	}

status_t sis630_shutdown3Dengine ( char *device_path, void *glContext ) {
	vddprintf(("sis630: Shutdown 3D Engine\n"));
	}

void sis630_get3DdeviceInfo ( char *device_path, const char **name, uint8 *depth, uint8 *stencil, uint8 *accum ) {
	*name = "SiS 630 Chipset";
	*depth = 1;
	*stencil = 0;
	*accum = 0;
	}

void sis630_get3Dmodes ( char *device_path, void (* callback)(video_mode *modes), uint32 min_color, uint32 min_depth, uint32 min_stencil, uint32 min_accum ) {
	static int32 widths[5] = {640 /*, 800, 1024, 1152, 1280*/};
	static int32 heights[5] = {480 /*, 600, 768, 864, 1024*/};
	int32 ct;
	video_mode m;

	if( min_stencil > BGL_NONE )
		return;
	if( min_accum > BGL_NONE )
		return;
	if( !(((min_color & BGL_BIT_DEPTH_MASK) < BGL_8_BIT) || ((min_color & BGL_BIT_DEPTH_MASK) == BGL_16_BIT)) )
		return;
	if( !((min_depth < BGL_8_BIT) || (min_depth == BGL_16_BIT)) )
		return;
	
	//The user asked for a color and depth mode we can do.
	m.color = (min_color & BGL_COLOR_BUFFER_MASK) | BGL_16_BIT;
	if( min_depth == BGL_NONE )
		m.depth = BGL_NONE;
	else
		m.depth = BGL_16_BIT;

	m.width = 0;
	m.height = 0;
	m.refresh = 0;
	m.stencil = BGL_NONE;
	m.accum = BGL_NONE;
	callback( &m );
	for( ct=0; ct<5; ct++ )
	{
		m.width = widths[ct];
		m.height = heights[ct];
		m.refresh = 60;
		callback( &m );
		/*m.refresh = 70;
		callback( &m );
		m.refresh = 75;
		callback( &m );
		m.refresh = 85;
		callback( &m );*/
	}
	
}

