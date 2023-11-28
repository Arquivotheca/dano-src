#include "ArcCatalog.h"

#include "SPackData.h"
#include "IArchiveItem.h"
#include "IArchivePatchItem.h"
#include "IArchiveScriptItem.h"

#include "InstallPack.h"

#include "DataID.h"

#include "MyDebug.h"
#include "Util.h"


ArcCatalog::ArcCatalog()
	:	fArcFile(0)
{	
	newFile = new ArchiveFileItem();
	newFolder = new ArchiveFolderItem();
	newPatch = new ArchivePatchItem();
	newScript = new ArchiveScriptItem();
	newLink = new ArchiveLinkItem();
	
	newFile->name = (char *)malloc(B_FILE_NAME_LENGTH);
	newFile->fType = (char *)malloc(B_FILE_NAME_LENGTH);
	
	newFolder->name = (char *)malloc(B_FILE_NAME_LENGTH);
	newPatch->name = (char *)malloc(B_FILE_NAME_LENGTH);
	newScript->name = (char *)malloc(B_FILE_NAME_LENGTH);
	
	newLink->name = (char *)malloc(B_FILE_NAME_LENGTH);
	newLink->linkPath = (char *)malloc(PATH_MAX);
}

ArcCatalog::~ArcCatalog()
{
	delete fArcFile;
	
	delete newFile;
	delete newFolder;
	delete newPatch;
	delete newScript;
	delete newLink;
}

status_t	ArcCatalog::Set(const InstallPack *pack)
{
	if (fArcFile)
		delete fArcFile;
	fArcFile = new BFile();
	*fArcFile = *pack->fArcFile;
	fPackData = pack->fPackData;
	fOffset = pack->catalogOffset;
	
	return B_NO_ERROR;
	// return fArcFile->Seek(offset,SEEK_SET);
}

status_t	ArcCatalog::Unset()
{
	delete fArcFile;
	fArcFile = NULL;
	return B_NO_ERROR;
}

status_t	ArcCatalog::Rewind()
{
	record_header	header;
	status_t		err = B_NO_ERROR;

	fArcFile->Seek(fOffset, SEEK_SET);

	fPackData->ReadRecordHeader(fArcFile,&header);	
	if (header.what != ID_FOLDERITEM || header.type != LIS_TYPE) {
		// unknown catalog item
		return B_ERROR;
	}
	return B_NO_ERROR;
}

void	ArcCatalog::GetFolderData(ArchiveItem **item)
{
	PackData	*data = fPackData;
	record_header	header;
	status_t		err;


	data->ReadRecordHeader(fArcFile,&header);	
	if (header.what != ID_FOLDERDATA || header.type != LIS_TYPE) {
		// unknown catalog item
		// doError("Error, expected to find top level folder");
		*item = NULL;		
		return;
	}
	
	newFolder->Unset();
	bool readingEntries = true;
	while (readingEntries) {
		data->ReadRecordHeader(fArcFile,&header);
		switch (header.what) {
			/// data items
			case ID_NAME:
				err = data->ReadString(fArcFile,&newFolder->name,B_FILE_NAME_LENGTH);
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
			case ID_OFFSET:
				err = data->ReadInt64(fArcFile,&newFolder->seekOffset);
				break;
			/// fix up vers format
			//case ID_VERS:
			//	err = data->ReadInt32(fArcFile,&newFolder->version);
			//	break;
			// fix up groups format
			case ID_GROUPS:
				err = data->ReadInt32(fArcFile,(int32 *)&newFolder->groups);
				break;
			case ID_PLAT:
				err = data->ReadInt32(fArcFile,&newFolder->platform);
				break;
			case END_TYPE:
				readingEntries = false;
				break;
			default:
				err = data->SkipData(fArcFile,&header);
		}
	}
	
	*item = newFolder;
	return;
}

status_t	ArcCatalog::StartFolderGet()
{

	// perpare for get folder subitem type
	//fPackData->ReadRecordHeader(fArcFile,&header);	
	//if (header.what != ID_FOLDITEMS || header.type != LIS_TYPE) {
		// unknown catalog item
		// doError("Error, expected to find top level folder");		
		//return B_ERROR;
//	}
	return B_NO_ERROR;
}

status_t	ArcCatalog::EndFolderGet()
{
	// skip extra list items until list is finished
	//record_header	header;
	//bool readingEntries = true;
	//while (readingEntries) {
		//fPackData->ReadRecordHeader(fArcFile,&header);
		//if (header.what == END_TYPE)
		//	readingEntries = false;
		//else
		//	fPackData->SkipData(fArcFile,&header);
	//}
	
	return B_NO_ERROR;
}

int32	ArcCatalog::GetFolderItemType()
{
	record_header	header;
	status_t		err = B_NO_ERROR;

	for (;;) {
		fPackData->ReadRecordHeader(fArcFile,&header);
		if (header.type == LIS_TYPE) {
			switch(header.what) {
				case ID_FOLDERITEM:
					return FOLDER_FLAG;
				case ID_FILEITEM:
					return FILE_FLAG;
				case ID_PATCHITEM:
					return PATCH_FLAG;
				case ID_SCRIPTITEM:
					return SCRIPT_FLAG;
				case ID_LINKITEM:
					return LINK_FLAG;
			}
		}
		else if (header.what == END_TYPE)
			return 0;
		else	
			fPackData->SkipData	(fArcFile,&header);
	}
	return 0;
}

void	ArcCatalog::GetItemData(ArchiveItem **item, int32 flag)
{
	switch(flag) {
		case FOLDER_FLAG:
			GetFolderData(item);
			return;
		case FILE_FLAG:
			GetFileData(item);
			return;
		case PATCH_FLAG:
			GetPatchData(item);
			return;
		case SCRIPT_FLAG:
			GetScriptData(item);
			return;
		case LINK_FLAG:
			GetLinkData(item);
			return;
		default:
			;
	}
}


void	ArcCatalog::GetLinkData(ArchiveItem **item)
{
	PRINT(("is link\n"));
	PackData		*data = fPackData;
	record_header	header;
	status_t		err;
	
	newLink->Unset();
	bool readingEntries = true;
	while (readingEntries) {
		fPackData->ReadRecordHeader(fArcFile,&header);
		switch (header.what) {
			/// data items
			case ID_NAME:
				err = data->ReadString(fArcFile,&newLink->name,B_FILE_NAME_LENGTH);
				break;
			case ID_LINK:
				err = data->ReadString(fArcFile,&newLink->linkPath,PATH_MAX);
				break;
			case ID_MODE:
				err = data->ReadInt32(fArcFile,(int32 *)&newLink->mode);
				break;
			case ID_DEST:
				err = data->ReadInt32(fArcFile,&newLink->destination);
				break;
			case ID_CUST:
				int32 c;
				err = data->ReadInt32(fArcFile,&c);
				newLink->customDest = c;
				break;
			case ID_REPL:
				err = data->ReadInt32(fArcFile,&newLink->replacement);
				break;
			case ID_CTIME:
				err = data->ReadInt32(fArcFile,&newLink->creationTime);
				break;
			case ID_MTIME:
				err = data->ReadInt32(fArcFile,&newLink->modTime);
				break;
			case ID_GROUPS:
				err = data->ReadInt32(fArcFile,(int32 *)&newLink->groups);
				break;
			case ID_PLAT:
				err = data->ReadInt32(fArcFile,&newLink->platform);
				break;
			// offset stuff
			case ID_OFFSET:
				err = data->ReadInt64(fArcFile,&newLink->seekOffset);
				break;
			//case ID_CMPSIZE:
			//	err = data->ReadInt64(fArcFile,&newFile->compressedSize);
			//	break;
			case ID_ORGSIZE:
				err = data->ReadInt64(fArcFile,&newLink->uncompressedSize);
				break;
			case END_TYPE:
				readingEntries = false;
				break;
			default:
				err = data->SkipData(fArcFile,&header);
		}
	}
	*item = newLink;
}

void	ArcCatalog::GetFileData(ArchiveItem **item)
{
	PRINT(("is file\n"));
	PackData		*data = fPackData;
	record_header	header;
	status_t		err;

	newFile->Unset();
	bool readingEntries = true;
	while (readingEntries) {
		fPackData->ReadRecordHeader(fArcFile,&header);
		switch (header.what) {
			/// data items
			case ID_NAME:
				err = data->ReadString(fArcFile,&newFile->name,B_FILE_NAME_LENGTH);
				break;
			case ID_MIME:
				err = data->ReadString(fArcFile,&newFile->fType,B_FILE_NAME_LENGTH);
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
				break;
			case ID_GROUPS:
				err = data->ReadInt32(fArcFile,(int32 *)&newFile->groups);
				break;
			case ID_PLAT:
				err = data->ReadInt32(fArcFile,&newFile->platform);
				break;
			// offset stuff
			case ID_OFFSET:
				err = data->ReadInt64(fArcFile,&newFile->seekOffset);
				break;
			//case ID_CMPSIZE:
			//	err = data->ReadInt64(fArcFile,&newFile->compressedSize);
			//	break;
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
	*item = newFile;
}

void	ArcCatalog::GetScriptData(ArchiveItem **item)
{
	PRINT(("is script\n"));
	PackData		*data = fPackData;
	record_header	header;
	status_t		err;

	newScript->Unset();
	bool readingEntries = true;
	while (readingEntries) {
		fPackData->ReadRecordHeader(fArcFile,&header);
		switch (header.what) {
			/// data items
			case ID_NAME:
				err = data->ReadString(fArcFile,&newScript->name,B_FILE_NAME_LENGTH);
				break;
			case ID_DEST:
				err = data->ReadInt32(fArcFile,&newScript->destination);
				break;
			case ID_CUST:
				int32 c;
				err = data->ReadInt32(fArcFile,&c);
				newScript->customDest = c;
				break;
			case ID_GROUPS:
				err = data->ReadInt32(fArcFile,(int32 *)&newScript->groups);
				break;
			case ID_PLAT:
				err = data->ReadInt32(fArcFile,&newScript->platform);
				break;
			// offset stuff
			case ID_OFFSET:
				err = data->ReadInt64(fArcFile,&newScript->seekOffset);
				break;
			//case ID_CMPSIZE:
			//	err = data->ReadInt64(fArcFile,&newScript->compressedSize);
			//	break;
			case ID_ORGSIZE:
				err = data->ReadInt64(fArcFile,&newScript->uncompressedSize);
				break;
			case END_TYPE:
				readingEntries = false;
				break;
			default:
				err = data->SkipData(fArcFile,&header);
		}
	}
	*item = newScript;
}

void	ArcCatalog::GetPatchData(ArchiveItem **item)
{
	PRINT(("is patch\n"));
	PackData		*data = fPackData;
	record_header	header;
	status_t		err;

	newPatch->Unset();
	bool readingEntries = true;
	while (readingEntries) {
		fPackData->ReadRecordHeader(fArcFile,&header);
		switch (header.what) {
			/// data items
			case ID_NAME:
				err = data->ReadString(fArcFile,&newPatch->name,B_FILE_NAME_LENGTH);
				break;
			case ID_MODE:
				err = data->ReadInt32(fArcFile,(int32 *)&newPatch->mode);
				break;
			case ID_DEST:
				err = data->ReadInt32(fArcFile,&newPatch->destination);
				break;
			case ID_CUST:
				int32 c;
				err = data->ReadInt32(fArcFile,&c);
				newPatch->customDest = c;
				break;
			/// fix up time format!!
			case ID_CTIME:
				err = data->ReadInt32(fArcFile,&newPatch->creationTime);
				break;
			case ID_MTIME:
				err = data->ReadInt32(fArcFile,&newPatch->modTime);
				break;
			/// fix up vers format
			//case ID_VERS:
			//	err = data->ReadInt32(fArcFile,&newPatch->version);
			//	break;
			// fix up groups format
			case ID_GROUPS:
				err = data->ReadInt32(fArcFile,(int32 *)&newPatch->groups);
				break;
			case ID_PLAT:
				err = data->ReadInt32(fArcFile,&newPatch->platform);
				break;
			// offset stuff
			case ID_OFFSET:
				err = data->ReadInt64(fArcFile,&newPatch->seekOffset);
				break;
			//case ID_CMPSIZE:
			//	err = data->ReadInt64(fArcFile,&newPatch->compressedSize);
			//	break;
			case ID_ORGSIZE:
				err = data->ReadInt64(fArcFile,&newPatch->uncompressedSize);
				break;
			case ID_OLDSIZE:
				err = data->ReadInt64(fArcFile,&newPatch->oldFileSize);
				break;
			case END_TYPE:
				readingEntries = false;
				break;
			default:
				err = data->SkipData(fArcFile,&header);
		}
	}
	*item = newPatch;
}
