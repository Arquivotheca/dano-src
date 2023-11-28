// InstallPack.cpp
#include "InstallPack.h"
#include <Resources.h>
#include <string.h>


#include "IArchiveItem.h"
#include "IArchivePatchItem.h"
#include "IArchiveScriptItem.h"

#include "SPackData.h"
#include "DataID.h"
#include "ArcCatalog.h"

#include "InstallMessages.h"
#include "IGroupList.h"
#include "InstallDefs.h"
#include "FileEntry.h"
#include "ErrHandler.h"

#include "Util.h"

#include "MyDebug.h"

// The original archive format version was 1.  Version 2 added support for
// three new replacement policies, but did not change the format at all.  
static const uint16 VERSION_ARRAY_END = 65535;
static const uint16 SUPPORTED_VERSIONS[] = { 1, 2, VERSION_ARRAY_END};

// 8 bytes
const 	int kMagicSize = 8;

uchar kPHeader[kMagicSize] = {
			'A','l','B','\032',0xFF,'\n','\r','\0'
};

char *ReadCatString(BFile *f, int32 len);

// when creating a new file, no existing ref
InstallPack::InstallPack()
	:	fArcFile(0),
		pEntry(0),
		fPackData(0),
		fCatalog(0)
{	
	curFolder = new ArchiveFolderItem();
	curFile = new ArchiveFileItem();
}


// when created from an existing entry_ref
InstallPack::InstallPack(entry_ref &ref, bool expectRes)
	:	fArcFile(0),
		pEntry(0),
		fPackData(0),
		fCatalog(0)
{
	long err;
	bool supportedVersion = false;

	curFolder = new ArchiveFolderItem();
	curFile = new ArchiveFileItem();

	long installerSize = 0;
	
	pEntry = new BEntry(&ref);
	if (pEntry && pEntry->InitCheck() < B_NO_ERROR) {
		doFatal("Could not find specified package file.");
		if (pEntry)
			delete pEntry;
		pEntry = NULL;
		return;
	}
	if (expectRes) {
		BFile			resFile(&ref,O_RDONLY);
		BResources		theRes(&resFile);
		
		err = theRes.ReadResource(SIZE_RES_TYPE,0,&installerSize,0,sizeof(installerSize));
		if (err <= B_ERROR) {
			doFatal("Could not read the size resource.");
			delete pEntry;
			pEntry = NULL;
			return;
		}
		installerSize = B_BENDIAN_TO_HOST_INT32(installerSize);
	}
	
	PRINT(("installerSize is %d\n",installerSize));
	entriesOffsetAdjust = installerSize;
	PRINT(("entriesAdjust is %d\n",entriesOffsetAdjust));
	SetFile();
	if (VerifyMagic(fArcFile) != B_NO_ERROR) {
		goto initErr;
	}
	arcHeader	head;
	if (ReadArcHeader(fArcFile,head) != B_NO_ERROR) {
		goto initErr;
	}
	off_t fSize;
	fArcFile->GetSize(&fSize);
	if (head.fileSize > fSize - entriesOffsetAdjust) {
		char info[40];
		sprintf(info,"(%Ld expected, %Ld current)",head.fileSize,fSize - entriesOffsetAdjust);
		doError("The archive data size is incorrect.\n%s",info);
		goto initErr;
	}
	// check version
	for (int32 i = 0; SUPPORTED_VERSIONS[i] != VERSION_ARRAY_END; i++) {
		if (head.vers == SUPPORTED_VERSIONS[i]) {
			supportedVersion = true;
			break;
		}
	}
	if (!supportedVersion) {
		doError("This package file contains a newer format than is supported by this \
version of SoftwareValet.  Please upgrade to the latest version of SoftwareValet \
and then try to install this package again.\n\nUpdated versions of SoftwareValet can \
be found on the BeDepot Web site at http://www.bedepot.com");
		goto initErr;
	}
	
	attribOffset += entriesOffsetAdjust;
	catalogOffset += entriesOffsetAdjust;
	
	ClearFile();
	return;
	
initErr:
	PRINT(("install pack open error\n"));

	delete pEntry;
	pEntry = NULL;
	ClearFile();
	return;
}

InstallPack::~InstallPack()
{
	if (pEntry)	delete pEntry;
	if (fArcFile) delete fArcFile;
	if (fPackData) delete fPackData;
	if (fCatalog)	delete fCatalog;
	
	delete curFile;
	delete curFolder;
}

#define PK_CORRUPT "The package file is corrupt "

status_t	InstallPack::ReadArcHeader(BFile *f, arcHeader &head)
{
	ErrHandler		err;
	int32			vers;
	uint64			flag;
	off_t			size;
	
	try {
		PackData	tPackData;
		
		record_header	header;
		err = tPackData.ReadRecordHeader(f,&header);
	
		if (header.what != ID_PKGHEADER || header.type != LIS_TYPE)
			err = B_ERROR;
	
		bool readingEntries = true;
		while (readingEntries) {
			err = tPackData.ReadRecordHeader(f,&header);
			switch (header.what) {
				case ID_FORMAT_VERS:
					err = tPackData.ReadInt32(f,&vers);
					head.vers = vers;
					break;
				case ID_ARCFLAGS:
					err = tPackData.ReadInt64(f,(int64 *)&flag);
					// 64bit to 32bit
					arcFlags = flag;
					break;
				case ID_FILESIZE:
					err = tPackData.ReadInt64(f,&size);
					head.fileSize = size;
					break;
				case ID_CATALOG_OFFSET:
					err = tPackData.ReadInt64(f,&size);
					catalogOffset = size;
					break;
				case ID_ATTRIB_OFFSET:
					err = tPackData.ReadInt64(f,&size);
					attribOffset = size;
					break;
				case END_TYPE:
					readingEntries = false;
					break;
				default:
					err = tPackData.SkipData(f,&header);
					break;
			}
		}
	}
	catch (ErrHandler::OSErr e) {
		return e.err;
	}
	return B_NO_ERROR;
}

long InstallPack::BeginDecompression()
{
	grandTotal = 0;
	
	// open the archive file for reading
	//cstrm.SetEntry(pEntry);
	//err = cstrm.COpen(O_RDONLY);
	//if (err < B_NO_ERROR) {
	//	doError(errOPENARC);
	//		return err;
	// }
	return B_NO_ERROR;
}

status_t	InstallPack::VerifyMagic(BFile *f)
{
	status_t err;
	PRINT(("entries offset adjust is %d\n",entriesOffsetAdjust));
	
	f->Seek(entriesOffsetAdjust,SEEK_SET);
	
	uchar header[kMagicSize];
	err = f->Read(header,kMagicSize);
	if (err != kMagicSize) {
		doError("Could not read complete file header (%s).",strerror(err));
	}
	else if (kPHeader[0] != header[0] || kPHeader[1] != header [1] || kPHeader[2] != header[2]) {
		// bad magic number
		doError("Invalid package file.");
	}
	else if (kPHeader[3] != header[3]) {
		doError(PK_CORRUPT"(control character stripping)");
	}
	else if (kPHeader[4] != header[4]) {
		doError(PK_CORRUPT"(high-bit strippping)");
	}
	else if (kPHeader[5] != header[5]) {
		doError(PK_CORRUPT"(newline conversion)");
	}
	else if (kPHeader[6] != header[6]) {
		doError(PK_CORRUPT"(carriage return converstion");
	}
	else if ('\n' == header[7]) {
		doError(PK_CORRUPT"(newline insertion)");
	}
	else if (kPHeader[7] != header[7]) {
		doError(PK_CORRUPT"(null stripping)");
	}
	else {
		return B_NO_ERROR;
	}
	return B_ERROR;
}

long InstallPack::EndDecompression()
{
	// return cstrm.CClose();
	return B_NO_ERROR;
}

long InstallPack::ExtractFile(BNode *outNode, off_t offset)
{
	status_t err;

	ASSERT(fArcFile);
	
	fArcFile->Seek(offset+entriesOffsetAdjust,SEEK_SET);
	err = fPackData->ExtractNode(fArcFile,outNode);

	return err;	
/***
	// numBytes is uncompressed size
	// setup decompression
	long err;

	// open the output file
	BFile	outFile(outEntry,O_RDWR);
	
	curOffset += entriesOffsetAdjust;
	
	cstrm.CurFile()->Seek(curOffset,SEEK_SET);
	
	ulong adler = adler32(0L, Z_NULL, 0);
	while ( TRUE ) {
		// long len = min_c(INBUFSIZ,numBytes);
		err = cstrm.CRead(inBuf,INBUFSIZ);
		
		long len = err;
		if (!len) {
			break;	
		}
		numBytes -= err;
		
		adler = adler32(adler, (Bytef *)inBuf, len);
		err = outFile.Write(inBuf,len);
		if (doCancel == TRUE) {
			// outFile.Close();
			return B_CANCELED;
		}
		bucket -= len;
		while (bucket < 0) {
			bucket += byteUpdateSize;
			BMessage upMsg(M_UPDATE_PROGRESS);
			upMsg.AddInt32("bytes",byteUpdateSize);
			statusMessenger.SendMessage(&upMsg);
		}
		if (len < INBUFSIZ)
			break;
	}
	PRINT(("total bytes decompressed %ld\n",cstrm.c_stream.total_in));
	PRINT(("total bytes output %ld\n",cstrm.c_stream.total_out));
	
	grandTotal = grandTotal + cstrm.c_stream.total_out;
	checksum = adler;
***/
	
	return B_NO_ERROR;
}


void InstallPack::ResetDecompress()
{
	// cstrm.CReset();
}



bool	InstallPack::SetFile()
{
	if (!fPackData)
		fPackData = new SPackData(statusMessenger, &doCancel);
	else
		fPackData->SetProgressMessenger(statusMessenger);

	if (fArcFile)
		delete fArcFile;
	fArcFile = new BFile(pEntry,O_RDONLY);
	return fArcFile->InitCheck();
};

void	InstallPack::ClearFile()
{
	if (fArcFile)
		delete fArcFile;
	fArcFile = NULL;
}
