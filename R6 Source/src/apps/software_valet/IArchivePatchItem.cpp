// ArchivePatchItem.cpp
#include "IArchivePatchItem.h"
#include "InstallerType.h"

#if DOES_PATCHING

#include "InstallPack.h"
#include "DestManager.h"
#include "InstallMessages.h"
#include "zlib.h"

#include "MyDebug.h"
#include "Util.h"
#include <Path.h>
#include <NodeInfo.h>
#include <string.h>



#include "AutoPtr.h"


ArchivePatchItem::ArchivePatchItem()
	: ArchiveFileItem()
{
	fType = NULL;
}

ArchivePatchItem::ArchivePatchItem(const ArchivePatchItem &item)
{
	oldFileSize = item.oldFileSize;
	oldCreationTime = item.oldCreationTime;
	oldModTime = item.oldModTime;
	fileRef = item.fileRef;
	
	groups = item.groups;
	destination = item.destination;
	customDest = item.customDest;
	replacement = item.replacement;
	uncompressedSize = item.uncompressedSize;
	creationTime = item.creationTime;
	modTime = item.modTime;
	platform = item.platform;
	seekOffset = item.seekOffset;	
	mode = item.mode;
	fType = NULL;
	
	free(name);
	name = strdup(item.name);
	
	memcpy(versionInfo, item.versionInfo, sizeof(versionInfo));
}

long ArchivePatchItem::ExtractItem(InstallPack *archiveFile,
									BDirectory *parDir,
									DestManager *dests,
									ulong inGrps,
									bool skip)
{
	parDir;
	
	// extract compressed patch to temp file
	// check original item
	// apply patch to original item (safely)
	
	// remove original item if desired
	long	err = B_NO_ERROR;
	
	if (!(groups & inGrps)) {
		PRINT(("patch %s not in group\n",name));
		return err;
	}
	if (skip) {
		BMessage skipMsg(M_UPDATE_PROGRESS);
		skipMsg.AddInt32("bytes",dests->AllocSize(uncompressedSize));
		archiveFile->statusMessenger.SendMessage(&skipMsg);
		return err;
	}
	PRINT(("extracting patch %s\n",name));

	// thread safety not critical, wrong!!
	static int fcount = 0;
	char tname[B_FILE_NAME_LENGTH];
	
	BDirectory *tmp = dests->TempDir();
	BFile		outFile;
	BEntry		outEntry;
	
	sprintf(tname,"tmpatch_0%d",fcount++);
	tmp->CreateFile(tname,&outFile);
	if (tmp->InitCheck() < B_NO_ERROR) {
		doError(errPATCHTMP);
		return B_ERROR;
	}
	tmp->FindEntry(tname,&outEntry);
	
	/*****	
	* extract compressed patch
	******/
	PRINT(("extracting patch file %s\n",name));

	BMessage upMsg(M_CURRENT_FILENAME);

	upMsg.AddString("mode","Patching: ");	
	upMsg.AddString("filename",name);
	archiveFile->statusMessenger.SendMessage(&upMsg);


	// updates bytes in progress bar for uncompressed size
	//	(ie the uncompressed size of the patch data)
	// we don't know the compressed size of the data!
	BFile	f(&outEntry,O_RDWR);
	err = f.InitCheck();
	if (err >= B_OK)
		err = archiveFile->ExtractFile(&f,seekOffset);
	if (err == B_CANCELED) {
		outEntry.Remove();
		return err;
	}
	if (err < B_NO_ERROR) {
		doError(errPATCHDCMP,name);
		outEntry.Remove();
		return err;
	}
	//if (archiveFile->checksum != adler32) {
	//	PRINT(("cur checksum %d, saved csum %d\n",
	//			archiveFile->cstrm.c_stream.adler, adler32));
	//	doError(errPATCHCKSUM,name);
		// should bail out!!
	//}
	archiveFile->ResetDecompress();
#if 0
	if (uncompressedSize != outFile.Size()) {
		doError(errPATCHBADSIZE,name);
	}
	// sizes won't match
#endif

	long extra = dests->AllocExtra(uncompressedSize);
	if (extra) {
		BMessage upMsg(M_UPDATE_PROGRESS);
		upMsg.AddInt32("bytes",extra);
		archiveFile->statusMessenger.SendMessage(&upMsg);
	}

	// difference between patch size and new output file
	// for progress bar
	// nice HACK!
	off_t outsz;
	outEntry.GetSize(&outsz);
	long remainingBytes = uncompressedSize - outsz;
	if (remainingBytes < 0)
		remainingBytes = 0;
	gLastVal = 0;

	// find original item
	
	BEntry		originalEntry(&fileRef);
	BFile		newFile;
	BEntry		newEntry;
	BPath		oPath, nPath, dPath;
	BDirectory	container;
	
	originalEntry.GetParent(&container);	
	originalEntry.GetPath(&oPath);
	outEntry.GetPath(&dPath);
	
	PRINT(("patch path is %s\n",dPath.Path()));
	
	char newName[B_FILE_NAME_LENGTH];
	sprintf(newName,"patchedfile_0%d",fcount);
	// PathForFile(&originalFile,oldFilName,bigPathSz);
	err = container.CreateFile(newName,&newFile);
	if (err == B_NO_ERROR) {
		PRINT(("new file successfully created\n"));
		
		container.FindEntry(newName,&newEntry);
		newEntry.GetPath(&nPath);
		
		// PathForFile(&newFile,newFilName,bigPathSz);
	
		PRINT(("applying patch, npath is %s, opath is %s\n",nPath.Path(),oPath.Path()));
		theArchiveFile = archiveFile;
			
		err = ApplyPatch(oPath.Path(),nPath.Path(),dPath.Path(),remainingBytes);
	
		if (err == B_NO_ERROR) {
			status_t nameErr = B_NO_ERROR;
			
			BEntry	sameNamed;
			
			// check if there is a rename conflict
			if (!container.FindEntry(name,&sameNamed)) {
				// safe rename in case
				bool setOrig = (originalEntry == sameNamed);
				nameErr = RenameExisting(&container,&sameNamed,".old");
				if (setOrig) originalEntry = sameNamed;
			}
			if (nameErr == B_NO_ERROR) {
				// rename the patch from temp to correct name
				nameErr = newEntry.Rename(name);
			}
			if (nameErr < B_NO_ERROR) {
				doError(errPATCHRENAME);
				newEntry.Remove();
			}
			else {
				if (dests->deleteOriginals) {
					// delete original file
					originalEntry.Remove();
				}

				ISetType(&newEntry);
				if (dests->logging) {
					char buf[B_FILE_NAME_LENGTH];
					
					WritePath(&container,dests->LogFile(),buf);
					sprintf(buf,"%s\n\tSize: %Ld bytes",name,uncompressedSize);
					dests->LogFile()->Write(buf,strlen(buf));
					if (fType && *fType)
						sprintf(buf,"\t\tType: %s\n",fType);
					else
						sprintf(buf,"\n");
					dests->LogFile()->Write(buf,strlen(buf));
					
					//if (version) {
					//	ParseVersion(version,buf);
					//	dests->LogFile()->Write(buf,strlen(buf));
					//}
					//else {
					//	buf[0] = '\n';
					//	dests->LogFile()->Write(buf,1);
					//}
					
					sprintf(buf,"\tPatched from original: ");
					dests->LogFile()->Write(buf,strlen(buf));
					dests->LogFile()->Write(oPath.Path(),strlen(oPath.Path()));
					buf[0] = '\n';
					dests->LogFile()->Write(buf,1);
				}
			}
		}
		else {
			// error occurred, delete the new file if patch failed
			newEntry.Remove();
			// container.Remove(&newFile);
		}
	}
	else {
		doError(errPATCHTMP); 
	}
	outEntry.Remove();
	
	return err;
}

void ArchivePatchItem::AdjustProgressBar(long curVal)
{
	curVal = (long)(curVal*progressAdjust);

  	// int pos;
	if (curVal < gLastVal)
  		gLastVal = 0;
	long delta = curVal - gLastVal;
  
	// delta is fed to the be progress bar
	BMessage updt(M_UPDATE_PROGRESS);
	updt.AddInt32("bytes",delta);
	theArchiveFile->statusMessenger.SendMessage(&updt);

	gLastVal = curVal;
}

long ArchivePatchItem::ApplyPatch(const char *orgFilNam,
								  const char *derivedFilNam,
								  const char *diffFilNam,
								  long remainBytes)
{
	FILE		*pOrgFil;
	FILE		*pDerivedFil;
	FILE		*pDiffFil;
	int			c;
//	TStr		str;
	long		blockLen;
	long		blockPos;
	int			tag;
	long		derivedSize;
	ulong		derivedSum;
	long		orgSize;
	ulong		orgSum;
	long		checkDerivedSize;
	ulong		checkDerivedSum;
	long		checkOrgSize;
	ulong		checkOrgSum;
	long		prevDiffPos;
	long		diffPos;
//	long		curPos;
	ulong		magicNum;
	AutoPtr<char>		cbuf(COPY_BUF_SIZ);

	/////////////
	copyBuf = cbuf.Value();
	if (!copyBuf) {
		doError(errNOCOPYBUF);
		return B_ERROR;
	}

	// open the temp file
	pDiffFil = fopen(diffFilNam,"rb");

	if (pDiffFil == NULL)
	{
		doError(errPATCHOPEN);
		return B_ERROR;
	}
	magicNum = ReadLongNBytes(pDiffFil,4);	
	

	/*
	* Read sizes and checksums of original and updated file
	*/
	checkOrgSize = ReadLongNBytes(pDiffFil,4);
	checkOrgSum = ReadLongNBytes(pDiffFil,4);
	orgSize = 0;
	orgSum = 0;
	checkDerivedSize = ReadLongNBytes(pDiffFil,4);
	checkDerivedSum = ReadLongNBytes(pDiffFil,4);
	derivedSize = 0;
	derivedSum = 0;

	{
		/*
		* Check file header
		*/
		if (magicNum != PATCH_MAGIC_NUM)
		{
			doError(errPATCHHEAD);
			fclose(pDiffFil);
			return B_ERROR;
		}

		pOrgFil = fopen(orgFilNam,"rb");
		if (pOrgFil == NULL)
		{
			doError(errPATCHOPENEXISTING);
			fclose(pDiffFil);
			return B_ERROR;
		}

		pDerivedFil = fopen(derivedFilNam,"wb");
		if (pDerivedFil == NULL)
		{
			doError(errPATCHOPENNEW);
			fclose(pDiffFil);
			fclose(pOrgFil);
			return B_ERROR;
		}

		orgSize = FileSize(pOrgFil);

		// Checksum stuff
		
		// InitProgressBar(0,FileSize(pOrgFil),"Checking original:");

		/*
		curPos = 0;
		while ((c = getc(pOrgFil)) != EOF)
		{
			// if ((curPos & 0xFFF) == 0) AdjustProgressBar(curPos);
			orgSum += c;
			curPos++;
		}
		*/
		long count = orgSize;
		while (count) {
			long amount = min_c(count,COPY_BUF_SIZ);
			long res = fread(copyBuf,sizeof(char),amount,pOrgFil);		
			if (res != amount) {
				break;
			}
			// loop over buffer for checksum
			orgSum = crc32(orgSum, (uchar *)copyBuf,amount);
			
			count -= amount;
		}
		
		
		
		fseek(pOrgFil,0L,SEEK_SET);
		// CloseProgressBar();

		off_t diffSize = FileSize(pDiffFil);
		// InitProgressBar(0,FileSize(pDiffFil),"Applying patch:");
		progressAdjust = ((float)remainBytes/(float)diffSize);
		PRINT(("PROGADJUST IS %f\n",progressAdjust));
		
		// progressAdjust = 1.0;
		
		tag = 0;
		prevDiffPos = 0;
		diffPos = 0;
		while (tag != cTagEOF && (c = getc(pDiffFil)) != EOF) {
			diffPos++;
			/*
			* Adjust bar every 2048 bytes
			*/
			if (diffPos - prevDiffPos > 4096) {
				AdjustProgressBar(diffPos);
				prevDiffPos = diffPos;
				
				// CHECK CANCEL
			}
			
			tag = c & 0x0F;
			switch (tag) {
				case cTagSmallDiff:
					blockLen = (c >> 4) + 1;
					CopyFileChars(blockLen,pDiffFil,pDerivedFil,&derivedSum);
					diffPos += blockLen;
					derivedSize += blockLen;
					break;
				case cTagMediumDiff:
					blockLen = (c >> 4);
					c = getc(pDiffFil);
					diffPos++;
					if (c != EOF) {
						blockLen = ((blockLen << 8) | c) + cSmallSize + 1;
						CopyFileChars(blockLen,pDiffFil,pDerivedFil,&derivedSum);
						diffPos += blockLen;
						derivedSize += blockLen;
					}
					break;
				case cTagLargeDiff:
					blockLen = (c >> 4);
					c = getc(pDiffFil);
					diffPos++;
					if (c != EOF) {
						blockLen = (blockLen << 8) | c;
						c = getc(pDiffFil);
						diffPos++;
						if (c != EOF) {
							blockLen = ((blockLen << 8) | c) + cMediumSize + 1;
						}
						CopyFileChars(blockLen,pDiffFil,pDerivedFil,&derivedSum);
						diffPos += blockLen;
						derivedSize += blockLen;
					}
	  				break;
	  			case cTagSmallNearCopy:
	  				blockLen = (c >> 4) + 1;
	  				blockPos = ReadLongNBytes(pDiffFil,2);
	  				diffPos += 2;
	  				fseek(pOrgFil,blockPos,SEEK_SET);
	  				CopyFileChars(blockLen,pOrgFil,pDerivedFil,&derivedSum);
	  				derivedSize += blockLen;
	  				break;
	  			case cTagMediumNearCopy:
	  				blockLen = (c >> 4);
	  				c = getc(pDiffFil);
	  				diffPos++;
	  				if (c != EOF) {
	  					blockLen = ((blockLen << 8) | c) + cSmallSize + 1;
	  					blockPos = ReadLongNBytes(pDiffFil,2);
	  					diffPos += 2;
	  					fseek(pOrgFil,blockPos,SEEK_SET);
	  					CopyFileChars(blockLen,pOrgFil,pDerivedFil,&derivedSum);
	  					derivedSize += blockLen;
	  				}
	  				break;
				case cTagLargeNearCopy:
					blockLen = (c >> 4);
					c = getc(pDiffFil);
					diffPos++;
					if (c != EOF) {
						blockLen = (blockLen << 8) | c;
						c = getc(pDiffFil);
						diffPos++;
						if (c != EOF) {
							blockLen = ((blockLen << 8) | c) + cMediumSize + 1;
							blockPos = ReadLongNBytes(pDiffFil,2);
							diffPos += 2;
							fseek(pOrgFil,blockPos,SEEK_SET);
							CopyFileChars(blockLen,pOrgFil,pDerivedFil,&derivedSum);
							derivedSize += blockLen;
						}
					}
					break;
				case cTagSmallDistantCopy:
					blockLen = (c >> 4) + 1;
					blockPos = ReadLongNBytes(pDiffFil,3);
					diffPos += 3;
					fseek(pOrgFil,blockPos,SEEK_SET);
					CopyFileChars(blockLen,pOrgFil,pDerivedFil,&derivedSum);
					derivedSize += blockLen;
					break;
				case cTagMediumDistantCopy:
					blockLen = (c >> 4);
					c = getc(pDiffFil);
					diffPos++;
					if (c != EOF)
					{
						blockLen = ((blockLen << 8) | c) + cSmallSize + 1;
						blockPos = ReadLongNBytes(pDiffFil,3);
						diffPos += 3;
						fseek(pOrgFil,blockPos,SEEK_SET);
						CopyFileChars(blockLen,pOrgFil,pDerivedFil,&derivedSum);
						derivedSize += blockLen;
					}
					break;
				case cTagLargeDistantCopy:
					blockLen = (c >> 4);
					c = getc(pDiffFil);
					diffPos++;
					if (c != EOF) {
						blockLen = (blockLen << 8) | c;
						c = getc(pDiffFil);
						diffPos++;
						if (c != EOF) {
							blockLen = ((blockLen << 8) | c) + cMediumSize + 1;
							blockPos = ReadLongNBytes(pDiffFil,3);
							diffPos += 3;
							fseek(pOrgFil,blockPos,SEEK_SET);
							CopyFileChars(blockLen,pOrgFil,pDerivedFil,&derivedSum);
							derivedSize += blockLen;
						}
					}
					break;
				case cTagSmallFarCopy:
					blockLen = (c >> 4) + 1;
					blockPos = ReadLongNBytes(pDiffFil,4);
					diffPos += 4;
					fseek(pOrgFil,blockPos,SEEK_SET);
					CopyFileChars(blockLen,pOrgFil,pDerivedFil,&derivedSum);
					derivedSize += blockLen;
					break;
				case cTagMediumFarCopy:
					blockLen = (c >> 4);
					c = getc(pDiffFil);
					diffPos++;
					if (c != EOF) {
						blockLen = ((blockLen << 8) | c) + cSmallSize + 1;
						blockPos = ReadLongNBytes(pDiffFil,4);
						diffPos += 4;
						fseek(pOrgFil,blockPos,SEEK_SET);
						CopyFileChars(blockLen,pOrgFil,pDerivedFil,&derivedSum);
						derivedSize += blockLen;
					}
					break;
				case cTagLargeFarCopy:
					blockLen = (c >> 4);
					c = getc(pDiffFil);
					diffPos++;
					if (c != EOF) {
						blockLen = (blockLen << 8) | c;
						c = getc(pDiffFil);
						diffPos++;
						if (c != EOF) {
							blockLen = ((blockLen << 8) | c) + cMediumSize + 1;
							blockPos = ReadLongNBytes(pDiffFil,4);
							diffPos += 4;
							fseek(pOrgFil,blockPos,SEEK_SET);
							CopyFileChars(blockLen,pOrgFil,pDerivedFil,&derivedSum);
							derivedSize += blockLen;
						}
					}
					break;
				case cTagEOF:
					break;
				default:
					doError(errPATCHTAG);
					tag = cTagEOF;
					break;
			}
		}

		fclose(pDerivedFil);
		fclose(pOrgFil);
		AdjustProgressBar(diffSize);

	} /* End of dummy block */
	long err = B_NO_ERROR;
	
	if (orgSize != checkOrgSize) {
		doError(errPATCHOSZ);
		err = B_ERROR;
	}
	else if (orgSum != checkOrgSum) {
		doError(errPATCHOCHK);
		err = B_ERROR;
	}
	if (derivedSize != checkDerivedSize) {
		doError(errPATCHNSZ);
		err = B_ERROR;
	}
	else if (derivedSum != checkDerivedSum) {
		doError(errPATCHNCHK);
		err = B_ERROR;
	}
	PRINT(("old size %d new size %d  old checksum %d  new checksum %d\n",
					checkDerivedSize,derivedSize,checkDerivedSum,derivedSum));

	fclose(pDiffFil);
	return err;
}


// Copy bytes with checksum
void ArchivePatchItem::CopyFileChars(long count, FILE *inFil, FILE *outFil, ulong *pSum)
{
	ulong	crc = *pSum;
	long res;
	while (count > 0) {
		long amount = min_c(count,COPY_BUF_SIZ);
		res = fread(copyBuf,1,amount,inFil);		
		res = fwrite(copyBuf,1,res,outFil);
		if (res != amount) {
			count = 0;
		}
		// loop over buffer for checksum
		
		crc = crc32(crc,(uchar *)copyBuf,amount);
		
		count -= amount;
	}
	*pSum = crc;
}

/* ***********************************************************************
*
* Read n bytes and return in a long (n=1,2,3,4)
*/

long ArchivePatchItem::ReadLongNBytes(FILE *pFil,int n)
{
	long x;
	int c;

	x = 0;
	while (n > 0) {
		c = getc(pFil);
		if (c == EOF) {
			n = 0;
		}
		else {
			x = (x << 8) | c;
		}
		n--;
	}
	return(x);
}

/***
long long ArchivePatchItem::ReadLongLong(FILE *pFil)
{
	long long x;
	int c;

	x = 0;
	n = 8;
	while (n > 0) {
		c = getc(pFil);
		if (c == EOF) {
			n = 0;
		}
		else {
			x = (x << 8) | c;
		}
		n--;
	}
	return(x);
}
***/

/* ***********************************************************************
*
* Calculate size of an open file
*/

long ArchivePatchItem::FileSize(FILE *pFil)
{
  long filSiz;

  long curPos;

  /*
  * Save file position
  */
  curPos = ftell(pFil);
  /*
  * Go to end of file and determine file size.
  */
  fseek(pFil,0L,SEEK_END);
  filSiz = ftell(pFil);
  /*
  * Restore previous position
  */
  fseek(pFil,curPos,SEEK_SET);

  return(filSiz);

}

/***
char *ArchivePatchItem::PathForDir(BDirectory *dir,char *buf, char *bufMax)
{
	BDirectory parent;
	char *tail;
	static char nameBuf[B_FILE_NAME_LENGTH];
	
	if (dir->GetParent(&parent) == B_NO_ERROR) {
		tail = PathForDir(&parent,buf,bufMax);
	}
	else {
		tail = buf;
		// no parents
		if (tail < bufMax-1) {
			*tail++ = '/';
		}
	}
	dir->GetName(nameBuf);
	char *n = nameBuf;
	while(tail < bufMax && (*tail++ = *n++))
		;
	tail--;
	if (tail < bufMax-1) *tail++ = '/';
	return tail;
}

void ArchivePatchItem::PathForFile(BFile *bfil, char *buf, long bufLen)
{
	char	*bufMax;
	char	*tail = buf;
	char	nameBuf[B_FILE_NAME_LENGTH];
	BDirectory parent;
	
	bufMax = buf + bufLen;

	if (bfil->GetParent(&parent) == B_NO_ERROR)
		tail = PathForDir(&parent,buf,bufMax);
	
	bfil->GetName(nameBuf);
	char *n = nameBuf;
	while(tail < bufMax && (*tail++ = *n++))
		;
	
	// check for overflow
	tail--;
	if (*tail) {
		// the full string didn't fit
		*buf = '\0';
	}
}
***/

#endif
