// **************************************************************************
//
//		This set of classes is not for public consumption
//		It has not been thoroughly tested and was designed
//			with a specific use in mind
//		User at your own risk
//
// **************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <interface_misc.h>
#include <Screen.h>
#include <Window.h>

#include "ListSelector.h"
#include "color_defs.h"


// **************************************************************************

TSelectionList::TSelectionList(char* delimiter, BMessage* m)
	: BList()
{
	fAlignment = B_ALIGN_RIGHT;
	if (delimiter)
		fDelimiter = strdup(delimiter);
	else
		fDelimiter = NULL;
	
	fFrame.Set(0,0,0,0);
	fSelection = 0;
	fMessage = m;
	fFloor = -1;
	fCeiling = -1;
	fIsCounter = false;
}

TSelectionList::TSelectionList(int32 floor, int32 ceiling,
	char* delimiter, BMessage* m)
	: BList()
{
	fAlignment = B_ALIGN_RIGHT;
	if (delimiter)
		fDelimiter = strdup(delimiter);
	else
		fDelimiter = NULL;
	
	fFrame.Set(0,0,0,0);
	fSelection = 0;
	fMessage = m;
	fFloor = floor;
	fCeiling = ceiling;
	fIsCounter = true;
}

TSelectionList::~TSelectionList()
{
	if (fDelimiter)
		free(fDelimiter);
	delete fMessage;
	
	fFloor = -1;
	fCeiling = -1;
	if (fIsCounter)
		return;
		
	const char* item;
	for (int32 i=0 ; i<CountItems() ; i++) {
		item = ItemAt(i);
		//	assumes that all list contents were allocated dynamically
		if (item)
			// this is really bad - should not free here
			// instead should use something like GetStringAt(BString &result, int32 index) const;
			free((char*)item);
	}
}

void
TSelectionList::AddItem(const char* item)
{
	if (!item || fIsCounter)
		return;
		
	char* i = strdup(item);
	BList::AddItem(i);
}

const char *
TSelectionList::ItemAt(int32 index) const
{
	if (fIsCounter) {
		char* str = (char*)malloc(32);
		// this is really bad - should not malloc here
		sprintf(str, "%ld", index);
		return str;
	}
	
	if (index < 0 || index >= CountItems())
		return NULL;
	
	if (fFloor != -1 && index < fFloor)
		index = fFloor;
	if (fCeiling != -1 && index > fCeiling)
		index = fCeiling;
			
	return (const char*)BList::ItemAt(index);
}

int32
TSelectionList::CountItems() const
{
	if (fIsCounter)
		return fCeiling - fFloor + 1;		
	else
		return BList::CountItems();
}

char *
TSelectionList::Delimiter() const
{
	return fDelimiter;
}

void 
TSelectionList::SetDelimiter(char* d)
{
	if (!d)
		return;
	
	if (fDelimiter && (strcmp(d, fDelimiter) == 0))
		return;
			
	if (fDelimiter)
		free(fDelimiter);
		
	fDelimiter = strdup(d);
}

BRect 
TSelectionList::Frame() const
{
	return fFrame;
}

void 
TSelectionList::SetFrame(BRect f)
{
	fFrame = f;
}

const char *
TSelectionList::SelectedText() const
{
	return ItemAt(Selection());
}

int32 
TSelectionList::Selection() const
{
	if (fFloor != -1 && fSelection < fFloor)
		return fFloor;
	if (fCeiling != -1 && fSelection > fCeiling)
		return fCeiling;	
	
	return fSelection;
}

void 
TSelectionList::SetSelection(int32 s)
{
	if (fSelection == s)
		return;
		
	if (!fIsCounter) {
		if (s < 0 || s >=CountItems())
			return;
	}
	
	if (fFloor != -1 && s < fFloor)
		s = fFloor;
	if (fCeiling != -1 && s > fCeiling)
		s = fCeiling;

	fSelection = s;
}

void
TSelectionList::SetFloor(int32 f)
{
	if (fIsCounter) {
		fFloor = f;
		return;
	}
		
	if (fFloor == f)
		return;
		
	if (f < 0)
		fFloor = -1;
	else if (f >= CountItems()-1)
		fFloor = CountItems()-1;
	else
		fFloor = f;
}

int32
TSelectionList::Floor() const
{
	if (fFloor == -1)
		return 0;
		
	return fFloor;
}

void
TSelectionList::SetCeiling(int32 c)
{
	if (fIsCounter) {
		fCeiling = c;
		return;
	}
	
	if (fCeiling == c)
		return;
		
	if (c < 0)
		fCeiling = -1;
	else if (c >= CountItems()-1)
		fCeiling = CountItems()-1;
	else
		fCeiling = c;
}

int32
TSelectionList::Ceiling() const
{
	if (fCeiling == -1)
		return CountItems()-1;
	
	return fCeiling;
}

BMessage *
TSelectionList::Message() const
{
	return fMessage;
}

void 
TSelectionList::SetMessage(BMessage* m)
{
	if (fMessage == m)
		return;
		
	if (fMessage)
		delete fMessage;
		
	fMessage = m;
}

void
TSelectionList::SetCounter(bool f)
{
	fIsCounter = f;
}

bool
TSelectionList::IsCounter() const
{
	return fIsCounter;
}

void
TSelectionList::SetAlignment(alignment a)
{
	if (fAlignment == a)
		return;
		
	fAlignment = a;
}

alignment
TSelectionList::Alignment() const
{
	return fAlignment;
}

// **************************************************************************

static float
FontHeight(BView* view, bool full)
{
	if (!view)
		return 0;
		
	font_height finfo;		
	view->GetFontHeight(&finfo);
	float h = finfo.ascent + finfo.descent;
	
	if (full)
		h += finfo.leading;
		
	return h;
}

static color_map*
ColorMap()
{
	color_map* cmap;
	
	BScreen screen(B_MAIN_SCREEN_ID);
	cmap = (color_map*)screen.ColorMap();
	
	return cmap;
}

static uint8
IndexForColor(uint8 r, uint8 g, uint8 b)
{
	BScreen screen(B_MAIN_SCREEN_ID);
	return screen.IndexForColor(r,g,b);
}

static uint8
IndexForColor(rgb_color c)
{
	return IndexForColor(c.red, c.green, c.blue);
}

static rgb_color
ColorForIndex(uint8 i)
{
	color_map *m = ColorMap();
	return m->color_list[i];
}

static uint8
GetHiliteColor(color_map* map, uint8 index)
{
	rgb_color color = map->color_list[index];
	color.red = (long)color.red * 2 / 3;
	color.green = (long)color.green * 2 / 3;
	color.blue = (long)color.blue * 2 / 3;
	
	return IndexForColor(color);
}

const int32 kIconWidth = 12;
const int32 kIconHeight = 7;
const color_space kIconColorSpace = B_COLOR_8_BIT;

const unsigned char kUpArrow [] = {
	0x1b,0x1b,0x1b,0x1b,0x1b,0x13,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x13,0x1e,0x13,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x13,0x1e,0x1b,0x18,0x13,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x13,0x1e,0x1b,0x1b,0x1b,0x18,0x13,0x1b,0x1b,0x1b,
	0x1b,0x13,0x1e,0x18,0x18,0x18,0x18,0x18,0x18,0x0b,0x1b,0x1b,
	0x1b,0x13,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x13,0x1b,
	0x1b,0x1b,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x1b
};

const unsigned char kDownArrow [] = {
	0x1b,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x0b,0x1b,0x1b,
	0x1b,0x13,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x16,0x0b,0x13,0x1b,
	0x1b,0x1b,0x13,0x1b,0x1b,0x1b,0x1b,0x16,0x0b,0x13,0x13,0x1b,
	0x1b,0x1b,0x1b,0x13,0x1b,0x1b,0x16,0x0b,0x13,0x13,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x13,0x16,0x0b,0x13,0x13,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x0b,0x13,0x13,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x13,0x1b,0x1b,0x1b,0x1b,0x1b
};

static void
HiliteBitmap(BBitmap* src, BBitmap* dest, color_map* cmap)
{
	uchar* srcbits = (uchar *)src->Bits();
	uchar* destbits = (uchar*)dest->Bits();
	
	if (!srcbits || !destbits) {
		return;
	}
	
	uint8 index;
	for (int32 i = 0; i < (kIconWidth * kIconHeight); i++) {
		index = srcbits[i];
		destbits[i] = GetHiliteColor(cmap, index);
	}
}

static void
DisableBitmap(BBitmap* src, BBitmap* dest)
{
	uchar* srcbits = (uchar *)src->Bits();
	uchar* destbits = (uchar*)dest->Bits();
	
	if (!srcbits || !destbits) {
		return;
	}
	
	uint8 index;
	rgb_color c;
	for (int32 i = 0; i < (kIconWidth * kIconHeight); i++) {
		index = srcbits[i];
		c = ColorForIndex(index);
		c = shift_color(c, 0.5);
		destbits[i] = IndexForColor(c);
	}
}

//

TListSelector::TListSelector(BRect frame, const char* label, BMessage* m)
	: 	BView(frame, label, B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE),
		BInvoker()
{
	SetFont(be_plain_font);
	SetMessage(m);
	
	fSelected = false;
	fEnabled = true;
	fActive = false;
	if (label)
		fLabel = strdup(label);
	else
		fLabel = NULL;
	fSleepTime = 200000;

	fNeedToInvert = false;
	
	BRect r(0,0,11,6);
	fUpArrow = new BBitmap(r, B_COLOR_8_BIT);
	fUpArrow->SetBits(kUpArrow, (kIconWidth*kIconHeight), 0, B_COLOR_8_BIT);
	fHiliteUpArrow = new BBitmap(r, B_COLOR_8_BIT);
	fDisabledUpArrow = new BBitmap(r, B_COLOR_8_BIT);

	fDownArrow = new BBitmap(r, B_COLOR_8_BIT);
	fDownArrow->SetBits(kDownArrow, (kIconWidth*kIconHeight), 0,
		B_COLOR_8_BIT);
	fHiliteDownArrow = new BBitmap(r, B_COLOR_8_BIT);
	fDisabledDownArrow = new BBitmap(r, B_COLOR_8_BIT);
	
	color_map* cmap;
	
	BScreen screen(B_MAIN_SCREEN_ID);
	cmap = (color_map*)screen.ColorMap();

	HiliteBitmap(fUpArrow, fHiliteUpArrow, cmap);
	HiliteBitmap(fDownArrow, fHiliteDownArrow, cmap);
	
	DisableBitmap(fUpArrow, fDisabledUpArrow);
	DisableBitmap(fDownArrow, fDisabledDownArrow);	

	fTargetList = -1;	// nothing active, no lists
	fListCount = 0;
	fList[0] = fList[1] = fList[2] = fList[3] = NULL;
	
	fOffscreenBits = NULL;
	fOffscreenView = NULL;
}


TListSelector::~TListSelector()
{
	if (fLabel)
		free(fLabel);
		
	delete fUpArrow;
	delete fHiliteUpArrow;
	delete fDisabledUpArrow;

	delete fDownArrow;
	delete fHiliteDownArrow;
	delete fDisabledDownArrow;
	
	delete fList[0];			// will delete all contents
	delete fList[1];
	delete fList[2];
	delete fList[3];
	
	delete fOffscreenBits;		// deletes os view
}

void 
TListSelector::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (Parent())
		SetViewColor(Parent()->ViewColor());
		
	if (!Messenger().IsValid()) {
		SetTarget(Window());
		//	need to set targets of lists
	}

	ResizeToPreferred();
}

void 
TListSelector::WindowActivated(bool state)
{
	BView::WindowActivated(state);
	Draw(Bounds());
}

void 
TListSelector::MakeFocus(bool focusState)
{
	BView::MakeFocus(focusState);
	MakeSelected(focusState);
	Draw(Bounds());
}

void
TListSelector::GetPreferredSize(float *width, float *height)
{
	// height
	// 2 pixel border, top
	// 1 pixel space
	// Fontheight(this, true)
	// 1 pixel space
	// 2 pixel border, bottom
	int32 fh = (int32)FontHeight(this, true);
	*height = 2 + 1 + fh + 1 + 2;

	// width	
	// width of label, 5 for gap
	// 2 pixel border, left
	// 1 pixel space, for focus mark
	// 1 pixel space, for focus mark
	// 17 pixel button
	*width = 0;
	if (fLabel && strlen(fLabel) > 0)
		*width = StringWidth(fLabel) + kLabelGap;
	*width += 2 + 1 + 1 + kArrowWidth;

	// 3 pixel pad, longest in list, 3 pixel pad
	// 3 pixel pad, delim, 3 pixel pad
	short listCount = TargetListCount();
	const char* text;
	TSelectionList* list;
	for (short listIndex=0 ; listIndex<listCount ; listIndex++) {
		list = ListAt(listIndex);
		if (list) {
			float maxw=0;
			if (list->IsCounter()) {
				char str[32];
				sprintf(str, "%ld", list->Ceiling());
				int32 max = (strlen(str) < 32) ? strlen(str) : 32;
				str[0] = 0;
				for (int32 i=0 ; i<max ; i++)
					sprintf(str, "%s%i", str, 0);
				maxw = StringWidth(str);
			} else {
				for (int32 i=0 ; i<list->CountItems() ; i++) {
					text = list->ItemAt(i);
					if (StringWidth(text) > maxw)
						maxw = StringWidth(text);
				}
			}
			maxw += 6;
		
			//	if this is not the first list
			//		offset for the delimiter
			if (listIndex > 0) {

				//	get the previous list's delimiter
				list = ListAt(listIndex-1);
				if (list->Delimiter())
					maxw += StringWidth(list->Delimiter())+kDelimiterPad;
			}
			
			*width += maxw;
		}
	}	
}

// attachedtowindow
// setfont
// addlist
// setlabel
void
TListSelector::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);

	if (fOffscreenView) {
		if (fOffscreenBits && fOffscreenBits->Lock()) {
			fOffscreenView->RemoveSelf();
			fOffscreenBits->Unlock();
		}
		delete fOffscreenView;
	}
	fOffscreenView = new BView(Bounds(), "", B_FOLLOW_NONE, 0);
	if (fOffscreenBits) {
		delete fOffscreenBits;
	}
	fOffscreenBits = new BBitmap(Bounds(), B_COLOR_8_BIT, true);
	fOffscreenBits->AddChild(fOffscreenView);

	fOffscreenBits->Lock();
	fOffscreenView->SetViewColor(ViewColor());
	fOffscreenBits->Unlock();
}

void 
TListSelector::Draw(BRect)
{
	if (!fOffscreenBits || !fOffscreenView || !Window() || !Parent())
		return;
		
	PushState();
	
	if (fOffscreenBits->Lock()) {	
		fOffscreenView->SetHighColor(fOffscreenView->ViewColor());
		fOffscreenView->FillRect(Bounds());	

		DrawLabel();
		DrawBevel();
		DrawFocusMark();
		DrawSelection();
		DrawButtons();
		
		fOffscreenView->Sync();
		fOffscreenBits->Unlock();
	}

	DrawBitmap(fOffscreenBits);
	
	PopState();
}

void 
TListSelector::DrawLabel()
{
	if (!fLabel)
	 	return;
	
	int32 fh = (int32)FontHeight(this, true);
	fOffscreenView->MovePenTo(0, Bounds().Height()/2 + fh/2);
	
	fOffscreenView->SetLowColor(fOffscreenView->ViewColor());
	fOffscreenView->SetHighColor(kBlack);
	
	fOffscreenView->DrawString(fLabel);
}

void 
TListSelector::DrawBevel()
{
	BRect bevelFrame(BevelFrame());
	
	fOffscreenView->BeginLineArray(8);

	BPoint pt1 = bevelFrame.RightTop();
	pt1.x -= 1;
	BPoint pt2 = bevelFrame.RightBottom();
	pt2.x -= 1;
	fOffscreenView->AddLine(pt1, pt2, kViewGray);
	pt1 = bevelFrame.LeftBottom();
	pt1.y -= 1;
	pt2.y -= 1;
	fOffscreenView->AddLine(pt1, pt2, kViewGray);
	
	pt1 = bevelFrame.LeftTop();
	pt2 = bevelFrame.RightTop();
	pt1.y +=1; pt2.y += 1;
	fOffscreenView->AddLine(pt1, pt2, kMediumGray);
	pt1.x += 1;
	pt2 = bevelFrame.LeftBottom();
	pt2.x += 1;
	fOffscreenView->AddLine(pt1, pt2, kMediumGray);

	fOffscreenView->AddLine(bevelFrame.LeftTop(),
		bevelFrame.RightTop(), kLightGray);
	fOffscreenView->AddLine(bevelFrame.LeftBottom(),
		bevelFrame.LeftTop(), kLightGray);

	pt1 = bevelFrame.RightTop();
	pt1.y += 1;
	fOffscreenView->AddLine(pt1, bevelFrame.RightBottom(), kWhite);
	pt1 = bevelFrame.LeftBottom();
	pt1.x += 1;
	fOffscreenView->AddLine(pt1, bevelFrame.RightBottom(), kWhite);	

	fOffscreenView->EndLineArray();
}

BRect 
TListSelector::BevelFrame() const
{
	BRect bFrame(Bounds());
	
	//	make space for label
	if (fLabel && strlen(fLabel) > 0)
		bFrame.left = StringWidth(fLabel) + kLabelGap;
	// make space for arrows
	bFrame.right -= kArrowWidth;
	
	return bFrame;
}

void 
TListSelector::DrawFocusMark()
{
	if (IsFocus() && Window()->IsActive()) {
		BRect fRect(BevelFrame());
		fRect.InsetBy(1, 1);
	
		fOffscreenView->SetLowColor(ViewColor());
		fOffscreenView->SetHighColor(keyboard_navigation_color());
		fOffscreenView->StrokeRect(fRect);
	}
}

void 
TListSelector::DrawSelection()
{
	fOffscreenView->SetDrawingMode(B_OP_COPY);
	fOffscreenView->SetHighColor(kBlack);

	float fh = FontHeight(this, true);
	float y = Bounds().Height()/2 + fh/2 - 1;

	rgb_color 	fillColor;
	BRect		selectionFrame;
	for (int32 listIndex=0 ; listIndex<fListCount ; listIndex++) {
		if (Selected() && TargetList()==listIndex) {
			fOffscreenView->SetLowColor(kLightGray);
			fillColor = kLightGray;
		} else {
			fOffscreenView->SetLowColor(kViewGray);
			fillColor = kViewGray;
		}
		fOffscreenView->SetHighColor(fillColor);
		
		selectionFrame = fList[listIndex]->Frame();
		fOffscreenView->FillRect(selectionFrame);	// 	erase the old or
													//	draw the selection
													//		color
									
		const char* str = fList[listIndex]->SelectedText();

		float offset=0;
		alignment listAlignment = fList[listIndex]->Alignment();
		if (listAlignment == B_ALIGN_RIGHT)
			offset = selectionFrame.Width() - StringWidth(str) - 3;
		else if (listAlignment == B_ALIGN_CENTER)
			offset = selectionFrame.Width()/2 - StringWidth(str)/2;
		else
			offset = 3;

		fOffscreenView->MovePenTo(selectionFrame.left+offset, y);
		
		fOffscreenView->SetHighColor(kBlack);		
		if (str)
			fOffscreenView->DrawString(str);
		
		// draw the delimiter for all except the last list
		if (listIndex+1 != fListCount) {
			const char* delimiter = fList[listIndex]->Delimiter();
			if (delimiter && strlen(delimiter) > 0) {
				BRect nextSelectionFrame = fList[listIndex+1]->Frame();
				float x = (	nextSelectionFrame.left -
							selectionFrame.right)/2 -
							StringWidth(delimiter)/2;
				fOffscreenView->MovePenTo(selectionFrame.right + x, y);
				fOffscreenView->SetLowColor(kViewGray);
				fOffscreenView->DrawString(delimiter);
			}
		}
	}
}

void 
TListSelector::DrawButtons()
{
	
	//	draw the arrow bitmaps
	fOffscreenView->SetDrawingMode(B_OP_OVER);
	
	if (!fEnabled)
		fOffscreenView->DrawBitmap(fDisabledUpArrow, UpArrowFrame());	
	else if (fNeedToInvert && Active() && Button() == kUpButton)
		fOffscreenView->DrawBitmap(fHiliteUpArrow, UpArrowFrame());
	else
		fOffscreenView->DrawBitmap(fUpArrow, UpArrowFrame());
	
	if (!fEnabled) 		
		fOffscreenView->DrawBitmap(fDisabledDownArrow, DownArrowFrame());
	else if (fNeedToInvert && Active() && Button() == kDownButton)
		fOffscreenView->DrawBitmap(fHiliteDownArrow, DownArrowFrame());
	else
		fOffscreenView->DrawBitmap(fDownArrow, DownArrowFrame());

	//	draw the bounding box for the arrow controls
	fOffscreenView->BeginLineArray(6);
	
	rgb_color c = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT);
	if (!fEnabled)
		c = shift_color(c, 0.5);
	fOffscreenView->AddLine(BPoint(Bounds().Width()-16, 0),
			BPoint(Bounds().Width()-1, 0), c);
	fOffscreenView->AddLine(BPoint(Bounds().Width()-1, 0),
			BPoint(Bounds().Width()-1, Bounds().Height()-1), c);
	fOffscreenView->AddLine(BPoint(Bounds().Width()-1, Bounds().Height()-1),
			BPoint(Bounds().Width()-16, Bounds().Height()-1), c);

	c.red = 255; c.green = 255; c.blue = 255;
	if (!fEnabled)
		c = shift_color(c, 0.5);
	fOffscreenView->AddLine(BPoint(Bounds().Width()-16, 1),
			BPoint(Bounds().Width(), 1), c);
	fOffscreenView->AddLine(BPoint(Bounds().Width(), 1),
			BPoint(Bounds().Width(), Bounds().Height()), c);
	fOffscreenView->AddLine(BPoint(Bounds().Width(), Bounds().Height()),
			BPoint(Bounds().Width()-16, Bounds().Height()), c);

	fOffscreenView->EndLineArray();
}

void 
TListSelector::MessageReceived(BMessage* m)
{
	switch(m->what) {
		default:
			BView::MessageReceived(m);
			break;
	}
}

void 
TListSelector::PressButton(button which)
{
	if (which == kNoButton)
		return;
		
	if (which == kUpButton) {
		fNeedToInvert = true;
		SetButton(kUpButton);
		SetActive(true);
	} else if (kDownButton) {
		fNeedToInvert = true;
		SetButton(kDownButton);
		SetActive(true);
	}
	
	snooze(50000);
	
	fNeedToInvert = false;
	SetButton(kNoButton);
	SetActive(false);
}

bool
TListSelector::HandleKeyDown()
{
	if (IsFocus() && !Selected()) {
		MakeSelected(true);
		SetTargetList(-1);
	}
		
	if (!IsEnabled() || !Selected())
		return false;
		
	return true;
}

void 
TListSelector::KeyDown(const char *key, int32 numBytes)
{
	if (!HandleKeyDown())
		return;
		
	switch (key[0]) {
		case B_LEFT_ARROW:
			PreviousTargetList();	// based on target list
			break;
		case B_RIGHT_ARROW:
			NextTargetList();		// based on target list
			break;
		case B_UP_ARROW:
			PressButton(kUpButton);
			break;
	 	case B_DOWN_ARROW:
			PressButton(kDownButton);
			break;
		
		case B_SPACE:
			Invoke();
			MakeSelected(false);
			break;
			
		default:
			BView::KeyDown(key,numBytes);
			break;
	}
}

void 
TListSelector::MouseDown(BPoint loc)
{
	if (!fEnabled)  
		return;
	
	if (!IsFocus())
		MakeFocus(true);
	MakeSelected(true);
		
	BRect frame;
	bool found=false;
	for (int32 listIndex=0 ; listIndex<fListCount ; listIndex++) {
		if (fList[listIndex]->Frame().Contains(loc)) {
			SetTargetList(listIndex);		// requests draw
			found = true;
			break;
		}
	}
	if (found)
		return;
	
	if (UpArrowFrame().Contains(loc)) {
		fNeedToInvert = true;
		frame = UpArrowFrame();
		SetButton(kUpButton);
		SetActive(true);						// calls invoke
	} else if (DownArrowFrame().Contains(loc)) {
		fNeedToInvert = true;
		frame = DownArrowFrame();
		SetButton(kDownButton);
		SetActive(true);						// calls invoke
	} else {
		BView::MouseDown(loc);
		SetButton(kNoButton);
		SetActive(false);
		return;
	}

/*	BPoint where;
	uint32 buttons;
	do {
		if (frame.Contains(where))
			SetActive(true);		// calls invoke

		snooze(fSleepTime);
		GetMouse(&where, &buttons);
	} while(buttons);*/
	
	fNeedToInvert = false;
	SetButton(kNoButton);
	SetActive(false);
}

void 
TListSelector::SetEnabled(bool e)
{
	if (fEnabled == e)
		return;
		
	fEnabled = e;
	
	Invalidate();
}

bool 
TListSelector::IsEnabled() const
{
	return fEnabled;
}

void 
TListSelector::SetLabel(const char* l)
{
	if (!l)
		return;
		
	if (fLabel)
		free(fLabel);
		
	fLabel = strdup(l);

	ResizeToPreferred();
	
	Draw(Bounds());
}

const char *
TListSelector::Label() const
{
	return fLabel;
}

status_t
TListSelector::Invoke(BMessage *msg)
{
	if (!msg)
		msg = fList[TargetList()]->Message();
	if (!msg)
		return B_BAD_VALUE;

	BMessage clone(*msg);

	clone.AddInt64("when", system_time());
	clone.AddPointer("source", this);
	clone.AddInt32("list", TargetList());
	clone.AddInt32("index", Selection());
	status_t err = BInvoker::Invoke(&clone);
	return err;
}

void 
TListSelector::SetActive(bool v)
{
	fActive = v;
	
	if (fActive) {
		if (fButton == kUpButton)
			Increment();
		else if (fButton == kDownButton)
			Decrement();
	}
		
	Draw(Bounds());
}

bool 
TListSelector::Active() const
{
	return fActive;
}

void 
TListSelector::SetButton(button which)
{
	if (fButton == which)
		return;
		
	fButton = which;
}

button 
TListSelector::Button() const
{
	return fButton;
}

void 
TListSelector::SetSleepTime(int32 t)
{
	fSleepTime = t;
}

void
TListSelector::MakeSelected(bool s)
{
	if (fSelected == s)
		return;
	
	fSelected = s;
			
	Draw(Bounds());
}

bool
TListSelector::Selected() const
{
	return fSelected;
}

void 
TListSelector::SetSelection(int32 s)
{
	SetSelection(s, TargetList());
}

void 
TListSelector::SetSelection(int32 index, short which)
{
	if (which < 0 || which >= fListCount)
		return;
		
	if (Selection(which) == index)
		return;

	if (index > fList[which]->Ceiling())
		index = fList[which]->Ceiling();
	if (index < fList[which]->Floor())
		index = fList[which]->Floor();

	fList[which]->SetSelection(index);

	Draw(Bounds());
}

int32 
TListSelector::Selection() const
{
	return Selection(TargetList());
}

int32 
TListSelector::Selection(short which) const
{
	if (which < 0 || which >= fListCount)
		return -1;

	return fList[which]->Selection();
}

const char*
TListSelector::SelectedText() const
{
	return SelectedText(TargetList());
}

const char*
TListSelector::SelectedText(short which) const
{
	if (which < 0 || which >= fListCount)
		return NULL;

	return fList[which]->SelectedText();
}

void
TListSelector::SetFloor(int32 f, short w)
{
	if (w < 0 || w >= fListCount)
		return;

	fList[w]->SetFloor(f);		
}

int32
TListSelector::Floor(short w) const
{
	if (w < 0 || w >= fListCount)
		return -1;
		
	return fList[w]->Floor();
}

void
TListSelector::SetCeiling(int32 c, short w)
{
	if (w < 0 || w >= fListCount)
		return;
		
	fList[w]->SetCeiling(c);
}

int32
TListSelector::Ceiling(short w) const
{
	if (w < 0 || w >= fListCount)
		return -1;
		
	return fList[w]->Ceiling();
}

void
TListSelector::SetAlignment(alignment a, short w)
{
	if (w < 0 || w >= fListCount)
		return;
		
	fList[w]->SetAlignment(a);
}

alignment
TListSelector::Alignment(short w) const
{
	if (w < 0 || w >= fListCount)
		return B_ALIGN_LEFT;
		
	return fList[w]->Alignment();
}

void 
TListSelector::Increment()
{
	if (TargetList() == -1)
		return;
	
	if (Selection()+1 > Ceiling(TargetList()))		// autowrap
		SetSelection(Floor(TargetList()));
	else
		SetSelection(Selection() + 1);
	Invoke();		
}

void 
TListSelector::Decrement()
{
	if (TargetList() == -1)
		return;
	
	if (Selection()-1 < Floor(TargetList()))			// autowrap
		SetSelection(Ceiling(TargetList()));
	else
		SetSelection(Selection() - 1);
	Invoke();		
}

void 
TListSelector::AddList(TSelectionList* list)
{
	if (fListCount == kMaxListCount)
		return;
	
	fListCount++;

	fList[fListCount-1] = list;	

	ResizeToPreferred();
	
	// 	scan through list, find longest
	//	expand frame based on string delimter and placement
	const char* text;
	float maxw=0;
	for (int32 i=0 ; i<fList[fListCount-1]->CountItems() ; i++) {
		text = fList[fListCount-1]->ItemAt(i);
		if (StringWidth(text) > maxw)
			maxw = StringWidth(text);
	}
	maxw += 6;
	
	//	make the frame to fit the max string
	BRect frame(BevelFrame());
	frame.left += 2;
	frame.top += 2;
	frame.bottom -= 2;
	frame.right = frame.left + maxw;
	
	//	if this is not the first list
	//		offset for the delimiter
	if (fListCount > 1) {
		float d = 0;
		//	get the previous list's delimiter
		if (fList[fListCount-2]->Delimiter())
			d = StringWidth(fList[fListCount-2]->Delimiter())+kDelimiterPad;
		
		//	get the previous list's frame	
		BRect prevFrame = fList[fListCount-2]->Frame();
		frame.left = prevFrame.right + d;
		frame.right = frame.left + maxw;
	}

	fList[fListCount-1]->SetFrame(frame);	
}

void
TListSelector::AddList(int32 floor, int32 ceiling, char* delimiter,
	BMessage* m)
{
	if (fListCount == kMaxListCount)
		return;
	
	fListCount++;
	fList[fListCount-1] = new TSelectionList(floor, ceiling, delimiter, m);

	ResizeToPreferred();
	
	char str[32];
	sprintf(str, "%i", ceiling);
	int32 max = (strlen(str) < 32) ? strlen(str) : 32;
	str[0] = 0;
	for (int32 i=0 ; i<max ; i++)
		sprintf(str, "%s%ld", str, 0);
	float maxw = StringWidth(str) + 6;
	
	BRect frame(BevelFrame());
	frame.left += 2;
	frame.top += 2;
	frame.bottom -= 2;
	frame.right = frame.left + maxw;
	
	if (fListCount > 1) {
		float d = 0;
		//	get the previous list's delimiter
		if (fList[fListCount-2]->Delimiter())
			d = StringWidth(fList[fListCount-2]->Delimiter())+kDelimiterPad;
			
		BRect prevFrame = fList[fListCount-2]->Frame();
		frame.left = prevFrame.right + d;
		frame.right = frame.left + maxw;
	}

	fList[fListCount-1]->SetFrame(frame);
}

int32 
TListSelector::ItemCount() const
{
	return ItemCount(TargetList());
}

int32 
TListSelector::ItemCount(short which) const
{
	if (which == -1 || which < 0 || which >= fListCount)
		return -1;
	
	return fList[which]->CountItems();	
}

const char *
TListSelector::ItemAt(int32 index) const
{
	return ItemAt(index, TargetList());
}

const char *
TListSelector::ItemAt(int32 index, short which) const
{
	if (which == -1 || which < 0 || which >= fListCount)
		return NULL;
		
	return fList[which]->ItemAt(index);
}

TSelectionList*
TListSelector::ListAt(short index) const
{
	if (index < 0 || index >= fListCount)
		return NULL;
		
	return fList[index];
}

void 
TListSelector::SetTargetList(short which)
{
	if (which < 0 || which >= fListCount)
		return;

	fTargetList = which;
	Draw(Bounds());
}

short 
TListSelector::TargetList() const
{
	if (!Selected())
		return -1;
	else	
		return fTargetList;
}

short
TListSelector::TargetListCount() const
{
	return fListCount;
}

void 
TListSelector::NextTargetList()
{
	Invoke();		

	short t = TargetList();
	if (t == -1)
		SetTargetList(0);
	else if (t == fListCount-1)
		SetTargetList(0);
	else
		SetTargetList(t+1);	
}

void 
TListSelector::PreviousTargetList()
{
	Invoke();		

	short t = TargetList();
	if (t == -1)
		SetTargetList(0);
	else if (t == 0)
		SetTargetList(fListCount-1);
	else
		SetTargetList(t-1);
}

BRect
TListSelector::UpArrowFrame() const
{
	BRect frame(Bounds().Width()-14, 3, Bounds().Width()-3, 9);
	return frame;
}

BRect
TListSelector::DownArrowFrame() const
{
	BRect frame(	Bounds().Width()-14, Bounds().Height()-8,
					Bounds().Width()-3, Bounds().Height()-2);
	return frame;
}
