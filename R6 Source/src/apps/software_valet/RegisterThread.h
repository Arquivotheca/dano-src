#ifndef _REGISTERTHREAD_H_
#define _REGISTERTHREAD_H_

#include "MThread.h"
#include "VHTConnection.h"
#include <Message.h>
#include <Messenger.h>


class RegisterThread : public MThread
{
public:
	RegisterThread(BMessage *,
					BMessage *,
					BMessenger &res);
	virtual long	Execute();
private:
	friend class RegisterView;
	BMessage		muser;
	BMessage		mpkg;
	BMessenger		response;
	
	VHTConnection	htconn;
};

#endif

