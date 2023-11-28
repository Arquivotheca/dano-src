#ifndef EDIT_WINDOW_H
#define EDIT_WINDOW_H

#ifndef _WINDOW_H
#include <Window.h>
#endif
#include <MediaKit.h>


class	DataView;
class	StatusView;

class TEditWindow : public BWindow {

public:
				TEditWindow(BRect, const char*);
				~TEditWindow();
virtual	void	MessageReceived(BMessage *b);
		void	TakeRef(entry_ref ref);
public:
		DataView	*dv;
		StatusView	*sv;
};

#endif
