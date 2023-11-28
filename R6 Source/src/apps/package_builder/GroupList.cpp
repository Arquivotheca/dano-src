#include <Be.h>
// GroupList.cpp

#include "GroupList.h"
#include "RList.h"
#include "FListView.h"
#include "Util.h"
#include "StLocker.h"

#include "MyDebug.h"


GroupList::GroupList()
{
	StBLock lk(&lock);
	
	masterList = new RList<GroupItem *>;
	
	reverseList = new RList<long>;
	
	viewList = new RList<IndexItem *>;
}

GroupList::~GroupList()
{
	StBLock lk(&lock);
		
	PRINT(("doing group list destructor\n"));
	
	for(long i = masterList->CountItems()-1; i >= 0; i--)
		delete masterList->ItemAt(i);
	delete masterList;
	
	for(long i = viewList->CountItems()-1; i >= 0; i--)
		delete viewList->ItemAt(i);
	delete viewList;
	
	delete reverseList;
}

void GroupList::DeSelectAll()
{	
	for(long i = viewList->CountItems()-1; i >= 0; i--) {
		viewList->ItemAt(i)->selected = FALSE;
	}
}

void GroupList::AddGroup(const char *name)
{
	StBLock lk(&lock);
	if (masterList->CountItems() >= 32) {
		doError("Sorry, no more groups can be added\n");
		return;
	}
	GroupItem *newGroupItem = new GroupItem();
	
	newGroupItem->name = strdup(name);
	masterList->AddItem(newGroupItem);
	
	IndexItem *item = new IndexItem;
	item->index = masterList->CountItems() - 1;
	viewList->AddItem(item);

	reverseList->AddItem(viewList->CountItems()-1);
}


// THIs is really ugly, should use pointers instead of
// these stupid index numbers!
long GroupList::RemoveGroup(long viewIndex)
{
	StBLock lk(&lock);
	
	long masterIndex = viewList->ItemAt(viewIndex)->index;
	PRINT(("view index is %d, master index is %d\n",viewIndex,masterIndex));
	
	delete viewList->ItemAt(viewIndex);
	viewList->RemoveIndex(viewIndex);
	
	long count = viewList->CountItems();
	// fix the reverse links since viewList has been shortened
	// this checks out ok,
	PRINT(("fixing reverse links\n"));
	for(long i = viewIndex; i < count; i++) {
		long forwardIndex = viewList->ItemAt(i)->index;
		if (forwardIndex != -1)
			reverseList->Items()[forwardIndex] -= 1;
	}
	PRINT(("revers links fixed\n"));
	// if this was not a separator item then
	// forward links need to be fixed
	if (masterIndex != -1) {
		PRINT(("item was not a separator\n"));
		// also must do master stuff
		delete masterList->ItemAt(masterIndex);
		masterList->RemoveIndex(masterIndex);
		reverseList->RemoveIndex(masterIndex);
		
		// update the forward links since master list is shortened
		long count = reverseList->CountItems();
		PRINT(("STARTING index in reverse list is %d, ENDING is %d\n",
					masterIndex,count-1));
		for (long i = masterIndex; i < count; i++) {
			// ok since back links have been fixed
			long reverseIndex = reverseList->ItemAt(i);
			// this does the forward link
			viewList->ItemAt(reverseIndex)->index = i;
		}
	}

#if DEBUG
	printf("view list says: \n");
	count =  viewList->CountItems();
	for (int i = 0; i < count; i++) {
		int mi = viewList->ItemAt(i)->index;
		printf("item %s ",(mi < 0) ? "separator" : masterList->ItemAt(mi)->name);
		if (mi >= 0) {
			int ri = reverseList->ItemAt(mi);
			if (ri == i) {
				printf("reverse match\n");
			}
			else
				printf("no match!\n");
		}
		else
			printf("\n");
	}
#endif

	return masterIndex;
}

// eventually do add separator at index
void GroupList::AddSeparator()
{
	StBLock lk(&lock);
	
	IndexItem *item = new IndexItem;
	item->index = -1;
	viewList->AddItem(item);
}


// these are view indices
void GroupList::ReorderItem(long oldIndex, long newIndex)
{
	StBLock	lk(&lock);	
	/// THis is a bit HACKY, but it works pretty darn well!
	
	long ind;
	PRINT(("old index is %d, new index is %d\n",oldIndex,newIndex));
	
	if (newIndex < oldIndex) {
		newIndex++;
		for (long i = newIndex; i < oldIndex; i++) {
			ind = viewList->ItemAt(i)->index;
			if (ind != -1)
				(reverseList->Items())[ind] += 1;
		}
		ind = viewList->ItemAt(oldIndex)->index;
		if (ind != -1)
			reverseList->SetItem(ind,newIndex);
	}
	else if (oldIndex < newIndex) {
		for (long i = oldIndex+1; i <= newIndex; i++) {
			ind = viewList->ItemAt(i)->index;
			if (ind != -1)
				(reverseList->Items())[ind] -= 1;
		}
		ind = viewList->ItemAt(oldIndex)->index;
		if (ind != -1)
			reverseList->SetItem(ind,newIndex);
	}
}

void GroupList::SetDescription(long index,const char *text)
{
	StBLock lk(&lock);
	
	long realIndex = viewList->ItemAt(index)->index;
	
	if (realIndex == -1) {
		return;
	}
	GroupItem *item = masterList->ItemAt(realIndex);
	if (item->description) free(item->description);
	
	item->description = strdup(text);
	item->dLength = strlen(text);
}

void GroupList::SetName(long index, const char *newName)
{
	StBLock lk(&lock);
	long realIndex = viewList->ItemAt(index)->index;
	
	if (realIndex == -1) {
		return;
	}
	
	GroupItem *gi = masterList->ItemAt(realIndex);

	free(gi->name);	
	gi->name = strdup(newName);
}

long GroupList::ViewIndexFor(long index)
{
	return reverseList->ItemAt(index);
}

void GroupList::SetHelp(long index, const char *hText)
{
	StBLock	lk(&lock);
	
	long realIndex = viewList->ItemAt(index)->index;
	if (realIndex == -1) {
		return;
	}
	GroupItem *item = masterList->ItemAt(realIndex);
	
	if (item->helpText) free(item->helpText);

	if (hText) {
		item->doHelp = TRUE;
		long len = strlen(hText);
		item->helpText = (char *)malloc(len+1);
		strcpy(item->helpText,hText);
		item->hLength = len;
	}
	else {
		item->doHelp = FALSE;
		item->helpText = NULL;
		item->hLength = 0;
	}
}
