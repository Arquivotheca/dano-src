#include "InstallWindow.h"
#include "ArcCatalog.h"
#include "InstallPack.h"
#include "IArchiveItem.h"
#include "IArchivePatchItem.h"
#include "IArchiveScriptItem.h"
#include "InstallerType.h"

#include "Util.h"
#include "MyDebug.h"

status_t InstallWindow::BuildGroupList( FileEntry **entryList,
								FileEntry *entryMax,
								bool topFolder,
								uint32 &instFoldGroups)
{
	long err = B_NO_ERROR;
	
	
	/*** get folder ***/

	ArchiveFolderItem	*curFolder;
	fCatalog->GetFolderData((ArchiveItem **)&curFolder);
	
	/**
		do action
	**/
	
	PRINT(("got folder %s\n",curFolder->name));
	if (!topFolder && curFolder->destination == D_INSTALL_FOLDER) {
		instFoldGroups |= curFolder->groups;
	}
	
	int32 subitem;
	subitem = 0;
	while (subitem = fCatalog->GetFolderItemType()) {
		switch(subitem) {
			case FOLDER_FLAG:
				err = BuildGroupList(entryList,entryMax,FALSE,instFoldGroups);
				if (err < B_NO_ERROR)
					return err;
				break;
			case FILE_FLAG:
			case PATCH_FLAG:
			case SCRIPT_FLAG:
			case LINK_FLAG:
				ArchiveItem		*curItem;
				fCatalog->GetItemData(&curItem, subitem);
				if ((*entryList) < entryMax)
				{
					(*entryList)->size = curItem->uncompressedSize;
					(*entryList)->groups = curItem->groups;
					(*entryList)->platform = curItem->platform;
					(*entryList)++;
					if (curItem->destination == D_INSTALL_FOLDER)
						instFoldGroups |= curItem->groups;
				}
				else {
					doError(errNUMENTRIES);
					err = B_ERROR;
				}
				break;
		}
	}
	return err;
}
