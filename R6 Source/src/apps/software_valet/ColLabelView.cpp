#include "ColLabelView.h"

#include "Util.h"
#include <Window.h>
#include <Application.h>
#include <stdio.h>

const char ColLabelView::resizecursor[] = {
0x10,0x01,0x07,0x07,
0x00,0x00,0x02,0x80,0x02,0x80,0x02,0x80,0x02,0x80,0x12,0x90,0x32,0x98,0x7e,0xfc,
0xfe,0xfe,0x7e,0xfc,0x32,0x98,0x12,0x90,0x02,0x80,0x02,0x80,0x02,0x80,0x02,0x80,
0x00,0x00,0x07,0xc0,0x07,0xc0,0x07,0xc0,0x17,0xd0,0x3f,0xf8,0x7f,0xfc,0xff,0xfe,
0xff,0xff,0xff,0xfe,0x7f,0xfc,0x3f,0xf8,0x17,0xd0,0x07,0xc0,0x07,0xc0,0x07,0xc0,
0x00,0x08,0x00,0x07 };

#if 0
const char ColLabelView::resizecursor[] = {
0x01,0x00,0x03,0x80,0x07,0xc0,0x0f,0xe0,0x03,0x80,0x03,0x80,0xff,0xfe,0x00,0x00,
0xff,0xfe,0x03,0x80,0x03,0x80,0x0f,0xe0,0x07,0xc0,0x03,0x80,0x01,0x00,0x00,0x00,
0x03,0x80,0x07,0xc0,0x0f,0xe0,0x1f,0xf0,0x0f,0xe0,0xff,0xfe,0xff,0xfe,0xff,0xfe,
0xff,0xfe,0xff,0xfe,0x0f,0xe0,0x1f,0xf0,0x0f,0xe0,0x07,0xc0,0x03,0x80,0x01,0x00,
0x00,0x07,0x00,0x07 };
#endif

ColLabelView::ColLabelView(BRect frame,
				const char *name,
				ulong resize, 
				ulong flags)
	: BView(frame,name,resize,flags /*| B_FULL_UPDATE_ON_RESIZE */)
{
	fColumns = NULL;
	fTarget = NULL;
	showingResizeCursor = false;
}

void ColLabelView::SetTarget(BView *v)
{
	fTarget = v;
}

void ColLabelView::MouseMoved(BPoint where, uint32 code,
							const BMessage *)
{
	if (Window()->IsActive()) {
		if (code == B_EXITED_VIEW) {
			if (showingResizeCursor) {
				be_app->SetCursor(B_HAND_CURSOR);
				showingResizeCursor = false;
			}
		} else if (MouseInDivider(where)) {
			if (!showingResizeCursor) {
				be_app->SetCursor(resizecursor);
				showingResizeCursor = true;					
			}		
		} else if (showingResizeCursor) { // mouse is not in the divider
			be_app->SetCursor(B_HAND_CURSOR);
			showingResizeCursor = false;		
		}
	}
}

void ColLabelView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetFont(be_plain_font);
	
	SetLowColor(light_gray_background);
	
	// no flicker on invalidate!
	SetViewColor(B_TRANSPARENT_32_BIT);
}

void ColLabelView::SetColumnList(RList<ColumnInfo *> *ci)
{
	fColumns = ci;
}


void ColLabelView::DrawBackground()
{
	BRect cur = Bounds();


	//cur.InsetBy(0,1);
	
	SetPenSize(1.0);
	SetHighColor(90,90,90);
	// horiz
	StrokeLine(BPoint(cur.left,cur.top),BPoint(cur.right,cur.top));
	StrokeLine(BPoint(cur.left,cur.bottom),BPoint(cur.right,cur.bottom));
	// vert
	//StrokeLine(BPoint(cur.left,cur.top+1),BPoint(cur.left,cur.bottom-1));
	//StrokeLine(BPoint(cur.right,cur.top+1),BPoint(cur.right,cur.bottom-1));

	cur.InsetBy(0,1);
	SetHighColor(232,232,232);
	// h
	StrokeLine(BPoint(cur.left,cur.top),BPoint(cur.right,cur.top));
	// v
	//StrokeLine(BPoint(cur.left,cur.top),BPoint(cur.left,cur.bottom));
	
	BRect c = cur;
	//c.left++;
	c.top++;
	SetHighColor(148,148,148);
	StrokeLine(BPoint(c.left,c.bottom),BPoint(c.right,c.bottom));
	// StrokeLine(BPoint(c.right,c.top),BPoint(c.right,c.bottom));

	c.bottom--;
	// c.right--;
	SetHighColor(255,255,255);
	FillRect(c);
	
	// c.left++;
	c.top++;
	SetHighColor(188,188,188);
	StrokeLine(BPoint(c.left,c.bottom),BPoint(c.right,c.bottom));
	// StrokeLine(BPoint(c.right,c.top),BPoint(c.right,c.bottom));
	
	c.InsetBy(0,1);
	SetHighColor(232,232,232);
	FillRect(c);
}

/*
index ColLabelView::CellHit(BPoint pt)
{

}
*/


// deal with hidden columns!!!
void ColLabelView::Draw(BRect up)
{
	up;
	if (!fColumns)
		return;
		
	BRect bnds = Bounds();
	/*
	// since we can't expand the clipping region!!!!!

	if (up != bnds) {
		Invalidate(bnds);
		// Window()->UpdateIfNeeded();
		return;
	}
	*/
	// inefficient
	DrawBackground();
	
	float drawBottom = bnds.top + 8;
	float drawMiddle = bnds.top + 13;
	float fudgeFactor = 6;
	
	SetLowColor(B_TRANSPARENT_32_BIT);
	SetDrawingMode(B_OP_OVER);

	long max = fColumns->CountItems();
	BRect frame = bnds;
	frame.left = 0;	
	for(long i = 0; i < max; i++)
	{
		ColumnInfo *cInfo = fColumns->ItemAt(i);
		if (cInfo->hidden)
			continue;
		
		long columnWidth = (long)cInfo->width;
		const char *c = cInfo->title;
		
		frame.right = frame.left + columnWidth - 1;
		SetHighColor(90,90,90);
		StrokeLine(BPoint(frame.right-1,frame.top),
					BPoint(frame.right-1,frame.bottom));
		SetHighColor(255,255,255);
		StrokeLine(BPoint(frame.right,frame.top+1),
					BPoint(frame.right,frame.bottom-1));
		
		bool newline = FALSE;
		while (*c) {
			if (*c++ == '\n') {
				newline = TRUE;
				break;
			}
		}

		SetHighColor(label_red);
		
		if (newline) {
			MovePenTo(frame.left+fudgeFactor,drawBottom);
			const char *c = cInfo->title;
			while(*c) {
				if (*c == '\n') {
					MovePenTo(frame.left+fudgeFactor,drawBottom+10);
					c++;
				}
				DrawChar(*c++);
			}
		}
		else {
			MovePenTo(frame.left+fudgeFactor,drawMiddle);
			DrawString(cInfo->title);
		}
			
		frame.left = frame.right + 1;
	}

	SetHighColor(0,0,0);
}

bool	ColLabelView::MouseInDivider(BPoint where)
{
	long max = fColumns->CountItems();
	BRect bnds = Bounds();
	BRect frame = bnds;
	frame.left = 0;	
	for(long i = 0; i < max; i++)
	{
		ColumnInfo *cInfo = fColumns->ItemAt(i);
		long columnWidth = (long)cInfo->width;
		if (cInfo->hidden)
			continue;
			
		if (frame.left > where.x) {
			return false;
		}
		
		frame.right = frame.left + columnWidth - 1;
		
		if (where.x > frame.right - 4 &&
			where.x < frame.right + 3)
		{
			return true;			
		}
		frame.left = frame.right + 1;
	}
	return false;
}

// deal with hidden columns!!!
void	ColLabelView::MouseDown(BPoint where)
{
	BRect tbnds;
	if (fTarget)
		tbnds = fTarget->Bounds();

	long max = fColumns->CountItems();
	BRect bnds = Bounds();
	BRect frame = bnds;
	frame.left = 0;	
	for(long i = 0; i < max; i++)
	{
		ColumnInfo *cInfo = fColumns->ItemAt(i);
		long columnWidth = (long)cInfo->width;
		if (cInfo->hidden)
			continue;
		if (frame.left > where.x) {
			break;
		}
		frame.right = frame.left + columnWidth - 1;
		
		if (where.x > frame.right - 4 &&
			where.x < frame.right + 3)
		{
			bool adjusted = false;	
			uint32 buttons;
			do {
				GetMouse(&where, &buttons);
				
				if (where.x <= bnds.right - 1) {
					float newwidth = where.x - frame.left;
					newwidth = max_c(cInfo->minWidth, newwidth);
					
					if (newwidth != cInfo->width) {
						adjusted = true;
						cInfo->width = newwidth;
						
						// compute invalid frame
						BRect r = frame;
						r.left += cInfo->minWidth - 2;
						r.right = bnds.right;
						Invalidate(r);
						Window()->UpdateIfNeeded();
						
						// redraw data
						if (fTarget) {
							BRect tr = tbnds;
							tbnds.left = frame.left;
							fTarget->Draw(tbnds);
							//Window()->UpdateIfNeeded();
						}
					}
				}
				snooze(30*1000);
			} while (buttons & B_PRIMARY_MOUSE_BUTTON);
			if (showingResizeCursor && !(Frame().Contains(where))) {
				showingResizeCursor = false;
				be_app->SetCursor(B_HAND_CURSOR);
			}
		}
		frame.left = frame.right + 1;
	}
}
