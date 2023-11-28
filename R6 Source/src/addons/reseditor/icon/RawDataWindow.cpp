#include <Debug.h>

#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TextControl.h>
#include <View.h>
#include <Autolock.h>
#include <List.h>

#include <math.h>
#include <limits.h>

#include "RawDataWindow.h"
#include "ArrayParser.h"
#include "utils.h"

static bool free_item_func(void* item)
{
	delete reinterpret_cast<TRawDataItem*>(item);
	return true;
}

// ------------------------------ TRawDataItem ------------------------------

TRawDataItem::TRawDataItem(const void* data, size_t size,
						   size_t bytes_per_entry,
						   const char* identifier,
						   const BMessage& meta_data)
	: fBytesPerEntry(bytes_per_entry), fIdentifier(identifier),
	  fMetaData(meta_data),
	  fWidth(0), fGuessWidth(0),
	  fHeight(0), fGuessHeight(0),
	  fColorSpace(B_NO_COLOR_SPACE), fGuessColorSpace(B_NO_COLOR_SPACE),
	  fIsCursor(false), fGuessIsCursor(false), fHaveGuessed(false)
{
	SetSize(size);
	WriteAt(0, data, size);
}

TRawDataItem::~TRawDataItem()
{
}

BBitmap* TRawDataItem::MakeBitmap() const
{
	BBitmap* bitmap = 0;
	if( ColorSpace() == B_RGB24 ) {
		bitmap = new BBitmap(BRect(0, 0, Width()-1, Height()-1), B_RGB32);
		uint8* dest = (uint8*)bitmap->Bits();
		uint8* dest_end = dest + bitmap->BitsLength();
		uint8* src = (uint8*)Buffer();
		uint8* src_end = src + BufferLength();
		while( (dest+3) < dest_end && (src+2) < src_end ) {
			*dest++ = *src++;
			*dest++ = *src++;
			*dest++ = *src++;
			*dest++ = 255;
		}
		return bitmap;
	}
	
	if( ColorSpace() == B_RGB15_BIG || ColorSpace() == B_RGBA15_BIG
			|| ColorSpace() == B_RGB15_BIG ) {
		bitmap = new BBitmap(BRect(0, 0, Width()-1, Height()-1),
							 (color_space)(ColorSpace() & (~0x1000)));
		uint8* dest = (uint8*)bitmap->Bits();
		uint8* dest_end = dest + bitmap->BitsLength();
		uint8* src = (uint8*)Buffer();
		uint8* src_end = src + BufferLength();
		while( (dest+1) < dest_end && (src+1) < src_end ) {
			*(dest+1) = *src;
			*dest = *(src+1);
			dest += 2;
			src += 2;
		}
		return bitmap;
	}
	
	if( ColorSpace() == B_RGB24_BIG ) {
		bitmap = new BBitmap(BRect(0, 0, Width()-1, Height()-1), B_RGB32);
		uint8* dest = (uint8*)bitmap->Bits();
		uint8* dest_end = dest + bitmap->BitsLength();
		uint8* src = (uint8*)Buffer();
		uint8* src_end = src + BufferLength();
		while( (dest+3) < dest_end && (src+2) < src_end ) {
			*dest++ = *(src+2);
			*dest++ = *(src+1);
			*dest++ = *src;
			*dest++ = 255;
			src += 3;
		}
		return bitmap;
	}
	
	if( ColorSpace() == B_RGB32_BIG || ColorSpace() == B_RGBA32_BIG ) {
		bitmap = new BBitmap(BRect(0, 0, Width()-1, Height()-1),
							 (color_space)(ColorSpace() & (~0x1000)));
		uint8* dest = (uint8*)bitmap->Bits();
		uint8* dest_end = dest + bitmap->BitsLength();
		uint8* src = (uint8*)Buffer();
		uint8* src_end = src + BufferLength();
		while( (dest+3) < dest_end && (src+3) < src_end ) {
			*(dest+3) = *src;
			*(dest+2) = *(src+1);
			*(dest+1) = *(src+2);
			*dest = *(src+3);
			dest += 4;
			src += 4;
		}
		return bitmap;
	}
	
	if( ColorSpace() == B_NO_COLOR_SPACE ) {
		if( BitmapFromCursor((const uint8*)Buffer(), BufferLength(),
							 0, 0, 0, &bitmap) == B_OK ) {
			if( bitmap ) return bitmap;
		}
	}
		
	bitmap = new BBitmap(BRect(0, 0, Width()-1, Height()-1), ColorSpace());
	memcpy(bitmap->Bits(), Buffer(),
		   bitmap->BitsLength() < (int32)BufferLength() ?
		   bitmap->BitsLength() : (int32)BufferLength());
		   
	return bitmap;
}

void TRawDataItem::SetBytesPerEntry(size_t value)
{
	fBytesPerEntry = value;
}

size_t TRawDataItem::BytesPerEntry() const
{
	return fBytesPerEntry;
}

void TRawDataItem::SetIdentifier(const char* value)
{
	fIdentifier = value;
}

bool TRawDataItem::HasIdentifier() const
{
	return fIdentifier.Length() > 0;
}

const char* TRawDataItem::Identifier() const
{
	return fIdentifier.String();
}

void TRawDataItem::SetWidth(uint32 value)
{
	fWidth = value;
}

uint32 TRawDataItem::Width(bool* guess) const
{
	if( fWidth > 0 ) {
		if( guess ) *guess = false;
		return fWidth;
	}
	if( guess ) *guess = true;
	return fGuessWidth;
}

void TRawDataItem::SetHeight(uint32 value)
{
	fHeight = value;
}

uint32 TRawDataItem::Height(bool* guess) const
{
	if( fHeight > 0 ) {
		if( guess ) *guess = false;
		return fHeight;
	}
	if( guess ) *guess = true;
	return fGuessHeight;
}

void TRawDataItem::ConstrainDimensions()
{
	int32 i = ClosestDimension(Width(), Height());
	if( i >= 0 ) GetDimension(i, &fWidth, &fHeight);
}

void TRawDataItem::SetColorSpace(color_space value)
{
	if( fColorSpace != value ) {
		ForgetDimensions();
		fColorSpace = value;
		if( value != B_NO_COLOR_SPACE ) fIsCursor = false;
	}
}

color_space TRawDataItem::ColorSpace(bool* guess) const
{
	if( fColorSpace != B_NO_COLOR_SPACE ) {
		if( guess ) *guess = false;
		return fColorSpace;
	}
	if( guess ) *guess = true;
	return fGuessColorSpace;
}

void TRawDataItem::SetIsCursor(bool isit)
{
	if( fIsCursor != isit ) {
		ForgetDimensions();
		fIsCursor = isit;
		if( fIsCursor ) fColorSpace = B_NO_COLOR_SPACE;
	}
}

bool TRawDataItem::IsCursor(bool* guess) const
{
	if( fIsCursor && fColorSpace == B_NO_COLOR_SPACE ) {
		if( guess ) *guess = false;
		return true;
	} else if( fColorSpace != B_NO_COLOR_SPACE ) {
		if( guess ) *guess = false;
		return false;
	}
	if( guess ) *guess = true;
	return fGuessIsCursor;
}

bool TRawDataItem::CanBeCursor() const
{
	return BufferLength() == 68 ? true : false;
}

void TRawDataItem::GuessAttributes() const
{
	if( fHaveGuessed ) return;
	
	TRawDataItem* This = const_cast<TRawDataItem*>(this);
	
	This->fHaveGuessed = true;
	
	if( CanBeCursor() ) {
		This->fGuessWidth = This->fGuessHeight = 16;
		This->fGuessColorSpace = B_NO_COLOR_SPACE;
		This->fGuessIsCursor = true;
		return;
	}
	
	This->fGuessIsCursor = false;
	switch( BytesPerEntry() ) {
		case 2:
			This->fGuessColorSpace = B_RGBA15;
			break;
		case 4:
			This->fGuessColorSpace = B_RGBA32;
			break;
		default:
			This->fGuessColorSpace = B_CMAP8;
			break;
	}
	
	int32 w, h;
	if( fMetaData.FindInt32("width", &w) != B_OK ) w = 0;
	if( fMetaData.FindInt32("height", &h) != B_OK ) h = 0;
	
	int32 d = ClosestDimension(w, h);
	if( d >= 0 ) {
		GetDimension(d, &This->fGuessWidth, &This->fGuessHeight);
	} else {
		This->fGuessWidth = 0;
		This->fGuessHeight = 0;
	}
}

status_t TRawDataItem::GetDimension(int32 which, uint32* w, uint32* h) const
{
	if( which >= fWidths.CountItems() ) {
		if( fWidths.CountItems() <= 0 ) {
			TRawDataItem* This = const_cast<TRawDataItem*>(this);
			MakePossibleDimensions(&This->fWidths, &This->fHeights);
		}
		
		if( which >= fWidths.CountItems() ) return B_ERROR;
	}
	
	*w = (int32)fWidths.ItemAt(which);
	*h = (int32)fHeights.ItemAt(which);
	return B_OK;
}

static inline int32 my_abs(int32 v)
{
	return v >= 0 ? v : -v;
}

int32 TRawDataItem::ClosestDimension(uint32 width, uint32 height) const
{
	int32 best_i=-1, best_diff=10000;
	int32 w, h;
	int32 i=0;
	while( GetDimension(i, (uint32*)&w, (uint32*)&h) == B_OK ) {
		int32 diff = (width > 0 ? my_abs(w-width) : 0)
				   + (height > 0 ? my_abs(h-height) : 0);
		if( width == 0 && height == 0 ) diff = my_abs(w-h) + 5;
		if( diff < best_diff ) {
			best_diff = diff;
			best_i = i;
		}
		i++;
	}
	
	return best_i;
}

const BList& TRawDataItem::WidthList() const
{
	if( fWidths.CountItems() <= 0 ) {
		TRawDataItem* This = const_cast<TRawDataItem*>(this);
		MakePossibleDimensions(&This->fWidths, &This->fHeights);
	}
	
	return fWidths;
}

const BList& TRawDataItem::HeightList() const
{
	if( fHeights.CountItems() <= 0 ) {
		TRawDataItem* This = const_cast<TRawDataItem*>(this);
		MakePossibleDimensions(&This->fWidths, &This->fHeights);
	}
	
	return fHeights;
}

void TRawDataItem::MakePossibleDimensions(BList* widths, BList* heights) const
{
	if( IsCursor() ) {
		widths->AddItem((void*)16);
		heights->AddItem((void*)16);
		return;
	}
	
	if( ColorSpace() == B_NO_COLOR_SPACE ) return;
	
	// Quick way to (presumably) determine the number of bits per pixel
	// and padding for this color space.  From there, we can figure out
	// which dimensions are -probably- valid.
	//
	// The assumtions we are making for this to be correct are:
	// * Bitmaps will never need a row padding of more than 256 pixels.
	// * The amount of padding needed for a bitmap is not dependent
	//   on its dimensions.
	BBitmap evenBitmap(BRect(0, 0, 255, 0), ColorSpace());
	BBitmap oddBitmap(BRect(0, 0, 256, 0), ColorSpace());
	int32 bitsPerPixel = (evenBitmap.BytesPerRow()*8) / 256;
	int32 bitsPadding = (oddBitmap.BytesPerRow()*8) - (bitsPerPixel*256);
	PRINT(("For color space %ld: bitsPerPixel=%lx, bitsPadding=%ld\n",
			ColorSpace(), bitsPerPixel, bitsPadding));
			
	for( int32 i=1; i<(int32)(BufferLength()/(bitsPerPixel/8)); i++ ) {
		int32 bitsPerRow = bitsPerPixel*i;
		if( bitsPadding > 0 ) {
			bitsPerRow = ((bitsPerRow+bitsPadding-1)/bitsPadding)*bitsPadding;
		}
		int32 other = BufferLength()/(bitsPerRow/8);
		if( other*(bitsPerRow/8) == (int32)BufferLength() ) {
			// Looks good!
			PRINT(("Found dimension: %ldx%ld\n", i, other));
			widths->AddItem((void*)i);
			heights->AddItem((void*)other);
		}
	}
	
	// This would be a more correct way to do the above...  but, as you
	// can imagine, it's painfully slow.
	#if 0
	for( int32 i=1; i<(int32)BufferLength(); i++ ) {
		BBitmap b(BRect(0, 0, i-1, 0), ColorSpace());
		if( b.BytesPerRow() >= (int32)BufferLength() ) break;
		if( b.BytesPerRow() > 0 ) {
			int32 other = BufferLength()/b.BytesPerRow();
			if( other*b.BytesPerRow() == (int32)BufferLength() ) {
				// Looks good!
				widths->AddItem((void*)i);
				heights->AddItem((void*)other);
			}
		}
	}
	#endif
}

void TRawDataItem::ForgetDimensions()
{
	fWidths.MakeEmpty();
	fHeights.MakeEmpty();
}

// ------------------------------ TBitmapView ------------------------------

class TBitmapView : public BView
{
public:
	TBitmapView(BRect frame, const char *name, BBitmap* bitmap = 0,
				uint32 resizeMask=B_FOLLOW_NONE, uint32 flags=B_WILL_DRAW)
		: BView(frame, name, resizeMask, flags),
		  fBitmap(0)
	{
		SetBitmap(bitmap);
	}
	
	virtual ~TBitmapView()
	{
		SetBitmap((BBitmap*)0);
	}
	
	void SetBitmap(const TRawDataItem* item)
	{
		if( item ) {
			SetBitmap(item->MakeBitmap());
		} else {
			SetBitmap((BBitmap*)0);
		}
	}
	
	void SetBitmap(BBitmap* bitmap)
	{
		if( fBitmap ) {
			delete fBitmap;
			fBitmap = 0;
		}
		fBitmap = bitmap;
		Invalidate();
	}
	
	const BBitmap* Bitmap() const
	{
		return fBitmap;
	}
	
	virtual	void Draw(BRect updateRect)
	{
		(void)updateRect;
		if( fBitmap ) DrawBitmap(fBitmap, BPoint(2, 2));
		AddBevel(this, Bounds(), false);
	}
	
	virtual	void GetPreferredSize(float *width, float *height)
	{
		if( fBitmap ) {
			BRect bounds = fBitmap->Bounds();
			*width = bounds.Width() + 4;
			*height = bounds.Height() + 4;
		} else {
			*width = 4;
			*height = 4;
		}
	}

private:
	BBitmap* fBitmap;
};

// ------------------------------ TRawDataWindow ------------------------------

enum {
	kSelectItem = '.sit',
	kSetDimension = '.sdm',
	kSetColorSpace = '.scs',
	kSetName = '.snm',
	kOkay = '.oky',
	kCancel = '.ccl',
	
	kNextItem = '.nit',
	kPrevItem = '.pit',
	kNextDimension = '.ndm',
	kPrevDimension = '.pdm',
	
	kLayoutViews = '.lyt'
};

static const BRect bogusRect(0, -400, 10, -390);
	
static BMenuField* BuildItemMenu(const BList& items)
{
	BMenu* menu = new BPopUpMenu("");
	for( int32 i=0; i<items.CountItems(); i++ ) {
		const TRawDataItem* item = (const TRawDataItem*)items.ItemAt(i);
		BMessage* msg = new BMessage(kSelectItem);
		msg->AddInt32("index", i);
		BString label;
		label << i << ". ";
		if( item->HasIdentifier() ) {
			label << item->Identifier();
		} else {
			label << "(" << item->BufferLength() << " bytes)";
		}
		BMenuItem* menuitem = new BMenuItem(label.String(), msg);
		menu->AddItem(menuitem);
	}
	
	BMenuField* f = new BMenuField(bogusRect,
								   "items", "Current ", menu,
								   false, B_FOLLOW_NONE);
	return f;
}

struct color_menu_item { const char* name; color_space cspace; };
static const color_menu_item color_menu_items[] = {
	{ "B_CMAP8", B_CMAP8 },
	{ "", B_NO_COLOR_SPACE },
	{ "B_RGB15", B_RGB15 },
	{ "B_RGBA15", B_RGBA15 },
	{ "B_RGB16", B_RGB16 },
	{ "B_RGB24", B_RGB24 },
	{ "B_RGB32", B_RGB32 },
	{ "B_RGBA32", B_RGBA32 },
	{ "", B_NO_COLOR_SPACE },
	{ "B_RGB15_BIG", B_RGB15_BIG },
	{ "B_RGBA15_BIG", B_RGBA15_BIG },
	{ "B_RGB16_BIG", B_RGB16_BIG },
	{ "B_RGB24_BIG", B_RGB24_BIG },
	{ "B_RGB32_BIG", B_RGB32_BIG },
	{ "B_RGBA32_BIG", B_RGBA32_BIG },
	{ "", B_NO_COLOR_SPACE },
	{ "Cursor", B_NO_COLOR_SPACE },
	{ NULL, B_NO_COLOR_SPACE }
};

static BMenuField* BuildColorMenu()
{
	BMenu* menu = new BPopUpMenu("");
	const color_menu_item* c = color_menu_items;
	while( c && c->name ) {
		if( *(c->name) ) {
			BMessage* msg = new BMessage(kSetColorSpace);
			msg->AddInt32("color_space", c->cspace);
			BMenuItem* item = new BMenuItem(c->name, msg);
			menu->AddItem(item);
		} else {
			menu->AddSeparatorItem();
		}
		c++;
	}
	
	BMenuField* f = new BMenuField(bogusRect,
								   "color_space", "Color:", menu,
								   false, B_FOLLOW_NONE);
	return f;
}

static BMenuField* BuildDimensionMenu(const BList& widths, const BList& heights)
{
	BMenu* menu = new BPopUpMenu("");
	for( int32 i=0; i<widths.CountItems(); i++ ) {
		int32 w = (int32)widths.ItemAt(i);
		int32 h = (int32)heights.ItemAt(i);
		BMessage* msg = new BMessage(kSetDimension);
		msg->AddInt32("width", w);
		msg->AddInt32("height", h);
		BString label;
		label << w << "x" << h;
		BMenuItem* item = new BMenuItem(label.String(), msg);
		menu->AddItem(item);
	}
	
	BMenuField* f = new BMenuField(bogusRect,
								   "dimensions", "Dimensions:", menu,
								   false, B_FOLLOW_NONE);
	return f;
}

TRawDataWindow::TRawDataWindow(BPoint around,
							   const BResourceAddonArgs& args,
							   BList* items)
	: BWindow(BRect(around.x, around.y, around.x+1, around.y+1),
			  "Insert Bitmap",
			  B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			  B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE,
			  B_CURRENT_WORKSPACE),
	  BResourceAddonBase(args),
	  fItems(items),
	  fMenuBar(0), fRoot(0), fContainer(0),
	  fItemList(0), fDimensions(0), fColorSpaces(0), fName(0), fPreview(0),
	  fOkay(0), fCancel(0),
	  fCurItem(0), fPrefWidth(0), fPrefHeight(0)
{
	// create the menu bar
	fMenuBar = new BMenuBar(BRect(0, 0, 0, 0), "menu");

	// File menu
	BMenu *menu = new BMenu("Window");
	fMenuBar->AddItem(new BMenuItem(menu));

	menu->AddItem(new BMenuItem("Okay", new BMessage(kOkay), 'W'));
	menu->AddItem(new BMenuItem("Cancel", new BMessage(kCancel)));

	menu = new BMenu("Bitmap");
	fMenuBar->AddItem(new BMenuItem(menu));
	menu->AddItem(new BMenuItem("Next Bitmap", new BMessage(kNextItem), 'N'));
	menu->AddItem(new BMenuItem("Previous Bitmap", new BMessage(kPrevItem), 'P'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Next Dimension", new BMessage(kNextDimension), 'L'));
	menu->AddItem(new BMenuItem("Previous Dimension", new BMessage(kPrevDimension), 'S'));
	
	AddChild(fMenuBar);
	BRect area = Bounds();
	area.top = fMenuBar->Bounds().bottom + 1;
	
	fRoot = new BView(area, "root", B_FOLLOW_ALL, B_WILL_DRAW);
	fRoot->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fRoot);
	
	Run();
	
	BMessage init(kLayoutViews);
	init.AddPoint("where", around);
	PostMessage(&init);
}

TRawDataWindow::~TRawDataWindow()
{
	if( fItems ) {
		fItems->DoForEach(free_item_func);
		fItems->MakeEmpty();
		delete fItems;
	}
}
						
	
void 
TRawDataWindow::MessageReceived(BMessage *m)
{
	switch (m->what) {
		case B_KEY_DOWN: {
			const char* str;
			if( m->FindString("bytes", &str) == B_OK ) {
				if( *str == '+' ) {
					StepCurrentWidth(1);
					return;
				} else if( *str == '-' ) {
					StepCurrentWidth(-1);
					return;
				}
			}
			BWindow::MessageReceived(m);
		} break;
		case kNextItem: {
			StepCurrentItem(1);
		} break;
		case kPrevItem: {
			StepCurrentItem(-1);
		} break;
		case kNextDimension: {
			StepCurrentWidth(1);
		} break;
		case kPrevDimension: {
			StepCurrentWidth(-1);
		} break;
		case kSelectItem: {
			int32 index;
			if( m->FindInt32("index", &index) == B_OK ) {
				TRawDataItem* it = (TRawDataItem*)fItems->ItemAt(index);
				SetCurrentItem(it);
			}
		} break;
		case kSetDimension: {
			int32 w, h;
			if( fCurItem && m->FindInt32("width", &w) == B_OK &&
					m->FindInt32("height", &h) == B_OK ) {
				fCurItem->SetWidth(w);
				fCurItem->SetHeight(h);
				LayoutViews();
			}
		} break;
		case kSetColorSpace: {
			color_space cs;
			if( fCurItem && m->FindInt32("color_space", (int32*)&cs) == B_OK ) {
				if( cs != fCurItem->ColorSpace() ) {
					if( cs != B_NO_COLOR_SPACE ) {
						fCurItem->SetColorSpace(cs);
					} else {
						fCurItem->SetIsCursor(true);
					}
					fCurItem->ConstrainDimensions();
					if( fDimensions ) {
						fDimensions->RemoveSelf();
						delete fDimensions;
						fDimensions = 0;
					}
				}
				LayoutViews();
				SetCurrentDimension(fCurItem->Width(), fCurItem->Height());
			}
		} break;
		case kCancel: {
			PostMessage(B_QUIT_REQUESTED);
		} break;
		case kOkay: {
			UpdateIdentifierName();
			BResourceCollection* c = WriteLock("Add Bitmap");
			if( c ) {
				for( int32 i=0; i<fItems->CountItems(); i++ ) {
					AddResourceItem(c, (TRawDataItem*)fItems->ItemAt(i));
				}
				WriteUnlock(c);
			}
			PostMessage(B_QUIT_REQUESTED);
		} break;
		case kLayoutViews: {
			BPoint where;
			m->FindPoint("where", &where);
			SetCurrentItem((TRawDataItem*)fItems->ItemAt(0));
			ConstrainFrame(&where);
			Show();
		} break;
		default:
			BWindow::MessageReceived(m);
	}
}

void
TRawDataWindow::FrameResized(float new_width, float new_height)
{
	(void)new_width;
	(void)new_height;
	
	//LayoutViews();
}

bool 
TRawDataWindow::QuitRequested()
{
	return true;
}

void
TRawDataWindow::StepCurrentItem(int32 offset)
{
	if( fItems->CountItems() <= 0 ) return;
	
	int32 next = offset - 1;
	if( fCurItem ) {
		for( int32 i=0; i<fItems->CountItems(); i++ ) {
			if( fItems->ItemAt(i) == fCurItem ) {
				next = i + offset;
				break;
			}
		}
	}
	if( next < 0 ) next = fItems->CountItems()-1;
	if( next >= fItems->CountItems() ) next = 0;
	SetCurrentItem((TRawDataItem*)fItems->ItemAt(next));
}

void
TRawDataWindow::StepCurrentWidth(int32 offset)
{
	if( !fCurItem || fCurItem->WidthList().CountItems() <= 0 ) return;
	
	uint32 w = fCurItem->Width() + offset;
	int32 i = fCurItem->ClosestDimension(w, 0);
	uint32 new_w, new_h;
	status_t err = B_ERROR;
	while( i >= 0 && (err=fCurItem->GetDimension(i, &new_w, &new_h)) == B_OK ) {
		if( offset > 0 && new_w >= w ) break;
		if( offset < 0 && new_w <= w ) break;
		i += (offset >= 0 ? 1 : -1);
	}
	if( err != B_OK ) {
		if( offset >= 0 ) {
			err = fCurItem->GetDimension(0, &new_w, &new_h);
		} else {
			err = fCurItem->GetDimension(fCurItem->WidthList().CountItems()-1,
										 &new_w, &new_h);
		}
	}
	fCurItem->SetWidth(new_w);
	fCurItem->SetHeight(new_h);
	LayoutViews();
	SetCurrentDimension(new_w, new_h);
}

void
TRawDataWindow::SetCurrentItem(TRawDataItem* item)
{
	if( item != fCurItem ) {
		UpdateIdentifierName();
		
		if( fDimensions ) fDimensions->RemoveSelf();
		delete fDimensions;
		fDimensions = 0;
		if( fColorSpaces ) fColorSpaces->RemoveSelf();
		delete fColorSpaces;
		fColorSpaces = 0;
		fCurItem = item;
		LayoutViews();
		
		if( fCurItem ) {
			for( int32 i=0; i<fItems->CountItems(); i++ ) {
				if( fItems->ItemAt(i) == item ) {
					SetCurrentItem(i);
					break;
				}
			}
			SetCurrentDimension(fCurItem->Width(), fCurItem->Height());
			SetCurrentColorSpace(fCurItem->ColorSpace(), fCurItem->CanBeCursor());
			if( fName ) fName->SetText(fCurItem->Identifier());
		}
	}
}

void
TRawDataWindow::SetCurrentItem(int32 index)
{
	if( !fItemList ) return;
	
	BMenu* menu = fItemList->Menu();
	if( menu ) {
		BMenuItem* item = menu->ItemAt(index);
		if( item ) item->SetMarked(true);
		else {
			item = menu->FindMarked();
			if( item ) item->SetMarked(false);
		}
	}
}

void
TRawDataWindow::SetCurrentDimension(uint32 w, uint32 h)
{
	if( !fDimensions ) return;
	
	BMenu* menu = fDimensions->Menu();
	if( menu ) {
		for( int32 i=0; i<menu->CountItems(); i++ ) {
			BMenuItem* item = menu->ItemAt(i);
			BMessage* msg = item ? item->Message() : 0;
			int32 cw, ch;
			if( msg && msg->FindInt32("width", &cw) == B_OK &&
					msg->FindInt32("height", &ch) == B_OK ) {
				if( (uint32)cw == w && (uint32)ch == h ) {
					item->SetMarked(true);
					break;
				} else {
					item->SetMarked(false);
				}
			}
		}
	}
}

void
TRawDataWindow::SetCurrentColorSpace(color_space cs, bool can_be_cursor)
{
	if( !fColorSpaces ) return;
	
	BMenu* menu = fColorSpaces->Menu();
	if( menu ) {
		for( int32 i=0; i<menu->CountItems(); i++ ) {
			BMenuItem* item = menu->ItemAt(i);
			BMessage* msg = item ? item->Message() : 0;
			color_space c;
			if( msg && msg->FindInt32("color_space", (int32*)&c) == B_OK ) {
				item->SetEnabled(can_be_cursor || c != B_NO_COLOR_SPACE);
				if( c == cs ) {
					item->SetMarked(true);
				} else {
					item->SetMarked(false);
				}
			}
		}
	}
}

void
TRawDataWindow::UpdateIdentifierName()
{
	if( fCurItem && fName ) {
		fCurItem->SetIdentifier(fName->Text());
	}
}

void
TRawDataWindow::LayoutViews()
{
	fPrefWidth = fPrefHeight = 0;
	
	if( !fCurItem ) return;
	
	BeginViewTransaction();
	
	static const float SPACER = 8;
	
	if( !fContainer ) {
		fContainer = new BBox(bogusRect, "container");
		fRoot->AddChild(fContainer);
	}
	
	if( !fItemList ) {
		fItemList = BuildItemMenu(*fItems);
		fItemList->SetFont(be_bold_font);
		fContainer->SetLabel(fItemList);
	}
	
	if( !fDimensions ) {
		if( fCurItem ) {
			fCurItem->GuessAttributes();
			fDimensions = BuildDimensionMenu(fCurItem->WidthList(),
											 fCurItem->HeightList());
		} else {
			BList widths, heights;
			fDimensions = BuildDimensionMenu(widths, heights);
		}
		fContainer->AddChild(fDimensions);
	}
	
	if( !fColorSpaces ) {
		fColorSpaces = BuildColorMenu();
		fContainer->AddChild(fColorSpaces);
	}
	
	if( !fName ) {
		fName = new BTextControl(bogusRect, "name", "Name: ", "",
								 new BMessage(kSetName), B_FOLLOW_NONE);
		fContainer->AddChild(fName);
	}
	
	if( !fPreview ) {
		fPreview = new TBitmapView(bogusRect, "bitmap", 0,
									B_FOLLOW_NONE);
		fContainer->AddChild(fPreview);
	}
	
	if( !fCancel ) {
		fCancel = new BButton(bogusRect, "cancel", "Cancel",
							new BMessage(kCancel), B_FOLLOW_NONE);
		fRoot->AddChild(fCancel);
	}
	
	if( !fOkay ) {
		fOkay = new BButton(bogusRect, "okay", "Okay",
							new BMessage(kOkay), B_FOLLOW_NONE);
		fRoot->AddChild(fOkay);
		SetDefaultButton(fOkay);
	}
	
	float itW, itH;
	float itD = GetMenuFieldSize(fItemList, &itW, &itH);
	
	const float containerOffset = 2 + SPACER;
	const float containerTopOffset = itH + SPACER;
	float containerWidth = 0;
	float containerHeight = 0;
	
	float dimW, dimH;
	float dimD = GetMenuFieldSize(fDimensions, &dimW, &dimH);
	containerWidth += dimW;
	
	float sizeW, sizeH;
	float sizeD = GetMenuFieldSize(fColorSpaces, &sizeW, &sizeH);
	containerWidth += SPACER + sizeW;
	containerHeight += dimH > sizeH ? dimH : sizeH;
	
	float nameW, nameH;
	fName->GetPreferredSize(&nameW, &nameH);
	containerHeight += SPACER + nameH;
	
	float bmW, bmH;
	fPreview->SetBitmap(fCurItem);
	fPreview->GetPreferredSize(&bmW, &bmH);
	if( bmW > fPrefWidth ) fPrefWidth = bmW;
	fPreview->ResizeTo(bmW, bmH);
	if( containerWidth < bmW ) containerWidth = bmW;
	containerHeight += SPACER + bmH;
	
	if( containerWidth < itW ) containerWidth = itW;
	
	// Now have the final container size -- resize the name text control
	// to fit into it.
	fName->ResizeToPreferred();
	fName->ResizeTo(containerWidth, nameH);
	
	containerWidth += containerOffset*2;
	containerHeight += containerTopOffset + containerOffset;
	
	fPrefHeight = SPACER + containerHeight + SPACER;
	fPrefWidth = SPACER + containerWidth + SPACER;
	
	float cancelW, cancelH;
	fCancel->GetPreferredSize(&cancelW, &cancelH);
	fCancel->ResizeTo(cancelW, cancelH);
	float okayW, okayH;
	fOkay->GetPreferredSize(&okayW, &okayH);
	fOkay->ResizeTo(okayW, okayH);
	float buttonH = okayH > cancelH ? okayH : cancelH;
	
	if( fPrefWidth < (cancelW+SPACER*3+okayW) ) fPrefWidth = cancelW+SPACER*3+okayW;
	
	// Now position all controls.
	fContainer->ResizeTo(containerWidth, containerHeight);
	fContainer->MoveTo(SPACER, SPACER);
	//fItemList->ResizeTo(itW, itH);
	fItemList->ResizeTo(fItemList->Bounds().Width(), itH);
	fItemList->SetDivider(itD);
	fDimensions->ResizeTo(dimW, dimH);
	//fDimensions->ResizeTo(fDimensions->Bounds().Width(), dimH);
	fDimensions->MoveTo(containerOffset, containerTopOffset);
	fDimensions->SetDivider(dimD);
	fColorSpaces->ResizeTo(sizeW, sizeH);
	//fColorSpaces->ResizeTo(fColorSpaces->Bounds().Width(), sizeH);
	fColorSpaces->MoveTo(containerWidth - containerOffset - sizeW,
						 containerTopOffset);
	fColorSpaces->SetDivider(sizeD);
	fName->MoveTo(containerOffset,
				  containerTopOffset + (dimH>sizeH ? dimH:sizeH));
	fPreview->MoveTo(containerOffset + floor((containerWidth-containerOffset*2-bmW)/2),
					 containerHeight - containerOffset - bmH);
	
	fCancel->MoveTo(SPACER, fPrefHeight + floor((buttonH-cancelH)/2));
	fOkay->MoveTo(fPrefWidth-SPACER - okayW,
				  fPrefHeight + floor((buttonH-okayH)/2));
	
	fPrefHeight += SPACER + buttonH + fMenuBar->Bounds().Height()+1;
	
	ResizeTo(fPrefWidth, fPrefHeight);
	ConstrainFrame();
	
	EndViewTransaction();
}

void TRawDataWindow::ConstrainFrame(BPoint* init_point)
{
	BScreen s(this);
	
	BRect wframe;
	if( init_point ) {
		wframe = BRect(*init_point,
					   BPoint(init_point->x+fPrefWidth, init_point->y+fPrefHeight));
	} else {
		wframe = Frame();
	}
	
	BRect sframe(s.Frame());
	
	if( wframe.Width() > sframe.Width() ) {
		wframe.right -= wframe.Width() - sframe.Width();
	}
	if( wframe.Height() > sframe.Height() ) {
		wframe.bottom -= wframe.Height() - sframe.Height();
	}
	
	if( wframe.left < sframe.left ) {
		wframe.right += sframe.left-wframe.left;
		wframe.left = sframe.left;
	}
	if( wframe.top < sframe.top ) {
		wframe.bottom += sframe.top-wframe.top;
		wframe.top = sframe.top;
	}
	if( wframe.right > sframe.right ) {
		wframe.left += wframe.right-sframe.right;
		wframe.right = sframe.right;
	}
	if( wframe.bottom > sframe.bottom ) {
		wframe.top += wframe.bottom-sframe.bottom;
		wframe.bottom = sframe.bottom;
	}
	
	MoveTo(wframe.left, wframe.top);
	ResizeTo(wframe.Width(), wframe.Height());
}

void TRawDataWindow::AddResourceItem(BResourceCollection* context,
									 const TRawDataItem* item)
{
	if( !item ) return;
	
	item->GuessAttributes();
	
	if( item->IsCursor() ) {
		BResourceHandle h;
		const char* name = item->HasIdentifier() ? item->Identifier() : "New Cursor";
		context->AddItem(&h, B_CURSOR_TYPE, 1, name,
						 item->Buffer(), item->BufferLength(), true,
						 context->B_RENAME_NEW_ITEM);
	
	} else {
		const BBitmap* bm = item->MakeBitmap();
		if( bm ) {
			BMessage arch;
			bm->Archive(&arch, false);
			BMallocIO io;
			if( arch.Flatten(&io) == B_OK ) {
				BResourceHandle h;
				const char* name = item->HasIdentifier()
								 ? item->Identifier() : "New Bitmap";
				context->AddItem(&h, B_BITMAP_TYPE, 1, name,
								 io.Buffer(), io.BufferLength(), true,
								 context->B_RENAME_NEW_ITEM);
			}
		}
		
		delete bm;
	}
}

// ------------------------------ myArrayParser ------------------------------

class myArrayParser : public BArrayParser
{
public:
	myArrayParser()
		: fItems(0)
	{
	}
	virtual ~myArrayParser()
	{
		if( fItems ) {
			fItems->DoForEach(free_item_func);
			fItems->MakeEmpty();
			delete fItems;
		}
	}
	
	virtual status_t ReadArray(const void* data, size_t size,
							   size_t bytes_per_entry,
							   const char* identifier,
							   const BMessage& meta_data)
	{
		if( !fItems ) fItems = new BList;
		fItems->AddItem(new TRawDataItem(data, size, bytes_per_entry,
										 identifier, meta_data));
		return B_OK;
	}
	
	BList* DetachItems()
	{
		BList* ret = fItems;
		fItems = 0;
		return ret;
	}
	
private:
	BList* fItems;
};

status_t parse_text_data(const BResourceAddonArgs& args,
						 BPoint where, const char* text, size_t length)
{
	myArrayParser parser;
	parser.Run(text, length);
	BList* items = parser.DetachItems();
	if( !items ) return B_ERROR;
	
	new TRawDataWindow(where, args, items);
	return B_OK;
}

status_t parse_message_data(const BResourceAddonArgs& args,
							BPoint where, const BMessage* message)
{
	
	const char* text = NULL;
	ssize_t size;
	if( message->FindData("text/plain", B_MIME_TYPE,
						  (const void**)&text, &size) == B_OK ) {
		return parse_text_data(args, where, text, size);
	}
	
	return B_ERROR;
}
