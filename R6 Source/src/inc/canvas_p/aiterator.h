#ifndef AITERATOR_H
#define AITERATOR_H

//===========================================================
// File: aIterator.h
//
// Contains interfaces for abstract Iterator 
// base class.  The typical usage of this class
// will be something like this:
//
//   myIterator<int> itr;
//
//	for (itr.init(); !itr.done(); itr.next())
//	{
//		if (itr() < 5)
//		printf("value less than 5\n");
//  }
//
// You will have to create sub-classes of the Iterator to
// actually do anything useful.
//===========================================================


//===========================================================
// Abstract Class: Iterator
//
//===========================================================
template <class AType> 
class Iterator
{
public:
			virtual	bool	init() = 0;
			virtual long	next()=0;
			virtual bool	done()=0;

			virtual	AType	operator()()=	0;
			virtual	long	operator++() = 0;
			virtual	void	operator=(AType newValue) = 0;

protected:

private:
};

#endif
