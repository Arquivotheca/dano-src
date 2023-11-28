/*****************************************************************************

     $Source: /net/bally/be/rcs/src/kit/interface/GraphicsEnv.cpp,v $

     $Revision: 1.101 $

     $Author: peter $

     $Date: 2000/06/23 20:57:26 $

     Copyright (c) 1994 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <DataIO.h>
#include <Debug.h>
#include <Screen.h>
#include <StreamIO.h>
#include <Window.h>

#ifndef _INTERFACE_DEFS_H
#include "InterfaceDefs.h"
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif

#ifndef _FONT_H
#include <Font.h>
#endif

#ifndef _MESSAGE_UTIL_H
#include <message_util.h>
#endif
#ifndef _INTERFACE_MISC_H
#include <interface_misc.h>
#endif
#ifndef _MESSAGES_H
#include <messages.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _SESSION_H
#include <session.h>
#endif
#ifndef _BITMAP_H
#include "Bitmap.h"
#endif
#ifndef _PATH_H
#include <Path.h>
#endif
#ifndef _FIND_DIRECTORY_H
#include <FindDirectory.h>
#endif
#include <RadioButton.h>
#include <TextView.h>
#include <TextViewSupport.h>
#include "screen_private.h"
#include "roster_private.h"
#include <input_server_private.h>
#include <debug_support.h>
#include <ByteOrder.h>
#include <Roster.h>
#include <NodeInfo.h>

#include <stdarg.h>

extern const uchar 		B_TRANSPARENT_8_BIT = 0xff;
extern const rgb_color	B_TRANSPARENT_32_BIT = {0x77, 0x74, 0x77, 0x00};			// new, r3

extern const rgb_color 	B_TRANSPARENT_COLOR 			= {0x77, 0x74, 0x77, 0x00};
extern const uint8		B_TRANSPARENT_MAGIC_CMAP8 		= 0xFF;

#if B_HOST_IS_LENDIAN
extern const uint16		B_TRANSPARENT_MAGIC_RGBA15 		= 0x39CE;
extern const uint16		B_TRANSPARENT_MAGIC_RGBA15_BIG 	= 0xCE39;
extern const uint32		B_TRANSPARENT_MAGIC_RGBA32 		= 0x00777477;
extern const uint32		B_TRANSPARENT_MAGIC_RGBA32_BIG 	= 0x77747700;
#else
extern const uint16		B_TRANSPARENT_MAGIC_RGBA15 		= 0xCE39;
extern const uint16		B_TRANSPARENT_MAGIC_RGBA15_BIG 	= 0x39CE;
extern const uint32		B_TRANSPARENT_MAGIC_RGBA32 		= 0x77747700;
extern const uint32		B_TRANSPARENT_MAGIC_RGBA32_BIG 	= 0x00777477;
#endif

// the following is not essential, as B_TRANSPARENT_32_BIT is a symbol that
// people link against, and so changing the values should be transparent 
// (pun intended) to the app, but we're being extra extra safe here
// (in case the values were saved to disk, for example)
extern const rgb_color	_B_SEMI_OLD_TRANSPARENT_32_BIT_ = {0x77, 0x74, 0x73, 0x00};	// dr9.1
extern const rgb_color	_B_OLD_TRANSPARENT_32_BIT_ = {0x77, 0x74, 0x73, 0x72};		// original

const char *B_UI_PANEL_BACKGROUND_COLOR				= "be:c:PanBg";
const char *B_UI_PANEL_TEXT_COLOR					= "be:c:PanTx";
const char *B_UI_DOCUMENT_BACKGROUND_COLOR			= "be:c:DocBg";
const char *B_UI_DOCUMENT_TEXT_COLOR				= "be:c:DocTx";
const char *B_UI_CONTROL_BACKGROUND_COLOR			= "be:c:CtlBg";
const char *B_UI_CONTROL_TEXT_COLOR					= "be:c:CtlTx";
const char *B_UI_CONTROL_BORDER_COLOR				= "be:c:CtlBr";
const char *B_UI_CONTROL_HIGHLIGHT_COLOR			= "be:c:CtlHg";
const char *B_UI_NAVIGATION_BASE_COLOR				= "be:c:NavBs";
const char *B_UI_NAVIGATION_PULSE_COLOR				= "be:c:NavPl";
const char *B_UI_SHINE_COLOR						= "be:c:Shine";
const char *B_UI_SHADOW_COLOR						= "be:c:Shadow";

const char *B_UI_TOOLTIP_BACKGROUND_COLOR			= "be:c:TipBg";
const char *B_UI_TOOLTIP_TEXT_COLOR					= "be:c:TipTx";
const char *B_UI_TOOLTIP_FONT						= "be:f:Tip";

const char *B_UI_MENU_BACKGROUND_COLOR			 	= "be:c:MenBg";
const char *B_UI_MENU_SELECTED_BACKGROUND_COLOR		= "be:c:MenSBg";
const char *B_UI_MENU_ITEM_TEXT_COLOR				= "be:c:MenTx";
const char *B_UI_MENU_SELECTED_ITEM_TEXT_COLOR		= "be:c:MenSTx";
const char *B_UI_MENU_SELECTED_BORDER_COLOR			= "be:c:MenSBr";
const char *B_UI_MENU_ITEM_TEXT_FONT				= "be:f:MenTx";
const char *B_UI_MENU_SEPARATOR						= "be:MenSep";
const char *B_UI_MENU_SHOW_TRIGGERS					= "be:MenTrig";
const char *B_UI_MENU_ZSNAKE						= "be:MenZSnake";

const char *B_UI_SUCCESS_COLOR						= "be:c:Success";
const char *B_UI_FAILURE_COLOR						= "be:c:Failure";

/*-------------------------------------------------------------*/
extern "C" long _init_interface_kit_();
extern "C" void _fini_interface_kit_(void);

static BLocker ui_settings_access("global_ui_settings");
static BMessage ui_settings;
static BMessage ui_names;
static const int32 NUM_COLORS = B_TOOLTIP_TEXT_COLOR+1;
struct ui_colors {
	rgb_color colors[NUM_COLORS];
	rgb_color success_color;
	rgb_color failure_color;
};
static const ui_colors default_ui_colors =
{
	{
		{	255,	255,	255,	255 },	//  0 - pad
		{	216,	216,	216,	255 },	//  1 - B_PANEL_BACKGROUND_COLOR
		{	216,	216,	216,	255 },	//  2 - B_MENU_BACKGROUND_COLOR
		{	255,	203,	0,		255 },	//  3 - B_WINDOW_TAB_COLOR
		{	170,	50,		184,	255 },	//  4 - B_NAVIGATION_BASE_COLOR
		{	255,	255,	255,	255 },	//  5 - B_DESKTOP_COLOR
		{	115,	120,	184,	255 },	//  6 - B_MENU_SELECTED_BACKGROUND_COLOR
		{	0,		0,		0,		255 },	//  7 - B_MENU_ITEM_TEXT_COLOR
		{	255,	255,	255,	255 },	//  8 - B_MENU_SELECTED_ITEM_TEXT_COLOR
		{	0,		0,		0,		255 },	//  9 - B_MENU_SELECTED_BORDER_COLOR
		{	0,		0,		0,		255 },	// 10 - B_PANEL_TEXT_COLOR
		{	255,	255,	255,	255 },	// 11 - B_DOCUMENT_BACKGROUND_COLOR
		{	0,		0,		0,		255 },	// 12 - B_DOCUMENT_TEXT_COLOR
		{	245,	245,	245,	255 },	// 13 - B_CONTROL_BACKGROUND_COLOR
		{	0,		0,		0,		255 },	// 14 - B_CONTROL_TEXT_COLOR
		{	0,		0,		0,		255 },	// 15 - B_CONTROL_BORDER_COLOR
		{	115,	120,	184,	255 },	// 16 - B_CONTROL_HIGHLIGHT_COLOR
		{	0,		0,		0,		255 },	// 17 - B_NAVIGATION_PULSE_COLOR
		{	255,	255,	255,	255 },	// 18 - B_SHINE_COLOR
		{	0,		0,		0,		255 },	// 19 - B_SHADOW_COLOR
		{	255,	255,	0,		255 },	// 20 - B_TOOLTIP_BACKGROUND_COLOR
		{	0,		0,		0,		255 },	// 21 - B_TOOLTIP_TEXT_COLOR
	},
	{	0,		255,	0,		255 },	// B_SUCCESS_COLOR
	{	255,	0,		0,		255 }	// B_FAILURE_COLOR
};
static ui_colors cached_ui_settings = default_ui_colors;

static const struct { const char* name; int32 index; }
ui_color_map[] =
{
	{ B_UI_PANEL_BACKGROUND_COLOR,				B_PANEL_BACKGROUND_COLOR },
	{ B_UI_PANEL_TEXT_COLOR,					B_PANEL_TEXT_COLOR },
	{ B_UI_DOCUMENT_BACKGROUND_COLOR,			B_DOCUMENT_BACKGROUND_COLOR },
	{ B_UI_DOCUMENT_TEXT_COLOR,					B_DOCUMENT_TEXT_COLOR },
	{ B_UI_CONTROL_BACKGROUND_COLOR,			B_CONTROL_BACKGROUND_COLOR },
	{ B_UI_CONTROL_TEXT_COLOR,					B_CONTROL_TEXT_COLOR },
	{ B_UI_CONTROL_BORDER_COLOR,				B_CONTROL_BORDER_COLOR },
	{ B_UI_CONTROL_HIGHLIGHT_COLOR,				B_CONTROL_HIGHLIGHT_COLOR },
	{ B_UI_NAVIGATION_BASE_COLOR,				B_NAVIGATION_BASE_COLOR },
	{ B_UI_NAVIGATION_PULSE_COLOR,				B_NAVIGATION_PULSE_COLOR },
	{ B_UI_SHINE_COLOR,							B_SHINE_COLOR },
	{ B_UI_SHADOW_COLOR,						B_SHADOW_COLOR },

	{ B_UI_TOOLTIP_BACKGROUND_COLOR,			B_TOOLTIP_BACKGROUND_COLOR },
	{ B_UI_TOOLTIP_TEXT_COLOR,					B_TOOLTIP_TEXT_COLOR },
	
	{ B_UI_MENU_BACKGROUND_COLOR,				B_MENU_BACKGROUND_COLOR },
	{ B_UI_MENU_SELECTED_BACKGROUND_COLOR,		B_MENU_SELECTED_BACKGROUND_COLOR },
	{ B_UI_MENU_ITEM_TEXT_COLOR,				B_MENU_ITEM_TEXT_COLOR },
	{ B_UI_MENU_SELECTED_ITEM_TEXT_COLOR,		B_MENU_SELECTED_ITEM_TEXT_COLOR },
	{ B_UI_MENU_SELECTED_BORDER_COLOR,			B_MENU_SELECTED_BORDER_COLOR },
	
	{ NULL, 0 }
};

static const struct { const char* name; const char* english; }
ui_name_map[] =
{
	{ B_UI_PANEL_BACKGROUND_COLOR,				"Panel Background" },
	{ B_UI_PANEL_TEXT_COLOR,					"Panel Text" },
	{ B_UI_DOCUMENT_BACKGROUND_COLOR,			"Document Background" },
	{ B_UI_DOCUMENT_TEXT_COLOR,					"Document Text" },
	{ B_UI_CONTROL_BACKGROUND_COLOR,			"Control Background" },
	{ B_UI_CONTROL_TEXT_COLOR,					"Control Text" },
	{ B_UI_CONTROL_BORDER_COLOR,				"Control Border" },
	{ B_UI_CONTROL_HIGHLIGHT_COLOR,				"Control Highlight" },
	{ B_UI_NAVIGATION_BASE_COLOR,				"Navigation Base" },
	{ B_UI_NAVIGATION_PULSE_COLOR,				"Navigation Pulse" },
	{ B_UI_SHINE_COLOR,							"Shine" },
	{ B_UI_SHADOW_COLOR,						"Shadow" },

	{ B_UI_TOOLTIP_BACKGROUND_COLOR,			"ToolTip Background" },
	{ B_UI_TOOLTIP_TEXT_COLOR,					"ToolTip Text" },
	{ B_UI_TOOLTIP_FONT,						"ToolTip" },
	
	{ B_UI_MENU_BACKGROUND_COLOR,				"Menu Background" },
	{ B_UI_MENU_SELECTED_BACKGROUND_COLOR,		"Menu Selected Background" },
	{ B_UI_MENU_ITEM_TEXT_COLOR,				"Menu Item Text" },
	{ B_UI_MENU_SELECTED_ITEM_TEXT_COLOR,		"Menu Selected Item Text" },
	{ B_UI_MENU_SELECTED_BORDER_COLOR,			"Menu Selected Border" },
	{ B_UI_MENU_ITEM_TEXT_FONT,					"Menu Item Text" },
	{ B_UI_MENU_SEPARATOR,						"Menu Separator" },
	{ B_UI_MENU_SHOW_TRIGGERS,					"Show Menu Triggers" },
	{ B_UI_MENU_ZSNAKE,							"Menu ZSnake" },
	
	{ B_UI_SUCCESS_COLOR,						"Success" },
	{ B_UI_FAILURE_COLOR,						"Failure" },
	
	{ NULL, 0 }
};

status_t update_ui_settings(const BMessage& changes, uint32 flags, const BMessage* names)
{
	BMessage container;
	container.AddMessage("be:settings", &changes);
	if (names) container.AddMessage("be:names", names);
	
	if ((flags&(B_APPLY_UI_SETTINGS|B_SAVE_UI_SETTINGS)) != 0) {
		BMallocIO io;
		status_t result = container.Flatten(&io);
		
		if (result == B_OK) {
			_BAppServerLink_ link;
			link.session->swrite_l(GR_SET_UI_SETTINGS);
			link.session->swrite_l(flags);
			link.session->swrite_l(io.BufferLength());
			link.session->swrite(io.BufferLength(), io.Buffer());
			link.session->flush();
			link.session->sread(sizeof(result), &result);
		}
		
		return result;
	} else if (be_app) {
		container.what = B_UI_SETTINGS_CHANGED;
		BMessenger(be_app).SendMessage(container);
	}
	
	return B_OK;
}

status_t get_ui_settings(BMessage* dest, BMessage* names)
{
	/*	This is *not* an unused variable.  It triggers the
		connection to the spp_server, if it hasn't been done yet. */
	_BAppServerLink_ dummyLink;
	
	ui_settings_access.Lock();
	*dest = ui_settings;
	if (names) *names = ui_names;
	ui_settings_access.Unlock();
	return B_OK;
}

status_t get_default_settings(BMessage* dest, BMessage* names)
{
	BFont f;
	status_t r;
	int32 i;
	
	for (i=0; ui_color_map[i].name; i++) {
		r = dest->AddRGBColor(ui_color_map[i].name,
								default_ui_colors.colors[ui_color_map[i].index]);
		if (r < B_OK) return r;
	}
	
	r = dest->AddRGBColor(B_UI_SUCCESS_COLOR, default_ui_colors.success_color);
	if (r < B_OK) return r;
	r = dest->AddRGBColor(B_UI_FAILURE_COLOR, default_ui_colors.failure_color);
	if (r < B_OK) return r;
	
	f.SetShear(90);
	f.SetRotation(0);
	f.SetSpacing(B_BITMAP_SPACING);
	f.SetEncoding(B_UNICODE_UTF8);
	f.SetFlags(0);
	
	f.SetFamilyAndFace("Swiss721 BT", B_REGULAR_FACE);
	f.SetSize(12);
	r = dest->AddFlat(B_UI_MENU_ITEM_TEXT_FONT, &f);
	if (r < B_OK) return r;
	r = dest->AddInt32(B_UI_MENU_SEPARATOR, 0);
	if (r < B_OK) return r;
	r = dest->AddBool(B_UI_MENU_SHOW_TRIGGERS, false);
	if (r < B_OK) return r;
	r = dest->AddBool(B_UI_MENU_ZSNAKE, false);
	
	f.SetTo(*be_plain_font, B_FONT_FAMILY_AND_STYLE|B_FONT_SIZE);
	r = dest->AddFlat(B_UI_TOOLTIP_FONT, &f);
	if (r < B_OK) return r;
	
	if (names) {
		for (i=0; ui_name_map[i].name; i++) {
			r = names->AddString(ui_name_map[i].name, ui_name_map[i].english);
			if (r < B_OK) return r;
		}
	}
	
	return r;
}

#if _R5_COMPATIBLE_
// ---- Begin Eddie compatibility hack

// Eddie accesses this old private stuff.  Shame, Pavel, shame!
struct general_ui_info {
	rgb_color	background_color;
	rgb_color	mark_color;
	rgb_color	highlight_color;
	bool		color_frame;
	rgb_color	window_frame_color;
};
extern general_ui_info	general_info;
general_ui_info general_info;
long get_general_ui_info(general_ui_info *info)
{
	*info = general_info;
	return sizeof(general_ui_info);
}

// ---- End Eddie compatibility hack
#endif

status_t set_window_decor(const char* name, const BMessage* globals, uint32)
{
	status_t result = B_OK;
	
	_BAppServerLink_ link;
	link.session->swrite_l(GR_SET_WINDOW_DECOR);
	link.session->swrite(name ? name : "");
	if (globals) {
		BMallocIO io;
		globals->Flatten(&io);
		link.session->swrite_l(io.BufferLength());
		link.session->swrite(io.BufferLength(), io.Buffer());
	} else {
		link.session->swrite_l(0);
	}
	
	link.session->flush();
	link.session->sread(sizeof(result), &result);
			
	return result;
}

status_t get_window_decor(BString* outName, BMessage* globals)
{
	status_t result = B_OK;
	
	_BAppServerLink_ link;
	link.session->swrite_l(GR_GET_WINDOW_DECOR);
	link.session->flush();
	
	link.session->sread(outName);
	
	int32 len = 0;
	link.session->sread(sizeof(int32), &len);
	if (len) {
		void* buf = malloc(len);
		if (!buf) result = B_NO_MEMORY;
		else {
			link.session->sread(len, buf);
			if (globals) {
				result = globals->Unflatten((const char*)buf);
			}
			free(buf);
		}
	}
	
	return result;
}

namespace BPrivate {
void cache_ui_settings(const BMessage& settings, const BMessage* names)
{
	rgb_color c;
	
	ui_settings_access.Lock();
	
	if (&settings != &ui_settings) ui_settings.Update(settings, true);
	
	for (int32 i=0; ui_color_map[i].name; i++) {
		if (settings.FindRGBColor(ui_color_map[i].name, &c) == B_OK) {
			// oh gack.  it looks like the stupid menu
			// preferences can create colors with a 0 alpha!!
			if (ui_color_map[i].index == B_MENU_BACKGROUND_COLOR)
				c.alpha = 255;
			cached_ui_settings.colors[ui_color_map[i].index] = c;
		}
	}
	
	if (settings.FindRGBColor(B_UI_SUCCESS_COLOR, &c) == B_OK)
		cached_ui_settings.success_color = c;
	if (settings.FindRGBColor(B_UI_FAILURE_COLOR, &c) == B_OK)
		cached_ui_settings.failure_color = c;
	
#if _R5_COMPATIBLE_
// ---- Begin Eddie Compatibility hack
	general_info.background_color = cached_ui_settings.colors[B_PANEL_BACKGROUND_COLOR];
	general_info.mark_color = cached_ui_settings.colors[B_NAVIGATION_BASE_COLOR];
	rgb_color cc = {255, 203, 0, 255};
	general_info.window_frame_color = cc;
	general_info.color_frame = true;
// ---- End Eddie compatibility hack
#endif
	
	if (names && names != &ui_names) ui_names.Update(*names, true);
	
	ui_settings_access.Unlock();
	
	_b_cache_menu_info(settings);
}
}

long _init_interface_kit_()
{
#if 1
	// First make sure defaults are installed.
	get_default_settings(&ui_settings, &ui_names);
	cache_ui_settings(ui_settings, NULL);
	
	_BAppServerLink_ link;
	int32		len;

	link.session->swrite_l(GR_GET_UI_SETTINGS);
	link.session->flush();
	link.session->sread(sizeof(int32), &len);
//+	PRINT(("getting %d bytes of ui_settings from app_server\n", len));
	if (len) {
		void* buf = malloc(len);
		if (buf) {
			link.session->sread(len, buf);
			BMessage data;
			if (data.Unflatten((const char*)buf) >= B_OK) {
				BMessage values;
				if (data.FindMessage("be:settings", &values) == B_OK)
					ui_settings.Update(values, true);
				if (data.FindMessage("be:names", &values) == B_OK)
					ui_names.Update(values, true);
			}
			free(buf);
		} else {
			_sPrintf("*** Unable to allocate %ld byte buffer for UI settings.\n", len);
		}
	}
#endif

	cache_ui_settings(ui_settings, NULL);
	
	// TextView stuff
	BTextView::sWidths = new _BWidthBuffer_();
	BTextView::sWidthSem = create_sem(0, "BTextView::sWidthSem");
	BTextView::sWidthAtom = 1;

	return B_NO_ERROR;
}

static BList destructors;
void BPrivate::add_ik_destructor(BPrivate::ik_destructor it)
{
	destructors.AddItem((void*)it);
}

void _fini_interface_kit_(void)
{
	for (int32 i=destructors.CountItems()-1; i>=0; i--) {
		BPrivate::ik_destructor it = (BPrivate::ik_destructor)(destructors.ItemAt(i));
		if (it) (*it)();
	}
	
	// text view
	delete BTextView::sWidths;
	delete_sem(BTextView::sWidthSem);
}

/* ----------------------------------------------------------- */

rgb_color ui_color(color_which which)
{
	if (which >= B_PANEL_BACKGROUND_COLOR && which < NUM_COLORS &&
			which != B_DESKTOP_COLOR && which != B_WINDOW_TAB_COLOR)
		return cached_ui_settings.colors[which];
	
	switch (which) {
		case B_DESKTOP_COLOR:
			return BScreen().DesktopColor();
		
		case B_WINDOW_TAB_COLOR:
			return make_color(255, 203, 0);

		case B_SUCCESS_COLOR:
			return cached_ui_settings.success_color;

		case B_FAILURE_COLOR:
			return cached_ui_settings.failure_color;
		
		case B_RANDOM_COLOR:
			{ uint32 c = ((rand()<<16) ^ (rand()));
			return *((rgb_color *)&c); }
		case B_MICHELANGELO_FAVORITE_COLOR:		return make_color(51, 102, 204);
		case B_DSANDLER_FAVORITE_SKY_COLOR:		return make_color(109, 195, 200);
		case B_DSANDLER_FAVORITE_INK_COLOR:		return make_color(66, 86, 165);
		case B_DSANDLER_FAVORITE_SHOES_COLOR:	return make_color(99, 75, 44);
		case B_DAVE_BROWN_FAVORITE_COLOR:		return make_color(56, 0, 142);

		default:
			break;
	}

	return make_color(255, 255, 255);
}

rgb_color ui_color(const char* name)
{
	ui_settings_access.Lock();
	rgb_color col;
	if (ui_settings.FindRGBColor(name, &col) != B_OK) {
		col.red = col.green = col.blue = col.alpha = 255;
	}
	ui_settings_access.Unlock();
	return col;
}

/* ----------------------------------------------------------- */

#if DEBUG
#define DB_INLINE
#else
#define DB_INLINE inline
#endif

static DB_INLINE void mix_color_func(rgb_color* target, const rgb_color other, uint8 amount)
{
	target->red = (uint8)( ((int16(other.red)-int16(target->red))*amount)/255
								+ target->red );
	target->green = (uint8)( ((int16(other.green)-int16(target->green))*amount)/255
								+ target->green );
	target->blue = (uint8)( ((int16(other.blue)-int16(target->blue))*amount)/255
								+ target->blue );
	target->alpha = (uint8)( ((int16(other.alpha)-int16(target->alpha))*amount)/255
								+ target->alpha );
}

static DB_INLINE void blend_color_func(rgb_color* target, const rgb_color other, uint8 amount)
{
	const uint8 alphaMix = (uint8)( ((int16(other.alpha)-int16(255-target->alpha))*amount)/255
									+ (255-target->alpha) );
	target->red = (uint8)( ((int16(other.red)-int16(target->red))*alphaMix)/255
								+ target->red );
	target->green = (uint8)( ((int16(other.green)-int16(target->green))*alphaMix)/255
								+ target->green );
	target->blue = (uint8)( ((int16(other.blue)-int16(target->blue))*alphaMix)/255
								+ target->blue );
	target->alpha = (uint8)( ((int16(other.alpha)-int16(target->alpha))*amount)/255
								+ target->alpha );
}

#if 0
static DB_INLINE uint8 shift_component(uint8 val, float percent)
{
	if (percent >= 1.0)
		return (uint8)(val * (2.0 - percent));
	else
		return (uint8)(255 - (percent * (255-val)));
		//return (val + ((255 - val) * (1.0 - percent)));
}
#endif

static DB_INLINE void tint_color_func(rgb_color* target, float percent)
{
	if (percent >= 1.0)
		mix_color_func(target, cached_ui_settings.colors[B_SHADOW_COLOR], (uint8)(255*(percent-1.0)));
	else
		mix_color_func(target, cached_ui_settings.colors[B_SHINE_COLOR], (uint8)(255*(1.0-percent)));
	#if 0
	target->red = shift_component(target->red, percent);
	target->green = shift_component(target->green, percent);
	target->blue = shift_component(target->blue, percent);
	#endif
}

static DB_INLINE void disable_color_func(rgb_color* target, const rgb_color background)
{
	blend_color_func(target, background, 255-70);
}

// --------------------------------------------------------------------------

rgb_color rgb_color::mix_copy(const rgb_color other, uint8 amount) const
{
	rgb_color ret = *this;
	mix_color_func(&ret, other, amount);
	return ret;
}

rgb_color& rgb_color::mix(const rgb_color other, uint8 amount)
{
	mix_color_func(this, other, amount);
	return *this;
}

rgb_color rgb_color::blend_copy(const rgb_color other, uint8 amount) const
{
	rgb_color ret = *this;
	blend_color_func(&ret, other, amount);
	return ret;
}

rgb_color& rgb_color::blend(const rgb_color other, uint8 amount)
{
	blend_color_func(this, other, amount);
	return *this;
}

rgb_color rgb_color::tint_copy(float tint) const
{
	rgb_color ret = *this;
	tint_color_func(&ret, tint);
	return ret;
}

rgb_color& rgb_color::tint(float tint)
{
	tint_color_func(this, tint);
	return *this;
}

rgb_color rgb_color::disable_copy(const rgb_color background) const
{
	rgb_color ret = *this;
	disable_color_func(&ret, background);
	return ret;
}

rgb_color& rgb_color::disable(const rgb_color background)
{
	disable_color_func(this, background);
	return *this;
}

// --------------------------------------------------------------------------

rgb_color mix_color(rgb_color color1, rgb_color color2, uint8 amount)
{
	mix_color_func(&color1, color2, amount);
	return color1;
}

rgb_color blend_color(rgb_color color1, rgb_color color2, uint8 amount)
{
	blend_color_func(&color1, color2, amount);
	return color1;
}

rgb_color tint_color(rgb_color c, float tint)
{
	tint_color_func(&c, tint);
	return c;
}

rgb_color disable_color(rgb_color color, rgb_color background)
{
	disable_color_func(&color, background);
	return color;
}

/* ----------------------------------------------------------- */

BDataIO& operator<<(BDataIO& io, rgb_color color)
{
#if SUPPORTS_STREAM_IO
	char buffer[64];
	sprintf(buffer, "rgb_color(%d,%d,%d,%d)",
			color.red, color.green, color.blue, color.alpha);
	return (io << buffer);
#else
	(void)color;
	return io;
#endif
}

/* ----------------------------------------------------------- */

rgb_color	shift_color(rgb_color c, float percent)
{
	return tint_color(c, percent);
}

/* ---------------------------------------------------------------- */

namespace BPrivate {

void set_int32_as_settings(int32 count, ...)
{
	va_list vl;
	va_start(vl, count);
	_BAppServerLink_ link;
	link.session->swrite_l(GR_SET_INT32_SETTING);
	link.session->swrite(sizeof(count), &count);
	while (count > 0) {
		int32 which = va_arg(vl, int32);
		int32 value = va_arg(vl, int32);
		link.session->swrite(sizeof(which), &which);
		link.session->swrite(sizeof(value), &value);
		count--;
	}
	link.session->flush();
	va_end(vl);
}

int32 get_int32_as_setting(int32 which)
{
	_BAppServerLink_ link;
	int32 value;
	link.session->swrite_l(GR_GET_INT32_SETTING);
	link.session->swrite(sizeof(which), &which);
	link.session->flush();
	link.session->sread(sizeof(value), &value);
	return value;
}

/* ---------------------------------------------------------------- */

void translate_points( BPoint offset, BPoint *dstPtArray, const BPoint *srcPtArray, int32 numPoints )
{
	for( int32 i=0; i<numPoints; i++ )
		*dstPtArray++ = *srcPtArray++ + offset;
}

void reflecty_points( BPoint *dstPtArray, const BPoint *srcPtArray, int32 numPoints )
{
	for( int32 i=0; i<numPoints; i++, dstPtArray++, srcPtArray++ )
	{
		dstPtArray->y = srcPtArray->y * (-1.0);
		dstPtArray->x = srcPtArray->x;
	}
}

void scale_points(  float scale, BPoint *dstPtArray, const BPoint *srcPtArray, int32 numPoints )
{
	for( int32 i=0; i<numPoints; i++ )
		*dstPtArray++ = (*srcPtArray++) * scale;
}

void morph_points( float value, BPoint *dstPtArray, const BPoint *srcPtArrayA, const BPoint *srcPtArrayB, int32 numPoints )
{
	for( int32 i=0; i<numPoints; i++ )
		*dstPtArray++ = ((*srcPtArrayB++ - *srcPtArrayA) * value) + *srcPtArrayA++;
}

void draw_poly( BView *target,
				BPoint *ptArray,
				int32 numPoints,
				const rgb_color &contentColor,
				const rgb_color &frameColor,
				const rgb_color &shadowColor,
				float penSize,
				uint8 dropShadow )
{
	float oldPen = target->PenSize();
	
	if( dropShadow )
	{
		rgb_color shadow = shadowColor;
		
		target->SetDrawingMode( B_OP_ALPHA );
		translate_points( BPoint(dropShadow,dropShadow), ptArray, ptArray, numPoints );
		for( ; dropShadow > 0; dropShadow--, translate_points( BPoint(-1,-1), ptArray, ptArray, numPoints ) )
		{
			shadow.alpha = shadowColor.alpha >> (dropShadow-1);
			target->SetHighColor( shadow );
			target->FillPolygon( ptArray, numPoints );
		}
	}
	target->SetDrawingMode( contentColor.alpha == 255 ? B_OP_COPY : B_OP_ALPHA );
	target->SetHighColor( contentColor );
	target->FillPolygon( ptArray, numPoints );
	target->SetPenSize( penSize );
	target->SetDrawingMode( frameColor.alpha == 255 ? B_OP_COPY : B_OP_ALPHA );
	target->SetHighColor( frameColor );
	target->StrokePolygon( ptArray, numPoints );
	target->SetPenSize( oldPen );
}

}

#if 0
/* ---------------------------------------------------------------- */

long set_general_ui_info(general_ui_info *info)
{
	memcpy(&(_ui_info_ptr_->general), info, sizeof(general_ui_info));
	memcpy(&(general_info), info, sizeof(general_ui_info));
	_save_ui_info_to_disk_((ui_info *) _ui_info_ptr_);
	return B_NO_ERROR;
}

/*-------------------------------------------------------------*/

uchar index_for_color(rgb_color c)
{
	long		index;
	const color_map	*map;

	if (*(long*)&c == *(long*)&B_TRANSPARENT_32_BIT) {
		return(B_TRANSPARENT_8_BIT);
	}

	map = system_colors();
	if (!map)
		return(0);

	index = (((c.red & 0xf8 ) << 7) |
		 ((c.green & 0xf8) << 2) |
		 ((c.blue & 0xf8) >> 3));

	return(map->index_map[index]);
}

/*-------------------------------------------------------------*/

uchar index_for_color(int r, int g, int b, int a)
{
	rgb_color a_color;

	a_color.red = r;
	a_color.green = g;
	a_color.blue = b;
	a_color.alpha = a;
	return(index_for_color(a_color));
}

/*-------------------------------------------------------------*/

int32 count_screens()
{ 
	return(1); 
}

/*-------------------------------------------------------------*/

rgb_color desktop_color()
{
	rgb_color	color;
	_BAppServerLink_ link;

	color.red = color.green = color.blue = 127;
	color.alpha = 255;
	link.session->swrite_l(GR_GET_DESKTOP_COLOR);
	link.session->swrite_l(0xffffffff);
	link.session->flush();
	link.session->sread(sizeof(rgb_color), &color);
	return(color);
}

/*-------------------------------------------------------------*/

void set_desktop_color(rgb_color c, bool stick)
{
	_BAppServerLink_ link;

	c.alpha = 255;
	link.session->swrite_l(GR_SET_DESKTOP_COLOR);
	link.session->swrite(sizeof(rgb_color), &c);
	link.session->swrite_l(0xffffffff);
	link.session->swrite_l(stick);
	link.session->flush();
}

/*-------------------------------------------------------------*/

void get_screen_info(screen_info* info)
{
	get_screen_info(0, info);
}

/*-------------------------------------------------------------*/
#include "PrivateScreen.h"
void get_screen_info(int32 index, screen_info *info)
{
	screen_desc scrn;
	BScreen screen( B_MAIN_SCREEN_ID );
	screen.private_screen()->get_screen_desc( &scrn );
									  
	info->mode = (color_space)scrn.video_mode;
	info->frame.Set(0, 0, scrn.h_size - 1, scrn.v_size - 1);
	info->spaces = scrn.space;
	info->min_refresh_rate = scrn.min_rate;
	info->max_refresh_rate = scrn.max_rate;
	info->refresh_rate = scrn.cur_rate;
	info->h_position = scrn.h_pos;
	info->v_position = scrn.v_pos;
	info->h_size = scrn.width;
	info->v_size = scrn.height;
}
#endif

/*-------------------------------------------------------------*/
#include <Screen.h>
const color_map *system_colors()
{ 
	BScreen screen( B_MAIN_SCREEN_ID );
	return screen.ColorMap(); 
}

/*-------------------------------------------------------------*/

status_t	get_mouse_type(int32 *type)
{
	BMessage reply;
	BMessage command(IS_GET_MOUSE_TYPE);

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_NO_ERROR)
		return (err);

	return (reply.FindInt32(IS_MOUSE_TYPE, type));
}

/*-------------------------------------------------------------*/

status_t	set_mouse_type(int32 type)
{
	BMessage reply;
	BMessage command(IS_SET_MOUSE_TYPE);
	command.AddInt32(IS_MOUSE_TYPE, type);

	return (_control_input_server_(&command, &reply));
}

/*-------------------------------------------------------------*/

status_t	get_mouse_map(mouse_map *map)
{
	BMessage reply;
	BMessage command(IS_GET_MOUSE_MAP);

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_NO_ERROR)
		return (err);

	const void	*theMap = NULL;
	ssize_t		theMapSize = 0;
	err = reply.FindData(IS_MOUSE_MAP, B_ANY_TYPE, &theMap, &theMapSize);
	if (err != B_NO_ERROR)
		return (err);
	memcpy(map, theMap, theMapSize);

	return (B_NO_ERROR);
}

/*-------------------------------------------------------------*/

status_t	set_mouse_map(mouse_map *map)
{
	BMessage reply;
	BMessage command(IS_SET_MOUSE_MAP);
	command.AddData(IS_MOUSE_MAP, B_ANY_TYPE, map, sizeof(mouse_map));

	return (_control_input_server_(&command, &reply));
}

/*-------------------------------------------------------------*/

status_t	get_click_speed(bigtime_t *speed)
{
	BMessage reply;
	BMessage command(IS_GET_CLICK_SPEED);

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_NO_ERROR)
		return (err);

	return (reply.FindInt64(IS_SPEED, speed));
}

/*-------------------------------------------------------------*/

status_t	set_click_speed(bigtime_t speed)
{
	BMessage reply;
	BMessage command(IS_SET_CLICK_SPEED);
	command.AddInt64(IS_SPEED, speed);

	return (_control_input_server_(&command, &reply));
}

/*-------------------------------------------------------------*/

status_t	get_mouse_speed(int32 *speed)
{
	BMessage reply;
	BMessage command(IS_GET_MOUSE_SPEED);

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_NO_ERROR)
		return (err);

	return (reply.FindInt32(IS_SPEED, speed));
}

/*-------------------------------------------------------------*/

status_t	set_mouse_speed(int32 speed)
{
	BMessage reply;
	BMessage command(IS_SET_MOUSE_SPEED);
	command.AddInt32(IS_SPEED, speed);

	return (_control_input_server_(&command, &reply));
}

/*-------------------------------------------------------------*/

status_t	get_mouse_acceleration(int32 *speed)
{
	BMessage reply;
	BMessage command(IS_GET_MOUSE_ACCELERATION);

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_NO_ERROR)
		return (err);

	return (reply.FindInt32(IS_SPEED, speed));
}

/*-------------------------------------------------------------*/

status_t	set_mouse_acceleration(int32 speed)
{
	BMessage reply;
	BMessage command(IS_SET_MOUSE_ACCELERATION);
	command.AddInt32(IS_SPEED, speed);

	return (_control_input_server_(&command, &reply));
}

/*-------------------------------------------------------------*/

status_t	set_key_repeat_rate(int32 rate)
{
	BMessage reply;
	BMessage command(IS_SET_KEY_REPEAT_RATE);
	command.AddInt32(IS_RATE, rate);

	return (_control_input_server_(&command, &reply));
}

/*-------------------------------------------------------------*/

status_t	set_key_repeat_delay(bigtime_t delay)
{
	BMessage reply;
	BMessage command(IS_SET_KEY_REPEAT_DELAY);
	command.AddInt64(IS_DELAY, delay);

	return (_control_input_server_(&command, &reply));
}
/*-------------------------------------------------------------*/

status_t	get_key_repeat_rate(int32 *rate)
{
	BMessage reply;
	BMessage command(IS_GET_KEY_REPEAT_RATE);

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_NO_ERROR)
		return (err);

	return (reply.FindInt32(IS_RATE, rate));
}

/*-------------------------------------------------------------*/

status_t	get_key_repeat_delay(bigtime_t *delay)
{
	BMessage reply;
	BMessage command(IS_GET_KEY_REPEAT_DELAY);

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_NO_ERROR)
		return (err);

	return (reply.FindInt64(IS_DELAY, delay));
}

/*-------------------------------------------------------------*/

status_t	set_screen_space(int32 index, uint32 res, bool stick)
{
	status_t			result;
	_BAppServerLink_	link;

	link.session->swrite_l(GR_SET_SCREEN_RES);
	link.session->swrite(sizeof(int32), &index);
	link.session->swrite(sizeof(int32), &res);
	link.session->swrite(sizeof(int32), &stick);
	link.session->flush();

	link.session->sread(sizeof(status_t), &result);
	return (result);
}

/*-------------------------------------------------------------*/

status_t	set_screen_refresh_rate(int32 index, float rate, bool stick)
{
	status_t			result;
	_BAppServerLink_	link;

	link.session->swrite_l(GR_SET_SCREEN_RATE);
	link.session->swrite(sizeof(int32), &index);
	link.session->swrite(sizeof(float), &rate);
	link.session->swrite_l(stick);
	link.session->flush();

	link.session->sread(sizeof(status_t), &result);
	return (result);
}

/*-------------------------------------------------------------*/

status_t	adjust_crt(int32 index, uchar x_pos, uchar y_pos,
				   uchar x_size, uchar y_size, bool stick)
{
	status_t			result;
	_BAppServerLink_	link;

	link.session->swrite_l(GR_SET_SCREEN_POS);
	link.session->swrite(sizeof(int32), &index);
	link.session->swrite(sizeof(char), &x_pos);
	link.session->swrite(sizeof(char), &y_pos);
	link.session->swrite(sizeof(char), &x_size);
	link.session->swrite(sizeof(char), &y_size);
	link.session->swrite_l(stick);
	link.session->flush();

	link.session->sread(sizeof(status_t), &result);
	return (result);
}

#if 0
/*-------------------------------------------------------------*/

void	activate_app(team_id team)
{
	int32		h0, v0, h, v;
	app_info	ainfo;
	bigtime_t	before, after;
	_BAppServerLink_ link;

	link.session->swrite_l(GR_ACTIVATE_TEAM);
	link.session->swrite_l(team);
	link.session->flush();
	link.session->sread(sizeof(int32), &h0);
	link.session->sread(sizeof(int32), &v0);
	link.session->sread(sizeof(int32), &h);
	link.session->sread(sizeof(int32), &v);
	
	if ((h != -10000) && (v != -10000)) {
		int32		i, step;
		float		dh, dv, x, y;
		float		dist;
	
		dh = (float)(h-h0);
		dv = (float)(v-v0);
		dist = sqrt(dh*dh+dv*dv);
		for (step=2; step<20; step++)
			if ((float)(step*(step+1)*(step-1)) > dist)
				break;
		dist = 6.0/(float)(step*(step+1)*(step-1));
		dh *= dist;
		dv *= dist;
		x = (float)h0+0.5;
		y = (float)v0+0.5;

		for (i=1; i<step; i++) {
			before = system_time() + 40000LL;
			x += dh*(float)(i*(step-i));
			y += dv*(float)(i*(step-i));
			{
				BMessage reply;
				BMessage command(IS_SET_MOUSE_POSITION);
				command.AddPoint(IS_WHERE, BPoint(h, v));
				_control_input_server_(&command, &reply);
			}
			after = system_time();
			before -= after;
			if (before > 0)
				snooze(before);
		}
	}

	// ### special case the browser for now to allow it to be activated
	// ### even if there are no windows open
	if (be_roster->GetAppInfo('SHRK', &ainfo) == B_NO_ERROR)
		if (team == ainfo.team)
			be_roster->UpdateActiveApp(ainfo.team);
}
#endif

#if 0
/*---------------------------------------------------------------*/
static void animate_window(BRect from_rect, BRect to_rect, int32 num_frames, bool
	restore_last)

/* there seemed to be a bug in the server-side part of this
function. (invalid values). I think that 15/16bpp might cause
much trouble as well. Be *very* careful if you intend to
use this function again. JBQ 051198 */
{
	int32	foo;

	_BAppServerLink_ link;
	link.session->swrite_l(GR_ZOOM);
	link.session->swrite_rect(&from_rect);
	link.session->swrite_rect(&to_rect);
	link.session->swrite(sizeof(int32), &num_frames);
	link.session->swrite(sizeof(bool), &restore_last);
	link.session->flush();
	link.session->sread(sizeof(int32), &foo);
}
#endif

/*-------------------------------------------------------------*/

int32	count_workspaces()
{
	_BAppServerLink_	link;
	int32				ws;

	link.session->swrite_l(GR_COUNT_WS);
	link.session->flush();

	link.session->sread(sizeof(int32), &ws);
	
	return ws;
}

/*-------------------------------------------------------------*/

void	set_workspace_count(int32 count)
{
	_BAppServerLink_	link;

	link.session->swrite_l(GR_SET_WS_COUNT);
	link.session->swrite(sizeof(int32), &count);
	link.session->flush();
}

/*-------------------------------------------------------------*/

int32	current_workspace()
{
	_BAppServerLink_	link;
	int32				ws;

	link.session->swrite_l(GR_CURRENT_WS);
	link.session->flush();

	link.session->sread(sizeof(int32), &ws);
	
	return ws;
}
/*-------------------------------------------------------------*/

void	activate_workspace(int32 ws)
{
	_BAppServerLink_	link;

	link.session->swrite_l(GR_SELECT_WS);
	link.session->swrite(sizeof(int32), &ws);
	link.session->flush();
}

/*-------------------------------------------------------------*/

bigtime_t	idle_time()
{
	bigtime_t	dtime;
	_BAppServerLink_	link;

	link.session->swrite_l(GR_GET_IDLE_TIME);
	link.session->flush();
	link.session->sread(sizeof(bigtime_t), &dtime);

	return dtime;
}

/*-------------------------------------------------------------*/

status_t	get_scroll_bar_info(scroll_bar_info *info)
{
	_BAppServerLink_ link;
	status_t		result;

	link.session->swrite_l(GR_GET_SCROLL_BAR_INFO);
	link.session->flush();

	link.session->sread(sizeof(int32), &result);
	link.session->sread(sizeof(bool), &info->proportional);
	link.session->sread(sizeof(bool), &info->double_arrows);
	link.session->sread(sizeof(int32), &info->knob);
	link.session->sread(sizeof(status_t), &info->min_knob_size);
	
	return result;
}

/*-------------------------------------------------------------*/

status_t	set_scroll_bar_info(scroll_bar_info *info)
{
	_BAppServerLink_ link;
	status_t		result;

	link.session->swrite_l(GR_SET_SCROLL_BAR_INFO);
	link.session->swrite(sizeof(bool), &info->proportional);
	link.session->swrite(sizeof(bool), &info->double_arrows);
	link.session->swrite(sizeof(int32), &info->knob);
	link.session->swrite(sizeof(int32), &info->min_knob_size);
	link.session->flush();

	link.session->sread(sizeof(status_t), &result);
	
	return result;
}

/*-------------------------------------------------------------*/
// If team_id == -1, count every window.

int32	count_windows(team_id app)
{
	_BAppServerLink_ link;
	int32			 result;

	link.session->swrite_l(GR_COUNT_WINDOWS);
	link.session->swrite_l(app);
	link.session->flush();
	link.session->sread(sizeof(int32), &result);

	return result;
}

/*-------------------------------------------------------------*/

int32	*get_token_list(team_id app, int32 *count)
{
	_BAppServerLink_ link;
	int32			size;
	int32			*p;

	link.session->swrite_l(GR_GET_TOKEN_LIST);
	link.session->swrite_l(app);
	link.session->flush();
	link.session->sread(sizeof(int32), &size);
	p = (int32 *)malloc(size);
	link.session->sread(size, p);
	*count = size / sizeof(int32); 
	return p;
}

/*-------------------------------------------------------------*/

window_info	*get_window_info(int32 a_token)
{
	_BAppServerLink_ link;
	int32			 size;
	window_info		 *p;

	link.session->swrite_l(GR_GET_WINDOW_INFO);
	link.session->swrite_l(a_token);
	link.session->flush();
	link.session->sread(sizeof(int32), &size);
	if (size == 0)
		return 0;

	p = (window_info *)malloc(size);
	link.session->sread(size, p);
	return p;
}

/*-------------------------------------------------------------*/

void	do_window_action(int32 window_token, int32 action, 
						 BRect zoomRect, bool zoom)
{
	int32				h0, v0, h, v;
	_BAppServerLink_	link;

	link.session->swrite_l(GR_DO_WINDOW_ACTION);
	link.session->swrite_l(window_token);
	link.session->swrite_l(action);
	link.session->swrite_rect_a(&zoomRect);
	link.session->swrite(sizeof(bool), &zoom);
	link.session->flush();
	link.session->sread(sizeof(int32), &h0);
	link.session->sread(sizeof(int32), &v0);
	link.session->sread(sizeof(int32), &h);
	link.session->sread(sizeof(int32), &v);
	
	if ((h != -10000) && (v != -10000)) {
		int32		i, step;
		float		dh, dv, x, y;
		float		dist;
		bigtime_t	before, after;
	
		if (h0 == -10000) {
			BMessage reply;
			BMessage command(IS_SET_MOUSE_POSITION);
			command.AddPoint(IS_WHERE, BPoint(h, v));
			_control_input_server_(&command, &reply);
		}
		else {	
			dh = (float)(h-h0);
			dv = (float)(v-v0);
			dist = sqrt(dh*dh+dv*dv);
			i = (int32)(dist*10);
			if (dist < 1.0)
				step = 0;
			else
				for (step=2; step<40; step++)
					if ((float)(step*(step+1)*(step-1)) > i)
						break;
			dist = 6.0/(float)(step*(step+1)*(step-1));
			dh *= dist;
			dv *= dist;
			x = (float)h0+0.5;
			y = (float)v0+0.5;
	
			for (i=1; i<step; i++) {
				before = system_time() + 12000LL;
				x += dh*(float)(i*(step-i));
				y += dv*(float)(i*(step-i));
				{
					BMessage reply;
					BMessage command(IS_SET_MOUSE_POSITION);
					command.AddPoint(IS_WHERE, BPoint(floor(x), floor(y)));
					_control_input_server_(&command, &reply);
				}
				after = system_time();
				before -= after;
				if (before > 0)
					snooze(before);
			}
		}
	}
}

/*-------------------------------------------------------------*/

void	do_minimize_team(BRect zoom_rect, team_id app, bool zoom)
{
	_BAppServerLink_ link;
	int32			 foo;

	link.session->swrite_l(GR_DO_MINI_TEAM);
	link.session->swrite_l(app);
	link.session->swrite_rect_a(&zoom_rect);
	link.session->swrite(sizeof(bool), &zoom);
	link.session->flush();
	link.session->sread(sizeof(int32), &foo);
}

/*-------------------------------------------------------------*/

void	do_bring_to_front_team(BRect zoom_rect, team_id app, bool zoom)
{
	_BAppServerLink_ link;
	int32			 foo;

	link.session->swrite_l(GR_DO_ACTIVATE_TEAM);
	link.session->swrite_l(app);
	link.session->swrite_rect_a(&zoom_rect);
	link.session->swrite(sizeof(bool), &zoom);
	link.session->flush();
	link.session->sread(sizeof(int32), &foo);
}

//------------------------------------------------------------------

status_t get_key_info(key_info *info)
{
	BMessage reply;
	BMessage command(IS_GET_KEY_INFO);

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_NO_ERROR)
		return (err);

	const void	*theInfo = NULL;
	ssize_t		theInfoSize = 0;
	err = reply.FindData(IS_KEY_INFO, B_ANY_TYPE, &theInfo, &theInfoSize);
	if (err != B_NO_ERROR)
		return (err);
	memcpy(info, theInfo, theInfoSize);

	return (B_NO_ERROR);
}

/*-------------------------------------------------------------*/

rgb_color keyboard_navigation_color()
{
	return ui_color(B_NAVIGATION_BASE_COLOR);
}

/*-------------------------------------------------------------*/

void set_focus_follows_mouse(bool follow)
{
	_BAppServerLink_	link;

	link.session->swrite_l(GR_SET_FOCUS_FOLLOWS_MOUSE);
	link.session->swrite(sizeof(bool), &follow);
	link.session->flush();
}

/*-------------------------------------------------------------*/

status_t get_deskbar_frame(BRect	*rect)
{
	if (!rect)
		return B_BAD_VALUE;

	BMessenger mess(TASK_BAR_MIME_SIG);
	//	workaround deadlock if this call is made
	//	from a view that is within Deskbar
	//	see notes and such and match functionality
	//	in BDeskbar::Frame()
	if (mess.Team() == be_app_messenger.Team()) {
		rect->Set(0.0,0.0,0.0,0.0);
		if (!be_app)
			return B_ERROR;
		
		//	find the window named 'Deskbar' if in the
		//	Deskbar team
		BWindow* window = NULL;	
		int32 count = be_app->CountWindows();
		for (int32 windex=0 ; windex<count ; windex++) {
			window = be_app->WindowAt(windex);
			if (window && strcmp(window->Title(), "Deskbar") == 0)
				break;	
		}
		if (window) {
			//	return the window frame directly
			*rect = window->Frame();
			return B_OK;
		}
		
		return B_ERROR;	
	}
	
	status_t		err;
	BMessage        msg(B_GET_PROPERTY);
	BMessage        reply;

	msg.AddSpecifier("Frame");
	msg.AddSpecifier("Window", "Deskbar");
	if ((err = mess.SendMessage(&msg, &reply)) == B_OK)
		err = reply.FindRect("result", rect);
	return err;
}

/*-------------------------------------------------------------*/

bool focus_follows_mouse()
{
	_BAppServerLink_	link;
	bool				ffm = false;
	
	link.session->swrite_l(GR_GET_FOCUS_FOLLOWS_MOUSE);
	link.session->flush();

	link.session->sread(sizeof(bool), &ffm);

	return (ffm);
}

/*-------------------------------------------------------------*/

void _find_unblocked_screen_(team_id bad_team, BScreen *, int32 *ws_index)
{
	int32				screen_index;
	_BAppServerLink_	link;
	
	link.session->swrite_l(GR_GET_UNBLOCKED_SCREEN);
	link.session->swrite(sizeof(team_id), &bad_team);
	link.session->flush();

	link.session->sread(4, &screen_index);
	link.session->sread(4, ws_index);
}

/*-------------------------------------------------------------*/

void set_mouse_mode(mode_mouse mode)
{
	uchar value = (uchar)mode;
	
	//  if its not an ffm mode (including warping and instant warping)
	//	and its not 0 then just turn ffm on
	if (mode == 2 || (mode != 7 && mode >= 4))
		value = 1;
		
	_BAppServerLink_	link;
	
	link.session->swrite_l(GR_SET_FOCUS_FOLLOWS_MOUSE);
	link.session->swrite(sizeof(uchar), &value);
	link.session->flush();
}

/*-------------------------------------------------------------*/

mode_mouse	mouse_mode()
{
	_BAppServerLink_	link;
	mode_mouse			mode;
	uchar 				value;
	
	link.session->swrite_l(GR_GET_FOCUS_FOLLOWS_MOUSE);
	link.session->flush();
	link.session->sread(sizeof(uchar), &value);
	
	mode = (mode_mouse)value;

	return mode;
}
