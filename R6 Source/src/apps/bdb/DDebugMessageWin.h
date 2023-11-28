// simple window to display the debugger() message

#ifndef DDebugMessageWin_H
#define DDebugMessageWin_H 1

#include <Window.h>
class BTextView;
class BString;

class DDebugMessageWin : public BWindow
{
public:
	DDebugMessageWin(const BString& message);
	~DDebugMessageWin();

private:
	BTextView* fMessageView;
};

#endif
