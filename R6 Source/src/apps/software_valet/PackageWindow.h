#ifndef _PACKAGEWINDOW_H_
#define _PACKAGEWINDOW_H_

#include <Window.h>
#include <View.h>
#include <Handler.h>


class PackageWindow : public BWindow
{
public:
	PackageWindow();

	virtual bool QuitRequested();
	virtual	void DispatchMessage(BMessage *msg, BHandler *handler);
};

class PackageView : public BView
{
public:
	PackageView(BRect frame);
	
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *);
};

#endif
