#ifndef _SETUPWINDOW_H_
#define _SETUPWINDOW_H_

#include <Window.h>
#include <View.h>
#include "PackageItem.h"

class SetupWindow : public BWindow
{
public:
					SetupWindow(bool forceShow = false);
	virtual	bool	QuitRequested();
			void	RemoveOldVersions(BEntry *appEnt);
			void	GetButtons();
private:
		PackageItem *InitializePackageDB();
		BMessage	cnfg;
};


class SetupView : public BView
{
public:
					SetupView(BRect f);
			
	//virtual void	MessageReceived(BMessage *msg);
	//virtual void	Draw(BRect r);
};

#endif
