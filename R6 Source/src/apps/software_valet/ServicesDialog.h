#ifndef _SERVICESDIALOG_H_
#define _SERVICESDIALOG_H_


#include "StatusDialog.h"
#include "VHTConnection.h"
#include "MThread.h"
#include <Messenger.h>

class ServicesThread : public MThread
{
public:
	ServicesThread(BMessenger &stat);
	
	VHTConnection	htconn;
protected:
	virtual long 	Execute();

private:

	BMessenger		response;
};


class ServicesDialog : public StatusDialog
{
public:
	ServicesDialog();
	virtual void	MessageReceived(BMessage *);
private:
	ServicesThread	*thread;
};

#endif
