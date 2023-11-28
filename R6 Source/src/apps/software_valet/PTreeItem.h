// PTreeItem.h

#include "CTreeItem.h"
#include "FSIcons.h"
#include "IArchiveItem.h"


class PTreeItem : public CTreeItem
{
public:
	PTreeItem(const char *name, BBitmap *icon)
		:	CTreeItem(name,icon)
	{
	};
};


class PTreeFolderItem : public PTreeItem
{
public:
	PTreeFolderItem(const char *_name)
		:	PTreeItem(_name,gGenericFolderSIcon)
	{
	};
};

class PTreeFileItem : public PTreeItem
{
public:
	PTreeFileItem(const char *_name)
		:	PTreeItem(_name,gGenericFileSIcon)
	{
	};
};
