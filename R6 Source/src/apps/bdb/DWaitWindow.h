// DWaitWindow.h

#ifndef DWAITWINDOW_H
#define DWAITWINDOW_H 1

#include <Window.h>
class BTextView;

class DWaitWindow : public BWindow
{
public:
	DWaitWindow(BRect frame, const char* msg);

private:
	BTextView* mText;
};

#endif
