// PositionWindow.cpp
#include <Window.h>
#include <Rect.h>
#include <Screen.h>

void PositionWindow(BWindow *w,float horizFrac, float vertFrac);
void PositionWindow(BWindow *, BPoint, float, float);

void PositionWindow(BWindow *w,float horizFrac, float vertFrac)
{
	float x, y;
	float h, v;
	
	BRect scFrame;
	{
		BScreen	screen(w);
		scFrame = screen.Frame();
	}
	scFrame.InsetBy(6,6);
	
	h = w->Frame().Width();
	v = w->Frame().Height();
	
	x = (scFrame.right - h)*horizFrac;
	if (x < scFrame.left)
		x = scFrame.left;
	if (x + h > scFrame.right)
		x = scFrame.right - h;
	y = (scFrame.bottom - v)*vertFrac;
	if (y < scFrame.top)
		y = scFrame.top;
	if (y + v > scFrame.bottom)
		y = scFrame.bottom - v;

	w->MoveTo(x,y);
}

void PositionWindow(BWindow *w, BPoint prefer, float horizFrac, float vertFrac)
{
	BRect scFrame;
	{
		BScreen	screen(w);
		scFrame = screen.Frame();
	}
	scFrame.InsetBy(8,26);
	if (scFrame.Contains(prefer))
		w->MoveTo(prefer);
	else
		PositionWindow(w,horizFrac,vertFrac);
}
