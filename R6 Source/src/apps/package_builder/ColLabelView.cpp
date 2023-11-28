#include <Be.h>

#include "ColLabelView.h"
#include "FListView.h"

#include "Util.h"

ColLabelView::ColLabelView(BRect frame,
				const char *name,
				ulong resize, 
				ulong flags)
	: BBox(frame,NULL,resize,flags)
{
	name;
	fColumns = NULL;
}

void ColLabelView::AttachedToWindow()
{
	BBox::AttachedToWindow();
	SetFont(be_plain_font);
	SetFontSize(9);
	SetViewColor(light_gray_background);
	SetLowColor(light_gray_background);
}

void ColLabelView::SetColumnList(RList<ColumnInfo *> *ci)
{
	fColumns = ci;
}

void ColLabelView::Draw(BRect up)
{
	BBox::Draw(up);
	
	float drawBottom = Bounds().top + 10;
	float drawMiddle = Bounds().top + 14;
	float fudgeFactor = 12;
	
	SetHighColor(200,0,0);
	SetDrawingMode(B_OP_COPY);

	long max = fColumns->CountItems();
	long columnLeft = 0;
	for(long i = 0; i < max; i++)
	{
		ColumnInfo *cInfo = fColumns->ItemAt(i);
		long columnWidth = cInfo->width;
		const char *c = cInfo->title;
		bool newline = FALSE;
		while (*c) {
			if (*c++ == '\n') {
				newline = TRUE;
				break;
			}
		}
		
		if (newline) {
			MovePenTo(columnLeft+fudgeFactor,drawBottom);
			const char *c = cInfo->title;
			while(*c) {
				if (*c == '\n') {
					MovePenTo(columnLeft+fudgeFactor,drawBottom+10);
					c++;
				}
				// bad dependency on DrawChar!!
				DrawChar(*c++);
			}
		}
		else {
			MovePenTo(columnLeft+fudgeFactor,drawMiddle);
			DrawString(cInfo->title);
		}
			
		columnLeft += columnWidth+1;
	}

//	SetHighColor(0,0,0);
//	StrokeRect(Bounds());
}
