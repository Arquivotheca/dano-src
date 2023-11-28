#include "utils.h"

#include <Message.h>
#include <Bitmap.h>
#include <MenuField.h>
#include <MenuItem.h>

#include <TranslationUtils.h>
#include <TranslatorRoster.h>

color_map*
ColorMap()
{
	color_map* cmap;
	
	BScreen screen(B_MAIN_SCREEN_ID);
	cmap = (color_map*)screen.ColorMap();
	
	return cmap;
}

uint8
IndexForColor(rgb_color c)
{
	return IndexForColor(c.red, c.green, c.blue);
}

uint8
IndexForColor(uint8 r, uint8 g, uint8 b)
{
	BScreen screen(B_MAIN_SCREEN_ID);
	return screen.IndexForColor(r,g,b);
}

rgb_color
ColorForIndex(uint8 i)
{
	color_map *m = ColorMap();
	return m->color_list[i];
}

//
//	checks for a valid rect
//	swaps points to make valid rect
//
BRect
MakeValidRect(BRect r)
{
	BRect retRect = r;
//printf("**\n");
//r.PrintToStream();
	if (!retRect.IsValid()) {
		float temp=0;
		if (retRect.left > retRect.right) {
			temp = retRect.left;
			retRect.left = retRect.right;
			retRect.right = temp;
		}
		if (retRect.top > retRect.bottom) {
			temp = retRect.top;
			retRect.top = retRect.bottom;
			retRect.bottom = temp;			
		}
	}
//retRect.PrintToStream();	
	return retRect;
}

void
ConstrainRect(bool constrain, int32 *startX, int32 *startY, int32 *fbX, int32 *fbY)
{
	if (constrain) {
		int32 inc;
		if (*fbX < *fbY)
			inc = abs((int)(*fbX - *startX));
		else
			inc = abs((int)(*fbY - *startY));
		
		if ((*startX < *fbX) && (*startY < *fbY)) {
			*fbX = *startX + inc;
			*fbY = *startY + inc;
		} else if ((*startX > *fbX) && (*startY > *fbY)) {
			*fbX = *startX - inc;
			*fbY = *startY - inc;
		} else if ((*startX > *fbX) && (*startY < *fbY)) {
			*fbX = *startX - inc;
			*fbY = *startY + inc;
		} else if ((*startX < *fbX) && (*startY > *fbY)) {
			*fbX = *startX + inc;
			*fbY = *startY - inc;
		}
		
	}
}

BRect
IntersectRects(BRect r1, BRect r2)
{
	BRect r3;
	
	r3.left = (r1.left < r2.left) ? r1.left : r2.left;
	r3.top = (r1.top < r2.top) ? r1.top : r2.top;
	r3.right = (r1.right > r2.right) ? r1.right : r2.right;
	r3.bottom = (r1.bottom > r2.bottom) ? r1.bottom : r2.bottom;
	
	return r3;
}

BRect
ContainingRect(BPoint p1, BPoint p2)
{
	BRect r;
	
	r.left		= (p1.x < p2.x) ? p1.x : p2.x;
	r.top		= (p1.y < p2.y) ? p1.y : p2.y;
	r.right		= (p1.x > p2.x) ? p1.x : p2.x;
	r.bottom	= (p1.y > p2.y) ? p1.y : p2.y;
	
	return r;
}

BRect ExtendRectPoint(BRect r, BPoint p)
{
	if( !r.IsValid() ) r = BRect(p, p);
	r.left		= (p.x < r.left) ? p.x : r.left;
	r.right		= (p.x > r.right) ? p.x : r.right;
	r.top		= (p.y < r.top) ? p.y : r.top;
	r.bottom	= (p.y > r.bottom) ? p.y : r.bottom;
	return r;
}

BRect ExtendRects(BRect r1, BRect r2)
{
	if( !r1.IsValid() ) return r2;
	if( !r2.IsValid() ) return r1;
	r1 = ExtendRectPoint(r1, r2.LeftTop());
	r1 = ExtendRectPoint(r1, r2.RightBottom());
	return r1;
}

void
AddBevel(BView *v, BRect bounds, bool outset)
{
	if (outset)
		bounds.InsetBy(-2, -2);

	v->BeginLineArray(8);
	
	v->AddLine(bounds.LeftBottom(), bounds.LeftTop(), kMediumGray);
	v->AddLine(bounds.LeftTop(), bounds.RightTop(), kMediumGray);
	v->AddLine(bounds.RightTop(), bounds.RightBottom(), kWhite);
	v->AddLine(bounds.RightBottom(), bounds.LeftBottom(), kWhite);
	
	bounds.InsetBy(1, 1);
	
	v->AddLine(bounds.LeftBottom(), bounds.LeftTop(), kDarkGray);
	v->AddLine(bounds.LeftTop(), bounds.RightTop(), kDarkGray);
	v->AddLine(bounds.RightTop(), bounds.RightBottom(), kLightGray);
	v->AddLine(bounds.RightBottom(), bounds.LeftBottom(), kLightGray);

	v->EndLineArray();
}

void
AddRaisedBevel(BView *v, BRect bounds, bool outset)
{
	rgb_color c;
	
	if (outset)
		bounds.InsetBy(-3, -3);

	v->SetHighColor(90,90,90,0);
	v->StrokeRect(bounds);
	
	bounds.InsetBy(1,1);
	
	v->BeginLineArray(8);
	
	c.red = c.green = c.blue = 160;		// dark gray
	v->AddLine(bounds.RightTop(), bounds.RightBottom(), c);
	v->AddLine(bounds.RightBottom(), bounds.LeftBottom(), c);
	c.red = c.green = c.blue = 195;		//gray
	v->AddLine(bounds.LeftBottom(), bounds.LeftTop(), c);
	v->AddLine(bounds.LeftTop(), bounds.RightTop(), c);
	
	bounds.InsetBy(1, 1);
		
	c.red = c.green = c.blue = 195;		// med gray
	v->AddLine(bounds.RightTop(), bounds.RightBottom(), c);
	v->AddLine(bounds.RightBottom(), bounds.LeftBottom(), c);
	c.red = c.green = c.blue = 230;		// neat white
	v->AddLine(bounds.LeftBottom(), bounds.LeftTop(), c);
	v->AddLine(bounds.LeftTop(), bounds.RightTop(), c);

	v->EndLineArray();
}

void
DrawFancyBorder(BView* v)
{
	BRect		b;

	rgb_color white = {255, 255, 255, 255};
	rgb_color hilite = tint_color(v->ViewColor(), 1.5);

	b = v->Bounds();

	v->BeginLineArray(10);

	// left side
	v->AddLine(BPoint(b.left, b.top),			BPoint(b.left, b.bottom-1),
		hilite);
	v->AddLine(BPoint(b.left+1, b.top+1),		BPoint(b.left+1, b.bottom-2),
		white);
	
	// bottom
	v->AddLine(BPoint(b.left+1, b.bottom-1),	BPoint(b.right-1, b.bottom-1),
		hilite);
	v->AddLine(BPoint(b.left+1, b.bottom),		BPoint(b.right-1, b.bottom),
		white);

	// right
	v->AddLine(BPoint(b.right-1, b.bottom-1),	BPoint(b.right-1, b.top),
		hilite);
	v->AddLine(BPoint(b.right, b.bottom),		BPoint(b.right, b.top+1),
		white);

	v->AddLine(BPoint(b.left+1, b.top),
			BPoint(b.right-1, b.top),
			hilite);
	v->AddLine(BPoint(b.left+2, b.top+1),
			BPoint(b.right-2, b.top+1),
			white);

	v->EndLineArray();
}

float GetMenuFieldSize(BMenuField* field, float* width, float* height)
{
	BMenu* popup = field->Menu();
	
	font_height fhs;
	field->GetFontHeight(&fhs);
	const float fh = fhs.ascent+fhs.descent+fhs.leading;
	float fw = field->StringWidth("WWWW");
	
	float pref_w=0;
	if( popup ) {
		int32 num = popup->CountItems();
		for( int32 i=0; i<num; i++ ) {
			BMenuItem* item = popup->ItemAt(i);
			if( item ) {
				const float w=field->StringWidth(item->Label());
				if( w > pref_w ) pref_w = w;
			}
		}
	}
	
	float lw = (field->Label() && *field->Label())
		? field->StringWidth(field->Label()) + field->StringWidth(" ") + 5
		: 0;
	field->SetDivider(lw);
	*width = (fw>pref_w?fw:pref_w) + 20 + lw;
	*height = fh + 12;
	return lw;
}

status_t BitmapFromCursor(const uint8* data, size_t size,
						  translator_info* out_info,
						  TranslatorBitmap* out_header,
						  BMessage* out_io_extension,
						  BBitmap** out_bitmap)
{
	if( out_bitmap ) *out_bitmap = 0;
	
	if( size != 68 && size != 0 ) return B_BAD_VALUE;
	
	if( out_info ) out_info->type = B_CURSOR_TYPE;
	if( out_header ) {
		out_header->magic = B_TRANSLATOR_BITMAP;
		out_header->bounds = BRect(0, 0, 15, 15);
		out_header->rowBytes = 16;
		out_header->colors = B_CMAP8;
		out_header->dataSize = 256;
	}
	if( out_io_extension && size > 3 ) {
		out_io_extension->AddInt32("be:y_hotspot", data[2]);
		out_io_extension->AddInt32("be:x_hotspot", data[3]);
	}
	
	if( size != 0 ) {
		if( data[0] != 16 ) return B_BAD_VALUE;
		if( data[1] != 1 ) return B_BAD_VALUE;
	}
	
	BScreen s;
	const uint8 black = s.IndexForColor(0, 0, 0);
	const uint8 white = s.IndexForColor(255, 255, 255);
	const uint8 gray = s.IndexForColor(128, 128, 128);
	
	if( out_bitmap ) {
		BBitmap* bm = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);
		if( data && size == 68 ) {
			uint8* bits = (uint8*)bm->Bits();
			for( int32 y=0; y<16; y++ ) {
				for( int32 x=0; x<16; x++ ) {
					bool color = (*(data+4+y*2+x/8)) & (0x80>>(x%8));
					bool trans = !( (*(data+4+32+y*2+x/8)) & (0x80>>(x%8)) );
					uint8* bitaddr = bits + y*bm->BytesPerRow() + x;
					if( trans ) {
						if( color ) *bitaddr = gray;
						else *bitaddr = B_TRANSPARENT_MAGIC_CMAP8;
					} else {
						*bitaddr = color ? black : white;
					}
				}
			}
		}
		*out_bitmap = bm;
	}
	
	return B_OK;
}

uint8* CursorFromBitmap(const BBitmap* bm,
						const BMessage* io_extension,
						size_t* out_size)
{
	if( bm->Bounds().Width() != 15 || bm->Bounds().Height() != 15 ||
			bm->ColorSpace() != B_CMAP8 ) return 0;
	
	uint8* cdata = (uint8*)malloc(68);
	if( out_size ) *out_size = 68;
	cdata[0] = 16;
	cdata[1] = 1;
	cdata[2] = cdata[3] = 0;
	if( io_extension ) {
		int32 val;
		if( io_extension->FindInt32("be:y_hotspot", &val) == B_OK ) {
			cdata[2] = (uint8)val;
		}
		if( io_extension->FindInt32("be:x_hotspot", &val) == B_OK ) {
			cdata[3] = (uint8)val;
		}
	}
	for( int32 i=0; i<32; i++ ) {
		cdata[4+i] = 0;
		cdata[4+32+i] = 0;
	}
	
	BScreen s;
	uint8* bits = (uint8*)bm->Bits();
	for( int32 y=0; y<16; y++ ) {
		for( int32 x=0; x<16; x++ ) {
			const uint8 index = *( bits + y*bm->BytesPerRow() + x);
			rgb_color rgb = index == B_TRANSPARENT_MAGIC_CMAP8
					? B_TRANSPARENT_COLOR : s.ColorForIndex(index);
			bool color, trans;
			if( rgb.alpha < 127 ) {
				color = false;
				trans = true;
			} else if( (rgb.red+rgb.green+rgb.blue) >= (255*2) ) {
				color = false;
				trans = false;
			} else if( (rgb.red+rgb.green+rgb.blue) >= 255 ) {
				color = true;
				trans = true;
			} else {
				color = true;
				trans = false;
			}
			if( color ) {
				(*(cdata+4+y*2+x/8)) |= (0x80>>(x%8));
			}
			if( !trans ) {
				(*(cdata+4+32+y*2+x/8)) |= (0x80>>(x%8));
			}
		}
	}
	
	return cdata;
}

void
PrintRGB(rgb_color c)
{
	printf("color is r:%i g:%i b:%i a: %i\n", c.red, c.green, c.blue, c.alpha);
}

bool
CompareRGB(rgb_color c1, rgb_color c2)
{
	return (c1.red == c2.red && c1.green == c2.green && c1.blue == c2.blue &&
		c1.alpha == c2.alpha);
}

// ------------------------------ TColorControl ------------------------------

TColorControl::TColorControl(	BPoint start,
								color_control_layout layout,
								float cell_size,
								const char *name,
								BMessage *message,
								bool use_offscreen)
	: BColorControl(start, layout, cell_size, name, message, use_offscreen),
	  fModificationMessage(0), fLastColor(ValueAsColor())
{
}

TColorControl::~TColorControl()
{
	SetModificationMessage(0);
}

void TColorControl::SetModificationMessage(BMessage* msg)
{
	delete fModificationMessage;
	fModificationMessage = msg;
}

BMessage* TColorControl::ModificationMessage() const
{
	return fModificationMessage;
}
	
void
TColorControl::MouseDown(BPoint pt)
{
	BColorControl::MouseDown(pt);
	fLastColor = ValueAsColor();
}

void
TColorControl::MouseMoved(BPoint where, uint32 transit, const BMessage* drop)
{
	BColorControl::MouseMoved(where, transit, drop);
	if( fLastColor != ValueAsColor() ) {
		fLastColor = ValueAsColor();
		if( ModificationMessage() ) {
			InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);
		}
	}
}
