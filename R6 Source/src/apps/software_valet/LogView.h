#ifndef _LOGVIEW_H_
#define _LOGVIEW_H_

#include <View.h>

class LogView : public BView
{
public:
	LogView(BRect frame);
	
	virtual void	AttachedToWindow();
};


#endif
