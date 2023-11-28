/*****************************************************************************

     $Source: /net/bally/be/rcs/src/inc/interface_p/interface_misc.h,v $

     $Revision: 1.26 $

     $Author: hiroshi $

     $Date: 1997/06/06 21:03:13 $

     Copyright (c) 1994 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#ifndef _INTERFACE_MISC_H
#define _INTERFACE_MISC_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#ifndef _INTERFACE_DEFS_H
#include <InterfaceDefs.h>
#endif
#ifndef _TRANSFORM_2D_H
#include <Transform2d.h>
#endif
#ifndef _BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _MENU_H
#include <Menu.h>
#endif

extern color_map *_system_cmap_;
extern key_map *_the_trans_maps_;
extern const char* B_ADD_ON_SIGNATURE_ENTRY;    // "add_on";
extern const char* B_LOAD_EACH_TIME_ENTRY;      // "be:load_each_time";
extern const char* B_UNLOAD_ON_DELETE_ENTRY;    // "be:unload_on_delete";
extern const char* B_ADD_ON_VERSION_ENTRY;      // "be:add_on_version";
extern const char* B_CLASS_NAME_ENTRY;          // "class";

// Magic modifier to BMenuItem::SetShortcut() to not add B_COMMAND_KEY.
// NOTE: Eventually, this should be made public.  Currently added for
// Steinberg only.
enum {
	B_NO_COMMAND_KEY = 0x80000000
};

enum {
	B_FORWARD,
	B_BACKWARD
};

/*-------------------------------------------------------------*/

namespace BPrivate {
	typedef void (* ik_destructor )();
	void add_ik_destructor(ik_destructor it);
}
using namespace BPrivate;

/*----------------------------------------------------------------*/
/* window part codes */

enum {
	B_UNKNOWN_AREA,
	B_TITLE_AREA,
	B_CONTENT_AREA,
	B_RESIZE_AREA,
	B_CLOSE_AREA,
	B_ZOOM_AREA,
	B_MINIMIZE_AREA
};

/*-------------------------------------------------------------*/

#define B_DEFAULT_PEN_SIZE			1
#define B_DEFAULT_PEN_LOCATION		B_ORIGIN
#define B_DEFAULT_HIGH_COLOR		0
#define B_DEFAULT_LOW_COLOR			255
#define B_DEFAULT_VIEW_COLOR		255
#define B_DEFAULT_DRAW_MODE			B_OP_COPY
#define B_DEFAULT_SOURCE_ALPHA		B_PIXEL_ALPHA
#define B_DEFAULT_ALPHA_FUNCTION	B_ALPHA_OVERLAY
#define B_DEFAULT_CAP_MODE			B_BUTT_CAP
#define B_DEFAULT_JOIN_MODE			B_BEVEL_JOIN
#define B_DEFAULT_MITER_LIMIT		10.0
#define B_DEFAULT_ORIGIN			B_ORIGIN
#define B_DEFAULT_TRANSFORM			BTransform2d::MakeIdentity()
#define B_DEFAULT_PATTERN			B_SOLID_HIGH

#define B_PEN_SIZE_VALID		0x00000001
#define B_PEN_LOCATION_VALID	0x00000002
#define B_HIGH_COLOR_VALID		0x00000004
#define B_LOW_COLOR_VALID		0x00000008
#define B_DRAW_MODE_VALID		0x00000010
#define B_LINE_MODE_VALID		0x00000020
#define B_ORIGIN_VALID			0x00000040
#define B_FONT_VALID			0x00000080
#define B_PATTERN_VALID			0x00000100
#define B_BLENDING_MODE_VALID	0x00000200
#define B_TRANSFORM_VALID		0x00000400

#define B_VIEW_COLOR_VALID		0x00010000

// cached view_info, used before the view gets attached to window...

struct _view_attr_ {
					_view_attr_();			// to init fields to default;
void				set_default_values();	// to init fields to default;

	/*
	 If some attribute is set before the view is attached then the
	 data should be saved in the appropriate field, and the appropriate
	 valid_flag should be set to 1.
	*/

	uint32			valid_flags;
	uint32			new_local;

	drawing_mode	draw_mode;
	source_alpha	srcAlpha;
	alpha_function	alphaFunc;
	float			pen_size;
	BPoint			pen_loc;
	rgb_color		high_color;
	rgb_color		low_color;
	cap_mode		line_cap;
	join_mode		line_join;
	float			miter_limit;
	BPoint			origin;
	BTransform2d	transform;
	pattern			pat;

	BFont			font;
	uint32			f_mask;
	uint32			f_nonDefault;

	rgb_color		view_color;
	bool			b_view_color;
	
	// These three actually only exist on the client side, but this
	// is a convenvient place for us to put them.
	const char*		view_ui_color;
	const char*		low_ui_color;
	const char*		high_ui_color;
};

/*-------------------------------------------------------------*/
// helper for accessing private parts of classes.  implementation
// is in each corresponding class.

class BBitmap;
class BFont;
class BPicture;
class BRegion;
class BShape;
namespace BPrivate {
class AppSession;
class IRegion;
}
namespace B {
namespace Interface2 {
class BView;
class BWindow;
}
}

using namespace BPrivate;

namespace BPrivate {
class IKAccess {
public:
static	void		ReadFont(BFont* font, AppSession* fromSession);
static	void		WriteFont(const BFont* font, AppSession* toSession);

static	void		ReadFontPacket(BFont* font, const void* packet);
static	void		WriteFontBasicPacket(const BFont* font, void* packet);

static	void		GetShapeData(const BShape* shape,
								 int32 *opCount, int32 *ptCount,
								 uint32 **opList, BPoint **ptList);
static	uint32		ShapeBuildingOp(const BShape* shape);

static	void		SetPictureToken(BPicture* picture, int32 token);
static	int32		PictureToken(const BPicture* picture);
static	bool		UsurpPicture(BPicture* picture, BPicture* previous,
								 bool append = false);
static	BPicture*	StepDownPicture(BPicture* picture);

static	int32		BitmapToken(const BBitmap* bitmap);

static	const IRegion*	RegionData(const BRegion* region);
static	IRegion*	RegionData(BRegion* region);

// IK2
static	int32		ViewToken(B::Interface2::BView* view);
static	AppSession*	WindowSession(B::Interface2::BWindow* window);
};
}

/*-------------------------------------------------------------*/
// structs for printing


typedef struct {
	BRect	clipping;
} _printing_state_;


/*-------------------------------------------------------------*/
// structs for line_arrays

struct _array_hdr_;

struct	_array_hdr_ {
	_array_hdr_	*next;
	long		size;
	long		count;
	long		busy;
	// An array of _XXX_data_ structs will follow this in memory
};

struct _line_data_ {
	float		x0;
	float		y0;
	float		x1;
	float		y1;
	rgb_color	col;
};

struct _rect_data_ {
	BRect		rect;
	bool		filled;
};

struct _array_data_ {
	_array_hdr_	*current;
	long		cur_size;
	long		cur_p;
	_array_hdr_	*list;
};

/*-------------------------------------------------------------*/

enum { NORMAL_VIEW, MENUBAR_VIEW, TEXT_VIEW, SCROLLBAR_VIEW };

class BWindow;
void _set_menu_sem_(BWindow *w, sem_id sem);

// Should match _PRIVATE_W_FLAG7_ defined in Window.h
const long _MENU_WINDOW_FLAG_	= 0x10000000;

// Should match _PRIVATE_W_FLAG8_ defined in Window.h
const long _BITMAP_WINDOW_FLAG_	= 0x20000000;

extern void get_screen_bitmap_(BBitmap *offscreen,BRect src, bool enable = FALSE);

extern rgb_color shift_color(rgb_color color, float percent);

extern void _b_cache_menu_info(const BMessage& src);

namespace BPrivate {

extern void set_int32_as_settings(int32 count, ...);
extern int32 get_int32_as_setting(int32 which);

void cache_ui_settings(const BMessage& src, const BMessage* names);

void translate_points( BPoint offset, BPoint *dstPtArray, const BPoint *srcPtArray, int32 numPoints );
void reflecty_points(BPoint *dstPtArray, const BPoint *srcPtArray, int32 numPoints );
void scale_points(  float scale, BPoint *dstPtArray, const BPoint *srcPtArray, int32 numPoints );
void morph_points( float value, BPoint *dstPtArray, const BPoint *srcPtArrayA, const BPoint *srcPtArrayB, int32 numPoints );

void draw_poly( BView *target,
				BPoint *ptArray,
				int32 numPoints,
				const rgb_color &contentColor,
				const rgb_color &frameColor,
				const rgb_color &shadowColor,
				float penSize,
				uint8 dropShadow );

}

using namespace BPrivate;

/*----------------------------------------------------------------*/

struct window_info {
	team_id		team;
    int32   	id;	  		  /* window's token */

	int32		thread;
	int32		client_token;
	int32		client_port;
	uint32		workspaces;

	int32		layer;
    uint32	  	w_type;    	  /* B_TITLED_WINDOW, etc. */
    uint32      flags;	  	  /* B_WILL_FLOAT, etc. */
	int32		window_left;
	int32		window_top;
	int32		window_right;
	int32		window_bottom;
	int32		show_hide_level;
	bool		is_mini;
	char		name[1];
};

int32		count_windows(team_id app);
window_info	*get_window_info(int32 a_token);
int32		*get_token_list(team_id app, int32 *count);

enum window_action {
	B_MINIMIZE_WINDOW,
	B_BRING_TO_FRONT
};

void		do_window_action(int32 window_id, int32 action, 
							 BRect zoomRect, bool zoom);
void		do_minimize_team(BRect zoom_rect, team_id team, bool zoom);
void		do_bring_to_front_team(BRect zoom_rect, team_id app, bool zoom);

/*----------------------------------------------------------------*/

extern const BFont* _be_symbol_font_;	// private 

/*----------------------------------------------------------------*/

#endif
