#ifndef _MANAGERLISTVIEW_H_
#define _MANAGERLISTVIEW_H_


// ManagerListView.h

#include "SimpleListView.h"
#include "ColLabelView.h"

class ManagerListView : public SimpleListView
{
public:
	ManagerListView(	BRect r,
						BHandler *_fTarget);
						
	virtual			~ManagerListView();	// remove items

virtual void		AttachedToWindow();
virtual void		Draw(BRect);
virtual void		DrawItem(BRect updt,
							long item,
							BRect *itemFrame = NULL);

virtual void		HighlightItem(bool on, long index, BRect *iFrame);
virtual void		KeyDown(const char *bytes, int32 numBytes);

virtual void		SelectionSet();
virtual void		Invoke(long index);
virtual void		MessageReceived(BMessage *msg);
//		void		DrawStringClipped(	const char *str,
//										BPoint where,
//										float len);
virtual void		FrameResized(float,float);
		void		FixHScroll();
		BView		*LabelView();
private:
	BHandler				*fTarget;
	const	long			IHeight;
	RList<ColumnInfo *>		*ColList;
//	BScrollBar				*hScroll;
	ColLabelView			*fCl;
	enum {
		ICONTAG, NAMETAG, VERSIONTAG, SIZETAG,
		DESCTAG, REGTAG
	} columnTags;
	BBitmap				*fOffBitmap;
	BView				*fOffView;
};


enum {
	M_ITEMS_SELECTED	= 'ISel',
	M_COLUMN_DISPLAY	= 'CDsp',
	M_FILTER_DISPLAY	= 'FDsp',
	M_SELECT_ALL		= 'SAll'
};

void DrawStringClipped(BView *v, const char *str,BPoint where,float len);

#endif
