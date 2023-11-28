// ColLabelView.h
#ifndef _COLLABELVIEW_H_
#define _COLLABELVIEW_H_

#include "RList.h"
#include <View.h>

struct ColumnInfo {
	ColumnInfo(long _t, float _w, float _mw, const char *_til)
		:	tag(_t), width(_w), minWidth(_mw), title(_til), hidden(FALSE) {};
	
	long 		tag;
	float 		width;
	float		minWidth;
	const char *title;
	bool		hidden;
};


class ColLabelView : public BView
{
public:
	ColLabelView(BRect frame,
				const char *name,
				ulong resize, 
				ulong flags);
				
virtual void 	Draw(BRect up);
virtual void	AttachedToWindow();
		void 	SetColumnList(RList<ColumnInfo *> *ci);
		void	DrawBackground();
virtual void	MouseDown(BPoint where);
		void	SetTarget(BView *v);
virtual void	MouseMoved(BPoint, uint32 code,
							const BMessage *);
		bool	MouseInDivider(BPoint where);
protected:
	RList<ColumnInfo *> *fColumns;
	
// currently unused
	long				cellShift;
	long				textIndent;
	BView				*fTarget;
	
	static	const char	resizecursor[];
private:
	bool showingResizeCursor;
};

class ColLabelBgView : public BView
{
public:
	ColLabelBgView(BRect frame,
				const char *name,
				ulong resize, 
				ulong flags);
virtual void 	Draw(BRect up);

};

#endif
