#ifndef _LABELVIEW_H
#define _LABELVIEW_H


// LabelView.h

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
