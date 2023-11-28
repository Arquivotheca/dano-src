//*****************************************************************************
//
//	File:		View.cpp
//
//	Description:	BView class.
//			Application View objects.
//
//	Written by:	Peter Schillings & Benoit Potrebic
//
//	Copyright 1992-97, Be Incorporated
//
//*****************************************************************************

#define _CACHE_	1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Debug.h>
#include <MessageQueue.h>
#include <PropertyInfo.h>
#include <Window.h>

#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _MESSAGES_H
#include <messages.h>
#endif
#ifndef _TOKEN_SPACE_H
#include <token.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _INTERFACE_DEFS_H
#include "InterfaceDefs.h"
#endif
#ifndef _BITMAP_H
#include "Bitmap.h"
#endif
#ifndef _POLYGON_H
#include "Polygon.h"
#endif
#ifndef _VIEW_H
#include "View.h"
#endif
#ifndef	_REGION_H
#include "Region.h"
#endif
#ifndef	_IREGION_H
#include "IRegion.h"
#endif
#ifndef _SESSION_H
#include <session.h>
#endif
#ifndef _TOOL_TIP_H
#include <ToolTip.h>
#endif
#ifndef _INTERFACE_MISC_H
#include <interface_misc.h>
#endif
#ifndef _MESSAGE_UTIL_H
#include <message_util.h>
#endif
#ifndef _APP_DEFS_PRIVATE_H
#include <AppDefsPrivate.h>
#endif
#ifndef _PICTURE_H
#include <Picture.h>
#endif
#ifndef _SCROLL_BAR_H
#include <ScrollBar.h>
#endif
#ifndef _ARCHIVE_DEFS_H
#include <archive_defs.h>
#endif
#ifndef _SHELF_H
#include <Shelf.h>
#endif
#ifndef _SHARED_FONTS_H
#include <shared_fonts.h>
#endif
#ifndef _SHARED_DEFAULTS_H
#include <shared_defaults.h>
#endif
#ifndef __BSTRING__
#include <String.h>
#endif

#if _R5_COMPATIBLE_
#include <Button.h>
#endif

extern const rgb_color _B_SEMI_OLD_TRANSPARENT_32_BIT_;
extern const rgb_color _B_OLD_TRANSPARENT_32_BIT_; 

#include <shared_defs.h>
#include <Shape.h>
#include <Cursor.h>
#include <StreamIO.h>
#include <StringIO.h>

//#define PRINT_OPS 1

#if PRINT_OPS
#define OPOUT str___
#define PRINT_OP_IN(view)															\
	{																				\
		BStringIO str___;															\
		str___ << "OP in ";															\
		if (view->Name() && *view->Name())											\
			str___ << view->Name() << "/";											\
		str___ << view;																\
		str___ << " of ";															\
		if (view->Window() && view->Window()->Name() && *view->Window()->Name())	\
			str___ << view->Window()->Name() << "/";								\
		str___ << view->Window() << ": ";
#define PRINT_OP(stream)															\
		str___ << stream
#define EXEC_OP(cmds)																\
		cmds
#define PRINT_OP_OUT()																\
		BErr << str___.String();													\
	}
#else
#define PRINT_OP_IN(view)
#define PRINT_OP(x)
#define EXEC_OP(cmds)
#define PRINT_OP_OUT()
#endif

/*---------------------------------------------------------------*/

// pre-calculate the following for efficient filtering
static const uint32 kSemiOldTransparent32Bit = *((uint32 *)&_B_SEMI_OLD_TRANSPARENT_32_BIT_);
static const uint32 kOldTransparent32Bit = *((uint32 *)&_B_OLD_TRANSPARENT_32_BIT_);

/*---------------------------------------------------------------*/

#define	is_short(n)	(n > -32767 && n < 32768)
#define kHideValue 16384
#define kIsShown 0

/*---------------------------------------------------------------*/
const pattern B_SOLID_HIGH = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}}; 
const pattern B_MIXED_COLORS = {{0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55}}; 
const pattern B_SOLID_LOW = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}; 

/*---------------------------------------------------------------*/

BView::BView(BRect frame, const char* name, uint32 resizeMask, uint32 flags)
	: BHandler()
{
	InitData(frame, name, resizeMask, flags);
}

/*------------------------------------------------------------*/

rgb_color _long_to_color_(uint32 color)
{
	rgb_color	rgb;

	rgb.red = (color >> 24);
	rgb.green = (color >> 16);
	rgb.blue = (color >> 8);
	rgb.alpha = color;

	return rgb;
}

/*------------------------------------------------------------*/

ulong _color_to_long_(rgb_color rgb)
{
	return (rgb.red << 24) | (rgb.green << 16) | (rgb.blue << 8) | rgb.alpha;
}

/*------------------------------------------------------------*/
BView::BView(BMessage *data)
	: BHandler(data)
{
	BRect		frame;
	const char	*str;
	long		mode;
	long		flags;
	long		error;
	float		f;
	long		l;

	// ??? how to handle errors here?

	error = data->FindRect(S_FRAME, &frame);
	error = error | data->FindString(S_NAME, &str);
	error = error | data->FindInt32(S_RESIZE_MODE, &mode);
	error = error | data->FindInt32(S_FLAGS, &flags);
	InitData(frame, str, mode, flags);

	bool hidden;
	if( data->FindBool(S_HIDDEN, &hidden) == B_OK && hidden ) Hide();
	
	if (data->HasString(S_TOOL_TIP_TEXT))
		data->FindString(S_TOOL_TIP_TEXT, &fToolTipText);
	
	uint32	mask = 0;
	BFont	aFont;

	if (data->HasString(S_FONT_FAMILY_STYLE)) {
		const char *str2 = NULL;
		data->FindString(S_FONT_FAMILY_STYLE, &str);
		data->FindString(S_FONT_FAMILY_STYLE, 1, &str2);
		if (aFont.SetFamilyAndStyle(str, str2) == B_OK) {
			mask |= B_FONT_FAMILY_AND_STYLE;
		} else {
			printf("Error setting family and style from archive: %s/%s\n", str, str2);
		}
	}

	if (data->HasFloat(S_FONT_FLOATS)) {
		float fsize;
		float fshear;
		float frotation;
		error = error | data->FindFloat(S_FONT_FLOATS, &fsize);
		error = error | data->FindFloat(S_FONT_FLOATS, 1, &fshear);
		error = error | data->FindFloat(S_FONT_FLOATS, 2, &frotation);
		
		if (fsize != -1) {
			mask |= B_FONT_SIZE;
			aFont.SetSize(fsize);
		}
		if (fshear != -1) {
			mask |= B_FONT_SHEAR;
			aFont.SetShear(fshear);
		}
		if (frotation != -1) {
			mask |= B_FONT_ROTATION;
			aFont.SetRotation(frotation);
		}
	}
		
	if (mask != 0)
		SetFont(&aFont, mask);

	if (data->HasFloat(S_PEN_SIZE)) {
		data->FindFloat(S_PEN_SIZE, &f);
		SetPenSize(f);
	}
	if (data->HasInt32(S_DRAW_MODE)) {
		data->FindInt32(S_DRAW_MODE, &l);
		SetDrawingMode((drawing_mode) l);
	}

	if (data->HasInt32(S_COLORS)) {
		ulong high;
		ulong low;
		ulong view;
		data->FindInt32(S_COLORS, (long*) &high);
		data->FindInt32(S_COLORS, 1, (long*) &low);
		data->FindInt32(S_COLORS, 2, (long*) &view);
		SetHighColor(_long_to_color_(high));
		SetLowColor(_long_to_color_(low));
		SetViewColor(_long_to_color_(view));
	}

	if (data->HasInt32(S_EVENT_MASK)) {
		uint32 mask,options;
		data->FindInt32(S_EVENT_MASK, (int32*)&mask);
		data->FindInt32(S_EVENT_MASK, 1, (int32*)&options);
		SetEventMask(mask,options);
	};

	if (data->HasPoint(S_PEN_LOCATION)) {
		BPoint p;
		data->FindPoint(S_PEN_LOCATION, &p);
		MovePenTo(p);
	};

	if (data->HasInt16(S_LINE_MODE_CAPJOIN)) {
		int16 cap,join;
		float miter = B_DEFAULT_MITER_LIMIT;
		data->FindInt16(S_LINE_MODE_CAPJOIN, &cap);
		data->FindInt16(S_LINE_MODE_CAPJOIN, 1, &join);
		if (data->HasFloat(S_LINE_MODE_MITER))
			data->FindFloat(S_LINE_MODE_MITER, &miter);
		SetLineMode((cap_mode)cap,(join_mode)join,miter);
	};

	if (data->HasPoint(S_ORIGIN_LOCATION)) {
		BPoint p;
		data->FindPoint(S_ORIGIN_LOCATION, &p);
		SetOrigin(p);
	};

	if (data->HasInt16(S_BLENDING_MODE)) {
		int16 srcAlpha,alphaFunc;
		data->FindInt16(S_BLENDING_MODE, &srcAlpha);
		data->FindInt16(S_BLENDING_MODE, 1, &alphaFunc);
		SetBlendingMode((source_alpha)srcAlpha, (alpha_function)alphaFunc);
	};

	if (data->HasInt32(S_DOUBLE_BUFFERING)) {
		uint32 doubleBuffering;
		data->FindInt32(S_DOUBLE_BUFFERING, (int32*)&doubleBuffering);
		SetDoubleBuffering(doubleBuffering);
		
#ifdef DIM_BACKGROUND_WINDOWS	
	} else {
		SetDoubleBuffering(B_UPDATE_INVALIDATED|B_UPDATE_EXPOSED);
#endif

	}
	
	UnarchiveChildren(data);
}

/*---------------------------------------------------------------*/

status_t BView::UnarchiveChildren(BMessage *data, BWindow *window)
{
	long		i = 0;
	BMessage	archive;
	BArchivable	*obj;

	while (data->FindMessage(S_VIEWS, i++, &archive) == B_OK) {
		obj = instantiate_object(&archive);
		if (!obj)
			continue;
//+		PRINT(("(view) AddChild: class_name=%s\n", class_name(obj)));
		BView *child = dynamic_cast<BView *>(obj);
		if (child)
			// GOOFY - can't call top_view->AddChild(). Must use
			// BWindow::AddChild().
			window ? window->AddChild(child) : AddChild(child);
	}
	return 0;
}

/*---------------------------------------------------------------*/

status_t BView::Archive(BMessage *data, bool deep) const
{
	BHandler::Archive(data, deep);

	data->AddRect(S_FRAME, Frame());

	if (fEventMask) {
		data->AddInt32(S_EVENT_MASK, fEventMask);
		data->AddInt32(S_EVENT_MASK, fEventOptions);
	};

	if (m_doubleBuffering) {
		data->AddInt32(S_DOUBLE_BUFFERING, m_doubleBuffering);
	};

	if (ResizingMode() != 0)
		data->AddInt32(S_RESIZE_MODE, ResizingMode());

	if (Flags() != 0)
		data->AddInt32(S_FLAGS, Flags());

	if( IsHidden(this) )
		data->AddBool(S_HIDDEN, true);
	
	if (fToolTipText.Length() > 0)
		data->AddString(S_TOOL_TIP_TEXT, fToolTipText.String());
	
	if (fState) {
		if (owner && ((fState->f_mask & B_FONT_ALL) != B_FONT_ALL))
			const_cast<BView*>(this)->fetch_font();
		uint32 f_mask = fState->f_nonDefault;

		// when archiving font info use the values the user/developer
		// selected, even though these values my be invalid (i.e. font
		// name doesn't exist)
		if (f_mask & B_FONT_FAMILY_AND_STYLE) {
			font_family	family;
			font_style	style;
			fState->font.GetFamilyAndStyle(&family, &style);
			data->AddString(S_FONT_FAMILY_STYLE, family);
			data->AddString(S_FONT_FAMILY_STYLE, style);
		}
		if ((f_mask & B_FONT_SIZE) || (f_mask & B_FONT_SHEAR) ||
			(f_mask & B_FONT_ROTATION)) {
				data->AddFloat(S_FONT_FLOATS,
					(f_mask & B_FONT_SIZE) ? fState->font.Size() : -1);
				data->AddFloat(S_FONT_FLOATS,
					(f_mask & B_FONT_SHEAR) ? fState->font.Shear() : -1);
				data->AddFloat(S_FONT_FLOATS,
					(f_mask & B_FONT_ROTATION) ? fState->font.Rotation() : -1);
		}
		if (fState->b_view_color || 
			(fState->new_local & B_HIGH_COLOR_VALID) ||
			(fState->new_local & B_LOW_COLOR_VALID)) {
				rgb_color c;
				c = HighColor();
				data->AddInt32(S_COLORS, _color_to_long_(c));
				c = LowColor();
				data->AddInt32(S_COLORS, _color_to_long_(c));
				data->AddInt32(S_COLORS, _color_to_long_(fState->view_color));
		}

		if (fState->new_local & B_PEN_SIZE_VALID)
			data->AddFloat(S_PEN_SIZE, PenSize());
		if (fState->new_local & B_DRAW_MODE_VALID)
			data->AddInt32(S_DRAW_MODE, DrawingMode());
		if (fState->new_local & B_PEN_LOCATION_VALID)
			data->AddPoint(S_PEN_LOCATION, PenLocation());
		if (fState->new_local & B_LINE_MODE_VALID) {
			data->AddInt16(S_LINE_MODE_CAPJOIN, LineCapMode());
			data->AddInt16(S_LINE_MODE_CAPJOIN, LineJoinMode());
			data->AddFloat(S_LINE_MODE_MITER, LineMiterLimit());
		};
		if (fState->new_local & B_ORIGIN_VALID)
			data->AddPoint(S_ORIGIN_LOCATION, Origin());
		if (fState->new_local & B_BLENDING_MODE_VALID) {
			source_alpha	srcAlpha;
			alpha_function	alphaFunc;
			GetBlendingMode(&srcAlpha, &alphaFunc);
			data->AddInt16(S_BLENDING_MODE, srcAlpha);
			data->AddInt16(S_BLENDING_MODE, alphaFunc);
		};
	}
	
	if (deep) {
		// now archive all the child views
		ArchiveChildren(data, deep);
	}
	return 0;
}

/*---------------------------------------------------------------*/

status_t BView::ArchiveChildren(BMessage *data, bool deep) const
{
	BView *child;
	for (child = first_child; child; child = child->next_sibling) {
		BMessage archive;
		status_t err = child->Archive(&archive, deep);
		if (!err)
			data->AddMessage(S_VIEWS, &archive);
	}
	return 0;
}

/*---------------------------------------------------------------*/

BArchivable *BView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BView"))
		return NULL;
	return new BView(data);
}

/*---------------------------------------------------------------*/

void BView::InitData(BRect frame, const char* name, uint32 resizeMask,
	uint32 flags)
{
	fCachedBounds = frame;
	f_type  = resizeMask | flags;

	attached = false;

	next_sibling		= NULL;
	prev_sibling		= NULL;
	first_child			= NULL;
	parent				= NULL;
	top_level_view		= false;
	owner				= NULL;
	origin_h			= 0;
	origin_v			= 0;
	comm				= NULL;
	fVerScroller		= NULL;
	fHorScroller		= NULL;
	f_is_printing		= false;
	fEventMask 			= 0;
	fEventOptions 		= 0;
	cpicture			= NULL;
	m_doubleBuffering 	= 0;
	fFocusNoScroll		= false;
	fPulseDir			= false;
	fPulseValue			= 255;
	fStateLevel			= 0;
	
	pr_state = (void *)malloc(sizeof(_printing_state_));

	SetName(name);
	// All views start out as shown
	fShowLevel = kIsShown;

	server_token = -1;

	// fState pointer to the currect grapchpics cache. So during Updates()
	// it points to a temporary cache
	fState = NULL;

	// fPermanentState points to the real graphics state. 
	fPermanentState = NULL;
	fShelf = NULL;

	fNoISInteraction = false;

	init_cache();
	
#ifdef DIM_BACKGROUND_WINDOWS		
	SetDoubleBuffering(B_UPDATE_INVALIDATED|B_UPDATE_EXPOSED);
#endif

}

/*---------------------------------------------------------------*/

void	BView::init_cache()
{
	if (!fPermanentState) {
		fPermanentState = new _view_attr_();

		// set fState only if it isn't already set
		if (fState == NULL)
			fState = fPermanentState;
	}
}

/*---------------------------------------------------------------*/

void	BView::SetToolTipText(const char* text)
{
	fToolTipText = text;
	RefreshToolTipInfo();
}

/*---------------------------------------------------------------*/

const char* BView::ToolTipText() const
{
	return fToolTipText.String();
}

/*---------------------------------------------------------------*/

status_t BView::GetToolTipInfo(BPoint /*where*/, BRect* outRegion,
								BToolTipInfo* outInfo)
{
	if( fToolTipText.Length() <= 0 ) {
		*outRegion = BRect();
		return B_NO_INIT;
	}
	
	*outRegion = Frame().OffsetToCopy(0, 0);
	if( outInfo ) outInfo->SetText(fToolTipText.String());
	return B_OK;
}

/*---------------------------------------------------------------*/

void	BView::RefreshToolTipInfo() const
{
	if (owner) owner->SendToolTipInfo(this);
}

/*---------------------------------------------------------------*/

void	BView::AttachedToWindow()
{
}

/*---------------------------------------------------------------*/

void	BView::AllAttached()
{
}

/*---------------------------------------------------------------*/

void	BView::DetachedFromWindow()
{
}

/*---------------------------------------------------------------*/

void	BView::AllDetached()
{
}

/*---------------------------------------------------------------*/

status_t BView::UISettingsChanged(const BMessage* changes, uint32 )
{
	// Need to re-draw view if any font or colors have changed.
	const char* name;
	type_code type;
	if (changes->GetInfo(B_RGB_COLOR_TYPE, 0, &name, &type) == B_OK ||
			changes->GetInfo(B_FONT_TYPE, 0, &name, &type) == B_OK) {
		refresh_colors(*changes);
		Invalidate();
	}
	return B_OK;
}

/*---------------------------------------------------------------*/

void	BView::handle_ui_settings(const BMessage* changes, uint32 flags)
{
	BView	*tmp;
	
	tmp = first_child;

	while (tmp != 0) {

		tmp->UISettingsChanged(changes, flags);
		if (tmp->first_child)
			tmp->handle_ui_settings(changes, flags);

		tmp = tmp->next_sibling;
	}
}

void BView::refresh_colors(const BMessage& settings)
{
	// Don't let our changes clear out any UI color names.
	fStateLevel++;
	
	const char* name;
	rgb_color col;
	if ((name=fState->view_ui_color) &&
			settings.FindRGBColor(name, &col) == B_OK &&
			col != fState->view_color) {
		SetViewColor(col);
		fState->view_ui_color = name;
	}
	if ((name=fState->low_ui_color) &&
			settings.FindRGBColor(name, &col) == B_OK &&
			col != fState->low_color) {
		SetLowColor(col);
	}
	if ((name=fState->high_ui_color) &&
			settings.FindRGBColor(name, &col) == B_OK &&
			col != fState->high_color) {
		SetHighColor(col);
	}
	
	fStateLevel--;
}

/*---------------------------------------------------------------*/

void	BView::check_lock() const
{
	if (owner) {
		owner->check_lock();
		if (server_token != owner->fLastViewToken) {
			owner->fLastViewToken = server_token;
			owner->a_session->swrite_l(GR_PICK_VIEW);
			owner->a_session->swrite_l(server_token);
		};
	};
}

/*---------------------------------------------------------------*/

void	BView::check_lock_no_pick() const
{
	if (owner) owner->check_lock();
}

/*---------------------------------------------------------------*/

bool	BView::remove_from_list(BView *a_view)
{
	BView	*tmp;
	BView	*tmp1;

	tmp = first_child;
	if (tmp == a_view) {
		first_child = tmp->next_sibling;
		if (first_child)
			first_child->prev_sibling = NULL;
		return(true);
	}

	while (tmp) {
		tmp1 = tmp->next_sibling;
		if (tmp1 == a_view) {
			tmp->next_sibling = tmp1->next_sibling;
			if (tmp->next_sibling)
				tmp->next_sibling->prev_sibling = tmp;
			return(true);	
		}
		tmp = tmp1;
	}

	DEBUGGER("BView::remove_from_list - error trying to remove a view");
	return(false);
}

/*---------------------------------------------------------------*/

int32	BView::CountChildren() const
{
	long	count = 0;
	BView	*tmp;
	
	check_lock_no_pick();

	tmp = first_child;
	while (tmp) {
		count++;
		tmp = tmp->next_sibling;
	}
	return count;
}

/*-------------------------------------------------------------*/

BView*	BView::ChildAt(int32 index) const
{
	BView	*tmp;
	
	check_lock_no_pick();

	tmp = first_child;
	while (index-- && tmp) {
		tmp = tmp->next_sibling;
	}
	return tmp;
}

/*-------------------------------------------------------------*/

BView*	BView::NextSibling() const
{
	return next_sibling;
}

/*-------------------------------------------------------------*/

BView*	BView::PreviousSibling() const
{
	return prev_sibling;
}

/*---------------------------------------------------------------*/

void	BView::SetFlags(uint32 flags)
{
	long		bid;
	AppSession	*a_session;
	ulong		resizemask;

	resizemask = ResizingMode();
	f_type = flags | resizemask;

	if (owner) {
		check_lock();

		if (flags & B_PULSE_NEEDED) {
			owner->enable_pulsing(true);
		}

		a_session = owner->a_session;
		a_session->swrite_l(GR_VIEW_FLAGS);
		a_session->swrite_l(f_type);
		Flush();
		a_session->sread(4, &bid);
	}
}

/*---------------------------------------------------------------*/

void	BView::SetResizingMode(uint32 resizeMask)
{
	long		bid;
	AppSession	*a_session;
	ulong		oldflags;

	oldflags = Flags();
	f_type = resizeMask | oldflags;

	if (owner) {
		check_lock();

		a_session = owner->a_session;
		a_session->swrite_l(GR_VIEW_FLAGS);
		a_session->swrite_l(f_type);
		Flush();
		a_session->sread(4, &bid);
	}
}

/*---------------------------------------------------------------*/

uint32 BView::EventMask()
{
	return fEventMask;
};

status_t BView::SetEventMask(uint32 mask, uint32 options)
{
	if ((fEventMask == mask) && (fEventOptions == options)) return B_OK;
	fEventMask = mask;
	fEventOptions = options;

	if (owner) {
		check_lock();
		AppSession	*a_session = owner->a_session;
		a_session->swrite_l(GR_VIEW_SET_EVENT_MASK);
		a_session->swrite_l(fEventMask);
		a_session->swrite_l(options);
	};
	
	return B_OK;
}

status_t BView::SetMouseEventMask(uint32 mask, uint32 options)
{
	status_t result = B_ERROR;

	if (owner) {
		check_lock();
		AppSession	*a_session = owner->a_session;
		a_session->swrite_l(GR_VIEW_AUGMENT_EVENT_MASK);
		a_session->swrite_l(mask);
		a_session->swrite_l(options);
		a_session->flush();
		a_session->sread(4,&result);
	};
	
	return result;
}

/*---------------------------------------------------------------*/

void	BView::update_cached_state()
{
	BView	*a_child;

	if (server_token == -1) return;

	// -------------- //

	// do any saving of server_side info before view is detached.

	if ((fState->f_mask & B_FONT_ALL) != B_FONT_ALL) fetch_font();
	fCachedBounds = Frame();
	if (fShowLevel < 0)
		fCachedBounds.OffsetBy(kHideValue, 0);
//+	if (strcmp(Name(), "Red View") == 0) {
//+		PRINT(("UC: (%x) \n\t", server_token)); PRINT_OBJECT(fCachedBounds);
//+	}

	// -------------- //

	a_child = first_child;
	while (a_child) {
		a_child->update_cached_state();
		a_child = a_child->next_sibling;
	}
}

/*---------------------------------------------------------------*/

bool BView::remove_self()
{
	bool result = false;

	if (owner) {
		check_lock_no_pick();
		if (owner->CurrentFocus() == this)
			MakeFocus(false);
		owner->detach_builder(this);
		update_cached_state();
	}

	if (parent)
		result = parent->remove_from_list(this);
	
	if (!result)
		return false;

	if (owner) {
		AppSession	*a_session;

		if (this == owner->CurrentFocus())
			owner->set_focus(NULL, !fNoISInteraction);

		if (this == (const BView *) owner->fKeyMenuBar)
			owner->fKeyMenuBar = NULL;

		if (server_token == owner->fLastViewToken)
			owner->fLastViewToken = -1;

		a_session = owner->a_session;
		a_session->swrite_l(GR_REMOVE_VIEW);
		a_session->swrite_l(server_token);

		set_owner(NULL);
	}	

	next_sibling = NULL;
	prev_sibling = NULL;
	parent = NULL;
	top_level_view = false;

	return(true);
}

/*---------------------------------------------------------------*/

bool BView::RemoveChild(BView* childView)
{
	bool		result;

	if (childView->parent == this)
		result = childView->remove_self();
	else
		result = false;

	return(result);
}

/*---------------------------------------------------------------*/

bool	BView::RemoveSelf()
{
	return remove_self();
}

/*---------------------------------------------------------------*/

BView::~BView()
{
	BView		*tmp;
	BView		*tmp1;

	if (owner)
		debugger("Trying to delete a view that belongs to a window. Call RemoveSelf first.\n");
	remove_self();

	tmp = first_child;

	while (tmp) {
		tmp1 = tmp->next_sibling;
		delete tmp;
		tmp = tmp1;
	}

	// disassociate with attached scrollbars
	if (fHorScroller)
		fHorScroller->SetTarget((BView *)NULL);
	if (fVerScroller)
		fVerScroller->SetTarget((BView *)NULL);

	SetName(NULL);

	if (fPermanentState) {
		delete fPermanentState;
		fPermanentState = 0;
	}
	free((char *)pr_state);
	pr_state = 0;
}

/*---------------------------------------------------------------*/

void	BView::AddChild(BView *child, BView *before)
{
	BWindow	*w = Window();
//+	bool	locked = false;
	if (w) {
		if (!LockLooper())
			return;
		w = Window();
	}

	if (child->RealParent()) {
		debugger("AddChild failed - the view already belonged to someone else.\n");
		if (w)
			w->Unlock();
		return;
	}

	if (before && (before->RealParent() != this)) {
		debugger("AddChild failed - the \"before\" view and \"this\" don't match.\n");
		if (w)
			w->Unlock();
		return;
	}

	child->parent = this;
	child->set_owner(owner);

	if (first_child == 0) {
		first_child = child;
		ASSERT(first_child->next_sibling == NULL);
		ASSERT(first_child->prev_sibling == NULL);
	} else {
		if (before) {
			// add the child before the "before" view
			child->prev_sibling = before->prev_sibling;
			if (child->prev_sibling)
				child->prev_sibling->next_sibling = child;
			before->prev_sibling = child;
			child->next_sibling = before;
			if (first_child == before)
				first_child = child;
		} else {
			BView	*tmp = first_child;
			// add the view to the end of the list
			while (tmp->next_sibling != 0)
				tmp = tmp->next_sibling;

			tmp->next_sibling = child;
			child->next_sibling = 0;
			child->prev_sibling = tmp;
		}
	}

	if (server_token >= 0) {
//+		if (strcmp(Name(), "Red View") == 0) {
//+			PRINT(("VB: ")); PRINT_OBJECT(fCachedBounds);
//+		}
		owner->view_builder(child);
		owner->attach_builder(child);
	}

	if (w)
		w->Unlock();
}

/*---------------------------------------------------------------*/

void	BView::set_owner(BWindow *the_owner)
{
	BView	*a_child;

// if we are changing the owner of the view, check to see if this
// view was the current focus of the owner.
// If yes, reset the current focus to no focus.

	if (the_owner == 0) {
		remove_comm_array();
		server_token = -1;
	}

	if ((the_owner != owner) && (owner)) {
		owner->RemoveHandler(this);

		// if there is an assoc shelf remove it as well.
		if (fShelf)
			owner->RemoveHandler(fShelf);
	}

	if (the_owner && (the_owner != owner)) {
#if DEBUG
		if (!the_owner->IsLocked())
			TRACE();
#endif
		the_owner->AddHandler(this);

		// If there is a 'shelf' then add to this window as well.
		if (fShelf)
			the_owner->AddHandler(fShelf);

		// the Nexthandler of view is the parent view. And if their
		// isn't a parent view then it is the window.
		SetNextHandler(top_level_view ? (BHandler *) the_owner : (BHandler *) parent);
	}

	owner = the_owner;
	
	a_child = first_child;
	while (a_child) {
		a_child->set_owner(the_owner);
		a_child = a_child->next_sibling;
	}
}

/*---------------------------------------------------------------*/

bool	BView::do_owner_check() const
{
 	if (owner) {
		check_lock();
 		return true;
 	} else {
		debugger("View method requires owner and doesn't have one.\n");
 		return false;
	}
}

/*---------------------------------------------------------------*/

BPoint BView::ConvertToScreen(BPoint pt) const
{
	ConvertToScreen(&pt);
	return pt;
}

/*---------------------------------------------------------------*/

void	BView::ConvertToScreen(BPoint *pt) const
{
	AppSession* a_session;

	if (!do_owner_check()) return;
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_PT_TO_SCREEN);
	a_session->swrite_point_a(pt);
	Flush();

	a_session->sread_point_a(pt);

	if (fShowLevel < 0) pt->x -= kHideValue;
}

/*---------------------------------------------------------------*/

BPoint BView::ConvertFromScreen(BPoint pt) const
{
	ConvertFromScreen(&pt);
	return pt;
}

/*---------------------------------------------------------------*/

void	BView::ConvertFromScreen(BPoint *pt) const
{
	AppSession* a_session;

	if (!do_owner_check()) return;

	a_session = owner->a_session;
	a_session->swrite_l(GR_PT_FROM_SCREEN);
	a_session->swrite_point_a(pt);
	Flush();

	a_session->sread_point_a(pt);
	if (fShowLevel < 0) pt->x += kHideValue;
}

/*---------------------------------------------------------------*/

BRect BView::ConvertToScreen(BRect rect) const
{
	ConvertToScreen(&rect);
	return rect;
}

/*---------------------------------------------------------------*/

void BView::ConvertToScreen(BRect *bounds) const
{
	AppSession* a_session;

	if (!do_owner_check()) return;

	a_session = owner->a_session;
	a_session->swrite_l(GR_RECT_TO_SCREEN);
	a_session->swrite_rect_a(bounds);
	Flush();

	a_session->sread_rect_a(bounds);
	if (fShowLevel < 0) bounds->OffsetBy(-kHideValue,0);
}

/*---------------------------------------------------------------*/

BRect BView::ConvertFromScreen(BRect rect) const
{
	ConvertFromScreen(&rect);
	return rect;
}

/*---------------------------------------------------------------*/

void BView::ConvertFromScreen(BRect *bounds) const
{
	AppSession* a_session;

	if (!do_owner_check()) return;

	a_session = owner->a_session;
	a_session->swrite_l(GR_RECT_FROM_SCREEN);
	a_session->swrite_rect_a(bounds);
	Flush();

	a_session->sread_rect_a(bounds);
	if (fShowLevel < 0) bounds->OffsetBy(kHideValue,0);
}

/*---------------------------------------------------------------*/

BPoint BView::ConvertToParent(BPoint pt) const
{
	ConvertToParent(&pt);
	return pt;
}

/*---------------------------------------------------------------*/

void BView::ConvertToParent(BPoint *location) const
{
	check_lock();

	if (RealParent()) {
		ConvertToScreen(location);
		RealParent()->ConvertFromScreen(location);
	}
}

/*---------------------------------------------------------------*/

BPoint BView::ConvertFromParent(BPoint pt) const
{
	ConvertFromParent(&pt);
	return pt;
}

/*---------------------------------------------------------------*/

void BView::ConvertFromParent(BPoint *location) const
{
	check_lock();

	if (RealParent()) {
		RealParent()->ConvertToScreen(location);
		ConvertFromScreen(location);
	}
}

/*---------------------------------------------------------------*/

BRect BView::ConvertToParent(BRect rect) const
{
	ConvertToParent(&rect);
	return rect;
}

/*---------------------------------------------------------------*/

void BView::ConvertToParent(BRect *bounds) const
{
	check_lock();

	if (RealParent()) {
		ConvertToScreen(bounds);
		RealParent()->ConvertFromScreen(bounds);
	}
}

/*---------------------------------------------------------------*/

BRect BView::ConvertFromParent(BRect rect) const
{
	ConvertFromParent(&rect);
	return rect;
}

/*---------------------------------------------------------------*/

void BView::ConvertFromParent(BRect *bounds) const
{
	check_lock();

	if (RealParent()) {
		RealParent()->ConvertToScreen(bounds);
		ConvertFromScreen(bounds);
	}
}

/*---------------------------------------------------------------*/

BRect	BView::Frame() const
{
	BRect		r;
	AppSession	*a_session;
	
	if (server_token == -1) {
		r = fCachedBounds;
	} else {
		check_lock();
		a_session = owner->a_session;
		bool isPrinting = Parent() && Parent()->IsPrinting();
		a_session->swrite_l(isPrinting?GR_GET_LOCATION:GR_GET_SCREWED_UP_LOCATION);
		Flush();

		a_session->sread_rect(&r);
		if (isPrinting) {
			BRect p = Parent()->Bounds();
			r.left -= p.left;
			r.right -= p.left;
			r.top -= p.top;
			r.bottom -= p.top;
		};
	}
	
	if (fShowLevel < 0)
		r.OffsetBy(-kHideValue, 0);
	
	return r;
}

/*---------------------------------------------------------------*/

BView	*BView::Parent() const
{
	// we hide the the private 'top_view'

	return(top_level_view ? NULL : parent);
}

/*---------------------------------------------------------------*/

BView	*BView::RealParent() const
{
	return(parent);
}

/*---------------------------------------------------------------*/

BRect	BView::Bounds() const
{
	BRect		r;
	AppSession	*a_session;
	
	if (f_is_printing) {
		return ((_printing_state_ *)(pr_state))->clipping;
	}

	if (server_token == -1) {
		r.top = 0;
		r.left = 0;
		r.bottom = fCachedBounds.bottom - fCachedBounds.top;
		r.right = fCachedBounds.right - fCachedBounds.left;
	} else {

		if (fCachedBounds.bottom >= fCachedBounds.top)  {
			return(fCachedBounds);
		}

		check_lock();
		a_session = owner->a_session;
		a_session->swrite_l(GR_GET_VIEW_BOUND);
		Flush();

		a_session->sread_rect_a(&r);
	}
	return r;
}

/*---------------------------------------------------------------*/

uint32 BView::DoubleBuffering()
{
	return m_doubleBuffering;
};

void BView::SetDoubleBuffering(uint32 updateTypes)
{
	if (m_doubleBuffering == updateTypes) return;

	m_doubleBuffering = updateTypes;

	if (!owner) return;

	check_lock();
	AppSession	*a_session;
	a_session = owner->a_session;
	a_session->swrite_l(GR_SET_VIEW_DOUBLE_BUFFER);
	a_session->swrite_l(updateTypes);
}

/*---------------------------------------------------------------*/

uint32 BView::GetUpdateHints()
{
	return 0; // XXX
}

void BView::Draw(BRect)
{
}

void BView::DrawAfterChildren(BRect)
{
}

/*---------------------------------------------------------------*/

void	BView::BeginRectTracking(BRect startRect, uint32 style)
{
	// get rect in global coordinates
	ConvertToScreen((BPoint*)&startRect.left);
	ConvertToScreen((BPoint*)&startRect.right);

	be_app->BeginRectTracking(startRect, (style == B_TRACK_WHOLE_RECT));
}

/*---------------------------------------------------------------*/

void	BView::EndRectTracking()
{
	be_app->EndRectTracking();
}

/*---------------------------------------------------------------*/

void	BView::MouseMoved(BPoint, uint32, const BMessage *)
{
}

/*---------------------------------------------------------------*/

void	BView::MouseDown(BPoint)
{
}

/*---------------------------------------------------------------*/

void	BView::MouseUp(BPoint)
{
}

/*---------------------------------------------------------------*/

void	BView::GetMouse(BPoint *location, uint32 *buttons, bool checkQueue)
{
	AppSession	*a_session;

	/*	The check_lock is special cased here because it turns out we can
		enter a nested event loop inside a GetMouse() call, which can
		change the session's "current" view.  So we'll be sure to set the
		current view manually right before we need it. */
 	if (owner)
		check_lock_no_pick();
 	else {
		debugger("View method requires owner and doesn't have one.\n");
 		return;
	}

	if (checkQueue) {
		/*
		 First, look at the message queue and see if there are any MOUSE UP/DOWN
		 events. As we walk the event queue delete any B_MOUSE_MOVED events.
		 Don't want this old data remaining in the queue.
		*/
	
		// This variable is used to flag if we found an update-pending
		// message, so that we should call do_draw_views() after unlocking
		// the queue.  (do_draw_views() will call user code, which could
		// lead to a deadlock if it tries to send a message.)
		bool foundUpdate = false;
		
		if (find_thread(NULL) == Window()->Thread())
			Window()->DequeueAll();

		BMessageQueue *q = Window()->MessageQueue();
		q->Lock();

		int32 index = 0;
		int32 count = q->CountMessages();
		int32 ccc = count;
		while (index < count) {
			BMessage *msg = q->FindMessage(index);
			if (!msg) {
				char	buf[200];
				sprintf(buf, "GetMouse - msg=%px, i=%ld, count=%ld, scount=%ld\n", 
					msg, index, count, ccc);
				PRINT((buf));
				SERIAL_PRINT((buf));
				DEBUGGER(buf);
			}
			if (msg->what == _UPDATE_) {
				foundUpdate = true;
				q->RemoveMessage(msg);
				delete (msg);
				count--;
			} else
				index++;
		};

		long		c = q->CountMessages();
		long		i = 0;
		int32		mouse_moved_count = 0;
		BMessage	*msg;
		BMessage	*lastMouseMoved = NULL;

		while (i < c) {
			msg = q->FindMessage(i);
			if (!msg) {
				break;
//				char buf[100];
//				sprintf(buf, "q=%x, i=%d, c=%d, real count=%d\n",
//					q, i, c, q->CountMessages());
//				DEBUGGER(buf);
			}
			if (msg->what == B_MOUSE_MOVED) {
				ASSERT(msg->HasInt32("buttons"));
				ASSERT(msg->HasPoint("be:view_where"));

				if (mouse_moved_count == 2) {
					ASSERT(lastMouseMoved);
					q->RemoveMessage(lastMouseMoved);
					delete lastMouseMoved;
					lastMouseMoved = NULL;
					c--;
				}
				else {
					mouse_moved_count++;
					i++;
				}
				lastMouseMoved = msg;
			} else {
				mouse_moved_count = 0;
				if ((msg->what==B_MOUSE_UP) || (msg->what==B_MOUSE_DOWN)) {
					mouse_moved_count = 0;
				
					ASSERT(msg->HasPoint("be:view_where"));
	
					BPoint	loc;
	
					// found an Up/Down event. So remove it and return this info.
					msg->FindPoint("be:view_where", &loc);
	
					BHandler	*handler;
					BView		*target;
					int32		token;
	
					Window()->find_token_and_handler(msg, &token, &handler);
	
					if (handler && (target = dynamic_cast<BView*>(handler)) != 0 &&
						target != this) {
	
							if (target->Window() != this->Window()) {
								// AHHH!!!! The view no longer belongs to this
								// window! abort
								goto bottom;
							}
	
							// The mouse location is in the coord space of some
							// other view.
							target->ConvertToScreen(&loc);
							ConvertFromScreen(&loc);
					}
	
					long bb;
					msg->FindInt32("buttons", &bb);
					*buttons = bb;
					*location = loc;
	
					q->RemoveMessage(msg);
					delete msg;
					q->Unlock();
					
					if (foundUpdate)
						Window()->do_draw_views();
					
					return;
				} else
					i++;
			}
			msg = NULL;
		}

		q->Unlock();
		
		if (foundUpdate)
			Window()->do_draw_views();

		// else don't have a MOUSE UP/DOWN event, so get latest from server
	}
	
bottom:
	if (server_token != owner->fLastViewToken) {
		owner->fLastViewToken = server_token;
		owner->a_session->swrite_l(GR_PICK_VIEW);
		owner->a_session->swrite(4,&server_token);
	};

	a_session = owner->a_session;
	a_session->swrite_l(GR_GET_VIEW_MOUSE);
	Flush();
	a_session->sread_coo(&location->x);
	a_session->sread_coo(&location->y);
	a_session->sread(4, buttons);
}

/*---------------------------------------------------------------*/

void	BView::MovePenTo(float x, float y)
{
	MovePenTo(BPoint(x, y));
}

/*---------------------------------------------------------------*/

void	BView::MovePenTo(BPoint pt)
{
	if ((fState->valid_flags & B_PEN_LOCATION_VALID) &&
		(fState->pen_loc.x == pt.x) &&
		(fState->pen_loc.y == pt.y))
		return;

	fState->pen_loc = pt;
	fState->valid_flags |= B_PEN_LOCATION_VALID;
	fState->new_local |= B_PEN_LOCATION_VALID;

	if (!owner) return;

	check_lock();
	AppSession	*a_session;
	a_session = owner->a_session;
	a_session->swrite_l(GR_MOVETO);
	a_session->swrite_coo_a(&pt.x);
	a_session->swrite_coo_a(&pt.y);
}

/*---------------------------------------------------------------*/

void	BView::MovePenBy(float x, float y)
{
	if (owner) {
		check_lock();
		AppSession	*a_session;
		fState->valid_flags &= ~B_PEN_LOCATION_VALID;
		a_session = owner->a_session;
		a_session->swrite_l(GR_MOVEBY);
		a_session->swrite_coo_a(&x);
		a_session->swrite_coo_a(&y);
	} else {
		fState->pen_loc = PenLocation() + BPoint(x, y);
		fState->valid_flags |= B_PEN_LOCATION_VALID;
	};
}

/*---------------------------------------------------------------*/

void	BView::SetPenSize(float size)
{	

	if ((fState->valid_flags & B_PEN_SIZE_VALID) &&
		(fState->pen_size == size))
		return;

	fState->pen_size = size;
	fState->valid_flags |= B_PEN_SIZE_VALID;
	fState->new_local |= B_PEN_SIZE_VALID;

	if (!owner) return;
	check_lock();

	AppSession	*a_session;
	a_session = owner->a_session;
	a_session->swrite_l(GR_SET_PEN_SIZE);
	a_session->swrite_coo_a(&size);
}

/*---------------------------------------------------------------*/

float BView::PenSize() const
{
	if (fState->valid_flags & B_PEN_SIZE_VALID) return fState->pen_size;
	if (owner) {
		check_lock();
		AppSession	*a_session;
		a_session = owner->a_session;
		a_session->swrite_l(GR_GET_PEN_SIZE);
		Flush();
		a_session->sread_coo_a(&fState->pen_size);
	} else {
		fState->pen_size = B_DEFAULT_PEN_SIZE;
	};
	fState->valid_flags |= B_PEN_SIZE_VALID;
	return fState->pen_size;
}

/*---------------------------------------------------------------*/

BPoint BView::PenLocation() const
{
	if (fState->valid_flags & B_PEN_LOCATION_VALID) return fState->pen_loc;
	if (owner) {
		check_lock();
		AppSession	*a_session;
		a_session = owner->a_session;
		a_session->swrite_l(GR_GET_PEN_LOC);
		Flush();
		a_session->sread_point_a(&fState->pen_loc);
	} else {
		fState->pen_loc = B_DEFAULT_PEN_LOCATION;
	};
	fState->valid_flags |= B_PEN_LOCATION_VALID;
	return fState->pen_loc;
}

/*---------------------------------------------------------------*/

rgb_color	BView::LowColor() const
{

	if (fState->valid_flags & B_LOW_COLOR_VALID) return fState->low_color;
	if (owner) {
		check_lock();
		AppSession	*a_session;
		a_session = owner->a_session;
		a_session->swrite_l(GR_GET_BACK_COLOR);
		Flush();
		a_session->sread(sizeof(rgb_color), &fState->low_color);
	} else {
		fState->low_color.red = 
		fState->low_color.blue = 
		fState->low_color.green = B_DEFAULT_LOW_COLOR;
		fState->low_color.alpha = 255;
	};
	fState->valid_flags |= B_LOW_COLOR_VALID;
	return fState->low_color ;
}

/*---------------------------------------------------------------*/

void	BView::SetHighColor(rgb_color a_color)
{
	// filter out references to the old transparent magic in case
	// someone hard-coded the values somewhere (yuck!)
	if ( (*((uint32 *)&a_color) == kOldTransparent32Bit) || 
		 (*((uint32 *)&a_color) == kSemiOldTransparent32Bit) )
		a_color = B_TRANSPARENT_32_BIT;         
	
	if ((fState->valid_flags & B_HIGH_COLOR_VALID) &&
		(*((uint32*)&fState->high_color) == *((uint32*)&a_color)))
		return;

	fState->high_color = a_color;
	fState->valid_flags |= B_HIGH_COLOR_VALID;
	fState->new_local |= B_HIGH_COLOR_VALID;
	if (!owner || (fStateLevel == 0 && !owner->InUpdate()))
		fState->high_ui_color = NULL;

	if (!owner) return;

	check_lock();
	AppSession	*a_session;
	a_session = owner->a_session;
	a_session->swrite_l(GR_FORE_COLOR);
	a_session->swrite(sizeof(rgb_color), &a_color);
}

/*---------------------------------------------------------------*/

rgb_color BView::HighColor() const
{

	if (fState->valid_flags & B_HIGH_COLOR_VALID) return fState->high_color;
	if (owner) {
		check_lock();
		AppSession	*a_session;
		a_session = owner->a_session;
		a_session->swrite_l(GR_GET_FORE_COLOR);
		Flush();
		a_session->sread(sizeof(rgb_color), &fState->high_color);
	} else {
		fState->high_color.red = 
		fState->high_color.blue = 
		fState->high_color.green = B_DEFAULT_HIGH_COLOR;
		fState->high_color.alpha = 255;
	}
	fState->valid_flags |= B_HIGH_COLOR_VALID;
	return fState->high_color;
}

/*---------------------------------------------------------------*/

void	BView::SetLowColor(rgb_color a_color)
{
	// filter out references to the old transparent magic in case
	// someone hard-coded the values somewhere (yuck!)
	if ( (*((uint32 *)&a_color) == kOldTransparent32Bit) || 
		 (*((uint32 *)&a_color) == kSemiOldTransparent32Bit) )
		a_color = B_TRANSPARENT_32_BIT;         
	
	if ((fState->valid_flags & B_LOW_COLOR_VALID) &&
		(*((uint32*)&fState->low_color) == *((uint32*)&a_color)))
		return;
	
	fState->low_color = a_color;
	fState->valid_flags |= B_LOW_COLOR_VALID;
	fState->new_local |= B_LOW_COLOR_VALID;
	if (!owner || (fStateLevel == 0 && !owner->InUpdate()))
		fState->low_ui_color = NULL;

	if (!owner) return;

	check_lock();
	AppSession	*a_session;
	a_session = owner->a_session;
	a_session->swrite_l(GR_BACK_COLOR);
	a_session->swrite(sizeof(rgb_color), &a_color);
}

/*---------------------------------------------------------------*/

void	BView::SetBlendingMode(source_alpha srcAlpha, alpha_function alphaFunc)
{
	short		tmpMode;

	if ((fState->valid_flags & B_BLENDING_MODE_VALID) &&
		(fState->srcAlpha == srcAlpha) &&
		(fState->alphaFunc == alphaFunc))
		return;

	fState->srcAlpha = srcAlpha;
	fState->alphaFunc = alphaFunc;
	fState->valid_flags |= B_BLENDING_MODE_VALID;
	fState->new_local |= B_BLENDING_MODE_VALID;
		
	if (!owner) return;

	check_lock();
	AppSession	*a_session;
	a_session = owner->a_session;
	a_session->swrite_l(GR_SET_BLENDING_MODE);
	tmpMode = srcAlpha;
	a_session->swrite(2, &tmpMode);
	tmpMode = alphaFunc;
	a_session->swrite(2, &tmpMode);
}

void	BView::GetBlendingMode(source_alpha *srcAlpha, alpha_function *alphaFunc) const
{
	if (fState->valid_flags & B_BLENDING_MODE_VALID) {
		*srcAlpha = fState->srcAlpha;
		*alphaFunc = fState->alphaFunc;
		return;
	};
	
	if (owner) {
		check_lock();
		AppSession	*a_session;
		a_session = owner->a_session;
		a_session->swrite_l(GR_GET_BLENDING_MODE);
		a_session->flush();
		a_session->sread(2, &fState->srcAlpha);
		a_session->sread(2, &fState->alphaFunc);
	} else {
		fState->srcAlpha = B_DEFAULT_SOURCE_ALPHA;
		fState->alphaFunc = B_DEFAULT_ALPHA_FUNCTION;
	};
	fState->valid_flags |= B_BLENDING_MODE_VALID;
};

/*---------------------------------------------------------------*/

void	BView::SetDrawingMode(drawing_mode mode)
{
	short		tmpMode;

	if ((fState->valid_flags & B_DRAW_MODE_VALID) &&
		(fState->draw_mode == mode))
		return;

	fState->draw_mode = mode;
	fState->valid_flags |= B_DRAW_MODE_VALID;
	fState->new_local |= B_DRAW_MODE_VALID;
		
	if (!owner) return;

	check_lock();
	AppSession	*a_session;
	a_session = owner->a_session;
	a_session->swrite_l(GR_SET_DRAW_MODE);
	tmpMode = mode;
	a_session->swrite(2, &tmpMode);
}

/*---------------------------------------------------------------*/

drawing_mode BView::DrawingMode() const
{

	if (fState->valid_flags & B_DRAW_MODE_VALID) return fState->draw_mode;
	if (owner) {
		check_lock();
		AppSession	*a_session;
		a_session = owner->a_session;
		a_session->swrite_l(GR_GET_DRAW_MODE);
		Flush();
		a_session->sread(2, &fState->draw_mode);
	} else {
		fState->draw_mode = B_DEFAULT_DRAW_MODE;
	};
	fState->valid_flags |= B_DRAW_MODE_VALID;
	return fState->draw_mode;
}

/*---------------------------------------------------------------*/

void BView::SetLineMode(	cap_mode lineCap,
							join_mode lineJoin,
							float miterLimit)
{
	short	modes[2];

	if ((fState->valid_flags & B_LINE_MODE_VALID) && 
		(fState->line_cap == lineCap) &&
		(fState->line_join == lineJoin) &&
		(fState->miter_limit == miterLimit))
		return;

	fState->line_cap = lineCap;
	fState->line_join = lineJoin;
	fState->miter_limit = miterLimit;
	fState->valid_flags |= B_LINE_MODE_VALID;
	fState->new_local |= B_LINE_MODE_VALID;
	
	if (!owner) return;

	check_lock();
	AppSession	*a_session;
	a_session = owner->a_session;
	modes[0] = lineCap;
	modes[1] = lineJoin;
	a_session->swrite_l(GR_SET_LINE_MODE);
	a_session->swrite(sizeof(short)*2, modes);
	a_session->swrite(sizeof(float), &miterLimit);
};

join_mode BView::LineJoinMode() const
{	
	if (fState->valid_flags & B_LINE_MODE_VALID) return fState->line_join;
	if (owner) {
		check_lock();
		AppSession	*a_session;
		a_session = owner->a_session;
		a_session->swrite_l(GR_GET_JOIN_MODE);
		a_session->swrite_l(GR_GET_CAP_MODE);
		a_session->swrite_l(GR_GET_MITER_LIMIT);
		Flush();
		a_session->sread(2,&fState->line_join);
		a_session->sread(2,&fState->line_cap);
		a_session->sread(4,&fState->miter_limit);
	} else {
		fState->line_join = B_DEFAULT_JOIN_MODE;
		fState->line_cap = B_DEFAULT_CAP_MODE;
		fState->miter_limit = B_DEFAULT_MITER_LIMIT;
	};
	fState->valid_flags |= B_LINE_MODE_VALID;
	return fState->line_join;
};

cap_mode BView::LineCapMode() const
{
	if (fState->valid_flags & B_LINE_MODE_VALID) return fState->line_cap;
	if (owner) {
		check_lock();
		AppSession	*a_session;
		a_session = owner->a_session;
		a_session->swrite_l(GR_GET_JOIN_MODE);
		a_session->swrite_l(GR_GET_CAP_MODE);
		a_session->swrite_l(GR_GET_MITER_LIMIT);
		Flush();
		a_session->sread(2,&fState->line_join);
		a_session->sread(2,&fState->line_cap);
		a_session->sread(4,&fState->miter_limit);
	} else {
		fState->line_join = B_DEFAULT_JOIN_MODE;
		fState->line_cap = B_DEFAULT_CAP_MODE;
		fState->miter_limit = B_DEFAULT_MITER_LIMIT;
	};
	fState->valid_flags |= B_LINE_MODE_VALID;
	return fState->line_cap;
};

float BView::LineMiterLimit() const
{
	if (fState->valid_flags & B_LINE_MODE_VALID) return fState->miter_limit;
	if (owner) {
		check_lock();
		AppSession	*a_session;
		a_session = owner->a_session;
		a_session->swrite_l(GR_GET_JOIN_MODE);
		a_session->swrite_l(GR_GET_CAP_MODE);
		a_session->swrite_l(GR_GET_MITER_LIMIT);
		Flush();
		a_session->sread(2,&fState->line_join);
		a_session->sread(2,&fState->line_cap);
		a_session->sread(4,&fState->miter_limit);
	} else {
		fState->line_join = B_DEFAULT_JOIN_MODE;
		fState->line_cap = B_DEFAULT_CAP_MODE;
		fState->miter_limit = B_DEFAULT_MITER_LIMIT;
	};
	fState->valid_flags |= B_LINE_MODE_VALID;
	return fState->miter_limit;
};

/*---------------------------------------------------------------*/

void	BView::StrokeLine(BPoint pt0, BPoint pt1, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("StrokeLine " << pt0 << "-" << pt1 << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_LINE);
	a_session->swrite(8,&pt0);
	a_session->swrite(8,&pt1);

	fState->pen_loc = pt1;
	fState->valid_flags |= B_PEN_LOCATION_VALID;
	fState->new_local |= B_PEN_LOCATION_VALID;
}

/*---------------------------------------------------------------*/

void	BView::StrokeLine(BPoint pt, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("StrokeLine (relative) " << fState->pen_loc << "-" << pt << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_LINETO);
	a_session->swrite(8,&pt);

	fState->pen_loc = pt;
	fState->valid_flags |= B_PEN_LOCATION_VALID;
	fState->new_local |= B_PEN_LOCATION_VALID;
}

/*---------------------------------------------------------------*/

void	BView::InvertRect(BRect r)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&B_SOLID_HIGH) != *((uint64*)&fState->pat))) {
		SetPattern(B_SOLID_HIGH);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("InvertRect " << r << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_RECT_INVERT);
	a_session->swrite_rect_a(&r);
}

/*---------------------------------------------------------------*/

void	BView::FillEllipse(BRect r, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("FillEllipse " << r << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_ELLIPSE_INSCRIBE_FILL);
	a_session->swrite(16,&r);
}

/*---------------------------------------------------------------*/

void	BView::FillEllipse(BPoint center, float xRadius, float yRadius, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("FillEllipse around " << center << ", xrad=" << xRadius
				<< ", yrad=" << yRadius << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_ELLIPSE_FILL);
	a_session->swrite(8,&center);
	a_session->swrite(4,&xRadius);
	a_session->swrite(4,&yRadius);
}

/*---------------------------------------------------------------*/

void	BView::StrokeEllipse(BRect r, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("StrokeEllipse " << r << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_ELLIPSE_INSCRIBE_STROKE);
	a_session->swrite(16,&r);
}

/*---------------------------------------------------------------*/

void	BView::StrokeEllipse(BPoint center, float xRadius, float yRadius, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("StrokeEllipse around " << center << ", xrad=" << xRadius
				<< ", yrad=" << yRadius << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_ELLIPSE_STROKE);
	a_session->swrite(8,&center);
	a_session->swrite(4,&xRadius);
	a_session->swrite(4,&yRadius);
}

/*---------------------------------------------------------------*/

void	BView::FillArc(BRect r, float start_angle, float arc_angle, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();
	
	PRINT_OP_IN(this);
	PRINT_OP("FillArc " << r << ", start=" << start_angle << ", end=" << arc_angle << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_ARC_INSCRIBE_FILL);
	a_session->swrite(16,&r);
	a_session->swrite(4,&start_angle);
	a_session->swrite(4,&arc_angle);
}

/*---------------------------------------------------------------*/

void	BView::FillArc(BPoint center, float xRadius, float yRadius, float start_angle, float arc_angle, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("FillArc around " << center << ", xrad=" << xRadius
				<< ", yrad=" << yRadius << ", start=" << start_angle
				<< ", end=" << arc_angle << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_ARC_FILL);
	a_session->swrite(8,&center);
	a_session->swrite(4,&xRadius);
	a_session->swrite(4,&yRadius);
	a_session->swrite(4,&start_angle);
	a_session->swrite(4,&arc_angle);
}

/*---------------------------------------------------------------*/

void	BView::StrokeArc(BRect r, float start_angle, float arc_angle, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("StrokeArc " << r << ", start=" << start_angle << ", end=" << arc_angle << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_ARC_INSCRIBE_STROKE);
	a_session->swrite(16,&r);
	a_session->swrite(4,&start_angle);
	a_session->swrite(4,&arc_angle);
}

/*---------------------------------------------------------------*/

void	BView::StrokeArc(BPoint center, float xRadius, float yRadius, float start_angle, float arc_angle, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("StrokeArc around " << center << ", xrad=" << xRadius
				<< ", yrad=" << yRadius << ", start=" << start_angle
				<< ", end=" << arc_angle << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_ARC_STROKE);
	a_session->swrite(8,&center);
	a_session->swrite(4,&xRadius);
	a_session->swrite(4,&yRadius);
	a_session->swrite(4,&start_angle);
	a_session->swrite(4,&arc_angle);
}

/*---------------------------------------------------------------*/

void	BView::StrokeRect(BRect r, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("StrokeArc " << r << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_RECTFRAME);
	a_session->swrite_rect_a(&r);
}

/*---------------------------------------------------------------*/

void	BView::FillRect(BRect r, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("FillRect " << r << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_RECTFILL);
	a_session->swrite_rect_a(&r);
}

/*---------------------------------------------------------------*/

void	BView::FillRegion(BRegion *a_region, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("FillRegion " << *a_region << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_FILL_REGION);
	a_session->swrite_region(a_region);
}

/*---------------------------------------------------------------*/

void	BView::StrokeRoundRect(BRect r, float xRadius, float yRadius, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("StrokeRoundRect " << r << ", xrad=" << xRadius << ", yrad=" << yRadius << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_ROUND_RECT_FRAME);
	a_session->swrite_rect_a(&r);
	a_session->swrite_coo_a(&xRadius);
	a_session->swrite_coo_a(&yRadius);
}

/*---------------------------------------------------------------*/

void	BView::FillRoundRect(BRect r, float xRadius, float yRadius, const pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("FillRoundRect " << r << ", xrad=" << xRadius << ", yrad=" << yRadius << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_ROUND_RECT_FILL);
	a_session->swrite_rect_a(&r);
	a_session->swrite_coo_a(&xRadius);
	a_session->swrite_coo_a(&yRadius);
}

/*---------------------------------------------------------------*/

char	*BView::test_area(int32 length)
{
	AppSession	*a_session;
	char		*tmp;

	if (!do_owner_check())
		return NULL;
		
	a_session = owner->a_session;
	a_session->swrite_l(GR_DATA);
	a_session->swrite_l(length);
	a_session->flush();
	a_session->sread(4, &tmp);

	return(tmp);
}

/*---------------------------------------------------------------*/

void	BView::DrawChar(char a_char, BPoint location)
{
	DrawString(&a_char, 1, location);
}

/*---------------------------------------------------------------*/

void	BView::DrawChar(char a_char)
{
	DrawString(&a_char, 1);
}

/*---------------------------------------------------------------*/

void	BView::DrawString(const char *a_string, BPoint loc, escapement_delta *delta)
{
	MovePenTo(loc);
	DrawString(a_string, delta);
}

/*---------------------------------------------------------------*/

void	BView::DrawString(const char *a_string, escapement_delta *delta)
{
	int32		real_length;
	short		string_length;
	AppSession	*a_session;
	uint		buffer[3];
	char        zero = 0;

	if (!owner) return;		
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("DrawString (no length) \"" << a_string << "\"" << endl);
	PRINT_OP_OUT();
	
	real_length = strlen(a_string);
	if (real_length == 0)
		return;
	if (real_length > 32766)
		real_length = 32766;
	string_length = real_length+1;

	a_session = owner->a_session;

	buffer[0] = GR_DRAW_STRING;
	if (delta == 0) {
		((float*)buffer)[1] = 0.0;
		((float*)buffer)[2] = 0.0;
	}
	else {
		((float*)buffer)[1] = delta->nonspace;
		((float*)buffer)[2] = delta->space;
	}
	a_session->swrite(12, buffer);
	a_session->swrite(2, &string_length);
	a_session->swrite(real_length, (void*)a_string);
	a_session->swrite(1, &zero);

	fState->valid_flags &= ~B_PEN_LOCATION_VALID;
}

/*---------------------------------------------------------------*/

void	BView::DrawString(const char *a_string, int32 length, BPoint loc,
						  escapement_delta *delta)
{
	MovePenTo(loc);
	DrawString(a_string, length, delta);
}

/*---------------------------------------------------------------*/

void	BView::DrawString(const char* a_string, int32 length, escapement_delta *delta)
// For strings that are not null terminated
{
	AppSession	*a_session;
	char		null_char = 0;
	long		buffer[3];
	
	PRINT_OP_IN(this);
	PRINT_OP("DrawString (length=" << length << ") \"" << a_string << "\"" << endl);
	PRINT_OP_OUT();
	
	if (!owner) return;
	if (length <= 0) return;
	if (length > 32766) length = 32766;
	length++;

	check_lock();

	a_session = owner->a_session;

	buffer[0] = GR_DRAW_STRING;
	if (delta == 0) {
		((float*)buffer)[1] = 0.0;
		((float*)buffer)[2] = 0.0;
	}
	else {
		((float*)buffer)[1] = delta->nonspace;
		((float*)buffer)[2] = delta->space;
	}

	a_session->swrite(12, buffer);
	
	short tempLength = length;
	a_session->swrite(2, &tempLength);
	a_session->swrite(length - 1, (void*)a_string);
	a_session->swrite(1, &null_char);

	fState->valid_flags &= ~B_PEN_LOCATION_VALID;
}

/*---------------------------------------------------------------*/

void BView::ForceFontFlags(uint32 flags)
{
	if (!owner) return;		
	check_lock();
	AppSession *a_session = owner->a_session;
	a_session->swrite_l(GR_SET_FONT_FLAGS);
	a_session->swrite(sizeof(flags), &flags);
};

void BView::ForceFontAliasing(bool enable)
{
	ForceFontFlags(enable ? B_DISABLE_ANTIALIASING : 0);
};

void BView::SetFont(
	const BFont	*font, 
	uint32		mask) 
{
	if (!mask) return;

	fState->font.SetTo(*font, mask);

	fState->f_mask |= mask;
	fState->f_nonDefault |= mask;

	/* do a lazy update with the app_server */
	fState->new_local |= B_FONT_VALID;
	set_font_state(&fState->font, mask);
}

/*---------------------------------------------------------------*/

void BView::set_font_state(
	const BFont	*font,
	uint32		mask) 
{
	AppSession	         *a_session;
	
	if (!owner) return;
	check_lock();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_SET_FONT_CONTEXT);
	IKAccess::WriteFont(font, a_session);
	a_session->swrite_l(mask);
}

uchar
BView::font_encoding() const
{
	BView *v = const_cast<BView*>(this);
	return v->fState->font.Encoding();
}

/*---------------------------------------------------------------*/

void BView::fetch_font()
{
	check_lock();
	AppSession *a_session = owner->a_session;
	a_session->swrite_l(GR_GET_FONT_CONTEXT);
	a_session->flush();

	BFont *font = &fState->font;
	
	IKAccess::ReadFont(font, a_session);
	fState->f_mask = B_FONT_ALL;
};

/*---------------------------------------------------------------*/

#if !_PR3_COMPATIBLE_
void BView::GetFont(BFont *font) const
#else
void BView::GetFont(BFont *font)
#endif
{
	if (((fState->f_mask & B_FONT_ALL) != B_FONT_ALL) && owner) {
		BView *v = const_cast<BView*>(this);
		v->fetch_font();
	};

	const BFont *srcFont = &fState->font;
	*font = *srcFont;
}

/*---------------------------------------------------------------*/

void
BView::TruncateString(
	BString*	in_out,
	uint32		mode,
	float		width) const
{
	BView *v = const_cast<BView*>(this);
	if (((fState->f_mask & B_FONT_ALL) != B_FONT_ALL) && owner) v->fetch_font();
	v->fState->font.TruncateString(in_out, mode, width);
}

/*---------------------------------------------------------------*/

float
BView::StringWidth(
	const char	*string) const 
{
	BView *v = const_cast<BView*>(this);

	if (((fState->f_mask & B_FONT_ALL) != B_FONT_ALL) && owner) v->fetch_font();

	return v->fState->font.StringWidth(string);
}

/*---------------------------------------------------------------*/

float
BView::StringWidth(
	const char	*string, 
	int32		length) const
{
	BView *v = const_cast<BView*>(this);

	if (((fState->f_mask & B_FONT_ALL) != B_FONT_ALL) && owner) v->fetch_font();

	return v->fState->font.StringWidth(string, length);
}

void
BView::GetStringWidths(
	char	*stringArray[],
	int32	lengthArray[],
	int32	numStrings,
	float	widthArray[]) const
{
	BView *v = const_cast<BView*>(this);
	if (((fState->f_mask & B_FONT_ALL) != B_FONT_ALL) && owner) v->fetch_font();
	v->fState->font.GetStringWidths(	(const char**)stringArray, lengthArray,
										numStrings, widthArray);
}

/*---------------------------------------------------------------*/

void BView::SetFontSize(
	float	size) 
{
	BFont aFont;
	aFont.SetSize(size);
	SetFont(&aFont, B_FONT_SIZE);
}

/*---------------------------------------------------------------*/

void BView::GetFontHeight(
	font_height	*height) const
{
	BView *v = const_cast<BView*>(this);
	if (((fState->f_mask & B_FONT_ALL) != B_FONT_ALL) && owner) v->fetch_font();
	v->fState->font.GetHeight(height);
}

/*---------------------------------------------------------------*/

void	BView::Invalidate()
{
	BRect	r;
	r = Bounds();
	Invalidate(r);
}

/*---------------------------------------------------------------*/

void	BView::DelayedInvalidate(bigtime_t delay, BRect invalRect)
{
	InvalidateAtTime(delay+system_time(), invalRect);
}

/*---------------------------------------------------------------*/

void	BView::InvalidateAtTime(bigtime_t time, BRect invalRect)
{
	if (time != B_INFINITE_TIMEOUT) {
		if (time > system_time()+1000) {
			BMessage msg(B_INVALIDATE);
			if (invalRect.IsValid()) msg.AddRect("be:area", invalRect);
			BMessenger(this).SendMessageAtTime(msg, time);
		} else if (invalRect.IsValid()) {
			Invalidate(invalRect);
		} else {
			Invalidate();
		}
	}
}

/*---------------------------------------------------------------*/

bool	BView::IsInvalidatePending() const
{
	if (!Window()) return false;
	
	BMessenger me(this);
	return Window()->MessageQueue()->FindMessage(B_INVALIDATE, me) != NULL;
}

/*---------------------------------------------------------------*/

void	BView::EndLineArray()
{
	AppSession	*a_session;

	if (!owner) return;
		
	if (!comm || !comm->current) {
		debugger("Can't call EndLineArray before BeginLineArray");
		return;
	}

//	_array_hdr_	*p = comm->current;
	
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("EndLineArray (" << comm->cur_p << " lines)" << endl);
	EXEC_OP(for (int32 i=0; i<comm->cur_p; i++)
				OPOUT << "  " << i << ": (" << ((_line_data_*)comm->current)[i].x0
						<< "," << ((_line_data_*)comm->current)[i].y0
						<< ")-(" << ((_line_data_*)comm->current)[i].x1
						<< "," << ((_line_data_*)comm->current)[i].y1 << ")" << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
//	p->count = comm->cur_p;
//	p->busy = 1;
	a_session->swrite_l(GR_START_VARRAY);
//	a_session->swrite_l((ulong)comm->current);
	a_session->swrite_l(comm->cur_p);
	a_session->swrite(sizeof(_line_data_)*comm->cur_p,comm->current);
	
	free(comm->current);
	comm->current = 0;
	comm->cur_size = 0;
}

/*---------------------------------------------------------------*/

void	BView::AddLine(BPoint pt0, BPoint pt1, rgb_color col)
{
	_line_data_	*p;

	if (!comm || !comm->current) {
		debugger("Can't call AddLine before BeginLineArray");
		return;
	}

//	p = (_line_data_ *)(((ulong)comm->current)+sizeof(_array_hdr_));
	p = (_line_data_*)comm->current;
	
	if (p) {
		if (comm->cur_p >= comm->cur_size)
			return;
	
		p += comm->cur_p;
		p->x0 = pt0.x;
		p->y0 = pt0.y;
		p->x1 = pt1.x;
		p->y1 = pt1.y;
		p->col= col;
		comm->cur_p++;
	}
}

/*---------------------------------------------------------------*/
#if 0
void	BView::remove_comm_array()
{
	_array_hdr_	*p;
	_array_hdr_	*p1;

	if (!comm)
		return;

	p = comm->list;
	while(p) {
		// is app_server is still using the data must wait for it.
		if (p->busy)
			Sync();
		p1 = p->next;
		sh_free((char *)p);
		p = p1;
	}
	comm->list = NULL;

	free(comm);
	comm = NULL;
}
#endif
void BView::remove_comm_array()
{
	if (comm) {
		free(comm);
		comm = NULL;
	};
}

/*---------------------------------------------------------------*/

_array_hdr_	*BView::new_comm_array(int32 /*cnt*/)
{
#if 0
	_array_hdr_	*p;

	ASSERT(comm);

	p = comm->list;

	while(p) {
		if (p->busy == 0 && p->size >= cnt) {
			p->busy = 1;
			return (p);
		}

		p = p->next;
	}

	if (cnt >= ((1 << 30) / sizeof(_line_data_)))
		// that's a lot of lines!!!
		return (NULL);

	p = (_array_hdr_ *)sh_malloc(sizeof(_array_hdr_) + sizeof(_line_data_)*cnt);
	if (p == NULL)
		// doh!
		return (NULL);

	p->size = cnt;
	p->count = 0;
	p->next = comm->list;
	p->busy = 1;
	comm->list = p;

	return (p);
#endif
	return NULL;
}

/*---------------------------------------------------------------*/

void	BView::BeginLineArray(int32 count)
{
	if (!owner) return;
	check_lock_no_pick();

	if (count < 0) {
		debugger("Calling BeginLineArray with a count < 0");
		return;
	}

	if (comm && comm->current) {
		debugger("Can't nest BeginLineArray calls");
		return;
	}

	if (!comm) {
		// first call to BeginLineArray allocs this block
		comm = (_array_data_ *) malloc(sizeof(_array_data_));
		if (comm == NULL)
			// doh!  fail silently...
			return;

		comm->list = 0;
		comm->current = 0;
	}

	comm->current = (_array_hdr_*)malloc(sizeof(_line_data_) * count);
	//new_comm_array(count);
	if (comm->current == NULL)
		// fail silently...
		return;

	comm->cur_size = count;
	comm->cur_p = 0;
}

/*---------------------------------------------------------------*/

void	BView::Invalidate(BRect r)
{
	if (!owner)
		return;
	check_lock();

	AppSession	*a_session = owner->a_session;
	a_session->swrite_l(GR_INVAL);
	a_session->swrite_rect(&r);

	Flush();
}

void	BView::Invalidate(const BRegion *r)
{
	if (!owner)
		return;
	check_lock();

	AppSession	*a_session = owner->a_session;
	a_session->swrite_l(GR_INVAL_REGION);
	a_session->swrite_region(r);

	Flush();
}

/*---------------------------------------------------------------*/

BPoint	BView::LeftTop() const
{
	return(BPoint(origin_h, origin_v));
}

/*---------------------------------------------------------------*/

BWindow	*BView::Window() const
{
	return(owner);
}

/*---------------------------------------------------------------*/

void	BView::ScrollBy(float dh, float dv)
{
	ScrollTo(BPoint(origin_h + dh, origin_v + dv));
}

/*---------------------------------------------------------------*/

void	BView::ScrollTo(BPoint where)
{
	if (!owner)
		return;
		
	check_lock();

	if (fHorScroller)
	{
		float min, max;
		fHorScroller->GetRange(&min, &max);
		if (where.x < min) 		where.x = min;
		else if (where.x > max)	where.x = max;
	}

	if (fVerScroller)
	{
		float min, max;
		fVerScroller->GetRange(&min, &max);
		if (where.y < min) 		where.y = min;
		else if (where.y > max)	where.y = max;
	}

	bool	dirty_x = (where.x != origin_h);
	bool	dirty_y = (where.y != origin_v);

	RealScrollTo(where);

	if (fVerScroller && dirty_y)
		fVerScroller->SetValue(where.y);

	if (fHorScroller && dirty_x)
		fHorScroller->SetValue(where.x);
}

/*---------------------------------------------------------------*/

void	BView::RealScrollTo(BPoint where)
{
	AppSession	*a_session;
	
	ASSERT(owner);

	a_session = owner->a_session;

	origin_h = where.x;
	origin_v = where.y;

	a_session->swrite_l(GR_VIEW_SET_ORIGIN);
	a_session->swrite_coo_a(&where.x);
	a_session->swrite_coo_a(&where.y);
}

/*---------------------------------------------------------------*/

void	BView::movesize(uint32 code, int32 h, int32 v)
{
	AppSession	*a_session;
	
	if (!owner) return;
	check_lock_no_pick();

	/*	We let the server_token be part of the protocol here
		because the common case will likely be many views being
		resized in a row, for instance for a client-side view
		layout lib. */

	a_session = owner->a_session;
	a_session->swrite_l(code);
	a_session->swrite_l(server_token);
	a_session->swrite_l(h);
	a_session->swrite_l(v);
} 

/*---------------------------------------------------------------*/

void	BView::MoveBy(float h, float v)
{
	if (owner)
		movesize(GR_MOVE_VIEW, h, v);
	else {
		fCachedBounds.OffsetBy(h,v);
	}
}

/*---------------------------------------------------------------*/

void	BView::MoveTo(float x, float y)
{
	MoveTo(BPoint(x, y));
}

/*---------------------------------------------------------------*/

void	BView::MoveTo(BPoint where)
{
	if (fShowLevel < 0)
		where.x += kHideValue;

	if (owner) {
		BView *parent = Parent();
		if (parent && parent->IsPrinting()) {
			where += parent->Bounds().LeftTop();
			movesize(GR_MOVETO_VIEW, where.x, where.y);
		} else {
			movesize(GR_SCREWED_UP_MOVETO_VIEW, where.x, where.y);
		};
	} else {
		fCachedBounds.OffsetTo(where);
	}
}

/*---------------------------------------------------------------*/

void	BView::ResizeBy(float h, float v)
{
	if (owner)
		movesize(GR_SIZE_VIEW, h, v);
	else {
		fCachedBounds.bottom += v;
		fCachedBounds.right += h;
	}
}

/*---------------------------------------------------------------*/

void	BView::ResizeTo(float w, float h)
{
	if (owner)
		movesize(GR_SIZETO_VIEW, w, h);
	else {
		fCachedBounds.bottom = fCachedBounds.top + h;
		fCachedBounds.right  = fCachedBounds.left + w;
	}
}

/*---------------------------------------------------------------*/

void	BView::Show()
{
	if (fShowLevel == -1)
		MoveBy(-kHideValue, 0);

	fShowLevel++;
}

/*---------------------------------------------------------------*/

void	BView::Hide()
{
	if (fShowLevel-- == kIsShown) {
		MoveBy(kHideValue, 0);
		// Do I or one of my children have focus?
		BView* f = owner ? owner->CurrentFocus() : NULL;
		while (f) {
			if (f == this) {
				// You have focus.  And now you don't.
				owner->navigate_to_next(B_FORWARD, false);
				f = NULL;
			} else {
				f = f->Parent();
			}
		}
	}
}

/*---------------------------------------------------------------*/

bool	BView::IsHidden(const BView* looking_from) const
{
	bool self = (fShowLevel < kIsShown);
	
	// NOTE: It was decided that views that don't belong to a window
	// can still be visible.

	// If the view itself is hidden then immediately return true
	if (self)
		return true;

	// If this is the highest level we are looking, then by that
	// measure we aren't hidden.
	if (looking_from == this)
		return false;
		
	// otherwise, if there's a parent view, returns its state. 
	if (RealParent())
		return RealParent()->IsHidden(this);
	// otherwise, if there's a window, returns its state. 
	else if (Window() && looking_from == 0)
		return Window()->IsHidden();
	// otherwise, the view must be visible.
	else
		return false;
}

bool	BView::IsHidden() const
{
	return IsHidden(0);
}

/*---------------------------------------------------------------*/

static BRect make_visible_in_parent(BView* view, BRect frame)
{
	BView* parent = view->Parent();
	if (!parent) return frame;
	
	BScrollBar* hbar = parent->ScrollBar(B_HORIZONTAL);
	if (hbar && hbar->IsHidden()) hbar = 0;
	BScrollBar* vbar = parent->ScrollBar(B_VERTICAL);
	if (vbar && vbar->IsHidden()) vbar = 0;
	
	BRect bounds = parent->Bounds();
	frame.OffsetBy(bounds.LeftTop());
	
//+	printf("Making visible view %s: ", view->Name()); frame.PrintToStream();
//+	printf("Making visible in   %s: ", parent->Name()); bounds.PrintToStream();
	
	bool changed = false;
	
	if (hbar) {
		float extra = floor( (bounds.Width()-frame.Width()) / 4 );
		if (extra > 4) extra = 4;
		if (extra < 0) extra = 0;
		if (frame.left > bounds.left && frame.right > bounds.right) {
			changed = true;
			float amount = frame.right-bounds.right+extra;
			if (bounds.left+amount > frame.left) amount = frame.left-bounds.left;
			bounds.OffsetBy(amount, 0);
		} if (frame.left < bounds.left && frame.right < bounds.right) {
			changed = true;
			float amount = frame.left-bounds.left-extra;
			if (bounds.right+amount < frame.right) amount = frame.right-bounds.right;
			bounds.OffsetBy(amount, 0);
		}
	}
	
	if (vbar) {
		float extra = floor( (bounds.Height()-frame.Height()) / 4 );
		if (extra > 4) extra = 4;
		if (extra < 0) extra = 0;
		if (frame.top > bounds.top && frame.bottom > bounds.bottom) {
			changed = true;
			float amount = frame.bottom-bounds.bottom+extra;
			if (bounds.top+amount > frame.top) amount = frame.top-bounds.top;
			bounds.OffsetBy(0, amount);
		} else if (frame.top < bounds.top && frame.bottom < bounds.bottom) {
			changed = true;
			float amount = frame.top-bounds.top-extra;
			if (bounds.bottom+amount < frame.bottom) amount = frame.bottom-bounds.bottom;
			bounds.OffsetBy(0, amount);
		}
	}
	
	if (changed) {
//+		printf("Scroll to: "); bounds.PrintToStream();
		if (hbar) hbar->SetValue(bounds.left);
		if (vbar) vbar->SetValue(bounds.top);
//+		printf("Final position: "); parent->Bounds().PrintToStream();
	}
	
	// Translate this view's frame into its parent view's
	// coordinates, removing any parts that are clipped by
	// the parent.
	bounds = parent->Bounds();
	frame = frame & bounds;
	frame.OffsetBy(-bounds.left, -bounds.top);
//+	printf("New visible view    %s: ", view->Name()); frame.PrintToStream();
	bounds = parent->Frame();
	frame.OffsetBy(bounds.left, bounds.top);
//+	printf("View in parent      %s: ", parent->Name()); frame.PrintToStream();
	
	return frame;
}

void	BView::MakeFocus(bool focusState)
{
	BView	*old_focus;

	if (owner) {
		check_lock_no_pick();

		old_focus = owner->CurrentFocus();		// get the current focus

		if (focusState) {
			if (old_focus && (old_focus != this)) {
				old_focus->fNoISInteraction = true;
				old_focus->MakeFocus(false);	// unfocus the current focus
				old_focus->fExplicitFocus = false;
				old_focus->fNoISInteraction = false;
			}
			if (old_focus != this && Parent()) {
				// make sure this view is visible in its parents,
				// scrolling if necessary; it would really be best
				// if this were moved out to a virtual function
//+				printf("Scrolling to make focus %s visible:\n", Name());
				BView* view = this;
				BRect frame = view->Frame();
				while (view && !fFocusNoScroll) {
					frame = make_visible_in_parent(view, frame);
					view = view->Parent();
				}
				fPulseDir = 0;
				fPulseValue = 255;
			}
			owner->set_focus(this, !fNoISInteraction);
		} else if (this == old_focus) {					// unfocusing the current focus
			owner->set_focus(NULL, !fNoISInteraction);	// nothing focused in this case
			fExplicitFocus = false;
		}
	}
}

/*---------------------------------------------------------------*/

void	BView::MakeFocusNoScroll(bool focusState)
{
	fFocusNoScroll = true;
	MakeFocus(focusState);
	fFocusNoScroll = false;
}

/*---------------------------------------------------------------*/

bool	BView::IsFocus() const
{
	if (owner) {
		// need this check_lock for things even as simple as this...
		// The window might get destroyed at ANY time leading to
		// undefined behavior.
		check_lock_no_pick();
		return(owner->CurrentFocus() == this);
	} else
		return(false);
}

/*---------------------------------------------------------------*/

bool	BView::IsNavigating() const
{
	if (owner) {
		// need this check_lock for things even as simple as this...
		// The window might get destroyed at ANY time leading to
		// undefined behavior.
		check_lock_no_pick();
		return(owner->fIsNavigating);
	} else
		return(false);
}

/*---------------------------------------------------------------*/

void	BView::SetExplicitFocus(bool explicitState)
{
	if (!explicitState || IsFocus())
		fExplicitFocus = explicitState;
}

/*---------------------------------------------------------------*/

bool	BView::IsExplicitFocus() const
{
	return fExplicitFocus;
}

/*---------------------------------------------------------------*/

rgb_color	BView::NextFocusColor() const
{
	return mix_color(ui_color(B_NAVIGATION_PULSE_COLOR),
					 ui_color(B_NAVIGATION_BASE_COLOR), fPulseValue);
}

/*---------------------------------------------------------------*/

bigtime_t	BView::NextFocusTime() const
{
	if (ui_color(B_NAVIGATION_PULSE_COLOR) == ui_color(B_NAVIGATION_BASE_COLOR))
		return B_INFINITE_TIMEOUT;
	
	if (fPulseDir) {
		int16 v = ((int16)fPulseValue) + 24;
		if (v >= 255) {
			v = 255;
			fPulseDir = false;
		}
		fPulseValue = (uint8)v;
	} else {
		int16 v = ((int16)fPulseValue) - 24;
		if (v <= 0) {
			v = 0;
			fPulseDir = true;
		}
		fPulseValue = (uint8)v;
	}
	return system_time()+30000;
}

/*---------------------------------------------------------------*/

void	BView::KeyUp(const char *, int32)
{
}

/*---------------------------------------------------------------*/

void	BView::KeyDown(const char *, int32)
{
	Window()->kb_navigate();
}

/*---------------------------------------------------------------*/
/*
void    BView::StringDown(const char *string, int32 length)
{
	const BFont *srcFont = (fState != NULL) ? &fState->font : 
											  be_plain_font;

	int32 i;
	
	if (srcFont->Encoding() == B_UNICODE_UTF8) {
		// filter only ASCII 7 bits
		for (i=0; i<length; i++)
			if ((((uchar*)string)[i] & 0x80) == 0)
				KeyDown((ulong)(((uchar*)string)[i]));
	}
	else {
		// everything go through
		for (i=0; i<length; i++)
			KeyDown((ulong)(((uchar*)string)[i]));
	}
}
*/
/*---------------------------------------------------------------*/

void	BView::Pulse()
{
}

/*---------------------------------------------------------------*/

void	BView::handle_tick()
{
	BView	*tmp;
	
	tmp = first_child;

	while (tmp != 0) {

		if (tmp->f_type & B_PULSE_NEEDED)
			tmp->Pulse();

		if (tmp->first_child)
			tmp->handle_tick();

		tmp = tmp->next_sibling;
	}
}

/*---------------------------------------------------------------*/

void	BView::CopyBits(BRect src, BRect dst)
{
	AppSession	*a_session;
	long		bid;

	if (!owner) return;
	check_lock();

	a_session = owner->a_session;

	PRINT_OP_IN(this);
	PRINT_OP("CopyBits from " << src << " to " << dst << endl);
	PRINT_OP_OUT();
	
	a_session->swrite_l(GR_MOVE_BITS);
	a_session->swrite_rect_a(&src);
	a_session->swrite_rect_a(&dst);
	Flush();
	a_session->sread(4, &bid);
}

/*---------------------------------------------------------------*/

void	BView::DrawBitmapAsync(const BBitmap *an_offscreen, BRect src, BRect dst)
{
	AppSession	*a_session;

	if (!owner) return;
	check_lock();

	a_session = owner->a_session;

	PRINT_OP_IN(this);
	PRINT_OP("DrawBitmapAsync " << an_offscreen << " from " << src << " to " << dst << endl);
	PRINT_OP_OUT();
	
	a_session->swrite_l(GR_SCALE_BITMAP1_A);
	a_session->swrite_rect_a(&src);
	a_session->swrite_rect_a(&dst);
	a_session->swrite_l(IKAccess::BitmapToken(an_offscreen));
}

/*---------------------------------------------------------------*/

void	BView::DrawBitmap(const BBitmap *an_offscreen, BRect src, BRect dst)
{
	AppSession	*a_session;
	long		bid;

	if (!owner) return;
	check_lock();

	a_session = owner->a_session;

	PRINT_OP_IN(this);
	PRINT_OP("DrawBitmap " << an_offscreen << " from " << src << " to " << dst << endl);
	PRINT_OP_OUT();
	
	a_session->swrite_l(GR_SCALE_BITMAP1);
	a_session->swrite_rect_a(&src);
	a_session->swrite_rect_a(&dst);
	a_session->swrite_l(IKAccess::BitmapToken(an_offscreen));

	/*	Sync for drawbitmap since their might be a race condition
		between the destruction message that has to go thru the
		app_port and the current draw message. */
	
	Flush();
	a_session->sread(4, &bid);
}

/*---------------------------------------------------------------*/

void	BView::DrawBitmapAsync(const BBitmap *an_offscreen, BRect r)
{
	AppSession	*a_session;

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("DrawBitmapAsync " << an_offscreen << " to " << r << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;

	a_session->swrite_l(GR_SCALE_BITMAP_A);
	a_session->swrite_rect_a(&r);
	a_session->swrite_l(IKAccess::BitmapToken(an_offscreen));
}

/*---------------------------------------------------------------*/

void	BView::DrawBitmap(const BBitmap *an_offscreen, BRect r)
{
	AppSession	*a_session;
	long		bid;

	if (!owner) return;
	check_lock();

	a_session = owner->a_session;

	PRINT_OP_IN(this);
	PRINT_OP("DrawBitmap " << an_offscreen << " to " << r << endl);
	PRINT_OP_OUT();
	
	a_session->swrite_l(GR_SCALE_BITMAP);
	a_session->swrite_rect_a(&r);
	a_session->swrite_l(IKAccess::BitmapToken(an_offscreen));

	/*	Sync for drawbitmap since their might be a race condition
		between the destruction message that has to go thru the
		app_port and the current draw message. */
	
	Flush();
	a_session->sread(4, &bid);
}

/*---------------------------------------------------------------*/

void	BView::DrawBitmap(const BBitmap *an_offscreen)
{
	DrawBitmap(an_offscreen, PenLocation());
}

/*---------------------------------------------------------------*/

void	BView::DrawBitmapAsync(const BBitmap *an_offscreen)
{
	DrawBitmapAsync(an_offscreen, PenLocation());
}

/*---------------------------------------------------------------*/

void	BView::DrawBitmapAsync(const BBitmap *an_offscreen, BPoint where)
{
	AppSession	*a_session;

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("DrawBitmapAsync " << an_offscreen << " at " << where << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_DRAW_BITMAP_A);
	a_session->swrite_coo_a(&where.x);
	a_session->swrite_coo_a(&where.y);
	a_session->swrite_l(IKAccess::BitmapToken(an_offscreen));
}

/*---------------------------------------------------------------*/

void	BView::DrawBitmap(const BBitmap *an_offscreen, BPoint where)
{
	AppSession	*a_session;
	long		bid;

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("DrawBitmap " << an_offscreen << " at " << where << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_DRAW_BITMAP);
	a_session->swrite_coo_a(&where.x);
	a_session->swrite_coo_a(&where.y);
	a_session->swrite_l(IKAccess::BitmapToken(an_offscreen));

	/*	Sync for drawbitmap since their might be a race condition
		between the destruction message that has to go thru the
		app_port and the current draw message. */

	Flush();
	a_session->sread(4, &bid);
}

/*---------------------------------------------------------------*/

void	BView::do_activate(int32 msg)
{
	BView	*tmp;
	
	WindowActivated(msg ? true : false);

	tmp = first_child;

	while (tmp) {

#ifdef DIM_BACKGROUND_WINDOWS	
		// Hack added by Alan
		if (owner && owner->fFeel != window_feel(1025)
			&& owner->fFeel != window_feel(1024))
			tmp->Invalidate();

#endif		
		tmp->do_activate(msg ? true : false);
		tmp = tmp->next_sibling;
	}
}

/*---------------------------------------------------------------*/

void	BView::WindowActivated(bool)
{
}

/*---------------------------------------------------------------*/

void	BView::FrameMoved(BPoint)
{
}

/*---------------------------------------------------------------*/

void	BView::FrameResized(float, float)
{
}

/*---------------------------------------------------------------*/

void BView::StrokePolygon(const BPoint *pt_list, int32 num_pts,
	bool closed, const pattern pat)
{
	AppSession	*a_session;
	short		tempNum,tempClosed;	

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	if (num_pts < 3) return;
	check_lock();
		
	PRINT_OP_IN(this);
	PRINT_OP("StrokePolygon of " << num_pts << " points" << endl);
	PRINT_OP_OUT();
	
	tempClosed = closed;
	tempNum = num_pts;

	a_session = owner->a_session;
	a_session->swrite_l(GR_POLYFRAME);
	a_session->swrite(2, &tempClosed);
	a_session->swrite(2, &tempNum);
	a_session->swrite(num_pts * sizeof(BPoint), (void *) pt_list);
}

/*---------------------------------------------------------------*/

void BView::StrokePolygon(const BPoint *pt_list, int32 num_pts, BRect,
	bool closed, const pattern pat)
{
	StrokePolygon(pt_list, num_pts, closed, pat);
}

/*---------------------------------------------------------------*/

void	BView::StrokePolygon(const BPolygon *a_poly, bool closed, const pattern pat)
{
	StrokePolygon(a_poly->fPts, a_poly->fCount, closed, pat);
}

/*---------------------------------------------------------------*/

void BView::FillPolygon(const BPoint *pt_list, int32 num_pts, const pattern pat)
{
	AppSession	*a_session;
	short		tempNum;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	if (num_pts < 3) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("FillPolygon of " << num_pts << " points" << endl);
	PRINT_OP_OUT();
	
	if (num_pts > 32767) tempNum = 32767;
	else tempNum = num_pts;

	a_session = owner->a_session;
	a_session->swrite_l(GR_POLYFILL);
	a_session->swrite(2, &tempNum);
	a_session->swrite(num_pts * sizeof(BPoint), (void *) pt_list);
}

/*---------------------------------------------------------------*/

void BView::FillPolygon(const BPoint *pt_list, int32 num_pts, BRect,
	const pattern pat)
{
	FillPolygon(pt_list, num_pts, pat);
}

/*---------------------------------------------------------------*/

void	BView::FillPolygon(const BPolygon *a_poly, const pattern pat)
{
	FillPolygon(a_poly->fPts, a_poly->fCount, pat);
}

/*---------------------------------------------------------------*/

#if 0
void	compute_poly_bounds(long num_pts, const BPoint* pt_list, BRect* r)
{
	float 	top;
	float	left;
	float	bottom;
	float	right;
	
	if (num_pts) {
			
		top = bottom = pt_list[0].y;
		left = right = pt_list[0].x;
		
		for (short i = 1; i < num_pts; ++i) {
			float	v, h;
			
			v = pt_list[i].y;
			h = pt_list[i].x;
			
			if (v < top)
				top = v;
			else if (v > bottom)
				bottom = v;
			
			if (h < left)
				left = h;
			else if (h > right)
				right = h;
		}

		r->Set(left, top, right, bottom);
	}
}
#endif

/*---------------------------------------------------------------*/

void BView::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
	const pattern pat)
{
	BPoint	ptList[3];
	
	ptList[0] = pt1;
	ptList[1] = pt2;
	ptList[2] = pt3;
	StrokePolygon(ptList, 3, true, pat);
}

/*---------------------------------------------------------------*/

void BView::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3, BRect,
	const pattern pat)
{
	BPoint	ptList[3];
	
	ptList[0] = pt1;
	ptList[1] = pt2;
	ptList[2] = pt3;
	StrokePolygon(ptList, 3, true, pat);
}

/*---------------------------------------------------------------*/

void BView::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3, const pattern pat)
{
	BPoint	ptList[3];
	
	ptList[0] = pt1;
	ptList[1] = pt2;
	ptList[2] = pt3;
	FillPolygon(ptList, 3, pat);
}

/*---------------------------------------------------------------*/

void BView::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3, BRect,
	const pattern pat)
{
	BPoint	ptList[3];
	
	ptList[0] = pt1;
	ptList[1] = pt2;
	ptList[2] = pt3;
	FillPolygon(ptList, 3, pat);
}

/*---------------------------------------------------------------*/

void BView::DoBezier(int32 gr, BPoint *controlPoints, pattern pat)
{
	AppSession	*a_session;

	if (!(fState->valid_flags & B_PATTERN_VALID) ||
		(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
		SetPattern(pat);
	}

	if (!owner) return;
	check_lock();
	
	PRINT_OP_IN(this);
	PRINT_OP((gr == GR_DRAW_BEZIER ? "StrokeBezier " : "FillBezier ")
				<< controlPoints[0] << " " << controlPoints[1] << " "
				<< controlPoints[2] << " " << controlPoints[3] << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(gr);
	a_session->swrite(sizeof(BPoint)*4,controlPoints);
}

void BView::StrokeBezier(BPoint *controlPoints, pattern p)
{
	DoBezier(GR_DRAW_BEZIER,controlPoints,p);
}

void BView::FillBezier(BPoint *controlPoints, pattern p)
{
	DoBezier(GR_FILL_BEZIER,controlPoints,p);
}

/*---------------------------------------------------------------*/

void BView::DoShape(int32 gr, BShape *shape, pattern pat)
{
	AppSession	*a_session;
	int32 opCount,ptCount;
	uint32 *opList;
	BPoint *ptList;

	IKAccess::GetShapeData(shape,&opCount,&ptCount,&opList,&ptList);
	const uint32 buildingOp = IKAccess::ShapeBuildingOp(shape);
	
	if (gr != GR_CLIP_TO_SHAPE) {
		if (!(fState->valid_flags & B_PATTERN_VALID) ||
			(*((uint64*)&pat) != *((uint64*)&fState->pat))) {
			SetPattern(pat);
		}
		if (!owner) return;
		check_lock();
		if ((!opCount && !buildingOp) || !ptCount) return;
	} else {
		if ((!opCount && !buildingOp) || !ptCount) {
			BRegion reg;
			reg.Set(BRect(1,1,0,0));
			ConstrainClippingRegion(&reg);
			return;
		};
	};
	
	PRINT_OP_IN(this);
	PRINT_OP((gr == GR_STROKE_SHAPE ? "StrokeShape " : "FillShape ") << *shape << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(gr);
	a_session->swrite_l(opCount+1);
	a_session->swrite_l(ptCount);
	a_session->swrite(sizeof(uint32)*opCount,opList);
	if (buildingOp & 0x30000000)
		a_session->swrite_l(buildingOp);
	else
		a_session->swrite_l(buildingOp | 0x30000000);
	a_session->swrite(sizeof(BPoint)*ptCount,ptList);
}

void BView::StrokeShape(BShape *shape, pattern p)
{
	DoShape(GR_STROKE_SHAPE,shape,p);
};

void BView::FillShape(BShape *shape, pattern p)
{
	DoShape(GR_FILL_SHAPE,shape,p);
};

/*---------------------------------------------------------------*/

BView	*BView::FindView(const char *name) const
{
	BView		*result;
	BView		*tmp;
	const char	*view_name;
	
	check_lock_no_pick();

	if (name == NULL)
		return NULL;

	if ((view_name = Name()) != 0) {
		if (strcmp(view_name, name) == 0)
			return((BView *) this);	// need the cast to make it non-const
	}
	
	tmp = first_child;

	while (tmp) {
		result = tmp->FindView(name);
		if (result)
			return(result);

		tmp = tmp->next_sibling;
	}
	return(NULL);
}

/*---------------------------------------------------------------*/

void	BView::GetClippingRegion(BRegion* a_region) const
{
	AppSession	*a_session;
	short		length;		
	clipping_rect		*data;
	clipping_rect 		tempRect;
	long		i;

	if (!do_owner_check()) return;
		
	if (f_is_printing) {
		a_region->Set(((_printing_state_ *)(pr_state))->clipping);
		return;
	}

	a_session = owner->a_session;

	a_session->swrite_l(GR_GET_CLIP);
	Flush();

	a_session->sread_rect(&tempRect);

	a_session->sread(2, &length);

	IRegion* reg = IKAccess::RegionData(a_region);
	data = reg->CreateRects(length, length);
	reg->SetRectCount(length);
	reg->Bounds() = tempRect;
	
	for (i = 0; i < length; i++) {
		a_session->sread_rect(data+i);
	}
	
	PRINT_OP_IN(this);
	PRINT_OP("GetClippingRegion " << *a_region << endl);
	PRINT_OP_OUT();
}

/*---------------------------------------------------------------*/

void    BView::ConstrainClippingRegion(BRegion* a_region)
{
	AppSession 	*a_session;

	if (!do_owner_check()) return;

	PRINT_OP_IN(this);
	PRINT_OP("ConstrainClippingRegion ");
	EXEC_OP(if (a_region) OPOUT << *a_region; else OPOUT << "(null)");
	PRINT_OP(endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;

	if (a_region == 0) {						//remove clipping case.
		a_session->swrite_l(GR_NO_CLIP);
	} else {
		a_session->swrite_l(GR_SET_CLIP);
		a_session->swrite_region(a_region);
	}
}

/*---------------------------------------------------------------*/

void	BView::DragMessage( BMessage *dragMessage, BBitmap *image,
	drawing_mode dragMode, BPoint offset, BHandler *reply_to)
{
	if (owner && image) {
		if (!reply_to)
			reply_to = this;
		else {
			// make sure the Handler has a looper
			if (!reply_to->Looper())
				debugger("warning - the Handler needs a looper");
		}

		check_lock_no_pick();

		bool gotButtons = dragMessage->HasInt32("buttons");
		if (!gotButtons) {
			BMessage *cur = Window()->CurrentMessage();
			if (cur) {
				// tack on the current pressed button so that the drag recipient
				// can tell which button was down at the beginning of the drag
				int32 buttons;
				if (cur->FindInt32("buttons", &buttons) == B_OK) {
					dragMessage->AddInt32("buttons", buttons);
					gotButtons = true;
				}
			}
		}
		if (!gotButtons) {
			BPoint tmp;
			uint32 buttons;
			GetMouse(&tmp, &buttons, false);
			dragMessage->AddInt32("buttons", buttons);
		}
		
		owner->start_drag(dragMessage, server_token, offset,
			IKAccess::BitmapToken(image), dragMode, reply_to);

		delete image;
	}
}

/*---------------------------------------------------------------*/

void	BView::DragMessage(BMessage *dragMessage, BBitmap *image, BPoint offset,
	BHandler *reply_to)
{
	DragMessage(dragMessage, image, B_OP_COPY, offset, reply_to);
}

/*---------------------------------------------------------------*/

void	BView::DragMessage(BMessage* aMessage, BRect dragRect,
	BHandler *reply_to)
{
	if (owner) {
		check_lock_no_pick();
		
		BPoint 		offset 	= B_ORIGIN;
		BMessage	*cur 	= Window()->CurrentMessage();
		
		if (cur) {
			BPoint where;
			if (cur->FindPoint("be:view_where", &where) == B_OK)
				offset = where - dragRect.LeftTop();

			// tack on the current pressed button so that the drag recipient
			// can tell which button was down at the beginning of the drag
			int32 buttons;
			if (cur->FindInt32("buttons", &buttons) == B_OK)
				aMessage->AddInt32("buttons", buttons);
		}

		if (!reply_to)
			reply_to = this;
		else {
			// make sure the Handler has a looper
			if (!reply_to->Looper())
				debugger("warning - the Handler needs a looper");
		}

		owner->start_drag(aMessage, server_token, offset, dragRect, reply_to);
	}
}

/*---------------------------------------------------------------*/

void	BView::Flush() const
{
	if (!Window()) return;
	Window()->Flush();
}

/*---------------------------------------------------------------*/

void	BView::Sync() const
{
	AppSession	*a_session;
	
	if (!do_owner_check()) return;
		
	a_session = owner->a_session;
	a_session->full_sync();
}

/*---------------------------------------------------------------*/

void
BView::GetPreferredSize(
	float	*width,
	float	*height)
{
	BRect bounds = Bounds();

	*width = bounds.Width();
	*height = bounds.Height();
}

/*---------------------------------------------------------------*/

void
BView::ResizeToPreferred()
{
	BRect	bounds = Bounds();
	float	width = bounds.Width();
	float	height = bounds.Height();
	float	newWidth = width;
	float	newHeight = height;

	GetPreferredSize(&newWidth, &newHeight);

	if ((newWidth != width) || (newHeight != height))
		ResizeTo(newWidth, newHeight);
}

/*---------------------------------------------------------------*/

void	BView::BeginPicture_pr(BPicture */*a_picture*/, BRect /*r*/)
{
/*
	AppSession	*a_session;

	if (!do_owner_check())
		return;
	
	cpicture = a_picture;
	a_session = owner->a_session;

	a_session->swrite_l(GR_START_PICTURE);
	a_session->swrite_l(server_token);
	a_session->swrite_rect_a(&r);
	a_session->flush();
*/
}

/*---------------------------------------------------------------*/

void	BView::SetDiskMode(char *filename, long offset)
{
	AppSession	*a_session;

	if (!do_owner_check()) return;
	
	PRINT_OP_IN(this);
	PRINT_OP("SetDiskMode" << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_START_DISK_PICTURE);
	a_session->swrite_l(offset);
	a_session->swrite_l(strlen(filename) + 1);
	a_session->swrite(strlen(filename) + 1, filename);
}

/*---------------------------------------------------------------*/

void	BView::BeginPicture(BPicture *pic)
{
	AppSession	*a_session;

	if (!do_owner_check()) return;
	if (pic == NULL) return;
	
	PRINT_OP_IN(this);
	PRINT_OP("BeginPicture " << pic << endl);
	PRINT_OP_OUT();
	
	if (!IKAccess::UsurpPicture(pic, cpicture)) {
		PRINT_OP_IN(this);
		PRINT_OP("Can't usurp picture!" << endl);
		PRINT_OP_OUT();
		return;
	}
	
	cpicture = pic;
	a_session = owner->a_session;
	
	a_session->swrite_l(GR_START_PICTURE);
}

/*---------------------------------------------------------------*/

void	BView::AppendToPicture(BPicture *pic)
{
	AppSession	*a_session;
	int32		picToken;

	if (!do_owner_check()) return;
	if (pic == NULL) return;
	if (IKAccess::PictureToken(pic) == NO_TOKEN) {
		BeginPicture(pic);
		return;
	}
	
	PRINT_OP_IN(this);
	PRINT_OP("AppendToPicture " << pic << endl);
	PRINT_OP_OUT();
	
	picToken = IKAccess::PictureToken(pic);
	if (!IKAccess::UsurpPicture(pic, cpicture, true)) return;
	cpicture = pic;
	a_session = owner->a_session;

	a_session->swrite_l(GR_RESTART_PICTURE);
	a_session->swrite_l(picToken);
}

/*---------------------------------------------------------------*/

BPicture *BView::EndPicture()
{
	BPicture *		pic;
	AppSession *	a_session;
	int32			picture_token;	

	if (!do_owner_check()) return NULL;
	if (cpicture == NULL) return NULL;
			
	PRINT_OP_IN(this);
	PRINT_OP("EndPicture" << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_END_PICTURE);
	a_session->flush();
	a_session->sread(4, &picture_token);

	pic = cpicture;
	cpicture = IKAccess::StepDownPicture(pic);
	IKAccess::SetPictureToken(pic, picture_token);
	return(pic);
}

/*---------------------------------------------------------------*/

void	BView::DrawPicture(const BPicture *a_pict)
{
	DrawPictureAsync(a_pict, PenLocation());
	owner->a_session->full_sync();
};

void	BView::DrawPictureAsync(const BPicture *a_pict)
{
	DrawPictureAsync(a_pict, PenLocation());
}

/*---------------------------------------------------------------*/

void	BView::DrawPicture(const char *filename, long offset, BPoint where)
{
	DrawPictureAsync(filename, offset, where);
	owner->a_session->full_sync();
}

/*---------------------------------------------------------------*/

void	BView::DrawPictureAsync(const char *filename, long offset, BPoint where)
{
	AppSession	*a_session;

	if (!do_owner_check()) return;
	
	PRINT_OP_IN(this);
	PRINT_OP("DrawPicture " << filename << " at " << where << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_PLAY_DISK_PICTURE);
	a_session->swrite_l(offset);
	a_session->swrite_l(strlen(filename) + 1);
	a_session->swrite(strlen(filename) + 1, const_cast<char*>(filename));
	a_session->swrite_coo_a(&where.x);
	a_session->swrite_coo_a(&where.y);
}

/*---------------------------------------------------------------*/

void	BView::DrawPicture(const BPicture *a_pict, BPoint where)
{
	DrawPictureAsync(a_pict,where);
	owner->a_session->full_sync();
}

void	BView::DrawPictureAsync(const BPicture *a_pict, BPoint where)
{
	AppSession	*a_session;

	if (!do_owner_check()) return;

	const int32 token = IKAccess::PictureToken(a_pict);
	if (token == NO_TOKEN) return;

	PRINT_OP_IN(this);
	PRINT_OP("DrawPicture " << a_pict << " at " << where << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_PLAY_PICTURE);
	a_session->swrite_l(token);
	a_session->swrite_coo_a(&where.x);
	a_session->swrite_coo_a(&where.y);
}

/*---------------------------------------------------------------*/

void BView::DoPictureClip(BPicture *a_pict, BPoint where, bool invert, bool sync)
{
	AppSession	*a_session;

	if (!do_owner_check()) return;

	const int32 token = IKAccess::PictureToken(a_pict);
	if (token == NO_TOKEN) return;

	PRINT_OP_IN(this);
	PRINT_OP((invert ? "ClipToInversePicture " : "ClipToPicture ") << a_pict << " at " << where << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_CLIP_TO_SHAPE);
	a_session->swrite_l(token);
	a_session->swrite(sizeof(BPoint),&where);
	a_session->swrite_l(invert);

	if (sync) owner->a_session->full_sync();
};

void BView::ClipToPicture(BPicture *picture, BPoint where, bool sync)
{
	DoPictureClip(picture,where,false,sync);
};

void BView::ClipToInversePicture(BPicture *picture, BPoint where, bool sync)
{
	DoPictureClip(picture,where,true,sync);
};

/*---------------------------------------------------------------*/

void	BView::SetPattern(pattern pat)
{
	AppSession	*a_session;

	/*	This is a private call, so make the assumption we know
		what we're doing and don't do any rejection. */
	fState->pat = pat;
	fState->valid_flags |= B_PATTERN_VALID;
	fState->new_local |= B_PATTERN_VALID;
	
	if (!owner) return;
	check_lock();

	a_session = owner->a_session;
	a_session->swrite_l(GR_SET_PATTERN);
	a_session->swrite(8,&pat);
};

/*---------------------------------------------------------------*/

void BView::SetTransform(const BTransform2d& transform)
{
	if (!do_owner_check()) return;

	if ((fState->valid_flags & B_TRANSFORM_VALID) &&
		(fState->transform == transform))
		return;

	fState->transform = transform;
	fState->valid_flags |= B_TRANSFORM_VALID;
	fState->new_local |= B_TRANSFORM_VALID;

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("SetTransform " << transform << endl);
	PRINT_OP_OUT();
	
	#if 0
	AppSession	*a_session;
	a_session = owner->a_session;
	a_session->swrite_l(GR_SET_TRANSFORM);
	a_session->swrite(sizeof(BTransform2d),&transform);
	#endif
};

void BView::GetTransform(BTransform2d* into) const
{
	if (fState->valid_flags & B_TRANSFORM_VALID) {
		*into = fState->transform;
		return;
	}
	if (owner) {
		check_lock();
		#if 0
		AppSession	*a_session;
		a_session = owner->a_session;
		a_session->swrite_l(GR_GET_TRANSFORM);
		Flush();
		a_session->sread(sizeof(BTransform2d),&fState->transform);
		#endif
		fState->transform = B_DEFAULT_TRANSFORM;
	} else {
		fState->transform = B_DEFAULT_TRANSFORM;
	};
	fState->valid_flags |= B_TRANSFORM_VALID;
	*into = fState->transform;
};

/*---------------------------------------------------------------*/

void BView::SetOrigin(BPoint pt)
{
	if (!do_owner_check()) return;

	if ((fState->valid_flags & B_ORIGIN_VALID) &&
		(fState->origin == pt))
		return;

	fState->origin = pt;
	fState->valid_flags |= B_ORIGIN_VALID;
	fState->new_local |= B_ORIGIN_VALID;

	if (!owner) return;
	check_lock();

	PRINT_OP_IN(this);
	PRINT_OP("SetOrigin " << pt << endl);
	PRINT_OP_OUT();
	
	AppSession	*a_session;
	a_session = owner->a_session;
	a_session->swrite_l(GR_SET_ORIGIN);
	a_session->swrite(sizeof(BPoint),&pt);
};

void BView::SetOrigin(float x, float y)
{
	SetOrigin(BPoint(x,y));
};

BPoint BView::Origin() const
{
	if (fState->valid_flags & B_ORIGIN_VALID) return fState->origin;
	if (owner) {
		check_lock();
		AppSession	*a_session;
		a_session = owner->a_session;
		a_session->swrite_l(GR_GET_ORIGIN);
		Flush();
		a_session->sread(sizeof(BPoint),&fState->origin);
	} else {
		fState->origin = B_DEFAULT_ORIGIN;
	};
	fState->valid_flags |= B_ORIGIN_VALID;
	return fState->origin;
};

/*---------------------------------------------------------------*/

void BView::PushState()
{
	AppSession	*a_session;

	if (!do_owner_check()) return;

	PRINT_OP_IN(this);
	PRINT_OP("PushState" << endl);
	PRINT_OP_OUT();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_PUSH_STATE);
	
	fState->valid_flags |= B_ORIGIN_VALID|B_TRANSFORM_VALID;
	fState->origin = B_DEFAULT_ORIGIN;
	fState->transform = B_DEFAULT_TRANSFORM;
	
	fStateLevel++;
};

void BView::PopState()
{
	AppSession	*a_session;

	if (!do_owner_check()) return;

	PRINT_OP_IN(this);
	PRINT_OP("PopState" << endl);
	PRINT_OP_OUT();
	
	if (fStateLevel > 0) {
		fStateLevel--;
		
		a_session = owner->a_session;
		a_session->swrite_l(GR_POP_STATE);
	
		fState->valid_flags = 0;
		fState->f_mask = 0;
		
	} else {
		printf("PopState() called more times than PushState!()\n");
	}
};

/*---------------------------------------------------------------*/

const char*	BView::ViewUIColor() const
{
	check_lock_no_pick();

	return fState->view_ui_color;
}

/*---------------------------------------------------------------*/

void BView::SetViewUIColor(const char* name)
{
	check_lock_no_pick();

	if (name) SetViewColor(ui_color(name));
	fState->view_ui_color = name;
}

/*---------------------------------------------------------------*/

const char*	BView::LowUIColor() const
{
	check_lock_no_pick();

	return fState->low_ui_color;
}

/*---------------------------------------------------------------*/

void BView::SetLowUIColor(const char* name)
{
	check_lock_no_pick();

	if (name) SetLowColor(ui_color(name));
	if (!owner || (fStateLevel == 0 && !owner->InUpdate()))
		fState->low_ui_color = name;
}

/*---------------------------------------------------------------*/

const char*	BView::HighUIColor() const
{
	check_lock_no_pick();

	return fState->high_ui_color;
}

/*---------------------------------------------------------------*/

void BView::SetHighUIColor(const char* name)
{
	check_lock_no_pick();

	if (name) SetHighColor(ui_color(name));
	if (!owner || (fStateLevel == 0 && !owner->InUpdate()))
		fState->high_ui_color = name;
}

/*---------------------------------------------------------------*/

void BView::SetColorsFromParent()
{
	// inherit view color from parent
	if (Parent()) {
		const char* col = Parent()->ViewUIColor();
		if (!col) col = Parent()->LowUIColor();
		if (col) {
			SetViewUIColor(col);
			SetLowUIColor(col);
		} else {
			rgb_color c = Parent()->ViewColor();
	
			if (c == B_TRANSPARENT_COLOR) c = Parent()->LowColor();
	
			SetViewColor(c);
			SetLowColor(c);
		}
	}
}

/*---------------------------------------------------------------*/

rgb_color	BView::ViewColor() const
{
	check_lock_no_pick();

	rgb_color color;
	if (fState->b_view_color)
		color = fState->view_color;
	else {
		color.red = color.green = color.blue = B_DEFAULT_VIEW_COLOR;
		color.alpha = 255;
	}
	return color;
}

/*---------------------------------------------------------------*/

void BView::SetViewColor(rgb_color c)
{
	// filter out references to the old transparent magic in case
	// someone hard-coded the values somewhere (yuck!)
	if ( (*((uint32 *)&c) == kOldTransparent32Bit) || 
		 (*((uint32 *)&c) == kSemiOldTransparent32Bit) )
		c = B_TRANSPARENT_32_BIT;         

#if _R5_COMPATIBLE_
	if ((dynamic_cast<BButton *>(this)) && (c == B_TRANSPARENT_32_BIT)) {
		// Ugly hack to fix applications that use liblayout (which is broken).
		return;
	}
#endif

	AppSession	*a_session;

	fState->view_color = c;
	fState->b_view_color = true;
	fState->view_ui_color = NULL;
	
	if (!owner) return;
	check_lock();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_SET_VIEW_COLOR);
	a_session->swrite(sizeof(rgb_color), &c);
}

/*---------------------------------------------------------------*/

void BView::SetViewCursor(const BCursor *cursor, bool sync)
{
	if (!owner) return;
	check_lock();
	
	AppSession *a_session = owner->a_session;
	a_session->swrite_l(GR_SET_VIEW_CURSOR);
	a_session->swrite_l(cursor->m_serverToken);

	if (sync) a_session->full_sync();
}

/*---------------------------------------------------------------*/

status_t BView::SetViewImage(	const BBitmap *bitmap,
								BRect srcRect, BRect dstRect,
								uint32 followFlags, uint32 options)
{
	int32 result;
	AppSession	*a_session;
	struct {	uint32 opCode;
				int32 token;
				BRect srcRect, dstRect;
				uint32 followFlags, options; } protocol;

	if (!do_owner_check()) return B_ERROR;

	protocol.opCode = GR_SET_VIEW_BITMAP;
	protocol.token = bitmap?IKAccess::BitmapToken(bitmap):NO_TOKEN;
	protocol.srcRect = srcRect;
	protocol.dstRect = dstRect;
	protocol.followFlags = followFlags;
	protocol.options = options;

	a_session = owner->a_session;
	a_session->swrite(sizeof(protocol),&protocol);
	a_session->flush();
	a_session->sread(4,&result);
	return result;
};

void BView::SetViewBitmap(	const BBitmap *bitmap,
							BRect srcRect, BRect dstRect,
							uint32 followFlags, uint32 options)
{
	SetViewImage(bitmap,srcRect,dstRect,followFlags,options);
};

void BView::SetViewBitmap(	const BBitmap *bitmap,
							uint32 followFlags, uint32 options)
{
	BRect bounds = bitmap?bitmap->Bounds():BRect();
	bounds.OffsetTo(0,0);
	SetViewImage(bitmap,bounds,bounds,followFlags,options);
};

void BView::ClearViewBitmap()
{
	BRect r;
	SetViewBitmap(NULL,r,r,B_FOLLOW_NONE,0);
};

/*---------------------------------------------------------------*/

status_t BView::SetViewOverlay(	const BBitmap *bitmap,
								BRect srcRect, BRect dstRect,
								rgb_color *colorKey,
								uint32 followFlags, uint32 options)
{
	status_t err = SetViewImage(bitmap,srcRect,dstRect,followFlags,options | AS_OVERLAY_BITMAP);
	owner->a_session->sread(sizeof(rgb_color),colorKey);
	return err;
};

status_t BView::SetViewOverlay(	const BBitmap *bitmap,
								rgb_color *colorKey,
								uint32 followFlags, uint32 options)
{
	BRect bounds = bitmap?bitmap->Bounds():BRect();
	bounds.OffsetTo(0,0);
	return SetViewOverlay(bitmap,bounds,bounds,colorKey,followFlags,options);
};

void BView::ClearViewOverlay()
{
	ClearViewBitmap();
};

/*-------------------------------------------------------------*/

#if _SUPPORTS_FEATURE_SCRIPTING
enum {
	PI_BASIC = 1,
	PI_VIEW_INDEX,
	PI_SHELF
};

static	property_info	prop_list[] = {
	{"Frame",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, PI_BASIC,
		{ B_RECT_TYPE },
		{},
		{}
	},
	{"Hidden",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, PI_BASIC,
		{ B_BOOL_TYPE },
		{},
		{}
	},
	{"ToolTipText",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, PI_BASIC,
		{ B_STRING_TYPE },
		{},
		{}
	},
	{"View",
		{B_COUNT_PROPERTIES},
		{B_DIRECT_SPECIFIER},
		NULL, PI_BASIC,
		{ B_INT32_TYPE },
		{},
		{}
	},
	{"View",
		{},			// allows any command
		{B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, B_NAME_SPECIFIER},
		NULL, PI_VIEW_INDEX,
		{},
		{},
		{}
	},
	{NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};


// If a view has a shelf then it will also support the "Shelf" keyword
static property_info	prop_list2[] = {
	{"Shelf",
		{},			// allows any command
		{B_DIRECT_SPECIFIER},
		NULL, PI_SHELF,
		{},
		{},
		{}
	},
	{NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};
#endif

/*-------------------------------------------------------------*/

BHandler *BView::ResolveSpecifier(BMessage *_SCRIPTING_ONLY(msg), int32 _SCRIPTING_ONLY(index),
	BMessage *_SCRIPTING_ONLY(spec), int32 _SCRIPTING_ONLY(form),
	const char *_SCRIPTING_ONLY(prop))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t	err = B_OK;
	BMessage	error_msg(B_MESSAGE_NOT_UNDERSTOOD);
	BHandler	*target = NULL;

//+	PRINT(("BView::Resolve: msg->what=%.4s, index=%d, form=0x%x, prop=%s\n",
//+		(char*) &(msg->what), index, spec->what, prop));

	BPropertyInfo	pi(prop_list);
	int32			i;
	uint32			user_data;
	
	if (fShelf) {
		BPropertyInfo	pi2(prop_list2);
		i = pi2.FindMatch(msg, index, spec, form, prop);
		user_data = i >= 0 ? prop_list2[i].extra_data : 0;
		if (user_data == PI_SHELF) {
			msg->PopSpecifier();
//+			PRINT(("sending to BShelf\n"));
			target = fShelf;
			goto done;
		}
	}

	i = pi.FindMatch(msg, index, spec, form, prop);
	user_data = i >= 0 ? prop_list[i].extra_data : 0;
	if ((index == 0) && (user_data == PI_BASIC)) {
		target = this;
	} else if (user_data == PI_VIEW_INDEX) {
		switch (form) {
			case B_REVERSE_INDEX_SPECIFIER:
			case B_INDEX_SPECIFIER: {
				/*
				 By-index implies non-recursive. Find the 4th child view of
				 the view. Don't descend to subviews.
				*/
				int	index = spec->FindInt32("index");
				if (form == B_REVERSE_INDEX_SPECIFIER)
					index = CountChildren() - index;
				BView *v =ChildAt(index);
				if (v) {
					msg->PopSpecifier();
					target = v;
				} else {
					err = B_BAD_INDEX;
					error_msg.AddString("message", "view index out of range");
				}
				break;
			}
			case B_NAME_SPECIFIER: {
				const char *name = spec->FindString(B_PROPERTY_NAME_ENTRY);
//+				PRINT(("	by name: (%s)\n", name ? name : "null"));
				if (!name) {
					err = B_BAD_SCRIPT_SYNTAX;
					break;
				}
				target = FindView(name);
				if (target) {
//+					PRINT(("passing along to view %s (%s)\n",
//+						target->Name(), class_name(target)));
					msg->PopSpecifier();
				} else {
					err = B_NAME_NOT_FOUND;
					error_msg.AddString("message", "view name not found");
				}
				break;
			}
			default:
				ASSERT(0);
		}
	}

done:

	if (err) {
		error_msg.AddInt32("error", err);
		msg->SendReply(&error_msg);
		target = NULL;
	} else if (!target) {
		target = BHandler::ResolveSpecifier(msg, index, spec, form, prop);
	}

	return target;
#else
	return NULL;
#endif
}

/*-------------------------------------------------------------*/

void BView::MessageReceived(BMessage *msg)
{
//+	PRINT(("what = %.4s\n", (char *) &(msg->what)));
	bool 		respond = false;
	bool		handled = false;
	BMessage	reply(B_REPLY);

#if _SUPPORTS_FEATURE_SCRIPTING
	status_t	err;
	BMessage	specifier;
	int32		form;
	const char	*prop;
	int32		cur;

	switch (msg->what) {
		case B_COUNT_PROPERTIES: {
			err = msg->GetCurrentSpecifier(&cur, &specifier, &form, &prop);
			if (!err && strcmp(prop, "View") == 0) {
				reply.AddInt32("result", CountChildren());
				respond = true;
			}
			break;
		}
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		{
			err = msg->GetCurrentSpecifier(&cur, &specifier, &form, &prop);
			if (!err && (strcmp(prop, "Frame") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddRect("result", Frame());
					respond = true;
				} else {
					BRect rect;
					err = msg->FindRect("data", &rect);
					if (!err) {
						MoveTo(rect.LeftTop());
						ResizeTo(rect.Width(), rect.Height());
						reply.AddInt32("error", B_OK);
						respond = true;
					}
				}
			} else if (!err && (strcmp(prop, "Hidden") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddBool("result", IsHidden());
					respond = true;
				} else {
					bool	hide;
					err = msg->FindBool("data", &hide);
					if (!err) {
						if (hide) {
							if (!IsHidden())
								Hide();
						} else {
							if (IsHidden())
								Show();
						}
						reply.AddInt32("error", B_OK);
						respond = true;
					}
				}
			} else if (!err && (strcmp(prop, "ToolTipText") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddString("result", ToolTipText());
					respond = true;
				} else {
					const char* text;
					err = msg->FindString("data", &text);
					if (!err) {
						SetToolTipText(text);
						reply.AddInt32("error", B_OK);
						respond = true;
					}
				}
			}
			break;
		}

		case B_MOUSE_WHEEL_CHANGED:
		{
			float xDelta, yDelta;

			BScrollBar *horizontalScrollBar = ScrollBar(B_HORIZONTAL);
			BScrollBar *verticalScrollBar = ScrollBar(B_VERTICAL);
			
			if (!horizontalScrollBar
				|| msg->FindFloat("be:wheel_delta_x", &xDelta) != B_OK)
				xDelta = 0;

			if (!verticalScrollBar
				|| msg->FindFloat("be:wheel_delta_y", &yDelta) != B_OK)
				yDelta = 0;
			
			if (xDelta || yDelta) {
				float xSmallStep = 50, xLargeStep = 500;
				float ySmallStep = 50, yLargeStep = 500;
				
				if (horizontalScrollBar) 
					horizontalScrollBar->GetSteps(&xSmallStep, &xLargeStep);

				if (verticalScrollBar) 
					verticalScrollBar->GetSteps(&ySmallStep, &yLargeStep);

				if (modifiers() & B_OPTION_KEY) {
					// turbo charge
					xDelta *= xLargeStep;
					yDelta *= yLargeStep;
				} else {
					// going a little more than by the small step
					// increment works better for text view
					xDelta *= xSmallStep * 3;
					yDelta *= ySmallStep * 3;
				}

				
				if (horizontalScrollBar) 
					horizontalScrollBar->SetValue(horizontalScrollBar->Value() + xDelta);
				if (verticalScrollBar) 
					verticalScrollBar->SetValue(verticalScrollBar->Value() + yDelta);
					
				handled = true;
			}
			break;
		}
	}
#endif

	if (!respond && !handled)
		BHandler::MessageReceived(msg);
	if (respond)
		msg->SendReply(&reply);
}

/*-------------------------------------------------------------*/

status_t	BView::GetSupportedSuites(BMessage *_SCRIPTING_ONLY(data))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	data->AddString("suites", "suite/vnd.Be-view");
	BPropertyInfo	pi(prop_list);
	data->AddFlat("messages", &pi);

	if (fShelf) {
		data->AddString("suites", "suite/vnd.Be-container-view");
		BPropertyInfo	pi(prop_list2);
		data->AddFlat("messages", &pi);
	}

	return BHandler::GetSupportedSuites(data);
#else
	return B_UNSUPPORTED;
#endif
}

/*-------------------------------------------------------------*/

void BView::UnsetScroller(BScrollBar *sb)
{
	if (sb == fHorScroller)
		fHorScroller = NULL;
	else
		fVerScroller = NULL;
}

/*-------------------------------------------------------------*/

void BView::SetScroller(BScrollBar *sb)
{
	if (sb->Orientation() == B_HORIZONTAL)
		fHorScroller = sb;
	else
		fVerScroller = sb;
}

/*-------------------------------------------------------------*/

BScrollBar *BView::ScrollBar(orientation posture) const
{
	if (posture == B_HORIZONTAL)
		return(fHorScroller);
	else
		return(fVerScroller);
}

/*-------------------------------------------------------------*/

bool	BView::IsPrinting() const
{
	return (f_is_printing);
}

/*---------------------------------------------------------------*/

void BView::set_cached_state()
{
	// go through all the attributes, one by one, and set them
	// if necessary

	// explicitly calling 'BView' versions of these methods so that
	// any overrides won't get called. These overrides were already called
	// once when the attribute was set the first time. Don't want to call
	// it a second time. Want this implementation to be private.

	if (fState->new_local & B_FONT_VALID) {
		fState->valid_flags &= ~B_FONT_VALID;
		set_font_state(&fState->font, B_FONT_ALL);
	};

	if (fState->new_local & B_PATTERN_VALID) {
		fState->valid_flags &= ~B_PATTERN_VALID;
		BView::SetPattern(fState->pat);
	};

	if (fState->new_local & B_DRAW_MODE_VALID) {
		fState->valid_flags &= ~B_DRAW_MODE_VALID;
		BView::SetDrawingMode(fState->draw_mode);
	};

	if (fState->new_local & B_BLENDING_MODE_VALID) {
		fState->valid_flags &= ~B_BLENDING_MODE_VALID;
		BView::SetBlendingMode(fState->srcAlpha,fState->alphaFunc);
	};

	if (fState->new_local & B_TRANSFORM_VALID) {
		fState->valid_flags &= ~B_TRANSFORM_VALID;
		BView::SetTransform(fState->transform);
	};
	if (fState->new_local & B_ORIGIN_VALID) {
		fState->valid_flags &= ~B_ORIGIN_VALID;
		BView::SetOrigin(fState->origin);
	};

	if (fState->new_local & B_LINE_MODE_VALID) {
		fState->valid_flags &= ~B_LINE_MODE_VALID;
		BView::SetLineMode(fState->line_cap,fState->line_join,fState->miter_limit);
	};

	if (fState->new_local & B_PEN_SIZE_VALID) {
		fState->valid_flags &= ~B_PEN_SIZE_VALID;
		BView::SetPenSize(fState->pen_size);
	};

	if (fState->new_local & B_PEN_LOCATION_VALID) {
		fState->valid_flags &= ~B_PEN_LOCATION_VALID;
		BView::MovePenTo(fState->pen_loc);
	};
	
	if (fState->new_local & B_LOW_COLOR_VALID) {
		fState->valid_flags &= ~B_LOW_COLOR_VALID;
		const char* name = fState->low_ui_color;
		BView::SetLowColor(fState->low_color);
		fState->low_ui_color = name;
	};

	if (fState->new_local & B_HIGH_COLOR_VALID) {
		fState->valid_flags &= ~B_HIGH_COLOR_VALID;
		const char* name = fState->high_ui_color;
		BView::SetHighColor(fState->high_color);
		fState->high_ui_color = name;
	};

	if (fEventMask) {
		uint32 mask = fEventMask;
		uint32 options = fEventOptions;
		fEventMask = fEventOptions = 0;
		BView::SetEventMask(mask,options);
	};

	if (m_doubleBuffering) {
		uint32 db = m_doubleBuffering;
		m_doubleBuffering = 0;
		BView::SetDoubleBuffering(db);
	};

	if (fState->b_view_color) {
		const char* name = fState->view_ui_color;
		BView::SetViewColor(fState->view_color);
		fState->view_ui_color = name;
	}
}

/*---------------------------------------------------------------*/

_view_attr_::_view_attr_()
{
	set_default_values();
}

/*---------------------------------------------------------------*/

void _view_attr_::set_default_values()
{
	//b_font = false;
	//b_font_size = false;
	//b_font_shear = false;
	//b_font_rotation = false;
	//b_symbol_set = false;
	//b_finfo_valid = false;
	//strcpy(fname, B_DEFAULT_FONT_NAME);
	//strcpy(finfo.name, B_DEFAULT_FONT_NAME);
	//fsize = finfo.size = B_DEFAULT_FONT_SIZE;
	//fshear = finfo.shear = B_DEFAULT_FONT_SHEAR;
	//frotation = finfo.rotation = B_DEFAULT_FONT_ROTATION;
	//strcpy(symbol_set, B_DEFAULT_SYMBOL_SET);

	font = *be_plain_font;
	f_mask = B_FONT_ALL;
	f_nonDefault = 0;

	origin = B_DEFAULT_ORIGIN;
	transform = B_DEFAULT_TRANSFORM;
	line_cap = B_DEFAULT_CAP_MODE;
	line_join = B_DEFAULT_JOIN_MODE;
	miter_limit = B_DEFAULT_MITER_LIMIT;
	draw_mode = B_DEFAULT_DRAW_MODE;
	pen_size = B_DEFAULT_PEN_SIZE;
	pen_loc = B_DEFAULT_PEN_LOCATION;
	srcAlpha = B_DEFAULT_SOURCE_ALPHA;
	alphaFunc = B_DEFAULT_ALPHA_FUNCTION;
	*((uint64*)&pat) = *((uint64*)&B_DEFAULT_PATTERN);
	view_color.red =
	view_color.green =
	view_color.blue = B_DEFAULT_VIEW_COLOR;
	view_color.alpha = 255;
	high_color.red =
	high_color.green =
	high_color.blue = B_DEFAULT_HIGH_COLOR;
	high_color.alpha = 255;
	low_color.red =
	low_color.green =
	low_color.blue = B_DEFAULT_LOW_COLOR;
	low_color.alpha = 255;

	view_ui_color = low_ui_color = high_ui_color = NULL;
	
	new_local = 0;
	valid_flags = 0xFFFFFFFF;
}

/*---------------------------------------------------------------*/

void BView::SetScale(float scale) const
{
	AppSession	*a_session;
	
	check_lock();
	
	a_session = owner->a_session;
	a_session->swrite_l(GR_SET_SCALE);
	a_session->swrite_coo_a(&scale);
	Flush();
}

/*---------------------------------------------------------------*/

void BView::TargetedByScrollView(BScrollView *)
{
}

/*---------------------------------------------------------------*/

void BView::set_shelf(BShelf *s)
{
	fShelf = s;
	if (owner)
		owner->AddHandler(fShelf);
}

/*---------------------------------------------------------------*/

BShelf *BView::shelf() const
{
	return fShelf;
}

/*---------------------------------------------------------------*/

uint32	BView::Flags() const
{
	return(f_type & ~(_RESIZE_MASK_));
}

/*---------------------------------------------------------------*/

uint32	BView::ResizingMode() const
{
	return(f_type & (_RESIZE_MASK_));
}

/*---------------------------------------------------------------*/

status_t BView::Perform(perform_code d, void *arg)
{
	return BHandler::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

BView::BView(const BView &) 
	:	BHandler()
	{}
BView &BView::operator=(const BView &) { return *this; }

/* ---------------------------------------------------------------- */

#include <fbc.h>

#if _PR2_COMPATIBLE_

extern "C" void _ReservedView1__5BViewFv(BView *const THIS, BRect update);

extern "C" void _ReservedView1__5BViewFv(BView *const THIS, BRect update)
{
	// explicit call to the new function! Don't make another virtual call
	// or we'll potentially end up calling client code again.
	THIS->BView::DrawAfterChildren(update);
}

#endif

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT status_t
	#if __GNUC__
	_ReservedView2__5BView
	#elif __MWERKS__
	_ReservedView2__5BViewFv
	#endif
	(BView* This, BPoint where, BRect* outRegion, BToolTipInfo* outInfo)
	{
		return This->BView::GetToolTipInfo(where, outRegion, outInfo);
	}

	_EXPORT status_t
	#if __GNUC__
	_ReservedView3__5BView
	#elif __MWERKS__
	_ReservedView3__5BViewFv
	#endif
	(BView* This, const BMessage* changes, uint32 flags)
	{
		return This->BView::UISettingsChanged(changes, flags);
	}
}
#endif

void 
BView::_ReservedView4()
{
}

void 
BView::_ReservedView5()
{
}

void 
BView::_ReservedView6()
{
}

void 
BView::_ReservedView7()
{
}

void 
BView::_ReservedView8()
{
}

#if !_PR3_COMPATIBLE_

void 
BView::_ReservedView9()
{
}

void 
BView::_ReservedView10()
{
}

void 
BView::_ReservedView11()
{
}

void 
BView::_ReservedView12()
{
}

void 
BView::_ReservedView13()
{
}

void 
BView::_ReservedView14()
{
}

void 
BView::_ReservedView15()
{
}

void 
BView::_ReservedView16()
{
}

#endif

/*---------- Deprecated 11/1999 (Maui) --------- */

#if __GNUC__ || __MWERKS__
extern "C" {

	_EXPORT void
	#if __GNUC__
	DrawPictureAsync__5BViewPclG6BPoint
	#elif __MWERKS__
	DrawPictureAsync__5BViewFPcl6BPoint
	#endif
	(BView* This, char *filename, long offset, BPoint where)
	{
		return This->DrawPictureAsync(filename, offset, where);
	}

}
#endif

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
