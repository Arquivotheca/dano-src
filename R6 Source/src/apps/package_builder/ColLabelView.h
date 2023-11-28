// ColLabelView.h
#include "FListView.h"

#ifndef _COLLABELVIEW_H
#define _COLLABELVIEW_H

class ColLabelView : public BBox
{
public:
	ColLabelView(BRect frame,
				const char *name,
				ulong resize, 
				ulong flags);
				
virtual void 	Draw(BRect up);
virtual void	AttachedToWindow();
		void 	SetColumnList(RList<ColumnInfo *> *ci);

protected:
	RList<ColumnInfo *> *fColumns;
};

#endif
