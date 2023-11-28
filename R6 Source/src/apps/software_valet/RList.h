#ifndef R_LIST_H
#define R_LIST_H

#include <List.h>

// make sure T is just a pointer type
template<class T>
class RList : public BList
{
//should the underlying list be private?

public:
	RList(long itemsPerBlock = 20)
		:  BList(itemsPerBlock) {}
		
	RList(const BList &list)
		:  BList(list) {}
	
	RList& operator=(const RList &from) {
		return (RList<T> &)(BList::operator=((BList)from));
	}
	bool	AddItem(T item) {
		return BList::AddItem((void *)item);
	}
	bool	AddItem(T item, long atIndex) {
		return BList::AddItem((void *)item, atIndex);
	}
	bool	AddList(RList *newItems) {
		return BList::AddList((BList *)newItems);
	}
	bool	AddList(RList *newItems, long atIndex) {
		return BList::AddList((BList *)newItems, atIndex);
	}
	bool	RemoveItem(T item) {
		return BList::RemoveItem((void *)item);
	}
	// notice this must be different than RemoveItem to
	// prevent ambiguity of types
	T		RemoveIndex(long index) {
		return (T)(BList::RemoveItem(index));
	}
	
	void	SetItem(long index, T value) {
		if (index >= 0 && index < CountItems())
			Items()[index] = value;
	}
		
	T		ItemAt(long index) const {
		return (T)(BList::ItemAt(index));
	}
	T		ItemAtFast(long i) const {
		return (T)(BList::ItemAtFast(i));
	}
	long	IndexOf(T item) const {
		return BList::IndexOf((void *)item);
	}
	T		FirstItem() const {
		return (T)BList::FirstItem();
	}
	T		LastItem() const {
		return (T)BList::LastItem();
	}
	bool	HasItem(T item) const {
		return BList::HasItem((void *)item);
	}
	long	CountItems() const {
		return BList::CountItems();
	}
	void	MakeEmpty() {
		BList::MakeEmpty();
	}
	bool	IsEmpty() const {
		return BList::IsEmpty();
	}
	void	DoForEach(bool (*func)(T)) {
		BList::DoForEach((bool (*)(void *))func);
	}
	void	DoForEach(bool (*func)(T,void *), void *data) {
		BList::DoForEach((bool (*)(void *,void *))func,data);
	}
	// this is a dangerous one
	T*		Items() const {
		return (T *)(BList::Items());
	}
	void	SortItems(int (*cmp)(const T*,const T*)) {
		BList::SortItems((int (*)(const void *,const void *))cmp);
	}
};

#endif
