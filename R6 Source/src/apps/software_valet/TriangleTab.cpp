#include "TriangleTab.h"
#include "MyDebug.h"

TriangleTab::TriangleTab(BRect frame,
					const char *name,
					const char *label,
					BMessage *message,
					ulong resizeMask,
					ulong flags)
	:	BControl(frame,name,label,message,resizeMask,flags | B_WILL_DRAW)
{
}

void TriangleTab::MouseDown(BPoint where)
{
	where;
	
	PRINT(("TT mousedown\n"));
	Animate((Value() == B_CONTROL_OFF));
	// BControl::MouseDown(where);
	SetValue(!Value());
	Invoke();
}

void TriangleTab::Animate(bool opening)
{	
	BRect bnds = Bounds();	
	BPoint pos = bnds.LeftTop();
	
	BPoint c = BPoint( pos.x + bnds.Width()/2.0, pos.y + bnds.Height()/2.0);
	
	// erase rectangle
	BRect er(c.x-8,c.y-8,c.x+8,c.y+8);
	
	float curangle = 0.0;
	float angle;
	
	BPoint pts[3];
	BPoint rpts[3];
	if (opening) {
		pts[0].x = c.x + 1;
		pts[0].y = c.y;
		pts[1].x = c.x - 2;
		pts[1].y = c.y + 4;
		pts[2].x = c.x - 2;
		pts[2].y = c.y - 4;
		
		angle = M_PI/2.0;
	}
	else {
		pts[0].x = c.x;
		pts[0].y = c.y + 1;
		pts[1].x = c.x - 4;
		pts[1].y = c.y - 2;
		pts[2].x = c.x + 4;
		pts[2].y = c.y - 2;
	
		angle = -M_PI/2.0;
	}
	long steps = 8;
	for (long i = 1; i < steps; i++) {
		curangle = (float)i/(float)steps * angle;
		for (long n = 0; n < 3; n++) {
			float x = pts[n].x - c.x;
			float y = pts[n].y - c.y;
			float sinA = sin(curangle);
			float cosA = cos(curangle);
			rpts[n].x = x*cosA - y*sinA + c.x;
			rpts[n].y = x*sinA + y*cosA + c.y;
		}
		// draw inside
		SetHighColor(200,200,255);
		FillPolygon(rpts,3);
		for (long n = 0; n < 3; n++) {
			float x = rpts[n].x - c.x;
			float y = rpts[n].y - c.y;
			rpts[n].x += x > 0 ? 1 : -1;
			rpts[n].y += y > 0 ? 1 : -1;
		}
		SetHighColor(0,0,0);
		StrokePolygon(rpts,3);
		Flush();
		snooze(1000*10);
		// erase
		SetHighColor(ViewColor());
		FillRect(er);
	}
}

void TriangleTab::Draw( BRect updt )
{
	PRINT(("triangle tab draw\n"));
	BControl::Draw(updt);
	
	BRect bnds = Bounds();	
	BPoint pos = bnds.LeftTop();
	
	BPoint c( pos.x + bnds.Width()/2.0, pos.y + bnds.Height()/2.0);

	BPoint	pts[3];
	if (Value() == B_CONTROL_ON) {
		pts[0].x = c.x;
		pts[0].y = c.y + 1;
		pts[1].x = c.x - 4;
		pts[1].y = c.y - 2;
		pts[2].x = c.x + 4;
		pts[2].y = c.y - 2;
	}
	else {
		pts[0].x = c.x + 1;
		pts[0].y = c.y;
		pts[1].x = c.x - 2;
		pts[1].y = c.y + 4;
		pts[2].x = c.x - 2;
		pts[2].y = c.y - 4;
	}
	SetHighColor(200,200,255);
	FillPolygon(pts,3);

	if (Value() == B_CONTROL_ON) {		
		pts[0].x = c.x;
		pts[0].y = c.y + 2;
		pts[1].x = c.x - 5;
		pts[1].y = c.y - 3;
		pts[2].x = c.x + 5;
		pts[2].y = c.y - 3;
	}
	else {
		pts[0].x = c.x + 2;
		pts[0].y = c.y;
		pts[1].x = c.x - 3;
		pts[1].y = c.y + 5;
		pts[2].x = c.x - 3;
		pts[2].y = c.y - 5;
	}

	SetHighColor(0,0,0);	
	StrokePolygon(pts,3);
}
