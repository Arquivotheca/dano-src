#include <Be.h>
// ArchivePatchItem.cpp

#include "ArchivePatchItem.h"
#include "PackArc.h"

#include "PackMessages.h"
#include "GStatusWind.h" // for generic setup message

#include "GlobalIcons.h"
#include "AutoPtr.h"
#include "ExtractUtils.h"

#include "MyDebug.h"
#include "Util.h"

ArchivePatchItem::ArchivePatchItem( BEntry &oldFile,
									BEntry &newFile)
	: ArchiveFileItem(newFile)
{
	// info is culled from the new file
	
	// append "Patch" to name?
	canDelete = TRUE;	

	// set icon to patch icon
	if (smallIcon && smallIcon != gGenericFileIcon)
		delete smallIcon;
	smallIcon = gPatchIcon;
	
	// old file info
	oldFile.GetSize(&oldFileSize);
	
	oldFile.GetCreationTime(&oldCreationTime);
	oldFile.GetModificationTime(&oldModTime);
}

ArchivePatchItem::ArchivePatchItem()
	: ArchiveFileItem()
{
	PRINT(("\n\nnewarchive patch item\n"));
	if (smallIcon && smallIcon != gGenericFileIcon)
		delete smallIcon;
	
	smallIcon = gPatchIcon;
	//tempFileRef.record = -1;
	//tempFileRef.database = -1;
	canDelete = TRUE;
}

ArchivePatchItem::~ArchivePatchItem()
{
	// remove associated temporary file
	BEntry	tempFile(&tempFileRef);
	if (tempFile.InitCheck() < B_NO_ERROR) {
		tempFile.Remove();
	}
}

long ArchivePatchItem::CompressItem(PackArc *archiveFile)
{
	PRINT(("entering ArchivePatchItem::CompressItem\n"));

	if (dirty == FALSE || deleteThis == TRUE)
		return B_NO_ERROR;
		
	status_t	err;
	long		res;
	
	// mark offset for where this item begins
	seekOffset = archiveFile->catalogOffset;
	off_t sz;
	
	PRINT(("calling add patch file for %s\n",name));
	BFile	theFile(&tempFileRef,O_RDONLY);
	err = theFile.InitCheck();
	if (err < B_OK) {
		res = doError("There was an error opening the patch data file.",
					"Continue","Cancel");
		if (res == 0) err = B_CANCELED;
		return err;
	}
	err = archiveFile->AddNode(&theFile, &sz, B_FILE_NODE);
	PRINT(("add patch file done\n"));
	if (err == B_CANCELED) {
		// compression was canceled
		PRINT(("*************Dirty is true for patch %s\n",name));
		dirty = TRUE;
		return B_CANCELED;
	}
	else if (err != B_NO_ERROR) {
		// option to continue or cancel
		doError("There was an error compressing the patch data.");		
		// this might allow us to continue
		// archiveFile->ResetStream();
		return err;
	}

	//theFile.GetSize(&sz);
	//if (archiveFile->cstream->c_stream.total_in != sz) {
	//	doError("Warning, patch data may have been unexpectedly modified");
	//}
	
	// remove the temporary file
	
	// remove temp file
	theFile.Unset();
	
	BEntry	tempEntry(&tempFileRef);
	tempEntry.Remove();

	compressedSize = sz;
	archiveFile->catalogOffset += compressedSize;
	// adler32 = archiveFile->cstream->c_stream.adler;
	
	/**
	if (archiveFile->catalogOffset != archiveFile->Seek(0,B_SEEK_MIDDLE)) {
		doError("Warning, after compression seek position does not match added offset");
	} **/
	
	// archiveFile->ResetStream();
	dirty = FALSE;
		
	return err;
}

long ArchivePatchItem::ExtractItem(PackArc *arcFile, BDirectory *dest)
{
	arcFile; dest;
	// skip extracting
	return B_NO_ERROR;
}

long ArchivePatchItem::TestPatch(PackArc *arcFile, entry_ref &oldFileRef,
									entry_ref &dirRef, const char *newName)
{
	arcFile; oldFileRef; dirRef; newName;
	static int tmpCount = 0;
	long	err = B_NO_ERROR;
#if 0
	BDirectory	tmp;	
	char		pbuf[80];
	sprintf(pbuf,"/boot/var/tmp/tmpatch_000000000%d",tmpCount++);
	
	entry_ref	tmpRef;
	get_ref_for_path(pbuf, &tmpRef);
	
	BFile		outFile(tmpRef);
	
	if (outFile.Error() < B_NO_ERROR) {
		doError("Could not create temporary patch file");
		return B_ERROR;
	}
	outFile.GetParent(&tmp);
	/*****	
	* extract compressed patch
	******/
	PRINT(("extracting patch file %s\n",name));

	BMessage upMsg(M_SETUP_STATUS);

	long fakeSize = uncompressedSize*2;
	//// fix this
	sprintf(pbuf,"Patching: %s",name);
	upMsg.AddString("message",pbuf);
	upMsg.AddInt32("bytes",fakeSize);
	arcFile->statusMessenger.SendMessage(&upMsg);

	// updates bytes for uncompressed size
	err = arcFile->Seek(seekOffset,B_SEEK_TOP);
	err = arcFile->ExtractFile(&outFile,compressedSize);
	if (err == B_CANCELED) {
		tmp.Remove(&outFile);
		return err;
	}
	if (err != B_NO_ERROR) {
		doError("Encountered error when decompressing patch");
		tmp.Remove(&outFile);
		return err;
	}
	if (arcFile->c_stream.adler != adler32) {
		char msg[B_FILE_NAME_LENGTH+70];
		sprintf(msg,"Warning, checksums do not not match. The patch for \"%s\" may be corrupt.",name);
		PRINT(("IN %d OUT %d\n",arcFile->c_stream.adler,adler32));
		doError(msg);
	}
	arcFile->ResetDecompress();
#if 0
	if (uncompressedSize != outFile.Size()) {
		char msg[B_FILE_NAME_LENGTH+80];
		sprintf(msg,"Warning, input and output file sizes do not match. The file \"%s\" may be corrupt.",name);
		doError(msg);
	}
#endif
	if (uncompressedSize < 1024) {
		BMessage upMsg(M_UPDATE_PROGRESS);
		upMsg.AddInt32("bytes",1024 - uncompressedSize);
		arcFile->statusMessenger.SendMessage(&upMsg);
	}

	// difference between patch size and new output file
	// for progress bar
	long remainingBytes = fakeSize - outFile.Size();
	if (remainingBytes < 0)
		remainingBytes = 0;
		
	gLastVal = 0;

	/*****
	* find original item
	******/
	
	// get original file/s
	static const long bigPathSz = 1024;
	AutoPtr<char> diffbuf(bigPathSz);
	AutoPtr<char> origbuf(bigPathSz);
	AutoPtr<char> newfbuf(bigPathSz);
	char	*diffFilName = diffbuf.Value();
	char    *oldFilName = origbuf.Value();
	char	*newFilName = newfbuf.Value();
	
	PathForFile(&outFile,diffFilName,bigPathSz);

	PRINT(("patch path is %s\n",diffFilName));
	
	
	BFile		originalFile(oldFileRef);
	BFile		newFile;
	BDirectory	container(dirRef);
	// check old file name, type, app size
	
	
	PathForFile(&originalFile,oldFilName,bigPathSz);
	
	err = container.Create(newName,&newFile);
	if (err == B_NO_ERROR) {
		PRINT(("new file successfully created\n"));		
		PathForFile(&newFile,newFilName,bigPathSz);
		PRINT(("applying patch\n"));
		theArchiveFile = arcFile;
			
		err = ApplyPatch(oldFilName,newFilName,diffFilName,remainingBytes);
	
		if (err == B_NO_ERROR) {
					
			if (type == 'BAPP') {
			
			}
			newFile.SetTypeAndApp(type,creator);
			newFile.SetModificationTime(modTime);
			newFile.SetCreationTime(creationTime);
		}
		else {
			// error occurred, delete the new file if patch failed
			container.Remove(&newFile);
		}
	}
	else {
		doError("Temporary patch file could not be created."); 
	}
	tmp.Remove(&outFile);
	
#endif
	return err;

}

void ArchivePatchItem::AdjustProgressBar(long curVal)
{
	curVal = curVal*progressAdjust;

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

long ArchivePatchItem::ApplyPatch(char *orgFilNam,
								  char *derivedFilNam,
								  char *diffFilNam,
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
		doError("Could not allocate copy buffer");
		return B_ERROR;
	}

	// open the temp file
	pDiffFil = fopen(diffFilNam,"rb");

	if (pDiffFil == NULL)
	{
		doError("Cannot open patch file");
		return B_ERROR;
	}
	magicNum = ReadLongNBytes(pDiffFil,4);	
	

	/*
	* Read sizes and checksums of original and updated file
	*/
	checkOrgSize = ReadLongNBytes(pDiffFil,4);
	checkOrgSum = ReadLongNBytes(pDiffFil,4);
	orgSize = 0;
	orgSum = crc32(0L, NULL, 0);
	checkDerivedSize = ReadLongNBytes(pDiffFil,4);
	checkDerivedSum = ReadLongNBytes(pDiffFil,4);
	
	derivedSize = 0;
	derivedSum = crc32(0L, NULL, 0);

	{
		/*
		* Check file header
		*/
		if (magicNum != PATCH_MAGIC_NUM)
		{
			doError("Patch header is invalid. The patch cannot be applied.");
			fclose(pDiffFil);
			return B_ERROR;
		}

		pOrgFil = fopen(orgFilNam,"rb");
		if (pOrgFil == NULL)
		{
			doError("Cannot open existing file. The patch cannot be applied.");
			fclose(pDiffFil);
			return B_ERROR;
		}

		pDerivedFil = fopen(derivedFilNam,"wb");
		if (pDerivedFil == NULL)
		{
			doError("Cannot open new file. The patch cannot be applied.");
			fclose(pDiffFil);
			fclose(pOrgFil);
			return B_ERROR;
		}

		orgSize += FileSize(pOrgFil);

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

		// InitProgressBar(0,FileSize(pDiffFil),"Applying patch:");
		progressAdjust = ((float)remainBytes/(float)(FileSize(pDiffFil)));
		PRINT(("PROGADJUST IS %f\n",progressAdjust));
		
		// progressAdjust = 1.0;
		
		tag = 0;
		prevDiffPos = 0;
		diffPos = 0;
		while (tag != cTagEOF && (c = getc(pDiffFil)) != EOF) {
			diffPos++;
			/*
			* Adjust bar every 8192 bytes
			*/
			if (diffPos - prevDiffPos > 8192) {
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
					doError("Unknown tag in patch file.");
					tag = cTagEOF;
					break;
			}
		}

		fclose(pDerivedFil);
		fclose(pOrgFil);
		// CloseProgressBar();

	} // End of dummy block
	long err = B_NO_ERROR;
	
	if (orgSize != checkOrgSize || orgSum != checkOrgSum) {
		doError("Old file is not the file from which the patch was created.");
		err = B_ERROR;
	}

	if (derivedSize != checkDerivedSize || derivedSum != checkDerivedSum) {
		doError("New file checksum deos not match. Patch could not be completed.");
		PRINT(("old size %d new size %d  old checksum %d  new checksum %d\n",
					checkDerivedSize,derivedSize,checkDerivedSum,derivedSum));
		err = B_ERROR;
	}

	fclose(pDiffFil);
	return err;
}

// copy bytes witch checksum
void ArchivePatchItem::CopyFileChars(long count, FILE *inFil, FILE *outFil, ulong *pSum)
{
	ulong	crc = *pSum;
	long res;
	while (count > 0) {
		long amount = min_c(count,COPY_BUF_SIZ);
		res = fread(copyBuf,sizeof(char),amount,inFil);		
		res = fwrite(copyBuf,sizeof(char),res,outFil);
		if (res != amount) {
			count = 0;
		}
		// loop over buffer for checksum
		
		crc = crc32(crc,(uchar *)copyBuf,amount);
		
		count -= amount;
	}
	*pSum = crc;
}

// Read n bytes and return in a long (n=1,2,3,4)
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

