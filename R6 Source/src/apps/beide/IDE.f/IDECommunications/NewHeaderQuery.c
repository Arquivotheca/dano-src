#include "BeIDEComm.h"

long
DoHeaderQuery(
	port_id 		wrPort,
	port_id 		rdPort,
	const char * 	fileNameIn,
	bool 			inSysTreeIn,
	char * 			filePathOut,
	FileHandle*		fileHandle);

char *	GetText(FileHandle inHandle);

area_id		gIDESharedArea = -1;
area_id		gCompilerSharedArea = -1;
void*		gAreaBase;

//	When the compiler sees an include file, it should call this
//	function to find the location of the include file, or get an
//	error code if the file can't be found. If the include is with
//	brackets, inSysTreeIn should be TRUE.
//	Returns a pathname to use for opening the file.
//

long
DoHeaderQuery(
	port_id 		wrPort,
	port_id 		rdPort,
	const char * 	fileNameIn,
	bool 			inSysTreeIn,
	char * 			filePathOut,
	FileHandle*		fileHandle)
{
	HeaderQuery message;
	HeaderReply reply;

	ASSERT(strlen(fileNameIn) < B_FILE_NAME_LENGTH);
	strncpy(message.fileName, fileNameIn, B_FILE_NAME_LENGTH);
	message.fileName[B_FILE_NAME_LENGTH-1] = 0;
	message.inSysTree = inSysTreeIn;

	long err = write_port(wrPort, kHeaderQuery, &message, sizeof(message));
	long id = kNullQuery;
	if (!err) {
		err = read_port(rdPort, &id, &reply, sizeof(reply));
		if (err > B_ERROR)
			err = B_NO_ERROR;
	}
	if (!err) {
		ASSERT(id == kHeaderReply);
		err = reply.errorCode;
	}
	if (!err) {
		ASSERT(strlen(reply.filePath) < MAX_PATH_LENGTH);
		strncpy(filePathOut, reply.filePath, MAX_PATH_LENGTH);
		filePathOut[MAX_PATH_LENGTH-1] = 0;
	}
	if (err)
		fileHandle->id = -1;
	else
	{
		*fileHandle = reply.handle;
	}

	return err;
}

#define FileHeader(inOffset) \
	((SharedFileRec*) ((char*) gAreaBase + (inOffset)))

//	Get a malloced block that holds the contents of the file represented
//	by the FileHandle.  Will return nil if errors occur.

char *
GetText(
	FileHandle		inFileHandle)
{
	char *		result = NULL;

	ASSERT(inFileHandle.id > B_ERROR);

	if (inFileHandle.id > B_ERROR)
	{
		// Save the cloned area for susequent uses
		if (gIDESharedArea != inFileHandle.id)
		{
			gAreaBase = (void*) B_PAGE_SIZE;
			gCompilerSharedArea = clone_area("compilerarea", &gAreaBase, B_ANY_ADDRESS, B_READ_AREA, inFileHandle.id);

			if (gCompilerSharedArea > B_ERROR)
			{
				gIDESharedArea = inFileHandle.id;
			}
		}
		
		if (gCompilerSharedArea > B_ERROR)
		{
			SharedFileRec*		header = FileHeader(inFileHandle.offset);
			char*				text = (char*) header + sizeof(SharedFileRec);
			result = (char*) malloc(header->length);
			
			if (result != NULL)
				memcpy(result, text, header->length);
		}
	}
	
	return result;
}