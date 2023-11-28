// DAboutWindow

#ifndef DABOUTWINDOW_H
#define DABOUTWINDOW_H 1

#include <Window.h>

class DAboutWindow : public BWindow
{
public:
	DAboutWindow(BRect frame);
	bool QuitRequested();
};

extern DAboutWindow* gAboutWin;

#endif
