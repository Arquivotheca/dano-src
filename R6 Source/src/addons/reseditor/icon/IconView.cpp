// (c) 1997, Be Incorporated
//
//

#include <Application.h>
#include <Clipboard.h>
#include <Message.h>

#include <stdio.h>
#include <string.h>

#include <Debug.h>

#include "IconView.h"
//#include "FatBitDefs.h"

const BRect kLargeIconRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1);
const BRect kMiniIconRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1);

const rgb_color kViewGray = { 216, 216, 216, 255};
const rgb_color kWhite = { 255, 255, 255, 255};
const rgb_color kThinGray = { 245, 245, 245, 255};
const rgb_color kBlack = { 0, 0, 0, 255};
const rgb_color kDarkGray = { 160, 160, 160, 255};
const rgb_color kMediumGray = { 190, 190, 190, 255};
const rgb_color kLightGray = { 216, 216, 216, 255};

const char *kLargeIconMimeType = "icon/large";
const char *kMiniIconMimeType = "icon/mini";

IconView::IconView(BRect frame, const char *name, BBitmap *largeIcon,
	BBitmap *miniIcon, bool owning, uint32 resizeFlags, uint32 flags)
	:	BView(frame, name, resizeFlags, flags),
		largeBitmap(0),
		miniBitmap(0),
		owning(owning)
{
	SetLargeIcon(largeIcon);
	SetMiniIcon(miniIcon);
	SetViewColor(kLightGray);
}

IconView::~IconView()
{
	if (owning) {
		delete largeBitmap;
		delete miniBitmap;
	}
}

void IconView::KeyDown(const char *bytes, int32 numBytes)
{
	switch (bytes[0]) {
	case B_DELETE:
	case B_BACKSPACE:
		Clear();
		break;	

	default:
		BView::KeyDown(bytes, numBytes);
		break;
	}
}

const BBitmap *
IconView::LargeIcon() const
{
	return largeBitmap;
}

const BBitmap *
IconView::MiniIcon() const
{
	return miniBitmap;
}

BBitmap *
IconView::LargeIcon()
{
	return largeBitmap;
}

BBitmap *
IconView::MiniIcon()
{
	return miniBitmap;
}

void 
IconView::CutPasteSetLargeIcon(BBitmap *bitmap)
{
	// plug in undo buffering here
	SetLargeIcon(bitmap);
}

void 
IconView::CutPasteSetMiniIcon(BBitmap *bitmap)
{
	// plug in undo buffering here
	SetMiniIcon(bitmap);
}

void 
IconView::SetLargeIcon(BBitmap *newBitmap)
{
	if (!owning || newBitmap != largeBitmap) {
		if (owning)
			delete largeBitmap;			
		largeBitmap = newBitmap;
		
		//if (Window()->Lock()) {
			BRect bounds(Bounds());
			bounds.InsetBy(-2, -2);
			Invalidate(bounds);
	//		Window()->Unlock();
		//}
	}
	else if (owning)
		delete newBitmap;
}

void 
IconView::SetMiniIcon(BBitmap *newBitmap)
{
	if (!owning || newBitmap != miniBitmap) {
		if (owning)
			delete miniBitmap;
		miniBitmap = newBitmap;
		
		//if (Window()->Lock()) {
			BRect bounds(Bounds());
			bounds.InsetBy(-2, -2);
			Invalidate(bounds);
		//	Window()->Unlock();
		//}
	}
	else if (owning)
		delete newBitmap;
}

void
IconView::Draw(BRect rect)
{
	DrawIconItem(rect, LargeIcon());
}

void 
IconView::MakeFocus(bool focusState)
{
	inherited::MakeFocus(focusState);
	Invalidate();
}

void 
IconView::MouseDown(BPoint where)
{
	bool wasFocused = IsFocus();
	if ((Flags() & B_NAVIGABLE) && !wasFocused)
		MakeFocus(true);
	
	uint32 buttons;
	BPoint mousePoint;
	
	for (int32 count = 0; ; count++) {
		GetMouse(&mousePoint, &buttons);
		if (!buttons)
			break;
		
		// if moved or pressed for a bit, try dragging
		if ((mousePoint != where || count > 5) && InitiateDrag(wasFocused))
			return;

		snooze(100000);
	}
}

bool 
IconView::InitiateDrag(bool)
{
	if (!LargeIcon() && !MiniIcon())
		// nothing to drag
		return false;

	BMessage iconContainer;
	
	GetLargeIcon(&iconContainer);
	GetMiniIcon(&iconContainer);

	be_app->SetCursor(B_HAND_CURSOR);
	DragMessage(&iconContainer, Bounds());
	return true;
}

void
IconView::MessageReceived(BMessage *message)
{
	// handle drops
	if (message->WasDropped()) {
		if (AcceptsDrop(message) && SetIcons(message))
			return;
	}
	switch (message->what) {
		case B_CUT:
			Cut();
			break;
		case B_COPY:
			Copy();
			break;
		case B_PASTE:
			Paste();
			break;
		//case B_CLEAR:
		//	Clear();
		//	break;
	}
	inherited::MessageReceived(message);
}

void 
IconView::DrawIconItem(BRect, const BBitmap *bitmap)
{
	BRect bounds(Bounds());
	
	if (IsFocus()) {
		SetHighColor(keyboard_navigation_color());
		StrokeRect(bounds);

		BeginLineArray(4);

	} else {
	
		BeginLineArray(8);

		AddLine(bounds.LeftBottom(), bounds.LeftTop(), kMediumGray);
		AddLine(bounds.LeftTop(), bounds.RightTop(), kMediumGray);
		AddLine(bounds.RightTop(), bounds.RightBottom(), kWhite);
		AddLine(bounds.RightBottom(), bounds.LeftBottom(), kWhite);
	
	}

	bounds.InsetBy(1, 1);
	
	AddLine(bounds.LeftBottom(), bounds.LeftTop(), kDarkGray);
	AddLine(bounds.LeftTop(), bounds.RightTop(), kDarkGray);
	AddLine(bounds.RightTop(), bounds.RightBottom(), kLightGray);
	AddLine(bounds.RightBottom(), bounds.LeftBottom(), kLightGray);

	EndLineArray();
	
	if (bitmap) {
		bounds.InsetBy(1, 1);
		SetDrawingMode(B_OP_OVER);
		SetLowColor(B_TRANSPARENT_32_BIT);
		DrawBitmap(bitmap, bounds);
	}
}

#if DEBUG

static void
DumpBitmap(const BBitmap *bitmap)
{
	if (!bitmap){
		printf("NULL bitmap passed to DumpBitmap\n");
		return;
	}
	int32 length = bitmap->BitsLength();
	
	const unsigned char *bitPtr = (const unsigned char *)bitmap->Bits();
	for (; length >= 0; length--) {
		for (int32 columnIndex = 0; columnIndex < 32; 
			columnIndex++, length--)
			printf("%c%c", "0123456789ABCDEF"[(*bitPtr)/0x10],
				"0123456789ABCDEF"[(*bitPtr++)%0x10]);
				
		printf("\n");
	}
	printf("\n");
}

#endif

void
IconView::Clear()
{
	CutPasteSetLargeIcon(NULL);
	CutPasteSetMiniIcon(NULL);
}

void 
IconView::Cut()
	{
	Copy();
	Clear();
	}

void 
IconView::Paste()
{
	if (!AcceptsPaste())
		return;

	be_clipboard->Lock();
	
	BMessage *message = be_clipboard->Data();
	SetIcons(message);
	
	be_clipboard->Unlock();
}

bool
IconView::SetIcons(BMessage *message)
{
	BBitmap *largeIcon = NULL;
	BBitmap *miniIcon = NULL;
	
	BMessage iconMessage;
	status_t err = message->FindMessage(kLargeIconMimeType, &iconMessage);
	if (!err)
		largeIcon = (BBitmap*) BBitmap::Instantiate(&iconMessage);
	

	err = message->FindMessage(kMiniIconMimeType, &iconMessage);
	if (!err) 
		miniIcon = (BBitmap*) BBitmap::Instantiate(&iconMessage);

	if (!largeIcon || !miniIcon)
		ImportRawBits(&largeIcon, &miniIcon, message);
		
	if (largeIcon)
		CutPasteSetLargeIcon(largeIcon);

	if (miniIcon)
		CutPasteSetMiniIcon(miniIcon);
	
	return largeIcon || miniIcon;
}

void 
IconView::Copy()
{
	be_clipboard->Lock();
	be_clipboard->Clear();
	
	BMessage *message = be_clipboard->Data();

	GetLargeIcon(message);
	GetMiniIcon(message);

	be_clipboard->Commit();
	be_clipboard->Unlock();
}

bool 
IconView::AcceptsPaste() const
{
	be_clipboard->Lock();	
	BMessage *message = be_clipboard->Data();
	bool result = AcceptsCommon(message);
	be_clipboard->Unlock();

	return result;
}

bool 
IconView::AcceptsDrop(const BMessage *message) const
{
	return AcceptsCommon(message)
		|| 	AcceptsRawBits(message);
}

bool 
IconView::AcceptsCommon(const BMessage *message) const
{
	bool result = message->HasMessage(kLargeIconMimeType);
	result |= message->HasMessage(kMiniIconMimeType);
	// add checks for any types that we handle in paste
	return result;
}

void 
IconView::GetLargeIcon(BMessage *message)
{
	if (!LargeIcon())
		return;
	
	BMessage *embeddedIcon = new BMessage();
	LargeIcon()->Archive(embeddedIcon);
	status_t err = message->AddMessage(kLargeIconMimeType, embeddedIcon);
	ASSERT(err == B_OK);
	(void)err;
}

void 
IconView::GetMiniIcon(BMessage *message)
{
	if (!MiniIcon())
		return;
	
	BMessage *embeddedIcon = new BMessage();
	MiniIcon()->Archive(embeddedIcon);
	status_t err = message->AddMessage(kMiniIconMimeType, embeddedIcon);	
	ASSERT(err == B_OK);
	(void)err;
}

const ssize_t kRaw24bit32x32Size = ssize_t((kLargeIconRect.Width() + 1) * 
	(kLargeIconRect.Height() + 1) * 3);
const ssize_t kRaw24bit16x16Size = ssize_t((kMiniIconRect.Width() + 1) * 
	(kMiniIconRect.Height() + 1) * 3);

const ssize_t kRaw8bit32x32Size = ssize_t((kLargeIconRect.Width() + 1) * 
	(kLargeIconRect.Height() + 1));
const ssize_t kRaw8bit16x16Size = ssize_t((kMiniIconRect.Width() + 1) * 
	(kMiniIconRect.Height() + 1));

bool 
IconView::AcceptsRawBits(const BMessage *message) const
{
	if (message->HasRef("refs")) {

		entry_ref ref;
		message->FindRef("refs", &ref);
		
		BFile file(&ref, O_RDWR);
		if (file.InitCheck() != B_NO_ERROR)
			return false;

		off_t size;
		file.GetSize(&size);

		PRINT(("raw bits %s%s\n",
			size == kRaw24bit32x32Size ? "large icon" : "",
			size == kRaw24bit16x16Size ? "mini icon" : ""));
		
		return size == kRaw24bit32x32Size || size == kRaw24bit16x16Size;
	}
	return false;
}

static void 
MakeTransparent(char* pixmap_shared, const char* the_mask, const BRect* r);

void 
IconView::ImportRawBits(BBitmap **large, BBitmap **mini, 
	const BMessage *message)
{
	if (message->HasRef("refs")) {
		entry_ref ref;
		message->FindRef("refs", &ref);
		BFile file(&ref, O_RDWR);
		if (file.InitCheck() != B_NO_ERROR)
			return;

		bool foundMask = false;
		entry_ref maskRef(ref);
		char nameBuffer[255];
		sprintf(nameBuffer, "%s.mask", ref.name);
		maskRef.set_name(nameBuffer);
		
		BFile maskFile(&maskRef, O_RDWR);

		if (maskFile.InitCheck() == B_NO_ERROR) {
			printf("found mask file\n");
			foundMask = true;
		} else
			printf(" no mask file %s found\n", maskRef.name);

		off_t size;
		file.GetSize(&size);

		char *buffer = new char [size];
		char *maskBuffer = NULL;
		if (foundMask)
			maskBuffer = new char [size];
			
		ssize_t numRead = file.Read(buffer, size);
		ASSERT(numRead == size);
		(void)numRead;

		if (maskFile.Read(maskBuffer, size) != size)
			foundMask = false;

		if (size == kRaw24bit32x32Size) {

			(*large) = new BBitmap(kLargeIconRect, B_COLOR_8_BIT);
			(*large)->SetBits(buffer, size, 0, B_RGB_32_BIT);

			if (foundMask) {
				memcpy(buffer, (*large)->Bits(), kRaw8bit32x32Size);
				MakeTransparent(buffer, maskBuffer, &kLargeIconRect);
				(*large)->SetBits(buffer, kRaw8bit32x32Size, 0, B_COLOR_8_BIT);
			}
		} else {
			(*mini) = new BBitmap(kMiniIconRect, B_COLOR_8_BIT);
			(*mini)->SetBits(buffer, size, 0, B_RGB_32_BIT);

			if (foundMask) {
				memcpy(buffer, (*mini)->Bits(), kRaw8bit16x16Size);

				MakeTransparent(buffer, maskBuffer, &kMiniIconRect);
				(*mini)->SetBits(buffer, kRaw8bit16x16Size, 0, B_COLOR_8_BIT);
			}
		}
		delete [] buffer;
		delete [] maskBuffer;
	}
}

static void 
Large8bitIconApply24bitMask(uchar* pixmap_shared, const uchar* the_mask) 
{
	long	i;
	long	j;
	ulong	tmp;
	const ulong	WHITE = 0xffffff00;

	for (i = 0; i < 32; i++) {
		for (j = 0; j < 32; j++) {
			tmp = *((ulong*)the_mask);
			if ((tmp & WHITE) == WHITE)
				*pixmap_shared = B_TRANSPARENT_8_BIT;
				
			pixmap_shared++;
			the_mask += 3;
		}
	}
}

static void 
Mini8bitIconApply24bitMask(uchar* pixmap_shared, const uchar* the_mask) 
{
	long	i;
	long	j;
	ulong	tmp;
	const ulong	WHITE = 0xffffff00;

	for (i = 0; i < 16; i++) {
		for (j = 0; j < 16; j++) {
			tmp = *((ulong*)the_mask);
			if ((tmp & WHITE) == WHITE) 
				*pixmap_shared = B_TRANSPARENT_8_BIT;

			pixmap_shared++;
			the_mask += 3;
		}
	}
}

static void 
MakeTransparent(char* pixmap_shared, const char* the_mask, const BRect* r) 
{
	PRINT(("using %s mask\n", r->Width() == 31 ? "large" : "small"));
	if (r->Width() == 31)
		Large8bitIconApply24bitMask((uchar *)pixmap_shared, (uchar *)the_mask);
	else
		Mini8bitIconApply24bitMask((uchar *)pixmap_shared, (uchar *)the_mask);
}
