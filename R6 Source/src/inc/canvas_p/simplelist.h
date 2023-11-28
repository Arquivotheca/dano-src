#ifndef SIMPLELIST_H
#define SIMPLELIST_H

#include<assert.h>

// Forward declarations
#include "alist.h"
#include "listiterator.h"
#include "doubleendedlist.h"

//======================================================
// template class: simplelist
//
// This class combines the base list class with some of 
// the more useful aspects of the listIterator template
// to make for a more useful list class.  The specific
// changes make it a bit easier to add and remove
// elements from the list.
//======================================================
template <class T> class simplelist : public doubleEndedList<T>
{
public:
				simplelist();
				simplelist(const List<T>& other);

	virtual void	AddBefore(T aValue, T before);
	virtual void	AddAfter(T aValue, T after);
	
	virtual void	Remove(T aValue);

protected:
	
private:
};

//===================================================
// Implementation: simplelist
//===================================================
template <class T>
simplelist<T>::simplelist()
	: doubleEndedList<T>()
{}

template <class T>
simplelist<T>::simplelist(const List<T>& other)
	: List<T>(other)
{
	if (other.IsEmpty())
	{
		fFirstLink = 0;
	} else
	{
		fFirstLink = other.fFirstLink->Copy();
	}
}


template <class T>
void
simplelist<T>::AddBefore(T aValue, T before)
{
	// use the iterator to find the right
	// spot in the list
	listIterator<T> iter(*this);

	for (iter.init(); !iter.done(); iter.next())
		if (iter() == before)
		{
			// Once we've found the right element,
			// use the iterator to remove it, we're done.
			iter.AddBefore(aValue);
			return ;
		}
}
template <class T>
void
simplelist<T>::AddAfter(T aValue, T after)
{
	// use the iterator to find the right
	// spot in the list
	listIterator<T> iter(*this);

	for (iter.init(); !iter.done(); iter.next())
		if (iter() == after)
		{
			// Once we've found the right element,
			// use the iterator to remove it, we're done.
			iter.AddAfter(aValue);
			return ;
		}
}


/*
	We have to do our own Remove() here because for a
	double ended list, you have to make sure the last
	link stays up to date.
*/

template <class T>
void
simplelist<T>::Remove(T aValue)
{
	// Find the element we are looking for
	if (IsEmpty())
		return;
		
	alink<T> *currentLink = fFirstLink;
	alink<T> *previousLink = 0;
	
	while (currentLink)
	{
		if (currentLink->fValue == aValue)
		{
			if (previousLink == 0)
			{
				fFirstLink = currentLink->fNextalink;
				if (fLastLink == currentLink)
					fLastLink = fFirstLink;
			} else
			{
				previousLink->fNextalink = currentLink->fNextalink;
				if (fLastLink == currentLink)
					fLastLink = previousLink;
			}
			
			delete currentLink;
			return;
		}	
		previousLink = currentLink;
		currentLink = previousLink->fNextalink;
	}
}

#endif
