#ifndef _T_BOOKMARK_FILE_H
#define _T_BOOKMARK_FILE_H

#ifndef _MESSENGER_H
#include <app/Messenger.h>
#endif

#ifndef _LIST_H
#include <support/List.h>
#endif

#ifndef _SUPPORT_DEFS_H
#include <support/SupportDefs.h>
#endif

#ifndef _ENTRY_H
#include <Entry.h>
#endif

#include <support/String.h>

enum {
	T_BOOKMARK_CHANGED			= 'Tbmc'
	/** Fields:
	*** (pointer) "bookmark" TBookmarkItem that changed.
	*** (int32) "changes" T_BOOKMARK_*_CHANGED flags.
	**/
};

enum {
	T_BOOKMARK_NAME_CHANGED			= 1<<0,
	T_BOOKMARK_URL_CHANGED			= 1<<1,
	T_BOOKMARK_DELETABLE_CHANGED	= 1<<2,
	T_BOOKMARK_SECURE_CHANGED		= 1<<3,
	T_BOOKMARK_CHILDREN_CHANGED		= 1<<4,
	
	T_BOOKMARK_ALL_CHANGES		= 0xffffffff
};

class TBookmarkWatcher;

class TBookmarkItem
{
public:
	TBookmarkItem(const char* name, const char* url);
	TBookmarkItem(const char* folder_name);
	virtual ~TBookmarkItem();

	virtual void NoteChange(uint32 what_changed, TBookmarkItem* who_changed,
							BMessenger source = BMessenger());
	
	virtual int32 CompareItems(const TBookmarkItem* i1,
							   const TBookmarkItem* i2) const;
	
	bool StartWatching(BMessenger target,
					   uint32 this_changes = T_BOOKMARK_ALL_CHANGES,
					   uint32 child_changes = T_BOOKMARK_NAME_CHANGED);
	bool StopWatching(BMessenger target);
	
	const char* MakeUniqueName(BString* inout_name) const;
	
	void SetName(const char* name);
	const char* Name() const;
	
	void SetURL(const char* url);
	const char* URL() const;
	
	void SetIsDeletable(bool state);
	bool IsDeletable() const;
	
	void SetSecure(bool state);
	bool IsSecure() const;
	
	void SetIsFolder(bool state);
	bool IsFolder() const;
	
	const TBookmarkItem* Parent() const;
	TBookmarkItem* Parent();
	
	// BList-like interface to child bookmarks.
	bool			AddItem(TBookmarkItem* item,
							BMessenger source = BMessenger());
	bool			AddItem(TBookmarkItem* item, int32 atIndex,
							BMessenger source = BMessenger());
	bool			RemoveItem(TBookmarkItem* item,
							   BMessenger source = BMessenger());
	TBookmarkItem*	RemoveItem(int32 index,
							   BMessenger source = BMessenger());
	void			MakeEmpty(BMessenger source = BMessenger());
	void			SortItems(int (*cmp)(const TBookmarkItem*,
										 const TBookmarkItem*),
							  BMessenger source = BMessenger());
	bool			SwapItems(int32 indexA, int32 indexB,
							  BMessenger source = BMessenger());
	bool			MoveItem(int32 fromIndex, int32 toIndex,
							 BMessenger source = BMessenger());
	TBookmarkItem*	ItemAt(int32) const;
	TBookmarkItem*	FirstItem() const;
	TBookmarkItem*	LastItem() const;
	int32			IndexOf(TBookmarkItem* item) const;
	int32			CountItems() const;
	bool			IsEmpty() const;
	
	// Batch changes.
	bool			AddItems(const BList& items,
							 BMessenger source = BMessenger());
	bool			AddItems(const BList& items, int32 atIndex,
							 BMessenger source = BMessenger());
	bool			RemoveIndices(const BList& indices,
								BMessenger source = BMessenger(),
								BList* out_items = 0);
	bool			RemoveItems(const BList& items,
								BMessenger source = BMessenger());
	
	// Return the location an item was added.  Note that BOTH AddItem()
	// and AddSortedItem() add the item in sorted order -- this one just
	// has a different return value.
	int32			AddSortedItem(TBookmarkItem* item,
									BMessenger source = BMessenger());

	// Sort with the standard ordering.
	void			SortItems();
	
private:
	// Move the item at the given index, if its sorted position has
	// changed.  Calls NoteChange() before performing a move.  Returns
	// true if a move occurred.
	bool MoveItemIfNeeded(int32 fromIndex);
	bool MoveSelfIfNeeded();
	
	// returns index == CountItems() to place at end.
	int32 find_sorted_position(TBookmarkItem* item,
							   int32 lower=0, int32 upper=-1);
	int32 add_sorted_item(TBookmarkItem* item);
	bool resort_item(int32 fromIndex, int32 lower=0, int32 upper=-1);
	
	enum find_op {
		FIND_IF_EXISTS = 0,
		FIND_OR_ADD = 1,
		FIND_AND_REMOVE = 2
	};
	TBookmarkWatcher* find_watcher(BMessenger who, find_op op = FIND_IF_EXISTS);
	
	BString fName;
	BString fURL;
	bool fDeletable;
	bool fSecure;
	bool fFolder;
	
	TBookmarkItem* fOwner;
	BList fItems;			// Contains TBookmarkItem objects
	BList fWatchers;		// Containers TBookmarkWatcher objects
};

class TBookmarkFile : private TBookmarkItem
{
public:
	TBookmarkFile();
	virtual ~TBookmarkFile();
	
	status_t SetTo(const entry_ref* ref, uint32 open_mode = B_CREATE_FILE);
	status_t Write();
	
	const TBookmarkItem* Root() const;
	TBookmarkItem* Root();
	
	virtual void NoteChange(uint32 what_changed, TBookmarkItem* who_changed,
							BMessenger source = BMessenger());
	
	void MakeDirty(bool state = true);
	bool IsDirty();
	
private:
	entry_ref fRef;
	bool fDirty;
};

#endif
