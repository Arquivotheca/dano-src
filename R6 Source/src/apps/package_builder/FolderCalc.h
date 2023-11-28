// FolderCalc.h

#include "MThread.h"
#include "FArchiveItem.h"

#ifndef _FOLDERCALC_H
#define _FOLDERCALC_H

// deletes itself when the thread finishes
class FolderCalc : public MThread
{
public:
				FolderCalc(ArchiveFolderItem *fold,
						sem_id calcSem, sem_id groupSem, long *count,
						BMessenger windUpdate);

virtual long	Execute();

private:
	ArchiveFolderItem	*fFold;
	sem_id				fCalcSem;
	sem_id				fGroupSem;
	sem_id				fCountMutex;
	long				*fCount;
	BMessenger			windUpdateMess;
};

#endif
