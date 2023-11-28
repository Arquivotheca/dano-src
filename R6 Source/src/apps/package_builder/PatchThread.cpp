#include <Be.h>
// PatchThread.cpp

#include "PatchThread.h"
#include "FilePatcher.h"
#include "PackMessages.h"

#include "PackWindow.h"
#include "ArchivePatchItem.h"

#include "MyDebug.h"
#include "Util.h"


PatchThread::PatchThread(entry_ref &oldFile, entry_ref &newFile,
						PackWindow *_pw,
						BMessage *_looperMsg,
						BLooper *_fileLooper,
						ArchivePatchItem *_arcItem,
						BMessenger _viewMess)
	: MThread("patch thread",B_NORMAL_PRIORITY),
	patcher(NULL),
	wind(_pw),
	looperMsg(_looperMsg),
	fileLooper(_fileLooper),
	arcItem(_arcItem),
	viewMess(_viewMess)
{
	// don't like this because it defeats the "immediate delete check"
	atomic_add(&wind->msgCount,1);
	
	patcher = new FilePatcher(oldFile, newFile);
	Run();
}

PatchThread::~PatchThread()
{
	delete looperMsg;
}

long PatchThread::Execute()
{
	entry_ref	patcherFile;
	long 		err;
	
	if (!patcher)
		return B_ERROR;

	patcherFile = patcher->MakePatch();
	err = patcher->Error();
	delete patcher;	

	arcItem->tempFileRef = patcherFile;
	
	if (err < B_NO_ERROR) {	
		PRINT(("Patch engine error\n"));
		// error condition!
		
		// need to remove item (only 1 item)
		// no need rewrite catalog since it hasn't been
		// added to the archive
		BLocker *tLock = NULL;
		if (looperMsg) {
			looperMsg->FindPointer("tree lock",(void **)&tLock);
			if (tLock) tLock->Lock();
		}
		
		ArchiveFolderItem *pf = arcItem->GetParent();
		pf->entries->RemoveItem(arcItem);
		pf->UpdateRemovedEntry(arcItem);
		
		if (tLock) tLock->Unlock();
		
		delete arcItem;	

		viewMess.SendMessage(M_ITEMS_UPDATED);
		atomic_add(&wind->msgCount,-1);
	}
	else if (fileLooper && looperMsg) {
		PRINT(("posting looper message\n"));	
		fileLooper->PostMessage(looperMsg);
	}
	else {
		PRINT(("no file looper or message\n"));
		atomic_add(&wind->msgCount,-1);
	}
			
	return 0;
}
