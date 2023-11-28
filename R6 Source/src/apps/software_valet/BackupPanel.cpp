#include "BackupPanel.h"
#include "PackageDB.h"
#include "PackageItem.h"
#include "CopyFile.h"
#include <NodeInfo.h>
#include <Window.h>

BackupPanel::BackupPanel(bool save, BMessenger *dest)
	:	MFilePanel(save ? B_SAVE_PANEL : B_OPEN_PANEL,
					dest,
					NULL,
					true,
					false,
					save ? 'BkTo' : 'RsFr',	// backup to, restore from
					NULL,
					"Select")
{
	if (save) {
		Window()->SetTitle("Backup to...");
		SetSaveText("SoftwareValetSettings.bak");
	}
	else
		Window()->SetTitle("Restore from...");
}

extern const char *kIPkgSig;
status_t	BackupRegistry(entry_ref &ref, const char *name)
{
	status_t	err;
	BDirectory	dDir(&ref);
	if ((err = dDir.InitCheck()) < B_OK)
		return err;
		
	if (dDir.Contains(name)) {
		BEntry	rm(&dDir,name);
		// if not a directory then overwrite
		if (!rm.IsDirectory())
			rm.Remove();
	}
	BDirectory	nDir;
	// create directory (may already be there in case we get an error)
	dDir.CreateDirectory(name,&nDir);
	// grab the directory
	if ((err = nDir.SetTo(&dDir,name)) < B_OK)
		return err;
	
	PackageDB	pDB;
	PackageItem	pitem;
	BFile		tmp;
	BMessenger	unused;
	for (;;) {
		if (pDB.GetNextPackage(&pitem, PackageDB::BASIC_DATA) < B_OK)
			break;
		
		BEntry	src(&pitem.fileRef);
		nDir.CreateFile(pitem.fileRef.name,&tmp);
		BEntry	dst(&nDir,pitem.fileRef.name);
		if (CopyFile(&dst,&src) >= B_OK) {
			tmp.SetTo(&dst,O_RDONLY);
			BNodeInfo	ninf(&tmp);
			if (ninf.InitCheck() >= B_OK)
				ninf.SetType(kIPkgSig);
		}
	}
	return B_NO_ERROR;
}
