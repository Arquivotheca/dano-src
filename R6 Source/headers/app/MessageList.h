/******************************************************************************
/
/	File:			MessageList.h
/
/	Description:	A linked list of BMessage objects.
/
/	Copyright 2000, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _MESSAGE_LIST_H
#define _MESSAGE_LIST_H

#include <BeBuild.h>
#include <Locker.h>
#include <Message.h>	/* For convenience */

enum {
	B_ANY_WHAT = 0
};

/*----------------------------------------------------------------------*/
/*----- BMessageList class --------------------------------------------*/

class	BMessageList {
public:
						BMessageList();
						~BMessageList();

						BMessageList(const BMessageList &);
		BMessageList&	operator=(const BMessageList &);

/* Transfer ownership of messages from argument to this message list */
		BMessageList&	Adopt(BMessageList &);
		
/* Queue operations, sorted by time stamp
   (Enqueue stamps the message with system_time() if there is no time stamp) */
		int32			EnqueueMessage(BMessage *msg);
		BMessage*		DequeueMessage(uint32 what = B_ANY_WHAT, bool *more = NULL);
		bigtime_t 		OldestMessage() const;

/* Iterating over messages */
		const BMessage*	Head() const;
		const BMessage*	Tail() const;
		const BMessage*	Next(const BMessage* current) const;
		const BMessage*	Previous(const BMessage* current) const;
		
/* Adding messages */
		bool			AddHead(BMessage* message);
		bool			AddTail(BMessage* message);
		bool			InsertBefore(BMessage* message, const BMessage* position);
		bool			InsertAfter(BMessage* message, const BMessage* position);
		
/* Removing messages */
		BMessage*		RemoveHead();
		BMessage*		RemoveTail();
		BMessage*		Remove(const BMessage* message);

/* Operations on entire list */
		int32			CountMessages(uint32 what = B_ANY_WHAT) const;
		bool			IsEmpty() const;
		void			MakeEmpty();
		
/*----- Private or reserved -----------------------------------------*/

private:
		BMessage		*fHead;
		BMessage		*fTail;
		int32			fCount;
		uint32			_reserved[2];
};

BDataIO& operator<<(BDataIO& io, const BMessageList& list);

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _MESSAGE_QUEUE_H */


