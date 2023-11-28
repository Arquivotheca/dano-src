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

#include <MessageList.h>
#include <StreamIO.h>
#include <String.h>

#ifndef _DEBUG_H
#include <Debug.h>
#endif

#include <ctype.h>

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
			last->fNext = copy;
			copy->fPrevious = last;
		}
		last = copy;
		p = p->fNext;
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
	bigtime_t thisStamp;
	if (!msg->HasWhen()) {
		thisStamp = system_time();
		msg->SetWhen(thisStamp);
	} else {
		thisStamp = msg->When();
	}
	
	BMessage* pos = fTail;
	while (pos && pos->When() > thisStamp) {
		pos = pos->fPrevious;
	}
	
	if (!InsertAfter(msg, pos)) {
#if SUPPORTS_STREAM_IO
		BErr << "Error inserting message: " << *msg << endl;
#endif
	}
	
	return fCount;
}

BMessage* BMessageList::DequeueMessage(uint32 what, bool *more)
{
	BMessage* msg = fHead;
	if (what != B_ANY_WHAT) {
		while (msg && (what != msg->what)) {
			msg = msg->fNext;
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
	return current->fNext;
}

const BMessage* BMessageList::Previous(const BMessage* current) const
{
	return current->fPrevious;
}

/*----------------------------------------------------------------*/

bool BMessageList::AddHead(BMessage* message)
{
	if (message->fNext || message->fPrevious) {
		return false;
	}
	message->fNext = fHead;
	if (fHead) fHead->fPrevious = message;
	fHead = message;
	if (!fTail) fTail = message;
	fCount++;
	return true;
}

bool BMessageList::AddTail(BMessage* message)
{
	if (message->fNext || message->fPrevious) {
		return false;
	}
	message->fPrevious = fTail;
	if (fTail) fTail->fNext = message;
	fTail = message;
	if (!fHead) fHead = message;
	fCount++;
	return true;
}

bool BMessageList::InsertBefore(BMessage* message, const BMessage* position)
{
	if (!position) return AddTail(message);
	if (message->fNext || message->fPrevious) {
		return false;
	}
	if (position->fPrevious) {
		message->fPrevious = position->fPrevious;
		position->fPrevious->fNext = message;
	} else {
		if (fHead != position) return false;
		fHead = message;
	}
	message->fNext = const_cast<BMessage*>(position);
	const_cast<BMessage*>(position)->fPrevious = message;
	fCount++;
	return true;
}

bool BMessageList::InsertAfter(BMessage* message, const BMessage* position)
{
	if (!position) return AddHead(message);
	if (message->fNext || message->fPrevious) {
		return false;
	}
	if (position->fNext) {
		message->fNext = position->fNext;
		position->fNext->fPrevious = message;
	} else {
		if (fTail != position) return false;
		fTail = message;
	}
	message->fPrevious = const_cast<BMessage*>(position);
	const_cast<BMessage*>(position)->fNext = message;
	fCount++;
	return true;
}

/*----------------------------------------------------------------*/

BMessage* BMessageList::RemoveHead()
{
	BMessage* ret = fHead;
	if (!ret) return NULL;
	fHead = ret->fNext;
	if (fTail == ret) fTail = NULL;
	if (fHead) fHead->fPrevious = NULL;
	ret->fNext = ret->fPrevious = NULL;
	fCount--;
	return ret;
}

BMessage* BMessageList::RemoveTail()
{
	BMessage* ret = fTail;
	if (!ret) return NULL;
	fTail = ret->fPrevious;
	if (fHead == ret) fHead = NULL;
	if (fTail) fTail->fNext = NULL;
	ret->fNext = ret->fPrevious = NULL;
	fCount--;
	return ret;
}

BMessage* BMessageList::Remove(const BMessage* message)
{
	if (message->fNext) {
		message->fNext->fPrevious = message->fPrevious;
	} else {
		if (fTail != message) {
			return NULL;
		}
		fTail = message->fPrevious;
	}
	if (message->fPrevious) {
		message->fPrevious->fNext = message->fNext;
	} else {
		if (fHead != message) {
			return NULL;
		}
		fHead = message->fNext;
	}
	const_cast<BMessage*>(message)->fNext
		= const_cast<BMessage*>(message)->fPrevious = NULL;
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
		BSer << "BMessageList: count out of sync, have " << fCount
			 << " but should be " << i << endl;
	}
#endif

	if (what == B_ANY_WHAT) return fCount;
	
	int32 count = 0;
	BMessage *msg = fHead;
	while (msg) {
		if (what == msg->what) count++;
		msg = msg->fNext;
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
		BMessage* next = cur->fNext;
		cur->fNext = cur->fPrevious = NULL;
		delete cur;
		cur = next;
	}
	fHead = fTail = NULL;
	fCount = 0;
}

/* ---------------------------------------------------------------- */

// This is duplicated from BMessage; should be shared.
static inline int isident(int c)
{
	return isalnum(c) || c == '_';
}

static inline bool isasciitype(char c)
{
	if( c >= ' ' && c < 127 && c != '\'' && c != '\\' ) return true;
	return false;
}

static inline char makehexdigit(uint32 val)
{
	return "0123456789ABCDEF"[val&0xF];
}

static void appendhexnum(uint32 val, BString* out)
{
	const int32 base = out->Length();
	char* str = out->LockBuffer(base + 9);
	str += base;
	for( int32 i=7; i>=0; i-- ) {
		str[i] = makehexdigit( val );
		val >>= 4;
	}
	str[8] = 0;
	out->UnlockBuffer(base + 9);
}

static const char* TypeToString(type_code type, BString* out,
								bool fullContext = true,
								bool strict = false)
{
	const char c1 = (char)((type>>24)&0xFF);
	const char c2 = (char)((type>>16)&0xFF);
	const char c3 = (char)((type>>8)&0xFF);
	const char c4 = (char)(type&0xFF);
	bool valid;
	if( !strict ) {
		valid = isasciitype(c1) && isasciitype(c2) && isasciitype(c3) && isasciitype(c4);
	} else {
		valid = isident(c1) && isident(c2) && isident(c3) && isident(c4);
	}
	if( valid && (!fullContext || c1 != '0' || c2 != 'x') ) {
		char* s = out->LockBuffer(fullContext ? 7 : 5);
		int32 i = 0;
		if( fullContext ) s[i++] = '\'';
		s[i++] = c1;
		s[i++] = c2;
		s[i++] = c3;
		s[i++] = c4;
		if( fullContext ) s[i++] = '\'';
		out->UnlockBuffer(fullContext ? 6 : 4);
		return out->String();
	}
	
	if( fullContext ) *out = "0x";
	else *out = "";
	appendhexnum(type, out);
	
	return out->String();
}

BDataIO& operator<<(BDataIO& io, const BMessageList& list)
{
#if SUPPORTS_STREAM_IO
	const BMessage* pos = list.Head();
	int32 index = 0;
	BString tmp;
	while (pos) {
		io << "Message #" << index << " (what=" << TypeToString(pos->what, &tmp)
		   << " at time=" << pos->When() << ")";
		pos = list.Next(pos);
		if (pos) io << endl;
		index++;
	}
#else
	(void)list;
#endif

	return io;
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
