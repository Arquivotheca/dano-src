//******************************************************************************
//
//	File:		MessageQueue.cpp
//
//	Description:	Implementation of the MessageQueue class.
//	
//	Written by:	Benoit Schillings
//
//	Copyright 1993, Be Incorporated
//
//	3/28/94		name changed to BMessageQueue	BGS
//******************************************************************************/

/*
 There's a bunch of debugging code here to work on bug #461.
*/

#include <MessageQueue.h>

#include <Autolock.h>
#include <message_util.h>

#ifndef _DEBUG_H
#include <Debug.h>
#endif

/*----------------------------------------------------------------*/

BMessageQueue::BMessageQueue()
	: fLocker("_msg_queue_")
{	
}

/*----------------------------------------------------------------*/

BMessageQueue::BMessageQueue(const char* name)
	: fLocker(name)
{	
}

/*----------------------------------------------------------------*/

bool	BMessageQueue::IsEmpty() const
{
	BAutolock l(fLocker);
	return fList.IsEmpty();
}

/*----------------------------------------------------------------*/

int32	BMessageQueue::CountMessages() const
{
	BAutolock l(fLocker);
	return fList.CountMessages();
}

/*----------------------------------------------------------------*/

BMessageQueue::~BMessageQueue()
{
	BAutolock l(fLocker);
	fList.MakeEmpty();
}

/*----------------------------------------------------------------*/

bool	BMessageQueue::Lock()
{
//##ASSERT((the_queue && message_count) || (!the_queue && !message_count));
	return fLocker.Lock();
}

/*----------------------------------------------------------------*/

void	BMessageQueue::Unlock()
{
//##ASSERT((the_queue && message_count) || (!the_queue && !message_count));
	fLocker.Unlock();
}

/*----------------------------------------------------------------*/

bool	BMessageQueue::IsLocked() const
{
	return fLocker.LockingThread() == find_thread(NULL);
}

/*----------------------------------------------------------------*/

void	BMessageQueue::UnlockFully()
{
	if (!IsLocked()) return;
	int32 i = fLocker.CountLocks();
	while (i > 0) fLocker.Unlock();
}

/*----------------------------------------------------------------*/

char	BMessageQueue::message_filter(BMessage *)
{
	return(1);
}

/*----------------------------------------------------------------*/

void	BMessageQueue::AddMessage(BMessage *msg)
{
	BAutolock l(fLocker);
	fList.EnqueueMessage(msg);
}

/*----------------------------------------------------------------*/

bool BMessageQueue::IsNextMessage(const BMessage* test) const
{
	BAutolock l(fLocker);
	return fList.Head() == test;
}

/*----------------------------------------------------------------*/

bigtime_t BMessageQueue::NextTime() const
{
	BAutolock l(fLocker);
	return fList.OldestMessage();
}

/*----------------------------------------------------------------*/

BMessage* BMessageQueue::NextMessage()
{
	BAutolock l(fLocker);
	return fList.RemoveHead();
}

/*----------------------------------------------------------------*/

bool	BMessageQueue::RemoveMessage(BMessage *an_event)
{
	BAutolock l(fLocker);
	return fList.Remove(an_event) ? true : false;
}

/*----------------------------------------------------------------*/

int32	BMessageQueue::RemoveMessages(uint32 what, const BMessenger* target,
									  BMessageList* into)
{
	BAutolock l(fLocker);
	
	const BMessage* i = fList.Head();
	int32 cnt = 0;
	while (i) {
		const BMessage* next = fList.Next(i);
		if (i->what == what && (!target || i->CompareDestination(*target))) {
			if (into) into->AddTail(fList.Remove(i));
			else delete fList.Remove(i);
			cnt++;
		}
		i = next;
	}
	return cnt;
}

/*----------------------------------------------------------------*/

BMessage *BMessageQueue::FindMessage(int32 index) const
{
	return FindMessage(TRUE, 0, index);
}

/*----------------------------------------------------------------*/

BMessage *BMessageQueue::FindMessage(uint32 what, int32 index) const
{
	return FindMessage(FALSE, what, index);
}

/*----------------------------------------------------------------*/

BMessage *BMessageQueue::FindMessage(uint32 what, const BMessenger& target, int32 index) const
{
	BAutolock l(fLocker);
	
	const BMessage* i = fList.Head();
	int32 cnt = 0;
	while (i) {
		if (i->what == what && i->CompareDestination(target)) {
			if (cnt == index) return const_cast<BMessage*>(i);
			cnt++;
		}
		i = fList.Next(i);
	}
	
	return NULL;
}

/*----------------------------------------------------------------*/

BMessage *BMessageQueue::FindMessage(bool anyWhat, uint32 what, int32 index) const
{
	BAutolock l(fLocker);
	
	const BMessage* i = fList.Head();
	int32 cnt = 0;
	while (i) {
		if (anyWhat || i->what == what) {
			if (cnt == index) return const_cast<BMessage*>(i);
			cnt++;
		}
		i = fList.Next(i);
	}
	
	return NULL;
}

/*----------------------------------------------------------------*/

BDataIO& operator<<(BDataIO& io, const BMessageQueue& queue)
{
#if SUPPORTS_STREAM_IO
	io << queue.fList;
#else
	(void)queue;
#endif
	return io;
}

/*----------------------------------------------------------------*/

BMessageQueue::BMessageQueue(const BMessageQueue &) {}
BMessageQueue &BMessageQueue::operator=(const BMessageQueue &) { return *this; }

/* ---------------------------------------------------------------- */

void BMessageQueue::_ReservedMessageQueue1() {}
void BMessageQueue::_ReservedMessageQueue2() {}
void BMessageQueue::_ReservedMessageQueue3() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
