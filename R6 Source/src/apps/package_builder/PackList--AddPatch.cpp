#include <Be.h>
#include "PackApplication.h"
#include "PackWindow.h"

#include "PackMessages.h"
#include "PackArc.h"
#include "PackListView.h"
#include "FArchiveItem.h"
#include "ArchivePatchItem.h"
#include "ExtractUtils.h"

#include "PatchThread.h"
#include "Util.h"

#include "MyDebug.h"


void PackList::AddPatch(BMessage *msg)
{
	PRINT(("PackList::AddPatch\n"));
	// doError("Patching not enabled yet in DR9");

	PackWindow *wind = (PackWindow *)Window();
	entry_ref	oldRef;
	entry_ref	newRef;
	BEntry 		oldEntry;
	BEntry		newEntry;

	
	ArchivePatchItem	*arcItem;
	ArchiveFolderItem	*destFolder;
	bool immed = wind->autoCompress;
	
	msg->FindRef("oldref",&oldRef);
	msg->FindRef("newref",&newRef);
	
	// AddPatch Items only to the toplevel???
	// try it without

	oldEntry.SetTo(&oldRef);
	newEntry.SetTo(&newRef);
	
	destFolder = CurrentFolder();
	arcItem = new ArchivePatchItem(oldEntry,newEntry);
	
	arcItem->SetParent(destFolder);
	arcItem->destination = D_INSTALL_FOLDER;
	ArchiveFolderItem *cur = destFolder;
	while( cur != NULL) {
		cur->numCompressibleItems++;
		cur->uncompressedSize += arcItem->uncompressedSize;		
		cur->groups |= arcItem->groups;		
		cur = cur->GetParent();
	}
	destFolder->AddEntry(arcItem);

	// thread will begin differencing

	BLooper *lp;
	if (immed) {
		lp = (BLooper *)wind->arcFile->fileLooper;
		PRINT(("auto compress TRUE\n"));	
	}
	else {
		PRINT(("auto compress FALSE\n"));	
		lp = NULL;
	}		
	BMessage *addMsg = immed ? new BMessage(M_ADD_PATCH) : NULL;
	if (addMsg) {
		addMsg->AddPointer("archive item",arcItem);
		
		addMsg->AddPointer("top folder",toplevel);
		addMsg->AddPointer("tree lock",&treeLock);
		addMsg->AddPointer("attributes",&(wind->attrib));
		wind->realUpdateNeeded = TRUE;
	}
	new PatchThread(oldRef,newRef,wind,addMsg,lp,arcItem,BMessenger(this));

	BEntry ofil(&oldRef);
	
	char mimestr[B_MIME_TYPE_LENGTH];

	GetEntrySignature(&ofil, mimestr);

	// doError("mimetype is: %s\n",mimestr);
	
	BMessage	mk(M_NEW_FINDITEM);
	mk.AddString("signature",mimestr);
	
	off_t fullSize;
	int32 smallSize;
	ofil.GetSize(&fullSize);
	smallSize = fullSize;
	mk.AddInt32("size",smallSize);

	char qName[B_FILE_NAME_LENGTH+8];
	sprintf(qName,"Find \"%s\"",oldRef.name);
	mk.AddString("text",qName);
	mk.AddBool("fromapp",TRUE);
	mk.AddPointer("arcitem",arcItem);
	
	Looper()->PostMessage(&mk,wind->FindView("Settings"));
	
	wind->attribDirty = TRUE;
	Update();
}

ArchivePatchItem *PackList::SelectedPatch()
{
	if (lowSelection == highSelection && lowSelection != -1) {
		ArchiveItem *ai = (ArchiveItem *)ItemAt(lowSelection);
		return cast_as(ai,ArchivePatchItem);
	}
	return NULL;
}

#if 0
void PackList::DoTestPatch(BMessage *msg)
{
	switch(msg->what) {
		case M_TEST_PATCH:
			ArchivePatchItem *arcItem = SelectedPatch();
			if (!arcItem)
				return;
				
			// the add refs message
			BMessage fPanelInfo(M_SEL_OLDPATCH);
			
			if (be_app->IsFilePanelRunning())
				be_app->CloseFilePanel();
			
			// set the window position (global) and other crap	
			// put in this window as the final destination for the message
			gPanelInfo.GetValues(fPanelInfo);
			gPanelInfo.SetOpenOwner(Window());
			
			fPanelInfo.AddMessenger("target",BMessenger(this));
			fPanelInfo.AddObject("archive item",arcItem);
			arcItem->canDelete = FALSE;
			if (be_app->RunFilePanel("PackageBuilder : Select original file to patch",
						"Add","Done",folders,fPanelInfo) == B_ERROR)
			{
				doError("Error opening the file panel.");
				gPanelInfo.SetOpenOwner(NULL);
			}
		case M_SEL_OLDPATCH:
		
			break;
		case M_SEL_NEWPATCH:
			break;
	}
}

#endif
