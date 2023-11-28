// Troll.cpp
#include "Troll.h"
#include "PackageDB.h"
#include "PackageItem.h"
#include "VHTUtils.h"
#include "VHTConnection.h"
#include "Util.h"
#include "SettingsManager.h"
#include "SResIO.h"
//#include "UpdateWindow.h"

#define MODULE_DEBUG 0
#include "MyDebug.h"
#include <stdlib.h>


extern SettingsManager	*gSettings;

int32	Troll()
{
	PackageDB		pDB;
	status_t		pkerr;
	PackageItem		pkgItem;
	int32			successfulCount = 0;
	BMessage		report;
	VHTConnection	htconn;		// web server connection
	BMessenger		status;		// invalid messenger since no UI
	
	
	//// first read in the waiting report
	
	BFile	sFile(&gSettings->sEntry,O_RDONLY);
	pkerr = sFile.InitCheck();
	if (pkerr >= B_OK) {
		SResIO	resIO;
		pkerr = resIO.SetTo(&sFile, 'Rept', kNewReport);
		if (pkerr >= B_OK) {
			//ssize_t	size;
			pkerr = report.Unflatten(&resIO);
		}
	}
	
	/// now loop through the packages
	
	htconn.SetResourceRelative(kUpdateConnect);
	
	pDB.Rewind();	
	for (;;) {
		// read in the next package
		pkerr = pDB.GetNextPackage(&pkgItem,PackageDB::ALL_DATA);
		if (pkerr < B_OK)
			break;
		
		// we got a package -- determine if we should try to get updates
		PRINT(("Troll: Read package %s\n",pkgItem.data.FindString("package")));
		// first do they have a BeDepot developer serial number
		if (!pkgItem.ValetSupported())
			continue;
		PRINT(("Troll: Package is ValetSupported\n"));
		
		
		// do we have a serialID, prefixID combo
		const char *sid = pkgItem.data.FindString("sid");
		const char *pid = pkgItem.data.FindString("pid");
		const char *vid = pkgItem.data.FindString("vid");
		
		bool haveSerialID = (sid && *sid) && (pid && *pid) && (strcmp(sid,"old") != 0);
		bool haveVersionID = (vid && *vid) && (pid && *pid) && (*vid != '0');
		
		// we must have either serialID, prefixID or versionID/prefixID
		if ( !(haveSerialID || haveVersionID) ) {
			PRINT(("Troll: Don't have serialID or don't have prefix/versionID\n"));
			continue;
		}
		
		// empty out the string we don't need
		if (!haveSerialID) {
			sid = B_EMPTY_STRING;
		}
		else if (!haveVersionID) {
			vid = B_EMPTY_STRING;
		}
		
		// insert code to check for supported services here!!!
		PRINT(("Troll: Checking for supported services\n"));
		// flags we get back ---
		// register supported, update supported, anon-serial supported
		bool updateSupported = true;
		bool registerSupported = true;
		bool anonSerialSupported = false;
	
		// no serial number and anon not allowed
		if (!haveSerialID && !anonSerialSupported) {
			PRINT(("Troll: No serialID, no anon support, cannot update\n"));
			continue;
		}
		
		if (!updateSupported) {
			PRINT(("Troll: Updating not a supported service\n"));
			continue;
		}
		
		// either we have a serialID/prefixID or we have a versionID/prefixID
		// now we set-up the data to post to the server
		BMessage		postData;	// data to send
		BMessage		output;		// the translated list of updates
		BMessage		resp;		// output from the server
		
		postData.AddString("sid",sid);
		postData.AddString("pid",pid);
		postData.AddString("vid",vid);
		
		PRINT(("Troll: performing update check\n"));
		
		pkerr = PerformUpdateCheck(&postData,&pkgItem,pDB,htconn,status,&report);
		if (pkerr < B_OK) {
			// error checking for update
			
			// should classify errors
			if (pkerr != -3) {
				// network error, 
				continue;
			}
			else {
				//web server error, protocol error, package error
				break;
			}
		}
		successfulCount++;
	}
	
	
	
	if (successfulCount > 0)
	{
		// count the actual number of packages w/ reported upgrades
		type_code	type;
		report.GetInfo("upgradelist",&type,&successfulCount);
		
		// save the report in the new report area to be viewed later
		BFile	sFile(&gSettings->sEntry,O_RDWR);
		pkerr = sFile.InitCheck();
		if (pkerr >= B_OK) {
			SResIO	resIO;
			pkerr = resIO.SetTo(&sFile, 'Rept', kNewReport);
			if (pkerr >= B_OK) {
				ssize_t	size;
				pkerr = report.Flatten(&resIO, &size);
			}
		}
		if (pkerr >= B_OK) {
			PRINT(("successfulCount is %d\n",successfulCount));
			// report save successful
			BMessage msg('Rept');
			msg.AddInt32("count",successfulCount);
			be_app->PostMessage(&msg);
		}
	}
	
	return successfulCount;
}

#include "SettingsManager.h"
#include "UpgradeList.h"
#include "StatusDialog.h"

extern SettingsManager *gSettings;

/// the master updating function
status_t	PerformUpdateCheck(BMessage		*msg,
							PackageItem		*it,
							PackageDB		&db,
							HTConnection	&htconn,
							BMessenger		&response,
							BMessage		*newReport)
{
	// setup default messages
	BMessage	errMsg(F_STATUS_ERROR);
	BMessage	statMsg(F_STATUS_UPDATE);
	
	BMessage	resp;			// will contain the server response
	
	char		errbuf[256];	// for printing generic errors

	status_t res;
	
	// increment the status bar
	ReplaceFloat(&statMsg,"amount",0.5);
	ReplaceString(&statMsg,"message","Connecting to server...");
	response.SendMessage(&statMsg);
	
	// connect to the server
	if ((res = htconn.Connect()) < B_NO_ERROR) {
		sprintf(errbuf,"Could not connect %s",strerror(res));
		errMsg.AddString("message",errbuf);
		response.SendMessage(&errMsg);
		return -3;
	}

	// increment the status bar
	ReplaceFloat(&statMsg,"amount",1.5);
	ReplaceString(&statMsg,"message","Looking for newer versions...");
	response.SendMessage(&statMsg);
	
#if DEBUG
	msg->PrintToStream();
#endif	
	// send data to the server (sid, pid, vid)
	res = htconn.Post(msg);
	if (res < 200 || res > 210) {
		if (res < B_NO_ERROR)
			sprintf(errbuf,"Error posting data %s",strerror(res));
		else
			sprintf(errbuf,"Error posting data %d",res);			
		errMsg.AddString("message",errbuf);
		response.SendMessage(&errMsg);
		return -1;
	}
	
	// get the full response
	GetValetResponse(htconn, &resp);
	htconn.Close();
	
	PRINT(("\n\Server response\n\n"));
#if DEBUG
	resp.PrintToStream();
#endif	
	// if we got service codes back from the server, add them to the packageitem
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
		// send a message instead
		resp.what = F_STATUS_ERROR;
		response.SendMessage(&resp);
		//return resp.FindInt32("status");
		return -1;
	}
	
	// copy in the data merging eliminating duplicates
	// first constructor newList
	UpgradeItemList		newList;
	
	type_code type;
	int32 totalItems;
	int32 i;
	resp.GetInfo("versionid", &type, &totalItems);		
	for (i = 0; i < totalItems; i++) {
		BMessage foo;
		const char *name;
		int nm = 0;
		while (resp.GetInfo(B_ANY_TYPE, nm++, &name, &type) >= B_OK) {
			const void *p;
			ssize_t bytes;
			if (resp.FindData(name,type,i,&p,&bytes) >= B_OK) {
				//PRINT(("name %s, size %d\n",name,bytes));
				foo.AddData(name,type,p,bytes,type != B_STRING_TYPE);
			}
		}
		// adding message
		newList.AddUpgrade(&foo);
	}
	
	// we are generating a report
	// many efficiency improvements possible here
	// and at lower levels
	if (newReport) {
		PRINT(("Entering report generate\n"));
	
		type_code		type;
		int32			count;
		
		bool			itemsAdded = false;
		BMessage		newItemMsg;
		
		/// upgrade item list #2
		
		UpgradeItemList existingList;
		int32			existingListIndex = -1;
		
		newReport->GetInfo("package",&type,&count);
		for (int eix = 0; eix < count; eix++) {
			if (strcasecmp(newReport->FindString("package",eix),
						it->data.FindString("package")) == 0 &&
				strcasecmp(newReport->FindString("version",eix),
						it->data.FindString("version")) == 0)
			{
				// found the package in the existing report
				PRINT(("found the package in the report\n"));
				BMessage existingItems;
				newReport->FindMessage("upgradelist",eix,&existingItems);
				
				PRINT(("setting existing list\n"));
				existingList.SetTo(&existingItems);
				existingListIndex = eix;
				break;
			}
		}
		PRINT(("existing report index %d\n",existingListIndex));
		
		UpgradeItemList *oldList = it->Updates();		
		int32 totalItems = newList.CountItems();
		int32 totalOld = oldList->CountItems();
		PRINT(("%d new items, %d old items in registry\n",totalItems,totalOld));
		for (int nix = 0; nix < totalItems; nix++) {
			// looping through the new items
			int oix;
			UpgradeItem *nit = newList.ItemAt(nix);
			
			int64 nitVersID;
			nit->data.FindInt64("versionid",&nitVersID);
			
			for (oix = 0; oix < totalOld; oix++) {
				// looping through the old items
				UpgradeItem *oit = oldList->ItemAt(oix);
				int64 oldVersID;
				oit->data.FindInt64("versionid",&oldVersID);
				if (nitVersID == oldVersID)
				{
					break;
				}
			}
			
			if (oix >= totalOld) {
				// if item not in the registry then add to the existing report
				if (existingListIndex >= 0) {
					PRINT(("add item to existing report\n"));
					// create new item to add to the front of the list (mereging)
					existingList.AddUpgrade(new UpgradeItem(*nit),0);
				}
				else {
					PRINT(("add item to new report\n"));
					newItemMsg.AddMessage("upgradeitem",&nit->data);
				}
				itemsAdded = true;
			}
		}
		if (itemsAdded) {
			// find where it goes in the existing report
			if (existingListIndex >= 0) {
				// most likely bombed here
				
				PRINT(("## 1 ## might be about to bomb!!!\n"));
				
				existingList.Archive(&newItemMsg,true);
				newReport->ReplaceMessage("upgradelist",existingListIndex,&newItemMsg);
			}
			else {
				PRINT(("completing new report\n"));

				newReport->AddMessage("upgradelist",&newItemMsg);
				// report identifiers
				newReport->AddString("package",it->data.FindString("package"));
				newReport->AddString("version",it->data.FindString("version"));
				
				newReport->AddString("sid",it->data.FindString("sid"));
				newReport->AddString("pid",it->data.FindString("pid"));
			}
		}
	}
	// archive the listing for storage in PackageItem
	it->updates.MakeEmpty();
	// might bomb here
	
	PRINT(("## 2 ## might be about to bomb!!!\n"));
				
	newList.Archive(&it->updates,true);
	
	// update the progress bar
	ReplaceFloat(&statMsg,"amount",1.0);
	ReplaceString(&statMsg,"message",totalItems ?
									"New version/s successfully found" :
									"No new versions found");
	response.SendMessage(&statMsg);
	
	// now update stuff on disk
	if (resp.HasString("sid")) {
		// the server gave us a new serial number
		ReplaceString(&it->data,"sid",resp.FindString("sid"));
	}
	else {
		// the user may have typed in a new serial number
		ReplaceString(&it->data,"sid",msg->FindString("sid"));
	}
	if (resp.HasString("pid")) {
		// the server gave us a new prefix id
		ReplaceString(&it->data,"pid",resp.FindString("pid"));
	}
	
	// the update was was successful
	// we have already filled in the register data
	// now we mark the time we checked
	ReplaceInt32(&it->updates,"lastcheckdate",time(0));
	db.WritePackage(it,PackageDB::ALL_DATA);
	
	// need send a message telling the application that things have been updated
	BMessage	dis('PDis');
	dis.AddRef("refs",&it->fileRef);
	be_app->PostMessage(&dis);
	
	return 0;
}
