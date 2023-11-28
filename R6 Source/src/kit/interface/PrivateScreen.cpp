#include "PrivateScreen.h"

#include <message_util.h>
#include <Application.h>
#include <session.h>
#include <messages.h>
#include <malloc.h>

// this will have to be moved into the screen pool when multiple
// monitors come about.
int32 client_count;

/*-------------------------------------------------------------*/
// private call. (still around for compatibility purpose).

extern void _get_screen_bitmap_(BBitmap *offscreen, BRect src, bool mask_cursor);

void    _get_screen_bitmap_(BBitmap *offscreen, BRect src, bool mask_cursor)
{
	_BAppServerLink_ link;
	ulong            pipo;
	BRect            dst;

	dst = src;
	dst.OffsetTo(0,0);
	link.session->swrite_l(GR_GET_SCREEN_BITMAP);
	link.session->swrite_l(offscreen->get_server_token());
	link.session->swrite_l(mask_cursor);
	link.session->swrite_rect_a(&src);
	link.session->swrite_rect_a(&dst);
	link.session->flush();
	link.session->sread(4, &pipo);
}

// Putting the default ID in here means that we can change
// its value in the future--it doesn't HAVE to be zero.
const screen_id B_MAIN_SCREEN_ID = { 0 };


BPrivateScreen::BPrivateScreen()
{
	uint32 offset;
	_BAppServerLink_ link;

	client_count = 0;
	link.session->swrite_l(GR_GET_SCS);
	link.session->flush();
	link.session->sread(4,&offset);
	if (offset == 0xFFFFFFFF) {
		link.session->sread(4,&offset);
		system_cmap = (color_map*)malloc(offset);
		link.session->sread(offset,system_cmap);
		free_cmap = 1;
	} else {
		system_cmap = (color_map*)be_app->global_ro_offs_to_ptr(offset);
		free_cmap = 0;
	};
	// most users don't want to know about retrace, so don't waste time getting it here
	retrace = -1;
}


BPrivateScreen::~BPrivateScreen()
{
	// nothing to do
	if (free_cmap) free(system_cmap);
}


static BPrivateScreen* CheckOut( void )
{
	int32 oldval = atomic_add( &client_count, 1 );
	if( oldval >= 0 ) {
		return _the_screen_;
	}
	
	// screen_id is no longer valid
	return NULL;
}


BPrivateScreen* BPrivateScreen::CheckOut( screen_id )
{
	return ::CheckOut();
}


BPrivateScreen* BPrivateScreen::CheckOut( BWindow * )
{
	return ::CheckOut();
}


void BPrivateScreen::Return( BPrivateScreen * )
{
	// doesn't actually have to be atomic...?
	int32 newval = atomic_add( &client_count, -1 );
	if( newval < 0 ) {
		// Bad news--we're inconsistent.
	}
}


status_t BPrivateScreen::SetToNext()
{
	return B_ERROR;
}


color_space BPrivateScreen::ColorSpace()
{
	display_mode dm;
	GetMode(0xFFFFFFFF, &dm);
	return (color_space)dm.space;
}


BRect BPrivateScreen::Frame()
{
	display_mode dm;
	GetMode(0xFFFFFFFF, &dm);
	return BRect( 0, 0, dm.virtual_width-1, dm.virtual_height-1);
}


screen_id BPrivateScreen::ID()
{
	return B_MAIN_SCREEN_ID;
}

sem_id BPrivateScreen::RetraceSemaphore()
{
	_BAppServerLink_ link;
	int32            result;

	link.session->swrite_l(GR_GET_RETRACE_SEMAPHORE);
	link.session->swrite_l(ID().id);
	link.session->flush();
	link.session->sread(4, &result);
	return result;
}

void* BPrivateScreen::BaseAddress()
{
	screen_desc desc;
	get_screen_desc( &desc );
	return desc.base;
}


uint32 BPrivateScreen::BytesPerRow()
{
	screen_desc desc;
	get_screen_desc( &desc );
	return desc.rowbyte;
}


status_t BPrivateScreen::WaitForRetrace(bigtime_t timeout)
{
	// get the semaphore if we haven't done so already
	if (retrace < 0) {
		retrace = RetraceSemaphore();
		// return the error if there was some problem
		if (retrace < 0) return retrace;
	}
	// wait for the shake...
	status_t result = acquire_sem_etc(retrace, 1, B_TIMEOUT, timeout);
	// Those bastards!  They killed Kenny!
	if (result == B_BAD_SEM_ID) retrace = -1;
	// let them know how we did
	return result;
}

  
uint8 BPrivateScreen::IndexForColor( uint8 r, uint8 g, uint8 b, uint8 a )
{
	int			index;

	if( r == B_TRANSPARENT_32_BIT.red &&
		g == B_TRANSPARENT_32_BIT.green &&
		b == B_TRANSPARENT_32_BIT.blue &&
		a == B_TRANSPARENT_32_BIT.alpha
	) {
		return B_TRANSPARENT_8_BIT;
	}

	if( system_cmap == NULL ) {
		return 0;
	}

	index = ((r & 0xf8) << 7) |
			((g & 0xf8) << 2) |
			((b & 0xf8) >> 3) ;

	return system_cmap->index_map[index];
}


rgb_color BPrivateScreen::ColorForIndex( const uint8 index )
{
	return system_cmap->color_list[index];
}


uint8 BPrivateScreen::InvertIndex( uint8 index )
{
   return system_cmap->inversion_map[index];
}


status_t BPrivateScreen::GetBitmap(BBitmap **screen_shot, bool draw_cursor, BRect *bound)
{
	BRect			from;
	display_mode	dm;

	GetMode(0xFFFFFFFF, &dm);
	if (bound != NULL) {
		if ((bound->left < 0.0) ||
			(bound->right > (dm.virtual_width-1)) ||
			(bound->top < 0.0) ||
			(bound->bottom > (dm.virtual_height-1)) ||
			(bound->left > bound->right) ||
			(bound->top > bound->bottom))
			return B_ERROR;
		from = *bound;
	}
	else {
		from.left = 0.0;
		from.top = 0.0;
		from.right = dm.virtual_width-1;
		from.bottom = dm.virtual_height-1;
	}
	*screen_shot = new BBitmap(BRect(0, 0, from.right-from.left, from.bottom-from.top),
						 	   (color_space)dm.space);
	if (!(*screen_shot)->IsValid()) {
		delete screen_shot;
		return B_ERROR;
	}
	_get_screen_bitmap_(*screen_shot, from, !draw_cursor);
	return B_NO_ERROR;
}


status_t BPrivateScreen::ReadBitmap(BBitmap *buffer, bool draw_cursor, BRect *bound)
{
	BRect			from;
	display_mode	dm;

	GetMode(0xFFFFFFFF, &dm);
	if (bound != NULL) {
		if ((bound->left < 0.0) ||
			(bound->right > (dm.virtual_width-1)) ||
			(bound->top < 0.0) ||
			(bound->bottom > (dm.virtual_height-1)) ||
			(bound->left > bound->right) ||
			(bound->top > bound->bottom))
			return B_ERROR;
		from = *bound;
	}
	else {
		from.left = 0.0;
		from.top = 0.0;
		from.right = dm.virtual_width-1;
		from.bottom = dm.virtual_height-1;
	}
	_get_screen_bitmap_(buffer, from, !draw_cursor);
	return B_NO_ERROR;
}

rgb_color BPrivateScreen::DesktopColor(uint32 index)
{
	rgb_color	color;
	_BAppServerLink_ link;

	color.red = color.green = color.blue = 127;
	color.alpha = 255;
	link.session->swrite_l(GR_GET_DESKTOP_COLOR);
	link.session->swrite_l(index);
	link.session->flush();
	link.session->sread(sizeof(rgb_color), &color);
	return(color);
}

void BPrivateScreen::SetDesktopColor( rgb_color c, uint32 index, bool stick )
{
	_BAppServerLink_ link;

	link.session->swrite_l(GR_SET_DESKTOP_COLOR);
	link.session->swrite(sizeof(rgb_color), &c);
	link.session->swrite_l(index);
	link.session->swrite_l(stick);
	link.session->flush();
}

status_t 
BPrivateScreen::ProposeMode(display_mode *target, const display_mode *low, const display_mode *high)
{
	_BAppServerLink_ link;
	status_t result;

	link.session->swrite_l(GR_PROPOSE_DISPLAY_MODE);
	link.session->swrite_l(ID().id);
	link.session->swrite(sizeof(display_mode), target);
	link.session->swrite(sizeof(display_mode), (void *)low);
	link.session->swrite(sizeof(display_mode), (void *)high);
	link.session->flush();
	link.session->sread(4, &result);
	link.session->sread(sizeof(display_mode), target);
	return result;
}


status_t
BPrivateScreen::GetModeList(display_mode **mode_list, uint32 *mode_count)
{
	_BAppServerLink_ link;
	status_t result;

	link.session->swrite_l(GR_GET_DISPLAY_MODE_LIST);
	link.session->swrite_l(ID().id);
	link.session->flush();
	link.session->sread(4, &result);
	if (result != B_OK) return result;
	link.session->sread(4, mode_count);
	*mode_list = (display_mode *)calloc(*mode_count, sizeof(display_mode));
	if (*mode_list) {
		link.session->sread(*mode_count * sizeof(display_mode), *mode_list);
	} else {
		// oops, out of memory.  flush the buffer
		display_mode mode;
		uint32 mc = *mode_count;
		while (mc--) link.session->sread(sizeof(display_mode), &mode);
		*mode_count = 0;
		return B_NO_MEMORY;
	}
	return B_OK;
}

status_t 
BPrivateScreen::GetMode(uint32 workspace, display_mode *mode)
{
	_BAppServerLink_ link;
	status_t result;

	link.session->swrite_l(GR_GET_DISPLAY_MODE);
	link.session->swrite_l(ID().id);
	link.session->swrite_l(workspace);
	link.session->flush();
	link.session->sread(4, &result);
	if (result == B_OK) link.session->sread(sizeof(display_mode), mode);
	return result;
}

status_t 
BPrivateScreen::SetMode(uint32 workspace, display_mode *mode, bool makeDefault)
{
	_BAppServerLink_ link;
	status_t result;

	link.session->swrite_l(GR_SET_DISPLAY_MODE);
	link.session->swrite_l(ID().id);
	link.session->swrite_l(workspace);
	link.session->swrite(sizeof(display_mode), mode);
	link.session->swrite_l((uint32)makeDefault);
	link.session->flush();
	link.session->sread(4, &result);
	if (result == B_OK) link.session->sread(sizeof(display_mode), mode);
	return result;
}

status_t 
BPrivateScreen::GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high)
{
	_BAppServerLink_ link;
	status_t result;

	link.session->swrite_l(GR_GET_PIXEL_CLOCK_LIMITS);
	link.session->swrite_l(ID().id);
	link.session->swrite(sizeof(display_mode), mode);
	link.session->flush();
	link.session->sread(4, &result);
	if (result == B_OK) {
		link.session->sread(4, low);
		link.session->sread(4, high);
	}
	return result;
}

status_t 
BPrivateScreen::GetTimingConstraints(display_timing_constraints *dtc)
{
	_BAppServerLink_ link;
	status_t result;

	link.session->swrite_l(GR_GET_TIMING_CONSTRAINTS);
	link.session->swrite_l(ID().id);
	link.session->flush();
	link.session->sread(4, &result);
	if (result == B_OK) link.session->sread(sizeof(display_timing_constraints), dtc);
	return result;
}

status_t 
BPrivateScreen::SetDPMS(uint32 dpms_state)
{
	_BAppServerLink_ link;
	status_t result;

	link.session->swrite_l(GR_SET_DPMS_STATE);
	link.session->swrite_l(ID().id);
	link.session->swrite_l(dpms_state);
	link.session->flush();
	link.session->sread(4, &result);
	return result;
}

uint32 
BPrivateScreen::DPMSState(void)
{
	_BAppServerLink_ link;
	status_t result;
	uint32 state = 0;

	link.session->swrite_l(GR_GET_DPMS_STATE);
	link.session->swrite_l(ID().id);
	link.session->flush();
	link.session->sread(4, &result);
	if (result == B_OK) link.session->sread(4, &state);
	return state;
}

uint32 
BPrivateScreen::DPMSCapabilites(void)
{
	_BAppServerLink_ link;
	status_t result;
	uint32 states = 0;

	link.session->swrite_l(GR_GET_DPMS_CAPABILITIES);
	link.session->swrite_l(ID().id);
	link.session->flush();
	link.session->sread(4, &result);
	if (result == B_OK) link.session->sread(4, &states);
	return states;
}

status_t 
BPrivateScreen::GetDeviceInfo(accelerant_device_info *adi)
{
	_BAppServerLink_ link;
	status_t result;

	link.session->swrite_l(GR_GET_ACCELERANT_DEVICE_INFO);
	link.session->swrite_l(ID().id);
	link.session->flush();
	link.session->sread(4, &result);
	if (result == B_OK) link.session->sread(sizeof(accelerant_device_info), adi);
	return result;
}

const color_map* BPrivateScreen::ColorMap()
{
	return system_cmap; 	
}


void BPrivateScreen::get_screen_desc( screen_desc *desc )
{
	int32 index;
	_BAppServerLink_ link;
	
	link.session->swrite_l(GR_GET_SCREEN_INFO);
	index = 0;
	link.session->swrite(sizeof(int32), &index);
	link.session->flush();
	link.session->sread(sizeof(screen_desc), desc);
}

extern "C" 
_EXPORT status_t _accelerant_perform_hack_(screen_id id, ssize_t in_size, void *in_info, ssize_t *out_size, void **out_info) {
	_BAppServerLink_ link;
	status_t result;
	link.session->swrite_l(GR_ACCELERANT_PERFORM_HACK);
	link.session->swrite_l(id.id);
	link.session->swrite_l(in_size);
	link.session->swrite(in_size, in_info);
	link.session->flush();
	link.session->sread(sizeof(result), &result);
	if (result == B_OK) {
		link.session->sread(sizeof(size_t), out_size);
		if (*out_size) {
			*out_info = (void *)malloc(*out_size);
			if (*out_info) link.session->sread(*out_size, *out_info);
			else {
				char buffer[32];
				ssize_t size = *out_size;
				*out_size = 0;
				*out_info = 0;
				result = B_NO_MEMORY;
				/* flush the buffer */
				while (size > 0) {
					link.session->sread(size > (int32)sizeof(buffer) ? sizeof(buffer) : size, buffer);
					size -= sizeof(buffer);
				}
			}
		} else *out_info = 0;
	}
	return result;
}
