/******************************************************************************
/
/	File:			List.h
/
/	Description:	BList class provides storage for pointers.
/					Not thread safe.
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/

#ifndef	_SUPPORT2_LIST_H
#define	_SUPPORT2_LIST_H

#include <SupportDefs.h>

namespace B {
namespace Support2 {

/*--------------------------------------------------------*/
/*----- BList class --------------------------------------*/

class BList {

public:
				BList(int32 initialAllocSize = 0);
				BList(const BList&);
virtual			~BList();

		BList	&operator=(const BList &from);

/* Adding and removing items. */
		bool	AddItem(void *item);
		bool	AddItem(void *item, int32 atIndex);
		bool	AddList(BList *newItems);
		bool	AddList(BList *newItems, int32 atIndex);
		bool	RemoveItem(void *item);
		void	*RemoveItem(int32 index);
		bool	RemoveItems(int32 index, int32 count);
		bool	RemoveItems(int32 index, int32 count,
							bool (*func)(void *, void *), void *);
		bool	ReplaceItem(int32 index, void *newItem);
		void	MakeEmpty();

/* Reordering items. */
		void	SortItems(int (*cmp)(const void *, const void *));
		bool	SwapItems(int32 indexA, int32 indexB);
		bool	MoveItem(int32 fromIndex, int32 toIndex);

/* Retrieving items. */
		void	*ItemAt(int32) const;
		void	*ItemAtFast(int32) const;
		void	*FirstItem() const;
		void	*LastItem() const;
		void	**Items() const;

/* Querying the list. */
		bool	HasItem(void *item) const;
		int32	IndexOf(void *item) const;
		int32	CountItems() const;
		bool	IsEmpty() const;

/* Iterating over the list. */
		void	DoForEach(bool (*func)(void *));
		void	DoForEach(bool (*func)(void *, void *), void *);

/*----- Private or reserved ---------------*/
private:

virtual	void			_ReservedList1();
virtual	void			_ReservedList2();

		void	Alloc(int32 initialAllocSize);
		void	Cleanup();
		void	Grow(int32 neededCount);
		void	Shrink(int32 neededCount);
		
		void**	fObjectList;
		size_t	fPhysicalSize;
		int32	fItemCount;
		void*	fSingleItem;		// was fBlockSize
		int32	fInitialSize;		// was _reserved[0]
		uint32	_reserved[1];		// was 2
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} } // namespace B::Support2

using namespace B::Support2;

#endif /* _SUPPORT2_LIST_H */
