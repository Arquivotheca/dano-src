#ifndef _PACKAGEITEM_H_
#define _PACKAGEITEM_H_

#define _EXPORTED_ 0

// PackageItem.h
#include "SimpleListItem.h"
#include "RList.h"
#include "UpgradeList.h"
#include <malloc.h>
#include <Entry.h>
#include <Message.h>
#include <Messenger.h>
#include <Node.h>
#include <string.h>


// Installed Package
class PackageItem : public ListItem
{
public:
	PackageItem();
	//PackageItem(const char *name,
	//			const char *version,
	//			const char *developer,
	//			const char *description );			
	//PackageItem(const PackageItem &item);
	//PackageItem & operator=(const PackageItem &item);
	
	//virtual 		~PackageItem();

	bool			ValetSupported();
	
// ref to the database entry (oldstyle)
	entry_ref		oldFileRef;
	
// ref to resource file and resource id (newstyle)
	entry_ref				fileRef;
	int32					resID;
	
	BMessage				data;
	BMessage				updates;
	
	UpgradeItemList				*Updates();
	
	// dependencies ?
	// RList<char *>	*uses;
	// "PackageBuilder/0.9d3/StarCode Software"
	// min version	0.9d2
	// expected version	0.9d3
	
	enum {
		REGISTERED_NO, REGISTERED_YES, REGISTERED_WAITING
	};

	enum {
		SOFT_TYPE_COMMERCIAL	= 0,
		SOFT_TYPE_TRIAL			= 1,
		SOFT_TYPE_FREEWARE		= 2,
		SOFT_TYPE_SHAREWARE		= 3,
		SOFT_TYPE_BETA			= 4,
		SOFT_TYPE_OTHER 		= 255
	};
	
	enum {
		MEDIA_TYPE_ESD		= 0,
		MEDIA_TYPE_PHYSICAL	= 1
	};
	
	/// UI stuff	
	//bool			busy;				// ???
	BMessenger		uinstWind;
	BMessenger		regWind;
	
	bool			fRemapped;
private:
	friend class	PackageDB;
	
	bool					fFilled;
	UpgradeItemList			fUpdates;
	char					supported;
};

// Uninstalled Package
class UPackageItem : public ListItem
{
public:
	UPackageItem(const char *fn, const char *pn) {
//		pn;
		fileName = strdup(fn);
		packageName = NULL;
	};
	~UPackageItem() {
		if (fileName)
			free(fileName);	
	
	};
	UPackageItem(entry_ref &_fRef) {
		fileName = NULL;
		fileRef = _fRef;
		
		BNode node(&fileRef);
		node_ref nodeRef;
		node.GetNodeRef(&nodeRef);
		inode = nodeRef.node;
	};
	entry_ref	fileRef;
	ino_t		inode;
	
	char		*fileName;
	char		*packageName;
	char		*versString;
	
};

enum {
	M_ITEM_MODIFIED		= 'MItM'
};

#define NEW_REG_SERIAL "0"

#endif
