// ============================================================
//  KouhoWindow.cpp	by Hiroshi Lockheimer
// ============================================================

#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include "KouhoWindow.h"
#include "HenkanManager.h"
#include "KanaKan.h"
#include "BitmapData.h"
#include "JapaneseCommon.h"

#include <Bitmap.h>
#include <Debug.h>
#include <Screen.h>
#include <ScrollBar.h>


#ifdef COUNTVIEW
const float		kCountViewHeight	= 10.0;
#endif

const float		kVWindowOffset		= 10.0;
const float		kHTextInset			= 5.0;
const float		kFontSizeReduction	= 2.0;
const float		kLabelWidth			= 11.0;
const float		kLabelImageInset	= 3.0;
const int32		kMaxKouhosVisible	= 10;
const rgb_color	kBorderColor		= {144, 144, 144, 255};
const rgb_color	kLabelViewColor		= {216, 216, 216, 255};
const rgb_color	kLabelLight			= {255, 255, 255, 255};
const rgb_color kLabelDark			= {184, 184, 184, 255};
const rgb_color	kLabelShadow		= {64, 64, 64, 255};
const rgb_color	kHilightColor		= {255, 152, 152, 255};


KouhoWindow::KouhoWindow(
	BPoint			where,
	float			height,
	HenkanManager	*henkanManager,
	KouhoView		**kouhoView)
		: BWindow(BRect(0.0, 0.0, 100.0, 100.0), B_EMPTY_STRING,
				  B_BORDERED_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
				  B_WILL_ACCEPT_FIRST_CLICK | B_NOT_RESIZABLE | 
				  B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE | B_AVOID_FOCUS)
{
	BRect			bounds = Bounds();
	const KanaKan	*kanaKan = henkanManager->PeekKanaKan();
	int32			numKouhos = kanaKan->CountKouhos(kanaKan->ActiveClause());
	bool			needScroller = numKouhos > kMaxKouhosVisible;

	BRect kouhoViewBounds = bounds;
	kouhoViewBounds.left = kLabelWidth + 1.0;
#ifdef COUNTVIEW
	kouhoViewBounds.bottom -= kCountViewHeight + 1.0;
#endif
	if (needScroller)
		kouhoViewBounds.right -= B_V_SCROLL_BAR_WIDTH - 1.0;
	*kouhoView = new KouhoView(kouhoViewBounds, henkanManager);
	
	if (needScroller) {
		BRect scrollBarBounds = bounds;
		scrollBarBounds.OffsetBy(1.0, 0.0);
		scrollBarBounds.InsetBy(0.0, -1.0);
		scrollBarBounds.left = scrollBarBounds.right - B_V_SCROLL_BAR_WIDTH + 1.0;
		AddChild(new BScrollBar(scrollBarBounds, B_EMPTY_STRING, *kouhoView, 
								0.0, 0.0, B_VERTICAL));
	}

#ifdef COUNTVIEW
	CountView *countView = new CountView(BRect(0.0, 0.0, 0.0, 0.0), 0, numKouhos);
	(*kouhoView)->SetCountView(countView);
#endif

	// add KouhoView after the ScrollBar so that it can fixup the scrollers
	AddChild(*kouhoView);	

	// re-get the bounds, KouhoView has changed it
	bounds = Bounds();

	// window gets moved here, resized in KouhoView::AttachedToWindow()
	BRect	screenFrame = BScreen().Frame();
	bool	bottomUp = false;

	if (kVWindowOffset + where.y + height + bounds.Height() <= screenFrame.bottom) 
		MoveTo(where.x - kLabelWidth, kVWindowOffset + where.y + height);
	else {
		MoveTo(where.x - kLabelWidth, where.y - bounds.Height() - kVWindowOffset);
		bottomUp = true;
	}
	(*kouhoView)->SetBottomUp(bottomUp);

	// add LabelView and CountView after KouhoView so that they get the correct window size
	BRect labelViewBounds = bounds;
	labelViewBounds.right = labelViewBounds.left + kLabelWidth;	
#ifdef COUNTVIEW
	labelViewBounds.bottom -= kCountViewHeight + 1.0;
#endif
	LabelView *labelView = new LabelView(labelViewBounds, (needScroller) ? kMaxKouhosVisible : numKouhos, 
										 ceil((*kouhoView)->ItemFrame(0).Height()), bottomUp);
	AddChild(labelView);

#ifdef COUNTVIEW
	if (bottomUp) {
		(*kouhoView)->MoveBy(0.0, kCountViewHeight + 1.0);
		labelView->MoveBy(0.0, kCountViewHeight + 1.0);
	}

	BRect countViewBounds = bounds;
	if (bottomUp)
		countViewBounds.bottom = countViewBounds.top + kCountViewHeight;
	else
		countViewBounds.top = countViewBounds.bottom - kCountViewHeight;
	countViewBounds.right++;
	countView->MoveTo(countViewBounds.LeftTop());
	countView->ResizeTo(countViewBounds.Width(), countViewBounds.Height());
	AddChild(countView);
#endif
}


KouhoView::KouhoView(
	BRect			frame,
	HenkanManager	*henkanManager)
		: BListView(frame, B_EMPTY_STRING, B_SINGLE_SELECTION_LIST, 
					B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE)
{
	fHenkanManager = henkanManager;
	fKanaKan = fHenkanManager->PeekKanaKan();
	fNoNotification = false;
	fHasScroller = false;
	fBottomUp = false;
#ifdef COUNTVIEW
	fCountView = NULL;
#endif

	SetMessage(new BMessage(B_CONTROL_INVOKED));
	
	BFont theFont(be_plain_font);
	theFont.SetSize(12.0);
	SetFont(&theFont);
}

void
KouhoView::DetachedFromWindow()
{
	BListView::DetachedFromWindow();
//	if(fHenkanManager->fExternalListview.IsValid())
//	{
//		BMessage remote(J_SET_KOUHO_LISTVIEW);
//		fHenkanManager->fExternalListview.SendMessage(&remote);
//	}
}

void
KouhoView::AttachedToWindow()
{
	BListView::AttachedToWindow();

	SetTarget(this);
	
	fHasScroller = ScrollBar(B_VERTICAL) != NULL;

	float	width = 0.0;
	float	height = 0.0;
	int32	activeClause = fKanaKan->ActiveClause();
	int32	numKouhos = fKanaKan->CountKouhos(activeClause);

	BMessage remote(J_SET_KOUHO_LISTVIEW);
	for (int32 i = 0; i < numKouhos; i++) {
		const char *kouhostring=fKanaKan->KouhoAt(activeClause, i);
		KouhoItem *item = new KouhoItem(kouhostring, i < 2, (numKouhos > 2) && (i == 1));
		AddItem(item);
		remote.AddString("kouho",kouhostring);

		float itemWidth = item->Width();
		width = (itemWidth > width) ? itemWidth : width;
	}
//	if (fHenkanManager->fExternalListview.IsValid())
//	{
//		// update the remote view
//		fHenkanManager->fExternalListview.SendMessage(&remote);
//	}

	SelectKouho(fKanaKan->SelectedKouho(activeClause), false);

	int32	numKouhosVisible = (numKouhos > kMaxKouhosVisible) ? kMaxKouhosVisible : numKouhos;
	BWindow	*window = Window();
	BRect	wBounds = window->Bounds();
	window->ResizeBy(kLabelWidth + (width + (fHasScroller ? B_V_SCROLL_BAR_WIDTH : 0.0)) - wBounds.Width(), 
#ifndef COUNTVIEW
					 (ItemFrame(0).Height() * numKouhosVisible) + (numKouhosVisible - 1) - wBounds.Height());
#else
					 kCountViewHeight + (ItemFrame(0).Height() * numKouhosVisible) + (numKouhosVisible - 1) - wBounds.Height());
#endif

	MakeFocus();
}


void
KouhoView::SelectionChanged()
{
#ifdef COUNTVIEW
	fCountView->SetCurCount((fBottomUp) ? CountItems() - CurrentSelection() : CurrentSelection() + 1);
#endif

	if (fNoNotification)
		return;

	int32 num=fBottomUp ? (fKanaKan->CountKouhos(fKanaKan->ActiveClause()) - 1) - CurrentSelection() : CurrentSelection();
	fHenkanManager->KouhoChanged(num);

//	if(fHenkanManager->fExternalListview.IsValid())
//	{
//		BMessage mes(J_SET_KOUHO_LIST_SELECTION);
//		mes.AddInt32("selectionindex",num);
//		fHenkanManager->fExternalListview.SendMessage(&mes);
//	}
}


void
KouhoView::MessageReceived(
	BMessage	*message)
{
	if( message->what == B_CONTROL_INVOKED ) {
		fHenkanManager->StopKouhoWindow();
		return;
	}
	
	BListView::MessageReceived(message);
}


int32
KouhoView::SelectKouho(
	int32	kouho,
	bool	visible)
{
	BWindow *window = Window();

	if (!window->Lock())
		return (0);

	int32 numItems = CountItems();
	int32 index = (fBottomUp) ? (numItems - 1) - kouho : kouho;
	if (visible) {
		BRect bounds = Bounds();

		if (fBottomUp) {
			for (index = numItems - 1; index >= 0; index--) {
				if (ItemFrame(index).Intersects(bounds))
					break; 
			}
			index -= kouho;
		}
		else {
			for (index = 0; index < numItems; index++) {
				if (ItemFrame(index).Intersects(bounds))
					break;
			}		
			index += kouho;
		}
	}

	index = (index < 0) ? 0 : index;
	index = (index >= numItems) ? numItems - 1 : index;

	fNoNotification = true;
	Select(index);
	fNoNotification = false;

//	if(fHenkanManager->fExternalListview.IsValid())
//	{
//		BMessage mes(J_SET_KOUHO_LIST_SELECTION);
//		mes.AddInt32("selectionindex",index);
//		fHenkanManager->fExternalListview.SendMessage(&mes);
//	}

	if (fHasScroller)
		ScrollToSelection();

	// sync the drawing just in case...
	window->Sync();
	window->Unlock();

	return ((fBottomUp) ? (numItems - 1) - index : index);
}


void
KouhoView::SetBottomUp(
	bool	bottomUp)
{
	if (bottomUp == fBottomUp)
		return;

	int32 numItems = CountItems();
	int32 numIters = numItems / 2;
	int32 lastIndex = numItems - 1;

	for (int32 i = 0; i < numIters; i++)
		SwapItems(i, lastIndex - i);
	ScrollToSelection();

	fBottomUp = bottomUp;
}


bool
KouhoView::IsBottomUp() const
{
	return (fBottomUp);
}


#ifdef COUNTVIEW
void
KouhoView::SetCountView(
	CountView	*countView)
{
	fCountView = countView;
}
#endif


KouhoItem::KouhoItem(
	const char	*item,
	bool		small,
	bool		border)
		: BListItem()
{
	fItem = strdup(item);
	fTextBaseline = 0.0;
	fSmall = small;
	fBorder = border;
}


KouhoItem::~KouhoItem()
{
	free(fItem);
}


void
KouhoItem::DrawItem(
	BView	*owner, 
	BRect	bounds, 
	bool	complete)
{	
	rgb_color	saveLow = owner->LowColor();
	bool		selected = IsSelected();

	if ((complete) || (selected)) {
		if (selected)
			owner->SetLowColor(kHilightColor);
		owner->FillRect(bounds, B_SOLID_LOW);
	}

	if (fBorder) {
		rgb_color saveHigh = owner->HighColor();
		
		owner->SetHighColor(kBorderColor);

		bool 	reverse = dynamic_cast<KouhoView *>(owner)->IsBottomUp();
		BPoint	a((reverse) ? bounds.LeftTop() : bounds.LeftBottom());
		BPoint	b((reverse) ? bounds.RightTop() : bounds.RightBottom());
		owner->StrokeLine(a, b);

		owner->SetHighColor(saveHigh);
	}

	owner->MovePenTo(bounds.left + kHTextInset, bounds.top + fTextBaseline);
	if (!fSmall) 
		owner->DrawString(fItem);
	else {
		BFont font;
		owner->GetFont(&font);
		owner->SetFontSize(font.Size() - kFontSizeReduction);
		owner->DrawString(fItem);
		owner->SetFontSize(font.Size());
	}

	owner->SetLowColor(saveLow);
}


void
KouhoItem::Update(
	BView		*/*owner*/, 
	const BFont	*font)
{
	BFont theFont(font);

	font_height fHeight;
	theFont.GetHeight(&fHeight);

	fTextBaseline = ceil(fHeight.ascent);
	
	SetHeight(ceil(fHeight.ascent + fHeight.descent + fHeight.leading));
	
	// adjust only the width, the height should be equal for all items
	if (fSmall)
		theFont.SetSize(theFont.Size() - kFontSizeReduction);
	SetWidth(kHTextInset + ceil(theFont.StringWidth(fItem)) + kHTextInset);
}


LabelView::LabelView(
	BRect	frame,
	int32	numKouhos,
	int32	itemHeight,
	bool	bottomUp)
		: BView(frame, B_EMPTY_STRING, B_FOLLOW_NONE, B_WILL_DRAW)
{
	BRect bitmapRect(0.0, 0.0, kLabelImagesWidth - 1.0, ceil(frame.Height()));
	fBitmap = new BBitmap(bitmapRect, B_COLOR_8_BIT);

	uchar	*bits = (uchar *)fBitmap->Bits();
	int32	rowBytes = fBitmap->BytesPerRow();
	int32	imageVInset = (((itemHeight - kLabelImagesHeight) / 2) + 1) * rowBytes;

	memset(bits, BScreen().IndexForColor(B_TRANSPARENT_32_BIT), fBitmap->BitsLength());

	for (int32 i = 0; i < numKouhos; i++) {
		const uchar	*image = kLabelImages[(bottomUp) ? (numKouhos - 1) - i : i];
		uchar		*curBits = bits + (i * ((itemHeight + 1) * rowBytes)) + imageVInset; 

		for (int32 j = 0; j < kLabelImagesHeight; j++)
			memcpy(curBits + (j * rowBytes), image + (j * kLabelImagesWidth), kLabelImagesWidth);
	}

	fTrackWhere = NULL;

	SetViewColor(kLabelViewColor);
}


LabelView::~LabelView()
{
	delete (fBitmap);
	delete (fTrackWhere);
}


void
LabelView::Draw(
	BRect	/*updateRect*/)
{
	BRect bounds = Bounds();

	BPoint rightTop = bounds.RightTop() - BPoint(1.0, 0.0);
	BPoint rightBottom = bounds.RightBottom() - BPoint(1.0, 0.0);

	SetDrawingMode(B_OP_COPY);
	BeginLineArray(5);
	AddLine(bounds.LeftBottom(), bounds.LeftTop(), kLabelLight);
	AddLine(bounds.LeftTop(), rightTop, kLabelLight);
	AddLine(rightTop, rightBottom, kLabelDark);
	AddLine(rightBottom, bounds.LeftBottom(), kLabelDark);
	AddLine(bounds.RightTop(), bounds.RightBottom(), kLabelShadow);
	EndLineArray();

	SetDrawingMode(B_OP_OVER);
	DrawBitmapAsync(fBitmap, BPoint(kLabelImageInset, 0.0));
}

#if 0    
extern "C" int _sPrintf(const char *, ...);


void
LabelView::MouseDown(
	BPoint	where) 
{
	MouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	fTrackWhere = new BPoint(where);
_sPrintf("mousedown\n");
}


void
LabelView::MouseUp(
	BPoint	where)
{
	delete (fTrackWhere);
	fTrackWhere = NULL;
_sPrintf("mouseup\n");
}


void
LabelView::MouseMoved(
	BPoint			where, 
	uint32			code, 
	const BMessage	*message)
{
	if (fTrackWhere == NULL)
		return;

	BPoint whereDelta = where - *fTrackWhere;
	if (whereDelta != B_ORIGIN) {
		Window()->MoveBy(whereDelta.x, whereDelta.y);
		Window()->Sync();
	}
_sPrintf("mousemoved %f %f %f %f\n", where.x, where.y, whereDelta.x, whereDelta.y);
}
#endif


#ifdef COUNTVIEW
CountView::CountView(
	BRect	frame, 
	int32	curCount,
	int32	totalCount)
		: BView(frame, B_EMPTY_STRING, B_FOLLOW_NONE, B_WILL_DRAW)
{
	fTotalCount = totalCount;

	char countStr[128] = "";
	sprintf(countStr, "%ld/%ld", totalCount);

	BRect bitmapRect(0.0, 0.0, (kLabelImagesWidth * strlen(countStr)) - 1.0, kLabelImagesHeight);
	fBitmap = new BBitmap(bitmapRect, B_COLOR_8_BIT);

	SetViewColor(kLabelViewColor);

	SetCurCount(curCount);
}


CountView::~CountView()
{
	delete (fBitmap);
}


void
CountView::Draw(
	BRect	updateRect)
{
	BRect bounds = Bounds();

	BPoint rightTop = bounds.RightTop() - BPoint(1.0, 0.0);
	BPoint rightBottom = bounds.RightBottom() - BPoint(1.0, 0.0);

	SetDrawingMode(B_OP_COPY);
	BeginLineArray(5);
	AddLine(bounds.LeftBottom(), bounds.LeftTop(), kLabelLight);
	AddLine(bounds.LeftTop(), rightTop, kLabelLight);
	AddLine(rightTop, rightBottom, kLabelDark);
	AddLine(rightBottom, bounds.LeftBottom(), kLabelDark);
	AddLine(bounds.RightTop(), bounds.RightBottom(), kLabelShadow);
	EndLineArray();

	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fBitmap, BPoint(kLabelImageInset, 0.0));
}


void
CountView::SetCurCount(
	int32	curCount)
{	
	const int32 kAlphaToLabel[] = {10, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8};
	const int32 kTableOffset = 0x2F;

	char countStr[128] = "";
	sprintf(countStr, "%ld/%ld", curCount, fTotalCount);
	
	int32	numDigits = strlen(countStr);
	uchar	*bits = (uchar *)fBitmap->Bits();
	int32	rowBytes = fBitmap->BytesPerRow();

	memset(bits, BScreen().IndexForColor(B_TRANSPARENT_32_BIT), fBitmap->BitsLength());

	for (int32 i = 0; i < numDigits; i++) {
		const uchar	*image = kLabelImages[kAlphaToLabel[countStr[i] - kTableOffset]];
		uchar		*curBits = bits + (i * (kLabelImagesWidth + 1)); 

		for (int32 j = 0; j < kLabelImagesHeight; j++)
			memcpy(curBits + (j * rowBytes), image + (j * kLabelImagesWidth), kLabelImagesWidth);
	}

	if (Window() != NULL)
		Invalidate();
}
#endif
