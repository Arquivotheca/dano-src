#ifndef _RWINDOW_H
#define _RWINDOW_H


class RWindow : public BWindow
{
public:
	RWindow(BRect frame, const char *title, window_type type, ulong flags);
	virtual void WindowActivated(bool active);
	virtual void Quit();
	static BList WindowList();
	static RWindow *Front();
	static long CountWindows();
	
private:

	static BList			windowList;
	static BLocker 			windowListLock;
};


#endif
