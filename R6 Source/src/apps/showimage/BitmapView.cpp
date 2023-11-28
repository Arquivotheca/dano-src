/*	BitmapView.cpp
*/

/* It is unfortunate that this program was grown right before deadline from a */
/* simple demonstration sample for Datatyps into the standard BeOS image */
/* viewing application. It leaves something to be desired in structure. */

#include "BitmapView.h"
#include "ShowWindow.h"
#include "prefs.h"
#include "prefix.h"
#include "ResourceData.h"

#include <interface/Alert.h>
#include <interface/Bitmap.h>
#include <interface/ScrollBar.h>
#include <string.h>
#include <stdio.h>
#include <TranslationKit.h>
#include <Clipboard.h>
#include <Directory.h>
#include <File.h>
#include <Path.h>
#include <support/DataIO.h>
#include <support/StreamIO.h>
#include <NodeInfo.h>
#include <ByteOrder.h>
#include <Screen.h>
#include <ResourceStrings.h>
#include <Autolock.h>
#include <stdlib.h>
#include <unistd.h>
#include <Beep.h>

#include <experimental/ResourceSet.h>

extern BResourceStrings g_strings;

static BResourceSet gResources;
static bool gResInitialized = false;
static BLocker gResLocker;

BResourceSet& Resources()
{
	if( gResInitialized ) return gResources;
	
	BAutolock l(&gResLocker);
	if( gResInitialized ) return gResources;
	
	gResources.AddResources((void*)&Resources);
	gResInitialized = true;
	
	return gResources;
}

static rgb_color gray = { 220, 220, 220, 255 };

bool BitmapView::fTempAnimBreak;

static rgb_color sCheckerGrayColor  = { 220, 220, 220, 255 };
static rgb_color sCheckerWhiteColor = { 255, 255, 255, 255 };

static BBitmap* sGrayChecker;


static BRect zoom_sel_rect(const BRect& in, const float zoom)
{
	return BRect(	in.left <= in.right ? (in.left*zoom) : ((in.left+1)*zoom)-1,
					in.top <= in.bottom ? (in.top*zoom) : ((in.top+1)*zoom)-1,
					in.left > in.right ? (in.right*zoom) : ((in.right+1)*zoom)-1,
					in.top > in.bottom ? (in.bottom*zoom) : ((in.bottom+1)*zoom)-1);
}
static BRect round(const BRect& in)
{
	return BRect(	floor(in.left+.5), floor(in.top+.5),
					floor(in.right+.5), floor(in.bottom+.5));
}

static float unzoom(const float in, const float zoom)
{
	return floor((in/zoom));
}

static BPoint unzoom(const BPoint& in, const float zoom)
{
	return BPoint(unzoom(in.x,zoom), unzoom(in.y,zoom));
}

BitmapView::BitmapView(
	BBitmap *		map,
	const char *	comment,
	const BMessage *slices) :
	BView(map->Bounds(), "bitmap", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED | B_NAVIGABLE),
	fMargins(-20,-20,20,20),
	fMouseDown(false)
{
	fZoom = 1.0;
	
	fClipCount = 0;
	fBitmap = map;
	fOwnership = true;
	fAdjustBars = true;
	SetLowColor(gray);
	SetViewColor(ui_color(B_UI_PANEL_BACKGROUND_COLOR));
	if (comment != NULL)
		strncpy(fComment, comment, 255);
	else
		fComment[0] = 0;
	fDrawComment = false;
	fComment[255] = 0;
	fSelection.Set(-1,-1,-2,-2);
	fDithered = 0;
	fFloater = 0;
	fUndo = 0;
	fPromised = 0;
	fCurCursor = B_CURSOR_SYSTEM_DEFAULT;
	
	if (slices) {
		SetSlices(slices);
		fSlicing = true;
	} else {
		fSlicing = false;
	}
	
	fMouseState = RESTING;
	
	//	reveal comment under picture
	if (fComment[0]) {
		BFont font;
		GetFont(&font);
		font_height fh;
		font.GetHeight(&fh);
		const BRect b(BitmapBounds());
		fCommentPos.y = b.bottom+fh.ascent+fh.leading+4;
		fCommentPos.x = (b.right-font.StringWidth(fComment))/2;
		fDrawComment = true;
		if (fMargins.bottom < fh.ascent+fh.descent+fh.leading+8)
			fMargins.bottom = fh.ascent+fh.descent+fh.leading+8;
	}

	uint8 patdata[4] = { 0xcc, 0x66, 0x33, 0x99 };
	int8 cycle = 0;

	for (int8 i = 0; i < 4; i++)
	{
		for (int8 j = 0; j < 8; j++)
		{
			fSelPattern[i].data[j] = patdata[cycle];
			cycle++;
			if (cycle > 3)
				cycle = 0;
		}
		cycle++;
	}

	static BBitmap gc(BRect(0, 0, 7, 7), B_RGBA32);
	sGrayChecker = &gc;
	rgb_color* bits = static_cast<rgb_color*>(sGrayChecker->Bits());
	for( int32 i = 0; i < sGrayChecker->BitsLength() / 4; i++ )
	{
		bits[i] = sCheckerGrayColor;
	}
	
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_SCROLLED | B_UPDATE_EXPOSED);
}


BitmapView::~BitmapView()
{
	if (fOwnership && fBitmap)
		delete fBitmap;
	delete fFloater;
	delete fDithered;
	delete fUndo;
	delete fPromised;
}


float
BitmapView::ZoomFactor() const
{
	return fZoom;
}

void
BitmapView::SetZoomFactor(
	float			zoom)
{
	if (zoom != fZoom) {
		fZoom = zoom;
		Invalidate();
		if (fAdjustBars)
			AdjustBars();
	}
}

BRect
BitmapView::BitmapBounds() const
{
	if (fBitmap) return zoom_sel_rect(fBitmap->Bounds(),fZoom);
	return BRect();
}

BRect
BitmapView::RealBitmapBounds() const
{
	if (fBitmap) return fBitmap->Bounds();
	return BRect();
}

void
BitmapView::SetOwnership(
	bool			ownership)
{
	fOwnership = ownership;
}


bool
BitmapView::GetOwnership()
{
	return fOwnership;
}


void
BitmapView::SetBitmap(
	BBitmap *		map)
{
	if (fOwnership && fBitmap)
		delete fBitmap;
	delete fDithered;
	fDithered = NULL;
	fBitmap = map;
	Invalidate();
	if (fAdjustBars)
		AdjustBars();
}


BBitmap *
BitmapView::GetBitmap()
{
	return fBitmap;
}


void
BitmapView::SetMargins(
	BRect			margins)
{
	fMargins = margins;
	if (fAdjustBars)
		AdjustBars();
}


BRect
BitmapView::GetMargins()
{
	return fMargins;
}


void
BitmapView::SetAdjustBars(
	bool			adjust)
{
	fAdjustBars = adjust;
	if (fAdjustBars)
		AdjustBars();
}


bool
BitmapView::GetAdjustBars()
{
	return fAdjustBars;
}

void
BitmapView::EnableSlicing(
	bool			enabled)
{
	if (fSlicing != enabled) {
		fSlicing = enabled;
		if (fSlicing) DropFloater();
		Invalidate();
	}
}

bool
BitmapView::IsSlicing() const
{
	return fSlicing;
}

void
BitmapView::SetSlices(
	const BMessage*	slices)
{
	fSlices.clear();
	if (slices) {
		const char* name;
		type_code type;
		void* cookie = NULL;
		while (slices->GetNextName(&cookie, &name, &type) == B_OK) {
			BRect r;
			if (slices->FindRect(name, &r) == B_OK)
				fSlices[BString(name)] = r;
		}
	}
	Invalidate();
}

void
BitmapView::GetSlices(BMessage* into) const
{
	map<BString,BRect>::const_iterator i(fSlices.begin());
	const map<BString,BRect>::const_iterator e(fSlices.end());
	while (i != e) {
		into->AddRect(i->first.String(), i->second);
		++i;
	}
}

void
BitmapView::AddSlice(
	const char *	name,
	BRect			slice,
	bool			unique)
{
	BString tmp;
	if (!name || !*name) name = "Unnamed";
	if (unique) {
		int32 seq = 1;
		do {
			tmp = name;
			if (seq > 1) {
				tmp << " " << seq;
			}
			seq++;
		} while (fSlices.find(tmp) != fSlices.end());
		name = tmp.String();
	}
	slice = round(slice);
	if (slice.left < 0) slice.left = 0;
	if (slice.top < 0) slice.top = 0;
	if (slice.right < slice.left) slice.right = slice.left;
	if (slice.bottom < slice.top) slice.bottom = slice.top;
	InvalidateSlice(name);
	fSlices[name] = slice;
	InvalidateSelection(slice);
	SetActiveSlice(name);
	SlicesChanged(NULL);
}

void
BitmapView::SetActiveSlice(
	const char *	name)
{
	if (fActiveSlice != name) {
		InvalidateSlice(fActiveSlice.String());
		fActiveSlice = name;
		InvalidateSlice(fActiveSlice.String());
		BMessage msg(msg_SelectSlice);
		Window()->PostMessage(&msg);
	}
}

const char*
BitmapView::ActiveSlice(
	BRect * 		rect) const
{
	if (rect) {
		map<BString,BRect>::const_iterator i = fSlices.find(fActiveSlice);
		if (i != fSlices.end()) {
			*rect = i->second;
			return i->first.String();
		}
		return NULL;
	}
	return fActiveSlice.String();
}

void
BitmapView::RemoveSlice(
	const char *	name)
{
	InvalidateSlice(name);
	fSlices.erase(BString(name));
	if (fActiveSlice == name)
		fActiveSlice = "";
	SlicesChanged(NULL);
}

void
BitmapView::InvalidateSelection(const BRect& r) const
{
	const_cast<BitmapView*>(this)->Invalidate(
		zoom_sel_rect(r,fZoom).InsetByCopy(-2, -2));
}

void
BitmapView::InvalidateSlice(const char* name) const
{
	if (name && *name) {
		map<BString,BRect>::const_iterator i(fSlices.find(BString(name)));
		if (i != fSlices.end())
			InvalidateSelection(i->second);
	}
}

void
BitmapView::SlicesChanged(const char* name) const
{
	BMessage msg(msg_SetDirty);
	msg.AddBool("slices", true);
	if (name && *name) msg.AddString("name", name);
	Window()->PostMessage(&msg);
}

void
BitmapView::AttachedToWindow()
{
}

void
BitmapView::Draw(
	BRect			area)
{
	if (IsPrinting()) {
		DrawBitmap(fBitmap, area, area);
		return;
	}

	SetDrawingMode(B_OP_COPY);
	if (!fBitmap) {
		FillRect(area, B_SOLID_LOW);
		return;
	}

	BView::Draw(area);
	
	BBitmap * draw = fBitmap;
	
	BScreen scrn(Window());

	color_space space = scrn.ColorSpace();
	switch (space) {
	case B_RGBA15:
	case B_RGBA15_BIG:
	case B_RGB15:
	case B_RGB15_BIG:
		draw = DitherBitmap(fBitmap, B_RGB15);
		break;
	case B_RGB16:
	case B_RGB16_BIG:
		draw = DitherBitmap(fBitmap, B_RGB16);
		break;
	case B_CMAP8:
		draw = DitherBitmap(fBitmap, B_CMAP8);
		break;
	default:
		break;
	}
	
	bool drawAlpha = false;
	color_space dcs = draw->ColorSpace();
	switch( dcs ) {
	case B_RGBA15:
	case B_RGBA15_BIG:
	case B_CMAP8:
	case B_RGBA32:
	case B_RGBA32_BIG:
		drawAlpha = true;
		break;
	default:
		break;
	}

	/* draw bitmap */
	BRect bmFrame = draw->Bounds();
	BRect frame = BitmapBounds(); 

	if( true == drawAlpha ) {	
		// Background Checkers
		rgb_color high = HighColor();
		
		SetHighColor(sCheckerWhiteColor);
		FillRect(frame);
		SetHighColor(sCheckerGrayColor);
		
		bool whitex = true;
		BRect fill;
		for( uint32 x = (uint32)frame.left; x < (uint32)frame.right; x += 8 )
		{
			fill.left = x;
			bool whitey = whitex;
			
			for( uint32 y = (uint32)frame.top; y < (uint32)frame.bottom; y += 8 )
			{
				if( false == whitey )
				{
					fill.top = y;
					fill.right = x + 7 > frame.right ? frame.right : x + 7;
					fill.bottom = y + 7 > frame.bottom ? frame.bottom : y + 7;

// todo: Investigate draw v fill speed issues, it seems like DrawBitmap would be faster...
//					BRect source = fill;
//					source.OffsetTo(0, 0);
//					DrawBitmapAsync(sGrayChecker, source, fill);

					FillRect(fill);
#if 0
// todo: Need to optimize bitmap drawing for this to work correctly.			
					if( area.Intersects(fill) )
					{
						// choose the faster method
						// FillRect is faster seeming ...
						DrawBitmapAsync(sGrayChecker, source, fill);
						FillRect(fill);
					}
#endif
				}
				whitey = !whitey;
			}
			whitex = !whitex;
		}
	
		SetHighColor(high);
		SetDrawingMode(B_OP_ALPHA); 
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	}
	
	DrawBitmapAsync(draw, bmFrame, frame);
	SetDrawingMode(B_OP_COPY);
	
	/* draw slices */
	if (fSlicing) {
		PushState();
		map<BString,BRect>::const_iterator i(fSlices.begin());
		const map<BString,BRect>::const_iterator e(fSlices.end());
		SetDrawingMode(B_OP_COPY);
		SetPenSize(1);
		static pattern marquee_pattern = {
			{ 0xf8, 0xf1, 0xe3, 0xc7, 0x8f, 0x1f, 0x3e, 0x7c }
		};
		while (i != e) {
			const BRect r = zoom_sel_rect(i->second,fZoom);
			if (i->first == fActiveSlice) {
				SetHighColor(255, 0, 0);
				SetLowColor(0, 255, 255);
				StrokeRect(r.InsetByCopy(-1, -1), marquee_pattern);
			} else {
				SetLowColor(0, 0, 0);
				SetHighColor(255, 255, 255);
			}
			StrokeRect(r, marquee_pattern);
			++i;
		}
		PopState();
	}
	
	/* draw floater */
	if (fFloater && (fSelection.left >= 0) && (fSelection.top >= 0)) {
		BRect r(fSelection);
		r.OffsetTo(B_ORIGIN);
		if( true == drawAlpha ) SetDrawingMode(B_OP_ALPHA); 
		DrawBitmapAsync(fFloater, r, zoom_sel_rect(fSelection,fZoom));
		SetDrawingMode(B_OP_COPY);
	}

	/* draw frame */
	frame.InsetBy(-1, -1);
	StrokeRect(frame);

	SetLowColor(255,255,255);
	/* draw selection */
	if (fSelection.left > -1) {
		StrokeSelection();
	}

	/* draw comment */
	if (fDrawComment) {
		BFont font;
		GetFont(&font);
		float width = font.StringWidth(fComment);
		font_height fh;
		font.GetHeight(&fh);
		float baseline = fh.ascent+fh.leading;
		BRect frame(fCommentPos.x-2, fCommentPos.y-baseline-1, 
			fCommentPos.x+width+1, fCommentPos.y+fh.descent+1);
		rgb_color low_color = { 255, 255, 222, 0 };
		rgb_color high_color = { 0, 0, 0, 0 };
		SetHighColor(high_color);
		SetLowColor(low_color);
		FillRect(frame, B_SOLID_LOW);
		StrokeRect(frame);
		DrawString(fComment, fCommentPos);
	}
}


void
BitmapView::MouseDown(
	BPoint		where)
{
	fMouseDown = true;
	MakeFocusNoScroll();
	SetExplicitFocus(true);
	
	where = unzoom(where, fZoom);
	
	fAnchor = fLastPos = where;
	fConstraint = fBitmap->Bounds();
	
	if (IsSlicing()) {
		MouseDownSlice(where);
		return;
	}
	
	BPoint next;
	uint32 flags;
	fTempAnimBreak = false;
	if (!fBitmap) {
no_action:
		Window()->Activate(true);
		return;
	}
	if ((fSelection.left > -1) && (fSelection.Contains(where))) {
		// drag detected!
		ShowWindow * sw = static_cast<ShowWindow *>(Window());
		if (!sw) goto no_action;
		int32 format, translator;
		if (!sw->GetDragInfo(&format, &translator)) {
			BAlert * alrt = new BAlert("No translators", 
				g_strings.FindString(1), g_strings.FindString(2));
			alrt->Go();
			goto no_action;
		}
		// wait for drag to occur 
		while (true) {
			GetMouse(&next, &flags);
			next = unzoom(next, fZoom);
			if (!((flags & B_PRIMARY_MOUSE_BUTTON) || (flags & B_SECONDARY_MOUSE_BUTTON))) {
				goto no_action;
			}
			if (next != where) {
				break;	// drag started 
			}
			snooze(50000);
			Pulse();
		}

		BMessage msg(B_SIMPLE_DATA);
		msg.AddInt32("be:_format", format);
		msg.AddInt32("be:_translator", translator);
		if (fFloater) {
			delete fPromised;
			fPromised = new BBitmap(fFloater->Bounds(), fFloater->ColorSpace());
			memcpy(fPromised->Bits(), fFloater->Bits(), fPromised->BitsLength());
			msg.AddRect("be:_frame", fPromised->Bounds());
			BPoint wh2(where.x-fSelection.left, where.y-fSelection.top);
			msg.AddPoint("be:_source_point", wh2);
			msg.AddPointer("be:_bitmap_ptr", fPromised);
		}
		else {
			delete fPromised;
			fPromised = NULL;
			msg.AddRect("be:_frame", fSelection);
			msg.AddPoint("be:_source_point", where);
			msg.AddPointer("be:_bitmap_ptr", fBitmap);
		}
		sw->AddDNDTypes(&msg);
		msg.AddInt32("be:actions", B_COPY_TARGET);
		msg.AddInt32("be:actions", B_TRASH_TARGET);
		msg.AddString("be:clip_name", g_strings.FindString(3));
		DoDrag(&msg, where);
		return;
	}
	Window()->Activate(true);

	if (fConstraint.Contains(where)) {
		BRect r = fSelection;
		DropFloater();
		fSelection = r;
		fMouseState = CREATING_FLOATER;
		
		ChooseCursor(R_CURS_Cross);
		BeginRectTracking(BRect(where, where), B_TRACK_RECT_CORNER);
		return;
	}
}


void
BitmapView::MouseDownSlice(
	BPoint		where)
{
	slice_location location;
	map<BString,BRect>::const_iterator i = SliceAt(where, true, &location);
	ChooseCursor(CursorFromLocation(location));
	
	if (location == SLICE_NOTHING) {
		fMouseState = CREATING_SLICE;
		BeginRectTracking(BRect(where, where), B_TRACK_RECT_CORNER);
	} else if (location == SLICE_SELECT) {
		SetActiveSlice(i->first.String());
		fMouseState = SELECTING_SLICE;
	} else {
		fMouseState = MOVING_SLICE;
		fMouseSlice = fActiveSlice;
		fOrigSlice = i->second;
		fMoveLocation = location;
	}
}

map<BString,BRect>::const_iterator
BitmapView::SliceAt(
	BPoint		where,
	bool		prefer_active,
	slice_location* location) const
{
	const map<BString,BRect>::const_iterator N(fSlices.end());
	map<BString,BRect>::const_iterator i = N;
	
	bool found_active = false;
	
	if (prefer_active) {
		if (fActiveSlice.Length() > 0) {
			i = fSlices.find(fActiveSlice);
			if (i != N) {
				if (i->second.InsetByCopy(-1, -1).Contains(where))
					found_active = true;
				else
					i = N;
			}
		}
	}
	
	if (i == N) {
		i = fSlices.begin();
		while (i != N) {
			if (i->second.Contains(where))
				break;
			++i;
		}
	}
	
	if (location) {
		*location = SLICE_NOTHING;
		if (i != N) {
			if (!found_active && prefer_active) {
				*location = SLICE_SELECT;
			} else {
				*location = SLICE_MOVE;
				float fudge = 0;
				if (i->second.Width() > 5)
					fudge = 1;
				else if (i->second.Width() > 10)
					fudge = 2;
				else if (i->second.Width() > 20)
					fudge = 3;
				if (i->second.Width() < 1)
					*location = SLICE_RIGHT;
				else if (where.x <= (i->second.left+fudge))
					*location = SLICE_LEFT;
				else if (where.x >= (i->second.right-fudge))
					*location = SLICE_RIGHT;
				
				slice_location yloc = SLICE_MOVE;
				fudge = 0;
				if (i->second.Height() > 5)
					fudge = 1;
				else if (i->second.Height() > 10)
					fudge = 2;
				else if (i->second.Height() > 20)
					fudge = 3;
				if (i->second.Height() < 1)
					yloc = SLICE_BOTTOM;
				else if (where.y <= (i->second.top+fudge))
					yloc = SLICE_TOP;
				else if (where.y >= (i->second.bottom-fudge))
					yloc = SLICE_BOTTOM;
				
				if (*location == SLICE_LEFT) {
					if (yloc == SLICE_TOP) *location = SLICE_LEFTTOP;
					else if (yloc == SLICE_BOTTOM) *location = SLICE_LEFTBOTTOM;
				} else if (*location == SLICE_RIGHT) {
					if (yloc == SLICE_TOP) *location = SLICE_RIGHTTOP;
					else if (yloc == SLICE_BOTTOM) *location = SLICE_RIGHTBOTTOM;
				} else
					*location = yloc;
			}
		}
	}
	
	return i;
}

int32
BitmapView::CursorFromLocation(
	slice_location location) const
{
	switch (location) {
		case SLICE_NOTHING:
			return R_CURS_Cross;
		case SLICE_SELECT:
			return -1;
		case SLICE_MOVE:
			return R_CURS_Move;
		case SLICE_LEFT:
		case SLICE_RIGHT:
			return R_CURS_LeftRight;
		case SLICE_TOP:
		case SLICE_BOTTOM:
			return R_CURS_UpDown;
		case SLICE_LEFTTOP:
		case SLICE_RIGHTBOTTOM:
			return R_CURS_DiagLRUL;
		case SLICE_LEFTBOTTOM:
		case SLICE_RIGHTTOP:
			return R_CURS_DiagURLL;
	}
	return -1;
}

void
BitmapView::MouseUp(
	BPoint			where)
{
	fMouseDown = false;
	where = unzoom(where, fZoom);
	
	if (fMouseState == CREATING_FLOATER) {
		if (fAnchor == fLastPos) {
			BRect old = fSelection;
			fSelection.Set(-1,-1,-2,-2);
			InvalidateSelection(old);
			Window()->UpdateIfNeeded();
		}
	} else if (fMouseState == CREATING_SLICE) {
		if (fSelection.IsValid() && fSelection.left >= 0 && fSelection.top >= 0) {
			AddSlice(NULL, fSelection, true);
			InvalidateSelection(fSelection);
			fSelection.Set(-1, -1, -2, -2);
		}
	} else if (fMouseState == MOVING_SLICE) {
		map<BString,BRect>::iterator i(fSlices.find(fMouseSlice));
		if (i != fSlices.end() && i->second != fOrigSlice) {
			SlicesChanged(fMouseSlice.String());
		}
	}
	
	if (IsSlicing()) {
		slice_location location;
		SliceAt(where, true, &location);
		ChooseCursor(CursorFromLocation(location));
	} else {
		ChooseCursor(-1);
	}
	
	fMouseState = RESTING;
	EndRectTracking();
	StrokeSelection();
}

void
BitmapView::MouseMoved(
	BPoint			where,
	uint32			code,
	const BMessage *)
{
	where = unzoom(where, fZoom);
	
	if (code == B_ENTERED_VIEW)
		SetViewCursor(fCurCursor);
	
	if (fMouseState == CREATING_FLOATER || fMouseState == CREATING_SLICE) {
//		ChooseCursor(R_CURS_Cross);
		BRect constraint = fBitmap->Bounds();
		where.ConstrainTo(constraint);
		if (fLastPos == where)
			return;
		const BRect old = fSelection;
		if (where.x < fAnchor.x) 
			fSelection.left = where.x;
		else
			fSelection.left = fAnchor.x;
		if (where.x > fAnchor.x)
			fSelection.right = where.x;
		else
			fSelection.right = fAnchor.x;
		if (where.y < fAnchor.y)
			fSelection.top = where.y;
		else
			fSelection.top = fAnchor.y;
		if (where.y > fAnchor.y)
			fSelection.bottom = where.y;
		else
			fSelection.bottom = fAnchor.y;
		fSelection = round(fSelection & fBitmap->Bounds());
		fLastPos = where;
	} else if (fMouseState == MOVING_SLICE) {
		ChooseCursor(CursorFromLocation(fMoveLocation));
		map<BString,BRect>::iterator i(fSlices.find(fMouseSlice));
		if (i != fSlices.end()) {
			BRect old = i->second;
			i->second = fOrigSlice;
			if (fMoveLocation == SLICE_MOVE) {
				i->second.OffsetBy(where-fAnchor);
			}
			if (fMoveLocation == SLICE_LEFT || fMoveLocation == SLICE_LEFTTOP ||
					fMoveLocation == SLICE_LEFTBOTTOM) {
				i->second.left += (where.x-fAnchor.x);
			}
			if (fMoveLocation == SLICE_TOP || fMoveLocation == SLICE_LEFTTOP ||
					fMoveLocation == SLICE_RIGHTTOP) {
				i->second.top += (where.y-fAnchor.y);
			}
			if (fMoveLocation == SLICE_RIGHT || fMoveLocation == SLICE_RIGHTTOP ||
					fMoveLocation == SLICE_RIGHTBOTTOM) {
				i->second.right += (where.x-fAnchor.x);
			}
			if (fMoveLocation == SLICE_BOTTOM || fMoveLocation == SLICE_LEFTBOTTOM ||
					fMoveLocation == SLICE_RIGHTBOTTOM) {
				i->second.bottom += (where.y-fAnchor.y);
			}
			i->second = round(i->second & fBitmap->Bounds());
			if (i->second != old) {
				InvalidateSelection(old);
				InvalidateSelection(i->second);
			}
		}
	} else if (fMouseState == SELECTING_SLICE) {
		ChooseCursor(-1);
	} else if (fMouseState == RESTING) {
		if (IsSlicing()) {
			slice_location location;
			SliceAt(where, true, &location);
			ChooseCursor(CursorFromLocation(location));
		} else {
			ChooseCursor(-1);
		}
	}
}

void
BitmapView::FrameResized(
	float			newWidth,
	float			newHeight)
{
	BView::FrameResized(newWidth, newHeight);
	if (fAdjustBars)
		AdjustBars();
}


	static void
	AdjustBar(
		BScrollBar *		bar,
		float				page,
		float				total,
		float				start)
	{
		if (total <= page) {
			bar->SetRange(start, start);
		} else {
			bar->SetRange(start, start+total-page);
		}
		float pgStep = page-4.0;
		if (pgStep<16.0)
			pgStep = 16.0;
		bar->SetSteps(4.0, pgStep);
		bar->SetProportion(page / total);
	}

void
BitmapView::AdjustBars()
{
	const BRect area(BitmapBounds());
	const BRect bounds(Bounds());

	BScrollBar *bar = ScrollBar(B_HORIZONTAL);
	if (bar)
		AdjustBar(bar, bounds.Width(), area.Width()+fMargins.Width()+1, fMargins.left);
	bar = ScrollBar(B_VERTICAL);
	if (bar)
		AdjustBar(bar, bounds.Height(), area.Height()+fMargins.Height()+1, fMargins.top);
}


void
BitmapView::WindowActivated(
	bool			state)
{
	BView::WindowActivated(state);
	if (fAdjustBars && state) {
		AdjustBars();
	}
}


void
BitmapView::SetSelection(
	BRect sel,
	BBitmap * map)
{
	if (map != fFloater) {
		DropFloater();
	}
	else if (fSelection.left > -1) {
		fFloater = NULL;
		BRect r = fSelection;
		fSelection.Set(-1,-1,-2,-2);
// draw?
		Invalidate(r*fZoom);
	}
	fSelection = sel;
	fFloater = map;
	if ((fSelection.left > -1) || fFloater) {
// draw?
		Invalidate(fSelection*fZoom);
	}
}


void
BitmapView::Pulse()
{
	BView::Pulse();
	if (fSelection.left < 0) return; /* no selection, no scroll */
	if (fTempAnimBreak) {
		if ( true == fMouseDown ) {
			return;
		}
		fTempAnimBreak = false;
	}
/*
	// removed in favor of faster App-Server doing rect tracking for us.
	// Pattern animation.
	// One wonders: why not just iterate through the 4 patterns?
	uint8 top_1 = fSelPattern[1].data[0];
	uint8 bot_3 = fSelPattern[3].data[7];
	for (int ix=0; ix<8; ix++) {
		fSelPattern[0].data[ix] = (fSelPattern[0].data[ix] >> 7) | (fSelPattern[0].data[ix] << 1);
		fSelPattern[1].data[ix] = fSelPattern[1].data[(ix+1)&7];
		fSelPattern[2].data[ix] = (fSelPattern[2].data[ix] >> 1) | (fSelPattern[2].data[ix] << 7);
		fSelPattern[3].data[7-ix] = fSelPattern[3].data[(6-ix)&7];
	}
	fSelPattern[1].data[7] = top_1;
	fSelPattern[3].data[0] = bot_3;
	StrokeSelection();
	Flush();
*/
}


void
BitmapView::StrokeSelection()
{
	const BRect s(zoom_sel_rect(fSelection,fZoom));
	
	StrokeRect(s, fSelPattern[0]);
}


void
BitmapView::MessageReceived(
	BMessage * message)
{
	switch (message->what) {
	case B_SIMPLE_DATA:
	case B_ARCHIVED_OBJECT:
		SimpleData(message);
		break;
	case B_COPY_TARGET:
		CopyTarget(message);
		break;
	case B_TRASH_TARGET:
		TrashTarget(message);
		break;
	default:
		BView::MessageReceived(message);
		break;
	}
}


void
BitmapView::SimpleData(
	BMessage * message)
{
	BBitmap * bitmap_ptr = NULL;
	BBitmap * drop_map = NULL;
	BRect fr;
	BPoint pt = B_ORIGIN;
	int w, h;
	BPoint target;

	if (message->FindRect("be:_frame", &fr)) {
		fr.Set(0,0,0,0);
	}
	if (message->WasDropped()) {
		target = message->DropPoint(&pt);
		ConvertFromScreen(&target);
		target = unzoom(target, fZoom);
		if (!message->FindPoint("be:_source_point", &pt)) {
			pt.x = pt.x - fr.left;
			pt.y = pt.y - fr.top;
		}
	}
	else if (!message->FindPoint("be:destination_point", &target)) {
		/* target is OK */
	}
	else {
		target = B_ORIGIN;
	}
	target.x -= pt.x;
	target.y -= pt.y;
	if (target.x < 0) {
		fr.left -= target.x;
		target.x = 0;
	}
	if (target.y < 0) {
		fr.top -= target.y;
		target.y = 0;
	}
	if ((fr.left > fr.right) || (fr.top > fr.bottom)) goto outside;

	if (!message->IsSourceRemote() && !message->FindPointer("be:_bitmap_ptr", (void**) &bitmap_ptr)) {
		/* this is one big race condition -- what if the source window went away? */
		/* make sure we have bitmap in our color space */
		if (message->FindRect("be:_frame", &fr)) {
			fr = bitmap_ptr->Bounds();
		}
		if (((bitmap_ptr == fBitmap) || ((fPromised != NULL) && (bitmap_ptr == fPromised))) && 
			(fFloater != NULL) && (target.x >= 0) && (target.y >= 0)) {
			drop_map = fFloater;
		}
		else {
			BRect temp(fr);
			fr.OffsetTo(B_ORIGIN);
			if (!fBitmap) {
				fBitmap = new BBitmap(fr, bitmap_ptr->ColorSpace());
				fOwnership = true;
			}
			drop_map = new BBitmap(fr, fBitmap->ColorSpace(), true);
			BView * v = new BView(fr, "_", B_FOLLOW_NONE, B_WILL_DRAW);
			drop_map->Lock();
			drop_map->AddChild(v);
			v->DrawBitmap(bitmap_ptr, temp, fr);
			v->Sync();
			drop_map->Unlock();
		}
	}
	else {
		if (message->what == B_ARCHIVED_OBJECT) {
			if (!validate_instantiation(message, "BBitmap")) goto err;
			drop_map = dynamic_cast<BBitmap *>(instantiate_object(message));
			if (!drop_map) goto err;
		}
		else if (message->HasRef("refs")) {
			BBitmapStream output;
			entry_ref ref;
			if (message->FindRef("refs", &ref)) goto err;
			BFile input;
			if (input.SetTo(&ref, O_RDONLY)) goto err;
			if (BTranslatorRoster::Default()->Translate(&input, NULL, NULL, &output, B_TRANSLATOR_BITMAP)) 
				goto err;
			if (output.DetachBitmap(&drop_map)) goto err;
		}
		else {
	err:
			beep();
			return;
		}
		if (message->FindRect("be:_frame", &fr)) {
			fr = drop_map->Bounds();
		}
	}
	w = (int)fr.Width();
	if (w+target.x > fBitmap->Bounds().right) {
		w = (int)(fBitmap->Bounds().right-target.x);
	}
	h = (int)fr.Height();
	if (h+target.y > fBitmap->Bounds().bottom) {
		h = (int)(fBitmap->Bounds().bottom-target.y);
	}
	if (drop_map) {
		fr.Set(target.x, target.y, target.x+w, target.y+h);
		SetSelection(fr, drop_map);
		delete fDithered;
		fDithered = NULL;
		Invalidate(fr*fZoom);
	}
outside:
		;
}


/* This function is unnecessarily hairy; using too many */
/* GOTOs and pointers as flags. If you thought hard about it, */
/* it could probably be re-ordered to make sense. I apologize */
/* for not doing that. */
void
BitmapView::CopyTarget(
	BMessage * message)
{
	const char * str;
	int32 format = -1;
	int32 translator = -1;
	BFile * out_file = NULL;
	BEntry out_file_entry;
	BPositionIO * out_data = NULL;
	BMallocIO * out_mem = NULL;
	ShowWindow * sw = dynamic_cast<ShowWindow *>(Window());

	if (!sw) {
		goto error;
	}
	if (!fBitmap) {
		goto error;
	}
	for (int ix=0; !message->FindString("be:types", ix, &str); ix++) {
		if (!strcasecmp(str, B_FILE_MIME_TYPE)) {
			goto do_file;
		}
	}

find_type:
	/* find matching type */
	for (int ix=0; !message->FindString("be:types", ix, &str); ix++) {
		const char *typeName = 0;
		message->FindString("be:type_descriptions", ix, &typeName);
		if (sw->MatchType(str, &format, &translator, typeName)) {
			goto do_data;
		}
	}

error:
	{
		delete out_file;
		delete out_mem;
		BMessage error(B_MESSAGE_NOT_UNDERSTOOD);
		error.AddInt32("error", B_NO_TRANSLATOR);
		message->SendReply(&error);
		return;
	}

do_data:
	out_mem = new BMallocIO;
	out_data = out_mem;
	out_file_entry.Unset();
	goto translate;

do_file:
	/* find matching type */
	for (int ix=0; !message->FindString("be:filetypes", ix, &str); ix++) {
		const char *typeName = 0;
		message->FindString("be:type_descriptions", ix, &typeName);
		if (sw->MatchType(str, &format, &translator, typeName)) {
//			PRINT(("matching type %s, format %d, translator %d\n",
//				str, format, translator));
			entry_ref dir;
			const char *name;
			if (message->FindString("name", &name))
				goto find_type;
	
			if (message->FindRef("directory", &dir)) {
				goto find_type;	/* fall back to data */
			}

			BDirectory d(&dir);
			out_file_entry.SetTo(&d, name);

			out_file = new BFile(&d, name, O_RDWR | O_CREAT | O_TRUNC);
			if (out_file->InitCheck()) {
				delete out_file;
				out_file = NULL;
				goto find_type;
			}
			out_data = out_file;
			{
				BNodeInfo mime(out_file);
				mime.SetType(str);
			}
			goto translate;
		}
	}
	goto find_type;	/* for data */

translate:
	{
		BRect fr;
		bool fr_ok = false;
		if (!message->FindRect("be:_frame", &fr)) fr_ok = true;
		if (!fr_ok) {
			const BMessage * prev = message->Previous();
			if (prev && !prev->FindRect("be:_frame", &fr)) fr_ok = true;
		}
		BBitmap * bm = NULL;
		if (fPromised) {
			bm = fPromised;
			fr = fPromised->Bounds();
		}
		else {
			if (!fr_ok) {
				fr = fBitmap->Bounds();
				bm = fBitmap;
			}
			else {	/* create clipping bitmap */
				fr = fr & fBitmap->Bounds();
				int w = (int)fr.Width()+1;
				if (w < 0) w = 0;
				bm = new BBitmap(fr, fBitmap->ColorSpace());
				/* Padding gets rounded down */
				int bpp = (int)fBitmap->BytesPerRow()/((int)fBitmap->Bounds().Width()+1);
				uchar * src = (uchar *)fBitmap->Bits();
				src += (int)fr.left*bpp;
				src += (int)fr.top*fBitmap->BytesPerRow();
				uchar * dst = (uchar *)bm->Bits();
	
				int32 count = (int)fr.Height();
				w = w*bpp;
				for (int ix=0; ix <= count; ix++) {
					memcpy(dst, src, w);
					dst += bm->BytesPerRow();
					src += fBitmap->BytesPerRow();
				}
			}
		}
		BBitmapStream strm(bm);
		status_t err = BTranslatorRoster::Default()->Translate(translator, &strm, 
			NULL, out_data, format);

		char msg[400];
		sprintf(msg, g_strings.FindString(4), strerror(err), err);
		PRINT((msg));
		if (err) {
			(new BAlert("", msg, g_strings.FindString(26)))->Go();
		}

		if (bm == fBitmap) {
			strm.DetachBitmap(&bm);
		}
		fPromised = NULL;	/* bitmap stream will delete it -- or it was already NULL */
		if (err < B_OK) {
			(void)out_file_entry.Remove();	/* ignore error, for instance if we translate to memory */
			goto error;
		}
	}
	if (out_mem != NULL) {
		BMessage reply(B_MIME_DATA);
		reply.AddData(str, B_MIME_TYPE, out_mem->Buffer(), out_mem->BufferLength());
		delete out_mem;
		message->SendReply(&reply);
	}
	
	if (out_file) {
		// if we created a clip file, mimeset it right away so the
		// right icon shows up
		BPath path;
		out_file_entry.GetPath(&path);
		update_mime_info(path.Path(), 0, 0, 0);
	}

	delete out_file;
}


void
BitmapView::CheckerBoard(
	BBitmap * map,
	BView *)
{
	uchar t8 = B_TRANSPARENT_8_BIT;
	rgb_color tc = B_TRANSPARENT_32_BIT;
	uint32 t32 = (tc.alpha<<24)|(tc.red<<16)|(tc.green<<8)|(tc.blue);
	uint16 t16 = ((tc.alpha&0x80)<<8)|((tc.red&0xf8)<<7)|((tc.green&0xf8)<<2)|
		((tc.blue&0xf8)>>3);

	switch (map->ColorSpace()) {
	case B_COLOR_8_BIT: {
			uint8 * ptr = (uint8 *)map->Bits();
			int add = map->BytesPerRow()/sizeof(uint8);
			int rows = (int)map->Bounds().Height()+1;
			while (rows > 0) {
				uint8 * next = ptr + add;
				int xx = (rows & 1);
				ptr += xx;
				for (int ix=xx; ix<add; ix+=2) {
					*(ptr++) = t8;
					ptr++;
				}
				ptr = next;
				rows--;
			}
		} break;
#if B_HOST_IS_LENDIAN
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
		t32 = B_SWAP_INT32(t32);
	case B_RGBA32:
	case B_RGB32: 
#else
	case B_RGB32:
	case B_RGBA32:
		t32 = B_SWAP_INT32(t32);
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
#endif
			{
			uint32 * ptr = (uint32 *)map->Bits();
			int add = map->BytesPerRow()/sizeof(uint32);
			int rows = (int)map->Bounds().Height()+1;
			while (rows > 0) {
				uint32 * next = ptr + add;
				int xx = (rows & 1);
				ptr += xx;
				for (int ix=xx; ix<add; ix+=2) {
					*(ptr++) = t32;
					ptr++;
				}
				ptr = next;
				rows--;
			}
		} break;
#if B_HOST_IS_LENDIAN
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
		t16 = B_SWAP_INT16(t16);
	case B_RGBA15:
	case B_RGB15: 
#else
	case B_RGB15:
	case B_RGBA15:
		t16 = B_SWAP_INT16(t16);
	case B_RGB15_BIG:
	case B_RGBA15_BIG: 
#endif
			{
			uint16 * ptr = (uint16 *)map->Bits();
			int add = map->BytesPerRow()/sizeof(uint16);
			int rows = (int)map->Bounds().Height()+1;
			while (rows > 0) {
				uint16 * next = ptr + add;
				int xx = (rows & 1);
				ptr += xx;
				for (int ix=xx; ix<add; ix+=2) {
					*(ptr++) = t16;
					ptr++;
				}
				ptr = next;
				rows--;
			}
		} break;
	default:
		/* do nothing */
		break;
	}
}


void
BitmapView::DoDrag(
	BMessage * message,
	BPoint pt)
{
	if (!fBitmap) return;
	fTempAnimBreak = true;
	/* We throttle size of dragged bitmap to ~ 0.5 MB */
	if ((fSelection.Width() < 1) || (fSelection.Height() < 1) || 
		(fSelection.Width()*fSelection.Height() > 100000)) {
		DragMessage(message, fSelection, this);
	}
	else {
		BBitmap * map;
		BRect fr(fSelection);
		fr.OffsetTo(B_ORIGIN);
		map = new BBitmap(fr, fBitmap->ColorSpace(), true);
		BView * v = new BView(fr, "view", B_FOLLOW_NONE, B_WILL_DRAW);
		map->Lock();
		map->AddChild(v);
		if (fFloater) {
			v->DrawBitmap(fFloater, B_ORIGIN);
		}
		else {
			v->DrawBitmap(fBitmap, fSelection, fr);
		}
		v->Sync();
#if 0	/* enable "transluscent" dragging */
		CheckerBoard(map, v);
#endif
		map->RemoveChild(v);
		delete v;
		map->Unlock();
		pt.x -= fSelection.left;
		pt.y -= fSelection.top;
		DragMessage(message, map, B_OP_BLEND, pt, this);
	}
}


BBitmap *
BitmapView::DitherBitmap(
	BBitmap * source, 
	color_space destination)
{
	if (!g_prefs.dither)
		return source;
	if (!source) 
		return source;
	if (source->ColorSpace() != B_RGB32 && source->ColorSpace() != B_RGBA32) 
		return source;

	bool alpha = false;
	if ((source->ColorSpace() == B_RGBA32) || (source->ColorSpace() == B_RGBA32_BIG)) {
		alpha = true;
	}
	switch (destination) {
	case B_CMAP8:
	case B_RGB15:
	case B_RGB16:
	case B_RGBA15:
		break;
	default:
		return source;
	}
	if (fDithered) {
		if (fDithered->ColorSpace() == destination)
			return fDithered;
		delete fDithered;
		fDithered = NULL;
	}
	BBitmap * map = new BBitmap(source->Bounds(), destination, false);
	void * out = map->Bits();
	uint32 out_rb = map->BytesPerRow();
	uint8 * in = (uint8 *)source->Bits();
	uint32 in_rb = source->BytesPerRow();
	int xs = (int)source->Bounds().Width()+1;
	const color_map * colors = system_colors();
	uint16 aval = ((alpha || (destination == B_RGBA15)) ? 0x8000 : 0);
	for (int iy=0; iy<map->Bounds().Height()+1; iy++) {
		int ered = 0;
		int egreen = 0;
		int eblue = 0;
		/* we are cheezy and only do right-propagation within each scanline */
		for (int ix=0; ix<xs; ix++) {

			eblue += in[ix*4];
			egreen += in[ix*4+1];
			ered += in[ix*4+2];

			switch (destination) {
				case B_CMAP8: {
					if (in[ix*4+3] < 128 && alpha) {
						((uint8 *)out)[ix] = B_TRANSPARENT_8_BIT;
						ered = egreen = eblue = 0;
					} else {
						uchar red = (ered > 255 ? 255 : (ered < 0 ? 0 : ered));
						uchar green = (egreen > 255 ? 255 : (egreen < 0 ? 0 : egreen));
						uchar blue = (eblue > 255 ? 255 : (eblue < 0 ? 0 : eblue));
						int pix = colors->index_map[(((int)red>>3)<<10)|(((int)green>>3)<<5)|(blue>>3)];
						rgb_color oc = colors->color_list[pix];
						((uint8 *)out)[ix] = pix;
						ered = ered-oc.red;
						egreen = egreen-oc.green;
						eblue = eblue-oc.blue;
					}
				}
				break;
				case B_RGB15:
				case B_RGBA15: {
					if (in[ix*4+3] < 128 && alpha) {
						((uint16 *)out)[ix] = 0x7fff;
						ered = egreen = eblue = 0;
					}
					else {
						uchar red = (ered > 255 ? 255 : (ered < 0 ? 0 : ered))&0xf8;
						uchar green = (egreen > 255 ? 255 : (egreen < 0 ? 0 : egreen))&0xf8;
						uchar blue = (eblue > 255 ? 255 : (eblue < 0 ? 0 : eblue))&0xf8;

						((uint16 *)out)[ix] = B_HOST_TO_LENDIAN_INT16(((int)red<<7)|((int)green<<2)|(blue>>3)|aval);

						ered = ered-(red|(red>>5));
						egreen = egreen-(green|(green>>5));
						eblue = eblue-(blue|(blue>>5));
					}
				}
				break;
				case B_RGB16: {
					uchar red = (ered > 255 ? 255 : (ered < 0 ? 0 : ered))&0xf8;
					uchar green = (egreen > 255 ? 255 : (egreen < 0 ? 0 : egreen))&0xfc;
					uchar blue = (eblue > 255 ? 255 : (eblue < 0 ? 0 : eblue))&0xf8;

					((uint16 *)out)[ix] = B_HOST_TO_LENDIAN_INT16(((int)red<<8)|((int)green<<3)|(blue>>3));

					ered = ered-(red|(red>>5));
					egreen = egreen-(green|(green>>6));
					eblue = eblue-(blue|(blue>>5));
				}
				break;
				default:
				break;
			}
		}
		out = (void *)((char *)out+out_rb);
		in += in_rb;
	}
	fDithered = map;
	return fDithered;
}


void
BitmapView::TrashTarget(
	BMessage *)
{
	if (IsSlicing()) {
		if (fActiveSlice.Length() > 0) {
			map<BString,BRect>::iterator i = fSlices.find(fActiveSlice);
			if (i != fSlices.end()) {
				InvalidateSlice(fActiveSlice.String());
				fSlices.erase(i);
				SlicesChanged();
			}
		}
	}
	
	BRect fr = fSelection;
	if (fFloater) {	/* delete the floater, not what's under it... */
		delete fUndo;
		fUndo = new undo_info(this, false);
		Invalidate(fr*fZoom);
		return;
	}
	if (!fSelection.IsValid()) {
		return;
	}
	BPoint target = fr.LeftTop();
	int w = (int)fr.Width();
	if (w+target.x > fBitmap->Bounds().right) {
		w = (int)(fBitmap->Bounds().right-target.x);
	}
	int h = (int)fr.Height();
	if (h+target.y > fBitmap->Bounds().bottom) {
		h = int(fBitmap->Bounds().bottom-target.y);
	}
	w++; h++;
	if ((w > 0) && (h > 0)) {
		/* figure out bytes per pixel -- padding gets rounded out */
		int bs = int(fBitmap->BytesPerRow()/(fBitmap->Bounds().Width()+1));
		int rb = fBitmap->BytesPerRow();
		uchar * dst = (uchar *)fBitmap->Bits()+(int)target.x*bs+(int)target.y*rb;
		bs = bs*w;
		int val = 255;
		if (fBitmap->ColorSpace() == B_CMAP8) {
			val = 63;
		}
		for (int ix=0; ix<h; ix++) {
			memset(dst, val, bs);
			dst += rb;
		}
		fr.Set(target.x, target.y, target.x+w-1, target.y+h-1);
		SetSelection(fr);
		delete fDithered;
		fDithered = NULL;
		Invalidate(fr*fZoom);
	}
}


void
BitmapView::DropFloater()
{
//	puts("DropFloater");
	if (fSelection.left > -1) {
		BBitmap * old_float = fFloater;
		BRect old_select = fSelection;
		if (old_float) {
//			puts("has old_float");
			delete fUndo;
			fUndo = new undo_info(this, true);
			int w = (int)old_select.Width();
			if (old_select.right > fBitmap->Bounds().right) {
				w = int(fBitmap->Bounds().right-old_select.left);
			}
			int h = (int)old_select.Height();
			if (old_select.bottom > fBitmap->Bounds().bottom) {
				h = int(fBitmap->Bounds().bottom-old_select.top);
			}
			w++; h++;
			if ((w > 0) && (h > 0)) {
				/* figure out bytes per pixel -- padding gets rounded out */
				int bs = int(old_float->BytesPerRow()/(old_float->Bounds().Width()+1));
				int rb = fBitmap->BytesPerRow();
				uchar * dst = (uchar *)fBitmap->Bits()+(int)old_select.left*bs+(int)old_select.top*rb;
				int rb2 = old_float->BytesPerRow();
				uchar * src = (uchar *)old_float->Bits();
				bs = bs*w;
				for (int ix=0; ix<h; ix++) {
					memcpy(dst, src, bs);
					dst += rb;
					src += rb2;
				}
			}
			delete fDithered;
			fDithered = NULL;
			BMessage msg(msg_SetDirty);
			msg.AddBool("dirty", true);
			Window()->PostMessage(&msg);
		}
		else {
			fSelection.Set(-1,-1,-2,-2);
		}
		Invalidate(old_select*fZoom);
	}
}


bool
BitmapView::HasSelection()
{
	if (IsSlicing())
		return fSlices.find(fActiveSlice) != fSlices.end();
	return (fSelection.left > -1);
}

bool
BitmapView::CanUndo()
{
	return fUndo != NULL;
}

bool
BitmapView::CanPaste()
{
	if (!be_clipboard->Lock()) return false;
	bool ok = false;
	if (IsSlicing()) {
		ok = be_clipboard->Data()->HasRect("data");
	} else {
		ok = validate_instantiation(be_clipboard->Data(), "BBitmap");
	}
	be_clipboard->Unlock();
	return ok;
}

void
BitmapView::Undo()
{
	InvalidateSelection(fSelection);
	undo_info * redo = new undo_info(this, fUndo && (fUndo->old_bits > -1));
	fUndo->apply(this);
	delete fUndo;
	fUndo = redo;
	delete fDithered;
	fDithered = NULL;
	InvalidateSelection(fSelection);
}

void
BitmapView::EditMunge(
	bool undo, 
	bool clip,
	bool clear)
{
	ASSERT(undo == clear);
	BRect r;
	map<BString,BRect>::iterator slice = fSlices.end();
	if (IsSlicing()) {
		slice = fSlices.find(fActiveSlice);
		if (slice != fSlices.end())
			r = slice->second;
		else
			r = BRect(-1, -1, -2, -2);
	} else {
		r = fSelection;
	}
	if (r.left < 0) return;
	
	if (clip) {
		if (!be_clipboard->Lock()) return;
		be_clipboard->Clear();
	}
	if (undo) delete fUndo;
	
	if (IsSlicing()) {
		if (slice != fSlices.end()) {
			if (clip) {
				be_clipboard->Data()->AddRect("data", slice->second);
				be_clipboard->Data()->AddString("name", slice->first);
			}
			if (clear) {
				RemoveSlice(slice->first.String());
				slice = fSlices.end();
			}
		}
	} else {
		if (fFloater) {
			if (clip) {
//				puts("pushing floater");
				fFloater->Lock();
				fFloater->Archive(be_clipboard->Data(), true);
				fFloater->Unlock();
			}
			if (undo) fUndo = new undo_info(this, false);
		}
		else {
			if (undo) fUndo = new undo_info(this, true);
			BRect fr(r);
			fr.OffsetTo(B_ORIGIN);
			BBitmap* n = NULL;
			int bprs = fBitmap->BytesPerRow();
			int bpp = bprs/(int)(fBitmap->Bounds().right+1);
			uchar * src = ((uchar *)fBitmap->Bits())+bpp*(int)r.left+bprs*(int)r.top;
			uchar* dst = NULL;
			int bprd = 0;
			if (clip) {
				n = new BBitmap(fr, fBitmap->ColorSpace());
				dst = (uchar *)n->Bits();
				bprd = n->BytesPerRow();
			}
			int h = (int)r.Height()+1;
			int wp = bpp*(int)(r.Width()+1);
			int byte = 0xff;
			if (fBitmap->ColorSpace() == B_CMAP8) {
				byte = 0x3f;
			}
			for (int ix=0; ix<h; ix++) {
				if (clip) memcpy(dst, src, wp);
				if (clear) memset(src, byte, wp);
				if (clip) dst += bprd;
				src += bprs;
			}
			if (clip) {
//				puts("pushing copy");
				n->Archive(be_clipboard->Data());
				delete n;
			}
			if (clear) {
				delete fDithered;
				fDithered = NULL;
			}
		}
	}
	if (clip) {
		if (!IsSlicing())
			be_clipboard->Data()->AddPoint("be:location", r.LeftTop());
		be_clipboard->Data()->what = B_ARCHIVED_OBJECT;
		be_clipboard->Commit();
		be_clipboard->Unlock();
	}
	if (clear && !IsSlicing()) {
		BMessage msg(msg_SetDirty);
		msg.AddBool("dirty", true);
		Window()->PostMessage(&msg);
		Invalidate(zoom_sel_rect(r,fZoom));
		Window()->UpdateIfNeeded();
	}
}

void
BitmapView::Cut()
{
	EditMunge(true, true, true);
}

void
BitmapView::Copy()
{
	EditMunge(false, true, false);
}

void
BitmapView::Paste()
{
	if (!be_clipboard->Lock()) return;
	if (IsSlicing()) {
		BRect r;
		if (be_clipboard->Data()->FindRect("data", &r) != B_OK)
			return;
		const char* n;
		if (be_clipboard->Data()->FindString("name", &n) != B_OK)
			n = NULL;
		AddSlice(n, r, true);
	} else {
		if (!validate_instantiation(be_clipboard->Data(), "BBitmap")) {
			return;
		}
		BPoint location = fSelection.LeftTop();
		if (location.x < 0) be_clipboard->Data()->FindPoint("be:location", &location);
		if (fFloater) {
			DropFloater();
		}
		else {
			delete fUndo;
			fUndo = new undo_info(this, false);
		}
		fFloater = dynamic_cast<BBitmap *>(instantiate_object(be_clipboard->Data()));
		if (fFloater) {
			fSelection = fFloater->Bounds();
			fSelection.OffsetBy(location);
			fSelection = fSelection & fBitmap->Bounds();
		}
		InvalidateSelection(fSelection);
	}
	be_clipboard->Unlock();
	Window()->UpdateIfNeeded();
}

void
BitmapView::Clear()
{
	EditMunge(true, false, true);
}

void
BitmapView::SelectAll()
{
	if (!IsSlicing()) {
		if (fFloater) {
			DropFloater();
		}
		fSelection = fBitmap->Bounds();
		InvalidateSelection(fSelection);
		Window()->UpdateIfNeeded();
	}
}


void
BitmapView::ChooseCursor(
	int32		id)
{
	const BCursor* curs;
	if (id < 0) curs = B_CURSOR_SYSTEM_DEFAULT;
	else curs = Resources().FindCursor(id);
	if (!curs) curs = B_CURSOR_SYSTEM_DEFAULT;
	if (curs != fCurCursor) {
		fCurCursor = curs;
		SetViewCursor(curs);
	}
}

BitmapView::undo_info::undo_info(BitmapView * view, bool save_bits)
{
	floater = view->fFloater;
	selection = view->fSelection;
	view->fFloater = NULL;
	view->fSelection.Set(-1,-1,-2,-2);
	old_bits = -1;
	char str[100];
	sprintf(str, "/tmp/.undo.%d.%d", (int)find_thread(NULL), (int)this);
	if (save_bits) {
		old_bits = open(str, O_RDWR | O_CREAT | O_EXCL);
	}
	if (old_bits >= 0) {
		unlink(str);	/* so we know it'll go away when we close it */
		int bpp = (int)(view->fBitmap->BytesPerRow()/(view->fBitmap->Bounds().Width()+1));
		int w = (int)selection.Width()+1;
		int h = (int)selection.Height()+1;
		int bpr = view->fBitmap->BytesPerRow();
		uchar * base = ((uchar *)view->fBitmap->Bits())+(int)selection.left*bpp+
			(int)selection.top*bpr;
		if (ftruncate(old_bits, w*h*bpp+4*sizeof(int)) < 0) {
			close(old_bits);
			old_bits = -1;
		}
		else {
			int undo = 'undo';
			lseek(old_bits, 0, 0);
			write(old_bits, &bpp, sizeof(bpp));
			write(old_bits, &w, sizeof(w));
			write(old_bits, &h, sizeof(h));
			write(old_bits, &undo, sizeof(undo));
			for (int ix=0; ix<h; ix++) {
				write(old_bits, base, w*bpp);
				base += bpr;
			}
		}
	}
}

BitmapView::undo_info::~undo_info()
{
	delete floater;
	close(old_bits);
}

void
BitmapView::undo_info::apply(BitmapView * view)
{
	view->fFloater = floater;
	view->fSelection = selection;
	floater = NULL;
	if (old_bits > -1) {
		lseek(old_bits, 0, 0);
		int undo = 0, bpp, w, h;
		read(old_bits, &bpp, sizeof(bpp));
		read(old_bits, &w, sizeof(w));
		read(old_bits, &h, sizeof(h));
		read(old_bits, &undo, sizeof(undo));	/* put undo here to detect problems in header */
		if (undo == 'undo') {
			int bpr = view->fBitmap->BytesPerRow();
			uchar * base = ((uchar *)view->fBitmap->Bits())+(int)selection.left*bpp+
				(int)selection.top*bpr;
			for (int ix=0; ix<h; ix++) {
				read(old_bits, base, w*bpp);
				base += bpr;
			}
		}
	}
}
