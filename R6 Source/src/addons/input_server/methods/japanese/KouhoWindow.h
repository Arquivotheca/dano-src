// ============================================================
//  KouhoWindow.h	by Hiroshi Lockheimer
// ============================================================

#ifndef _KOUHOWINDOW_H
#define _KOUHOWINDOW_H

#include <Window.h>
#include <ListView.h>
#include <ListItem.h>


class HenkanManager;
class KanaKan;
class KouhoView;

#ifdef COUNTVIEW
class CountView;
#endif


class KouhoWindow : public BWindow {
public:
						KouhoWindow(BPoint			where, 
									float			height,
									HenkanManager	*henkanManager,
									KouhoView		**kouhoView);
};


class KouhoView : public BListView {
public:
							KouhoView(BRect		frame, 
							HenkanManager		*henkanManager);

	virtual void		AttachedToWindow();
	virtual void		DetachedFromWindow();
	virtual void		SelectionChanged();
	virtual void		MessageReceived(BMessage* message);

	int32				SelectKouho(int32 kouho, bool visible);
	void				SetBottomUp(bool bottomUp);
	bool				IsBottomUp() const;

#ifdef COUNTVIEW
	void				SetCountView(CountView *countView);
#endif

private:
	HenkanManager*		fHenkanManager;
	const KanaKan*		fKanaKan;
	bool				fNoNotification;
	bool				fHasScroller;
	bool				fBottomUp;
#ifdef COUNTVIEW
	CountView*			fCountView;
#endif
};


class KouhoItem : public BListItem {
public:
						KouhoItem(const char *item, bool small, bool border);
	virtual				~KouhoItem();

	virtual void		DrawItem(BView *owner, BRect bounds, bool complete = false);
	virtual void		Update(BView *owner, const BFont *font);

private:
	char*				fItem;
	float				fTextBaseline;
	bool				fSmall;
	bool				fBorder;
};


class LabelView : public BView {
public:
							LabelView(BRect	frame, 
									  int32	numKouhos, 
									  int32	itemHeight, 
									  bool	bottomUp);
	virtual					~LabelView();

	virtual void			Draw(BRect updateRect);
#if 0
	virtual void			MouseDown(BPoint where);
	virtual void			MouseUp(BPoint where);
	virtual void			MouseMoved(BPoint where, uint32 code, const BMessage *message);
#endif

private:
	BBitmap*				fBitmap;	
	BPoint*					fTrackWhere;
};


#ifdef COUNTVIEW
class CountView : public BView {
public:
							CountView(BRect frame, int32 curCount, int32 totalCount);
	virtual					~CountView();

	virtual void			Draw(BRect updateRect);

	void					SetCurCount(int32 curCount);

private:
	BBitmap*				fBitmap;
	int32					fTotalCount;
};
#endif

#endif