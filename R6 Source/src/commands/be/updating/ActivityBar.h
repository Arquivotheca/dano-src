#ifndef ACTIVITY_BAR_H
#define ACTIVITY_BAR_H

#include "RenderView.h"

class class ActivityBar : public BRenderView
{
	public:
		ActivityBar( BRect frame, const char *name, uint32 resizeMask = B_FOLLOW_TOP | B_FOLLOW_LEFT, uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS );
		~ActivityBar( void );
		
	protected:
		virtual void ThreadInit( void );
		virtual BRect Render( void );
};

#endif
