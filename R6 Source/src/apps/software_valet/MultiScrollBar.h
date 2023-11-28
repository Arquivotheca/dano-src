#ifndef _MULTISCROLLBAR_H_
#define _MULTISCROLLBAR_H_


#include "RList.h"
#include <ScrollBar.h>


class MultiScrollBar : public BScrollBar
{
public:
	MultiScrollBar(	BRect frame,
					const char *name,
					BView *target,
					long min,
					long max,
					orientation direction);
	virtual	~MultiScrollBar();
	
	void			AddExtraTarget(BView *);
	
	virtual void	ValueChanged(float newValue);
private:
	RList<BView *>	*extraTargets;
	long			oldValue;
};

#endif

