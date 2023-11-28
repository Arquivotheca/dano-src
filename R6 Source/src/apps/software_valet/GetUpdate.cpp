#include "GetUpdate.h"
#include "VHTUtils.h"
#include "Util.h"

#include <StatusBar.h>
#include <Messenger.h>
#include <Application.h>
#include <stdio.h>
#include <string.h>

status_t OpenWebBrowser(const char *arg);

GetUpdateDialog::GetUpdateDialog(BMessage *data)
	:	StatusDialog("Getting information for this update..."),
		thread(NULL)
{
	Lock();
	
	// set up the status bar
	BStatusBar *bar = (BStatusBar *)FindView("status");
	if (bar) {
		bar->SetMaxValue(3.0);
	}
	BMessenger tmp(this);
	thread = new GetUpdateThread(data,tmp);
	Unlock();
	Show();
	thread->Run();
}

void GetUpdateDialog::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case F_STATUS_ERROR: {
			thread = NULL;
			StatusDialog::MessageReceived(msg);
			break;
		}
		case B_CANCELED: {
			if (thread) {
				thread->htconn.Interrupt();
				thread = NULL;
				PostMessage(B_QUIT_REQUESTED);	
			}
			break;
		}
		default:
			if (msg->HasBool("quit") && msg->FindBool("quit"))
				thread = NULL;
			StatusDialog::MessageReceived(msg);
	}
}

GetUpdateThread::GetUpdateThread(BMessage *inData, BMessenger &stat)
	:	MThread(),
		response(stat),
		data(*inData)
{
}

long GetUpdateThread::Execute()
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

	htconn.SetResourceRelative(kGetUpdateConnect);
	if ((res = htconn.Connect()) < B_NO_ERROR) {
		sprintf(errbuf,"Could not connect. Error: %s",strerror(res));
		errMsg.AddString("message",errbuf);
		response.SendMessage(&errMsg);
		return -1;
	}

	res = htconn.Post(&data);
	if (res < 200 || res > 210) {
		if (res < B_NO_ERROR)
			sprintf(errbuf,"Error posting data. Error: %s",strerror(res));
		else
			sprintf(errbuf,"Error posting data. HTTP Error %d",res);
		errMsg.AddString("message",errbuf);
		response.SendMessage(&errMsg);
		//doError(errbuf);
		return -1;
	}
	ReplaceFloat(&statMsg,"amount",1.0);
	ReplaceString(&statMsg,"message","Requesting update...");
	response.SendMessage(&statMsg);
	
	
	BMessage	resp;
	
	GetValetResponse(htconn, &resp);
	htconn.Close();

#if DEBUG	
	resp.PrintToStream();
#endif
		
	if (resp.FindInt32("status") < 0) {
		resp.what = F_STATUS_ERROR;
		response.SendMessage(&resp);
		return -1;
	}

	statMsg.AddBool("quit",true);
	ReplaceFloat(&statMsg,"amount",2.0);
	ReplaceString(&statMsg,"message","Finished");
	response.SendMessage(&statMsg);

	const char *url = resp.FindString("url");
	if (url && *url) {
		// doError(resp.FindString("url"));
		if (resp.FindString("free")[0] == '1') {
			resp.what = 'Url ';
			be_app->PostMessage(&resp);
		}
		else {
			OpenWebBrowser(url);
		}
	}
	else {
		doError("Error, the server did not specify the URL where the file \
can be obtained. Please email custsupport@be.com so the problem can be fixed.");
	}

	return B_OK;
}
