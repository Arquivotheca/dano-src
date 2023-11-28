#ifndef _UNINSTALLWINDOW_H_
#define _UNINSTALLWINDOW_H_

// UninstallWindow.h

#include <View.h>
#include <Window.h>
#include <Messenger.h>
#include <Rect.h>

class UninstallWindow : public BWindow
{
public:
	UninstallWindow(const char *pkname, BMessenger &);
	
	virtual bool	QuitRequested();
};


/*****************************************************/
class MThread;
class UninstallTree;
class PackageItem;

class UninstallView : public BView
{
public:
	UninstallView(BRect frame, BMessenger &);
	virtual ~UninstallView();
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage *msg);
	
	virtual void	Draw(BRect up);
	
			void	FindFiles(const char *pkname);
	
	enum {
		U_REMOVE	= 'URmv',
		U_PROGRESS	= 'UPrg',
		U_DONE		= 'UDon',
		U_CANCEL	= 'UCan'
	};
		MThread			*uninstThread;
private:	

	UninstallTree	*uTree;
	PackageItem		*it;
	BMessenger		flistView;
};

#endif
