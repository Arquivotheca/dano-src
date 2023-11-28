// Attributes.h
#include "RList.h"
#include "GroupList.h"
#include "DestinationList.h"
#include <Path.h>

#ifndef _ATTRIBUTES_H
#define _ATTRIBUTES_H

class AttribData
{
public:
	AttribData();
	~AttribData();
	void SetupDefaults();

	// shared with edit window	
	GroupList			*groupList;
	
	// also shared with another edit window
	RList<DestItem *>	*defaultDestList;
	DestList			*customDestList;
	
	// installer settings stuff
	bool				doInstallFolder;
	bool				doFolderPopup;
	bool				showLicense;
	BPath				licenseFile;
	
	BBitmap				*splashBitmap;
	
	// package description
	char 				*descriptionText;
	
	// package help
	bool				doPackageHelp;
	char				*packageHelpText;
	
	// group help
	bool				doGroupHelp;
	char				*groupHelpText;

	// general package and developer info	
	char		*name;
	char		*version;
	char		*developer;
	char		*description;

	time_t		releaseDate;
	char		*serialno;

	int32		softType;
	char		*prefixID;
	char		*versionID;
	
	bool		doReg;
	bool		doUpdate;
	bool		abortScript;
};

enum {
	PKGF_ISUPDATE =		0x00000001,
	PKGF_DOREG =		0x00000002,
	PKGF_DOUPDATE =		0x00000004,
	PKGF_ABORTSCRIPT =	0x00000008
};

extern const int kPathsCount;

struct findDestStruct {
	const char *path;
	const char *name;
	long	   code;
};

extern struct findDestStruct kPaths[];
#endif
