#ifndef _SIMPLELISTITEM_H_
#define _SIMPLELISTITEM_H_

// SimpleListItem.h


class ListItem
{
public:
	ListItem() :
		selected(false /*FALSE*/) {};
	virtual ~ListItem() {};
	bool selected;
};

#endif
