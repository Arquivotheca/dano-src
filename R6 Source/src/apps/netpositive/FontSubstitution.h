// ===========================================================================
//	FontSubstitution.h
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#ifndef __FONTSUBSTITUTION__
#define __FONTSUBSTITUTION__

#include "FolderWatcher.h"
#include <String.h>

class FontSubFolder : public FolderWatcher {
public:
	virtual FolderItem*	CreateFolderItem() const;

static FolderWatcher*	CreateFolderWatcher();	
static const char*		MapFont(const char *origFont, int& matchType);

		enum {kMatched, kMapped, kNoMatch};

protected:
						FontSubFolder(const BEntry& path, 
							  		  const char *fileType,
							  		  const char *appSig,
							  		  BLooper *looper, 
							  		  bool recursive,
							  		  bool createDir);
static int32			InitThreadEntry(void *data);
};

class FontSubItem : public FolderItem {
friend class FontSubFolder;
public:
	virtual void		ReadMessageAttributes(BMessage *msg);
	
protected:
	BString	mFont;
	BString	mMapsTo;
};

#endif
