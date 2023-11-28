#include "ChildWindow.h"

#ifndef _SPLASHWINDOW_H
#define _SPLASHWINDOW_H

class PackWindow;

class SplashWindow : public ChildWindow
{
public:
	SplashWindow(const char *title, BBitmap **,PackWindow *parW);			
};



class SplashView : public BView
{
public:
					SplashView(BRect b,BBitmap **,PackWindow *parW);
	
	virtual			~SplashView();
	virtual void	AttachedToWindow();
	virtual	void	MessageReceived(BMessage *msg);
	virtual void	Draw(BRect up);
	virtual void	KeyDown(const char *bytes, int32 numBytes);
private:
	BBitmap **splashBitmap;
	PackWindow	*parW;
};

#endif
