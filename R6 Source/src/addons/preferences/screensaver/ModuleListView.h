#if ! defined MODULELISTVIEW_INCLUDED
#define MODULELISTVIEW_INCLUDED 1

#include <ListView.h>
#include <Rect.h>
#include <Point.h>

class BScrollView;
class BMessage;

class ModuleListView : public BListView
{
	BScrollView	*sv;

public:
			ModuleListView(BRect frame, char *name);
	void	TargetedByScrollView(BScrollView *v);
	void	MouseMoved(BPoint point, uint32 transit, const BMessage *message);
	void	MessageReceived(BMessage *msg);
//	void	MouseDown(BPoint where);
};

#endif
