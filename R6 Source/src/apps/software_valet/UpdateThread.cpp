#include "UpdateThread.h"
#include "Troll.h"
#include "StatusDialog.h"
#include "ValetMessage.h"
#include "VHTUtils.h"
#include "PackageItem.h"
#include "PackageDB.h"
#include "Util.h"

#define MODULE_DEBUG 0
#include "MyDebug.h"
	

UpdateThread::UpdateThread(BMessenger &res, BMessage *dat)
	:	MThread("register net")
{
	response = res;		// status
	//display = disp;		// display the packages
	
	mpkg = *dat;
}

long UpdateThread::Execute()
{
	PackageDB	db;
	PRINT(("update execute!!!\n"));
	
	status_t 	res;
	type_code	type;
	int32		pkgCount;
	
	// we have been passed a data message containing lists of
	// recordref ("refs"), sid, pid, vid
	// each index represents a package
	
	// set the http address relative to our base server
	htconn.SetResourceRelative(kUpdateConnect);
	
	// loop through the packages
	mpkg.GetInfo("refs",&type,&pkgCount);
	for (int32 ix = 0; ix < pkgCount; ix++)
	{
		BMessage	msg;
		
		PackageItem	pitem;
		PackageItem *it = &pitem;
		
		// read in the package
		mpkg.FindRef("refs",ix,&it->fileRef);
		BEntry	ent(&it->fileRef);
		if (db.ReadPackage(it,&ent,PackageDB::BASIC_DATA) < B_OK)
			continue;
		
		// copy in the data to send up
		// add prefixID (if any -- no prefixID needed if the server obtains it from version)
		msg.AddString("pid",mpkg.FindString("pid",ix));
		// add versionID (if any)
		msg.AddString("vid",mpkg.FindString("vid",ix));
		// add serialID (if any)
		msg.AddString("sid",mpkg.FindString("sid",ix));
		
		// send the pid,vid,sid combo as well as the package item
		// to the master function
		res = PerformUpdateCheck(&msg,it,db,htconn,response,NULL);
		
		// keep going or not??
		
	}
	// all out updating is done close the window
	// close the status window
	// we need to make sure that the status bar amounts match
	BMessage	closeMsg(F_STATUS_UPDATE);
	closeMsg.AddBool("quit",true);
	response.SendMessage(&closeMsg);
	
	return 0;
}
