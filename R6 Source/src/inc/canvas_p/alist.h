#ifndef ALIST_H
#define ALIST_H

#include<assert.h>

// Forward declarations
template <class AType> class List;
template <class AType> class alink;
template <class AType> class listIterator;


//==============================================
//
//==============================================

template <class AType> class alink
{
public:
		alink<AType>	*	Insert(AType aValue);



	// Data
	AType	fValue;
	alink<AType> *	fNextalink;

protected:
private:
				alink(AType aValue, alink<AType> * nextalink);

		alink<AType> *	Copy();


	// friends
	friend class List<AType>;
	friend class listIterator<AType>;
};

//==============================================
//
//==============================================
template <class AType> class List
{
public:
				List();
				List(const List<AType>& other);
	virtual	~List();

	virtual	void	Add(AType aValue);
	virtual	void	DeleteAll();
			AType	FirstElement() const;
	virtual	bool	Includes(const AType& aValue) const;
			bool	IsEmpty() const;
	virtual	void	RemoveFirst();


protected:
	alink<AType>*	fFirstLink;
	
	// friends
	friend class listIterator<AType>;
	
private:
	List<AType> & operator = (const List<AType>& other);
	
};


//===================================================
// Implementation: alink
//===================================================
template <class AType>
alink<AType>::alink(AType aValue, alink<AType>* nextalink)
	: fValue(aValue),
	fNextalink(nextalink)
{
}

template <class AType>
alink<AType> *
alink<AType>::Insert(AType aValue)
{
	fNextalink = new alink<AType>(aValue, fNextalink);
	assert(fNextalink != 0);

	return fNextalink;
}

template <class AType>
alink<AType> *
alink<AType>::Copy()
{
	alink<AType> * newalink;

	if (fNextalink != 0)
		newalink = new alink<AType>(fValue, fNextalink->Copy());
	else
		newalink = new alink<AType>(fValue, 0);

	assert(newalink != 0);

	return newalink;
}

//===================================================
// Implementation: list
//===================================================
template <class AType>
List<AType>::List()
	: fFirstLink(0)
{
	//printf("List()\n");
}

template <class AType>
List<AType>::List(const List<AType>& other)
{
	printf("List(const List&)\n");
	
	if (other.IsEmpty())
	{
		fFirstLink = 0;
	} else
	{
		fFirstLink = other.fFirstLink->Copy();
	}
}

template <class AType>
void
List<AType>::Add(AType aValue)
{
	fFirstLink = new alink<AType>(aValue,fFirstLink);
	assert(fFirstLink!=0);
}

template <class AType>
bool
List<AType>::IsEmpty() const
{
	return (0 == fFirstLink);
}

template <class AType>
AType
List<AType>::FirstElement() const
{
	assert(fFirstLink != 0);
	return fFirstLink->fValue;
}

template <class AType>
void
List<AType>::RemoveFirst()
{
	assert(fFirstLink != 0);

	alink<AType> * tmpPtr = fFirstLink;

	fFirstLink = tmpPtr->fNextalink;

	delete tmpPtr;
}

template <class AType>
bool
List<AType>::Includes(const AType& aValue) const
{
	for (alink<AType> * alink = fFirstLink; alink; alink = alink->fNextalink)
	{
		if (aValue == alink->fValue)
			return true;
	}

	// The value wasn't found, so return 0 (false);
	return false;
}

template <class AType>
List<AType>::~List()
{
	DeleteAll();
}

template <class AType>
void
List<AType>::DeleteAll()
{
	if (!fFirstLink)
		return;
		
	alink<AType> * nextalink;
	for (alink<AType> * alink = fFirstLink; alink != 0; alink = nextalink)
	{
		nextalink = alink->fNextalink;
		alink->fNextalink = 0;
		delete alink;
	}

	fFirstLink = 0;
}

#endif
