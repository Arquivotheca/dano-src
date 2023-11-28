/******************************************************************************
/
/	File:			MessageQueue.h
/
/	Description:	BMessageQueue class creates objects that are used by
/					BLoopers to store in-coming messages.
/
/	Copyright 1995-98, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _MESSAGE_QUEUE_H
#define _MESSAGE_QUEUE_H

#include <BeBuild.h>
#include <Locker.h>
#include <Message.h>	/* For convenience */
#include <MessageList.h>

class BDataIO;

/*----------------------------------------------------------------------*/
/*----- BMessageQueue class --------------------------------------------*/

class	BMessageQueue {
public:
					BMessageQueue();
					BMessageQueue(const char* name);
virtual				~BMessageQueue();

/* Atomic queue manipulation and query */
		void		AddMessage(BMessage *an_event);
		
		bool		IsNextMessage(const BMessage* test) const;
		bigtime_t	NextTime() const;
		BMessage	*NextMessage();		// removes from queue
		
		bool		RemoveMessage(BMessage *an_event);
		int32		RemoveMessages(uint32 what, const BMessenger* target = NULL,
								   BMessageList* into = NULL);
		int32		CountMessages() const;
		bool		IsEmpty() const;

/* Operations that must be protected by a lock */
		BMessage	*FindMessage(int32 index) const;
		BMessage	*FindMessage(uint32 what, int32 index = 0) const;
		BMessage	*FindMessage(uint32 what, const BMessenger& target, int32 index = 0) const;
		
/* Queue locking */
		bool		Lock();
		void		Unlock();
		
		bool		IsLocked() const;
		void		UnlockFully();
		
/*----- Private or reserved -----------------------------------------*/

private:

virtual	void		_ReservedMessageQueue1();
virtual	void		_ReservedMessageQueue2();
virtual	void		_ReservedMessageQueue3();

friend	BDataIO&	operator<<(BDataIO& io, const BMessageQueue& queue);

					BMessageQueue(const BMessageQueue &);
		BMessageQueue &operator=(const BMessageQueue &);

		char		message_filter(BMessage *an_event);
		BMessage	*FindMessage(bool anyWhat, uint32 what, int32 index) const;

		BMessageList	fList;
		uint32			_reserved[1];
mutable	BLocker			fLocker;
};

BDataIO& operator<<(BDataIO& io, const BMessageQueue& queue);

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _MESSAGE_QUEUE_H */


