#include "wave_view.h"


//--------------------------------------------------------------


TWaveView::TWaveView (BPoint where, BView *parent)
  : BView (BRect(where.x+0, where.y, where.x + 29, where.y + 155), "main view",
		   B_FOLLOW_NONE, B_WILL_DRAW)
{
	parent->AddChild(this);
	SetViewColor(200,200,200);
}


//--------------------------------------------------------------

TWaveView::~TWaveView()
{
}


//--------------------------------------------------------------


void	TWaveView::Draw(BRect rr)
{
	BRect	r;
	
	
	r = Bounds();
	//r.InsetBy(1,1);
	SetHighColor(240,240,240);
	
	MovePenTo(BPoint(r.right, r.top));
	StrokeLine(BPoint(r.right, r.bottom));
	StrokeLine(BPoint(r.left, r.bottom));
	SetHighColor(170,170,170);
	StrokeRect(BRect(r.left + 1, r.top + 1, r.right -1, r.bottom - 1));
	SetHighColor(200,200,200);
	MovePenTo(BPoint(r.left, r.bottom - 1));
	StrokeLine(BPoint(r.left, r.top + 0));
	StrokeLine(BPoint(r.right - 1, r.top + 0));
	
}


//--------------------------------------------------------------

