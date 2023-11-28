/*
	MailListObserver.h
*/
#ifndef _MAIL_LIST_OBSERVER_H
#define _MAIL_LIST_OBSERVER_H
#include <GHandler.h>
#include "SummaryContainer.h"

class MailListObserver : public SummaryObserver {
	public:
								MailListObserver(atomref<GHandler> target);
								~MailListObserver();
		
		virtual status_t		Update(uint32 event);
	
	private:
		atomref<GHandler> fTarget;
};

#endif
