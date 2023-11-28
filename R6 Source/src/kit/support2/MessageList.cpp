//******************************************************************************
//
//	File:		MessageList.cpp
//
//	Description:	Implementation of the MessageList class.
//	
//	Written by:	Dianne Hackborn
//
//	Copyright 2000, Be Incorporated
//
//******************************************************************************/

#include <support2/MessageList.h>

#include <support2/Message.h>
#include <support2/StdIO.h>
#include <support2/ITextStream.h>
#include <support2/StdIO.h>
#include <support2/String.h>
#include <support2/Debug.h>
#include <kernel/OS.h>

namespace B {
namespace Support2 {

/*----------------------------------------------------------------*/

BMessageList::BMessageList()
	: fHead(NULL), fTail(NULL), fCount(0)
{
}

BMessageList::~BMessageList()
{
	MakeEmpty();
}

BMessageList::BMessageList(const BMessageList &o)
	: fHead(NULL), fTail(NULL), fCount(0)
{
	*this = o;
}

BMessageList& BMessageList::operator=(const BMessageList &o)
{
	MakeEmpty();
	BMessage* p = o.fHead;
	BMessage* last = NULL;
	while (p) {
		BMessage* copy = new BMessage(*p);
		if (!last) {
			fHead = copy;
		} else {
			last->m_next = copy;
			copy->m_prev = last;
		}
		last = copy;
		p = p->m_next;
	}
	fTail = last;
	return *this;
}

BMessageList& BMessageList::Adopt(BMessageList &o)
{
	MakeEmpty();
	fHead = o.fHead;
	fTail = o.fTail;
	fCount = o.fCount;
	o.fHead = o.fTail = NULL;
	o.fCount = 0;
	return *this;
}

/*----------------------------------------------------------------*/

int32 BMessageList::EnqueueMessage(BMessage *msg)
{
	bigtime_t thisStamp = msg->When();
	if (thisStamp <= 0) {
		thisStamp = system_time();
		msg->SetWhen(thisStamp);
	}
	
	BMessage* pos = fTail;
	while (pos && pos->When() > thisStamp) {
		pos = pos->m_prev;
	}
	
	if (!InsertAfter(msg, pos)) {
		berr << "Error inserting message: " << *msg << endl;
	}
	
	return fCount;
}

BMessage* BMessageList::DequeueMessage(uint32 what, bool *more)
{
	BMessage* msg = fHead;
	if (what != B_ANY_WHAT) {
		while (msg && (what != msg->What())) {
			msg = msg->m_next;
		}
	}

	if (msg) Remove(msg);

	if (more) *more = fCount ? true : false;
	return msg;
}

bigtime_t BMessageList::OldestMessage() const
{
	if (fHead) return fHead->When();
	return B_INFINITE_TIMEOUT;
};

/*----------------------------------------------------------------*/

const BMessage* BMessageList::Head() const
{
	return fHead;
}

const BMessage* BMessageList::Tail() const
{
	return fTail;
}

const BMessage* BMessageList::Next(const BMessage* current) const
{
	return current->m_next;
}

const BMessage* BMessageList::Previous(const BMessage* current) const
{
	return current->m_prev;
}

/*----------------------------------------------------------------*/

bool BMessageList::AddHead(BMessage* message)
{
	if (message->m_next || message->m_prev) {
		return false;
	}
	message->m_next = fHead;
	if (fHead) fHead->m_prev = message;
	fHead = message;
	if (!fTail) fTail = message;
	fCount++;
	return true;
}

bool BMessageList::AddTail(BMessage* message)
{
	if (message->m_next || message->m_prev) {
		return false;
	}
	message->m_prev = fTail;
	if (fTail) fTail->m_next = message;
	fTail = message;
	if (!fHead) fHead = message;
	fCount++;
	return true;
}

bool BMessageList::InsertBefore(BMessage* message, const BMessage* position)
{
	if (!position) return AddTail(message);
	if (message->m_next || message->m_prev) {
		return false;
	}
	if (position->m_prev) {
		message->m_prev = position->m_prev;
		position->m_prev->m_next = message;
	} else {
		if (fHead != position) return false;
		fHead = message;
	}
	message->m_next = const_cast<BMessage*>(position);
	const_cast<BMessage*>(position)->m_prev = message;
	fCount++;
	return true;
}

bool BMessageList::InsertAfter(BMessage* message, const BMessage* position)
{
	if (!position) return AddHead(message);
	if (message->m_next || message->m_prev) {
		return false;
	}
	if (position->m_next) {
		message->m_next = position->m_next;
		position->m_next->m_prev = message;
	} else {
		if (fTail != position) return false;
		fTail = message;
	}
	message->m_prev = const_cast<BMessage*>(position);
	const_cast<BMessage*>(position)->m_next = message;
	fCount++;
	return true;
}

/*----------------------------------------------------------------*/

BMessage* BMessageList::RemoveHead()
{
	BMessage* ret = fHead;
	if (!ret) return NULL;
	fHead = ret->m_next;
	if (fTail == ret) fTail = NULL;
	if (fHead) fHead->m_prev = NULL;
	ret->m_next = ret->m_prev = NULL;
	fCount--;
	return ret;
}

BMessage* BMessageList::RemoveTail()
{
	BMessage* ret = fTail;
	if (!ret) return NULL;
	fTail = ret->m_prev;
	if (fHead == ret) fHead = NULL;
	if (fTail) fTail->m_next = NULL;
	ret->m_next = ret->m_prev = NULL;
	fCount--;
	return ret;
}

BMessage* BMessageList::Remove(const BMessage* message)
{
	if (message->m_next) {
		message->m_next->m_prev = message->m_prev;
	} else {
		if (fTail != message) {
			return NULL;
		}
		fTail = message->m_prev;
	}
	if (message->m_prev) {
		message->m_prev->m_next = message->m_next;
	} else {
		if (fHead != message) {
			return NULL;
		}
		fHead = message->m_next;
	}
	const_cast<BMessage*>(message)->m_next
		= const_cast<BMessage*>(message)->m_prev = NULL;
	fCount--;
	return const_cast<BMessage*>(message);
}

/*----------------------------------------------------------------*/

int32 BMessageList::CountMessages(uint32 what) const
{
#if DEBUG
	int32 i=0;
	const BMessage* m = Head();
	while (m) {
		m = Next(m);
		i++;
	}
	if (i != fCount) {
		bser << "BMessageList: count out of sync, have " << fCount
			 << " but should be " << i << endl;
	}
#endif

	if (what == B_ANY_WHAT) return fCount;
	
	int32 count = 0;
	BMessage *msg = fHead;
	while (msg) {
		if (what == msg->What()) count++;
		msg = msg->m_next;
	};
	
	return count;
}

bool BMessageList::IsEmpty() const
{
	return !fHead;
}

void BMessageList::MakeEmpty()
{
	BMessage* cur = fHead;
	while (cur) {
		BMessage* next = cur->m_next;
		cur->m_next = cur->m_prev = NULL;
		delete cur;
		cur = next;
	}
	fHead = fTail = NULL;
	fCount = 0;
}

/* ---------------------------------------------------------------- */

ITextOutput::arg operator<<(ITextOutput::arg io, const BMessageList& list)
{
	const BMessage* pos = list.Head();
	int32 index = 0;
	BString tmp;
	while (pos) {
		io << "Message #" << index << " (what=" << BTypeCode(pos->What())
		   << " at time=" << pos->When() << ")";
		pos = list.Next(pos);
		if (pos) io << endl;
		index++;
	}
	
	return io;
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

} }	// namespace B::Support2
