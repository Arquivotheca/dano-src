// ArchiveScriptItem.h

#include "FArchiveItem.h"

class ArchiveScriptItem : public ArchiveFileItem
{
public:
			ArchiveScriptItem();
			ArchiveScriptItem(BEntry &f, struct stat *stBuf = NULL);
virtual 	~ArchiveScriptItem();
	
	
};
