#ifndef _MANAGERWINDOW_H_
#define _MANAGERWINDOW_H_

#include <Window.h>
#include <View.h>
#include <Messenger.h>

#include "RList.h"
// ManagerWindow.h

class SettingsManager;

class ManagerWindow : public BWindow
{
public:
	ManagerWindow(SettingsManager *);
	
	virtual bool	QuitRequested();
	// virtual void	MessageReceived(BMessage *m);
	virtual void	DispatchMessage(BMessage *msg, BHandler *h);
	
	enum {
		PKG_DISPLAY		= 'PDis'
	};
private:
	SettingsManager *settings;
};


class PackageItem;
class PackageDB;
class ManagerListView;

class	BBView : public BView
{
public:
	BBView(BRect frame,
				const char *name,
				uint32 resizeMask,
				uint32 flags);
				
	virtual void Draw(BRect fr);
};

class ManagerView : public BView
{
public:
	ManagerView(BRect frame);
	virtual			~ManagerView();
	
		status_t	ReadAllPackages();
		void		UpdateViewList(bool filterDups);
		
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage *m);
			
			void	HandleSelection(PackageItem *it, bool multiple);
			
	enum {
		M_REGISTER		= 'MReg',
		M_UPDATE		= 'MUpd',
//		M_TROLL			= 'MTro',
		M_REMOVE		= 'MRmv',
		M_UNINSTALL		= 'MUni',
		M_SETTINGS		= 'MSet',
		M_MOREINFO		= 'MInf',
		M_SHOWLOG		= 'SLog'
	};
	
	static char 			*btnnames[];
	BButton					**btns;
	PackageItem				*curSel;
	bool					mulSel;
	RList <PackageItem *>	fItems;
	ManagerListView 		*lv;
	
	PackageDB		*pDB;
	BMessenger		backupPanel;
	BMessenger		restorePanel;
};

extern const char		*kListenerSig;

enum {
	PKG_UPDATE =	'PUpd'
};

#endif
