#ifndef _ZOOM_CONTROL_H_
#define _ZOOM_CONTROL_H_

#include <MenuBar.h>

class ZoomControl : public BMenuBar
{
	public:
						ZoomControl(BRect frame, const char *name);
		void			AttachedToWindow();
		status_t		SetTarget(BHandler *handler);
	private:
		BMenu *			fMenu;
};



#endif
