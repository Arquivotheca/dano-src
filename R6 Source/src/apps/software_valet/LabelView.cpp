#include "LabelView.h"
#include "Util.h"

/////////////////////////////////////////////////////////////////////////////

LabelView::LabelView(BRect fr, const char *text)
		: BTextView(fr,B_EMPTY_STRING,BRect(0,0,100,10),
					B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
					B_WILL_DRAW | B_FRAME_EVENTS)
{
	SetText(text);
}

void LabelView::AttachedToWindow()
{
	BTextView::AttachedToWindow();

	BView *parent = Parent();
	if (parent) SetViewColor(parent->ViewColor());
	//else SetViewColor(light_gray_background);
	
	SetLowColor(ViewColor());
	
	BRect r = Bounds();
	r.InsetBy(2,2);
	SetTextRect(r);
	SetWordWrap(TRUE);
	MakeSelectable(FALSE);
	MakeEditable(FALSE);
	
	BFont	inFont(*be_plain_font);
	inFont.SetSize(10.0);
	
	SetFontAndColor(&inFont);
	SetFontSize(10.0);
	//MakeResizable(TRUE);
}

void LabelView::FrameResized(float w, float h)
{
	BTextView::FrameResized(w, h);
	BRect r = Bounds();
	r.InsetBy(2,2);
	SetTextRect(r);
}
