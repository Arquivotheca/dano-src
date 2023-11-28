#include "ColorPicker.h"

//
//
//

TColorSwatch::TColorSwatch(BRect r, rgb_color c, BMessage *msg)
	: BControl(r, "swatch", "", msg, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
		B_WILL_DRAW | B_NAVIGABLE)
{
	fSelected = false;
	fColor = c;
}

TColorSwatch::~TColorSwatch()
{
}

inline bool operator==(rgb_color c1, rgb_color c2)
{
	return (*((uint32*)&c1)) == (*((uint32*)&c2));
}

void
TColorSwatch::Draw(BRect updateRect)
{
	BControl::Draw(updateRect);
	
	BRect r = Bounds();
	
	if( ! (fColor.operator==(B_TRANSPARENT_COLOR)) ) {
		SetHighColor(fColor);
		r.InsetBy(2,2);
		FillRect(r);
	} else {
		SetFont(be_plain_font);
		float w = StringWidth("T");
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		rgb_color black = { 0, 0, 0, 255 };
		SetHighColor(black);
		font_height fh;
		be_plain_font->GetHeight(&fh);
		DrawString("T", BPoint((r.right+1-w)/2, (r.bottom+1-fh.ascent-fh.descent)/2+fh.ascent));
	}

	AddBevel(this, Bounds(), false);

	if (fSelected && Window()->IsActive() && IsEnabled()) {
		SetHighColor(102,102,152);
//		SetHighColor(keyboard_navigation_color());
		r = Bounds();
		r.InsetBy(1,1);
		StrokeRect(r);
	}
	
}

status_t
TColorSwatch::Invoke(BMessage *msg)
{
	return BControl::Invoke(msg);
}

void 
TColorSwatch::MessageReceived(BMessage *message)
{
/*	if (message->WasDropped()) {
		rgb_color *color;
		long size;
		if (message->FindData("RGBColor", 'RGBC', &color, &size) == B_NO_ERROR) {
			
			fColor = *color;
			Invalidate();
			return;
		}
	}*/			// !!!!! need to add passing of new color to parent
	BControl::MessageReceived(message);
}

void
TColorSwatch::MouseDown(BPoint pt)
{
	if (!fSelected) {
		fSelected = true;
		((TColorPicker*)Parent())->ChangeSelection(this);
		Invoke();
		BControl::MouseDown(pt);
		Draw(Bounds());
	}
}

rgb_color
TColorSwatch::Color()
{
	return fColor;
}

void
TColorSwatch::SetColor(rgb_color c)
{
	fColor = c;
	Invalidate();
}

bool
TColorSwatch::Selected()
{
	return fSelected;
}

void
TColorSwatch::SetSelected(bool state)
{
	fSelected = state;
	Draw(Bounds());
}

//


const int32 kHCells = 8;
const int32 kVCells = 32;
const int32 kCellSize = 8;

TColorControl::TColorControl(BPoint loc, int32 initialSelection)
	: BView( BRect(loc.x,loc.y,loc.x+(kVCells*8+2),loc.y+(kHCells*8+2)), "color picker",
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW ),
	fSelected(initialSelection)
{
}


TColorControl::~TColorControl()
{
}

void 
TColorControl::Draw(BRect)
{
	float left,right,top,bottom;
	rgb_color c;
	
	BeginLineArray( kHCells * kVCells + 4);
	
	c.red = c.green = c.blue = 255;
	
	left = 0; right = Bounds().Width();
	top = bottom = 0;
	for (int32 hindex=0 ; hindex < (kHCells + 2) ; hindex++) {
		AddLine(BPoint(left,top), BPoint(right,bottom), c);
		top = bottom = top + (kCellSize + 1); 
	}
	
	left = right = 0;
	top = 0;  bottom = Bounds().Height();
	for (int32 vindex=0 ; vindex < (kVCells + 2) ; vindex++) {
		AddLine(BPoint(left,top), BPoint(right,bottom), c);
		left = right = left + (kCellSize + 1);
	}
	
	EndLineArray();
	
	StrokeRect(Bounds());
}

void
TColorControl::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}

void
TColorControl::MouseDown(BPoint pt)
{
	BView::MouseDown(pt);
}
		
void 
TColorControl::KeyDown(const char *bytes, int32 n)
{
	if (!IsFocus())
		return;
		
	switch (bytes[0]) {
		case B_TAB:
printf("got a tab\n");
			break;
		case B_DOWN_ARROW:
		case B_LEFT_ARROW:
			break;
		case B_UP_ARROW:
		case B_RIGHT_ARROW:
			break;
		case B_ENTER:
		case B_SPACE:
			break;
		default:
			BView::KeyDown(bytes, n);
			break;
	}
}

//
										
extern uchar	cursor_dropper[];

// 88				
TColorPicker::TColorPicker(BRect frame, rgb_color foreColor, rgb_color backColor)
	: BBox(frame, "color picker", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE_JUMP)
{
	fSelected = true;
	fForeColor = foreColor;
	fBackColor = backColor;
	
	float w,h;
	fColors = new BColorControl( BPoint(10+48+10,10), B_CELLS_32x8, 8, "color picker",
		new BMessage(msg_color_pick));
	fColors->GetPreferredSize(&w,&h);
	fColors->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
//	fColors = new TColorControl(BPoint(68,10),1);
	
	ResizeTo(20 + (kToolCount*kToolWidth) + (kToolCount-1)*2, 10 + h + 10);
	
	SetFont(be_plain_font);
	SetFontSize(9.0);
	
	font_height f;
	be_plain_font->GetHeight(&f);
	float fh = f.ascent + f.descent;
		
	BRect r(10,10,10+32,26);
	fForeColorBtn = new TColorSwatch(r, foreColor, new BMessage(msg_fore_color));
	AddChild(fForeColorBtn);

	w = StringWidth("Front") + 10;
	r.top = 28; r.bottom = r.top + fh;
	r.left += 3;
	r.right = r.left + w;
	fForeColorLabel = new BStringView(r, "fore", "Front", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	AddChild(fForeColorLabel);	
	
	r.left -= 3;	r.right = r.left + 32;
	r.bottom = 10 + h; r.top = r.bottom - 16;
	fBackColorBtn = new TColorSwatch(r, backColor, new BMessage(msg_back_color));
	AddChild(fBackColorBtn);
	
	w = StringWidth("Back") + 10;
	r.bottom = r.top + 1; r.top = r.bottom - fh;
	r.left += 3;
	r.right = r.left + w;
	fBackColorLabel = new BStringView(r, "back", "Back", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	AddChild(fBackColorLabel);

	r.left = fColors->Frame().left - 20;
	r.right = r.left + 16;
	r.top = fColors->Frame().top + 20;
	r.bottom = r.top + 16;
	fTransparentBtn = new BButton(r, "trans","T", new BMessage(msg_trans_color), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fTransparentBtn);

	AddChild(fColors);
	fColors->SetValue((rgb_color) fForeColor);
}

TColorPicker::~TColorPicker()
{
}

void
TColorPicker::AttachedToWindow()
{
	fColors->SetTarget(this,NULL);
	fForeColorBtn->SetTarget(this,NULL);
	fBackColorBtn->SetTarget(this,NULL);
	fTransparentBtn->SetTarget(this,NULL);
	fForeColorBtn->SetSelected(true);
}

void
TColorPicker::Draw(BRect updateRect)
{
	BBox::Draw(updateRect);
}

void
TColorPicker::MessageReceived(BMessage *msg)
{
	uint8 i=0;
	BMessage *newMsg;

	switch (msg->what) {
		case msg_trans_color:		// from T button
			fColors->SetValue(B_TRANSPARENT_8_BIT);
			if (fSelected) {
				newMsg = new BMessage(msg_fore_color);
				fForeColorBtn->SetColor(B_TRANSPARENT_32_BIT);
			} else {
				newMsg = new BMessage(msg_back_color);
				fBackColorBtn->SetColor(B_TRANSPARENT_32_BIT);
			}
			newMsg->AddInt8("colorindex", B_TRANSPARENT_8_BIT);
			SetMessage(newMsg);
			Invoke();
			break;
		case msg_color_pick:	// from color picker
			i = IndexForColor(fColors->ValueAsColor());
			if (fSelected) {
				newMsg = new BMessage(msg_fore_color);
				fForeColorBtn->SetColor(fColors->ValueAsColor());
			} else {
				newMsg = new BMessage(msg_back_color);
				fBackColorBtn->SetColor(fColors->ValueAsColor());
			}
			newMsg->AddInt8("colorindex", i);
			SetMessage(newMsg);
			Invoke();
			break;
		case msg_fore_color:
			fSelected = true;
			break;
		case msg_back_color:
			fSelected = false;
			break;
			
		default:
			BBox::MessageReceived(msg);
			break;
	}
}

void
TColorPicker::MouseMoved(BPoint where, uint32 code, const BMessage *a_message)
{
#if 0
	switch (code) {
		case B_EXITED_VIEW:
			if ( (Window()->IsActive()) && (a_message == NULL) )
				be_app->SetCursor(B_HAND_CURSOR);
			break;
			
		default:
			if (Window()->IsActive())
				be_app->SetCursor(cursor_dropper);

			BBox::MouseMoved(where, code, a_message);
			break;
	}
#endif
	if ( (Window()->IsActive()) && (a_message == NULL) )
		be_app->SetCursor(B_HAND_CURSOR);
	BBox::MouseMoved(where, code, a_message);
}

void
TColorPicker::GetPreferredSize(float *w, float *h)
{
	fColors->GetPreferredSize(w,h);
	*w += 10+32+10;		// to accomodate the color swatches
}

void
TColorPicker::Colors(rgb_color *fore, rgb_color *back)
{
	*fore = fForeColor;
	*back = fBackColor;
}

void
TColorPicker::SetColors(rgb_color fore, rgb_color back)
{
	SetBackColor(back);
	SetForeColor(fore);
}

void
TColorPicker::SetForeColor(rgb_color c)
{
	fForeColor = c;
	
	fForeColorBtn->SetColor(fForeColor);
		
	fColors->SetValue(c);
}

void
TColorPicker::SetForeColor(uint8 i)
{
	if (i == B_TRANSPARENT_8_BIT) {
		fForeColor = B_TRANSPARENT_32_BIT;
		fColors->SetValue(B_TRANSPARENT_8_BIT);
	} else {
		fForeColor = ColorForIndex(i);
		fColors->SetValue(fForeColor);
	}
	
	fForeColorBtn->SetColor(fForeColor);
}

void
TColorPicker::SetBackColor(rgb_color c)
{
	fBackColor = c;
	
	fBackColorBtn->SetColor(fBackColor);
		
	fColors->SetValue(c);
}

void
TColorPicker::SetBackColor(uint8 i)
{
	if (i == B_TRANSPARENT_8_BIT) {
		fBackColor = B_TRANSPARENT_32_BIT;
		fColors->SetValue(B_TRANSPARENT_8_BIT);
	} else {
		fBackColor = ColorForIndex(i);
		fColors->SetValue(fBackColor);
	}
	
	fBackColorBtn->SetColor(fBackColor);
}

bool
TColorPicker::Selection()
{
	return fSelected;
}

void
TColorPicker::ChangeSelection(TColorSwatch *selectedIcon)
{
	if (selectedIcon == fForeColorBtn) {
		fBackColorBtn->SetSelected(false);
		fColors->SetValue(fForeColorBtn->Color());
	} else {
		fForeColorBtn->SetSelected(false);
		fColors->SetValue(fBackColorBtn->Color());
	}
}

