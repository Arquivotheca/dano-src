#include "ServicesDialog.h"
#include "Util.h"
#include <StatusBar.h>

ServicesDialog::ServicesDialog()
	:	StatusDialog("Now checking the server to determine if\
 updating and registration services are supported for this software..."),
		thread(NULL)
{
	Lock();
	
	// set up the status bar
	BStatusBar *bar = (BStatusBar *)FindView("status");
	if (bar) {
		bar->SetMaxValue(3.0);
	}
	BMessenger tmp(this);
	thread = new ServicesThread(tmp);
	Unlock();
	Show();
	thread->Run();
}

void ServicesDialog::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_CANCELED: {
			if (thread) {
				thread->htconn.Interrupt();
				PostMessage(B_QUIT_REQUESTED);	
			}
			break;
		}
		default:
			StatusDialog::MessageReceived(msg);
	}
}

ServicesThread::ServicesThread(BMessenger &stat)
	:	MThread(),
		response(stat)
{
}

long ServicesThread::Execute()
{
	BMessage	errMsg(F_STATUS_ERROR);
	BMessage	statMsg(F_STATUS_UPDATE);
	
	char		errbuf[256];

	status_t res;
	res;
	errbuf;

	ReplaceFloat(&statMsg,"amount",0.5);
	ReplaceString(&statMsg,"message","Connecting to server...");
	response.SendMessage(&statMsg);
	snooze(1000*1000);
	/****
	if ((res = htconn.Connect(kServicesConnectString)) < B_NO_ERROR) {
		sprintf(errbuf,"Could not connect %s",strerror(res));
		PRINT((errbuf));
		PRINT(("\n"));
		errMsg.AddString("message",errbuf);
		response.SendMessage(&errMsg);
		return -1;
	}
	****/
	
	ReplaceFloat(&statMsg,"amount",1.0);
	ReplaceString(&statMsg,"message","Checking supported services...");
	response.SendMessage(&statMsg);
	
	snooze(500*1000);
	
	ReplaceFloat(&statMsg,"amount",0.75);
	ReplaceString(&statMsg,"message","Registration is supported...");
	response.SendMessage(&statMsg);
	
	snooze(500*1000);

	ReplaceFloat(&statMsg,"amount",0.75);
	ReplaceString(&statMsg,"message","Updating is supported...");
	response.SendMessage(&statMsg);
	
	snooze(500*1000);

	statMsg.AddBool("quit",true);
	ReplaceString(&statMsg,"message","Finished");
	response.SendMessage(&statMsg);
	
	snooze(500*1000);
	
	/****
	res = htconn.Post(data);
	if (res < 200 || res > 210) {
		if (res < B_NO_ERROR)
			sprintf(errbuf,"Error posting data %s",strerror(res));
		else
			sprintf(errbuf,"Error posting data %d",res);			
		errMsg.AddString("message",errbuf);
		response.SendMessage(&errMsg);
		return -1;
	}
	
	BMessage	resp;
	
	GetValetResponse(htconn, &resp);
	
	if (resp.FindInt32("status") < 0) {
		//// send a message instead
		if (resp.FindString("message")) {
			resp.what = F_STATUS_ERROR;
			response.SendMessage(&resp);
			// return -1;
		}
		else {
			errMsg.AddString("message","Fatal protocol error.\
 SoftwareValet received an unexpected response from the server.");
			response.SendMessage(&errMsg);
		}
	}
	else {
		resp.PrintToStream();

	}
	****/
		
	htconn.Close();

	return 0;
}
