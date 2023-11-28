// InstallView.h
#include <VolumeRoster.h>
#include "IGroupList.h"
#include "InstallerType.h"


class IconMenuItem;
class HelpButton;
class MyVolumeQuery;
class BPopUpMenu;
class BTextView;
class BStringView;

class InstallView : public BView
{
public:
	InstallView(BRect frame,
				const char *name,
				ulong resizeMask,
				ulong flags,GroupList *grps,
				char *description);
virtual			~InstallView();
				
virtual void	AttachedToWindow();
virtual void	Draw(BRect up);
virtual	void	MessageReceived(BMessage *msg);

		void	SelectDefaultFolder();
		void	SelectDefaultVolume();
		void	SetFolderPopup();
BMenuBar		*volMBar;
BPopUpMenu		*volPopup;
IconMenuItem	*volSuperItem;
// long			chosenVolItem;
long			allocBlockSize;

BMenuBar		*foldMBar;
BPopUpMenu		*foldPopup;
IconMenuItem	*foldSuperItem;
long			prevFolder;

GroupList		*groupList;
char			*dText;

private:	
	BTextView		*descText;
	BStringView		*sizeText;
	BStringView		*spaceFree;
	
	#if USING_HELP
	HelpButton		*grpsHelpBtn;
	#endif
	
	BVolumeRoster	VRoster;
};

#define VIEW_TOP_FRAME		(100)
#define VIEW_BOTTOM_FRAME	(8 + 20 + 4 + 8)
#define VOLUME_MENU_MAX		(90)
#define FOLDER_MENU_MAX		(180)
#define VOL_DIVIDER			44
#define FOLD_DIVIDER		18

/*********************
	Error messages
*********************/

/////// InstallView.cpp
#define errNOTOPLEVEL	"Error, the initial folder or file entry list could not be found."
#define errBADCATALOG	"Error, catalog read expected to find toplevel folder. The package cannot be installed."
#define errNOLOWMEM		"Low memory mode is not supported in the self-extracting installer!"

