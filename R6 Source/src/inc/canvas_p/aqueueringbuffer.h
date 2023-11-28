#ifndef QUEUERINGBUFFER_H
#define QUEUERINGBUFFER_H

#include "aqueue.h"
#include "avector.h"

#include <Autolock.h>
#include <Locker.h>
#include <OS.h>

//========================================================
// Class: queueRingBuffer
//
// This is an implementation of a ringbuffer which uses
// the queue protocol for access.
//========================================================

template <class AType> class queueRingBuffer : public Queue<AType>
{
public:
		queueRingBuffer(const uint32 size);
		queueRingBuffer(const queueRingBuffer& other);
		
	virtual uint32	Capacity() const;	// How many can it hold
	
	// Implementation of queue protocol
	virtual bool			empty() ;
	virtual int32			size() ;		// How many currently held
	virtual AType&			front();		// Report the item at the front
	virtual const AType&	front()const;		// Report the item at the front as const

	virtual void	push(const AType& val);
	virtual AType&	pop();

	// And some additions
	virtual	void	MakeEmpty();
	

protected:
	BLocker		fLock;
	Vector<AType>	fData;		// The sequence that holds the data
	const uint32	fMaxSize;	// Max num items in queue
	uint32		fSize;		// How many items in queue
	uint32		fNextSlot;	// Where next item will be enqueued
	uint32		fFront;		// Where Front and Dequeue will get item from
};


//===================================================
// Implementation: queueRingBuffer
//===================================================

template <class AType> 
queueRingBuffer<AType>::queueRingBuffer(const uint32 size)
	: fMaxSize(size),
	fLock(),
	fData(size),
	fSize(0)
{
	// Initialize sequence to being empty
	MakeEmpty();
}

template <class AType> 
queueRingBuffer<AType>::queueRingBuffer(const queueRingBuffer& other)
	: fMaxSize(other.fMaxSize),
	fLock(),
	fData(other.fData),
	fNextSlot(other.fNextSlot),
	fFront(other.fFront),
	fSize(other.fSize)
{
}

template <class AType> 
void 
queueRingBuffer<AType>::MakeEmpty()
{
	BAutolock autoLock(&fLock);
	
	fNextSlot = 0;
	fFront = 0;
	fSize = 0;
}

template <class AType> 
bool
queueRingBuffer<AType>::empty()
{
	BAutolock autoLock(&fLock);

	return (fNextSlot == fFront);
}


template <class AType>
uint32
queueRingBuffer<AType>::Capacity() const
{
	return fMaxSize;	
}

template <class AType>
int32
queueRingBuffer<AType>::size()
{
	BAutolock autoLock(&fLock);
	
	return fSize;	
}

template <class AType> 
void
queueRingBuffer<AType>::push(const AType &val)
{
	BAutolock autoLock(&fLock);

	fData[fNextSlot] = val;
	fNextSlot++;
	if (fNextSlot >= fMaxSize)
		fNextSlot = 0;
		
	if (fSize < fMaxSize)
		fSize++;
}

template <class AType> 
AType &
queueRingBuffer<AType>::pop()
{
	BAutolock autoLock(&fLock);

	int32 dataloc = fFront;
	
	fFront++;
	if (fFront >= fMaxSize)
		fFront = 0;
	
	if (fSize > 0)
		fSize--;

	return fData[dataloc];
}

template <class AType> 
const AType &
queueRingBuffer<AType>::front() const
{
	BAutolock autoLock(&((queueRingBuffer<AType> *) this)->fLock);

	return fData[fFront];
}

template <class AType> 
AType &
queueRingBuffer<AType>::front()
{
	BAutolock autoLock(&fLock);

	return fData[fFront];
}

#endif
