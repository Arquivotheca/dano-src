#include <Be.h>
// FArchiveItem.cpp

#include "FArchiveItem.h"
#include "DestinationList.h"

#include "Replace.h"

#include "Util.h"
#include "MyDebug.h"

ArchiveItem::ArchiveItem()
	: ListItem()
{
	Init();
}

ArchiveItem::ArchiveItem(BEntry &itemRef)
	: ListItem()
{	
	itemRef.GetRef(&fRef);
	Init();
}

void ArchiveItem::Init()
{
	smallIcon = NULL;
	compressedSize = 0;
	uncompressedSize = 0;
	name = NULL;
	
	dirty = TRUE;
	deleteThis = FALSE;
	groups = 1;
	customDest = FALSE;
	destination = D_PARENT_FOLDER;
	replacement = R_ASK_USER;
	platform = 0xFFFFFFFF;

	srcPath = 0;
	seekOffset = -1;
	
	memset(versionInfo,0,sizeof(versionInfo));
}

ArchiveItem::~ArchiveItem()
{
	free(name);
	free(srcPath);
}	

void	ArchiveItem::BuildCompressTree()
{
}

ulong	ArchiveItem::GetUnionGroups(ulong bits)
{
	return bits | groups;
}

void	ArchiveItem::SetGroups(ulong bits, bool mark)
{
	if (!mark)
		groups &= bits;
	else
		groups |= bits;
}


bool	ArchiveItem::IsFolder()
{
	return false;
}
