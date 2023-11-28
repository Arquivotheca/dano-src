// ArchiveScriptItem.h
#ifndef _IARCHIVESCRIPTITEM_H_
#define _IARCHIVESCRIPTITEM_H_

#include "IArchiveItem.h"
#include <Directory.h>


class ArchiveScriptItem : public ArchiveFileItem
{
public:
						ArchiveScriptItem();
	virtual 			~ArchiveScriptItem();
	
	virtual	long	ExtractItem(InstallPack *archiveFile, 
									BDirectory *parDir, 
									DestManager *dests, 
									ulong inGrps,
									bool skip);
};

#endif
