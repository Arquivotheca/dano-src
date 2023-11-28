// ArchiveScriptItem.cpp

#include "IArchiveScriptItem.h"
#include "InstallMessages.h"
#include "InstallPack.h"
#include "DestManager.h"
#include "PackAttrib.h"
#include "Util.h"

#include <unistd.h>
#include <sys/wait.h>
#include <Path.h>

#include "MyDebug.h"
#include <stdlib.h>
#include <string.h>

extern const char *errCONTINUE;
long handleInstallError(const char *format,
						const char *text,
						long errCode,
							// for logging
						DestManager *dests);
						
						

ArchiveScriptItem::ArchiveScriptItem()
	:	ArchiveFileItem()
{
}

// don't want to have replacement options
ArchiveScriptItem::~ArchiveScriptItem()
{

}

long	ArchiveScriptItem::ExtractItem(InstallPack *archiveFile, 
					BDirectory *parDir, 
					DestManager *dests, 
					ulong inGrps,
					bool skip)
{
	long err = B_NO_ERROR;
	BEntry 			outFile;
	BDirectory		*destDir;
	
	if (!(groups & inGrps)) {
		PRINT(("script %s not in group\n",name));
		return err;
	}
	if (skip) {
		BMessage skipMsg(M_UPDATE_PROGRESS);
		skipMsg.AddInt32("bytes",dests->AllocSize(uncompressedSize));
		archiveFile->statusMessenger.SendMessage(&skipMsg);
		return err;
	}
	PRINT(("extracting script %s\n",name));

	BMessage upMsg(M_CURRENT_FILENAME);
	
	upMsg.AddString("filename",name);
	archiveFile->statusMessenger.SendMessage(&upMsg);
	
	/// DO CANCEL CHECK HERE
	if (dests->canceled)
		return B_CANCELED;

	destDir = dests->DirectoryFor(destination,parDir,customDest);
	if (!destDir) {
		return handleInstallError(errNODESTDIR,name,err,dests);
	}
	
	const char *scrname = "tmpscript00001";
	{
		BFile		newfile;
		BDirectory	*tmpDir = dests->TempDir();
		
		err = tmpDir->CreateFile(scrname, &newfile);
		tmpDir->FindEntry(scrname,&outFile);
		// dests->TagFile(&outFile);
	}
	
	// fix error reporting
	if (err != B_NO_ERROR) {
		return handleInstallError(errMKFILE,name,err,dests);
	}
	
	bool oktoexec = true;
	//PRINT(("created new file calling extract\n"));	
	if (archiveFile->arcFlags & CD_INSTALL) {
		// PRINT(("copying file from CD-ROM\n"));	

		BMessage upMsg(M_UPDATE_PROGRESS);
		upMsg.AddInt32("bytes",uncompressedSize);
		archiveFile->statusMessenger.SendMessage(&upMsg);

		if (archiveFile->doCancel) {
			return B_CANCELED;
		}

		// CopyFile(
	}
	else {
		BFile	f(&outFile,O_RDWR);
		err = f.InitCheck();
		if (err >= B_OK)
			err = archiveFile->ExtractFile(&f,seekOffset);
		if (err == B_CANCELED) {
			outFile.Remove();
			return err;
		}
		if (err < B_NO_ERROR) {
						// reset decompression so we can continue if possible!
			archiveFile->ResetDecompress();
						// might want error codes here??
			return handleInstallError(errDECOMPRESS,name,err,dests);
		}
//		if (archiveFile->checksum != adler32) {
//			doError(errCHKSUM,name,errCONTINUE,NULL,NULL);
//			PRINT(("IN %d OUT %d\n",archiveFile->checksum,adler32));
//			oktoexec = false;
//		}
		archiveFile->ResetDecompress();
		
		off_t outSize;
		outFile.GetSize(&outSize);
		if (uncompressedSize != outSize) {
			doError(errBADSIZE,name,errCONTINUE,NULL,NULL);
			oktoexec = false;
		}
	}
	
	long extra = dests->AllocExtra(uncompressedSize);
	if (extra) {
		BMessage upMsg(M_UPDATE_PROGRESS);
		upMsg.AddInt32("bytes",extra);
		archiveFile->statusMessenger.SendMessage(&upMsg);
	}

	if (dests->logging) {
		char buf[B_FILE_NAME_LENGTH];
		sprintf(buf,"Executing shell script %s\n",name);
		dests->LogFile()->Write(buf,strlen(buf));
	}
	
	// execute script
	if (oktoexec) {
		PRINT(("ok to execute\n"));
		BPath	execpath;
		outFile.GetPath(&execpath);
		BPath	destPath(destDir, NULL, true);
		//BPath	oldPath(".",NULL, true);
			
		int		apipe[2];
		
		if (pipe(apipe) >= 0) {
			pid_t	pid = fork();
			
			if (pid == 0) {		// child
				// close read end of pipe
				close(apipe[0]);
					
				// normalized paths
				
				chdir(destPath.Path());
				dup2(apipe[1], STDOUT_FILENO);
				dup2(apipe[1], STDERR_FILENO);
				close(apipe[1]);

				execl(execpath.Path(),execpath.Path(),NULL);
				exit(0);
			}
			else if (pid > 0) {
				// close write end of pipe
				close(apipe[1]);
				
				const int bufsiz = 1024;
				int	amount;
				char buf[bufsiz];
				
				while ((amount = read(apipe[0], buf, bufsiz)) > 0) {
					if (dests->logging) {
						dests->LogFile()->Write(buf,amount);
					}
				}

				//while (wait(NULL) != pid)
				//	;
				
				int child_status;
				pid_t wait_status; 

				wait_status = waitpid(pid, &child_status, 0);
				close(apipe[0]);
				
				if (wait_status != pid) {
					child_status = -1;
				}
				
				if (child_status != 0 && dests->attr->abortScripts)
					err = B_ERROR;
			}
		}			
		// system(epath.Path());
		//chdir(oldPath.Path());
	}
	
	// log result	
	outFile.Remove();

	return err;
}

