#include "PackAttrib.h"
#include <stdlib.h>


PackAttrib::PackAttrib()
	:	doInstallFolder(true),
		doFolderPopup(true),
		splashBitmap(0),
		fileCount(0),
		instFolderGroups(0),
		licenseText(0),
		licenseSize(0),
		licenseStyle(0),
		descriptionText(0),
		packageHelpText(0),
		abortScripts(false),
		name(0),
		version(0),
		developer(0),
		description(0),
		devSerial(0),
		releaseDate(0),
		doReg(false),
		doUpdate(false)
{
	groupList = new GroupList;
	
	// deleted by DestManager (gross, check this to make sure)
	defaultDest = new RList<DestItem *>;
	customDest = new RList<DestItem *>;
}

PackAttrib::~PackAttrib()
{
	delete groupList;
	
	free(descriptionText);
	free(packageHelpText);
		
	free(name);
	free(version);
	free(developer);
	free(description);
	free(devSerial);
}
