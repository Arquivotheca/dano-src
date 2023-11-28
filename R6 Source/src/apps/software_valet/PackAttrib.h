#ifndef _PACKATTRIB_H_
#define _PACKATTRIB_H_


// PackAttrib.h
#include "IGroupList.h"
#include "RList.h"
#include <Message.h>
#include <Bitmap.h>



class DestItem;

class PackAttrib
{
public:
	PackAttrib();
	~PackAttrib();
	void SetupDefaults();

	GroupList			*groupList;
	
	RList<DestItem *>	*defaultDest;
	RList<DestItem *>	*customDest;
	
	// installer settings stuff
	bool				doInstallFolder;
	bool				doFolderPopup;


	BBitmap				*splashBitmap;
	
	// total filecount
	long				fileCount;
	uint32				instFolderGroups;

	// license text
	char				*licenseText;
	size_t				licenseSize;
	char				*licenseStyle;
	// package description
	char 				*descriptionText;
	// packagehelp	
	char				*packageHelpText;

	bool				abortScripts;	
	// general package and developer info	
	char		*name;
	char		*version;
	char		*developer;
	char		*description;
	char		*devSerial;
	
	int32		releaseDate;
	bool		doReg;
	bool		doUpdate;
	
	BMessage	contactInfo;
	
	// information downloaded w/ package
	BMessage	downloadInfo;
};

enum {
	PKGF_ISUPDATE =		0x00000001,
	PKGF_DOREG =		0x00000002,
	PKGF_DOUPDATE = 	0x00000004,
	PKGF_ABORTSCRIPT =	0x00000008
};

#endif
