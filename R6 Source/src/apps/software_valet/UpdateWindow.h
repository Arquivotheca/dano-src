#ifndef _UPDATEWINDOW_H_
#define _UPDATEWINDOW_H_


#include "RList.h"
#include <Message.h>
#include <View.h>
#include <Window.h>
#include <StringView.h>
#include <TextView.h>

class UpgradeItemList;
class UpgradeItem;
class UpItemsView;
	
class UpdateDisplayView : public BView
{
public:
	UpdateDisplayView ( BRect frame );
	
	virtual void	AttachedToWindow();
	void SetItemList( RList < UpgradeItem * > *, const char *msg = NULL);
private:	
	UpItemsView *vw;
};

class UpdateWindow : public BWindow
{
public:
					UpdateWindow(bool canShowOld);
	virtual 		~UpdateWindow();
	virtual void 	MessageReceived(BMessage *);
			void	SetControls();
private:
	UpdateDisplayView			*upView;
	BStringView					*countView;
	BTextView					*titleView;
	
	RList<UpgradeItemList *>	reportList;
	BMessage					pinfo;
	int32						curIndex;
	int32						totalCount;
};

#endif

