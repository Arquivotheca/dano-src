#ifndef __LIST_TEMPLATE__
#define __LIST_TEMPLATE__

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#ifndef _BE_H
#include <List.h>
#endif

#include <Debug.h>


// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)


// this glue class is here to provide a better iterator function,
// support of owning and sorting with context
class PointerList : public BList {
public:
	PointerList(const PointerList &list);
	PointerList(int32 itemsPerBlock = 20, bool owning = false);
	virtual ~PointerList();

	typedef void *(* GenericEachFunction)(void *, void *);
	typedef int (* GenericCompareFunction)(const void *, const void *);
	void *EachElement(GenericEachFunction, void *);
	
	void SortItems(int (*cmp)(const void *, const void *, const void *), void *);
		// SortItems with context

	bool AddUnique(void *);
		// return true if item added or already in the list
	bool AddUnique(void *, GenericCompareFunction);

	bool Owning() const;
private:
	typedef BList _inherited;
	const bool owning;
};

// TypedList -
// adds proper typing, owning, sorting with context
// can be passed to calls with BList * parameters
template<class T>
class TypedList : public PointerList {
public:
	TypedList(int32 itemsPerBlock = 20, bool owning = false);
		// if <owning> passed as true, delete will delete all
		// the items first
	TypedList(const TypedList&);
	virtual ~TypedList();

	TypedList &operator=(const TypedList &);

	// iteration and sorting
	typedef T (* EachFunction)(T, void *);
	typedef const T (* ConstEachFunction)(const T, void *);
	typedef int (* CompareFunction)(const T *, const T *);
	typedef int (* CompareFunctionWithContext)(const T *, const T *, void *);

	// adding and removing
	bool AddItem(T);
	bool AddItem(T, int32);
	bool AddList(TypedList *);
	bool AddList(TypedList *, int32);
	bool AddUnique(T);
	bool AddUnique(T, CompareFunction);
	
	bool RemoveItem(T);
	T RemoveItem(int32);
	T RemoveItemAt(int32);
		// same as RemoveItem(int32), RemoveItem does not work when T is a scalar

	void MakeEmpty();

	// item access
	T ItemAt(int32) const;
	T ItemAtFast(int32) const;
		// does not do an index range check

	T FirstItem() const;
	T LastItem() const;
	
	T Items() const;

	// misc. getters
	int32 IndexOf(const T) const;
	bool HasItem(const T) const;
	bool IsEmpty() const;
	int32 CountItems() const;


	T EachElement(EachFunction, void *);
	const T EachElement(ConstEachFunction, void *) const;
		// Do for each are obsoleted by this list, possibly add
		// them for convenience
	void SortItems(CompareFunction);
	void SortItems(CompareFunctionWithContext, void *);
	bool ReplaceItem(int32 index, T item);
	bool SwapItems(int32 a, int32 b);
	bool MoveItem(int32 from, int32 to);

private:
	typedef PointerList _inherited;
	friend class ParseArray;
};

// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)


template<class T> 
TypedList<T>::TypedList(int32 itemsPerBlock, bool owning)
	:	PointerList(itemsPerBlock, owning)
{
	ASSERT(!owning);
}

template<class T> 
TypedList<T>::TypedList(const TypedList<T> &list)
	:	PointerList(list)
{
	ASSERT(!list.Owning());
	// copying owned lists does not work yet
}

template<class T> 
TypedList<T>::~TypedList()
{
	if (Owning())
		// have to nuke elements first
		MakeEmpty();
}

template<class T> 
TypedList<T> &
TypedList<T>::operator=(const TypedList<T> &from)
{
	ASSERT(!from.Owning());
	// copying owned lists does not work yet

	return (TypedList<T> &)_inherited::operator=(from);
}

template<class T> 
bool 
TypedList<T>::AddItem(T item)
{
	return _inherited::AddItem((void *)item);
		// cast needed for const flavors of T
}

template<class T> 
bool 
TypedList<T>::AddItem(T item, int32 atIndex)
{
	return _inherited::AddItem((void *)item, atIndex);
}

template<class T> 
bool 
TypedList<T>::AddList(TypedList<T> *newItems)
{
	return _inherited::AddList(newItems);
}

template<class T> 
bool
TypedList<T>::AddList(TypedList<T> *newItems, int32 atIndex)
{
	return _inherited::AddList(newItems, atIndex);
}

template<class T>
bool 
TypedList<T>::AddUnique(T item)
{
	return _inherited::AddUnique((void *)item);
}

template<class T>
bool 
TypedList<T>::AddUnique(T item, CompareFunction function)
{
	return _inherited::AddUnique((void *)item, (GenericCompareFunction)function);
}


template<class T> 
bool 
TypedList<T>::RemoveItem(T item)
{
	bool result = _inherited::RemoveItem((void *)item);
	
	if (result && Owning()) {
		delete item;
	}

	return result;
}

template<class T> 
T 
TypedList<T>::RemoveItem(int32 index)
{
	return (T)_inherited::RemoveItem(index);
}

template<class T> 
T 
TypedList<T>::RemoveItemAt(int32 index)
{
	return (T)_inherited::RemoveItem(index);
}

template<class T> 
T 
TypedList<T>::ItemAt(int32 index) const
{
	return (T)_inherited::ItemAt(index);
}

template<class T> 
T 
TypedList<T>::ItemAtFast(int32 index) const
{
	return (T)_inherited::ItemAtFast(index);
}

template<class T> 
int32 
TypedList<T>::IndexOf(const T item) const
{
	return _inherited::IndexOf((void *)item);
}

template<class T> 
T 
TypedList<T>::FirstItem() const
{
	return (T)_inherited::FirstItem();
}

template<class T> 
T 
TypedList<T>::LastItem() const
{
	return (T)_inherited::LastItem();
}

template<class T> 
bool 
TypedList<T>::HasItem(const T item) const
{
	return _inherited::HasItem((void *)item);
}

template<class T> 
bool 
TypedList<T>::IsEmpty() const
{
	return _inherited::IsEmpty();
}

template<class T> 
int32 
TypedList<T>::CountItems() const
{
	return _inherited::CountItems();
}

template<class T> 
void 
TypedList<T>::MakeEmpty()
{
	if (Owning()) {
		int32 numElements = CountItems();
		
		for (int32 count = 0; count < numElements; count++)
			// this is probably not the most efficient, but
			// is relatively indepenent of BList implementation
			// details
			RemoveItem(LastItem());
	}
	_inherited::MakeEmpty();
}

template<class T> 
T
TypedList<T>::EachElement(EachFunction func, void *params)
{ 
	return (T)_inherited::EachElement((GenericEachFunction)func, params); 
}


template<class T> 
const T
TypedList<T>::EachElement(ConstEachFunction func, void *params) const
{ 
	return (const T)
		const_cast<TypedList<T> *>(this)->_inherited::EachElement(
		(GenericEachFunction)func, params); 
}


template<class T>
T
TypedList<T>::Items() const
{
	return (T)_inherited::Items();
}

template<class T> 
void
TypedList<T>::SortItems(CompareFunction function)
{ 
	ASSERT(sizeof(T) == sizeof(void *));
	BList::SortItems((GenericCompareFunction)function);
}

template<class T> 
void
TypedList<T>::SortItems(CompareFunctionWithContext function,
	void *params)
{ 
	ASSERT(sizeof(T) == sizeof(void *));
	_inherited::SortItems((int (*)(const void *,
		const void *, const void *))function, params);
}

template<class T>
bool TypedList<T>::ReplaceItem(int32 index, T item)
{
	return PointerList::ReplaceItem(index, (void *)item);
}

template<class T>
bool TypedList<T>::SwapItems(int32 a, int32 b)
{
	return PointerList::SwapItems(a, b);
}

template<class T>
bool TypedList<T>::MoveItem(int32 from, int32 to)
{
	return PointerList::MoveItem(from, to);
}

// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)
// DEPRECATED, DO NOT USE (USE BObjectList instead)

#endif
