#include <Be.h>
// Attributes.cpp
#include "Attributes.h"

static char kDefaultHelpText[] = \
"Choosing Groups\n\
================\n\
Choose the selected portions of the software that you \
would like to install by selecting from the \"Groups\" \
list on the left. You can view a description (if any) \
and the approximate installed size for each group to \
the right of the list.\n\
\n\
You can select multiple groups by shift or command \
clicking on the desired groups. The approximate total \
installation size will be displayed to the right.\n\
\n\
Destinations\n\
================\n\
To control where software will be installed, use the \
bottom left popup menu to select the desired disk. To \
choose the folder where the primary package files will be \
installed, use the popup menu to the right. Note that some installations \
may not have this menu if they control the installation folder.\n\
\n\
Installation\n\
===============\n\
Click on the \"Begin\" button.\n\
\n\
If there are patches to apply to existing files, the installer \
will show a list of the files. You can choose to delete the \
old files following patching.\n\
\n\
Finishing\n\
===============\n\
If the installation is cancelled, any files that \
were deleted are moved back to their original location.\n\
";

static char *kDefaultDescText = \
"\n\
MyCoolApp         v0.0                 January 1, 1900\n\
\n\
Welcome to the installer for MyCoolApp.\n\
To begin installation, click the \"Begin\" button.\n\
\n\
For assistance with installation, contact tech support at \
techsupport@mycompany.com.\n\
";

AttribData::AttribData()
{
	groupList = new GroupList();		

	/// make this DestList type!	
	defaultDestList = new RList<DestItem *>;

	customDestList = new DestList;
	
	descriptionText = strdup(kDefaultDescText);
	
	doPackageHelp = true;
	packageHelpText = strdup(kDefaultHelpText);
	
	doGroupHelp = FALSE;
	groupHelpText = strdup(B_EMPTY_STRING);
	
	doInstallFolder = TRUE;
	showLicense = FALSE;
	doFolderPopup = TRUE;
	
	splashBitmap = NULL;
	
	name = strdup(B_EMPTY_STRING);
	version = strdup(B_EMPTY_STRING);
	developer = strdup(B_EMPTY_STRING);
	description = strdup(B_EMPTY_STRING);
	releaseDate = time(NULL);
	serialno = strdup(B_EMPTY_STRING);
	
	softType = 0;
	prefixID = strdup(B_EMPTY_STRING);
	versionID = strdup(B_EMPTY_STRING);
	
	licenseFile.SetTo(B_EMPTY_STRING);
	doReg = doUpdate = abortScript = false;
}

AttribData::~AttribData()
{
	// these lists take care of their items
	delete groupList;
	delete customDestList;

	for (long i = defaultDestList->CountItems()-1; i >= 0; i--)
		delete defaultDestList->ItemAt(i);
	delete defaultDestList;
	
	free(descriptionText);
	free(packageHelpText);
	free(groupHelpText);

	delete splashBitmap;
			
	free(name);
	free(version);
	free(developer);
	free(description);
	free(serialno);
	free(prefixID);
	free(versionID);
}


struct findDestStruct kPaths[] = {
	{"home","B_USER_DIRECTORY",B_USER_DIRECTORY},
	{"home/config","B_USER_CONFIG_DIRECTORY",B_USER_CONFIG_DIRECTORY},
	{"home/config/add-ons","B_USER_ADDONS_DIRECTORY",B_USER_ADDONS_DIRECTORY},
	{"home/config/boot","B_USER_BOOT_DIRECTORY",B_USER_BOOT_DIRECTORY},
	{"home/config/fonts","B_USER_FONTS_DIRECTORY",B_USER_FONTS_DIRECTORY},
	{"home/config/lib","B_USER_LIB_DIRECTORY",B_USER_LIB_DIRECTORY},
	{"home/config/be","B_USER_DESKBAR_DIRECTORY",B_USER_DESKBAR_DIRECTORY},
	{"home/config/settings","B_USER_SETTINGS_DIRECTORY",B_USER_SETTINGS_DIRECTORY},

	{"common/config/servers","B_COMMON_SERVERS_DIRECTORY",B_COMMON_SERVERS_DIRECTORY},
	{"common/config/bin","B_COMMON_BIN_DIRECTORY",B_COMMON_BIN_DIRECTORY},
	{"common/config/boot","B_COMMON_BOOT_DIRECTORY",B_COMMON_BOOT_DIRECTORY},	
	{"common/config/etc","B_COMMON_ETC_DIRECTORY",B_COMMON_ETC_DIRECTORY},
	{"common/config/documentation","B_COMMON_DOCUMENTATION_DIRECTORY",B_COMMON_DOCUMENTATION_DIRECTORY},
	{"common/config/settings","B_COMMON_SETTINGS_DIRECTORY",B_COMMON_SETTINGS_DIRECTORY},
	{"common/config/lib","B_COMMON_LIB_DIRECTORY",B_COMMON_LIB_DIRECTORY},
	{"common/config/add-ons","B_COMMON_ADDONS_DIRECTORY",B_COMMON_ADDONS_DIRECTORY},
	{"develop","B_COMMON_DEVELOP_DIRECTORY",B_COMMON_DEVELOP_DIRECTORY},
	{"var/log","B_COMMON_LOG_DIRECTORY",B_COMMON_LOG_DIRECTORY},
	{"var/spool","B_COMMON_SPOOL_DIRECTORY",B_COMMON_SPOOL_DIRECTORY},
	{"var/tmp","B_COMMON_TEMP_DIRECTORY",B_COMMON_TEMP_DIRECTORY},
	
	// these are duplicated
	// /boot/home
	//home/config
	//home/config/add-ons
	//home/config/boot
	//home/config/fonts
	//home/config/lib
	//home/config/settings
	{"apps","B_APPS_DIRECTORY",B_APPS_DIRECTORY},
	{"preferences","B_PREFERENCES_DIRECTORY",B_PREFERENCES_DIRECTORY},
	{"home/Desktop","B_DESKTOP_DIRECTORY",B_DESKTOP_DIRECTORY}

};
const int kPathsCount = 23;

void AttribData::SetupDefaults()
{
	groupList->AddGroup("Standard Install");
	groupList->AddSeparator();
	groupList->AddGroup("Minimal Install");
	
	RList<DestItem *> *l = defaultDestList;
	
	for (int i = 0; i < kPathsCount; i++) {
		l->AddItem(new DestItem(kPaths[i].path, kPaths[i].name, kPaths[i].code));
	}
}
