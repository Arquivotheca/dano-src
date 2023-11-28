
#include <Path.h>
#include <support/Debug.h>

#include "BookmarkFile.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

// --------------- URL Encoding/Decoding stolen from URL.cpp ---------------

static int HexToInt(const char *str)
{
	int val = 0;
	for (int i = 0; i < 2; i++) {
		unsigned char c = str[i];
		if (c >= '0' && c <= '9')
			val = (val << 4) | (c - '0');
		else if (c >= 'a' && c <= 'f')
			val = (val << 4) | (c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			val = (val << 4) | (c - 'A' + 10);
		else
			break;
	}

	return val;
}

static bool IsValidURLChar(char c)
{
	const char *kValidCharacters = ":@&=$-_.!*'(),/?~";
	return isalpha(c) || isdigit(c) || strchr(kValidCharacters, c) != 0;
}

static void UnescapeString(BString* buf)
{
	if (buf->Length() <= 0) return;
	
	char* out = buf->LockBuffer(buf->Length());
	const char* in = out;
	const char* start = in;
	while (*in) {	
		if (*in == '%') {
			in++;
			char c = HexToInt(in);
			if (c == '\0')
				c = ' ';
			
			// Kludge: If the title contains \n, \t, or \r, which Opera
			// may include, then replace with a space.
			if( (c == '\n') || (c == '\t') || (c == '\r') ) c = ' ';
			
			*out++ = c;
			if( *in ) in++;
			if( *in ) in++;
		} else if (*in == '+') {
			*out++ = ' ';
			in++;
		} else
			*out++ = *in++;
	}
	
	*out = '\0';
	buf->UnlockBuffer((size_t)(out-start));
}

static size_t GetEscapedLength(const char *c)
{
	if (!c || !*c) return 0;
	
	size_t len = 0;
	while (*c) {
		if (*c != ' ' && !IsValidURLChar(*c))
			len += 3;
		else
			len++;
		c++;
	}

	return len;
}

static const char* EscapeString(BString* outString, const char *in)
{
	size_t len = GetEscapedLength(in);
	if (len == 0) {
		*outString = "";
		return outString->String();
	}
	
	char* out = outString->LockBuffer(len);
	while( *in ) {
		if( *in == ' ' ) *out++ = '+';
		else if (!IsValidURLChar(*in)) {
			sprintf(out, "%%%02x", *(uchar*) in);
			out += 3;
		} else
			*out++ = *in;
		in++;
	}

	*out = '\0';
	outString->UnlockBuffer(len);
	return outString->String();
}


// --------------------------- TBookmarkWatcher ---------------------------

class TBookmarkWatcher
{
public:
	TBookmarkWatcher(BMessenger target,
					 uint32 object_changes, uint32 child_changes)
		: fTarget(target),
		  fObjectChanges(object_changes), fChildChanges(child_changes)
	{
	}
	
	~TBookmarkWatcher()
	{
	}

	bool IsValid() const							{ return fTarget.IsValid(); }
	
	void SetTarget(BMessenger messenger)			{ fTarget = messenger; }
	BMessenger Target() const						{ return fTarget; }
	
	void SetObjectChanges(uint32 object_changes)	{ fObjectChanges = object_changes; }
	uint32 ObjectChanges() const					{ return fObjectChanges; }
	
	void SetChildChanges(uint32 child_changes)		{ fChildChanges = child_changes; }
	uint32 ChildChanges() const						{ return fChildChanges; }

private:
	BMessenger fTarget;
	uint32 fObjectChanges;
	uint32 fChildChanges;
};

// --------------------------- TBookmarkItem ---------------------------

TBookmarkItem::TBookmarkItem(const char* name, const char* url)
	: fName(name), fURL(url), fDeletable(true), fSecure(false),
	  fFolder(false), fOwner(0)
{
}

TBookmarkItem::TBookmarkItem(const char* folder_name)
	: fName(folder_name), fDeletable(true), fSecure(false),
	  fFolder(true), fOwner(0)
{
}

static bool free_watcher_func(void* item)
{
	PRINT(("Deleting target %p\n", item));
	delete reinterpret_cast<TBookmarkWatcher*>(item);
	return false;
}

TBookmarkItem::~TBookmarkItem()
{
	fWatchers.DoForEach(free_watcher_func);
	fWatchers.MakeEmpty();
	MakeEmpty();
}

void TBookmarkItem::NoteChange(uint32 what_changed, TBookmarkItem* who_changed,
							   BMessenger source)
{
	if( this == who_changed || this == who_changed->Parent() ) {
		for( int32 i=0; i<fWatchers.CountItems(); i++ ) {
			TBookmarkWatcher* watcher = (TBookmarkWatcher*)fWatchers.ItemAt(i);
			if( watcher && watcher->IsValid() && watcher->Target() != source ) {
				uint32 look_for = this == who_changed
								? watcher->ObjectChanges()
								: watcher->ChildChanges();
				if( (look_for&what_changed) != 0 ) {
					BMessage msg(T_BOOKMARK_CHANGED);
					msg.AddPointer("bookmark", who_changed);
					msg.AddInt32("changes", what_changed);
					watcher->Target().SendMessage(&msg);
				}
			}
		}
	}
	
	if( Parent() ) Parent()->NoteChange(what_changed, who_changed);
}

int32 TBookmarkItem::CompareItems(const TBookmarkItem* i1,
								  const TBookmarkItem* i2) const
{
	if( i1->IsFolder() != i2->IsFolder() ) {
		if( i1->IsFolder() ) return -1;
		return 1;
	}
	
	int32 cmp = i1->fName.ICompare(i2->fName);
	if( cmp == 0 ) cmp = i1->fURL.ICompare(i2->fURL);
	
	if( cmp == 0 ) {
		if( i1 < i2 ) cmp = -1;
		else if( i1 > i2 ) cmp = 1;
	}
	
	return cmp;
}

bool TBookmarkItem::StartWatching(BMessenger target,
								  uint32 this_changes, uint32 child_changes)
{
	if( !target.IsValid() ) return false;
	
	TBookmarkWatcher* watcher = find_watcher(target, FIND_OR_ADD);
	if( watcher ) {
		watcher->SetObjectChanges(this_changes);
		watcher->SetChildChanges(child_changes);
		return true;
	}
	
	return false;
}

bool TBookmarkItem::StopWatching(BMessenger target)
{
	if( !target.IsValid() ) return false;
	TBookmarkWatcher* watcher = find_watcher(target, FIND_AND_REMOVE);
	delete watcher;
	return watcher != 0 ? true : false;
}

const char* TBookmarkItem::MakeUniqueName(BString* inout_name) const
{
	int32 num = 1;
	int32 baseLen = inout_name->Length();
	for( int32 i=0; i<CountItems(); i++ ) {
		TBookmarkItem* it = ItemAt(i);
		const char* itname = it ? it->Name() : 0;
		if( itname && inout_name->ICompare(itname, baseLen) == 0 ) {
			if( itname[baseLen] == 0 ) {
				if( 1 >= num ) num = 2;
			} else if( itname[baseLen] == ' ' ) {
				itname += baseLen + 1;
				if( *itname == 0 ) {
					if( 1 >= num ) num = 2;
				} else {
					const char* start = itname;
					while( *itname >= '0' && *itname <= '9' ) itname++;
					if( *itname == 0 ) {
						int32 val = atoi(start);
						if( val >= num ) num = val+1;
					}
				}
			}
		}
	}
	
	if( num >= 2 ) {
		*inout_name << " " << num;
	}
	
	return inout_name->String();
}

void TBookmarkItem::SetName(const char* name)
{
	if( fName != name ) {
		NoteChange(T_BOOKMARK_NAME_CHANGED, this);
		fName = name;
		MoveSelfIfNeeded();
	}
}

const char* TBookmarkItem::Name() const
{
	return fName.String();
}

void TBookmarkItem::SetURL(const char* url)
{
	if( fURL != url ) {
		NoteChange(T_BOOKMARK_URL_CHANGED, this);
		if( url && *url ) SetIsFolder(false);
		fURL = url;
		MoveSelfIfNeeded();
	}
}

const char* TBookmarkItem::URL() const
{
	return fURL.String();
}

void TBookmarkItem::SetIsDeletable(bool state)
{
	if( fDeletable != state ) {
		NoteChange(T_BOOKMARK_DELETABLE_CHANGED, this);
		fDeletable = state;
	}
}

bool TBookmarkItem::IsDeletable() const
{
	return fDeletable;
}

void TBookmarkItem::SetSecure(bool state)
{
	if( fSecure != state ) {
		NoteChange(T_BOOKMARK_SECURE_CHANGED, this);
		fSecure = state;
	}
}

bool TBookmarkItem::IsSecure() const
{
	return fSecure;
}

void TBookmarkItem::SetIsFolder(bool state)
{
	if( fFolder != state ) {
		fFolder = state;
		if (!fFolder) {
			MakeEmpty();
			MoveSelfIfNeeded();
		} else {
			if (fURL.Length() > 0) SetURL("");
			else MoveSelfIfNeeded();
		}
	}
}

bool TBookmarkItem::IsFolder() const
{
	return fFolder;
}

const TBookmarkItem* TBookmarkItem::Parent() const
{
	return fOwner;
}

TBookmarkItem* TBookmarkItem::Parent()
{
	return fOwner;
}

bool TBookmarkItem::AddItem(TBookmarkItem* item, BMessenger source)
{
	return AddSortedItem(item, source) >= 0 ? true : false;
}

bool TBookmarkItem::AddItem(TBookmarkItem* item, int32 atIndex, BMessenger source)
{
	if( item->Parent() != 0 ) debugger("bookmark already has a parent");
	
	NoteChange(T_BOOKMARK_CHILDREN_CHANGED, this, source);
	bool res = fItems.AddItem(item, atIndex);
	if( res ) {
		item->fOwner = this;
	} else {
		TRESPASS();
	}
	return res;
}

bool TBookmarkItem::RemoveItem(TBookmarkItem* item, BMessenger source)
{
	if( item->Parent() != this ) debugger("item is not a child of the parent it is being removed from");
	
	NoteChange(T_BOOKMARK_CHILDREN_CHANGED, this, source);
	bool res = fItems.RemoveItem(item);
	if( res ) {
		item->fOwner = 0;
	} else {
		TRESPASS();
	}
	return res;
}

TBookmarkItem* TBookmarkItem::RemoveItem(int32 index, BMessenger source)
{
	NoteChange(T_BOOKMARK_CHILDREN_CHANGED, this, source);
	TBookmarkItem* b = (TBookmarkItem*)fItems.RemoveItem(index);
	if( b ) {
		b->fOwner = 0;
	} else {
		TRESPASS();
	}
	return b;
}

static bool free_bookmark_func(void* item)
{
	delete reinterpret_cast<TBookmarkItem*>(item);
	return false;
}

void TBookmarkItem::MakeEmpty(BMessenger source)
{
	if( CountItems() > 0 ) {
		NoteChange(T_BOOKMARK_CHILDREN_CHANGED, this, source);
		fItems.DoForEach(free_bookmark_func);
		fItems.MakeEmpty();
	}
}

void TBookmarkItem::SortItems(int (*cmp)(const TBookmarkItem*,
										 const TBookmarkItem*),
							  BMessenger source)
{
	NoteChange(T_BOOKMARK_CHILDREN_CHANGED, this, source);
	fItems.SortItems((int (*)(const void*, const void*))cmp);
}

bool TBookmarkItem::SwapItems(int32 indexA, int32 indexB, BMessenger source)
{
	NoteChange(T_BOOKMARK_CHILDREN_CHANGED, this, source);
	bool result = fItems.SwapItems(indexA, indexB);
	return result;
}

bool TBookmarkItem::MoveItem(int32 fromIndex, int32 toIndex, BMessenger source)
{
	NoteChange(T_BOOKMARK_CHILDREN_CHANGED, this, source);
	bool result = fItems.MoveItem(fromIndex, toIndex);
	return result;
}

TBookmarkItem* TBookmarkItem::ItemAt(int32 index) const
{
	return (TBookmarkItem*)fItems.ItemAt(index);
}

TBookmarkItem* TBookmarkItem::FirstItem() const
{
	return (TBookmarkItem*)fItems.FirstItem();
}

TBookmarkItem* TBookmarkItem::LastItem() const
{
	return (TBookmarkItem*)fItems.LastItem();
}

int32 TBookmarkItem::IndexOf(TBookmarkItem* item) const
{
	return fItems.IndexOf(item);
}

int32 TBookmarkItem::CountItems() const
{
	return fItems.CountItems();
}

bool TBookmarkItem::IsEmpty() const
{
	return fItems.IsEmpty();
}

bool TBookmarkItem::AddItems(const BList& items, BMessenger source)
{
	NoteChange(T_BOOKMARK_CHILDREN_CHANGED, this, source);
	
	bool okay = true;
	for( int32 i=0; i<items.CountItems(); i++ ) {
		TBookmarkItem* item = (TBookmarkItem*)items.ItemAt(i);
		if( item ) {
			if( item->Parent() != 0 ) debugger("bookmark already has a parent");
			if( add_sorted_item(item) >= 0 ) {
				item->fOwner = this;
			} else {
				TRESPASS();
				okay = false;
			}
		}
	}
	
	return okay;
}

bool TBookmarkItem::AddItems(const BList& items, int32 atIndex, BMessenger source)
{
	NoteChange(T_BOOKMARK_CHILDREN_CHANGED, this, source);
	
	bool okay = true;
	for( int32 i=0; i<items.CountItems(); i++ ) {
		TBookmarkItem* item = (TBookmarkItem*)items.ItemAt(i);
		if( item ) {
			if( item->Parent() != 0 ) debugger("bookmark already has a parent");
			if( fItems.AddItem(item, atIndex) ) {
				item->fOwner = this;
			} else {
				TRESPASS();
				okay = false;
			}
		}
	}
	
	return okay;
}

bool TBookmarkItem::RemoveIndices(const BList& indices, BMessenger source,
								  BList* out_items)
{
	NoteChange(T_BOOKMARK_CHILDREN_CHANGED, this, source);
	
	bool okay = true;
	for( int32 i=0; i<indices.CountItems(); i++ ) {
		TBookmarkItem* item = (TBookmarkItem*)
			fItems.RemoveItem((int32)indices.ItemAt(i));
		if( item ) {
			item->fOwner = 0;
			if( out_items ) out_items->AddItem(item);
		} else {
			okay = false;
		}
	}
	
	return okay;
}

bool TBookmarkItem::RemoveItems(const BList& items, BMessenger source)
{
	NoteChange(T_BOOKMARK_CHILDREN_CHANGED, this, source);
	
	bool okay = true;
	for( int32 i=0; i<items.CountItems(); i++ ) {
		TBookmarkItem* item = (TBookmarkItem*)items.ItemAt(i);
		if( item ) {
			if( item->Parent() != this ) debugger("item is not a child of the parent it is being removed from");
			if( fItems.RemoveItem(item) ) {
				item->fOwner = 0;
			} else {
				okay = false;
			}
		}
	}
	
	return okay;
}

static int sort_compare_func(const TBookmarkItem* i1, const TBookmarkItem* i2)
{
	return i1->Parent()->CompareItems(i1, i2);
}

void TBookmarkItem::SortItems()
{
	SortItems(sort_compare_func);
}

int32 TBookmarkItem::AddSortedItem(TBookmarkItem* item, BMessenger source)
{
	if( item->Parent() != 0 ) debugger("bookmark already has a parent");
	
	NoteChange(T_BOOKMARK_CHILDREN_CHANGED, this, source);
	int32 pos = add_sorted_item(item);
	if( pos >= 0 ) {
		item->fOwner = this;
	} else {
		TRESPASS();
	}
	return pos;
}

bool TBookmarkItem::MoveItemIfNeeded(int32 fromIndex)
{
	TBookmarkItem* item = ItemAt(fromIndex);
	if( !item ) return false;
	
	TBookmarkItem* prev = fromIndex > 0 ? ItemAt(fromIndex-1) : 0;
	TBookmarkItem* next = fromIndex < CountItems()-1 ? ItemAt(fromIndex+1) : 0;
	
	if( (prev && CompareItems(prev, item) > 0) ) {
		NoteChange(T_BOOKMARK_CHILDREN_CHANGED, this);
		return resort_item(fromIndex, 0, fromIndex-1);
	} else if ( (next && CompareItems(item, next) > 0) ) {
		NoteChange(T_BOOKMARK_CHILDREN_CHANGED, this);
		return resort_item(fromIndex, fromIndex+1);
	}
	
	return false;
}

bool TBookmarkItem::MoveSelfIfNeeded()
{
	if( Parent() ) return Parent()->MoveItemIfNeeded(Parent()->IndexOf(this));
	return false;
}

int32 TBookmarkItem::find_sorted_position(TBookmarkItem* item,
										  int32 lower, int32 upper)
{
	if( !item ) {
		TRESPASS();
		return -1;
	}
	
	// Find general vicinity with binary search.
	if( upper < 0 ) upper = CountItems() - 1;
	
	int32 minIndex = lower;
	int32 maxIndex = upper;
	
	while( lower < upper ) {
		int32 middle = lower + (upper-lower+1)/2;
		int32 cmp = CompareItems(item, ItemAt(middle));
		if (cmp < 0) 
			upper = middle - 1;
		else if (cmp > 0) 
			lower = middle + 1;
		else 
			lower = upper = middle;
	}
	
	// At this point, 'upper' and 'lower' are at the last found item.
	// Arbitrarily use 'upper' and determine the final insertion
	// point -- either before or after this item.
	if (upper < minIndex) 
		upper = minIndex;
	else if (upper < (maxIndex+1) && CompareItems(item, ItemAt(upper)) > 0) 
		upper++;
	
	return upper > CountItems() ? CountItems() : upper;
}

int32 TBookmarkItem::add_sorted_item(TBookmarkItem* item)
{
	int32 pos = find_sorted_position(item);
	if( ! fItems.AddItem(item, pos) ) pos = -1;
	return pos;
}

bool TBookmarkItem::resort_item(int32 fromIndex, int32 lower, int32 upper)
{
	int32 toIndex = find_sorted_position(ItemAt(fromIndex), lower, upper);
	if( toIndex < 0 ) return false;
	if( toIndex == fromIndex ) return true;
	
	// If moving lower into list, end index must be bumped back to take
	// into account the items removal from earlier in the list.
	if( toIndex > fromIndex+1 ) toIndex--;
	
	PRINT(("Resorting: moving from index %ld to index %ld\n",
			fromIndex, toIndex));
	
	return fItems.MoveItem(fromIndex, toIndex);
}

TBookmarkWatcher* TBookmarkItem::find_watcher(BMessenger who, find_op op)
{
	TBookmarkWatcher* found = 0;
	
	int32 j=0;
	for( int32 i=0; i<fWatchers.CountItems(); i++ ) {
		TBookmarkWatcher* bw = (TBookmarkWatcher*)fWatchers.ItemAt(i);
		if( !bw || !bw->IsValid() ) {
			PRINT(("Deleting invalid target %p at %ld\n", bw, i));
			delete bw;
			continue;
		}
		
		if( !found && who == bw->Target() ) {
			PRINT(("Found target %p at %ld\n", bw, i));
			found = bw;
			if( op == FIND_AND_REMOVE ) continue;
		}
		
		if( j < i ) {
			PRINT(("Moving target %p from %ld to %ld\n", bw, i, j));
			fWatchers.ReplaceItem(j, bw);
		}
		
		j++;
	}
	
	if( j < fWatchers.CountItems() ) {
		PRINT(("Shortening list by removing %ld at %ld\n",
				fWatchers.CountItems()-j, j));
		fWatchers.RemoveItems(j, fWatchers.CountItems()-j);
	}
	
	if( !found && op == FIND_OR_ADD ) {
		found = new TBookmarkWatcher(who, 0, 0);
		PRINT(("Adding new target %p at end (%ld)\n",
				found, fWatchers.CountItems()));
		fWatchers.AddItem(found);
	}
	
	return found;
}

// --------------------------- TBookmarkFile ---------------------------

namespace TBookmarkPrivate {
	class ReadBuffer {
	public:
		ReadBuffer()	: fSize(1024), fUsed(0), fData((char*)malloc(fSize))
		{
		}
		~ReadBuffer()
		{
			free(fData);
		}
		
		char* Buffer() const			{ return fData; }
		size_t BufferLength() const		{ return fUsed; }
		
		bool GetS(FILE* file)
		{
			size_t last_off = 0;
			while( !feof(file) ) {
				fgets(fData+last_off, fSize-last_off, file);
				for( fUsed=last_off; fUsed<fSize; fUsed++ ) {
					if( fData[fUsed] == '\n' ) fData[fUsed] = 0;
					if( fData[fUsed] == 0 ) return feof(file) ? false : true;
				}
				fSize += 1024;
				fData = (char*)realloc(fData, fSize);
				last_off = fUsed;
			}
			return false;
		}
	
	private:
		size_t fSize;
		size_t fUsed;
		char* fData;
	};
};

using namespace TBookmarkPrivate;

TBookmarkFile::TBookmarkFile()
	: TBookmarkItem("Top"), fDirty(false)
{
}

TBookmarkFile::~TBookmarkFile()
{
}

static int32 read_old_bookmarks(TBookmarkItem* folder, int32 level,
								FILE* file, ReadBuffer& buffer,
								bool need_read = true)
{
	while( !need_read || !feof(file) ) {
		if( need_read ) {
			if( !buffer.GetS(file) ) return -1;
		}
		need_read = true;
		
		char* s = buffer.Buffer();
		int32 new_level = 0;
		while( *s == '\t' ) {
			s++;
			new_level++;
		}
		if( new_level < level ) return new_level;
		
		char* base = s;
		while( *s && *s != '\t' ) s++;
		BString name(base, (size_t)(s-base));
		UnescapeString(&name);
		
		if( *s != '\t' ) {
			// This line is a folder.
			TBookmarkItem* item = new TBookmarkItem(name.String());
			folder->AddItem(item);
			new_level = read_old_bookmarks(item, new_level+1, file, buffer);
			if( new_level < level ) return new_level;
			need_read = false;
			
		} else {
			// This line is an actual bookmark.
			while( *s == '\t' ) s++;
			base = s;
			while( *s && *s != '\t' ) s++;
			BString url(base, (size_t)(s-base));
			
			TBookmarkItem* item = new TBookmarkItem(name.String(), url.String());
			folder->AddItem(item);
		}
	}
	
	return -1;
}

static int32 read_bookmarks(TBookmarkItem* folder, int32 level,
							FILE* file, ReadBuffer& buffer,
							bool need_read = true)
{
	TBookmarkItem* current = NULL;
	
	while( !feof(file) ) {
		if( need_read ) {
			if( !buffer.GetS(file) ) return -1;
		}
		need_read = true;
		const char* s = buffer.Buffer();
		
		// Find indentation level.
		int32 new_level = 0;
		while( *s == '\t' ) {
			s++;
			new_level++;
		}
		
		// Skip empty lines.
		while (*s && isspace(*s)) s++;
		if (!*s) continue;
		
		// Move up/down hierarchy according to indentation.
		if( new_level < level ) return new_level;
		if( new_level > level ) {
			new_level = read_bookmarks(current ? current : folder,
									   new_level, file, buffer, false);
			need_read = false;
			continue;
		}
		
		// Find first space or '='
		const char* p = s;
		while (*p && *p != ' ' && *p != '=') p++;
		if (!*p) continue;
		
		if (*p == ' ') {
			// This is the start of a new item.
			BString name(p+1);
			UnescapeString(&name);
			if (strncmp(s, "Folder ", 7) == 0) {
				// This is a new folder item.
				current = new TBookmarkItem(name.String());
				folder->AddItem(current);
				
			} else if (strncmp(s, "Favorite ", 9) == 0) {
				// This is a new favorite item.
				current = new TBookmarkItem(name.String(), "Unknown URL");
				folder->AddItem(current);
				
			}
			
		} else if (current) {
			// This is an attribute of the last item.
			BString key(s, (int32)(p-s));
			s = p+1;
			while (isspace(*s)) s++;
			p = s;
			while (*p && *p != '\n' && *p != '\r') p++;
			BString value(s, (int32)(p-s));
			
			if (key.ICompare("URL") == 0) {
				current->SetURL(value.String());
			} else if (key.ICompare("NoDelete") == 0) {
				const int32 intValue = atol(value.String());
				
				bool boolValue = false;
				if (value.ICompare("true") == 0) boolValue = true;
				else if (intValue) boolValue = true;
			
				current->SetIsDeletable(!boolValue);
			} else if (key.ICompare("Secure") == 0) {
				const int32 intValue = atol(value.String());
				
				bool boolValue = false;
				if (value.ICompare("true") == 0) boolValue = true;
				else if (intValue) boolValue = true;
			
				current->SetSecure(boolValue);
			}
		}
	}
	
	return -1;
}

status_t TBookmarkFile::SetTo(const entry_ref* ref, uint32 open_mode)
{
	if( !ref ) return B_BAD_VALUE;
	
	MakeEmpty();
	fRef = *ref;
	
	BEntry e;
	status_t err = e.SetTo(&fRef);
	if( err != B_OK ) return err;
	
	BPath path;
	err = path.SetTo(&e);
	if( err != B_OK ) return err;
	
	FILE* f = fopen(path.Path(), "rb");
	if( !f && (open_mode&B_CREATE_FILE) != 0 ) {
		// Create if doesn't exit.
		f = fopen(path.Path(), "a+");
	}
	if( !f ) return B_ENTRY_NOT_FOUND;
	if( ferror(f) != B_OK ) return ferror(f);
	
	ReadBuffer buffer;
	
	// Read header.  If this fails, there is no header so we just
	// have any empty bookmark list.
	if (buffer.GetS(f)) {
		// Now read the items, selecting the format based on the
		// header.
		const char* s = buffer.Buffer();
		if (strncmp(s, "@Favorites", 10) != 0) {
			read_old_bookmarks(Root(), 0, f, buffer, false);
		} else {
			read_bookmarks(Root(), 0, f, buffer);
		}
	}
	
	err = ferror(f);
	
	MakeDirty(false);
	
	fclose(f);
	return err;
}

static const char* level_indent(int32 level)
{
	static const char space[] =
	"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	int32 off = sizeof(space) - level - 1;
	if( off < 0 ) off = 0;
	return &space[off];
}

static void write_bookmarks(TBookmarkItem* folder, int32 level,
							FILE* file)
{
	for( int32 i=0; ferror(file) == B_OK && i<folder->CountItems(); i++ ) {
		TBookmarkItem* it = folder->ItemAt(i);
		if( it ) {
			fputs(level_indent(level), file);
			BString buf;
			if( it->IsFolder() ) {
				fputs("Folder ", file);
				fputs(EscapeString(&buf, it->Name()), file);
				fputs("\n", file);
				write_bookmarks(it, level+1, file);
			} else {
				fputs("Favorite ", file);
				fputs(EscapeString(&buf, it->Name()), file);
				fputs("\n", file);
				fputs(level_indent(level), file);
				fputs("URL=", file);
				BString url(it->URL());
				url.RemoveSet("\n\r");
				fputs(url.String(), file);
				fputs("\n", file);
				if (!it->IsDeletable()) {
					fputs(level_indent(level), file);
					fputs("NoDelete=true\n", file);
				}
				if (it->IsSecure()) {
					fputs(level_indent(level), file);
					fputs("Secure=true\n", file);
				}
			}
		}
	}
}

status_t TBookmarkFile::Write()
{
	BEntry e;
	status_t err = e.SetTo(&fRef);
	if( err != B_OK ) return err;
	
	BPath path;
	err = path.SetTo(&e);
	if( err != B_OK ) return err;
	
	FILE* f = fopen(path.Path(), "wb");
	fputs("@Favorites 1\n", f);
	write_bookmarks(Root(), 0, f);
	err = ferror(f);
	
	if( err == B_OK ) MakeDirty(false);
	
	fclose(f);
	return err;
}

const TBookmarkItem* TBookmarkFile::Root() const
{
	return this;
}

TBookmarkItem* TBookmarkFile::Root()
{
	return this;
}

void TBookmarkFile::NoteChange(uint32 what_changed, TBookmarkItem* who_changed,
							   BMessenger source)
{
	TBookmarkItem::NoteChange(what_changed, who_changed, source);
	MakeDirty(true);
}

void TBookmarkFile::MakeDirty(bool state)
{
	fDirty = state;
}

bool TBookmarkFile::IsDirty()
{
	return fDirty;
}
