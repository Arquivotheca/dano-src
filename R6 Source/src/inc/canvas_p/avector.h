#ifndef AVECTOR_H
#define AVECTOR_H

#include <assert.h>
#include <malloc.h>

//====================================================================
// Template Interface: vector
//
// A vector simply encapsulates a array of elements of a specified
// type.  This vector implementation can be resized using the SetSize()
// method.  
//
// The SetSize method is called externally to this object, so someone
// else is responsible for actually creating the space necessary.  It is
// often common to allocate double the current size whenever more space
// is needed.  This generally gives good preformance.
//====================================================================

template <class T>
class Vector
{
public:
		Vector(const unsigned long nElements);
		Vector(const unsigned long nElements, const T& initialValue);
		Vector(const Vector&);
		~Vector();
		
		unsigned long	Length() const;
		unsigned long	SetSize(const unsigned long nElements);
		unsigned long	SetSize(const unsigned long nElements, const T& newValue);
		
		void	SetAllToValue(const T& newValue);
		
		T& operator[](const unsigned long idx) const;

protected:
	T*			fData;
	unsigned long	fSize;
};


//==============================================
// Implementation
//==============================================

template<class T> Vector<T>::Vector(const unsigned long nElements)
	: fData(0),
	fSize(0)
{
	fData = (T *)malloc(nElements * sizeof(T));
	//new T[nElements];
	
	// Make sure the buffer is actually allocated
	// Really, this should throw an exception, but we'll
	// let the underlying allocation code do that for us.
	// even this assertion is a bit nasty in that your progam
	// will typically exit on an assertion, and that may not be what you want.
	assert(fData != 0);
	fSize = nElements;
}

template<class T> 
Vector<T>::Vector(const unsigned long nElements, const T& initialValue)
	: fData(0),
	fSize(0)
{
	fData = (T *)malloc(nElements * sizeof(T));
	//fData = new T[nElements];
	assert(fData!=0);
	fSize = nElements;

	// Assign the initial value to all the cells
	for (unsigned long counter = 0; counter < nElements; counter++)
		fData[counter] = initialValue;
}

template<class T> Vector<T>::Vector(const Vector<T>& other)
	: fData(0),
	fSize(0)
{
	fData = (T *)malloc(other.fSize * sizeof(T));
	//fData = new T[other.fSize];
	assert(fData != 0);

	fSize = other.fSize;

	for (unsigned long counter = 0; counter < fSize; counter++)
   		fData[counter] = other.fData[counter];
}

template<class T> 
Vector<T>::~Vector()
{
	fSize =0;
	if (fData)
		free(fData);
	//delete [] fData;
	fData = 0;
}

template<class T> 
T& 
Vector<T>::operator[](const unsigned long idx) const
{
	assert(idx < fSize);

	return fData[idx];
}

template<class T> 
unsigned long
Vector<T>::Length() const
{
	return fSize;
}

template<class T> 
unsigned long
Vector<T>::SetSize(const unsigned long nElements, const T& initialValue)
{
	T* newData = (T *)malloc(nElements * sizeof(T));
	//T* newData = new T[nElements];
	
	assert(newData != 0);
	long counterLimit;

	if (nElements <= fSize)
	{
		counterLimit = nElements;
	} else
	{
		counterLimit = fSize;

		// Set the initial values for the new cells
		for (long ctr3=fSize; ctr3<nElements; ctr3++)
		{
			newData[ctr3] = initialValue;
		}
	}

	// Copy the existing values into the new data area
	for (long ctr1 =0; ctr1 < counterLimit; ctr1++)
	{
			newData[ctr1] = fData[ctr1];
	}

	free(fData);
	//delete [] fData;
	fData = newData;
	fSize = nElements;

	return fSize;
}

template<class T> 
unsigned long
Vector<T>::SetSize(const unsigned long nElements)
{
	T* newData = (T *)malloc(nElements * sizeof(T));
	//T* newData = new T[nElements];
	assert(newData != 0);

	if (nElements <= fSize)
	{
		// Shrinking down from our current size
		for (long ctr1 =0; ctr1 < nElements; ctr1++)
		{
			newData[ctr1] = fData[ctr1];
		}
	} else
	{
		for (long ctr2 = 0; ctr2 < fSize; ctr2++)
		{
			newData[ctr2] = fData[ctr2];
		}

	}

	free(fData);
	//delete [] fData;
	fData = newData;
	fSize = nElements;

	return fSize;

}

template<class T> 
void
Vector<T>::SetAllToValue(const T& newValue)
{
	// Shrinking down from our current size
	for (unsigned long ctr1 =0; ctr1 < fSize; ctr1++)
	{
		fData[ctr1] = newValue;
	}
}



// Utility functions for vectors and others
template <class T>
T Max(T a, T b)
{
	if (a<b)
		return b;
	return a;
}

template <class T>
void Swap(Vector<T>& theSeq, unsigned long idx1, unsigned long idx2)
{
	
	T tempValue = theSeq[idx1];
	theSeq[idx1] = theSeq[idx2];
	theSeq[idx2] = tempValue;
}



#endif
