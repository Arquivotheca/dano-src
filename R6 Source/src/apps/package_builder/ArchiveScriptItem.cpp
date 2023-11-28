#include <Be.h>
// ArchiveScriptItem.cpp

#include "ArchiveScriptItem.h"
#include "GlobalIcons.h"


ArchiveScriptItem::ArchiveScriptItem()
	:	ArchiveFileItem()
{
	// unset small icon in the future
	smallIcon = gScriptIcon;
}


ArchiveScriptItem::ArchiveScriptItem( BEntry &f,
							struct stat *stBuf)
	:	ArchiveFileItem(f,stBuf)
{
	// unset small icon in the future
	smallIcon = gScriptIcon;
}

// don't want to have replacement options
ArchiveScriptItem::~ArchiveScriptItem()
{
	// prevent delete in parent
	smallIcon = NULL;
}
