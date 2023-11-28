#ifndef _QUEUE_H
#define _QUEUE_H

#include <Debug.h>

class QueueEntry {
public:
	inline QueueEntry();
	inline virtual ~QueueEntry();
private:
	QueueEntry *fNext, *fPrev;
	friend class Queue;
};

class Queue {
public:
	inline Queue();
	inline void Enqueue(QueueEntry *entry);
	inline QueueEntry* Dequeue();
	inline QueueEntry* Head();
	inline void RemoveEntry(QueueEntry *entry);
	inline int32 CountItems() const;
	inline bool IsEmpty() const;
	QueueEntry* GetNext(QueueEntry *entry) const;
	QueueEntry* GetPrev(QueueEntry *entry) const;

private:
	QueueEntry fDummyHead;
};

inline QueueEntry::QueueEntry()
	:	fNext(0),
		fPrev(0)
{
}

inline Queue::Queue()
{
	fDummyHead.fNext = &fDummyHead;
	fDummyHead.fPrev = &fDummyHead;
}

inline QueueEntry::~QueueEntry()
{
}

inline void Queue::Enqueue(QueueEntry *entry)
{
	ASSERT(entry->fPrev == 0);
	ASSERT(entry->fNext == 0);

	entry->fPrev = &fDummyHead;
	entry->fNext = fDummyHead.fNext;
	entry->fPrev->fNext = entry;
	entry->fNext->fPrev = entry;
}

inline QueueEntry* Queue::Dequeue()
{
	QueueEntry *entry = fDummyHead.fPrev;
	if (entry == &fDummyHead)
		return 0;

	ASSERT(entry->fNext->fPrev == entry);
	ASSERT(entry->fPrev->fNext == entry);
		
	entry->fNext->fPrev = entry->fPrev;
	entry->fPrev->fNext = entry->fNext;
	
	// Setting these to zero indicates this isn't queued
	entry->fNext = 0;
	entry->fPrev = 0;

	return entry;
}

inline void Queue::RemoveEntry(QueueEntry *entry)
{
	if (entry->fPrev != 0) {
		ASSERT(entry->fNext != 0);
		ASSERT(entry->fNext->fPrev == entry);
		ASSERT(entry->fPrev->fNext == entry);
		
		entry->fNext->fPrev = entry->fPrev;
		entry->fPrev->fNext = entry->fNext;
		entry->fNext = 0;
		entry->fPrev = 0;
	}
}

inline QueueEntry* Queue::Head()
{
	if (fDummyHead.fPrev == &fDummyHead)
		return 0;
	
	return fDummyHead.fPrev;
}

inline int32 Queue::CountItems() const
{
	int32 count = 0;
	for (QueueEntry *entry = fDummyHead.fNext; entry != &fDummyHead;
		entry = entry->fNext)
		count++;
		
	return count;
}

inline bool Queue::IsEmpty() const
{
	return fDummyHead.fNext == &fDummyHead;
}

inline QueueEntry* Queue::GetPrev(QueueEntry *entry) const
{
	return entry->fNext == &fDummyHead ? 0 : entry->fNext;
}

inline QueueEntry* Queue::GetNext(QueueEntry *entry) const
{
	return entry->fPrev == &fDummyHead ? 0 : entry->fPrev;
}



#endif
