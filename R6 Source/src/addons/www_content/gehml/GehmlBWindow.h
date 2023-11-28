
#ifndef _GEHMLBWINDOW_H_
#define _GEHMLBWINDOW_H_

#include "GehmlWindow.h"

class GehmlBWindow : public GehmlWindow
{
	public:
										GehmlBWindow(
											const BRect &frame,
											window_look look,
											window_feel feel,
											uint32 flags,
											uint32 workspace = B_CURRENT_WORKSPACE);
		virtual							~GehmlBWindow();

		virtual	void					Acquired();
		virtual	status_t				HandleMessage(BMessage *msg);

		virtual	void					SetVisible(bool visibility);
};

#endif
