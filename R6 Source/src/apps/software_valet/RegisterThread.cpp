

#include "RegisterThread.h"
#include "ValetMessage.h"
#include "VHTUtils.h"
#include "PackageDB.h"
#include "PackageItem.h"
#include "SettingsManager.h"
#include "Log.h"
#include "Util.h"
#include <stdlib.h>

extern SettingsManager *gSettings;

#define MODULE_DEBUG 0
#include "MyDebug.h"

void LogFailedRegister(PackageItem *it, BMessage &resp);

void LogFailedRegister(PackageItem *it, BMessage &resp)
{
	Log	valetLog;
	BMessage	logMsg(Log::LOG_REGISTER);
	logMsg.AddString("pkgname",it->data.FindString("package"));
	char buf[80];
	const char *dat = resp.FindString("message");
	if (dat) {
		strncpy(buf,dat,80);
		buf[79] = 0;
		logMsg.AddString("strings",buf);
	}
	logMsg.AddString("strings","Registration Failed");
	valetLog.PostLogEvent(&logMsg);
}

// register data
// package data
RegisterThread::RegisterThread(BMessage *packages, BMessage *user, BMessenger &res)
	:	MThread("register net")
{
	if (packages)
		mpkg = *packages;
	if (user)
		muser = *user;
	response = res;
}

long RegisterThread::Execute()
{
	PackageDB	db;
	
	BMessage	errMsg(F_REG_ERROR);
	char		errbuf[256];
	status_t 	res;

	// set the url relative to the default server
	htconn.SetResourceRelative(kRegConnect);

	// loop through the message
	// it contains "items" a pointer to the PackageItem
	// as well as sid, vid, and pid
	type_code	type;
	int32		regCount;	
	mpkg.GetInfo("items",&type,&regCount);
	
	for (int32 ix = 0; ix < regCount; ix++)
	{
		BMessage	msg = muser;

		PackageItem *it;
		mpkg.FindPointer("items",ix,(void **)&it);
		if (!it)
			continue;
			
		// add prefixID (if any -- no prefixID needed if the server obtains it from version)
		msg.AddString("pid",mpkg.FindString("pid",ix));
		// add versionID (if any)
		msg.AddString("vid",mpkg.FindString("vid",ix));
		// add serialID (if any)
		msg.AddString("sid",mpkg.FindString("sid",ix));
		
		// update the progress bar
		response.SendMessage(F_REG_CONNECT);
		
		// connect to the server
		if ((res = htconn.Connect()) < B_NO_ERROR) {
			sprintf(errbuf,"Could not connect: %s.",strerror(res));
			errMsg.AddString("message",errbuf);
			response.SendMessage(&errMsg);
			return -3;
		}
		PRINT(("connected!\n"));	
		response.SendMessage(F_REG_SENDING);
		res = htconn.Post(&msg);
		if (res < 200 || res > 210) {
			if (res < B_NO_ERROR)
				sprintf(errbuf,"Error posting data: %s.",strerror(res));
			else
				sprintf(errbuf,"Error posting data: HTTP Error %d.",res);			
			errMsg.AddString("message",errbuf);
			response.SendMessage(&errMsg);

			LogFailedRegister(it, errMsg);			
			return -1;
		}
		
		BMessage resp;
		GetValetResponse(htconn, &resp);
		htconn.Close();
#if DEBUG		
		resp.PrintToStream();
#endif		
		// check services
		int32	upservice = -1;
		if (resp.HasString("upservice")) {
			upservice = atol(resp.FindString("upservice"));
			ReplaceBool(&it->data,"upservice",upservice);
		}
		int32	regservice = -1;
		if (resp.HasString("regservice")) {
			regservice = atol(resp.FindString("regservice"));
			ReplaceBool(&it->data,"regservice",regservice);
		}
		
		if (resp.FindInt32("status") < 0) {
			if (upservice >= 0 || regservice >= 0) {
				// got service values along with the error
				// so we need to update the package on disk
				db.WritePackage(it,PackageDB::BASIC_DATA);
				BMessage	disp('PDis');
				disp.AddRef("refs",&it->fileRef);
				be_app->PostMessage(&disp);
			}
			LogFailedRegister(it, resp);
			resp.what = F_REG_ERROR;
			response.SendMessage(&resp);
			return -1;
		}
		
		if (resp.HasString("sid")) {
			// the server gave us a new serial number
			ReplaceString(&it->data,"sid",resp.FindString("sid"));
		}
		else {
			// the user may have typed in a new serial number
			ReplaceString(&it->data,"sid",msg.FindString("sid"));
		}
		if (resp.HasString("pid")) {
			// the server gave us a new prefix id
			ReplaceString(&it->data,"pid",resp.FindString("pid"));
		}
		
		// the registration was successful
		// here we need to mark the package as registered and then write out to disk!!!
		ReplaceInt32(&it->data,"registered",PackageItem::REGISTERED_YES);
		db.WritePackage(it,PackageDB::BASIC_DATA);
		
		resp.what = F_REG_DONE;
		resp.AddRef("refs",&it->fileRef);
		response.SendMessage(&resp);
		
		// log the register
		{
			Log	valetLog;
			
			BMessage	logMsg(Log::LOG_REGISTER);
			logMsg.AddString("pkgname",it->data.FindString("package"));
			logMsg.AddString("strings","Registration Successful");
			valetLog.PostLogEvent(&logMsg);
		}
	}
	
	response.SendMessage(F_REG_ALLDONE);
	return 0;
}
