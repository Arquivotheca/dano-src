/***************************************************************************
//
//	File:			EntryList.h
//
//	Description:	BEntryList class and entry_ref struct
//
//	Copyright 1992-98, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _STORAGE2_ENTRYLIST_H
#define _STORAGE2_ENTRYLIST_H

#include <dirent.h>
#include <limits.h>
#include <support2/SupportDefs.h>
#include <storage2/StorageDefs.h>

namespace B {
namespace Storage2 {

class BEntryList
{
public:

						BEntryList();
#if !_PR3_COMPATIBLE_
virtual					~BEntryList();
#else
						~BEntryList();
#endif

virtual status_t		GetNextEntry(BEntry *entry, 
									 bool traverse=false) = 0;
virtual status_t		GetNextRef(entry_ref *ref) = 0;
virtual int32			GetNextDirents(struct dirent *buf, 
						   		size_t length, int32 count = INT_MAX) = 0;

virtual status_t		Rewind() = 0;
virtual int32			CountEntries() = 0;

private:

#if !_PR3_COMPATIBLE_
virtual	void			_ReservedEntryList1();
virtual	void			_ReservedEntryList2();
virtual	void			_ReservedEntryList3();
virtual	void			_ReservedEntryList4();
virtual	void			_ReservedEntryList5();
virtual	void			_ReservedEntryList6();
virtual	void			_ReservedEntryList7();
virtual	void			_ReservedEntryList8();
#endif

};

} } // namespace B::Storage2

#endif	// _STORAGE2_ENTRYLIST_H
