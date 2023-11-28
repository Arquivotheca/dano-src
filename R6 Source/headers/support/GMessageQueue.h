
#ifndef _G_MESSAGE_QUEUE_H
#define _G_MESSAGE_QUEUE_H

#include <BeBuild.h>
#include <Message.h>
#include <MessageList.h>
#include <Locker.h>
#include <Gehnaphore.h>

#define ANY_WHAT 0

/*----------------------------------------------------------------------*/
/*----- GMessageQueue class --------------------------------------------*/

class GMessageQueue {
public:
					GMessageQueue();	
					~GMessageQueue();

		bool		QueueMessage(BMessage *msg, bigtime_t when=-1);
		BMessage *	DequeueMessage(uint32 what = ANY_WHAT, bool *more = NULL, bigtime_t *when = NULL);
		BMessage *	PeekMessage();
		bigtime_t 	OldestMessage();
		int32		CountMessages(uint32 what) const;
		bool		IsEmpty() const;

private:

		BMessageList	m_list;
		uint32			m_reserved[6];
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _G_MESSAGE_QUEUE_H */
