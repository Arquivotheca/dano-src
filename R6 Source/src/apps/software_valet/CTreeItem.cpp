#include <Bitmap.h>
#include "CTreeItem.h"

#include "MyDebug.h"

// make Valet CTreeItem like this version
// so that they can eventually share the same code!


CTreeItem::CTreeItem(const char *name, BBitmap *icon)
	:	TreeItem(name, icon),
		freeBitmap(FALSE)
{
}

CTreeItem::~CTreeItem()
{
	if (freeBitmap && bitmap)
		delete bitmap;
}

float CTreeItem::Height()
{
	return 19.0;
}

void CTreeItem::CollapseTo(long level, long maxitems)
{
	// this can stay expanded
	// unless it has more than
	long count = 0;
	CTreeItem* here = (CTreeItem *)Children();
	while( here)
	{
		if (here->HasChildren()) {
			if (level >= 1)
				here->CollapseTo(level-1,maxitems);
			else {
				here->Collapse();
				here->DoForAllChildren(&TreeItem::Collapse);
			}
		}
		here = (CTreeItem *)here->Sibling();
		count++;
	}
	if (count > maxitems)
		Collapse();
}

void CTreeItem::DrawLabel( TreeView* owner)
{
	if (Label()) {
		owner->SetFont(be_plain_font);
		//owner->SetFontSize(9);	
		font_height info;
		be_plain_font->GetHeight(&info);

		float text_offset = (Height() - info.ascent)/2.0 + info.ascent;

		BPoint location(Position());
		float h = Height();

		location.x += owner->ItemIndent() + h/2.0 + BOX_INDENT + 4;
		location.y += text_offset;

		owner->DrawString(Label(), location);
		
		// PRINT(("draw: %s\n",Label()));
	}
}

void CTreeItem::SetIcon(BBitmap *icon)
{
	bitmap = icon;
}

void CTreeItem::DrawIcon( TreeView* owner)
{
	BPoint location( Position());
	const BBitmap *bitmap = Icon();
	BRect br = bitmap->Bounds();
	float height = Height();
	
	location.x += owner->ItemIndent() + BOX_INDENT - (height / 2);

	// Adjust to center bitmap
	location.x += (height - br.Width()) / 2.0;
	location.y += (height - br.Height()) / 2.0;

#if 0
	if (isSelected()) {
		// owner->DrawBitmapAsync(HIcon(), location);
		
		owner->SetPenSize(2.0);
		owner->SetHighColor(100,0,0);
		BPoint p0, p1;
		
		p0.x = 3.0;
		p0.y = location.y + height/2.0 - 3.0;
		p1.x = p0.x + 3;
		p1.y = location.y + height - 6.0;
		owner->StrokeLine(p0,p1);
		
		p0.x = p1.x + 3;
		p0.y = location.y + 2.0;
		
		owner->StrokeLine(p1,p0);
		
		owner->SetHighColor(0,0,0);
		owner->SetPenSize(1.0);
	}
#endif
	if (bitmap) {
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmapAsync(bitmap, location);
		owner->SetDrawingMode(B_OP_COPY);
	}
	
	/*** draw checkmark **/
}

/*********** triangle tab stuff ****************/
#pragma mark ----triangle tab stuff----

void CTreeItem::AnimateBox(TreeView *owner, bool opening)
{		
	BPoint pos = Position();
	BPoint c( pos.x + BOX_INDENT + 8, pos.y + Height()/2.0);
	
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
		
		// erase
		owner->SetHighColor(owner->ViewColor());
		owner->FillRect(er);
		// draw inside
		owner->SetHighColor(200,200,255);
		owner->FillPolygon(rpts,3);
		for (long n = 0; n < 3; n++) {
			float x = rpts[n].x - c.x;
			float y = rpts[n].y - c.y;
			rpts[n].x += x > 0 ? 1 : -1;
			rpts[n].y += y > 0 ? 1 : -1;
		}
		owner->SetHighColor(0,0,0);
		owner->StrokePolygon(rpts,3);
		owner->Flush();
		snooze(1000*10);
	}
}

void CTreeItem::DrawPlusBox( TreeView* owner)
{
	BPoint pos = Position();
	
	BPoint c( pos.x + BOX_INDENT + 8, pos.y + Height()/2.0);

	BPoint	pts[3];
	pts[0].x = c.x + 1;
	pts[0].y = c.y;
	pts[1].x = c.x - 2;
	pts[1].y = c.y + 4;
	pts[2].x = c.x - 2;
	pts[2].y = c.y - 4;
	
	owner->SetHighColor(200,200,255);
	owner->FillPolygon(pts,3);

	pts[0].x = c.x + 2;
	pts[0].y = c.y;
	pts[1].x = c.x - 3;
	pts[1].y = c.y + 5;
	pts[2].x = c.x - 3;
	pts[2].y = c.y - 5;
	
	owner->SetHighColor(0,0,0);
	owner->StrokePolygon(pts,3);
}

void CTreeItem::DrawMinusBox( TreeView* owner)
{
	BPoint pos = Position();
	BPoint c( pos.x + BOX_INDENT + 8, pos.y + Height()/2.0);

	BPoint	pts[3];
	pts[0].x = c.x;
	pts[0].y = c.y + 1;
	pts[1].x = c.x - 4;
	pts[1].y = c.y - 2;
	pts[2].x = c.x + 4;
	pts[2].y = c.y - 2;
	
	owner->SetHighColor(200,200,255);
	owner->FillPolygon(pts,3);
		
	pts[0].x = c.x;
	pts[0].y = c.y + 2;
	pts[1].x = c.x - 5;
	pts[1].y = c.y - 3;
	pts[2].x = c.x + 5;
	pts[2].y = c.y - 3;

	owner->SetHighColor(0,0,0);	
	owner->StrokePolygon(pts,3);
}
