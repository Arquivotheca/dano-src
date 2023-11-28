#include "BitmapControls.h"
#include "utils.h"

#include <Message.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TextControl.h>
#include <Debug.h>
#include <String.h>

#include <TranslationDefs.h>
#include <TranslatorRoster.h>

#include <stdlib.h>

struct color_menu_item { const char* name; color_space cspace; };
static const color_menu_item color_menu_items[] = {
	{ "B_CMAP8", B_CMAP8 },
	{ "B_RGB15", B_RGB15 },
	{ "B_RGBA15", B_RGBA15 },
	{ "B_RGB16", B_RGB16 },
	{ "B_RGB32", B_RGB32 },
	{ "B_RGBA32", B_RGBA32 },
	{ NULL, B_NO_COLOR_SPACE }
};

static BMenuField* BuildColorMenu()
{
	BMenu* menu = new BPopUpMenu("");
	const color_menu_item* c = color_menu_items;
	while( c && c->name ) {
		BMessage* msg = new BMessage(kColorSpaceChangedMsg);
		msg->AddInt32("color_space", c->cspace);
		BMenuItem* item = new BMenuItem(c->name, msg);
		menu->AddItem(item);
		c++;
	}
	
	BMenuField* f = new BMenuField(BRect(0, 0, 15, 15),
								   "color_space", "Color:", menu,
								   false, B_FOLLOW_NONE);
	return f;
}

static BMenuItem* BuildFormatItem(const char* name, const char* mime,
								  type_code type, type_code translator)
{
	BMessage* msg = new BMessage(kFormatChangedMsg);
	msg->AddInt32("format", type);
	msg->AddInt32("translator", translator);
	msg->AddString("mime_sig", mime);
	msg->AddString("name", name);
	BMenuItem* item = new BMenuItem(name, msg);
	return item;
}

static BMenuField* BuildFormatMenu()
{
	BTranslatorRoster* roster = BTranslatorRoster::Default();
	
	translator_id* t = 0;
	int32 tc = 0;
	if( roster->GetAllTranslators(&t, &tc) != B_OK ) {
		return 0;
	}
	
	BMenu* menu = new BPopUpMenu("");
	for( int32 i = -1; i<tc; i++ ) {
		if( i == -1 ) {
			BMenuItem* item = BuildFormatItem("Archived Bitmap",
											  "application/x-vnd.Be.ArchivedBitmap",
											  B_BITMAP_TYPE, 0);
			menu->AddItem(item);
			menu->AddSeparatorItem();
		} else {
			const translation_format* ifmt = 0;
			int32 ifmtc = 0;
			const translation_format* ofmt = 0;
			int32 ofmtc = 0;
			if( roster->GetInputFormats(t[i], &ifmt, &ifmtc) != B_OK ) continue;
			if( roster->GetOutputFormats(t[i], &ofmt, &ofmtc) != B_OK ) continue;
			int32 j;
			
			// Can this translator read and write bitmaps?
			for( j=0; j<ifmtc; j++ ) {
				if( ifmt[j].type == B_TRANSLATOR_BITMAP ) break;
			}
			if( j >= ifmtc ) continue;
			for( j=0; j<ofmtc; j++ ) {
				if( ofmt[j].type == B_TRANSLATOR_BITMAP ) break;
			}
			if( j >= ofmtc ) continue;
			
			// Find all formats that it can both read and write.
			for( j=0; j<ifmtc; j++ ) {
				if( ifmt[j].type != B_TRANSLATOR_BITMAP ) {
					for( int32 k=0; k<ofmtc; k++ ) {
						if( ifmt[j].type == ofmt[j].type ) {
							BMenuItem* item = BuildFormatItem(
									ifmt[j].name, ifmt[j].MIME,
									ifmt[j].type, t[i]);
							menu->AddItem(item);
							break;
						}
					}
				}
			}
		}
	}
	delete[] t;
	
	BMenuField* f = new BMenuField(BRect(0, 0, 15, 15),
								   "format", "Format:", menu,
								   false, B_FOLLOW_NONE);
	return f;
}

TBitmapControls::TBitmapControls(BRect frame, const char* name,
								 uint32 resize, uint32 flags)
	: BView(frame, name, resize, flags),
	  fWidth(0), fHeight(0), fColorSpace(0), fFormat(0)
{
	fWidth = new BTextControl(BRect(0, 0, 15, 15), "width", "Width:", "",
							  new BMessage(kDimensChangedMsg), B_FOLLOW_NONE);
	AddChild(fWidth);
	fWidth->SetDivider(fWidth->StringWidth(fWidth->Label())
						+ fWidth->StringWidth(" "));
	fHeight = new BTextControl(BRect(0, 0, 15, 15), "height", "Height:", "",
							  new BMessage(kDimensChangedMsg), B_FOLLOW_NONE);
	fHeight->SetDivider(fHeight->StringWidth(fHeight->Label())
						+ fHeight->StringWidth(" "));
	AddChild(fHeight);
	fColorSpace = BuildColorMenu();
	AddChild(fColorSpace);
	fFormat = BuildFormatMenu();
	AddChild(fFormat);
}

TBitmapControls::~TBitmapControls()
{
}

void TBitmapControls::SetAllTargets(BMessenger who)
{
	fTarget = who;
	if( fColorSpace ) fColorSpace->Menu()->SetTargetForItems(who);
	if( fFormat ) fFormat->Menu()->SetTargetForItems(who);
}

void TBitmapControls::SetAttributes(float width, float height,
									color_space colors, type_code type)
{
	BString buf;
	if( fWidth ) {
		buf << (int32)(width+.5);
		fWidth->SetText(buf.String());
	}
	if( fHeight ) {
		buf = "";
		buf << (int32)(height+.5);
		fHeight->SetText(buf.String());
	}
	if( fColorSpace ) {
		BMenu* menu = fColorSpace->Menu();
		if( menu ) {
			for( int32 i=0; i<menu->CountItems(); i++ ) {
				BMenuItem* item = menu->ItemAt(i);
				BMessage* msg = item ? item->Message() : 0;
				color_space c;
				if( msg && msg->FindInt32("color_space", (int32*)&c) == B_OK ) {
					if( c == colors ) {
						item->SetMarked(true);
						break;
					} else {
						item->SetMarked(false);
					}
				}
			}
		}
	}
	if( fFormat ) {
		BMenu* menu = fFormat->Menu();
		if( menu ) {
			for( int32 i=0; i<menu->CountItems(); i++ ) {
				BMenuItem* item = menu->ItemAt(i);
				BMessage* msg = item ? item->Message() : 0;
				type_code f;
				if( msg && msg->FindInt32("format", (int32*)&f) == B_OK ) {
					if( f == type ) {
						item->SetMarked(true);
						break;
					} else {
						item->SetMarked(false);
					}
				}
			}
		}
	}
}

void TBitmapControls::AttachedToWindow()
{
	if( Parent() ) SetViewColor(Parent()->ViewColor());
}

void TBitmapControls::AllAttached()
{
	if( fWidth ) fWidth->SetTarget(BMessenger(this));
	if( fHeight ) fHeight->SetTarget(BMessenger(this));
	FrameResized(Bounds().Width()+1, Bounds().Height()+1);
}

void TBitmapControls::DetachedFromWindow()
{
}

void TBitmapControls::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	*height = 0;
	
	float pw=0, ph=0;
	if( fWidth ) {
		fWidth->GetPreferredSize(&pw, &ph);
		*width = pw;
		*height = ph;
	}
	
	if( fHeight ) {
		fHeight->GetPreferredSize(&pw, &ph);
		*width = *width > pw ? *width : pw;
		*height += ph + 4;
	}
	
	if( fColorSpace ) {
		GetMenuFieldSize(fColorSpace, &pw, &ph);
		*width = *width > pw ? *width : pw;
		*height += ph + 4;
	}
	
	if( fFormat ) {
		GetMenuFieldSize(fFormat, &pw, &ph);
		*width = *width > pw ? *width : pw;
		*height += ph + 4;
	}
}

void TBitmapControls::FrameResized(float width, float height)
{
	(void)height;
	
	float y=0;
	float pw=0, ph=0;
	if( fWidth ) {
		fWidth->GetPreferredSize(&pw, &ph);
		fWidth->MoveTo(0, 0);
		fWidth->ResizeTo(width, ph);
		y += ph + 4;
	}
	if( fHeight ) {
		fHeight->GetPreferredSize(&pw, &ph);
		fHeight->MoveTo(0, y);
		fHeight->ResizeTo(width, ph);
		y += ph + 4;
	}
	if( fColorSpace ) {
		GetMenuFieldSize(fColorSpace, &pw, &ph);
		fColorSpace->MoveTo(0, y); //(width-pw)/2, y);
		fColorSpace->ResizeTo(pw, ph);
		y += ph + 4;
	}
	if( fFormat ) {
		GetMenuFieldSize(fFormat, &pw, &ph);
		fFormat->MoveTo(0, y); //(width-pw)/2, y);
		fFormat->ResizeTo(pw, ph);
		y += ph + 4;
	}
}

void TBitmapControls::MessageReceived(BMessage* msg)
{
	switch( msg->what ) {
		case kDimensChangedMsg: {
			BMessage f(*msg);
			if( fWidth ) {
				float w = atof(fWidth->Text());
				if( w > 0 ) {
					f.AddFloat("width", w);
				}
			}
			if( fHeight ) {
				float h = atof(fHeight->Text());
				if( h > 0 ) {
					f.AddFloat("height", h);
				}
			}
			fTarget.SendMessage(&f);
		} break;
		
		default:
			BView::MessageReceived(msg);
	}
}
