#ifndef GRAPHICVIEW_H
#define GRAPHICVIEW_H

#include "graphic.h"
#include "interactor.h"

#include <Handler.h>

class BWindow;

class BGraphicView : public BMGraphicGroup, public BMMouseTracker, public BHandler
{
public:
			BGraphicView(BRect frame,
						const char *name,
						uint32 resizeMask,
						uint32 flags);
	virtual 	~BGraphicView();
	
	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	
	virtual void	SetWindow(BWindow *);
	
protected:
	
private:
};

#endif
