//******************************************************************************
//
//
//	Copyright 2001, Be Incorporated
//
//******************************************************************************

#include "ColorSelector.h"

#include <ColorControl.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <StringView.h>
#include <Window.h>

#include <Debug.h>
#include <StreamIO.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace ColorControlPrivate {

enum {
	CMD_CHOOSE_UI_COLOR		= 'cuic',
	CMD_SET_UI_COLOR		= 'suic'
};

class ColorMenuItem : public BMenuItem {
public:
	ColorMenuItem(	const char* label, BMessage* msg,
					rgb_color color, rgb_color initial_color)
		: BMenuItem(label, msg)
	{
		fColor = color;
		fInitialColor = initial_color;
	}
	
	void ResetColors(rgb_color col, rgb_color initial)
	{
		if (fColor != col || fInitialColor != initial) {
			fColor = col;
			fInitialColor = initial;
			BMenu* parent = Menu();
			if (parent) parent->Invalidate(Frame());
		}
	}
	
	void SetColor(rgb_color col)
	{
		if (fColor != col) {
			fColor = col;
			BMenu* parent = Menu();
			if (parent) parent->Invalidate(Frame());
		}
	}
	
	rgb_color Color() const
	{
		return fColor;
	}
	
	rgb_color InitialColor() const
	{
		return fInitialColor;
	}
	
	virtual	void DrawContent()
	{
		BRect		b = Frame();
		BMenu		*parent = Menu();
		BPoint		loc = parent->PenLocation();
		
		enum {
			W_CHAR = 0,
			A_CHAR = 1,
			OPEN_CHAR = 2,
			CLOSE_CHAR = 3,
			SPACE_CHAR = 4,
			NUM_CHARS = 5
		};
		float escapements[NUM_CHARS];
		BFont font;
		parent->GetFont(&font);
		font.GetEscapements("WA() ", NUM_CHARS, escapements);
		for (int32 i=0; i<NUM_CHARS; i++) {
			escapements[i] *= font.Size();
		}
		
		const float blockWidth = escapements[W_CHAR]+escapements[A_CHAR];
		
		const rgb_color old_col = parent->HighColor();
		font_height fh;
		
		const bool showInitial = fInitialColor != fColor;
		
		b.InsetBy(1, 1);
		b.bottom -= 1;
		b.left = loc.x;
		
		if (showInitial) {
			parent->GetFontHeight(&fh);
			parent->DrawString("(", BPoint(b.left, loc.y+fh.ascent));
		}
		b.left += escapements[OPEN_CHAR];
		b.InsetBy(2, 2);
		b.right = b.left + escapements[W_CHAR];
		if (showInitial) {
			parent->SetHighColor(fInitialColor);
			parent->FillRect(b);
		}
		b.InsetBy(-1, -1);
		if (showInitial) {
			parent->SetHighColor(old_col);
			parent->StrokeRect(b);
		}
		b.InsetBy(-1, -1);
		if (showInitial) {
			parent->DrawString(")", BPoint(b.right+1, loc.y+fh.ascent));
		}
		b.right += escapements[CLOSE_CHAR] + 1;
		
		b.left = b.right + escapements[SPACE_CHAR];
		b.right = b.left + blockWidth;
		
		parent->SetHighColor(fColor);
		parent->FillRect(b);
		parent->SetHighColor(old_col);
		b.InsetBy(-1, -1);
		parent->StrokeRect(b);
		
		parent->MovePenTo(b.right + escapements[SPACE_CHAR]*2 + 2, loc.y);
		
		BMenuItem::DrawContent();
	}
	
	virtual	void GetContentSize(float *w, float *h)
	{
		BMenuItem::GetContentSize(w, h);
		*w += Menu()->StringWidth("(W) WA  ")+4 + 2;
	}
	
private:
	rgb_color fColor, fInitialColor;
};

}
using namespace ColorControlPrivate;

static void populate_colors(BMenu* dest, const BMessage& src,
							const BMessage& names, const BMessage* initial)
{
	void* cookie = NULL;
	const char* name;
	type_code type;
	int32 n;
	rgb_color col;
	while (src.GetNextName(&cookie, &name, &type, &n) == B_OK) {
		if (type == B_RGB_COLOR_TYPE && src.FindRGBColor(name, &col) == B_OK) {
			// See if this field already exists in the menu.
			const int32 n = dest->CountItems();
			int32 i;
			bool found = false;
			for (i=0; i<n; i++) {
				BMenuItem* it = dest->ItemAt(i);
				const char* field;
				if (it && it->Message() &&
						it->Message()->FindString("field", &field) == B_OK
						&& strcmp(field, name) == 0) {
					ColorMenuItem* ci = dynamic_cast<ColorMenuItem*>(it);
					if (ci) ci->SetColor(col);
					found = true;
					break;
				}
			}
			
			if (found) continue;
			
			// This color doesn't currently exist; add it in.
			BMessage* msg = new BMessage(CMD_CHOOSE_UI_COLOR);
			msg->AddString("field", name);
			const char* label;
			if (names.FindString(name, &label) != B_OK || !label || !*label) {
				label = strrchr(name, ':');
				if (label && *label) label++;
				if (!label || !*label) label = name;
			}
			// Alphabetical order.
			for (i=0; i<n; i++) {
				BMenuItem* it = dest->ItemAt(i);
				if (it && strcmp(it->Label(), label) > 0)
					break;
			}
			rgb_color init_col;
			if (!initial || initial->FindRGBColor(name, &init_col) != B_OK)
				init_col = col;
			ColorMenuItem* ci = new ColorMenuItem(label, msg, col, init_col);
			dest->AddItem(ci, i);
		}
	}
}

enum {
	FRAME = 5,
	SPACE = 5
};

ColorSelector::ColorSelector(	BRect frame, const char* name,
								const char* label,
								const BMessage& colors, const BMessage& names,
								BMessage* model,
								uint32 resizeMask, uint32 flags)
	:	BControl(frame, name, label, model, resizeMask, flags),
		fNames(names), fInitColors(colors), fColors(colors), fSizeValid(false)
{
	const BRect dummyRect(-100, -100, -10, -10);
	
	fColorMenu = new BPopUpMenu("Colors");
	populate_colors(fColorMenu, fColors, fNames, &fInitColors);
	fColorField = new BMenuField(dummyRect, "Color", "Color: ", fColorMenu,
								 B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP,
								 B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE);
	AddChild(fColorField);
	fColorField->SetFont(be_bold_font);
	
	fColorPalette = new BColorControl(dummyRect.LeftTop(), B_CELLS_32x8, 8,
									  "Palette", new BMessage(CMD_SET_UI_COLOR),
									  true);
	fColorPalette->SetModeFlags(B_CC_32BIT_MODE | B_CC_SHOW_SWATCH);
	AddChild(fColorPalette);
}

ColorSelector::~ColorSelector()
{
}

void ColorSelector::ExtractColors(BMessage* dest, const BMessage& src)
{
	void* cookie = NULL;
	const char* name;
	type_code type;
	int32 n;
	while (src.GetNextName(&cookie, &name, &type, &n) == B_OK) {
		if (type == B_RGB_COLOR_TYPE) {
			for (int32 i=0; i<n; i++) {
				const void* data;
				ssize_t size;
				if (src.FindData(name, type, i, &data, &size) == B_OK
						&& size == sizeof(rgb_color)) {
					dest->AddData(name, type, data, size, true, i==0 ? n : 1);
				}
			}
		}
	}
}

void ColorSelector::AttachedToWindow()
{
	BControl::AttachedToWindow();
	
	BMessenger me(this);
	fColorPalette->SetTarget(me);
	fColorMenu->SetTargetForItems(me);
	BMenuItem* it = fColorMenu->ItemAt(0);
	if (it) {
		it->SetMarked(true);
		it->Invoke();
	}
	
	LayoutViews(true);
}

void ColorSelector::LayoutViews(bool really)
{
	float mw, mh, cw, ch;
	fColorField->SetDivider(fColorField->StringWidth(fColorField->Label()) + 5);
	fColorField->GetPreferredSize(&mw, &mh);
	fColorPalette->GetPreferredSize(&cw, &ch);
	
	if (really) {
		BRect b(Bounds());
		if (Window()) Window()->BeginViewTransaction();
		fColorField->MoveTo(b.left, b.top);
		fColorField->ResizeTo(b.Width()+1, mh);
		fColorPalette->MoveTo(b.left, b.top+mh+SPACE);
		fColorPalette->ResizeTo(cw, ch);
		if (Window()) Window()->EndViewTransaction();
	}
	
	fPrefWidth = (mw > cw ? mw : cw);
	fPrefHeight = mh + SPACE + ch;
	fSizeValid = true;
}

void ColorSelector::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped()) {
		rgb_color color;
		if (msg->FindRGBColor("RGBColor", &color) == B_OK) {
			if (fColorPalette) {
				fColorPalette->SetValue(color);
				fColorPalette->Invoke();
			}
		} else {
			BControl::MessageReceived(msg);
		}
		return;
	}
	
	switch (msg->what) {
		case CMD_CHOOSE_UI_COLOR: {
			const char* field;
			if (msg->FindString("field", &field) == B_OK) {
				fCurrentField = field;
				rgb_color color;
				if (fColors.FindRGBColor(field, &color) == B_OK) {
					fColorPalette->SetValue(color);
				}
			}
		} break;
		
		case CMD_SET_UI_COLOR: {
			if (fCurrentField.Length() > 0 && Message()) {
				BMessage upd(*Message());
				BMessage colors;
				rgb_color c = fColorPalette->ValueAsColor();
				c.alpha = 255;
				colors.AddRGBColor(fCurrentField.String(), c);
				upd.AddMessage("changes", &colors);
				Invoke(&upd);
			}
		} break;
		
		default:
			BControl::MessageReceived(msg);
			break;
	}
}

void ColorSelector::FrameResized(float, float)
{
	LayoutViews(true);
}

void ColorSelector::GetPreferredSize(float* width, float* height)
{
	if (!fSizeValid)
		LayoutViews(false);
	*width = fPrefWidth;
	*height = fPrefHeight;
}

/*------------------------------------------------------------*/

bool ColorSelector::IsDirty() const
{
	bool dirty = false;
	
	void* cookie = NULL;
	const char* name;
	type_code type;
	int32 n;
	while (!dirty && fInitColors.GetNextName(&cookie, &name, &type, &n) == B_OK) {
		for (int32 i=0; i<n; i++) {
			const void* data1;
			const void* data2;
			ssize_t size1;
			ssize_t size2;
			if (fInitColors.FindData(name, type, i, &data1, &size1) == B_OK &&
					( fColors.FindData(name, type, i, &data2, &size2) != B_OK ||
					  size1 != size2 || memcmp(data1, data2, size1) != 0)) {
				dirty = true;
			}
		}
	}

	return dirty;
}

void ColorSelector::SetTo(const BMessage& colors)
{
	fInitColors.MakeEmpty();
	ExtractColors(&fInitColors, colors);
	fColors = fInitColors;
	
	fColorMenu->RemoveItems(0, fColorMenu->CountItems(), true);
	populate_colors(fColorMenu, fColors, fNames, &fInitColors);
	
	if (Window()) ColorSelector::AttachedToWindow();
}

void ColorSelector::Update(const BMessage& changes)
{
	BMessage colors;
	ExtractColors(&colors, changes);
	if (!colors.IsEmpty()) {
		fColors.Update(colors);
		populate_colors(fColorMenu, fColors, fNames, &fInitColors);
		rgb_color color;
		if (fColors.FindRGBColor(fCurrentField.String(), &color) == B_OK) {
			fColorPalette->SetValue(color);
		}
	}
}

void ColorSelector::Revert()
{
	Update(fInitColors);
}

const BMessage& ColorSelector::CurrentColors() const
{
	return fColors;
}

const BMessage& ColorSelector::InitialColors() const
{
	return fInitColors;
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
