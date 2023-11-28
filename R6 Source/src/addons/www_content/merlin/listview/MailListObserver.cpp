/*
	MailListObserver.cpp
*/
#include "MailListObserver.h"
#include "PostOffice.h"


MailListObserver::MailListObserver(atomref<GHandler> target)
	:	SummaryObserver(),
		fTarget(target)
{

}


MailListObserver::~MailListObserver()
{
	//unregister ourselves
	SummaryContainer *summary = Wagner::PostOffice::MailMan()->LoadedSummaryContainer();
	if (summary != NULL)
		summary->RemoveObserver(this);
}

status_t MailListObserver::Update(uint32 event)
{
	if (fDirty) {
		fDirty = false;
		BMessage *message = new BMessage('sync');
		message->AddInt32("event", event);
		fTarget->PostMessage(message);
	}
}

// End of MailListObserver.cpp
