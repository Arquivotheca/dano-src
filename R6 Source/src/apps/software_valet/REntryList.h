#ifndef _RENTRYLIST_H_
#define _RENTRYLIST_H_

#include <EntryList.h>
#include <Entry.h>
#include <Query.h>
#include "RList.h"

class REntryList : public BEntryList
{
public:
						REntryList();
						REntryList(BEntryList &q);
		virtual			~REntryList();
												
virtual status_t		GetNextEntry(BEntry *entry, bool=true);
virtual status_t		GetNextRef(entry_ref *ref);
virtual int32			GetNextDirents(struct dirent *buf, 
						   		size_t length, int32 count = INT_MAX);

virtual status_t		Rewind();
virtual int32			CountEntries();
		void			AddRef(const entry_ref *ref);

		void			RemoveCurrent();
		void			QueryAnd(REntryList *q);
		void			SetTo(BQuery *q);
private:
	int32	index;
	RList<entry_ref *>	*list;
};

#endif

