//****************************************************************************************
//
//	File:		TabView.cpp
//
//	Written by:	Robert Chinn
//
//	Copyright 1997, Be Incorporated
//
//****************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define S_TAB_LOCATION			"_tab_location"	// !! move to archive_defs
#include <InterfaceDefs.h>
#include <archive_defs.h>
#include <interface_misc.h>

#include <Window.h>
#include <Button.h>
#include <TabView.h>


#define _ENABLE_BEVELS_ 0		// draws fancy border
#define _SIMPLE_BORDER_ 1		// draws simple, original border
#define _MY_DEBUG_ 0
#define ENABLE_TAB_LOCATION 0


namespace BPrivate
{
rgb_color kLabelColor;
rgb_color kLowColor;
rgb_color kHighColor;

const char* const kUntitledTabName = "Untitled Tab";
const int32 kDefaultTabWidth = 100;
const int32 kTabLabelPadding = 20;

enum tab_location {								// !! rename to B 
	tabs_on_top,
	tabs_on_bottom,
	tabs_on_right,
	tabs_on_left
};

static inline rgb_color DisabledColor(rgb_color c) {
	return c.disable(ui_color(B_PANEL_BACKGROUND_COLOR));
}

} using namespace BPrivate;


/*------------------------------------------------------------*/

BTab::BTab(BView* contents)
	: BArchivable()
{
	fEnabled = true;
	fSelected = false;
	fFocus = false;
	if (contents)
		fView = contents;
	else
		fView = NULL;
}

BTab::~BTab()
{
	if (fView) {
		if (fSelected)
			fView->RemoveSelf();
		
		if (fView) 
			delete fView;
	}
}

BTab::BTab(BMessage* data)
	: BArchivable(data)
{
	bool b=false;
	if (data->HasBool(S_DISABLED)) {
		data->FindBool(S_DISABLED, &b);
		SetEnabled(!b);
	} else
		SetEnabled(true);

	fSelected = false;
	fFocus = false;
	fView = NULL;				//	the view will be reattached when the tabview
								//	is unarchived, there are two lists archived
								//	from a tabview list of tabs, list of views
}

BArchivable*
BTab::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data, "BTab"))
		return NULL;

	return new BTab(data);
}

status_t
BTab::Archive(BMessage* data, bool deep) const
{
	BArchivable::Archive(data,deep);

	if (!fEnabled)
		data->AddBool(S_DISABLED, TRUE);
		
	return B_OK;
}

status_t
BTab::Perform(uint32 d, void *arg)
{
	return BArchivable::Perform(d, arg);
}

const char*
BTab::Label() const
{
	if (fView)
		return fView->Name();
	else
		return kUntitledTabName;
}

void
BTab::SetLabel(const char* label)
{
	if (!label || !fView)
		return;
	
	if (fView)
		fView->SetName(label);		
}

bool
BTab::IsSelected() const
{
	return fSelected;
}

void
BTab::Select(BView* owner)
{
	if (!owner)
		return;
		
	BView* tabContents = View();
	// 	tabContents is the view to be shown in this tab
	//	owner is the tabview that owns this tab
	if (tabContents) {
		if (!owner->Window())
			return;
		
		owner->AddChild(tabContents);
		tabContents->Show();
		fSelected = true;
	}
}

void
BTab::Deselect()
{
	if (!fSelected)
		return;
		
	BView* tabContents = View();
	if (tabContents) {
		tabContents->RemoveSelf();
		fSelected = false;
	}
}

void
BTab::SetEnabled(bool on)
{
	fEnabled = on;
}

bool
BTab::IsEnabled() const
{
	return fEnabled;
}
		
void
BTab::MakeFocus(bool infocus)
{
	fFocus = infocus;
}

bool
BTab::IsFocus() const
{
	return fFocus;
}

void
BTab::SetView(BView* v)
{
	if (!v)
		return;
		
	if (v == fView)
		return;
		
	if (fView) {
		fView->RemoveSelf();
		delete fView;
	}
	
	fView = v;
}

BView*
BTab::View() const
{
	return fView;
}

void
BTab::DrawFocusMark(BView* owner, BRect frame)
{
	owner->PushState();

#if ENABLE_TAB_LOCATION
	tab_location tabLocation = (dynamic_cast<BTabView*>(owner))->TabLocation();
#else
	tab_location tabLocation = tabs_on_top;
#endif
		
	if (IsFocus() && (owner->Window())->IsActive())
		owner->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	else
		owner->SetHighColor(owner->ViewColor());

	owner->SetLowColor(owner->ViewColor());
		
	if (tabLocation == tabs_on_left )
		owner->StrokeLine(	BPoint(frame.right-3,frame.top + 10),
							BPoint(frame.right-3,frame.bottom - 10));
	else if (tabLocation == tabs_on_right) 
		owner->StrokeLine(	BPoint(frame.left+3,frame.top + 10),
							BPoint(frame.left+3,frame.bottom - 10));
	else if (tabLocation == tabs_on_bottom)
		owner->StrokeLine(BPoint(frame.left+10,frame.top + 3),
			BPoint(frame.right-10,frame.top + 3));
	else
		owner->StrokeLine(BPoint(frame.left+10,frame.bottom - 3),
			BPoint(frame.right-10,frame.bottom - 3));

	owner->PopState();
}

void
BTab::DrawLabel(BView* owner, BRect frame)
{
	owner->PushState();
	const char* tab_label = Label();
	if (tab_label == NULL || strlen(tab_label) <= 0)
		return;
		
	font_height finfo;
	if (IsEnabled())	owner->SetHighColor(kLabelColor);
	else				owner->SetHighColor(DisabledColor(kLabelColor));
	owner->SetLowColor(owner->ViewColor());
		
#if ENABLE_TAB_LOCATION
	tab_location tabLocation = (dynamic_cast<BTabView*>(owner))->TabLocation();
#else
	tab_location tabLocation = tabs_on_top;
#endif
	if (tabLocation == tabs_on_left || tabLocation == tabs_on_right) {
		BFont font;
		owner->GetFont(&font);
		
		if (tabLocation == tabs_on_left)
			font.SetRotation(90.0);
		else
			font.SetRotation(270.0);
			
		owner->SetFont(&font);
		
		font.GetHeight(&finfo);
		float y = frame.Height()/2 - (owner->StringWidth(tab_label)/2);
		float x = frame.Width()/2 - (finfo.ascent + finfo.descent)/2;
		
		// drawstring draws from the baseline
		if (tabLocation == tabs_on_left)
			owner->DrawString(tab_label, BPoint(frame.right - x - 3, frame.bottom - y));
		else
			owner->DrawString(tab_label, BPoint(frame.left + x + 5, frame.top + y));
	} else {
		owner->GetFontHeight(&finfo);
		float fontheight = (finfo.ascent + finfo.descent) / 2.0;
		float titleCenter = owner->StringWidth(tab_label) / 2.0;
		float tabHCenter = frame.Width() / 2.0;
		float tabVCenter = frame.Height() / 2.0;			
		
		if (tabLocation == tabs_on_top)
			owner->MovePenTo(BPoint(frame.left - (titleCenter - tabHCenter),
									frame.top + tabVCenter + fontheight - 3));
		else
			owner->MovePenTo(BPoint(frame.left - (titleCenter - tabHCenter),
									frame.top + tabVCenter + fontheight - 2));

		owner->DrawString(tab_label);
	}
		
	owner->PopState();
}

//	Notes for tab drawing
//		the tab frame is for each tab is butted end to end
//		to draw the overlap, the tab frame is expanded with respect
//			to its orientation
//		position is its ordering
//		full is used to determine whether to complete the tail of the tab
//		this method of drawing is quite obtuse, who the hell designed this?

//	draw tabs from bottom to top, first to last, respectively
static void
DrawLeftTab(BView *v, BRect r, tab_position p, bool full)
{
	r.top -= 3; r.bottom += 3;
	v->BeginLineArray(17);
	
	// erase baseline
	if (p == B_TAB_FRONT) {
		v->AddLine(BPoint(r.right, r.top), BPoint(r.right, r.bottom), v->ViewColor());
		v->AddLine(BPoint(r.right-1, r.top+4), BPoint(r.right-1, r.bottom-4), v->ViewColor());
	} else
		v->AddLine(BPoint(r.right-1, r.top+4), BPoint(r.right-1, r.top+4), v->ViewColor());
	
	// bottom, right curve
	if (p != B_TAB_ANY)	{	// first or selected tab
		v->AddLine(BPoint(r.right, r.bottom), BPoint(r.right, r.bottom-1), kLowColor);
		v->AddLine(BPoint(r.right-1, r.bottom-2), BPoint(r.right-1, r.bottom-2), kLowColor);
		v->AddLine(BPoint(r.right-2, r.bottom-3), BPoint(r.right-3, r.bottom-3), kLowColor);
	}
	
	// bottom wall
	v->AddLine(BPoint(r.right-4, r.bottom-4), BPoint(r.left+4, r.bottom-4), kLowColor);
	
	// bottom, left curve
	v->AddLine(BPoint(r.left+3, r.bottom-5), BPoint(r.left+2, r.bottom-5), kLowColor);
	v->AddLine(BPoint(r.left+1, r.bottom-6), BPoint(r.left+1, r.bottom-7), kLowColor);
	
	// left wall
	v->AddLine(BPoint(r.left, r.top+8), BPoint(r.left, r.bottom-8), kLowColor);
	
	// top, left curve
	v->AddLine(BPoint(r.left+1, r.top+7), BPoint(r.left+1, r.top+6), kLowColor);
	v->AddLine(BPoint(r.left+3, r.top+5), BPoint(r.left+2, r.top+5), kLowColor);
	
	// top wall
	v->AddLine(BPoint(r.left+4, r.top+4), BPoint(r.right-4, r.top+4), kLowColor);
	
	// top, right curve
	v->AddLine(BPoint(r.right-2, r.top+3), BPoint(r.right-3, r.top+3), kLowColor);
	if (full) {	// tab is selected or last
		v->AddLine(BPoint(r.right-1, r.top+2), BPoint(r.right-1, r.top+2), kLowColor);
		v->AddLine(BPoint(r.right, r.top+1), BPoint(r.right, r.top), kLowColor);
	}

	v->EndLineArray();
}

// 	draw tabs from top to bottom, first to last, respectively
static void 
DrawRightTab(BView *v, BRect r, tab_position p, bool full)
{
	r.top -= 3; r.bottom += 3;
	v->BeginLineArray(15);
	
	// erase baseline
	if (p == B_TAB_FRONT) {
		v->AddLine(BPoint(r.left, r.top), BPoint(r.left, r.bottom), v->ViewColor());
		v->AddLine(BPoint(r.left+1, r.top+4), BPoint(r.left+1, r.bottom-4), v->ViewColor());
	} else
		v->AddLine(BPoint(r.left+1, r.bottom-4), BPoint(r.left+1, r.bottom-4), v->ViewColor());
	
	// top, left curve
	if (p != B_TAB_ANY)	{	// first or selected tab
		v->AddLine(BPoint(r.left, r.top), BPoint(r.left, r.top+1), kLowColor);
		v->AddLine(BPoint(r.left+1, r.top+2), BPoint(r.left+1, r.top+2), kLowColor);
		v->AddLine(BPoint(r.left+2, r.top+3), BPoint(r.left+3, r.top+3), kLowColor);
	}
	
	// top wall
	v->AddLine(BPoint(r.left+4, r.top+4), BPoint(r.right-4, r.top+4), kLowColor);
	
	// top, right curve
	v->AddLine(BPoint(r.right-3, r.top+5), BPoint(r.right-2, r.top+5), kLowColor);
	v->AddLine(BPoint(r.right-1, r.top+6), BPoint(r.right-1, r.top+7), kLowColor);
	
	// right wall
	v->AddLine(BPoint(r.right, r.top+8), BPoint(r.right, r.bottom-8), kLowColor);
	
	// bottom, right curve
	v->AddLine(BPoint(r.right-1, r.bottom-7), BPoint(r.right-1, r.bottom-6), kLowColor);
	v->AddLine(BPoint(r.right-3, r.bottom-5), BPoint(r.right-2, r.bottom-5), kLowColor);
	
	// bottom wall
	v->AddLine(BPoint(r.right-4, r.bottom-4), BPoint(r.left+4, r.bottom-4), kLowColor);
	
	// bottom, left curve
	v->AddLine(BPoint(r.left+2, r.bottom-3), BPoint(r.left+3, r.bottom-3), kLowColor);
	if (full) {	// tab is selected or last
		v->AddLine(BPoint(r.left+1, r.bottom-2), BPoint(r.left+1, r.bottom-2), kLowColor);
		v->AddLine(BPoint(r.left, r.bottom-1), BPoint(r.left, r.bottom), kLowColor);
	}

	v->EndLineArray();
}

//	draw tabs left to right
static void 
DrawBottomTab(BView *v, BRect r, tab_position p, bool full)
{
	r.left -= 3; r.right += 3;
	v->BeginLineArray(17);
	
	// erase baseline
	if (p == B_TAB_FRONT) {
		v->AddLine(BPoint(r.left, r.top), BPoint(r.right, r.top), v->ViewColor());
		v->AddLine(BPoint(r.left+4, r.top+1), BPoint(r.right-4, r.top+1), v->ViewColor());
	} else
		v->AddLine(BPoint(r.left+4, r.top+1), BPoint(r.left+4, r.top+1), v->ViewColor());

	// top, left curve
	if (full) {				
		v->AddLine(BPoint(r.left, r.top), BPoint(r.left+1, r.top), kLowColor);
		v->AddLine(BPoint(r.left+2, r.top+1), BPoint(r.left+2, r.top+1), kLowColor);
		v->AddLine(BPoint(r.left+3, r.top+2), BPoint(r.left+3, r.top+3), kLowColor);
	}
	
	// left wall
	v->AddLine(BPoint(r.left+4, r.top+4), BPoint(r.left+4, r.bottom-4), kLowColor);
	
	// bottom, left curve
	v->AddLine(BPoint(r.left+5, r.bottom-3), BPoint(r.left+5, r.bottom-2), kLowColor);
	v->AddLine(BPoint(r.left+6, r.bottom-1), BPoint(r.left+7, r.bottom-1), kLowColor);
	
	// bottom
	v->AddLine(BPoint(r.left+8, r.bottom), BPoint(r.right-8, r.bottom), kLowColor);
	
	// bottom, right curve
	v->AddLine(BPoint(r.right-6, r.bottom-1), BPoint(r.right-7, r.bottom-1), kLowColor);
	v->AddLine(BPoint(r.right-5, r.bottom-3), BPoint(r.right-5, r.bottom-2), kLowColor);
	
	// right wall
	v->AddLine(BPoint(r.right-4, r.top+4), BPoint(r.right-4, r.bottom-4), kLowColor);
	
	// top right curve
	v->AddLine(BPoint(r.right-3, r.top+2), BPoint(r.right-3, r.top+3), kLowColor);
	if (p != B_TAB_ANY) {
		v->AddLine(BPoint(r.right-2, r.top+1), BPoint(r.right-2, r.top+1), kLowColor);
		v->AddLine(BPoint(r.right, r.top), BPoint(r.right-1, r.top), kLowColor);
	}

	v->EndLineArray();
}

static void
DrawTopTab(BView *v, BRect r, tab_position p, bool full)
{
	r.left -= 3; r.right += 3;
	v->BeginLineArray(39);
	
	// erase baseline
	if (p == B_TAB_FRONT) {
		v->AddLine(BPoint(r.left, r.bottom), BPoint(r.right, r.bottom), v->ViewColor());
		v->AddLine(BPoint(r.left+4, r.bottom-1), BPoint(r.right-4, r.bottom-1), v->ViewColor());
	} else
		v->AddLine(BPoint(r.right-4, r.bottom-1), BPoint(r.right-4, r.bottom-1), v->ViewColor());

	// bottom, left curve
	if (p != B_TAB_ANY)	{	// first or selected tab
#if _SIMPLE_BORDER_
		v->AddLine(BPoint(r.left, r.bottom), BPoint(r.left+1, r.bottom), kHighColor);
#else
		v->AddLine(BPoint(r.left, r.bottom), BPoint(r.left+1, r.bottom), kLowColor);
#endif
#if _ENABLE_BEVELS_
		v->AddLine(BPoint(r.left+2, r.bottom), BPoint(r.left+2, r.bottom),
			shift_color(kLowColor, 0.25));
		v->AddLine(BPoint(r.left+3, r.bottom), BPoint(r.left+3, r.bottom), kHighColor);
#endif

#if _SIMPLE_BORDER_
		v->AddLine(BPoint(r.left+2, r.bottom-1), BPoint(r.left+2, r.bottom-1), kHighColor);
#else
		v->AddLine(BPoint(r.left+2, r.bottom-1), BPoint(r.left+2, r.bottom-1), kLowColor);
#endif
#if _ENABLE_BEVELS_
		v->AddLine(BPoint(r.left+3, r.bottom-1), BPoint(r.left+3, r.bottom-1),
			shift_color(kLowColor, 0.25));
		v->AddLine(BPoint(r.left+4, r.bottom+1), BPoint(r.left+4, r.bottom-1), kHighColor);
#endif

#if _SIMPLE_BORDER_
		v->AddLine(BPoint(r.left+3, r.bottom-2), BPoint(r.left+3, r.bottom-3), kHighColor);
#else
		v->AddLine(BPoint(r.left+3, r.bottom-2), BPoint(r.left+3, r.bottom-3), kLowColor);
#endif
#if _ENABLE_BEVELS_
		v->AddLine(BPoint(r.left+4, r.bottom-2), BPoint(r.left+4, r.bottom-3),
			shift_color(kLowColor, 0.25));
		v->AddLine(BPoint(r.left+5, r.bottom-2), BPoint(r.left+5, r.bottom-3), kHighColor);
#endif
	}
	
	// left wall
#if _SIMPLE_BORDER_
	v->AddLine(BPoint(r.left+4, r.bottom-4), BPoint(r.left+4, r.top+4), kHighColor);
#else
	v->AddLine(BPoint(r.left+4, r.bottom-4), BPoint(r.left+4, r.top+4), kLowColor);
#endif
#if _ENABLE_BEVELS_
	v->AddLine(BPoint(r.left+5, r.bottom-4), BPoint(r.left+5, r.top+4),
		shift_color(kLowColor, 0.25));
	v->AddLine(BPoint(r.left+6, r.bottom-4), BPoint(r.left+6, r.top+4), kHighColor);
#endif
	
	// top, left curve
#if _SIMPLE_BORDER_
	v->AddLine(BPoint(r.left+5, r.top+3), BPoint(r.left+5, r.top+2), kHighColor);
#else
	v->AddLine(BPoint(r.left+5, r.top+3), BPoint(r.left+5, r.top+2), kLowColor);
#endif
#if _ENABLE_BEVELS_
	v->AddLine(BPoint(r.left+6, r.top+3), BPoint(r.left+6, r.top+2),
		shift_color(kLowColor, 0.25));
#endif
#if _SIMPLE_BORDER_
	v->AddLine(BPoint(r.left+6, r.top+1), BPoint(r.left+7, r.top+1), kHighColor);
#else
	v->AddLine(BPoint(r.left+6, r.top+1), BPoint(r.left+7, r.top+1), kLowColor);
#endif
#if _ENABLE_BEVELS_
	v->AddLine(BPoint(r.left+6, r.top+2), BPoint(r.left+7, r.top+2),
		shift_color(kLowColor, 0.25));
	v->AddLine(BPoint(r.left+7, r.top+3), BPoint(r.left+7, r.top+3), kHighColor);
#endif
	
	// top
#if _SIMPLE_BORDER_
	v->AddLine(BPoint(r.left+8, r.top), BPoint(r.right-8, r.top), kHighColor);
#else
	v->AddLine(BPoint(r.left+8, r.top), BPoint(r.right-8, r.top), kLowColor);
#endif
#if _ENABLE_BEVELS_
	v->AddLine(BPoint(r.left+8, r.top+1), BPoint(r.right-8, r.top+1),
		shift_color(kLowColor, 0.25));
	v->AddLine(BPoint(r.left+8, r.top+2), BPoint(r.right-8, r.top+2), kHighColor);
#endif
	
	// top, right curve
#if _SIMPLE_BORDER_
	v->AddLine(BPoint(r.right-6, r.top+1), BPoint(r.right-7, r.top+1), kHighColor);
#else
	v->AddLine(BPoint(r.right-6, r.top+1), BPoint(r.right-7, r.top+1), kLowColor);
#endif
#if _ENABLE_BEVELS_
	v->AddLine(BPoint(r.right-6, r.top+2), BPoint(r.right-7, r.top+2),
		shift_color(kLowColor, 0.25));
#endif
	v->AddLine(BPoint(r.right-5, r.top+3), BPoint(r.right-5, r.top+2), kLowColor);
#if _ENABLE_BEVELS_
	v->AddLine(BPoint(r.right-6, r.top+3), BPoint(r.right-6, r.top+3),
		shift_color(kLowColor, 0.75));
	v->AddLine(BPoint(r.right-7, r.top+3), BPoint(r.right-7, r.top+3),
		shift_color(kLowColor, 0.5));
#endif	
	// right wall
	v->AddLine(BPoint(r.right-4, r.top+4), BPoint(r.right-4, r.bottom-4), kLowColor);
#if _ENABLE_BEVELS_
	v->AddLine(BPoint(r.right-5, r.top+4), BPoint(r.right-5, r.bottom-4),
		shift_color(kLowColor, 0.75));
	v->AddLine(BPoint(r.right-6, r.top+4), BPoint(r.right-6, r.bottom-4),
		shift_color(kLowColor, 0.5));
#endif
	
	// bottom, right curve
	v->AddLine(BPoint(r.right-3, r.bottom-2), BPoint(r.right-3, r.bottom-3), kLowColor);
#if _ENABLE_BEVELS_
	v->AddLine(BPoint(r.right-4, r.bottom-2), BPoint(r.right-4, r.bottom-3),
		shift_color(kLowColor, 0.75));
	v->AddLine(BPoint(r.right-5, r.bottom-2), BPoint(r.right-5, r.bottom-3),
		shift_color(kLowColor, 0.5));
#endif

	if (full) {	// tab is selected or last
		v->AddLine(BPoint(r.right-2, r.bottom-1), BPoint(r.right-2, r.bottom-1), kLowColor);
#if _ENABLE_BEVELS_
		v->AddLine(BPoint(r.right-3, r.bottom-1), BPoint(r.right-3, r.bottom-1),
			shift_color(kLowColor, 0.75));
		v->AddLine(BPoint(r.right-4, r.bottom-1), BPoint(r.right-4, r.bottom-1),
			shift_color(kLowColor, 0.5));
#endif
	
		v->AddLine(BPoint(r.right, r.bottom), BPoint(r.right-1, r.bottom), kLowColor);
#if _ENABLE_BEVELS_
		v->AddLine(BPoint(r.right-2, r.bottom), BPoint(r.right-2, r.bottom),
			shift_color(kLowColor, 0.75));
		v->AddLine(BPoint(r.right-3, r.bottom), BPoint(r.right-3, r.bottom),
			shift_color(kLowColor, 0.5));
#endif
	}

	v->EndLineArray();
}

void
BTab::DrawTab(BView *owner, BRect frame, tab_position position, bool full)
{
	if (!owner)
		return;
		
	owner->PushState();
	owner->SetPenSize(1.0);	
	owner->SetHighColor(kLabelColor);
	owner->SetLowColor(owner->ViewColor());
	
#if ENABLE_TAB_LOCATION
	tab_location tabLocation = (dynamic_cast<BTabView*>(owner))->TabLocation();
#else
	tab_location tabLocation = tabs_on_top;
#endif
	if (tabLocation == tabs_on_top)				DrawTopTab(owner, frame, position, full);
	else if (tabLocation == tabs_on_bottom)		DrawBottomTab(owner, frame, position, full);
	else if (tabLocation == tabs_on_left)		DrawLeftTab(owner, frame, position, full);
	else if (tabLocation == tabs_on_right)		DrawRightTab(owner, frame, position, full);
	
	owner->PopState();
	DrawLabel(owner, frame);
	DrawFocusMark(owner,frame);
}

void BTab::_ReservedTab1() {}
void BTab::_ReservedTab2() {}
void BTab::_ReservedTab3() {}
void BTab::_ReservedTab4() {}
void BTab::_ReservedTab5() {}
void BTab::_ReservedTab6() {}
void BTab::_ReservedTab7() {}
void BTab::_ReservedTab8() {}
void BTab::_ReservedTab9() {}
void BTab::_ReservedTab10() {}
void BTab::_ReservedTab11() {}
void BTab::_ReservedTab12() {}

BTab &BTab::operator=(const BTab &) {return *this;}

/*------------------------------------------------------------*/

BTabView::BTabView(BRect frame, const char *name,
	button_width width,
	uint32 resizingMode, uint32 flags)
	: BView( frame, name, resizingMode, flags)
{
	fTabWidthSetting = width;	
#if ENABLE_TAB_LOCATION
	fTabLocation = tabs_on_top;
#endif
	_InitObject();
}

#if ENABLE_TAB_LOCATION
BTabView::BTabView(BRect frame, const char *name,
	tab_location location, button_width width, 
	uint32 resizingMode, uint32 flags)
	: BView( frame, name, resizingMode, flags)
{
	fTabWidthSetting = width;
	fTabLocation = location;
	_InitObject();
}
#endif

//	correctly places the frame of the view container relative to the
//		tabview and the tab location
static BRect
GetContainerViewFrame(BRect bounds, tab_location where, float tabHeight)
{
	BRect newFrame(bounds);
	
	//	inset by 1 so that the 1 pixel border of TabView is still visible
	newFrame.InsetBy(1, 1);
	if (where == tabs_on_top)
		newFrame.top += tabHeight; // MA + 1;
	else if (where == tabs_on_bottom)
		newFrame.bottom -= tabHeight;
	else if (where == tabs_on_left)
		newFrame.left += tabHeight + 1;
	else if (where == tabs_on_right)
		newFrame.right -= tabHeight;

	return newFrame;
}

void
BTabView::_InitObject()
{
	kLowColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_4_TINT);
	kHighColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_MAX_TINT);
	kLabelColor = ui_color(B_MENU_ITEM_TEXT_COLOR);

	fTabList = new BList();
	fTabWidth = 0;
	fSelection = 0;
	fFocus = -1;
	
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);

	//	set the tab height relative to the height of the view's font	
	font_height finfo;
	GetFontHeight(&finfo);
	fTabHeight = (finfo.ascent + finfo.descent + finfo.leading) + 8;

	// 	add the view that all subviews will be displayed within
	//	the bounds are inset to allow for the bevel drawing
#if ENABLE_TAB_LOCATION
	BRect vframe = GetContainerViewFrame(Bounds(), TabLocation(), TabHeight());
#else
	BRect vframe = GetContainerViewFrame(Bounds(), tabs_on_top, TabHeight());
#endif	
	fContainerView = new BView(vframe,"view container", B_FOLLOW_ALL, B_WILL_DRAW);
	fContainerView->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);	// changed from kGray
	fContainerView->SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	AddChild(fContainerView);
}

BTabView::~BTabView()
{
	BTab* tab=NULL;
	int32 count = CountTabs();

	while (count--) {		//changed to delete from the end, small performance gain
		// don't use RemoveTab here, it will attempt to call Draw
		tab = (BTab*)fTabList->ItemAt(count);
		delete tab;						// deletes view as well
		fTabList->RemoveItem(count);
	}
	
	delete fTabList;
}

BTabView::BTabView(BMessage* msg)
	: BView(msg)
{
	fContainerView = NULL;
	fTabList = new BList();

	int16 temp16=0;
	if (msg->FindInt16(S_BUTTON_WIDTH,&temp16) == B_OK)
		fTabWidthSetting = (button_width)temp16;
	else
		fTabWidthSetting = B_WIDTH_AS_USUAL;

	fTabWidth = 0;		// needs to be calculated against all tab names for width_from_widest
	if (msg->FindFloat(S_HEIGHT,&fTabHeight) != B_OK) {
		font_height finfo;
		GetFontHeight(&finfo);
		fTabHeight = (finfo.ascent + finfo.descent + finfo.leading) + 8;
	}	
	fFocus = -1;

	if (msg->FindInt32(S_SELECTED, &fSelection) != B_OK)
		fSelection = 0;
	
	//	reconnect the view container
	if (!fContainerView)
		fContainerView = ChildAt(0);

	//	unarchive the list of tabs and their corresponding views
	int32 i=0;
	BMessage archive;
	while(msg->FindMessage(S_LIST_ITEMS, i++, &archive) == B_OK) {
		BArchivable		*obj=NULL;
		BTab			*t=NULL;

		//	get a tab from the tab list
		obj = instantiate_object(&archive);

		if (!obj)
			continue;
		else
			t = dynamic_cast<BTab*>(obj);
		
		//	find the corresponding view  in the view list		
		BMessage subarchive;
		if (msg->FindMessage(S_VIEW_LIST, i-1, &subarchive) == B_OK) {

			obj = instantiate_object(&subarchive);
			if (!obj)
				continue;

			BView* v = dynamic_cast<BView*>(obj);			

			//	add the view to the tab
			if (v)
				AddTab(v,t);
		}
		
		archive.MakeEmpty();
	}
	
#if ENABLE_TAB_LOCATION
	int32 location;
	if (msg->FindInt32(S_TAB_LOCATION, &location) == B_OK)
		fTabLocation = (tab_location)location;
	else
		fTabLocation = tabs_on_top;	
#endif

/*	BFont f;
	f.SetSpacing(B_STRING_SPACING);
	SetFont(&f, B_FONT_SPACING); */
	
/*	uint32	mask = 0;
	BFont	aFont;

	if (!msg || !msg->HasString(S_FONT_FAMILY_STYLE))
		mask |= B_FONT_FAMILY_AND_STYLE;
	if (!msg || !msg->HasFloat(S_FONT_FLOATS))
		mask |= B_FONT_SIZE;

	if (mask != 0)
		SetFont(&aFont, mask);	*/
}

BArchivable*
BTabView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BTabView"))
		return NULL;
	return new BTabView(data);
}

status_t
BTabView::Archive(BMessage *data, bool deep) const
{
	// 	detach the current tabs contents so that they are not
	//	archived initially
	BTab* current_tab = TabAt(Selection());
	BView* current_v = current_tab->View();
	current_v->RemoveSelf();

	//	archive the view container
	BView::Archive(data, deep);
	
	//	archive the attributes of the tabview
	data->AddInt16(S_BUTTON_WIDTH, fTabWidthSetting);
	//	width is calculated from tabwidthsetting
	data->AddFloat(S_HEIGHT, fTabHeight);
	data->AddInt32(S_SELECTED, fSelection);

	//	manually archive all of the tab contents
	//	there are two lists of the contents created
	//		that are unique to tabview
	//		1 - the list of the tabs
	//		2 - the list of the tabs contents - the top level view
	//			is archived assuming that all subviews will be archived
	//			correctly
	if (deep) {
		int32	err=0;
		int32 	tabcount = CountTabs();

		for (int32 tabindex=0 ; tabindex<tabcount ; tabindex++) {
			BMessage	archive;
			BTab* tab = TabAt(tabindex);
		 	if (tab) {
				err = tab->Archive(&archive, true);
				if (!err)
					data->AddMessage(S_LIST_ITEMS, &archive);
											
				BView* v = tab->View();
				if (v) {
					BMessage	subarchive;

					err = v->Archive(&subarchive,true);
					if (!err)
						data->AddMessage(S_VIEW_LIST, &subarchive);
				} 
			} 
		}
		
	}

#if ENABLE_TAB_LOCATION
	data->AddInt32(S_TAB_LOCATION, fTabLocation);
#endif
	//
	//	reselect the current selection
	//
	BTab* tab = TabAt(Selection());
	if (tab) {
		BView* v = ContainerView();
		
		if (v)
			tab->Select(v);

	}

	return B_OK;
}

status_t
BTabView::Perform(perform_code d, void *arg)
{
	return BView::Perform(d,arg);
}

void
BTabView::WindowActivated(bool state)
{
	BView::WindowActivated(state);
	
	//	redraw and refresh things like the focus mark
	if (IsFocus())
	{
		Invalidate();
		Window()->UpdateIfNeeded();
	}
}

void
BTabView::AttachedToWindow()
{
	BView::AttachedToWindow();

	Select(fSelection);
}

void
BTabView::AllAttached()
{
	BView::AllAttached();
}

void
BTabView::AllDetached()
{
	BView::AllDetached();
}

void
BTabView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}

void
BTabView::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}

void
BTabView::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}

void
BTabView::FrameResized(float new_width, float new_height)
{
// 9/11/2000: This crashes because of an app_server bug
// we don't need this code, though
#if 0
	// if the frame changed size then we need to force an extra update
	// event to get the frame area to draw correctly.
	BRect	b = Bounds();

	// can't trust new_width and new_height because Update events are
	// handled out of turn. This means that the Update event corresponding
	// to this FrameResized event might have already been handled. This
	// gets the redraw code out of synch.

	new_width = b.Width();
	new_height = b.Height();

	if (new_width > fBounds.Width()) {
		// need to redraw the ALL the area between the new right side and the
		// previous location of the right border
		BRect	inval = b;
		inval.right -= 1;
		inval.left = fBounds.right - 1;
		Invalidate(inval);
	} else if (new_width < fBounds.Width()) {
		// need to force update in the area that contains the new right border
		BRect inval = b;
		inval.left = inval.right - 1;
		Invalidate(inval);
	}
	if (new_height > fBounds.Height()) {
		// need to redraw the ALL the area between the new bottom and the
		// previous location of the bottom border
		BRect	inval = b;
		inval.bottom -= 1;
		inval.top = fBounds.bottom - 1;
		Invalidate(inval);
	} else if (new_height < fBounds.Height()) {
		// need to redraw the area the should contain the new bottom border
		BRect inval = b;
		inval.top = inval.bottom - 1;
		Invalidate(inval);
	}

	fBounds = b;
#endif
	BView::FrameResized(new_width,new_height);
	//
	//	should all of the subviews be resized as well
	//	if so, the resize flags need to be checked
	//		and the views will need to be resized appropriately
}

//	how to handle tabs that are not enabled?
void 
BTabView::KeyDown(const char *bytes, int32 n)
{
	if (IsHidden())
		return;
		
	int32 sel=FocusTab();
	int32 tabCount = CountTabs();
#if ENABLE_TAB_LOCATION
	tab_location tabLocation = TabLocation();
#else
	tab_location tabLocation = tabs_on_top;
#endif

	switch (bytes[0]) {
		case B_UP_ARROW:
			if (tabLocation == tabs_on_left) {	
				sel++;
				if (sel >= tabCount) sel = 0;
				SetFocusTab(sel, true);
			} else if (tabLocation == tabs_on_right) {
				sel--;
				if (sel < 0) sel = tabCount-1;
				SetFocusTab(sel, true);
			}
			break;
		case B_DOWN_ARROW:
			if (tabLocation == tabs_on_left) {	
				sel--;
				if (sel < 0) sel = tabCount-1;
				SetFocusTab(sel, true);
			} else if (tabLocation == tabs_on_right) {
				sel++;
				if (sel >= tabCount) sel = 0;
				SetFocusTab(sel, true);
			}
			break;
		case B_LEFT_ARROW:
			if (tabLocation == tabs_on_top) {
				sel--;
				if (sel < 0) sel = tabCount-1;
				SetFocusTab(sel, true);
			} else if (tabLocation == tabs_on_bottom) {
				sel++;
				if (sel >= tabCount) sel = 0;
				SetFocusTab(sel, true);
			}
			break;
		case B_RIGHT_ARROW:
			if (tabLocation == tabs_on_top) {
				sel++;
				if (sel >= tabCount) sel = 0;
				SetFocusTab(sel, true);
			} else if (tabLocation == tabs_on_bottom) {
				sel--;
				if (sel < 0) sel = tabCount-1;
				SetFocusTab(sel, true);
			}
			break;
		case B_ENTER:
		case B_SPACE:
			if (Selection() != FocusTab())
				Select(FocusTab());
			break;
		default:
			BView::KeyDown(bytes, n);
			break;
	}
}

void
BTabView::MouseDown(BPoint where)
{
	BRect tabFrame;
	int32 tabCount = CountTabs();
	int32 selTab=-1;	
	//
	//	find the tab that was clicked in
	//	
	for (int32 i=0 ; i<tabCount ; i++) {
		tabFrame = TabFrame(i);
		if (tabFrame.Contains(where)) {
			selTab = i;
			break;
		}
	}
	
	if (selTab != Selection() && (selTab >= 0 && selTab < tabCount))
		Select(selTab);
	else
		BView::MouseDown(where);
}

void
BTabView::MouseUp(BPoint pt)
{
	BView::MouseUp(pt);
}

void
BTabView::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	BView::MouseMoved(pt, code, msg);
}

void
BTabView::Pulse()
{
	BView::Pulse();
}

void
BTabView::Select(int32 newSelection)
{
	if (newSelection < 0 || newSelection >= CountTabs())
		return;

	// deselect the old
	BTab* tab = TabAt(Selection());
	if (tab) {
		tab->Deselect();
	}
	
	// select the new
	tab = TabAt(newSelection);
	if (tab) {
		BView* v = ContainerView();
		
		if (v) {
			tab->Select(v);
			fSelection = newSelection;
		} 
	}

	if (LockLooper())
	{
		Invalidate();
		UnlockLooper();
	}
}

int32
BTabView::Selection() const
{
	return fSelection;
}

void
BTabView::MakeFocus(bool focusState)
{
	BView::MakeFocus(focusState);
	// 	tabview just got the focus,
	//	set the frontmost tab as the current focus
	SetFocusTab(Selection(), focusState);
}

void
BTabView::SetFocusTab(int32 f, bool focusState)
{
	if (f < 0 && f >= CountTabs())
		return;
		
	BTab* tab=NULL;
	if (focusState) {
		// update the tab if it is already focused
		if (f == FocusTab()) {
			tab = TabAt(FocusTab());
			if (tab) {
				tab->MakeFocus(true);
				tab->DrawFocusMark( this, TabFrame(FocusTab()));
			}
			return;
		} else {
			// 	set the focus false for the last tab
			//	if the tabview has not been focused, fFocus = -1
			tab = TabAt(FocusTab());
			if (tab) {
				tab->MakeFocus(false);
				tab->DrawFocusMark( this, TabFrame(FocusTab()));
			}						
			//	set the focus to the desired tab		
			fFocus = f;		
			tab = TabAt(FocusTab());
			if (tab) {
				tab->MakeFocus(true);
				tab->DrawFocusMark( this, TabFrame(FocusTab()));
			}
		}
	} else {
		// toggle the current tab's focus
		tab = TabAt(FocusTab());
		if (tab) {
			tab->MakeFocus(false);
			tab->DrawFocusMark( this, TabFrame(FocusTab()));
		}
	}
}


int32 BTabView::FocusTab() const
{
	return fFocus;
}

void BTabView::Draw(BRect)
{
	DrawBox(DrawTabs());
}

//	the tab row starts +3 from the left and extends -3 to the right
//	the drawing, to accomodate overlaps starts -3 from the first tabs left
//	start of tab is 6 pixels from start corner
BRect BTabView::TabFrame(int32 index) const
{
	// find the start and end point of the tab
	//	this is a left to right calculation
	float start=6, end=0;
	if (fTabWidthSetting==B_WIDTH_AS_USUAL) {			 // set width, no padding
		start += index * kDefaultTabWidth;
		end = start + kDefaultTabWidth;
	} else if (fTabWidthSetting==B_WIDTH_FROM_WIDEST) {	 // widest width plus padding
		start += index * fTabWidth;
		end = start + fTabWidth;
	} else if (fTabWidthSetting == B_WIDTH_FROM_LABEL) { // calculates each ind plus padding
		// width is relative to all tabs
		// must iterate from 1 to target to get the correct location		
		BTab*	t=NULL;
		end = start;
		for (int32 i=0 ; i<=index ; i++) {
			t = TabAt(i);
			if (t) {
				start = end;
				end += StringWidth((const char*)t->Label()) + kTabLabelPadding;
			}
		}
	}
	
	//	calculate the frame of the tab based on its location
	BRect frame;
	float tabHeight = TabHeight();
#if ENABLE_TAB_LOCATION
	tab_location tabLocation = TabLocation();
#else
	tab_location tabLocation = tabs_on_top;
#endif
	if (tabLocation == tabs_on_top) {				// tabs start at left, go right
		frame.left = start; frame.right = end;
		frame.top = 0; frame.bottom = tabHeight;
	} else if (tabLocation == tabs_on_bottom) {		// tabs start at right bottom, go left
		frame.bottom = Bounds().Height();
		frame.top = frame.bottom - tabHeight;
		frame.right = Bounds().Width() - start;
		frame.left = frame.right - (end - start);
	} else if (tabLocation == tabs_on_right) {		// tabs start at right top, go down
		frame.right = Bounds().Width();
		frame.left = frame.right - tabHeight; 
		frame.top = start; frame.bottom = end;
	} else if (tabLocation == tabs_on_left) {		// tabs start at left bottom, go up
		frame.left = 0; frame.right = tabHeight;
		frame.bottom = Bounds().Height() - start;
		frame.top = frame.bottom - (end - start);
	}
	
	return frame;
}

BRect
BTabView::DrawTabs()
{
	int32 pcount = CountTabs();
	int32 currSelection = Selection();
	BRect selFrame(Bounds()), frame;
	
	selFrame.bottom = selFrame.top + TabHeight();
		
	for (int32 index=0 ; index<pcount ; index++) {
		BTab* tab = TabAt(index);
		if (!tab)
			continue;
	
		frame = TabFrame(index);
		
		if (index==currSelection) {
			tab->DrawTab(this, frame, B_TAB_FRONT);
			selFrame = frame;
		} else if (index==0)
			tab->DrawTab(this, frame, B_TAB_FIRST, (index != (currSelection-1)));
		else 
			tab->DrawTab(this, frame, B_TAB_ANY, (index != (currSelection-1)));
	}
	
	return selFrame;
}

void
BTabView::DrawBox(BRect tabFrame)
{
#if ENABLE_TAB_LOCATION
	tab_location tabLocation = TabLocation();
#else
	tab_location tabLocation = tabs_on_top;
#endif

	BPoint topLeft, topRight, bottomLeft, bottomRight;

	//	expand the tab frame based on the orientation to accomodate overlap
	if (CountTabs() > 0) {
		if (tabLocation == tabs_on_top || tabLocation == tabs_on_bottom) {
			tabFrame.left -= 3; tabFrame.right += 3;
	 	} else {
			tabFrame.top -= 3; tabFrame.bottom += 3;
		}

		//	calculate the four corners based on the tab location
	
		topLeft.x = (tabLocation == tabs_on_left) ? tabFrame.Width() : 0;
		if (tabLocation == tabs_on_top)	topLeft.y = tabFrame.Height();
		else topLeft.y = 0;
	
		topRight.y = topLeft.y;
		if (tabLocation == tabs_on_right) topRight.x = Bounds().Width() - tabFrame.Width();
		else topRight.x = Bounds().Width();
	
		bottomRight.x = topRight.x;
		if (tabLocation == tabs_on_bottom) bottomRight.y = Bounds().Height() - tabFrame.Height();
		else bottomRight.y = Bounds().Height();
	
		bottomLeft.y = bottomRight.y;
		if (tabLocation == tabs_on_left) bottomLeft.x = tabFrame.Width();
		else bottomLeft.x = 0;
	} else {
		// make the top full left to right
		// needs to be expanded for tab location later on
		tabFrame.left = 0; tabFrame.right = 0;
		topLeft.x = 0; topLeft.y = tabFrame.Height();
		topRight.x = Bounds().Width(); topRight.y = topLeft.y;
		bottomRight.x = topRight.x; bottomRight.y = Bounds().Height();
		bottomLeft.y = bottomRight.y; bottomLeft.x = 0;
	}

#if _ENABLE_BEVELS_
	// erase the interior bevels
	PushState();
	BRect f(topLeft, bottomRight);
	f.InsetBy(1,1);
	SetHighColor(ViewColor());
	StrokeRect(f);
	f.InsetBy(1,1);
	StrokeRect(f);
	PopState();	
#endif
	
	//	draw the four sides, bevels included, plus tab baseline cleanup
	BeginLineArray(21);
	
	// 	right
	if (tabLocation == tabs_on_right) {
		AddLine( topRight, tabFrame.LeftTop(), kLowColor);
		AddLine( tabFrame.LeftBottom(), bottomRight, kLowColor);

#if _ENABLE_BEVELS_
		lineStart = topRight; lineStart.x--; lineStart.y++;
		lineEnd = tabFrame.LeftTop(); lineEnd.x--; lineEnd.y--;
		AddLine( lineStart, lineEnd, shift_color(kLowColor, 0.75));		
		lineStart.x--; lineStart.y++;
		lineEnd.x--; lineEnd.y--;
		AddLine( lineStart, lineEnd, shift_color(kLowColor, 0.5));		
#endif
	} else {
		AddLine( topRight , bottomRight, kLowColor);
#if _ENABLE_BEVELS_
		lineStart = topRight; lineStart.x--; lineStart.y += 2;
		lineEnd = bottomRight; lineEnd.x--; lineEnd.y--;
		AddLine( lineStart, lineEnd, shift_color(kLowColor, 0.75));		
		lineStart.x--; lineStart.y++;
		lineEnd.x--; lineEnd.y--;
		AddLine( lineStart, lineEnd, shift_color(kLowColor, 0.5));		
#endif
	}
	
	//	bottom
	if (tabLocation == tabs_on_bottom) {
		AddLine( bottomLeft, tabFrame.LeftTop(), kLowColor);
		AddLine( tabFrame.RightTop(), bottomRight, kLowColor);
	
#if _ENABLE_BEVELS_
		// left of tab
		lineStart = bottomLeft; lineStart.x--; lineStart.y--;
		lineEnd = tabFrame.LeftTop(); lineEnd.x--; lineStart.y--;
		AddLine( lineStart, lineEnd, shift_color(kLowColor, 0.75));		
		lineStart.x--; lineStart.y--;
		lineEnd.x--; lineStart.y--;
		AddLine( lineStart, lineEnd, shift_color(kLowColor, 0.5));		
		// right of tab
		lineStart = tabFrame.RightTop(); lineStart.x--; lineStart.y--;
		lineEnd = bottomRight; lineEnd.x--; lineStart.y--;
		AddLine( lineStart, lineEnd, shift_color(kLowColor, 0.75));		
		lineStart.x--; lineStart.y--;
		lineEnd.x--; lineStart.y--;
		AddLine( lineStart, lineEnd, shift_color(kLowColor, 0.5));		
#endif
	} else {
		AddLine( bottomLeft, bottomRight, kLowColor);
#if _ENABLE_BEVELS_
		lineStart = bottomLeft; lineStart.x += 2; lineStart.y--;
		lineEnd = bottomRight; lineEnd.x--; lineEnd.y--;
		AddLine( lineStart, lineEnd, shift_color(kLowColor, 0.75));		
		lineStart.x++; lineStart.y--;
		lineEnd.x--; lineEnd.y--;
		AddLine( lineStart, lineEnd, shift_color(kLowColor, 0.5));
#endif
	}
	
	// 	left
	if (tabLocation == tabs_on_left) {
		AddLine( bottomLeft, tabFrame.RightBottom(), kLowColor);
		AddLine( tabFrame.RightTop(), topLeft, kLowColor);

#if _ENABLE_BEVELS_
		lineStart = bottomLeft; lineStart.x += 2; lineStart.y += 2;
		lineEnd = tabFrame.RightBottom(); lineEnd.x += 2; lineEnd.y -= 2;
		AddLine( lineStart, lineEnd, kHighColor);
		lineStart = tabFrame.RightTop(); lineStart.x += 2; lineStart.y += 2;
		lineEnd = topLeft; lineEnd.x += 2; lineEnd.y -= 2;
		AddLine( lineStart, lineEnd, kHighColor);
#endif
	} else {
#if _SIMPLE_BORDER_
		if (tabLocation == tabs_on_top)
			AddLine( bottomLeft, topLeft, kHighColor);
		else	
			AddLine( bottomLeft, topLeft, kLowColor);
#else
		AddLine( bottomLeft, topLeft, kLowColor);
#endif
#if _ENABLE_BEVELS_
		lineStart = bottomLeft; lineStart.x += 2; lineStart.y -= 2;
		lineEnd = topLeft; lineEnd.x += 2; lineEnd.y += 2;
		AddLine( lineStart, lineEnd, kHighColor);
#endif
	}
	
	//	top
	if (tabLocation == tabs_on_top) {
#if _SIMPLE_BORDER_
		AddLine( topLeft, tabFrame.LeftBottom(), kHighColor);
		AddLine( tabFrame.RightBottom(), topRight, kHighColor);
#else
		AddLine( topLeft, tabFrame.LeftBottom(), kLowColor);
		AddLine( tabFrame.RightBottom(), topRight, kLowColor);
#endif
#if _ENABLE_BEVELS_
		lineStart = topLeft; lineStart.x += 2; lineStart.y += 2;
		lineEnd = tabFrame.LeftBottom(); lineEnd.x--; lineEnd.y += 2;
		AddLine( lineStart, lineEnd, kHighColor);
		lineStart = tabFrame.RightBottom(); lineStart.x++; lineStart.y += 2;
		lineEnd = topRight; lineEnd.x -= 2; lineEnd.y += 2;
		AddLine( lineStart, lineEnd, kHighColor);
#endif
	} else {
		AddLine( topLeft, topRight, kLowColor);
#if _ENABLE_BEVELS_
		lineStart = topLeft; lineStart.x += 2; lineStart.y += 2;
		lineEnd = topRight; lineEnd.x -= 2; lineEnd.y += 2;
		AddLine( lineStart, lineEnd, kHighColor);
#endif
	}
		
	EndLineArray();
}

void
BTabView::SetFlags(uint32 flags)
{
	BView::SetFlags(flags);
}

void
BTabView::SetResizingMode(uint32 mode)
{
	BView::SetResizingMode(mode);
}

void
BTabView::GetPreferredSize( float *width, float *height)
{
	BView::GetPreferredSize(width,height);
}

void
BTabView::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}

BHandler*
BTabView::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BView::ResolveSpecifier(msg, index, spec, form, prop);
}

status_t
BTabView::GetSupportedSuites(BMessage *data)
{
	return BView::GetSupportedSuites(data);
}

void
BTabView::AddTab(BView *v, BTab* tab)
{
//	ASSERT_WITH_MESSAGE(v, "view for tab must not be null");
	if (tab) {
		tab->SetView(v);
		fTabList->AddItem(tab);
	} else
		// setview performed in constructor
		fTabList->AddItem(new BTab(v));
	
	//	recalculate the tabwidth, only need to do this for width_from_widest
	//	others are calculated in tabframe
	if (fTabWidthSetting==B_WIDTH_FROM_WIDEST) {
		float sWidth = kTabLabelPadding;
		if (v->Name())
			sWidth += StringWidth(v->Name());
		fTabWidth = (fTabWidth < sWidth) ? sWidth : fTabWidth;
	}
}

#if !_PR3_COMPATIBLE_
BTab*
BTabView::RemoveTab(int32 i)
#else
BTab*
BTabView::RemoveTab(int32 i) const
#endif
{
	// !! this method should not be const, thus:
	BTabView* self = const_cast<BTabView*>(this);	
	
	//	do some bounds checking for safety, who needs safety
	if (i<0 || i>=fTabList->CountItems())
		return NULL;

	// remove the tab from the list
	BTab* tab = static_cast<BTab*>(fTabList->RemoveItem(i));
	// deselect the tab
	tab->Deselect();	// calls RemoveSelf if necessary
	
	//	decrement the current selection
	if (i <= fSelection && fSelection != 0)
	{
		self->fSelection--;
		// 	select the new tab
		self->Select(fSelection);	// Select() will redraw the TabView
	}
	else
	{
		if (LockLooper())
		{
			Invalidate();
			Window()->UpdateIfNeeded();
			UnlockLooper();
		}
	}

	return tab;
}


int32 BTabView::IndexForTab(const BTab *tab) const
{
	BTab *t;
	int32 index = 0;
	while ((t = TabAt(index)))
	{
		if (t == tab)
			return index;
		index++;
	}
	return -1;
}

BTab *BTabView::RemoveTab(const BTab *tab)
{
	return RemoveTab(IndexForTab(tab));
}

void BTabView::Select(const BTab *tab)
{
	Select(IndexForTab(tab));
}

void BTabView::SetFocusTab(const BTab *tab, bool focusState)
{
	SetFocusTab(IndexForTab(tab), focusState);
}

BRect BTabView::TabFrame(const BTab *tab) const
{
	return TabFrame(IndexForTab(tab));
}


// 	simple list accessor, returns null if out of bounds
BTab*
BTabView::TabAt(int32 index) const
{
	return (BTab*)fTabList->ItemAt(index);
}

void
BTabView::SetTabWidth(button_width s)
{
	if (s == fTabWidthSetting)
		return;
	
	fTabWidthSetting = s;

	//	need to recalculate the tabwidth based on the new tab width setting
	//	only for this setting, all others are done in TabFrame
	if (fTabWidthSetting==B_WIDTH_FROM_WIDEST) {
		int32 tcount = CountTabs();
		
		fTabWidth = 0;		// restart from 0 in case it is now smaller
		//	iterate through all of the tabs and find the largest tab label
		for (int32 i=0 ; i<tcount ; i++) {
			BView* v = ViewForTab(i);

			float sWidth = kTabLabelPadding;
			if (v->Name())
				sWidth += StringWidth(v->Name());
			fTabWidth = (fTabWidth < sWidth) ? sWidth : fTabWidth;
	
		}
	} 

	if (Window()) DrawBox(DrawTabs());
}

button_width
BTabView::TabWidth() const
{
	return fTabWidthSetting;
}

void
BTabView::SetTabHeight(float h)
{
	if (h == fTabHeight)
		return;
		
	// 	if the new height is larger the diff will be positive
	//	if the new height is smaller the diff will be negative
	float diff = h - fTabHeight;
	
	//	only move the view container if the tabs are on top or left
	//	resize the view container in all cases	
#if ENABLE_TAB_LOCATION
	tab_location tabLoc = TabLocation();
#else
	tab_location tabLoc = tabs_on_top;
#endif
	if (tabLoc == tabs_on_top) {
		fContainerView->MoveBy(0,diff);
		fContainerView->ResizeBy(0,-diff);
	} else if (tabLoc == tabs_on_bottom) {
		fContainerView->ResizeBy(0,-diff);
	} else if (tabLoc == tabs_on_left) {
		fContainerView->MoveBy(diff, 0);
		fContainerView->ResizeBy(-diff, 0);
	} else if (tabLoc == tabs_on_right) {
		fContainerView->ResizeBy(-diff, 0);
	}

	fTabHeight = h;

	if (LockLooper())
	{
		Invalidate();
		Window()->UpdateIfNeeded();
		UnlockLooper();
	}
}

// 	tab height includes the height of the text
//	plus padding plus base line for box
float
BTabView::TabHeight() const
{
	return fTabHeight;
}

BView*
BTabView::ContainerView() const
{
	return fContainerView;
}

int32
BTabView::CountTabs() const
{
	return fTabList->CountItems();
}

BView*
BTabView::ViewForTab(int32 tabIndex) const
{
	BTab* tab = TabAt(tabIndex);
	if (!tab)
		return NULL;
	else
		return TabAt(tabIndex)->View();
}

#if ENABLE_TAB_LOCATION
void
BTabView::SetTabLocation(tab_location location)
{
	if (fTabLocation == location)
		return;

	fTabLocation = location;

	BRect vframe = GetContainerViewFrame(Bounds(), fTabLocation, TabHeight());

	fContainerView->ResizeTo(vframe.Width(), vframe.Height());
	fContainerView->MoveTo(vframe.LeftTop());

	if (LockLooper())
	{
		Invalidate();
		Window()->UpdateIfNeeded();
		UnlockLooper();
	}
}

tab_location
BTabView::TabLocation() const
{
	return fTabLocation;
}

void
BTabView::SetFont(const BFont *font, uint32 mask)
{
	BView::SetFont(font, mask&~B_FONT_SPACING);

	// recalc the height necessary for the tab
	font_height finfo;
	font->GetHeight(&finfo);
	float h = (finfo.ascent + finfo.descent + finfo.leading) + 8;
	
	// calling settabheight will reposition the view container 
	SetTabHeight(h);	// redraws tabs and box
}

//	correctly resets fTabWidth
//	calling tab->SetLabel will not
void
BTabView::SetTabLabel(int32 tabIndex, const char* label)
{
	if (!label)
		return;
		
	BTab* tab = TabAt(tabIndex);	// returns null if out of bounds
	if (tab == NULL)
		return;
			
	//	if the labels are the same don't bother
	if (strcmp(tab->Label(), label) == 0)
		return;
	
	tab->SetLabel(label);
	if (fTabWidthSetting==B_WIDTH_FROM_WIDEST) {
		BView* v = tab->View();
		if (v) {
			int32 sWidth = kTabLabelPadding;
			if (v->Name())
				sWidth += StringWidth(v->Name());
			fTabWidth = (fTabWidth < sWidth) ? sWidth : fTabWidth;
		}
	}
	//	other two tab width settings are calculated dynamically in TabFrame

	if (LockLooper())
	{
		Invalidate();
		Window()->UpdateIfNeeded();
		UnlockLooper();
	}
}

const char*
BTabView::TabLabel(int32 tabIndex) const
{
	BTab* tab = TabAt(tabIndex);
	if (!tab)
		return NULL;
	else
		return TabAt(tabIndex)->Label();
}
#endif //	ENABLE_TAB_LOCATION

#if 0		// not finished
void
BTabView::SetTabEnabled(int32 tabIndex, bool on)
{
	BTab* tab = TabAt(tabIndex);
	if (tab)
		tab->SetEnabled(on);
}

bool
BTabView::IsTabEnabled(int32 tabIndex) const
{
	BTab* tab = TabAt(tabIndex);
	if (tab)
		return tab->IsEnabled();
	else
		return false;
}
#endif
		
void BTabView::_ReservedTabView1() {}
void BTabView::_ReservedTabView2() {}
void BTabView::_ReservedTabView3() {}
void BTabView::_ReservedTabView4() {}
void BTabView::_ReservedTabView5() {}
void BTabView::_ReservedTabView6() {}
void BTabView::_ReservedTabView7() {}
void BTabView::_ReservedTabView8() {}
void BTabView::_ReservedTabView9() {}
void BTabView::_ReservedTabView10() {}
void BTabView::_ReservedTabView11() {}
void BTabView::_ReservedTabView12() {}

// empty assignment and copy constructor to prevent default ones from
// doing the wrong thing
BTabView::BTabView(const BTabView &v)
	: BView(v) {}

BTabView &BTabView::operator=(const BTabView &) {return *this;}
