#ifndef LISTITERATOR_H
#define LISTITERATOR_H

#include "aiterator.h"
#include "alist.h"

//===========================================
// Class Template: listIterator
// The concrete template class implementation of Iterator
//===========================================
template<class AType> 
class listIterator : public Iterator<AType>
{
public:
				listIterator(List<AType>& aList);

			// Implement the Iterator protocol
			virtual	bool	init();
			virtual	long	next();
			virtual bool	done();
			
			virtual	AType	operator()();
			virtual	long	operator++();
			virtual	void	operator=(AType newValue);

			// Specifics to listIterator
			void	RemoveCurrent();
			void	AddBefore(AType newValue);
			void	AddAfter(AType newValue);

protected:
	alink<AType> *	fCurrentalink;
	alink<AType> *	fPreviousLink;
	List<AType> &	fList;

private:

};

//===========================================
// Class Template: listIterator
// Implementations
//===========================================

template <class AType>
listIterator<AType>::listIterator(List<AType>& aList)
	: fList(aList)
{
	init();
}

// Implement the Iterator protocol
template <class AType>
bool
listIterator<AType>::init()
{
	fPreviousLink = 0;
	fCurrentalink = fList.fFirstLink;
	
	return fCurrentalink != 0;
}

template <class AType>
AType
listIterator<AType>::operator()()
{
	assert(fCurrentalink != 0);
	
	return fCurrentalink->fValue;
}

template <class AType>
void
listIterator<AType>::operator=(AType newValue)
{
	assert(fCurrentalink != 0);
	
	fCurrentalink->fValue = newValue;
}


template <class AType>
bool
listIterator<AType>::done()
{
	if (fCurrentalink == 0)
		if (fPreviousLink != 0)
			fCurrentalink = fPreviousLink->fNextalink;
	
	return fCurrentalink == 0;
}


template <class AType>
long
listIterator<AType>::operator ++()
{
	if (fCurrentalink == 0)
	{
		if (fPreviousLink == 0)
			fCurrentalink = fList.fFirstLink;
		else
			fCurrentalink = fPreviousLink->fNextalink;
	} else
	{
		fPreviousLink = fCurrentalink;
		fCurrentalink = fCurrentalink->fNextalink;
	}

	return fCurrentalink != 0;
}

template <class AType>
long
listIterator<AType>::next()
{
	if (fCurrentalink == 0)
	{
		if (fPreviousLink == 0)
			fCurrentalink = fList.fFirstLink;
		else
			fCurrentalink = fPreviousLink->fNextalink;
	} else
	{
		fPreviousLink = fCurrentalink;
		fCurrentalink = fCurrentalink->fNextalink;
	}

	return fCurrentalink != 0;
}


// list Iterators have these specifics
template <class AType>
void
listIterator<AType>::RemoveCurrent()
{
	assert(fCurrentalink != 0);
	
	if (fPreviousLink == 0)
		fList.fFirstLink = fCurrentalink->fNextalink;
	else
		fPreviousLink->fNextalink = fCurrentalink->fNextalink;
	
	delete fCurrentalink;
	
	fCurrentalink = 0;
}

template <class AType>
void
listIterator<AType>::AddBefore(AType newValue)
{
	if (fPreviousLink)
		fPreviousLink = fPreviousLink->Insert(newValue);
	else
	{
		fList.List<AType>::Add(newValue);
		fPreviousLink = fList.fFirstLink;
		fCurrentalink = fPreviousLink->fNextalink;
	}
}

template <class AType>
void
listIterator<AType>::AddAfter(AType newValue)
{
	if (fCurrentalink != 0)
	{
		fCurrentalink->Insert(newValue);
	} else if (fPreviousLink != 0)
		fCurrentalink = fPreviousLink->Insert(newValue);
	else
		fList.List<AType>::Add(newValue);
}


#endif
