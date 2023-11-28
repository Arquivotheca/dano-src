#ifndef _HELPFILTER_H_
#define _HELPFILTER_H_

#include <MessageFilter.h>
#include <Rect.h>
#include <StringView.h>

class HelpFilter : public BMessageFilter
{
public:
				HelpFilter(const char *txt);
	
	virtual 	~HelpFilter();
	
	virtual		filter_result Filter(BMessage *m, BHandler **);
private:
	char			*fTxt;
};

class HelpStringView : public BStringView
{
public:
	HelpStringView(BRect bounds,
				const char *name, 
				const char *text,
				uint32 resizeFlags =
					B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW);

	virtual void	AttachedToWindow();
};


#endif
