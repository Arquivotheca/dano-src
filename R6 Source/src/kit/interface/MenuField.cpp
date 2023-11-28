//*****************************************************************************
//
//	File:		MenuField.cpp
//
//	Written By:		Peter Potrebic
//	Rewritten By:	Dianne Hackborn
//
//	Copyright 1996, Be Incorporated
//
//*****************************************************************************

#include <stdlib.h>
#include <string.h>
#include <Debug.h>

#ifndef _MENU_FIELD_H
#include <MenuField.h>
#endif
#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _MENU_BAR_H
#include <MenuBar.h>
#endif
#ifndef _FONT_H
#include <Font.h>
#endif
#ifndef _GLYPH_H
#include <Glyph.h>
#endif
#ifndef _REGION_H
#include <Region.h>
#endif
#ifndef _MESSAGE_FILTER_H
#include <MessageFilter.h>
#endif
#ifndef _INTERFACE_MISC_H
#include <interface_misc.h>
#endif
#ifndef _MENU_PRIVATE_H
#include <MenuPrivate.h>
#endif
#ifndef _ARCHIVE_DEFS_H
#include <archive_defs.h>
#endif
#ifndef _BEEP_H
#include <Beep.h>
#endif
#include <MessageRunner.h>
#include <Window.h>

#include <StreamIO.h>

#include "ctype.h"

#define V_EXTRA	3
#define SHADOW 2
#define H_EXTRA	3
#define PRIV_MENU_BAR	"_mc_mb_"

namespace BPrivate {

/*------------------------------------------------------------*/

class BMCItem : public BMenuItem {
public:
					BMCItem(BMenu *sub);
					BMCItem(BMessage *data);
static	BArchivable	*Instantiate(BMessage *data);

virtual	void		SetLabel(const char *name);
virtual	void		SetEnabled(bool state);

protected:
virtual	void		GetContentSize(float *width, float *height);

private:
		bool		fLastEnabled;
};

/*------------------------------------------------------------*/

}	// namespace BPrivate

using namespace BPrivate;

BMCItem::BMCItem(BMenu *sub)
	: BMenuItem(sub, NULL), fLastEnabled(true)
{
}

/* ---------------------------------------------------------------- */

BMCItem::BMCItem(BMessage *data)
	: BMenuItem(data), fLastEnabled(IsEnabled())
{
}


/* ---------------------------------------------------------------- */

BArchivable *BMCItem::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BPrivate::BMCItem"))
		return NULL;
	return new BMCItem(data);
}


/*------------------------------------------------------------*/

namespace BPrivate {

/*------------------------------------------------------------*/

class BMCFilter : public BMessageFilter {
public:
						BMCFilter(BMenuField *mc, ulong what);

virtual	filter_result	Filter(BMessage *msg, BHandler **target);

private:
		BMenuField	*fField;
};

/*------------------------------------------------------------*/

class BMCMenuBar : public BMenuBar {

public:
						BMCMenuBar(BRect f, bool fixed_size, BMenuField *mc);
						BMCMenuBar(BMessage *data);
static	BArchivable		*Instantiate(BMessage *data);

virtual	void			AttachedToWindow();
virtual	void			MakeFocus(bool state);
virtual	void			FrameResized(float new_width, float new_height);
virtual	void			MessageReceived(BMessage *msg);
virtual	void			Draw(BRect update);

virtual void			AdjustSubmenuLocation(BPoint* inout_location,
											  BMenu* submenu);
		
		BMenuField*		Field();
		void			SetControlBackground(rgb_color color);
		void			LabelStateChanged();
		
		BMenuField		*fMC;
		bool			fFixedSize;
		BMessageRunner	*fRunner;
		
		rgb_color		fControlBackground;
};

/*------------------------------------------------------------*/

}	// namespace BPrivate

using namespace BPrivate;

// Need these so that old archives of BMCMenuBar (which weren't in the
// BPrivate namespace) will still work.

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT BArchivable*
	#if __GNUC__
	Instantiate__9_BMCItem_P8BMessage
	#elif __MWERKS__
	Instantiate__9_BMCItem_FP8BMessage
	#endif
	(BMessage* data)
	{
		if (!validate_instantiation(data, "_BMCItem_"))
			return NULL;
		return new BMCItem(data);
	}

	_EXPORT BArchivable*
	#if __GNUC__
	Instantiate__12_BMCMenuBar_P8BMessage
	#elif __MWERKS__
	Instantiate__12_BMCMenuBar_FP8BMessage
	#endif
	(BMessage* data)
	{
		if (!validate_instantiation(data, "_BMCMenuBar_"))
			return NULL;
		BMCMenuBar* bar = new BMCMenuBar(data);
		// Backwards compatibility -- old archives won't have
		// these set correctly.
		bar->SetBorder(B_BORDER_OFF);
		bar->SetItemMargins(0, 0, 0, 0);
		return bar;
	}

}
#endif
	
/*------------------------------------------------------------*/

BMenuField::BMenuField(BRect frame, const char *name, const char *label,
	BMenu *menu, uint32 resize, uint32 flags)
	: BView(frame, name, resize, flags | B_FULL_UPDATE_ON_RESIZE)
{
	InitObject(label);

	fMenu = menu;

	frame.OffsetTo(B_ORIGIN);
	fMenuBar = new BMCMenuBar(frame, false, this);
	AddChild(fMenuBar);
	fMenuBar->AddItem(new BMCItem(fMenu));
	
/*	BFont f;
	f.SetSpacing(B_STRING_SPACING);
	BView::SetFont(&f, B_FONT_SPACING); */
	
	DistributeFont();

	InitObject2();
}

/*------------------------------------------------------------*/

BMenuField::BMenuField(BRect frame, const char *name, const char *label,
	BMenu *menu, bool fixed_size, uint32 resize, uint32 flags)
	: BView(frame, name, resize, flags | B_FULL_UPDATE_ON_RESIZE)
{
	InitObject(label);

	fMenu = menu;

	fFixedSizeMB = fixed_size;

	frame.OffsetTo(B_ORIGIN);
	fMenuBar = new BMCMenuBar(frame, fixed_size, this);
	AddChild(fMenuBar);
	fMenuBar->AddItem(new BMCItem(fMenu));
	
	DistributeFont();

	InitObject2();
}

/*------------------------------------------------------------*/

void BMenuField::InitObject2()
{
	fMenuBar->AddFilter(new BMCFilter(this, B_MOUSE_DOWN));
	LayoutViews(true, true);
}

/*------------------------------------------------------------*/

void BMenuField::InitObject(const char *label)
{
	fMenu = NULL;
	fAlign =  B_ALIGN_LEFT;
	fEnabled = true;
	fSelected = false;
	fMenuTaskID = -1;
	fStringWidth = 0;
	fFixedSizeMB = false;
	fPopUpMarker = true;
	fHasLabel = true;
	
	fLabel = NULL;
	SetLabel(label);

	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	
	// if there is no label, then the popup should fill entire view.
	if (!label)
		fDivider = 0;
	else {
		fDivider = floor(Frame().Width() / 2.0);
	}
}

/*------------------------------------------------------------*/

void
BMenuField::DistributeFont(const BFont* font, uint32 mask)
{
	BFont tmp;
	if (!font) {
		GetFont(&tmp);
		font = &tmp;
		mask = B_FONT_ALL;
	}
	
	fMenuBar->SetFont(font, mask);
	DistributeMenuFont(fMenu, font, mask);
}

/*------------------------------------------------------------*/

void
BMenuField::DistributeMenuFont(BMenu* menu, const BFont* font, uint32 mask)
{
	menu->SetFont(font, mask);

	// iterate through all the submenus and set their font as well
	BMenu *subMenu = NULL;
	for (int32 i = 0; (subMenu = menu->SubmenuAt(i)) != NULL; i++)
		DistributeMenuFont(subMenu, font, mask);
}

/*------------------------------------------------------------*/

BMenuField::~BMenuField()
{
	if (fLabel) {
		free(fLabel);
		fLabel = NULL;
	}
	if (fMenuTaskID >= 0) {
		// someone's deleting this object at a bad time
		status_t		err;
		do {
			long dummy;
			err = wait_for_thread(fMenuTaskID,&dummy);
		} while (err == B_INTERRUPTED);
	}
}


/* ---------------------------------------------------------------- */

BMenuField::BMenuField(BMessage *data)
	: BView(data)
{
	const char *str;
	data->FindString(S_LABEL, &str);
	InitObject(str);

	SetFlags(Flags()|B_FULL_UPDATE_ON_RESIZE);
	
	fMenuBar = dynamic_cast<BMCMenuBar *>(FindView(PRIV_MENU_BAR));
	ASSERT(fMenuBar);

	// The menu is the first menu in the menu bar
	fMenu = fMenuBar->SubmenuAt(0);
	ASSERT(fMenu);

	InitObject2();

	bool	b;
	long	l;

	data->FindBool(S_DISABLED, &b);
	SetEnabled(!b);
	
	data->FindInt32(S_ALIGN, &l);
	SetAlignment((alignment) l);

	// don't call SetDivider(). That would shift the menubar again.
	data->FindFloat(S_DIVIDER, &fDivider);

	bool fixed;
	if (data->FindBool("be:fixeds", &fixed) == B_OK)
		fFixedSizeMB = fixed;

	if( data->FindBool("be:dmark", &b) == B_OK && !b )
		HidePopUpMarker();
}

/* ---------------------------------------------------------------- */

status_t BMenuField::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);

	// The child menubar along with the menu will get archived by inherited

	if (Label())
		data->AddString(S_LABEL, Label());
	if (!IsEnabled())
		data->AddBool(S_DISABLED, true);
	data->AddInt32(S_ALIGN, Alignment());

	data->AddFloat(S_DIVIDER, Divider());

	if (fFixedSizeMB)
		data->AddBool("be:fixeds", true);
	
	if (!fPopUpMarker)
		data->AddBool("be:dmark", false);

	return 0;
}

/* ---------------------------------------------------------------- */

BArchivable *BMenuField::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BMenuField"))
		return NULL;
	return new BMenuField(data);
}

/*------------------------------------------------------------*/

void	BMenuField::AttachedToWindow()
{
	SetColorsFromParent();
	fMenuTaskID = -1;

	ClearPartialMatch();
	
	if (fLabel)
		fStringWidth = StringWidth(fLabel);
}

/*------------------------------------------------------------*/

void	BMenuField::AllAttached()
{
	BView::AllAttached();
	fMenuBar->SetControlBackground(LowColor());
	LayoutViews();
}

/*------------------------------------------------------------*/

void BMenuField::Draw(BRect update)
{
	BRect bounds = Bounds();

	const rgb_color bg = B_TRANSPARENT_COLOR; //LowColor();
	const rgb_color fg = HighColor();
	const rgb_color fc = NextFocusColor();
	
	// Propagate any color changes in our control down to menu.
	fMenuBar->SetControlBackground(LowColor());
	
	PushState();
	DrawLabel(bounds, update);
	PopState();
	
	BRect b = GetBorderFrame();

	BPopUpMenuGlyph popGlyph;
	popGlyph.SetFrame(BRect(b.left - BPopUpMenuGlyph::kButtonWidth, b.top,
							b.left, b.bottom));
	popGlyph.SetPressed(fSelected);
	popGlyph.SetActive(false);
	popGlyph.SetFocused(IsFocus() && Window()->IsActive());
	popGlyph.SetEnabled(IsEnabled() && fMenu->IsEnabled());
	popGlyph.SetRightAttach(fHasLabel);

	if (fHasLabel) {
		BButtonBorderGlyph glyph;
		b.left -= 4;
		glyph.SetFrame(b);
		b.left += 4;
		glyph.SetPressed(fSelected);
		glyph.SetActive(false);
		glyph.SetFocused(IsFocus() && Window()->IsActive());
		glyph.SetEnabled(IsEnabled() && fMenu->IsEnabled());
		
		if (update.Intersects(glyph.CoveredFrame())) {
			if (fPopUpMarker && !popGlyph.IsEnabled()) {
				// The pop up mark will be drawn using alpha blending,
				// so we need to clip to make sure that the border doesn't
				// draw into its area.
				popGlyph.InverseClipView(this);
			}
			
			// Draw border
			glyph.SetBackgroundColor(bg);
			if (glyph.IsFocused())	glyph.SetBorderColor(fc);
			else					glyph.SetBorderColor(fg);
			glyph.SetFillColor(fMenuBar->ViewColor());
			glyph.SetLabelColor(fg);
			
			PushState();
			glyph.Draw(this);
			PopState();
			
			ConstrainClippingRegion(NULL);
		}
	}
	
	if (fPopUpMarker) {
		if( update.Intersects(popGlyph.CoveredFrame()) ) {
			
			popGlyph.SetBackgroundColor(bg);
			if (popGlyph.IsFocused())	popGlyph.SetBorderColor(fc);
			else						popGlyph.SetBorderColor(fg);
			popGlyph.SetFillColor(ui_color(B_MENU_SELECTED_BACKGROUND_COLOR));
			popGlyph.SetLabelColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
			
			popGlyph.Draw(this);
		}
	}
	
	if (IsFocus() && Window()->IsActive() && !IsInvalidatePending())
		InvalidateAtTime(NextFocusTime());
}

/*------------------------------------------------------------*/

void BMenuField::DrawLabel(BRect bounds, BRect update)
{
	font_height	finfo;
	BRect		r;

	if (!fLabel)
		return;

	BRect b = fMenuBar->Frame();
	
	r = bounds;
	r.right = r.left + fDivider - H_EXTRA;

	if (!r.Intersects(update))
		return;

	GetFontHeight(&finfo);

	bounds.right = bounds.left + fDivider;

	BPoint loc;
	switch (fAlign) {
		case B_ALIGN_LEFT:
			loc.x = bounds.left + H_EXTRA;
			break;
		case B_ALIGN_CENTER:
			{
			float center = (bounds.right - bounds.left) / 2;
			loc.x = center - (fStringWidth/2);
			break;
			}
		case B_ALIGN_RIGHT:
			{
			loc.x = bounds.right - fStringWidth - H_EXTRA;
			break;
			}
	}
	loc.y = b.top + finfo.ascent;
	if (loc.y+finfo.descent > bounds.bottom) {
		loc.y = bounds.bottom - finfo.descent;
	}
	
	MovePenTo(loc);

	if (!IsEnabled() || !fMenu->IsEnabled()) SetHighColor(HighColor().disable(LowColor()));
	if (HighColor().alpha != 255) SetDrawingMode(B_OP_ALPHA);
	
	DrawString(fLabel);
}

/*------------------------------------------------------------*/

long BMenuField::MenuTask(void *arg)
{
	BMenuField	*mc = (BMenuField *) arg;
	BRect		bounds;
	
	if (!mc->LockLooper())
		goto error;

	mc->fSelected = true;
	bounds = mc->Bounds();
	mc->Invalidate(bounds);
	mc->UnlockLooper();

	bool state;
	do {
		snooze(20000);
		if (B_OK!=mc->LockLooperWithTimeout(2000000))
			goto error;
		state = mc->fMenuBar->fTracking;
		mc->UnlockLooper();
	} while(state);

	if (!mc->LockLooper())
		goto error;
	mc->fSelected = false;
	mc->Invalidate(bounds);
	mc->fMenuTaskID = -1;
	mc->UnlockLooper();

error:

	return 0;
}

/*------------------------------------------------------------*/

void BMenuField::LayoutViews(bool attach, bool initial)
{
	float menuWidth, menuHeight;
	fMenuBar->GetPreferredSize(&menuWidth, &menuHeight);
	const BRect viewBounds(Bounds());
	
	const float extraLeft = fDivider
						  + (fPopUpMarker ? BPopUpMenuGlyph::kButtonWidth : 0);
	
	// first figure out what our dimensions should be
	if (initial || fFixedSizeMB)
		menuWidth = viewBounds.Width() - extraLeft - H_EXTRA*2 - SHADOW;
	
	float extraTop = 0;
	if (!attach) {
		// if the view is now being attached, we will take care of
		// any height issues by resizing it below; otherwise, all we
		// can do is move the menu bar inside to make it look nice
		if (menuHeight+V_EXTRA*2+SHADOW > viewBounds.Height()) {
			// if not fitting, just move around a bit to look better.
			extraTop = floor( (viewBounds.Height()-(menuHeight+V_EXTRA*2+SHADOW))/2 );
		} else {
			// center the menu in this field
			extraTop = floor( (viewBounds.Height()-V_EXTRA*2-menuHeight)/2 + .5 );
			if ((extraTop+V_EXTRA*2+menuHeight+SHADOW) > viewBounds.Height()) {
				extraTop = floor( (viewBounds.Height()-V_EXTRA*2-SHADOW-menuHeight)/2 + .5 );
			}
			if (extraTop < 0) extraTop = 0;
		}
	}
	
	// only resize this view if not a fixed size menu field.
	ResizeTo((!fFixedSizeMB) ? (menuWidth+extraLeft+H_EXTRA*2+SHADOW) : viewBounds.Width(),
			 attach ? (menuHeight+V_EXTRA*2+SHADOW) : viewBounds.Height());
	
	if (menuWidth != fMenuBar->Bounds().Width()
			|| menuHeight != fMenuBar->Bounds().Height()) {
		fMenuBar->ResizeTo(menuWidth, menuHeight);
		//fMenuBar->InvalidateLayout();
	}
	fMenuBar->MoveTo(extraLeft + H_EXTRA, extraTop + V_EXTRA);
	
	// show or hide the menu bar, depending on whether it has a label
	if (fMenu->IsLabelFromMarked() ||
			(fMenu->Superitem() && fMenu->Superitem()->Label() && *fMenu->Superitem()->Label())) {
		fHasLabel = true;
	} else {
		fHasLabel = false;
	}
}

/*------------------------------------------------------------*/

BRect BMenuField::GetBorderFrame() const
{
	const BRect bounds(Bounds());
	BRect b = fMenuBar->Frame();
	b.left -= H_EXTRA;
	b.right = fHasLabel ? (b.right+H_EXTRA) : b.left;
	b.top -= V_EXTRA;
	b.bottom += V_EXTRA;
	if( b.top < bounds.top ) b.top = bounds.top;
	if( b.bottom > (bounds.bottom-SHADOW) ) b.bottom = bounds.bottom-SHADOW;
	return b;
}

bool BMenuField::ExtendPartialMatch(const char* bytes, int32 numBytes)
{
	if (numBytes < 0) numBytes = strlen(bytes);
	if (numBytes <= 0) return false;
	if (*bytes <= ' ' || *bytes == 127) return false;
	if (*bytes == B_ESCAPE && numBytes == 1) {
		ClearPartialMatch();
		return true;
	}
	
	if (!fMenu || !fMenu->IsRadioMode()) {
		beep();
		return true;
	}
	
	fPartialMatch.Append(bytes, numBytes);
	BMenuItem* item = FindPartialMatch(fPartialMatch.String());
	if (!item) {
		ClearPartialMatch();
		beep();
		return true;
	}
	
	if (item != fMenu->FindMarked()) {
		item->SetMarked(true);
		if (item->IsMarked()) ((BInvoker*)item)->Invoke();
	}
	
	return true;
}

BMenuItem* BMenuField::FindPartialMatch(const char* prefix)
{
	if (!fMenu || !prefix || fPartialMatch.Length() <= 0) return NULL;
	
	const int32 N = fMenu->CountItems();
	for (int32 i=0; i<N; i++) {
		BMenuItem* item = fMenu->ItemAt(i);
		if (!item || !item->IsEnabled()) continue;
		
		const char* label = item->Label();
		const char* match = prefix;
		if (!label) continue;
		
		while (*label && *match) {
			if (*label == ' ') {
				label++;
				continue;
			}
			
			if (*label != *match && tolower(*label) != tolower(*match)) {
				break;
			}
			
			label++;
			match++;
		}
		
		if (!*match || *label == *match) return item;
	}
	
	return NULL;
}

void BMenuField::ClearPartialMatch()
{
	fPartialMatch = "";
}
		
/*------------------------------------------------------------*/

void BMenuField::MakeFocus(bool state)
{
	if (state == IsFocus())
		return;
	
	if (state && (!IsEnabled() || !fMenu->IsEnabled()) && IsNavigating()) {
		// if this control is disabled and the user is trying to
		// tab-navigate into it, don't take focus.
		return;
	}
	
	ClearPartialMatch();
	
	BView::MakeFocus(state);
	if (Window()) Invalidate();
}

/*------------------------------------------------------------*/

void BMenuField::KeyDown(const char *bytes, int32 numBytes)
{
	// if not enabled, don't let user bring up menu
	if (!IsEnabled() || !fMenu->IsEnabled()) {
		BView::KeyDown(bytes, numBytes);
		return;
	}
	
	uchar key = bytes[0];
	
	switch (key) {
		case B_LEFT_ARROW:
		case B_UP_ARROW:
		case B_DOWN_ARROW:
		case B_RIGHT_ARROW:
			if (fMenu->IsRadioMode()) {
				ClearPartialMatch();
				BMenuItem* item = fMenu->FindMarked();
				int32 delta = ((key == B_LEFT_ARROW || key == B_UP_ARROW) ? -1 : 1);
				int32 idx = fMenu->IndexOf(item)+delta;
				bool changed = false;
				while (!changed && idx >= 0 && idx < fMenu->CountItems()) {
					item = fMenu->ItemAt(idx);
					if (item && item->IsEnabled()) {
						item->SetMarked(true);
						if (item->IsMarked()) {
							changed = true;
							((BInvoker*)item)->Invoke();
						}
					}
					idx += delta;
				}
				break;
			}
			
			// if not in radio mode, fall through to pop up the menu
		case B_SPACE: {
			ClearPartialMatch();
			BRect b = Bounds();
			b = fMenuBar->ConvertFromParent(b);
			fMenuBar->StartMenuBar(0, true, true, &b);
			fSelected = true;
			break;
		}
		default:
			if (!ExtendPartialMatch(bytes)) BView::KeyDown(bytes, numBytes);
			break;
		}
}

/*------------------------------------------------------------*/

void BMenuField::MouseDown(BPoint /*where*/)
{
	// if not enabled, don't let user bring up menu
	if (!IsEnabled() || !fMenu->IsEnabled()) return;
	
	ClearPartialMatch();
	
	if ((Flags() & B_NAVIGABLE) != 0 && !IsFocus())
		MakeFocus(true);
	
	SetExplicitFocus();
	
	BRect b = Bounds();
	b = fMenuBar->ConvertFromParent(b);

	fMenuBar->StartMenuBar(0, false, true, &b);
	fMenuTaskID = spawn_thread(BMenuField::MenuTask, "_m_task_",
		B_NORMAL_PRIORITY, this);
	if (fMenuTaskID >= 0)
		resume_thread(fMenuTaskID);
}

/*------------------------------------------------------------*/

void	BMenuField::SetLabel(const char *text)
{
	if (fLabel && (strcmp(fLabel, text) == 0))
		return;

	if (fLabel)
		free(fLabel);
	fLabel = strdup(text);

	if (Window()) {
		Invalidate();
		if (fLabel)
			fStringWidth = StringWidth(fLabel);
	}
}

/*------------------------------------------------------------*/

const char*	BMenuField::Label() const
{
	return(fLabel);
}

/*------------------------------------------------------------*/

void	BMenuField::SetEnabled(bool on)
{
	if (fEnabled != on) {
		fEnabled = on;

		// and need to adjust the menubar
		fMenuBar->SetEnabled(fEnabled);

		if (Window()) {
			fMenuBar->Invalidate(fMenuBar->Bounds());
			Invalidate(Bounds());
		}
	}
}

/*------------------------------------------------------------*/

bool	BMenuField::IsEnabled() const
{
	return fEnabled;
}

/*------------------------------------------------------------*/

BMenuItem *BMenuField::MenuItem() const
{
	return fMenuBar->ItemAt(0);
}

/*------------------------------------------------------------*/

BMenuBar *BMenuField::MenuBar() const
{
	return fMenuBar;
}

/*------------------------------------------------------------*/

BMenu *BMenuField::Menu() const
{
	return fMenu;
}

/* -------------------------------------------------------------------- */

void BMenuField::SetAlignment(alignment align)
{
	fAlign = align;
}

/*------------------------------------------------------------*/

alignment BMenuField::Alignment() const
{
	return fAlign;
}

/* -------------------------------------------------------------------- */

void BMenuField::SetDivider(float divide)
{
	if (divide < 0 || divide > Bounds().right)
		return;

	if (fDivider != divide) {
		fDivider = divide;
		LayoutViews();
	}

	if (Window()) {
		Invalidate();
		fMenuBar->Invalidate();
		Window()->UpdateIfNeeded();
	}
}

/*------------------------------------------------------------*/

float BMenuField::Divider() const
{
	return fDivider;
}

/*------------------------------------------------------------*/

void BMenuField::ShowPopUpMarker()
{
	if (!fPopUpMarker) {
		fPopUpMarker = true;
		LayoutViews();
	}
}

/*------------------------------------------------------------*/

void BMenuField::HidePopUpMarker()
{
	if (fPopUpMarker) {
		fPopUpMarker = false;
		LayoutViews();
	}
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

BMCMenuBar::BMCMenuBar(BRect frame, bool fixed_size, BMenuField *mc)
	: BMenuBar(frame, PRIV_MENU_BAR, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_ITEMS_IN_ROW, !fixed_size)
{
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	SetFlags(Flags() | B_FRAME_EVENTS);
	SetBorder(B_BORDER_OFF);
	fMC = mc;

	fFixedSize = fixed_size;
	fRunner = NULL;

	fControlBackground = B_TRANSPARENT_COLOR;
	
	SetItemMargins(0, 0, 0, 0);
}

/* ---------------------------------------------------------------- */

BMCMenuBar::BMCMenuBar(BMessage *data)
	: BMenuBar(data)
{
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	SetFlags(Flags() | B_FRAME_EVENTS);
	fMC = NULL;
	fRunner = NULL;

	fControlBackground = B_TRANSPARENT_COLOR;
	
	bool	resize;
	if (data->FindBool(S_RESIZE_TO_FIT, &resize) == B_OK)
		fFixedSize = !resize;
	else
		fFixedSize = true;
}

/* ---------------------------------------------------------------- */

BArchivable *BMCMenuBar::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BPrivate::BMCMenuBar"))
		return NULL;
	return new BMCMenuBar(data);
}

/*------------------------------------------------------------*/

void BMCMenuBar::AttachedToWindow()
{
	/*
	 don't want this to ever be the key menu bar
	*/
	fMC = (BMenuField *) Parent();
	BMenuBar *key = Window()->KeyMenuBar();
	BMenuBar::AttachedToWindow();
	Window()->SetKeyMenuBar(key);
}

/*------------------------------------------------------------*/

void BMCMenuBar::FrameResized(float, float)
{
	BMenuField *parent = dynamic_cast<BMenuField*>(Parent());
	if (!fFixedSize) parent->LayoutViews();
}

/* ---------------------------------------------------------------- */

void BMCMenuBar::MessageReceived(BMessage *msg)
{
	if (msg->what == 'TICK') {
		BMenuItem *mi = ItemAt(0);
		if (mi && mi->Submenu() && !mi->Submenu()->Window()) {
			// submenu isn't up, so we want to exit tracking
			BMessage	kd(B_KEY_DOWN);
			kd.AddInt8("byte", B_ESCAPE);
			kd.AddInt8("key", B_ESCAPE);
			kd.AddInt32("modifiers", 0);
			kd.AddInt8("raw_char", B_ESCAPE);

			Window()->PostMessage(&kd, this);
		}
	}
	BView::MessageReceived(msg);
}


/*------------------------------------------------------------*/

void BMCMenuBar::MakeFocus(bool state)
{
	if (state == IsFocus())
		return;

	BMenuBar::MakeFocus(state);

	/*
	 This business with the 'TICK' message is to deal with bug #8340.
	 Avoid the wierd state of menu_item in menubar being selected without
	 the submenu being visible.
	*/
	if (state) {
		ASSERT(fRunner == NULL);
		BMessage	msg('TICK');
		fRunner = new BMessageRunner(BMessenger(this), &msg, 50000);
	} else {
		ASSERT(fRunner != NULL);
		delete fRunner;
		fRunner = NULL;
	}

	if (!state) {
		ASSERT(fMC);
		fMC->fSelected = false;
		fMC->Invalidate();
	}
}

/*------------------------------------------------------------*/

void BMCMenuBar::Draw(BRect updateRect)
{
	if (!IsEnabled()) {
		PushState();
		SetViewColor(ViewColor().disable(fControlBackground));
		SetLowColor(LowColor().disable(fControlBackground));
		SetHighColor(HighColor().disable(fControlBackground));
		BMenuBar::Draw(updateRect);
		PopState();
	} else {
		BMenuBar::Draw(updateRect);
	}
}

/*------------------------------------------------------------*/

void BMCMenuBar::AdjustSubmenuLocation(BPoint* inout_location, BMenu* submenu)
{
	fMC->ClearPartialMatch();
	
	BPoint pt = fMC->GetBorderFrame().LeftTop();
	fMC->ConvertToScreen(&pt);
	float ml=0, mt=0, mr=0, mb=0;
	submenu->GetFrameMargins(&ml, &mt, &mr, &mb);
	pt.y -= mt - V_EXTRA;
	
	inout_location->x = pt.x-1;
	if (inout_location->y > pt.y) inout_location->y = pt.y;
}

/*------------------------------------------------------------*/

BMenuField* BMCMenuBar::Field()
{
	return fMC;
}

/*------------------------------------------------------------*/

void BMCMenuBar::SetControlBackground(rgb_color color)
{
	fControlBackground = color;
}

/*------------------------------------------------------------*/

void BMCMenuBar::LabelStateChanged()
{
	fMC->LayoutViews();
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

BMCFilter::BMCFilter(BMenuField *mc, ulong what)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, what)
{
	fField = mc;
}

/*------------------------------------------------------------*/

filter_result	BMCFilter::Filter(BMessage *msg, BHandler **target)
{
	if (msg->what == B_MOUSE_DOWN) {
		// The 'where' is in coord space of the MenuBar. Need to translate
		// into local coord space
		BPoint pt;
		BView	*v = dynamic_cast<BView*>(*target);
		ASSERT(v);
		msg->FindPoint("be:view_where", &pt);
		pt = v->ConvertToParent(pt);
		msg->ReplacePoint("be:view_where", pt);
		*target = fField;
	}

	return B_DISPATCH_MESSAGE;
}

/*-------------------------------------------------------------*/

BMenuField &BMenuField::operator=(const BMenuField &) {return *this;}

/*-------------------------------------------------------------*/

BHandler *BMenuField::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BView::ResolveSpecifier(msg, index, spec, form, prop);
}

/*---------------------------------------------------------------*/

status_t	BMenuField::GetSupportedSuites(BMessage *data)
{
	return BView::GetSupportedSuites(data);
}

/*----------------------------------------------------------------*/

status_t BMenuField::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BMenuField::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}

/* ---------------------------------------------------------------- */

void BMenuField::AllDetached()
{
	BView::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BMenuField::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BMenuField::WindowActivated(bool state)
{
	BView::WindowActivated(state);
	if (IsFocus()) Invalidate();
}

/* ---------------------------------------------------------------- */

void BMenuField::MouseUp(BPoint pt)
{
	BView::MouseUp(pt);
}

/*---------------------------------------------------------------*/

void	BMenuField::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void	BMenuField::FrameResized(float new_width, float new_height)
{
	BView::FrameResized(new_width, new_height);
}
/* ---------------------------------------------------------------- */

void BMenuField::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	BView::MouseMoved(pt, code, msg);
}

/*---------------------------------------------------------------*/

void BMenuField::SetFont(const BFont *font, uint32 mask)
{
	/*
	 This function/override did not exist in R5. So if adding functionality
	 here must consider the implications of that fact.
	*/
	BView::SetFont(font, mask&~B_FONT_SPACING);
	//DistributeFont(font, mask);
}

/*---------------------------------------------------------------*/

void BMenuField::ResizeToPreferred()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::ResizeToPreferred();
}

/*---------------------------------------------------------------*/

void BMenuField::GetPreferredSize(float *width, float *height)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	
	(*width) = (*height) = 0;
	
	if (!fFixedSizeMB) {
		// We are dynamically sizing, so we can compute our desired
		// size.
		BMenu* popup = Menu();
		const char* label = (popup && popup->Superitem()) ? popup->Superitem()->Label() : 0;
		if ((label) && (*label == 0)) label = 0;
		
		if (popup && popup->IsLabelFromMarked()) {
			*width = 0;
			int32 num = popup->CountItems();
			for (int32 i=0; i<num; i++) {
				BMenuItem* item = popup->ItemAt(i);
				if (item && item->Label()) {
					const float w = StringWidth(item->Label());
					if( w > (*width) ) {
						(*width) = w;
						label = item->Label();
					}
				}
			}
		} else if (label) {
			(*width) = StringWidth(label);
		}
		
		if (fPopUpMarker) (*width) += BPopUpMenuGlyph::kButtonWidth;
		if (label) (*width) += H_EXTRA*2;
		
		(*width) += SHADOW;
	
	} else {
		// If fixed size, just have to stay at the current size.
		// (This is required for backwards compatibility.)
		(*width) = Bounds().Width();
	}
	
	if (fMenuBar) (*height) = fMenuBar->Frame().Height();
	(*height) += V_EXTRA*2 + SHADOW;
	
	if (Label() && *Label()) (*width) += StringWidth(Label()) + H_EXTRA;
}


void BMenuField::_ReservedMenuField1() {}
void BMenuField::_ReservedMenuField2() {}
void BMenuField::_ReservedMenuField3() {}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

void BMCItem::SetLabel(const char* label)
{
	const char* orig = Label();
	const bool changed = ((label && *label) != (orig && *orig));
	BMenuItem::SetLabel(label);
	if (changed) {
		BMCMenuBar	*mbar = dynamic_cast<BMCMenuBar*>(Menu());
		ASSERT(mbar);
		// Personal Assistant calls this without locking the window,
		// so make sure it is locked here.
		if (mbar->LockLooperWithTimeout(5*1000000) >= B_OK) {
			mbar->LabelStateChanged();
			mbar->UnlockLooper();
		}
	}
}

void BMCItem::SetEnabled(bool state)
{
	BMenuItem::SetEnabled(state);
	if (state != fLastEnabled) {
		fLastEnabled = state;
		BMCMenuBar	*mbar = dynamic_cast<BMCMenuBar*>(Menu());
		ASSERT(mbar);
		if (mbar->LockLooperWithTimeout(5*1000000) >= B_OK) {
			mbar->Field()->Invalidate();
			mbar->UnlockLooper();
		}
	}
}

/*------------------------------------------------------------*/

void BMCItem::GetContentSize(float *width, float *height)
{
	BMCMenuBar	*mbar = dynamic_cast<BMCMenuBar*>(Menu());

	ASSERT(mbar);

	BMenuItem::GetContentSize(width, height);

	if (mbar->fFixedSize) {
		// want this item to take up the entire width of the menubar
		*width = mbar->Frame().Width();
	}

//+	PRINT(("(%s) width=%d\n", Label(), (int) *width));
}
