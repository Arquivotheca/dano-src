#include <Be.h>
// FolderCalc.cpp

#include "FolderCalc.h"
#include "PackMessages.h"

#include "MyDebug.h"
#include "Util.h"

FolderCalc::FolderCalc(ArchiveFolderItem *fold,
						sem_id calcSem,
						sem_id groupSem,
						long	*count,
						BMessenger windUpdate)
	: MThread("folder calc",B_NORMAL_PRIORITY),
	fFold(fold),
	fCalcSem(calcSem),
	fGroupSem(groupSem),
	fCount(count),
	windUpdateMess(windUpdate)
{
	Run();
}

long FolderCalc::Execute()
{
	long tCount;
	tCount = atomic_add(fCount,1);
	if (tCount == 0)
		acquire_sem(fGroupSem);
	
	PRINT(("folder calc %d starting\n",Thread()));
	if (fFold->dirty != TRUE) {
		doError("Archive item should be tagged as dirty");
		// just ignore clean items
	}
	else if (fFold->deleteThis != TRUE)
	{
		PRINT(("Folder Calc thread got folder item\n"));
		fFold->GatherEntries();
		
		// Gather information for the parents
		ArchiveFolderItem *cur = fFold->GetParent();
		while( cur != NULL) {
			cur->uncompressedSize += fFold->uncompressedSize;
			cur->numCompressibleItems += fFold->numCompressibleItems;
			cur = cur->GetParent();
		}
		fFold->canEnter = TRUE;
		BMessage updtMessage(M_ITEMS_UPDATED);
		updtMessage.AddPointer("item",fFold);
		windUpdateMess.SendMessage(&updtMessage);
	}

	tCount = atomic_add(fCount,-1);
	if (tCount == 1) {
		release_sem(fGroupSem);
	}
		
	release_sem(fCalcSem);
	
	PRINT(("folder calc thread %d\n",Thread()));
	return B_NO_ERROR;
}
