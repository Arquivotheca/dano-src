#ifndef _REGISTERWINDOW_H_
#define _REGISTERWINDOW_H_

#include <Window.h>
#include <Message.h>
#include <Messenger.h>

class PackageItem;

class RegisterWindow : public BWindow
{
public:
	RegisterWindow(	PackageItem *sel,
					BMessenger updt,
					bool quitWhenDone = false/*,
					bool freeP = false*/);
	virtual ~RegisterWindow();
	virtual void	Quit();
private:
	PackageItem *sel;
	BMessage	curPkgs;
	bool fQuitWhenDone;
};

#endif

