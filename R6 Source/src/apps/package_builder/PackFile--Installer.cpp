#include <Be.h>
#include "PackArc.h"

//#include "FArchiveItem.h"
//#include "ArchivePatchItem.h"

#include "PackMessages.h"
#include <Path.h>
#include <NodeInfo.h>
//#include "GroupList.h"
//#include "DestinationList.h"
//#include "Attributes.h"
#include "InstallDefs.h"

#include "Util.h"
#include "MyDebug.h"


// not necessarily the ideal location for this function...

long	CopyFile(BEntry *dst, BEntry *src, BMessenger msg);
long	CopyBytes(BFile *dst, BFile *src, long count);

long PackArc::BuildInstaller(entry_ref &dirRef, const char *name, bool sea)
{
	long			err;
	// long			bytes;
	
	BFile			*instFile;
	BEntry			instEnt;
	BResources		*instRes;
	
	BDirectory		destDir(&dirRef);

	BEntry			installerStubFile;
	if (sea) {	
		BEntry			appEnt;
		BDirectory		appDirectory;
		
		
		app_info		info;
		
		be_app->GetAppInfo(&info);
		appEnt = BEntry(&info.ref);
		appEnt.GetParent(&appDirectory);
		err = appDirectory.FindEntry("InstallerStub",&installerStubFile);
		if (err != B_NO_ERROR) {
			doError("The installer file could not be found. Please make sure the file\
	 \"InstallerStub\" is in the same folder as the Package Builder application");
			return err;
		}
	}
	PRINT(("\n\npack file is building the installer... "));
	
	instFile = new BFile();
	destDir.CreateFile(name,instFile);
	destDir.FindEntry(name,&instEnt);
	
	if (sea) {
		CopyFile(&instEnt,&installerStubFile,statusMessenger);
	
		// instEnt
		// set mime
		BPath path;
		instEnt.GetPath(&path);
		// important to do this syncronous here
		// false, true, false = non-recursive, syncronous, no force
		update_mime_info(path.Path(), false, true, false);

		instFile->SetTo(&instEnt,O_RDWR);
		instRes= new BResources(instFile);
		// write resource to get correct size
		int32 installerSize = 0;
		err = instRes->AddResource(SIZE_RES_TYPE,0,&installerSize,sizeof(int32),"");
		
		if (err != B_NO_ERROR) {
			doError("Error adding resources to the self-extracting file");
			delete instRes;
			delete instFile;
			return err;
		}
		//// close for flush
		//instFile.Close();
		delete instRes;
	}
	delete instFile;
	
	instFile = new BFile(&instEnt,O_RDWR);
		
	if (sea) {
		off_t sz;
		
		instFile->GetSize(&sz);
		int32 installerSize = 0;
		installerSize = sz;
		// write total size into resource
		instRes= new BResources(instFile);
		
		// instFile.Open(B_READ_WRITE);
		// fill in size
		PRINT(("installerSize is %d\n", installerSize));
		installerSize = B_HOST_TO_BENDIAN_INT32(installerSize);
		err = instRes->WriteResource(SIZE_RES_TYPE,0,&installerSize,0,sizeof(installerSize));
		if (err != B_NO_ERROR) {
			doError("Error writing resources to the self-extracting file");
			delete instRes;
			delete instFile;
			return err;
		}
		delete instRes;
	}
	
	// copy everything
	SetFile(O_RDONLY);
	instFile->Seek(0,SEEK_END);
	
	off_t csize;
	fArcFile->GetSize(&csize);
	CopyBytes(instFile,fArcFile,csize);
	
	ClearFile();
	delete instFile;

	// set executable
	BNode	node(&instEnt);

	if (sea) {
		// get umask
		mode_t mode = umask(0);
		umask(mode);	
		node.SetPermissions(mode | S_IXUSR | S_IXGRP);
	}
	else {
		BNodeInfo	ninf(&node);
		ninf.SetType("application/x-scode-UPkg");
	}
	
	return B_NO_ERROR;
}

#if 0
long PackFile::WriteInstallerData(long bytes,BFile *resFile)
{
	long err;
	long readAmount = INBUFSIZ;
	while (bytes > 0) {
		if (bytes < INBUFSIZ) {
			readAmount = bytes;
		}
		err = Read(inBuf,readAmount);
		if (err != readAmount || err == B_ERROR) {
			doError("Fatal error reading package file");
			return err;
		}
		bucket -= readAmount;
		while (bucket < 0) {
			bucket += byteUpdateSize;
			BMessage upMsg(M_UPDATE_PROGRESS);
			upMsg.AddLong("bytes",byteUpdateSize);
			statusMessenger.SendMessage(&upMsg);
		}
		err = resFile->Write(inBuf,readAmount);
		if (err != readAmount || err == B_ERROR) {
			doError("Fatal error writing to installer file");
			return err;
		}
		bytes -= readAmount;
		
		// update status bar!!!
	}
	return bytes;
}

// use exceptions
long PackFile::CopyInstallerAttributes(BFile &instFil)
{
	long	headErr;
	long	err;
	AttribHeader	head;
			
	do {
		headErr = Read(&head,sizeof(head));
		if (headErr < B_NO_ERROR)
			break;
		
		switch (head.code) {
			case A_LICENSE:
			default:
				// copy it
				instFil.Write(&head,sizeof(head));
				uchar	*aData = (uchar *)malloc(head.size);
				Read(aData,head.size);
				instFil.Write(aData,head.size);
				free(aData);
				break;
		}
	} while (headErr == sizeof(AttribHeader) && head.code != A_LAST_ATTRIBUTE);
	
	err = Close();

	return err;	
}

#endif
