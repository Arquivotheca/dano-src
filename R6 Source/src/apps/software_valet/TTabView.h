#ifndef _TTabView_H
#define _TTabView_H

#ifndef _VIEW_H
#include <View.h>
#endif

class TTabView : public BView
{
public:
			TTabView(BRect inFrame, const char *inName, ulong inresizingMode, ulong inFlags);
	virtual			~TTabView();
	virtual void 	Draw(BRect inUpdateRect);
	virtual void	AddChild(BView*,const char*);
	virtual void	AddChild(BView*);
	virtual void	AttachedToWindow();
	virtual void	MouseDown(BPoint);
	
	virtual void	ActivateView(long i);
			long	CurrentView();
private:
	BList	*mLabels;
	BList	*mViews;
	long	mCurrent;
	long	mBehavior;

};

#endif
