
#include "GMessageQueue.h"

GMessageQueue::GMessageQueue()
{
}

GMessageQueue::~GMessageQueue()
{
}

bool GMessageQueue::QueueMessage(BMessage *msg, bigtime_t thisStamp)
{
	if (thisStamp == -1) thisStamp = system_time();
	msg->SetWhen(thisStamp);
	m_list.EnqueueMessage(msg);
	return true;
}

bigtime_t GMessageQueue::OldestMessage()
{
	return m_list.OldestMessage();
};

BMessage * GMessageQueue::PeekMessage()
{
	return const_cast<BMessage*>(m_list.Head());
};

BMessage * GMessageQueue::DequeueMessage(uint32 what, bool *more, bigtime_t *when)
{
	BMessage* m = m_list.DequeueMessage(what, more);
	if (when) {
		*when = m ? m->When() : 0;
	}
	return m;
}

int32 GMessageQueue::CountMessages(uint32 what) const
{
	return m_list.CountMessages(what);
}

bool 
GMessageQueue::IsEmpty() const
{
	return m_list.IsEmpty();
}

