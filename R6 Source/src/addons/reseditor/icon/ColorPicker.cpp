#include "ColorPicker.h"

#include "utils.h"

#include <experimental/BitmapTools.h>

#include <Debug.h>
#include <Slider.h>

// ------------------------------ TColorSwatch ------------------------------

TColorSwatch::TColorSwatch(BRect r, rgb_color c, BMessage *msg, const char* tip)
	: BControl(r, "swatch", "", msg, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
		B_WILL_DRAW | B_NAVIGABLE),
	  BToolTipable(*this, tip)
{
	fSelected = false;
	fColor = c;
	SetViewColor(B_TRANSPARENT_COLOR);
}

TColorSwatch::~TColorSwatch()
{
}

void
TColorSwatch::Draw(BRect updateRect)
{
	BControl::Draw(updateRect);
	
	static const rgb_color black = { 0, 0, 0, 255 };
	static const rgb_color white = { 255, 255, 255, 255 };
	
	BRect r = Bounds();
	
	SetFont(be_bold_font);
	float w = StringWidth("T");
	font_height fh;
	be_plain_font->GetHeight(&fh);
	
	SetDrawingMode(B_OP_COPY);

	r.InsetBy(2,2);
	
	float left = floor( r.left + (r.Width()+1-w) / 2 + .5 );
	BRect inside(left-2, r.top, left+w+2, r.bottom);
	if( inside.left < r.left ) inside.left = r.left;
	if( inside.right > r.right ) inside.right = r.right;
	
	float hashWidth = floor((r.Width()-inside.Width())/4);
	if( hashWidth < 0 ) hashWidth = 0;
	inside.left = r.left + hashWidth*2;
	inside.right = r.right - hashWidth*2;
	
	SetHighColor(mix_color(ui_color(B_PANEL_BACKGROUND_COLOR), fColor, fColor.alpha));
	FillRect(inside);
	
	float ymiddle = floor((r.top+r.bottom)/2);
	if( hashWidth > 0 ) {
		SetHighColor(mix_color(black, fColor, fColor.alpha));
		FillRect(BRect(r.left, r.top, r.left+hashWidth-1, ymiddle));
		FillRect(BRect(r.left+hashWidth, ymiddle+1, r.left+hashWidth*2-1, r.bottom));
		FillRect(BRect(r.right-hashWidth*2+1, r.top, r.right-hashWidth, ymiddle));
		FillRect(BRect(r.right-hashWidth+1, ymiddle+1, r.right, r.bottom));
		SetHighColor(mix_color( white, fColor, fColor.alpha));
		FillRect(BRect(r.left+hashWidth, r.top, r.left+hashWidth*2-1, ymiddle));
		FillRect(BRect(r.left, ymiddle+1, r.left+hashWidth-1, r.bottom));
		FillRect(BRect(r.right-hashWidth+1, r.top, r.right, ymiddle));
		FillRect(BRect(r.right-hashWidth*2+1, ymiddle+1, r.right-hashWidth, r.bottom));
	}
	
	SetHighColor(mix_color(black, fColor, fColor.alpha));
	SetLowColor(mix_color(ui_color(B_PANEL_BACKGROUND_COLOR), fColor, fColor.alpha));

	DrawString("T", BPoint(left,
						   r.top + (r.Height()+1-fh.ascent-fh.descent)/2 + fh.ascent));

	if( fSelected ) AddBevel(this, Bounds(), false);
	else AddRaisedBevel(this, Bounds().InsetBySelf(-1, -1), false);
	
	if( IsFocus() && Window()->IsActive() ) {
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(Bounds().InsetBySelf(1, 1));
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
	if (message->WasDropped()) {
		rgb_color *color;
		long size;
		if (message->FindData("RGBColor", 'RGBC', (const void **)&color, &size) == B_OK) {
			((TColorPicker*)Parent())->SetColorFor(this, *color, true);
			return;
		}
	}
	BControl::MessageReceived(message);
}

void
TColorSwatch::MouseDown(BPoint pt)
{
	bigtime_t doubleClickSpeed;
	get_click_speed(&doubleClickSpeed);
	bigtime_t startDragTime = system_time() + doubleClickSpeed;
	
	for (;;) {
		BPoint newWhere;
		uint32 buttons;
		GetMouse(&newWhere, &buttons);
		if (!buttons)
			break;
		
		BRect moveMargin(pt, pt);
		moveMargin.InsetBy(-1, -1);
		
		if (system_time() > startDragTime || !moveMargin.Contains(newWhere)) {
			
			// initiate drag&drop - drag a color swatch around
			const int32 kSwatchSize = 12;
			BRect bitmapBounds(0, 0, kSwatchSize, kSwatchSize);
			BBitmap *bitmap = new BBitmap(bitmapBounds, B_RGBA32, true);
			BView *view = new BView(bitmapBounds, "", B_FOLLOW_NONE, 0);
			bitmap->AddChild(view);
			bitmap->Lock();
			rgb_color value = Color();
			view->SetHighColor(Color());
			view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			view->FillRect(view->Bounds());
			DrawFancyBorder(view);
			view->Sync();
			bitmap->Unlock();
			BMessage dragMessage(B_SIMPLE_DATA);
			dragMessage.AddData("RGBColor", 'RGBC', &value, sizeof(value));
			DragMessage(&dragMessage, bitmap, B_OP_ALPHA,
						BPoint(kSwatchSize - 2, kSwatchSize - 2));
			return;
		} 
	}
	
	if (!fSelected) {
		fSelected = true;
		((TColorPicker*)Parent())->ChangeSelection(this);
		Invoke();
		Draw(Bounds());
	}
}

void 
TColorSwatch::KeyDown(const char *bytes, int32 n)
{
	if (!IsFocus()) {
		BControl::KeyDown(bytes, n);
		return;
	}
		
	switch (bytes[0]) {
		case B_ENTER:
		case B_SPACE:
			fSelected = true;
			((TColorPicker*)Parent())->ChangeSelection(this);
			Invoke();
			Draw(Bounds());
			break;
		default:
			BControl::KeyDown(bytes, n);
			break;
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
	if( c != fColor ) {
		fColor = c;
		Draw(Bounds());
	}
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

// ------------------------------ TSlider ------------------------------

class TSlider : public BSlider, public BToolTipable
{
public:
	TSlider(BRect frame,
			const char *name,
			const char *label,
			const char *tip,
			BMessage *message,
			int32 minValue,
			int32 maxValue,
			orientation posture /*= B_HORIZONTAL*/,
			thumb_style thumbType = B_BLOCK_THUMB,
			uint32 resizingMode = B_FOLLOW_LEFT |
								B_FOLLOW_TOP,
			uint32 flags = B_NAVIGABLE | B_WILL_DRAW |
								B_FRAME_EVENTS)
		: BSlider(frame, name, label, message, minValue, maxValue,
				  posture, thumbType, resizingMode, flags),
		  BToolTipable(*this, tip)
	{
	}
	
	virtual ~TSlider()
	{
	}
};


// ------------------------------ TColorPicker ------------------------------

// 88				
TColorPicker::TColorPicker(BRect frame, rgb_color foreColor, rgb_color backColor)
	: BView(frame, "color picker", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE_JUMP)
{
	fSelected = true;
	fForeColor = foreColor;
	fBackColor = backColor;
	
	float w,h;
	fColors = new TColorControl( BPoint(32+5+16+5,0), B_CELLS_32x8, 8, "color picker",
								 new BMessage(msg_new_color), true);
	fColors->SetModificationMessage(new BMessage(msg_mod_color));
	fColors->GetPreferredSize(&w,&h);
	fColors->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
//	fColors = new TColorControl(BPoint(68,10),1);
	
	float pw, ph;
	GetPreferredSize(&pw, &ph);
	ResizeTo(pw, ph);
	
	SetFont(be_plain_font);
	SetFontSize(9.0);
	
	font_height f;
	be_plain_font->GetHeight(&f);
	float fh = f.ascent + f.descent;
		
	float swatchHeight = floor( (ph - fh*2 - 8)/2 + .5 );
	
	BRect r(0,0,32,swatchHeight);
	fForeColorBtn = new TColorSwatch(r, foreColor, new BMessage(msg_fore_color),
									 "Foreground Color (Primary Mouse Button)");
	AddChild(fForeColorBtn);

	w = StringWidth("Front") + 10;
	r.top = swatchHeight+2; r.bottom = r.top + fh;
	r.left += 3;
	r.right = r.left + w;
	fForeColorLabel = new BStringView(r, "fore", "Front", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	AddChild(fForeColorLabel);	
	
	r.left -= 3;	r.right = r.left + 32;
	r.bottom = h; r.top = r.bottom - swatchHeight;
	fBackColorBtn = new TColorSwatch(r, backColor, new BMessage(msg_back_color),
									 "Background Color (Secondary Mouse Button)");
	AddChild(fBackColorBtn);
	
	w = StringWidth("Back") + 10;
	r.bottom = r.top - 2; r.top = r.bottom - fh;
	r.left += 3;
	r.right = r.left + w;
	fBackColorLabel = new BStringView(r, "back", "Back", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	AddChild(fBackColorLabel);

	r.left = fColors->Frame().left - 20;
	r.right = r.left + 16;
	r.top = fColors->Frame().top;
	r.bottom = fColors->Frame().bottom;
	fAlphaSlider = new TSlider(r, "alpha", 0, "Alpha Channel Level",
							   new BMessage(msg_new_color),
							   0, 255, B_VERTICAL, B_TRIANGLE_THUMB,
							   B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fAlphaSlider->SetModificationMessage(new BMessage(msg_mod_color));
	fAlphaSlider->SetValue(255);
	AddChild(fAlphaSlider);

	AddChild(fColors);
	fColors->SetValue((rgb_color) fForeColor);
}

TColorPicker::~TColorPicker()
{
}

void
TColorPicker::AttachedToWindow()
{
	if( Parent() ) SetViewColor(Parent()->ViewColor());
	if( fColors ) fColors->SetViewColor(B_TRANSPARENT_COLOR);
	fColors->SetTarget(this);
	fForeColorBtn->SetTarget(this);
	fBackColorBtn->SetTarget(this);
	fAlphaSlider->SetTarget(this);
	fForeColorBtn->SetSelected(true);
}

void
TColorPicker::Draw(BRect updateRect)
{
	BView::Draw(updateRect);
}

void
TColorPicker::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped()) {
		rgb_color *color;
		long size;
		if (msg->FindData("RGBColor", 'RGBC', (const void **)&color, &size) == B_OK) {
			if( Selection() ) SetForeColor(*color);
			else SetBackColor(*color);
			return;
		}
	}
	
	switch (msg->what) {
		case msg_mod_color: {
			rgb_color col = fColors->ValueAsColor();
			col.alpha = fAlphaSlider->Value();
			PRINT(("Modified new color: 0x%04lx\n", *(int32*)&col));
			if (fSelected) {
				fForeColorBtn->SetColor(col);
			} else {
				fBackColorBtn->SetColor(col);
			}
		} break;
		case msg_new_color: {
			rgb_color col = fColors->ValueAsColor();
			col.alpha = fAlphaSlider->Value();
			PRINT(("Selected new color: 0x%04lx\n", *(int32*)&col));
			BMessage newMsg;
			if (fSelected) {
				newMsg.what = msg_fore_color;
				fForeColorBtn->SetColor(col);
			} else {
				newMsg.what = msg_back_color;
				fBackColorBtn->SetColor(col);
			}
			newMsg.AddInt32("colorindex", *(int32*)&col);
			InvokeNotify(&newMsg, B_CONTROL_INVOKED);
		} break;
		case msg_fore_color:
			fSelected = true;
			break;
		case msg_back_color:
			fSelected = false;
			break;
			
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void
TColorPicker::MouseMoved(BPoint where, uint32 code, const BMessage *a_message)
{
	BView::MouseMoved(where, code, a_message);
}

void
TColorPicker::GetPreferredSize(float *w, float *h)
{
	fColors->GetPreferredSize(w,h);
	*w += PrefixWidth();		// to accomodate the color swatches
}

float
TColorPicker::PrefixWidth() const
{
	return 32+5+16+5;
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
TColorPicker::SetForeColor(rgb_color c, bool report)
{
	fForeColor = c;
	fForeColorBtn->SetColor(fForeColor);
		
	if( fSelected ) {
		fColors->SetValue(c);
		fAlphaSlider->SetValue(c.alpha);
	}
	
	if( report ) {
		BMessage newMsg(msg_fore_color);
		newMsg.AddInt32("colorindex", *(int32*)&c);
		InvokeNotify(&newMsg, B_CONTROL_INVOKED);
	}
}

void
TColorPicker::SetBackColor(rgb_color c, bool report)
{
	fBackColor = c;
	fBackColorBtn->SetColor(fBackColor);
		
	if( !fSelected ) {
		fColors->SetValue(c);
		fAlphaSlider->SetValue(c.alpha);
	}
	
	if( report ) {
		BMessage newMsg(msg_back_color);
		newMsg.AddInt32("colorindex", *(int32*)&c);
		InvokeNotify(&newMsg, B_CONTROL_INVOKED);
	}
}

void
TColorPicker::SetColorFor(TColorSwatch* who, rgb_color c, bool report)
{
	if( who == fForeColorBtn ) SetForeColor(c, report);
	else if( who == fBackColorBtn ) SetBackColor(c, report);
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
		fAlphaSlider->SetValue(fForeColorBtn->Color().alpha);
	} else {
		fForeColorBtn->SetSelected(false);
		fColors->SetValue(fBackColorBtn->Color());
		fAlphaSlider->SetValue(fBackColorBtn->Color().alpha);
	}
}
