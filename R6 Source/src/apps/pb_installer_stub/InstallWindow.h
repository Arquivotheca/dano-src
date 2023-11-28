// InstallWindow.h
#ifndef _INSTALLWINDOW_H_
#define _INSTALLWINDOW_H_


#include "FileEntry.h"
#include "RList.h"
#include "IStatusWindow.h"

#include <Locker.h>
#include <Window.h>
#include <Volume.h>

class DestManager;
class InstallPack;
class ArchiveFolderItem;
class InstallLooper;
class PackAttrib;
class ArcCatalog;

class InstallWindow : public BWindow
{
public:
				InstallWindow(const char *name, InstallPack *arc,bool _lowMem);

virtual			~InstallWindow();
virtual bool	QuitRequested();

virtual void	MessageReceived(BMessage *m);

		bool	CheckPossibleInstall(BVolume &vol);
		void	DoInstall(BMessage *msg);
		
		void	Abort();
		
	DestManager			*dest;
	InstallPack			*archiveFile;

	FileEntry			*entryList;
	long				entryCount;
	ArchiveFolderItem	*topfolder;  // NULL in lowmemory mode
	InstallLooper		*installLooper;

	PackAttrib			*attr;

	ulong			selectedGroups;
	ulong			selectedPlatforms;

	long			groupBytes;
	long			itemCount;
	
	long			volumeID;
	char			*dirName;
	entry_ref		instDirRef;
	bool			custom;
	bool			installing;
	bool			lowMemMode;

	static			void	SetCloseApp(bool);	
	static			bool	LockWindowList();
	static			void	UnlockWindowList();
	
	static RList<InstallWindow *>	windowList;
private:
	static BLocker	windowListLock;
	
	ArcCatalog		*fCatalog;
	static bool		fCloseApp;	
	status_t		BuildGroupList( FileEntry **entryList,
								FileEntry *entryMax,
								bool topFolder,
								uint32 &instFoldGroups);
};


/**********************
	Error messages
**********************/

/////// InstallWindow.cpp
#define errREADONLY		"Cannot install on the volume \"%s\" because it is locked or read-only."
#define errNOSPACE		"Cannot install on the volume \"%s\" because there is not enough free space."
#define errNONBVOL		"Cannot install on the volume \"%s\" because it does not support the reuqired filesystem features\nTry installing to a BeOS volume."

#define errCANTINST		"Sorry installation cannot begin."
// generic

#endif
