#ifndef _LOGLISTVIEW_H_
#define _LOGLISTVIEW_H_

#include <ListView.h>
#include <Rect.h>

class LogListView : public BListView
{
public:
	LogListView(BRect r);
	virtual ~LogListView();
	
	virtual	void	AttachedToWindow();
};

#endif

