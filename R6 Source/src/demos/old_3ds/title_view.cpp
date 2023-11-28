#include "title_view.h"


//--------------------------------------------------------------


TTitleView::TTitleView (BPoint where, BView *parent)
  : BView (BRect(where.x+0, where.y, where.x + 29, where.y + 55), "Title view",
		   B_FOLLOW_NONE, B_WILL_DRAW)
{
	parent->AddChild(this);
	SetViewColor(216,216,216);
}


//--------------------------------------------------------------

TTitleView::~TTitleView()
{
}


//--------------------------------------------------------------


void	TTitleView::Draw(BRect rr)
{
	BFont	font(be_fixed_font);
	
	font.SetSize(9.5);
	font.SetRotation(-90);
	SetFont(&font);
	MovePenTo(BPoint(11,7));
	SetDrawingMode(B_OP_OVER);
	SetHighColor(50,50,200);
	DrawString("Testing");
}


//--------------------------------------------------------------

