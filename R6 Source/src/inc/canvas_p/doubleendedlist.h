#ifndef DOUBLEENDEDLIST_H
#define DOUBLEENDEDLIST_H

#include "alist.h"
#include <stdio.h>

template <class T> class doubleEndedList : public List<T>
{
public:
			doubleEndedList();
			doubleEndedList(const doubleEndedList &other);
			
	// from list<T>
	virtual void	Add(T value);
	virtual void	DeleteAll();
	virtual void	RemoveFirst();
	
			void	AddToEnd(T value);
			
protected:
	alink<T>		*fLastLink;
	
private:
};



//=========================================================
// 
//=========================================================
template <class T> 
doubleEndedList<T>::doubleEndedList()
	: List<T>(),
	fLastLink(0)
{
}

template <class T> 
doubleEndedList<T>::doubleEndedList(const doubleEndedList &other)
	: List<T>(other),
	fLastLink(other.fFirstLink)
{
	// we need to find the last link, and assign it.
	// we could use an iterator, but it is simpler to 
	// just iterate over the links ourself.
	fLastLink = fFirstLink;
	if (fLastLink)
		while (LastLink->fNextLink)
			fLastLink = fLastLink->fNextLink;
}

template <class T>
void
doubleEndedList<T>::Add(T value)
{
	if (IsEmpty())
	{
		List<T>::Add(value);
		fLastLink = fFirstLink;
	} else
		List<T>::Add(value);
}

template <class T>
void
doubleEndedList<T>::AddToEnd(T value)
{
	if (fLastLink != 0)
		fLastLink = fLastLink->Insert(value);
	else
		Add(value);
}

template <class T>
void
doubleEndedList<T>::DeleteAll()
{
	List<T>::DeleteAll();
	fLastLink = 0;
}


template <class T>
void
doubleEndedList<T>::RemoveFirst()
{
	List<T>::RemoveFirst();
	if (IsEmpty())
		fLastLink = 0;
}


#endif
