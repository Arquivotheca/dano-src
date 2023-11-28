#ifndef __OBJECT_LIST__
#define __OBJECT_LIST__

#ifndef _BE_H
#include <List.h>
#endif

#include <Debug.h>

class _PointerList_ : public BList {
public:
	_PointerList_(const _PointerList_ &list);
	_PointerList_(int32 itemsPerBlock = 20, bool owning = false);

	typedef void *(* GenericEachFunction)(void *, void *);
	typedef int (* GenericCompareFunction)(const void *, const void *);
	typedef int (* GenericCompareFunctionWithState)(const void *, const void *,
		void *);
	void *EachElement(GenericEachFunction, void *);
	void SortItems(GenericCompareFunction);
	void SortItems(GenericCompareFunctionWithState, void *state);
	void HSortItems(GenericCompareFunction);
	void HSortItems(GenericCompareFunctionWithState, void *state);
	
	void *BinarySearch(const void *, GenericCompareFunction) const;
	void *BinarySearch(const void *, GenericCompareFunctionWithState, void *state) const;

	bool Owning() const;
	bool ReplaceItem(int32, void *);

protected:
	bool owning;
};

template<class T>
class BObjectList : private _PointerList_ {
public:

	// iteration and sorting
	typedef T *(* EachFunction)(T *, void *);
	typedef const T *(* ConstEachFunction)(const T *, void *);
	typedef int (* CompareFunction)(const T *, const T *);
	typedef int (* CompareFunctionWithState)(const T *, const T *, void *state);

	BObjectList(int32 itemsPerBlock = 20, bool owning = false);
	BObjectList(const BObjectList &list);
		// clones list; if list is owning, makes copies of all
		// the items

	virtual ~BObjectList();

	BObjectList &operator=(const BObjectList &list);
		// clones list; if list is owning, makes copies of all
		// the items
	
	// adding and removing
	// ToDo:
	// change Add calls to return const item 
	bool AddItem(T *);
	bool AddItem(T *, int32);
	bool AddList(BObjectList *);
	bool AddList(BObjectList *, int32);
	
	bool RemoveItem(T *, bool deleteIfOwning = true);
		// if owning, deletes the removed item
	T *RemoveItemAt(int32);
		// returns the removed item
	
	void MakeEmpty();

	// item access
	T *ItemAt(int32) const;

	bool ReplaceItem(int32 index, T *);
		// if list is owning, deletes the item at <index> first
	T *SwapWithItem(int32 index, T *newItem);
		// same as ReplaceItem, except does not delete old item at <index>,
		// returns it instead
	
	T *FirstItem() const;
	T *LastItem() const;
	
	// misc. getters
	int32 IndexOf(const T *) const;
	bool HasItem(const T *) const;
	bool IsEmpty() const;
	int32 CountItems() const;


	T *EachElement(EachFunction, void *);
	const T *EachElement(ConstEachFunction, void *) const;
		// Do for each are obsoleted by this list, possibly add
		// them for convenience
	void SortItems(CompareFunction);
	void SortItems(CompareFunctionWithState, void *state);
	void HSortItems(CompareFunction);
	void HSortItems(CompareFunctionWithState, void *state);

	// list must be sorted with CompareFunction for these to work
	const T *BinarySearch(const T &, CompareFunction) const;
	const T *BinarySearch(const T &, CompareFunctionWithState, void *state) const;
	
	// deprecated API, will go away
	BList *AsBList()
		{ return this; }
	const BList *AsBList() const
		{ return this; }
private:
	void SetItem(int32, T *);
};
	
template<class Item, class Result, class Param1>
Result 
WhileEachListItem(BObjectList<Item> *list, Result (Item::*func)(Param1), Param1 p1)
{
	Result result = 0; 
	int32 count = list->CountItems();
	
	for (int32 index = 0; index < count; index++)
		if ((result = (list->ItemAt(index)->*func)(p1)) != 0)
			break;

	return result;
}

template<class Item, class Result, class Param1>
Result 
WhileEachListItem(BObjectList<Item> *list, Result (*func)(Item *, Param1), Param1 p1)
{
	Result result = 0; 
	int32 count = list->CountItems();
	
	for (int32 index = 0; index < count; index++)
		if ((result = (*func)(list->ItemAt(index), p1)) != 0)
			break;

	return result;
}

template<class Item, class Result, class Param1, class Param2>
Result 
WhileEachListItem(BObjectList<Item> *list, Result (Item::*func)(Param1, Param2),
	Param1 p1, Param2 p2)
{
	Result result = 0; 
	int32 count = list->CountItems();
	
	for (int32 index = 0; index < count; index++)
		if ((result = (list->ItemAt(index)->*func)(p1, p2)) != 0)
			break;

	return result;
}

template<class Item, class Result, class Param1, class Param2>
Result 
WhileEachListItem(BObjectList<Item> *list, Result (*func)(Item *, Param1, Param2),
	Param1 p1, Param2 p2)
{
	Result result = 0; 
	int32 count = list->CountItems();
	
	for (int32 index = 0; index < count; index++)
		if ((result = (*func)(list->ItemAt(index), p1, p2)) != 0)
			break;

	return result;
}

template<class Item, class Result, class Param1, class Param2, class Param3, class Param4>
Result 
WhileEachListItem(BObjectList<Item> *list, Result (*func)(Item *, Param1, Param2,
	Param3, Param4), Param1 p1, Param2 p2, Param3 p3, Param4 p4)
{
	Result result = 0; 
	int32 count = list->CountItems();
	
	for (int32 index = 0; index < count; index++)
		if ((result = (*func)(list->ItemAt(index), p1, p2, p3, p4)) != 0)
			break;

	return result;
}

template<class Item, class Result>
void 
EachListItemIgnoreResult(BObjectList<Item> *list, Result (Item::*func)())
{
	int32 count = list->CountItems();
	for (int32 index = 0; index < count; index++)
		(list->ItemAt(index)->*func)();
}

template<class Item, class Param1>
void 
EachListItem(BObjectList<Item> *list, void (*func)(Item *, Param1), Param1 p1)
{
	int32 count = list->CountItems();
	for (int32 index = 0; index < count; index++)
		(func)(list->ItemAt(index), p1);
}

template<class Item, class Param1, class Param2>
void 
EachListItem(BObjectList<Item> *list, void (Item::*func)(Param1, Param2),
	Param1 p1, Param2 p2)
{
	int32 count = list->CountItems();
	for (int32 index = 0; index < count; index++)
		(list->ItemAt(index)->*func)(p1, p2);
}

template<class Item, class Param1, class Param2>
void 
EachListItem(BObjectList<Item> *list, void (*func)(Item *,Param1, Param2),
	Param1 p1, Param2 p2)
{
	int32 count = list->CountItems();
	for (int32 index = 0; index < count; index++)
		(func)(list->ItemAt(index), p1, p2);
}

template<class Item, class Param1, class Param2, class Param3, class Param4>
void 
EachListItem(BObjectList<Item> *list, void (*func)(Item *,Param1, Param2,
	Param3, Param4), Param1 p1, Param2 p2, Param3 p3, Param4 p4)
{
	int32 count = list->CountItems();
	for (int32 index = 0; index < count; index++)
		(func)(list->ItemAt(index), p1, p2, p3, p4);
}

// inline code

inline
_PointerList_::_PointerList_(const _PointerList_ &list)
	:	BList(list),
		owning(list.owning)
{
}

inline
_PointerList_::_PointerList_(int32 itemsPerBlock = 20, bool owningList)
	:	BList(itemsPerBlock),
		owning(owningList)
{
}

inline bool
_PointerList_::Owning() const
{
	return owning;
}

template<class T> 
BObjectList<T>::BObjectList(int32 itemsPerBlock, bool owning)
	:	_PointerList_(itemsPerBlock, owning)
{
}

template<class T> 
BObjectList<T>::BObjectList(const BObjectList<T> &list)
	:	_PointerList_(list)
{
	owning = list.owning;
	if (owning) {
		// make our own copies in an owning list
		int32 count = list.CountItems();
		for	(int32 index = 0; index < count; index++) {
			T *item = list.ItemAt(index);
			if (item)
				item = new T(*item);
			SetItem(index, item);
		}
	}
}

template<class T> 
BObjectList<T>::~BObjectList()
{
	if (Owning())
		// have to nuke elements first
		MakeEmpty();

}

template<class T> 
BObjectList<T> &
BObjectList<T>::operator=(const BObjectList<T> &list)
{
	owning = list.owning;
	BObjectList<T> &result = (BObjectList<T> &)_PointerList_::operator=(list);
	if (owning) {
		// make our own copies in an owning list
		int32 count = list.CountItems();
		for	(int32 index = 0; index < count; index++) {
			T *item = list.ItemAt(index);
			if (item)
				item = new T(*item);
			ReplaceItem(index, item);
		}
	}
	return result;
}

template<class T> 
bool 
BObjectList<T>::AddItem(T *item)
{
	// need to cast to void * to make T work for const pointers
	return _PointerList_::AddItem((void *)item);
}

template<class T> 
bool 
BObjectList<T>::AddItem(T *item, int32 atIndex)
{
	return _PointerList_::AddItem((void *)item, atIndex);
}

template<class T> 
bool 
BObjectList<T>::AddList(BObjectList<T> *newItems)
{
	return _PointerList_::AddList(newItems);
}

template<class T> 
bool 
BObjectList<T>::AddList(BObjectList<T> *newItems, int32 atIndex)
{
	return _PointerList_::AddList(newItems, atIndex);
}


template<class T> 
bool 
BObjectList<T>::RemoveItem(T *item, bool deleteIfOwning)
{
	bool result = _PointerList_::RemoveItem((void *)item);
	
	if (result && Owning() && deleteIfOwning)
		delete item;

	return result;
}

template<class T> 
T *
BObjectList<T>::RemoveItemAt(int32 index)
{
	return (T *)_PointerList_::RemoveItem(index);
}

template<class T> 
inline T *
BObjectList<T>::ItemAt(int32 index) const
{
	return (T *)_PointerList_::ItemAt(index);
}

template<class T>
bool 
BObjectList<T>::ReplaceItem(int32 index, T *item)
{
	if (owning)
		delete ItemAt(index);
	return _PointerList_::ReplaceItem(index, (void *)item);
}

template<class T>
T *
BObjectList<T>::SwapWithItem(int32 index, T *newItem)
{
	T *result = ItemAt(index);
	_PointerList_::ReplaceItem(index, (void *)newItem);
	return result;
}

template<class T>
void 
BObjectList<T>::SetItem(int32 index, T *newItem)
{
	_PointerList_::ReplaceItem(index, (void *)newItem);
}

template<class T> 
int32 
BObjectList<T>::IndexOf(const T *item) const
{
	return _PointerList_::IndexOf((void *)item);
}

template<class T> 
T *
BObjectList<T>::FirstItem() const
{
	return (T *)_PointerList_::FirstItem();
}

template<class T> 
T *
BObjectList<T>::LastItem() const
{
	return (T *)_PointerList_::LastItem();
}

template<class T> 
bool 
BObjectList<T>::HasItem(const T *item) const
{
	return _PointerList_::HasItem((void *)item);
}

template<class T> 
bool 
BObjectList<T>::IsEmpty() const
{
	return _PointerList_::IsEmpty();
}

template<class T> 
int32 
BObjectList<T>::CountItems() const
{
	return _PointerList_::CountItems();
}

template<class T> 
void 
BObjectList<T>::MakeEmpty()
{
	if (owning) {
		for (int32 count = CountItems() - 1; count >= 0; count--)
			RemoveItem(ItemAt(count));
	}
	_PointerList_::MakeEmpty();
}

template<class T> 
T *
BObjectList<T>::EachElement(EachFunction func, void *params)
{ 
	return (T *)_PointerList_::EachElement((GenericEachFunction)func, params); 
}


template<class T> 
const T *
BObjectList<T>::EachElement(ConstEachFunction func, void *params) const
{ 
	return (const T *)
		const_cast<BObjectList<T> *>(this)->_PointerList_::EachElement(
		(GenericEachFunction)func, params); 
}


template<class T> 
void
BObjectList<T>::SortItems(CompareFunction function)
{ 
	_PointerList_::SortItems((GenericCompareFunction)function); 
}

template<class T>
void 
BObjectList<T>::SortItems(CompareFunctionWithState function, void *state)
{
	_PointerList_::SortItems((GenericCompareFunctionWithState)function, state); 
}

template<class T> 
void
BObjectList<T>::HSortItems(CompareFunction function)
{ 
	_PointerList_::HSortItems((GenericCompareFunction)function); 
}

template<class T>
void 
BObjectList<T>::HSortItems(CompareFunctionWithState function, void *state)
{
	_PointerList_::HSortItems((GenericCompareFunctionWithState)function, state); 
}

template<class T>
const T *
BObjectList<T>::BinarySearch(const T &key, CompareFunction func) const
{
	return (const T *)_PointerList_::BinarySearch(&key,
		(GenericCompareFunction)func);
}

template<class T>
const T *
BObjectList<T>::BinarySearch(const T &key, CompareFunctionWithState func, void *state) const
{
	return (const T *)_PointerList_::BinarySearch(&key,
		(GenericCompareFunctionWithState)func, state);
}

#endif
