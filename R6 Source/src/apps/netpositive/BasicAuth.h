//=============================================================================
//  BasicAuth.h
//  Copyright 1998 by Be Incorporated.
//=============================================================================

#ifndef __BASICAUTH__
#define __BASICAUTH__
 
#include "FolderWatcher.h"

#include <SupportDefs.h>
#include <String.h>


//=============================================================================
//	Lookup a password for a www realm or ftp host
//	If none is found, present a dialog and ask the user

class BasicAuthItem;

class BasicAuthFolder : public FolderWatcher {
public:
	virtual FolderItem*		CreateFolderItem() const;

	static FolderWatcher*	CreateFolderWatcher();

	// 		For basic WWW auth
	
	const char*	Get(const char *realm);
	void		Bad(const char *realm);
	void		Good(const char *realm);

	//		For ftp
	
	bool		GetFTP(const char *realm, char* user, char* pass, bool mustAsk);
	
	
protected:
	virtual FolderItem* FindItem(const BMessage& attrs) const;
	virtual FolderItem* FindItem(const char *filename) const {return FolderWatcher::FindItem(filename);}
	BasicAuthFolder(const BEntry& path,const char *fileType,const char *appSig,BLooper *looper,bool recursive,bool createDir);

	static int32		InitThreadEntry(void *data);
	
	static	bool		RunPasswordWindow(const char *realm, BString& user, BString& auth, bool ftp, bool *save);
	void		Delete(BasicAuthItem *ba);
};

extern BasicAuthFolder *gBasicAuthFolder;


class BasicAuthItem : public FolderItem {		
public:			
	BasicAuthItem();
	BasicAuthItem(const char* realm, const char *user, const char *auth, bool ftp, bool save);
	virtual void	ReadMessageAttributes(BMessage *msg);
	void		Save(BMessage& msg);
	bool		ShouldSave() {return mSave;}
	void		ShouldSave(bool save) {mSave = save;}
	
//	static BasicAuthItem*	FindEntry(const char *filename);
//	static BasicAuthItem*	FindEntry(const BasicAuth *duplicate);
	
	const char*	Realm();
	const char*	User();
	const char*	Auth();
	
	
	bool		mFTP;			// For ftp
	BString		mUser;
	
	BString		mAuth;			// For basic WWW auth
	BString		mRealm;			// http realm of ftp host
	
	bool		mSave;
};


#endif
