// InstSetWindow.h
//#include "GDescriptionView.h"
#include "PackListView.h"
#include "ChildWindow.h"

#ifndef _INSTSETWINDOW_H
#define _INSTSETWINDOW_H

class PackWindow;

enum {
	M_INST_FOLDER =		'iNfL',
	M_DO_LICENSE =		'DLic',
	M_DO_INSTALLFOLDER=	'DIns',
	M_DO_FOLDERPOPUP=	'DFPo',
	M_DO_PACKAGEHELP=	'DPkH',
	M_DO_GROUPSHELP=	'DGrH',
	M_DO_ABORTSCRIPT =	'DAbS'
	// M_SET_PACKAGEHELP=	'SPkH'
};


class InstSetWindow : public ChildWindow
{
public:
	InstSetWindow(const char *title,PackWindow *parentWin);
	// appease the compiler
virtual			~InstSetWindow();

virtual void	MessageReceived(BMessage *);
virtual bool	QuitRequested();
virtual void	WindowActivated(bool state);

		void	SetDescription();
		void	SetInstallFolder();
		void	SetHelp();

	
	PackWindow		*pw;
	PackList		*parentList;
private:
	BMessenger	licPanelMsngr;
};

class STextField;

class InstSetView : public BView
{
public:
				InstSetView(BRect frame,const char *name,PackWindow *pw);
virtual void	AllAttached();
	
	BTextControl		*inFolderText;
	STextField			*desc;
	STextField			*pkgHelp;
	STextField			*grpsHelp;
	BStringView 		*lFilename;
	
	PackWindow			*pw;
};
#endif
