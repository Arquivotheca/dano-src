// GroupList.h
#include "FListView.h"
#include <stdlib.h>

#ifndef _GROUPLIST_H
#define _GROUPLIST_H

#include <Locker.h>

const long maxGroups = 32;

class IndexItem : public ListItem
{
public:
	long index;
};

class GroupItem
{
public:
	GroupItem() {
		name = NULL;
		description = NULL;
		dLength = 0;
		doHelp = FALSE;
		helpText = NULL;
		hLength = 0;
	}
	~GroupItem() {
		if (name) free(name);
		if (description) free(description);
		if (helpText) free(helpText);
	}
	char	*name;  			// null if separator??
	char	*description;
	long	dLength;
	
	bool	doHelp;
	char	*helpText;
	long	hLength;
	
	// currently unused
	void	*iconBits;
	long	groupBytes;
};


class GroupList
{
public:
	GroupList();
	~GroupList();
	
	void 		AddGroup(const char *nm);
	void 		AddSeparator();
	void 		SetName(long index, const char *nm);
	void		SetName(IndexItem *ii, const char *newName);
	long		ViewIndexFor(long index);
	void		ReorderItem(long oldI, long newI);
	long		RemoveGroup(long index);
	void		DeSelectAll();
	void		SetDescription(long index,const char *text);
	void		SetHelp(long index, const char *text);
	
	BLocker lock;
	// unordered group list
	RList<GroupItem *>		*masterList;
	
	// master indices to view indices
	RList<long>				*reverseList;
	
	// ordered group list with separators, master indices, selection
	RList<IndexItem *>		*viewList;
};

#endif
