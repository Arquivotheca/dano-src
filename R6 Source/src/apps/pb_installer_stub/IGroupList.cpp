// GroupList.cpp
#include "IGroupList.h"
#include "RList.h"
#include "Util.h"

#include "MyDebug.h"
#include <string.h>


GroupList::GroupList()
{
	lock.Lock();
	masterList = new RList<GroupItem *>;
	reverseList = new RList<long>;
	viewList = new RList<IndexItem *>;
	
	lock.Unlock();
}

GroupList::~GroupList()
{
	lock.Lock();
	
	PRINT(("doing group list destructor\n"));
	
	for (long i = masterList->CountItems()-1;i >= 0; i--)
		delete masterList->ItemAt(i);	
	delete masterList;
	
	for (long i = viewList->CountItems()-1; i >= 0; i--)
		delete viewList->ItemAt(i);
	delete viewList;
	
	delete reverseList;
	
	lock.Unlock();
}

void GroupList::DeSelectAll()
{	
	for(long i = viewList->CountItems()-1; i >= 0; i--) {
		viewList->ItemAt(i)->selected = FALSE;
	}
}

void GroupList::AddGroup(const char *name)
{
	lock.Lock();
	if (masterList->CountItems() >= 32) {
		doError(errMAXGRPS);
		lock.Unlock();
		return;
	}
	GroupItem *newGroupItem = new GroupItem();
	
	newGroupItem->name = new char[strlen(name)+1];
	strcpy(newGroupItem->name,name);
	masterList->AddItem(newGroupItem);
	
	IndexItem *item = new IndexItem;
	item->index = masterList->CountItems() - 1;
	viewList->AddItem(item);

	reverseList->AddItem(viewList->CountItems()-1);
	
	lock.Unlock();
}


// eventually do add separator at index
void GroupList::AddSeparator()
{
	lock.Lock();
	
	IndexItem *item = new IndexItem;
	item->index = -1;
	viewList->AddItem(item);
	
	lock.Unlock();
}

void GroupList::SetName(long index, const char *newName)
{
	lock.Lock();
	long realIndex = viewList->ItemAt(index)->index;
	
	if (realIndex == -1) {
		lock.Unlock();
		return;
	}
	
	GroupItem *gi = masterList->ItemAt(realIndex);
	delete gi->name;
	
	gi->name = new char[strlen(newName)+1];
	strcpy(gi->name,newName);
	
	lock.Unlock();
}

long GroupList::ViewIndexFor(long index)
{
	return reverseList->ItemAt(index);
}
