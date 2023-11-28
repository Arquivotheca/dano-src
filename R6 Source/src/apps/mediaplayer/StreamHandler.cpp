#include <Debug.h>
#include "debug.h"
#include "StreamHandler.h"

BObjectList<StreamHandlerInfo> *StreamHandler::fStreamHandlers = 0;

StreamHandler::StreamHandler()
{
}

StreamHandler::~StreamHandler()
{
}

status_t StreamHandler::SetTo(const URL&, BString &)
{
	return B_OK;
}

status_t StreamHandler::Unset()
{
	return B_OK;
}

ssize_t StreamHandler::ReadAt(off_t, void*, size_t)
{
	return B_ERROR;;
}

bool StreamHandler::IsDataAvailable(off_t, size_t)
{
	return true;
}

void StreamHandler::SetCookies(const char *)
{
}


off_t StreamHandler::Seek(off_t, uint32)
{
	return -1;
}

off_t StreamHandler::Position() const
{
	return -1;
}

off_t StreamHandler::GetDownloadedSize() const
{
	return -1;
}

size_t StreamHandler::GetLength() const
{
	return 0;
}

ssize_t StreamHandler::WriteAt(off_t, const void*, size_t)
{
	debugger("should not be here: HTTPStream::WriteAt()\n");
	return B_ERROR;
}

bool StreamHandler::IsContinuous() const
{
	return false;
}

bool StreamHandler::IsBuffered() const
{
	return false;
}

const char* StreamHandler::GetConnectionName()
{
	return "";
}

StreamHandler* StreamHandler::InstantiateStreamHandler(const char *scheme)
{
	if (fStreamHandlers == 0)
		return 0;

	writelog("Find handler for %s\n", scheme);
	for (int32 index = 0; index < fStreamHandlers->CountItems(); index++)
		if (fStreamHandlers->ItemAt(index)->fScheme.ICompare(scheme) == 0)
			return fStreamHandlers->ItemAt(index)->Instantiate(scheme);
		
	return 0;
}

void StreamHandler::RegisterStreamHandler(const char *scheme,
	StreamHandlerInstantiateFunc instantiate)
{
	if (fStreamHandlers == 0)
		fStreamHandlers = new BObjectList<StreamHandlerInfo>;

	StreamHandlerInfo *info = new StreamHandlerInfo;
	info->fScheme = scheme;
	info->Instantiate = instantiate;	
	fStreamHandlers->AddItem(info);
}

void StreamHandler::DescribeConnection(BString&)
{
}

float StreamHandler::BufferUtilization() const
{
	return 1.0;
}

void StreamHandler::GetStats(struct player_stats *)
{
}




