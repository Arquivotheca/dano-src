#include <Be.h>
#include "PackArc.h"
#include "PackApplication.h"
#include "Replace.h"

#include "ErrHandler.h"
#include "FileLooper.h"

#include "FArchiveItem.h"
#include "ArchivePatchItem.h"
#include "ArchiveScriptItem.h"

#include "PackMessages.h"

#include "SPackData.h"
#include "DataID.h"

#include "DestinationList.h"

#include <sys/socket.h>		// for htonX and ntohX macros
#include <arpa/inet.h>		// for htonX and ntohX macros
#include "Util.h"

#include "MyDebug.h"

//#define htonll(x)	x;
//#define ntohll(x)	x;

// The original archive format version was 1.  Version 2 added support for
// three new replacement policies, but did not change the format otherwise.
static const uint16 VERSION_ARRAY_END = 65535;
static const uint16 SUPPORTED_VERSIONS[] = { 1, 2, VERSION_ARRAY_END};
// IT IS IMPORTANT THAT Replace.cpp BE KEPT UP TO DATE WITH THE VERSION DEFINES!
const uint16 CURRENT_VERSION = 2;
const uint16 COMPAT_VERSION = 1;

// 8 bytes
const 	int kMagicSize = 8;

// 54 bytes
const	int kEntriesOffset = kMagicSize + PackData::RECORD_HEADER_SZ*(2 + 5) +
							sizeof(uint32)*1 +
							sizeof(uint64)*4;
							
							
const uchar kPHeader[kMagicSize] = {
			'A','l','B','\032',0xFF,'\n','\r','\0'
};


// construct a new archive object
// not associated with a particular file
// can be associated with a particular file
// for operations
PackArc::PackArc()
	:	catalogOffset(0),
		attribOffset(0),
		entriesOffset(0),
		instDataSize(0),
		arcFlags(0),
		fileLooper(NULL),
		fArcFile(NULL),
		fPackData(NULL),
		fErr(B_NO_ERROR),
		fVersion(CURRENT_VERSION)
{
	//updateMessenger = NULL;
	//statusMessenger = NULL;
}

// when created from an existing record ref
PackArc::PackArc(entry_ref *ref, uint16 version)
	:	fArcRef(*ref),
		catalogOffset(0),
		attribOffset(0),
		entriesOffset(0),
		instDataSize(0),
		arcFlags(0),
		fileLooper(NULL),
		fArcFile(NULL),
		fPackData(NULL),
		fErr(B_NO_ERROR),
		fVersion(version)
{
	PRINT(("PackArc::PackArc created with entry\n")); 
	
	bool supportedVersion = false;
	
	BFile	arcFile(ArcRef(),O_RDONLY);
	
	if (arcFile.InitCheck() != B_NO_ERROR) {
		doError("Error opening package file, open call failed");
		goto initErr;
	}
	if (VerifyMagic(&arcFile) != B_NO_ERROR) {
		goto initErr;
	}
	arcHeader	head;
	if (ReadArcHeader(&arcFile,head) != B_NO_ERROR) {
		goto initErr;
	}
	off_t	fSize;
	arcFile.GetSize(&fSize);
	if (head.fileSize != fSize) {
		doError("The file size is incorrect.");
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
		doError("Format version number is unsupported.");
		goto initErr;
	}

	PRINT(("Read file: catalog offset is %d, attribOffset is %d\n",catalogOffset,attribOffset));
	return;
	
initErr:
	fErr = B_ERROR;
	return;
}

status_t	PackArc::InitCheck()
{
	return fErr;
}

#define PK_CORRUPT "The package file is corrupt "

status_t	PackArc::ReadArcHeader(BFile *f, arcHeader &head)
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
		entriesOffset = f->Position();
	}
	catch (ErrHandler::OSErr e) {
		return e.err;
	}
	return B_NO_ERROR;
}

// called after WriteHeader
status_t PackArc::WriteArcHeader(BFile *f)
{
	PackApp *app = dynamic_cast<PackApp *>(be_app);
//	bool compat = false;
	
	PRINT(("PackArc::WriteArcHeader\n"));
	ErrHandler	err;
	off_t		size;

	if (kEntriesOffset > entriesOffset) {
		// we will write more data than we have space for
		// so we should make room!!
		doError("Warning: header size adjustment required.");
	}
	
	try {
		err = f->Seek(kMagicSize,SEEK_SET);
		
		err = fPackData->WriteRecordHeader(f,ID_PKGHEADER,LIS_TYPE,0);
		err = fPackData->WriteInt32(f,fVersion, ID_FORMAT_VERS);
		err = fPackData->WriteInt64(f,arcFlags,ID_ARCFLAGS);
		// write fileSize
		f->GetSize(&size);
		err = fPackData->WriteInt64(f,size,ID_FILESIZE);
		err = fPackData->WriteInt64(f,catalogOffset,ID_CATALOG_OFFSET);
		err = fPackData->WriteInt64(f,attribOffset,ID_ATTRIB_OFFSET);
		
		err = fPackData->WriteEndHeader(f);
	}
	catch (ErrHandler::OSErr e) {
		return e.err;
	}
	return B_NO_ERROR;
}

status_t	PackArc::VerifyMagic(BFile *f)
{
	status_t err;
	f->Seek(0,SEEK_SET);
	
	uchar header[kMagicSize];
	err = f->Read(header,kMagicSize);
	if (err != kMagicSize) {
		doError("Could not read complete file header.");
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

status_t	PackArc::WriteMagic(BFile *f)
{
	status_t err;
	f->Seek(0,SEEK_SET);
	
	err = f->Write(kPHeader,kMagicSize);
	if (err != kMagicSize) {
		doError("Could not write complete file header.");
	}
	return err;
}

// make a new empty file
status_t PackArc::SetupNew(uint32 fileFlags)
{
	PRINT(("PackArc::SetupNew\n"));
	status_t err;

	BFile	curFile(ArcRef(),O_RDWR);
	err = curFile.InitCheck();
	if (err != B_NO_ERROR) {
		doError("Error opening package file, open call failed.");
		return err;
	}
	
	arcFlags = fileFlags;

	// kEntriesOffset (for this version)
	catalogOffset = attribOffset = entriesOffset = kEntriesOffset;

	// print out error messages?
	err = WriteMagic(&curFile);
	if (err < B_NO_ERROR)
		return err;
		
	fPackData = new SPackData(statusMessenger,&fileLooper->doCancel);
	// must be ok
	err = WriteArcHeader(&curFile);
	delete fPackData;
	
	if (err < B_NO_ERROR)
		return err;
		
	return B_NO_ERROR;
}

void PackArc::SetupLooper(PackWindow *document)
{
	updateMessenger = BMessenger((BWindow *)document);
	fileLooper = new FileLooper();
	fileLooper->packArc = this;
	fileLooper->wind = document;
	fileLooper->Run();
}

PackArc::~PackArc()
{
	// figure out
	// called when window is destroyed
	// PRINT(("posting file looper quit requested\n"));
	if (fileLooper)
		fileLooper->PostMessage(B_QUIT_REQUESTED);
}

// this is called when there are items in the compress queue
status_t PackArc::BeginCompression()
{
	status_t err = B_NO_ERROR;
	
	// if (!zcodec) zcodec = new ZlCodec();
	
	/***
	cstream->SetRef(ArcRef());
	err = cstream->COpen(O_RDWR);
	cstream->CurFile()->Seek(catalogOffset,SEEK_SET);
	
	int32 ssize = cstream->CurFile()->Position();
	PRINT(("begin compression: catOffset %d, position %d\n",catalogOffset,ssize));
	***/
	
	SetFile(O_RDWR);
	
	fArcFile->Seek(catalogOffset,SEEK_SET);
	fPackData = new SPackData(statusMessenger,&fileLooper->doCancel);
	return err;
}

status_t PackArc::BeginDecompression()
{
	//	status_t err;
	// if (!zcodec) zcodec = new ZlCodec();
	
	// cstream->SetRef(ArcRef());
	// return cstream->COpen(O_RDONLY);

	SetFile(O_RDONLY);
	fPackData = new SPackData(statusMessenger,&fileLooper->doCancel);	
	return B_NO_ERROR;
}

/*
*	Close stream with no flush (reset called), file close
*	set offsets, file size, rewind if canceled!!!
*/
status_t PackArc::EndCompression()
{
	PRINT(("PackArc::EndCompression\n"));
	status_t err;
	
	// BFile	*curFile = cstream->CurFile();	
	
	// set size to the current location
	// this is bad if we delete a file
	// be sure to seek back!
	
#if DEBUG
	if (backBytes > 0)
		doError("backBytes is positive");
	else if (backBytes < 0)
		PRINT(("warning backBytes is negative %d\n",backBytes));
#endif	
	
	ASSERT(fArcFile);
	catalogOffset = fArcFile->Position();

	PRINT(("setting file size... "));
	err = fArcFile->SetSize(catalogOffset);
	PRINT(("done\n"));
	
	// close no flush, no error
	//cstream->CClose(FALSE);
	//PRINT(("grand total is %d\n",grandTotal));

	ClearFile();	
	delete fPackData;
	return err;
}

/*
*	Close the stream, close file, no flushing
*/
status_t PackArc::EndDecompression()
{
	PRINT(("PackArc::EndDecompression\n"));
	// closing true, flushing false
	// return cstream->CReset(TRUE,FALSE);
	
	ClearFile();
	delete fPackData;
	return B_NO_ERROR;
}

/*
*	Close the cstream with flush, leave file open, use to recover
*/
status_t PackArc::ResetStream()
{
	PRINT(("PackArc::ResetStream\n"));
	// closing false, flushing true
	// return cstream->CReset(FALSE,TRUE);
	
	return B_NO_ERROR;
}


status_t PackArc::SetFile(uint32 mode)
{
	if (fArcFile)	delete fArcFile;
	fArcFile = new BFile(ArcRef(),mode);
	return fArcFile->InitCheck();
}

void PackArc::ClearFile()
{
	if (!fArcFile)
		return;
	delete fArcFile;
	fArcFile = NULL;
}

// write the catalog and attributes
status_t PackArc::WriteFinalData(ArchiveFolderItem *fold,
									bool isCD,
									AttribData *att)
{
	status_t	err;

	err = SetFile(O_RDWR);
	if (err != B_NO_ERROR) {
		doError("Error opening file for catalog writing.");
		return err;
	}
	
	fPackData = new SPackData(statusMessenger,&fileLooper->doCancel);	
	fArcFile->Seek(catalogOffset,SEEK_SET);
	
	PRINT(("writing catalog\n"));
	long bytes = 0;
	err = WriteCatalogFolder(fold,bytes,isCD);
	
	// sanity checks

#if DEBUG
	if ((bytes + entriesOffset) < catalogOffset) {
		PRINT(("total compressed bytes: %d   headersize: %d   cat offset: %d\n",
			bytes, entriesOffset, catalogOffset));
		char buf[80];
		long diff = catalogOffset - (bytes + entriesOffset);
		sprintf(buf,"%d",diff);
		doError("Warning: file data size is greater than the sum of the compressed contents. Difference %s",buf);
	}
	else
#endif
	if ((bytes + entriesOffset) > catalogOffset) {
		PRINT(("total compressed bytes: %d   headersize: %d   cat offset: %d\n",
			bytes, entriesOffset, catalogOffset));
		doError("Warning: file data size is less than the sum of the compressed contents. Some compressed data may be lost");
	}
	
	attribOffset = fArcFile->Position();		
	
	PRINT(("Attribute offset %d\n",attribOffset));
	//fArcFile->Seek(archiveFile->attribOffset,B_SEEK_TOP);
	PRINT(("Writing attributes.... "));
	NewWriteAttributes(att,fold->numCompressibleItems+1);
	PRINT(("DONE\n"));

	PRINT(("Setting file size..."));
	err = fArcFile->SetSize(fArcFile->Position());
	
	//archiveFile->Close();
	//archiveFile->Open(B_READ_WRITE);
	//PRINT(("DONE\n"));
	err = WriteArcHeader(fArcFile);
	
	PRINT(("Attribute offset %d\n",attribOffset));
	
	//archiveFile->Close();
	//PRINT(("WROTE CATALOG catoffset is %d attribOffset is %d file size is %d\n",catalogOffset,
	//		attribOffset, fArcFile->Size()));

	ClearFile();
	delete fPackData;
	return err;
}

status_t	PackArc::WriteArcItemCommon(ArchiveItem *arcItem)
{
	PackData *data = fPackData;

	data->WriteString(fArcFile,arcItem->name,ID_NAME);
	data->WriteInt32(fArcFile,arcItem->groups,ID_GROUPS);
	data->WriteInt32(fArcFile,arcItem->destination,ID_DEST);
	data->WriteInt32(fArcFile,(arcItem->customDest) ? 1 : 0, ID_CUST);
	data->WriteInt32(fArcFile,arcItem->replacement,ID_REPL);
	
	data->WriteInt32(fArcFile,arcItem->platform,ID_PLAT);
	data->WriteInt32(fArcFile,arcItem->creationTime,ID_CTIME);
	data->WriteInt32(fArcFile,arcItem->modTime,ID_MTIME);
	
	if (arcItem->seekOffset >= 0) {
		data->WriteInt64(fArcFile,arcItem->seekOffset,ID_OFFSET);
		data->WriteInt64(fArcFile,arcItem->compressedSize,ID_CMPSIZE);
		data->WriteInt64(fArcFile,arcItem->uncompressedSize,ID_ORGSIZE);
	}
	data->WriteEndHeader(fArcFile);
	
	return B_OK;
}

// could write empty folder if they don't have any clean files
status_t PackArc::WriteCatalogFolder(ArchiveFolderItem *fold,long &bytes, bool isCD)
{
	RList<ArchiveItem *> *items = fold->compressEntries;
	long count, cleanCount;
	
	//long nameLen = strlen(fold->name);
	status_t	err = B_NO_ERROR;
	
	PackData *data = fPackData;
	

	data->WriteRecordHeader(fArcFile,ID_FOLDERITEM,LIS_TYPE,0);
	
	data->WriteRecordHeader(fArcFile,ID_FOLDERDATA,LIS_TYPE,0);

	data->WriteString(fArcFile,fold->name,ID_NAME);
	data->WriteInt32(fArcFile,fold->groups,ID_GROUPS);
	data->WriteInt32(fArcFile,fold->destination,ID_DEST);
	data->WriteInt32(fArcFile,(fold->customDest) ? 1 : 0,ID_CUST);
	data->WriteInt32(fArcFile,fold->replacement,ID_REPL);
	data->WriteInt32(fArcFile,fold->platform,ID_PLAT);
	data->WriteInt32(fArcFile,fold->creationTime,ID_CTIME);
	data->WriteInt32(fArcFile,fold->modTime,ID_MTIME);
	
	if (fold->seekOffset >= 0) {
		data->WriteInt64(fArcFile,fold->seekOffset,ID_OFFSET);
		data->WriteInt64(fArcFile,fold->dataCompressedSize,ID_CMPSIZE);
		bytes += fold->dataCompressedSize;
	}
	data->WriteEndHeader(fArcFile);
	
	// data->WriteRecordHeader(fArcFile,ID_FOLDITEMS,LIS_TYPE,0);
	// write items inside this folder
	count = items->CountItems();
	cleanCount = 0;
	for(long i = 0; i < count; i++) {
		ArchiveItem *arcItem = items->ItemAt(i);
		if (arcItem->IsFolder()) {
			WriteCatalogFolder((ArchiveFolderItem *)arcItem,bytes,isCD);
			cleanCount++;
		}
		else if (arcItem->dirty == FALSE || isCD) {
			if (typeid(*arcItem) == typeid(ArchiveFileItem)) {
				ArchiveFileItem *fItem = (ArchiveFileItem *)arcItem;
				
				data->WriteRecordHeader(fArcFile,ID_FILEITEM,LIS_TYPE,0);
				// mimetype
				data->WriteString(fArcFile,fItem->fType,ID_MIME);
				// app signature
				if (fItem->fSignature)
					data->WriteString(fArcFile,fItem->fSignature,ID_APP_SIG);
				data->WriteInt32(fArcFile,fItem->mode,ID_MODE);
								
				// neutralize version info byte-order
				uint32 outVersion[5];
				outVersion[0] = B_HOST_TO_BENDIAN_INT32(fItem->versionInfo[0]);
				outVersion[1] = B_HOST_TO_BENDIAN_INT32(fItem->versionInfo[1]);
				outVersion[2] = B_HOST_TO_BENDIAN_INT32(fItem->versionInfo[2]);
				outVersion[3] = B_HOST_TO_BENDIAN_INT32(fItem->versionInfo[3]);
				outVersion[4] = B_HOST_TO_BENDIAN_INT32(fItem->versionInfo[4]);
				
				// write out new-style version info
				data->WriteBuf(fArcFile,(char *)outVersion,sizeof(outVersion),ID_VERSINFO);
				
				WriteArcItemCommon(arcItem);
				bytes += fItem->compressedSize;
			}
			else if (typeid(*arcItem) == typeid(ArchivePatchItem)) {
				ArchivePatchItem *pitem = (ArchivePatchItem *)arcItem;
				
				data->WriteRecordHeader(fArcFile,ID_PATCHITEM,LIS_TYPE,0);
				data->WriteInt32(fArcFile,pitem->mode,ID_MODE);
				data->WriteInt64(fArcFile,pitem->oldFileSize,ID_OLDSIZE);
				WriteArcItemCommon(arcItem);				
				bytes += pitem->compressedSize;
			}
			else if (typeid(*arcItem) == typeid(ArchiveScriptItem)) {
				ArchiveScriptItem *fItem = (ArchiveScriptItem *)arcItem;
				
				data->WriteRecordHeader(fArcFile,ID_SCRIPTITEM,LIS_TYPE,0);
	
				WriteArcItemCommon(arcItem);				
				bytes += fItem->compressedSize;
			}
			else if (typeid(*arcItem) == typeid(ArchiveLinkItem)) {
				ArchiveLinkItem *lItem = (ArchiveLinkItem *)arcItem;
				
				data->WriteRecordHeader(fArcFile,ID_LINKITEM,LIS_TYPE,0);
				data->WriteString(fArcFile,lItem->fLinkPath,ID_LINK);
				WriteArcItemCommon(arcItem);
				bytes += lItem->compressedSize;
			}
			cleanCount++;
		}
	}
	// data->WriteEndHeader(fArcFile);
	data->WriteEndHeader(fArcFile);

	return B_NO_ERROR;
}




// additions are simply appended
// the old catalog (in memory) is written back out
// and the new entries are appended
// optionally the catalog could be stored as a resource?
	
// if the archiveFile is valid send a message to the
// compress BLooper

// this should be called in a separate thread

// in assertion , file has been seeked to the proper position
// compression has been setup
status_t PackArc::AddNode(BNode *inNode, off_t *outSize, node_flavor flavor)
{
	// make sure codec is setup
	// make sure callback is setup
	
	status_t	err;
	off_t		startPos;
		
	ASSERT(fArcFile);
	startPos = fArcFile->Position();

	// compress the archiveitem
	
	err = fPackData->AddNodeEntry(fArcFile, inNode, flavor);
	
	if (err != B_NO_ERROR) {
		// if there was any error or cancelation
		// rewind to the beginning
		fArcFile->Seek(startPos,SEEK_SET);
	}
	*outSize = fArcFile->Position() - startPos;
	return err;
}


// assume the output is opened and seeked to the proper offset
long PackArc::ExtractFile(BNode		*outNode,
							long	numBytes,
							off_t	offset)
{
	numBytes;
	// numBytes is uncompressed size
	// setup decompression
	status_t err;

	ASSERT(fArcFile);
	fArcFile->Seek(offset,SEEK_SET);
	err = fPackData->ExtractNode(fArcFile,outNode);

	return err;
}

status_t PackArc::RemoveItems(RList<ArchiveItem *> *deleteList,
							RList<ArchiveItem *> *cleanList)
{

	ArchiveItem *current;

	// put the items in increasing order of offset	
	off_t		placeOffset;	// offset of to copy data to
	off_t 		startOffset;	// offset to copy data from
	off_t 		skipBytes;		// number of bytes to copy
	off_t 		bytesDeleted;	// total number of bytes removed
	status_t	err;
	int cleanIndex = 0;

	int count = deleteList->CountItems();
	PRINT(("there are %d files to remove\n",count));
	
	int cCount = cleanList->CountItems();
	PRINT(("there are %d offsets to fixup\n",cCount));

	if (count <= 0)
		return B_NO_ERROR;
		
	//
	BFile	f(ArcRef(),O_RDWR);
	
	err = f.InitCheck();
	if (err != B_NO_ERROR) {
		doError("Error opening archive file for deletion. Could not delete.");
	}
	placeOffset = deleteList->ItemAt(0)->seekOffset;
	bytesDeleted = 0;
	
	// 8k copy buffer
	const int		DELETE_BUFSIZ = 8192;
	char *buff = new char[DELETE_BUFSIZ];

	for(long i = 0; i < count; i++) {
		current = deleteList->ItemAt(i);
		
		BMessage upMsg(M_CURRENT_FILENAME);
		upMsg.AddString("filename",current->name);
		statusMessenger.SendMessage(&upMsg);
		
		off_t currentCompressedSize;
		if (current->IsFolder())
			currentCompressedSize = ((ArchiveFolderItem *)current)->dataCompressedSize;
		else
			currentCompressedSize = current->compressedSize;
		
		startOffset = current->seekOffset + currentCompressedSize;
		long nextOffset = (i == count-1) ?
							catalogOffset :
							deleteList->ItemAt(i+1)->seekOffset;
		skipBytes = nextOffset - startOffset;
		PRINT(("item \"%s\" placeOffset %d  startOffset %d  numBytes %d\n",
			current->name, placeOffset, startOffset, skipBytes));
		
		// this loop moves any data back before the next deleted file 
		while(skipBytes > 0) {
			err = f.Seek(startOffset,SEEK_SET);
			err = f.Read(buff,min_c(skipBytes,DELETE_BUFSIZ));
			if (err < B_NO_ERROR) {
				doError("Error reading file during deletion.");
				break;
				// now what!
			}
			startOffset += err;
			f.Seek(placeOffset,SEEK_SET);
			err = f.Write(buff,err);
			if (err < B_NO_ERROR) {
				doError("Error writing file during deletion.");
				break;
				// now what
			}
			skipBytes -= err;
			// increment placeOffset as we copy
			placeOffset += err;
		}
		bucket -= currentCompressedSize;
		while (bucket < 0) {
			PRINT(("sending update message\n"));
			bucket += byteUpdateSize;
			BMessage upMsg(M_UPDATE_PROGRESS);
			upMsg.AddInt32("bytes",byteUpdateSize);
			statusMessenger.SendMessage(&upMsg);
		}
		
		bytesDeleted += currentCompressedSize;

		while(cleanIndex < cCount) {
			ArchiveItem *oldFile = cleanList->ItemAt(cleanIndex);
			if (oldFile->seekOffset < nextOffset) {
				PRINT(("fixing offset \"%s\" old offset %d   new offset %d\n",
					oldFile->name, oldFile->seekOffset, oldFile->seekOffset - bytesDeleted));
				
				oldFile->seekOffset -= bytesDeleted;
				cleanIndex++;
			}
			else if (oldFile->seekOffset == nextOffset) {
				PRINT(("fixing offset \"%s\" old offset %d\n",
					oldFile->name, oldFile->seekOffset));
				doError("Error in deletion, offset to update equals next offset to delete");
				break;
			}
			else {
				//PRINT(("last offset %d\n",cleanIndex-1));
				break;
			}	
		}
	}
	
	// this is a bug if there are no files to remove
	// we fix this with a check at the beginning
	// this may still be buggy!!!
	catalogOffset = placeOffset;
	f.SetSize(catalogOffset);
	
	delete buff;
	return err;
}

extern BBitmap *gGenericFileIcon;

// put cd stuff here

void PackArc::ReadCatalog(ArchiveFolderItem *toplevel)
{
	// check the open state of the file...
	
	SetFile(O_RDONLY);
	fPackData = new SPackData(statusMessenger,&fileLooper->doCancel);	
	
	// seek to the catalog start
	off_t pos = fArcFile->Seek(catalogOffset,SEEK_SET);
	// PRINT(("seeking to catalogOffset %d\n",pos));
	
	record_header	header;
	fPackData->ReadRecordHeader(fArcFile,&header);
	if (header.what != ID_FOLDERITEM || header.type != LIS_TYPE) {
		// unknown catalog item
		doError("Error, expected to find top level folder");
		return;
	}
	
	free(toplevel->name);	
	// read the rest of the entries
	ReadCatalogFolder(NULL,toplevel);
	toplevel->destination = D_INSTALL_FOLDER;

	ClearFile();
	delete fPackData;
}




// inside open file, seek to catalog offset, read top folder
void PackArc::ReadCatalogFolder(ArchiveFolderItem *parent, ArchiveFolderItem *toplevel)
{
	bool		isCD = arcFlags & CD_INSTALL;

	PackData	*data = fPackData;
	record_header	header;
	status_t		err;	
	
	PRINT(("is folder\n"));
			
	ArchiveFolderItem *newFolder;
	if (parent)
		newFolder = new ArchiveFolderItem();
	else {
		newFolder = toplevel;
	}
	
	newFolder->dirty = FALSE;
	newFolder->SetParent(parent);
//	newFolder->numEntries = 0;
	newFolder->canEnter = TRUE;

	// read the entries for this folder	
	bool readingEntries = true;
	while (readingEntries) {
		data->ReadRecordHeader(fArcFile,&header);
		ArchiveFileItem *newFile;
		ArchivePatchItem *newPatch;
		ArchiveScriptItem *newScript;
		ArchiveLinkItem *newLink;
		switch (header.what) {
			case ID_FOLDERDATA: {
				/// data items
				record_header	header;
				bool readingEntries = true;
				while (readingEntries) {
					data->ReadRecordHeader(fArcFile,&header);
					switch (header.what) {
						case ID_NAME:
							err = data->ReadString(fArcFile,&newFolder->name);
							break;
						case ID_DEST:
							err = data->ReadInt32(fArcFile,&newFolder->destination);
							break;
						case ID_CUST:
							int32 c;
							err = data->ReadInt32(fArcFile,&c);
							newFolder->customDest = c;
							break;
						case ID_REPL:
							err = data->ReadInt32(fArcFile,&newFolder->replacement);
							break;
						/// fix up time format!!
						case ID_CTIME:
							err = data->ReadInt32(fArcFile,&newFolder->creationTime);
							break;
						case ID_MTIME:
							err = data->ReadInt32(fArcFile,&newFolder->modTime);
							break;
						case ID_PLAT:
							err = data->ReadInt32(fArcFile,(int32 *)&newFolder->platform);
							break;

						/// fix up vers format
						//case ID_VERS:
							//err = data->ReadInt32(fArcFile,&newFolder->version);
						//	break;
						// fix up groups format
						case ID_GROUPS:
							err = data->ReadInt32(fArcFile,(int32 *)&newFolder->groups);
							break;
						/// list items
						case ID_OFFSET:
							err = data->ReadInt64(fArcFile,&newFolder->seekOffset);
							if (newFolder->seekOffset >= 0)
								newFolder->numCompressibleItems++;
							break;
						case ID_CMPSIZE:
							err = data->ReadInt64(fArcFile,&newFolder->dataCompressedSize);
							parent->compressedSize += newFolder->dataCompressedSize;
							break;
						case END_TYPE:
							readingEntries = false;
							break;
						default:
							err = data->SkipData(fArcFile,&header);
					}
				}
				break;
			}
			case ID_FILEITEM:
			case ID_PATCHITEM:
			case ID_SCRIPTITEM: 
			case ID_LINKITEM: {
				ArchiveItem *item(NULL);
				switch (header.what) {
					case ID_FILEITEM:
						newFile = new ArchiveFileItem();
						ReadCatalogFile(newFile);
						item = newFile;
						break;
					case ID_PATCHITEM:
						newPatch = new ArchivePatchItem();
						ReadCatalogPatch(newPatch);
						item = newPatch;
						break;
					case ID_SCRIPTITEM:				
						newScript = new ArchiveScriptItem();
						ReadCatalogScript(newScript);
						item = newScript;
						break;
					case ID_LINKITEM:
						newLink = new ArchiveLinkItem();
						ReadCatalogLink(newLink);
						item = newLink;
						break;
					default:
						err = data->SkipData(fArcFile,&header);
						break;
				}
//				newFolder->numEntries++;				
				item->SetParent(newFolder);
				newFolder->AddEntry(item);
				newFolder->numCompressibleItems++;
				newFolder->compressedSize += item->compressedSize;
				newFolder->uncompressedSize += item->uncompressedSize;
				newFolder->groups |= item->groups;
				break;
			}
			case ID_FOLDERITEM:
				ReadCatalogFolder(newFolder);
//				newFolder->numEntries++;
				break;
			case END_TYPE:
				readingEntries = false;
				break;
			default:
				err = data->SkipData(fArcFile,&header);
		}
	}
	
	if (parent) {
		parent->AddEntry(newFolder);
		parent->numCompressibleItems += newFolder->numCompressibleItems;
		parent->compressedSize += newFolder->compressedSize;
		parent->uncompressedSize += newFolder->uncompressedSize;
		// set group bitmap for parent
		parent->groups |= newFolder->groups;
	}
}

void PackArc::ReadCatalogFile(ArchiveFileItem *newFile)
{
	PRINT(("is file\n"));
	PackData		*data = fPackData;
	record_header	header;
	status_t		err;
	
	newFile->dirty = FALSE;
	newFile->smallIcon = gGenericFileIcon;

	bool readingEntries = true;
	while (readingEntries) {
		fPackData->ReadRecordHeader(fArcFile,&header);
		switch (header.what) {
			/// data items
			case ID_NAME:
				err = data->ReadString(fArcFile,&newFile->name);
				break;
			case ID_MIME:
				err = data->ReadString(fArcFile,&newFile->fType);
				break;
			case ID_APP_SIG:
				err = data->ReadString(fArcFile,&newFile->fSignature);
				break;
			case ID_MODE:
				err = data->ReadInt32(fArcFile,(int32 *)&newFile->mode);
				break;
			case ID_DEST:
				err = data->ReadInt32(fArcFile,&newFile->destination);
				break;
			case ID_CUST:
				int32 c;
				err = data->ReadInt32(fArcFile,&c);
				newFile->customDest = c;
				break;
			case ID_REPL:
				err = data->ReadInt32(fArcFile,&newFile->replacement);
				break;
			/// fix up time format!!
			case ID_CTIME:
				err = data->ReadInt32(fArcFile,&newFile->creationTime);
				break;
			case ID_MTIME:
				err = data->ReadInt32(fArcFile,&newFile->modTime);
				break;
			/// fix up vers format
			//case ID_VERS:
			//	err = data->ReadInt32(fArcFile,&newFile->version);
			//	break;
			// fix up groups format
			case ID_VERSINFO:
				// read new-style version info
				err = data->ReadBuf(fArcFile, (char *)newFile->versionInfo,
									sizeof(newFile->versionInfo));
				// de-neutralize version info byte-order
				newFile->versionInfo[0] = B_BENDIAN_TO_HOST_INT32(newFile->versionInfo[0]);
				newFile->versionInfo[1] = B_BENDIAN_TO_HOST_INT32(newFile->versionInfo[1]);
				newFile->versionInfo[2] = B_BENDIAN_TO_HOST_INT32(newFile->versionInfo[2]);
				newFile->versionInfo[3] = B_BENDIAN_TO_HOST_INT32(newFile->versionInfo[3]);
				newFile->versionInfo[4] = B_BENDIAN_TO_HOST_INT32(newFile->versionInfo[4]);
				
				PRINT(("***********************\nVersionInfo %d.%d.%d %d/%d\n",
							newFile->versionInfo[0],
							newFile->versionInfo[1],
							newFile->versionInfo[2],
							newFile->versionInfo[3],
							newFile->versionInfo[4]));
				break;
			case ID_PLAT:
				err = data->ReadInt32(fArcFile,(int32 *)&newFile->platform);
				break;
			case ID_GROUPS:
				err = data->ReadInt32(fArcFile,(int32 *)&newFile->groups);
				break;
			// offset stuff
			case ID_OFFSET:
				err = data->ReadInt64(fArcFile,&newFile->seekOffset);
				break;
			case ID_CMPSIZE:
				err = data->ReadInt64(fArcFile,&newFile->compressedSize);
				break;
			case ID_ORGSIZE:
				err = data->ReadInt64(fArcFile,&newFile->uncompressedSize);
				break;
			case END_TYPE:
				readingEntries = false;
				break;
			default:
				err = data->SkipData(fArcFile,&header);
		}
	}
}

void PackArc::ReadCatalogScript(ArchiveScriptItem *newFile)
{
	PRINT(("is file\n"));
	PackData		*data = fPackData;
	record_header	header;
	status_t		err;
	
	newFile->dirty = FALSE;
	// newFile->smallIcon = gGenericFileIcon;

	bool readingEntries = true;
	while (readingEntries) {
		fPackData->ReadRecordHeader(fArcFile,&header);
		switch (header.what) {
			/// data items
			case ID_NAME:
				err = data->ReadString(fArcFile,&newFile->name);
				break;
			case ID_DEST:
				err = data->ReadInt32(fArcFile,&newFile->destination);
				break;
			case ID_CUST:
				int32 c;
				err = data->ReadInt32(fArcFile,&c);
				newFile->customDest = c;
				break;
			// fix up groups format
			case ID_GROUPS:
				err = data->ReadInt32(fArcFile,(int32 *)&newFile->groups);
				break;
			// offset stuff
			case ID_OFFSET:
				err = data->ReadInt64(fArcFile,&newFile->seekOffset);
				break;
			case ID_CMPSIZE:
				err = data->ReadInt64(fArcFile,&newFile->compressedSize);
				break;
			case ID_ORGSIZE:
				err = data->ReadInt64(fArcFile,&newFile->uncompressedSize);
				break;
			case ID_PLAT:
				err = data->ReadInt32(fArcFile,(int32 *)&newFile->platform);
				break;
			case END_TYPE:
				readingEntries = false;
				break;
			default:
				err = data->SkipData(fArcFile,&header);
		}
	}
}

void PackArc::ReadCatalogPatch(ArchivePatchItem *newFile)
{
	PRINT(("is patch\n"));
	PackData		*data = fPackData;
	record_header	header;
	status_t		err;
	
	newFile->dirty = FALSE;

	bool readingEntries = true;
	while (readingEntries) {
		fPackData->ReadRecordHeader(fArcFile,&header);
		switch (header.what) {
			/// data items
			case ID_NAME:
				err = data->ReadString(fArcFile,&newFile->name);
				break;
			//case ID_MIME:
			//	err = data->ReadString(fArcFile,&newFile->fType);
			//	break;
			case ID_MODE:
				err = data->ReadInt32(fArcFile,(int32 *)&newFile->mode);
				break;
			case ID_DEST:
				err = data->ReadInt32(fArcFile,&newFile->destination);
				break;
			case ID_CUST:
				int32 c;
				err = data->ReadInt32(fArcFile,&c);
				newFile->customDest = c;
				break;
			//case ID_REPL:
			//	err = data->ReadInt32(fArcFile,&newFile->replacement);
			//	break;
			/// fix up time format!!
			case ID_CTIME:
				err = data->ReadInt32(fArcFile,&newFile->creationTime);
				break;
			case ID_MTIME:
				err = data->ReadInt32(fArcFile,&newFile->modTime);
				break;
			/// fix up vers format
			//case ID_VERS:
			//	err = data->ReadInt32(fArcFile,&newFile->version);
			//	break;
			// fix up groups format
			case ID_GROUPS:
				err = data->ReadInt32(fArcFile,(int32 *)&newFile->groups);
				break;
			// offset stuff
			case ID_OFFSET:
				err = data->ReadInt64(fArcFile,&newFile->seekOffset);
				break;
			case ID_CMPSIZE:
				err = data->ReadInt64(fArcFile,&newFile->compressedSize);
				break;
			case ID_ORGSIZE:
				err = data->ReadInt64(fArcFile,&newFile->uncompressedSize);
				break;
			case ID_OLDSIZE:
				err = data->ReadInt64(fArcFile,&newFile->oldFileSize);
				break;
			case ID_PLAT:
				err = data->ReadInt32(fArcFile,(int32 *)&newFile->platform);
				break;
			case END_TYPE:
				readingEntries = false;
				break;
			default:
				err = data->SkipData(fArcFile,&header);
		}
	}
}

void PackArc::ReadCatalogLink(ArchiveLinkItem *newFile)
{
	PRINT(("is link\n"));
	PackData		*data = fPackData;
	record_header	header;
	status_t		err;
	
	newFile->dirty = false;

	bool readingEntries = true;
	while (readingEntries) {
		fPackData->ReadRecordHeader(fArcFile,&header);
		switch (header.what) {
			/// data items
			/// data items
			case ID_NAME:
				err = data->ReadString(fArcFile,&newFile->name);
				break;
			case ID_LINK:
				err = data->ReadString(fArcFile,&newFile->fLinkPath);
				break;
			//case ID_MODE:
			//	err = data->ReadInt32(fArcFile,(int32 *)&newFile->mode);
			//	break;
			case ID_DEST:
				err = data->ReadInt32(fArcFile,&newFile->destination);
				break;
			case ID_CUST:
				int32 c;
				err = data->ReadInt32(fArcFile,&c);
				newFile->customDest = c;
				break;
			case ID_REPL:
				err = data->ReadInt32(fArcFile,&newFile->replacement);
				break;
			case ID_CTIME:
				err = data->ReadInt32(fArcFile,&newFile->creationTime);
				break;
			case ID_MTIME:
				err = data->ReadInt32(fArcFile,&newFile->modTime);
				break;
			case ID_PLAT:
				err = data->ReadInt32(fArcFile,(int32 *)&newFile->platform);
				break;
			case ID_GROUPS:
				err = data->ReadInt32(fArcFile,(int32 *)&newFile->groups);
				break;
			// offset stuff
			case ID_OFFSET:
				err = data->ReadInt64(fArcFile,&newFile->seekOffset);
				break;
			case ID_CMPSIZE:
				err = data->ReadInt64(fArcFile,&newFile->compressedSize);
				break;
			case ID_ORGSIZE:
				err = data->ReadInt64(fArcFile,&newFile->uncompressedSize);
				break;
			case END_TYPE:
				readingEntries = false;
				break;
			default:
				err = data->SkipData(fArcFile,&header);
		}
	}
}
