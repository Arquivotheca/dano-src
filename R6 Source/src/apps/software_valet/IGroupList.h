// IGroupList.h
#ifndef _IGROUPLIST_H_
#define _IGROUPLIST_H_

#include "SimpleListView.h"
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
		helpText = NULL;
		hLength = 0;
	}
	virtual ~GroupItem() {
		if (name != NULL)
			delete[] name;
		if (description != NULL)
			delete[] description;
		if (helpText)
			delete[] helpText;
	}
	char	*name;  			// null if separator??
	char	*description;
	long	dLength;
	
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
	long		ViewIndexFor(long index);
	void		DeSelectAll();
	
	BLocker lock;
	// unordered group list
	RList<GroupItem *>		*masterList;
	
	// master indices to view indices
	RList<long>				*reverseList;
	
	// ordered group list with separators, master indices, selection
	RList<IndexItem *>		*viewList;
};

/**********************
	Error messages
**********************/

#define errMAXGRPS		"Sorry, no more groups can be added. There is a limit of 32 groups per package."

#endif
