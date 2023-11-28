#include "REntryList.h"
#include "MyDebug.h"

REntryList::REntryList()
{
	index = -1;
	list = new RList<entry_ref *>;
}

REntryList::REntryList(BEntryList &q)
{
	index = -1;
	list = new RList<entry_ref *>;
	q.Rewind();
	
	entry_ref	*ref = new entry_ref;
	
	while(q.GetNextRef(ref) == B_NO_ERROR){
		list->AddItem(ref);
	}
	delete ref;
}

REntryList::~REntryList()
{
	for (int32 i = list->CountItems()-1; i >= 0; i--)
		delete list->ItemAt(i);
	delete list;
}

void		REntryList::AddRef(const entry_ref *ref)
{
	list->AddItem(new entry_ref(*ref));
}

status_t	REntryList::GetNextEntry(BEntry *entry, bool)
{
	index++;
	if (index >= list->CountItems()) {
		index--;
		return B_ERROR;
	}
	return entry->SetTo(list->ItemAt(index));
}

status_t	REntryList::GetNextRef(entry_ref *ref)
{
	index++;
	if (index >= list->CountItems()) {
		index--;
		return B_ERROR; 
	}
		
	*ref = *(list->ItemAt(index));
	return B_NO_ERROR;
}

void		REntryList::RemoveCurrent()
{
	if (index >= 0 && index < list->CountItems()) {
		delete list->RemoveIndex(index);
		index--;	
	}
}

status_t	REntryList::GetNextDirents(struct dirent *buf,
									size_t length,
									int32 count)
{
	buf, length, count;
	return B_NO_ERROR;
}

status_t	REntryList::Rewind()
{
	index = -1;
	return B_NO_ERROR;
}

int32		REntryList::CountEntries()
{
	return list->CountItems();
}

void	REntryList::SetTo(BQuery *q)
{
	list->MakeEmpty();
	entry_ref r;
	BEntry	entry;
	q->Rewind();
	while (q->GetNextEntry(&entry) == B_NO_ERROR) {
		entry.GetRef(&r);
		PRINT(("adding name: %s\n",r.name));
		AddRef(&r);
	}
//	q->Rewind();
}

void REntryList::QueryAnd(REntryList *q)
{
	Rewind();
	entry_ref	ourRef;
	entry_ref	qRef;
	
	bool found;
	
	while (GetNextRef(&ourRef) == B_NO_ERROR) {

		PRINT(("searching for %s\n",ourRef.name));
		// try to find in the query list
		
		status_t err;
		err = q->Rewind();
		found = FALSE;
		while(!found && (q->GetNextRef(&qRef) >= B_NO_ERROR)) {
			PRINT(("comparing %s\n",qRef.name));
			
			if (qRef == ourRef) {
				found = TRUE;
				break;
			}
		}
		if (found) {
			// keep it
			PRINT(("found\n"));
		}
		else {
			PRINT(("notfound\n"));
			RemoveCurrent();
		}
	}
}
