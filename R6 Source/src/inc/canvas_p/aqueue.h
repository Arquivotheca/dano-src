#ifndef AQUEUE_H
#define AQUEUE_H

//========================================================
// Protocol Template: Queue
//
// The protocol for a common data structure, the queue.
// More specific implementations will utilize various low
// level data structures for actual data storage.
//========================================================

template <class AType> class Queue
{
public:
	virtual bool			empty()  =0;	// is the queue empty
	virtual int32			size() =0;		// How many items are in queue
	virtual AType&			front() = 0;		// Report the item at the front
	virtual const AType&	front()const=0;		// Report the item at the front as const
	//virtual AType& 			back()=0;			// report item at the back
	//virtual const AType& 	back()const=0;		// report item at the back as const
	
	virtual void	push(const AType &val)=0;		// put a new item into the queue
	virtual AType&	pop()=0;						// Pop current front item out of queue
};

#endif
