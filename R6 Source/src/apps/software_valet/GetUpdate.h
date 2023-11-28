#ifndef _GETUPDATE_H_
#define _GETUPDATE_H_


#include "StatusDialog.h"
#include "VHTConnection.h"
#include "MThread.h"

#include <Message.h>
#include <Messenger.h>

class GetUpdateThread : public MThread
{
public:
	GetUpdateThread(BMessage *inData, BMessenger &stat);
	
	VHTConnection	htconn;
protected:
	virtual long 	Execute();

private:
	BMessage		data;
	BMessenger		response;
};


class GetUpdateDialog : public StatusDialog
{
public:
	GetUpdateDialog(BMessage *inData);
	
	virtual void	MessageReceived(BMessage *);
private:
	GetUpdateThread	*thread;
};


#endif
