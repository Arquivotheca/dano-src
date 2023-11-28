#include "KalView.h"

KalView::KalView (BRect rect, char *title)
	   	:BView (rect, title, B_FOLLOW_ALL, B_WILL_DRAW)
{
}

void KalView::AttachedToWindow()
{
	SetViewColor(0, 0, 0);
};

void KalView::Draw (BRect updateRect)
{
	SetHighColor(0, 0, 0);
	FillRect(updateRect);
}
