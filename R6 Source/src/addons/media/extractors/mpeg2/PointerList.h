#ifndef C_POINTER_LIST_H

#define C_POINTER_LIST_H

#include <List.h>
#include <Debug.h>

template<class T>
class CPointerList : private BList
{
	CPointerList(const CPointerList &);
	CPointerList &operator= (const CPointerList &);

	public:
		CPointerList (int32 itemsPerBlock=20);
		virtual ~CPointerList ();

		bool AddItem (T *item);
		bool AddItem (T *item, int32 index);
		bool RemoveItem (T *item);
		T *RemoveItem (int32 index);
		
		void MakeEmpty(); // does call destructor of items removed
		
		inline T *ItemAt (int32 index);
		inline const T *ItemAt (int32 index) const;

		// the following 2 are for convenience only
		inline T *operator[] (int32 index);
		inline const T *operator[] (int32 index) const;

		inline int32 CountItems() const;
		inline bool IsEmpty() const;
		
		void SortItems (int (*Compare)(const T *a, const T *b));
		void SortItems (int (*Compare)(const T *a, const T *b), int32 from, int32 to);

		void SwapItems (int32 i, int32 j);

		bool BinarySearch (const T &what, int32 *center,
							int (*Compare)(const T *a, const T *b)) const;		

		bool BinaryInsert (T *item, int (*Compare)(const T *a, const T *b), bool unique=false);
};

template<class T>
CPointerList<T>::CPointerList(int32 itemsPerBlock)
	: BList(itemsPerBlock)
{
}

template<class T>
CPointerList<T>::~CPointerList()
{
	MakeEmpty();
}

template<class T>
bool 
CPointerList<T>::AddItem(T *item)
{
	return BList::AddItem(item);
}

template<class T>
bool 
CPointerList<T>::AddItem(T *item, int32 index)
{
	return BList::AddItem(item,index);
}

template<class T>
bool 
CPointerList<T>::RemoveItem(T *item)
{
	return BList::RemoveItem(item);
}

template<class T>
T *
CPointerList<T>::RemoveItem(int32 index)
{
	return static_cast<T *>(BList::RemoveItem(index));
}

template<class T>
void 
CPointerList<T>::MakeEmpty()
{
	for (int32 i=CountItems()-1;i>=0;--i)
		delete RemoveItem(i);
}

template<class T>
T *
CPointerList<T>::ItemAt(int32 index)
{
	return static_cast<T *>(BList::ItemAt(index));
}

template<class T>
const T *
CPointerList<T>::ItemAt(int32 index) const
{
	return static_cast<T *>(BList::ItemAt(index));
}

template<class T>
T *
CPointerList<T>::operator[](int32 index)
{
	return static_cast<T *>(BList::ItemAt(index));
}

template<class T>
const T *
CPointerList<T>::operator[](int32 index) const
{
	return static_cast<T *>(BList::ItemAt(index));
}

template<class T>
int32 
CPointerList<T>::CountItems() const
{
	return BList::CountItems();
}

template<class T>
bool 
CPointerList<T>::IsEmpty() const
{
	return BList::IsEmpty();
}

template<class T>
void 
CPointerList<T>::SwapItems(int32 i, int32 j)
{
	BList::SwapItems(i,j);
}

template<class T>
void 
CPointerList<T>::SortItems (int (*Compare)(const T *a, const T *b))
{
	SortItems(Compare,0,CountItems()-1);
}

template<class T>
void 
CPointerList<T>::SortItems (int (*Compare)(const T *a, const T *b),
							int32 from, int32 to)
{
	int32 end=to;
	
	bool dirty;
	do
	{
		dirty=false;
		
		for (int32 i=from;i<end;++i)
		{
			if ((*Compare)(ItemAt(i+1),ItemAt(i))<0)
			{
				SwapItems(i,i+1);
				dirty=true;
			}
		}
		
		--end;
	}
	while (dirty);
}

template<class T>
bool
CPointerList<T>::BinarySearch (const T &what, int32 *center,
								int (*Compare)(const T *a, const T *b)) const
{
	*center=-1;
	int32 left=0;
	int32 right=CountItems()-1;
	
	while (left<=right)
	{
		*center=(left+right)/2;
		
		int relation=(*Compare)(&what,ItemAt(*center));
		
		if (relation<0) // what<ItemAt(center)
			right=*center-1;
		else if (relation>0) // what>ItemAt(center)
			left=*center+1;
		else
			return true;
	}
	
	return false;
}

template<class T>
bool 
CPointerList<T>::BinaryInsert(T *item, int (*Compare)(const T *a, const T *b),
								bool unique)
{
	int32 index;
	bool found=BinarySearch(*item,&index,Compare);
	
	if (found)
	{
		if (unique)
			return false;
		else
			AddItem(item,index);
	}
	else
	{
		if (index<=0 || (*Compare)(item,ItemAt(index))<0)
		{
			while (index>0 && (*Compare)(item,ItemAt(index))<0)
				--index;
			
			AddItem(item,index+1);
		}
		else
		{
			while (index<CountItems() && (*Compare)(item,ItemAt(index))>0)
				++index;
				
			AddItem(item,index);
		}
	}
	
	return true;
}

#endif

