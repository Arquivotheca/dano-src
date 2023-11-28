//========================================================================
//	MList.h
//	Copyright 1995 - 97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	A templated list class.  Since BList only holds values of 4 bytes
//	this class also has that limitation.
//	BDS

#ifndef _MLIST_H
#define _MLIST_H

#include <List.h>
#include <string.h>

template <class T>
class MList : public BList {
public:
				MList(int32 itemsPerBlock = 20) 
					: BList(itemsPerBlock) {}

virtual			~MList() {}

//		MList	&operator=(const MList &from);
		bool	AddItem(T inItem);
		bool	AddItem(T inItem, int32 inAtIndex);
		bool	RemoveItem(T inItem);
//		T		RemoveItem(int32 inIndex, int32 ignore);
		T		RemoveItemAt(int32 inIndex);
		T		ItemAt(int32) const;
		T		ItemAtFast(int32) const;
		int32	IndexOf(T inItem) const;
		T		FirstItem() const;
		T		LastItem() const;
		bool	HasItem(T inItem) const;
		bool	GetFirstItem(T& outItem) const;
		bool	GetLastItem(T& outItem) const;
		bool	GetNthItem(T& outItem, int32 inIndex) const;
		void	MoveItem(int32 inFromPosition, int32 inToPosition);
		void	MoveItemTo(T inItem, int32 inToPosition);
//		void	MoveItem(T inItem, int32 inToPosition, int32 ignore);
		void	SwapItems(int32 inFromPosition, int32 inToPosition);
};

template <class T>
inline void		MList<T>::MoveItem(int32 inFromPosition, int32 inToPosition)
{
	if (inToPosition >= CountItems())
		inToPosition = CountItems() - 1;

	if (inFromPosition != inToPosition)
	{
		T*	items = (T*) Items();
		T	save = items[inFromPosition];
		
		if (inFromPosition < inToPosition)
			memmove(&items[inFromPosition], &items[inFromPosition + 1], (inToPosition - inFromPosition) * sizeof(void*));
		else
			memmove(&items[inToPosition + 1], &items[inToPosition], (inFromPosition - inToPosition) * sizeof(void*));
			
		items[inToPosition] = save;
	}
}

template <class T>
inline void		MList<T>::MoveItemTo(T inItem, int32 inToPosition)
{
	if (inToPosition >= CountItems())
		inToPosition = CountItems() - 1;

	int32	fromIndex = IndexOf(inItem);
	
	if (fromIndex >= 0 && fromIndex != inToPosition)
	{
		T*	items = (T*) Items();
		
		if (fromIndex < inToPosition)
			memmove(&items[fromIndex], &items[fromIndex + 1], (inToPosition - fromIndex) * sizeof(void*));
		else
			memmove(&items[inToPosition + 1], &items[inToPosition], (fromIndex - inToPosition) * sizeof(void*));
			
		items[inToPosition] = inItem;
	}
}

template <class T>
inline void		MList<T>::SwapItems(int32 inFromPosition, int32 inToPosition)
{
	T*	items = (T*) Items();
	T	save = items[inToPosition];
	items[inToPosition] = items[inFromPosition];
	items[inFromPosition] = save;
}

// Useful typedefs
typedef MList<char*>	StringList;


template <class T>
inline bool		MList<T>::AddItem(T inItem)
{
	return BList::AddItem((void*)inItem);
}

template <class T>
inline bool		MList<T>::AddItem(T inItem, int32 inAtIndex)
{
	return BList::AddItem((void*)inItem, inAtIndex);
}

template <class T>
inline bool		MList<T>::RemoveItem(T inItem)
{
	return BList::RemoveItem((void*)inItem);
}
template <class T>
inline T		MList<T>::RemoveItemAt(int32 inIndex)
{
	return (T) BList::RemoveItem(inIndex);
}

template <class T>
inline bool		MList<T>::GetNthItem(T &outItem, int32 inIndex) const
{
	if (inIndex < 0 || inIndex >= BList::CountItems())
		return false;
	else
	{
		outItem = (T) BList::ItemAtFast(inIndex);
		return true;
	}
}

template <class T>
inline int32		MList<T>::IndexOf(T inItem) const
{
	return BList::IndexOf((void*) inItem);
}

template <class T>
inline bool		MList<T>::GetFirstItem(T &outItem) const
{
	if (BList::CountItems() > 0)
	{
		outItem = (T) BList::FirstItem();
		return true;
	}
	else
		return false;
}

template <class T>
inline bool		MList<T>::GetLastItem(T &outItem) const
{
	if (BList::CountItems() > 0)
	{
		outItem = (T) BList::LastItem();
		return true;
	}
	else
		return false;	
}

template <class T>
inline T 		MList<T>::FirstItem() const
{
	return (T) BList::FirstItem();
}

template <class T>
inline T 		MList<T>::LastItem() const
{
	return (T) BList::LastItem();
}

template <class T>
inline bool		MList<T>::HasItem(T inItem) const
{
	return BList::HasItem((void*) inItem);
}

template <class T>
inline T 		MList<T>::ItemAt(int32 index) const
{
	return (T) BList::ItemAt(index);
}

template <class T>
inline T 		MList<T>::ItemAtFast(int32 index) const
{
	return (T) BList::ItemAtFast(index);
}

#endif
