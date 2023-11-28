// LabelView.h

#ifndef LABEL_VIEW_H
#define LABEL_VIEW_H

#include <TextView.h>

class LabelView : public BTextView
{
public:
	LabelView(BRect fr,const char *txt);
	
	virtual void	AttachedToWindow();
	virtual void	FrameResized(float w, float h);
private:
	const char		*txtPtr;
};

#endif
