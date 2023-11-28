#ifndef _UPDATETHREAD_H_
#define _UPDATETHREAD_H_


#include "MThread.h"
#include "VHTConnection.h"
#include <Messenger.h>
#include <Message.h>

class UpdateThread : public MThread
{
public:
	UpdateThread(BMessenger &res, BMessage *);
	virtual long	Execute();

	VHTConnection	htconn;
private:
	static const char *connectString;
	BMessenger		response;
	BMessage		mpkg;	
};

#endif
