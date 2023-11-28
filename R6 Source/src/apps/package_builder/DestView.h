// DestView.h

#include "DestinationList.h"

#ifndef _DESTVIEW_H
#define _DESTVIEW_H

class EditFindView;

/***** hack ******/
class MyControlView : public BView
{
public:
	MyControlView(	BRect frame,
					const char *name,
					ulong resizeMask,
					ulong flags);
	virtual void	Show();
	virtual void	Hide();
};

class PackWindow;

class DestView : public BView
{
public:
	DestView(BRect fr,DestList *theList,PackWindow *pw);
	
	virtual void AttachedToWindow();
	virtual void AllAttached();
	virtual void MessageReceived(BMessage *);
	
	void		ShowFindView(bool state);

private:
	DestList 		*fList;
	EditFindView	*findView;
	BTextControl	*nControl;
	PackWindow		*pw;
};



class DestListView;

class EditFindView : public BBox
{
public:
	EditFindView(BRect fr,DestListView *_listing);
	
	void		SetEnabled(bool on);
	virtual void MessageReceived(BMessage *msg);
	virtual void AllAttached();
private:
	DestListView	*listing;
	BTextControl	*typeView;
//	BTextControl	*creaView;
	BTextControl	*sizeView;
};

#endif
