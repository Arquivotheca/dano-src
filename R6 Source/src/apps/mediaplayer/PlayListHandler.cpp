#include <DataIO.h>
#include <string.h>
#include "PlayListHandler.h"
#include "debug.h"

BObjectList<PlayListHandlerInfo>* PlayListHandler::fPlayListHandlers = 0;

PlayListHandler* PlayListHandler::InstantiateHandler(BPositionIO *stream)
{
	if (fPlayListHandlers == 0)
		return 0;

	for (int32 handlerNum = 0; handlerNum < fPlayListHandlers->CountItems();
		handlerNum++) {
		PlayListHandler *handler = fPlayListHandlers->ItemAt(handlerNum)->Instantiate(stream);
		if (handler)
			return handler;
	}

	return 0;
}

void PlayListHandler::RegisterHandler(PlayListInstantiateFunc instantiate)
{
	if (fPlayListHandlers == 0)
		fPlayListHandlers = new BObjectList<PlayListHandlerInfo>;

	PlayListHandlerInfo *info = new PlayListHandlerInfo;
	info->Instantiate = instantiate;
	fPlayListHandlers->AddItem(info);	
}

PlayListHandler::PlayListHandler(BPositionIO *stream)
	:	fStream(stream)
{
}

PlayListHandler::~PlayListHandler()
{
	delete fStream;
}

status_t PlayListHandler::GetNextFile(char*, size_t)
{
	return B_ERROR;
}

void PlayListHandler::Rewind()
{
}

BPositionIO* PlayListHandler::Stream()
{
	return fStream;
}

class M3UHandler : public PlayListHandler {
public:
	static PlayListHandler *InstantiateHandler(BPositionIO *stream);
	M3UHandler(BPositionIO *stream);
	virtual ~M3UHandler();
	virtual status_t GetNextFile(char *out_url, size_t size);	
	virtual void Rewind();
};

class SCPLSHandler : public PlayListHandler {
public:

	static PlayListHandler *InstantiateHandler(BPositionIO *stream);
	SCPLSHandler(BPositionIO *stream);
	virtual ~SCPLSHandler();
	virtual status_t GetNextFile(char *out_url, size_t size);	
	virtual void Rewind();
};


PlayListHandler* M3UHandler::InstantiateHandler(BPositionIO *stream)
{
	char urlStart[32];
	stream->ReadAt(0, urlStart, 7);
	if (strncmp(urlStart, "http://", 7) == 0)
		return new M3UHandler(stream);

	return 0;
}

M3UHandler::M3UHandler(BPositionIO *stream)
	:	PlayListHandler(stream)
{
	stream->Seek(0LL, SEEK_SET);
}


M3UHandler::~M3UHandler()
{
}

status_t M3UHandler::GetNextFile(char *out_url, size_t size)
{
	Stream()->ReadAt(0, out_url, size);
	for (unsigned int i = 0; i < size; i++) {
		if (out_url[i] == '\n') {
			out_url[i] = 0;
			break;
		}
	}
	
	return B_OK;
}

void M3UHandler::Rewind()
{
}

PlayListHandler* SCPLSHandler::InstantiateHandler(BPositionIO *stream)
{
	const char *identifier = "[playlist]\n";
	char str[20];
	stream->ReadAt(0, str, strlen(identifier));
	if (strncmp(identifier, str, strlen(identifier)) == 0)
		return new SCPLSHandler(stream);

	return 0;
}


SCPLSHandler::SCPLSHandler(BPositionIO *stream)
	:	PlayListHandler(stream)
{
}


SCPLSHandler::~SCPLSHandler()
{
}

status_t SCPLSHandler::GetNextFile(char *out_url, size_t)
{
	char data[1024];
	Stream()->ReadAt(0, data, 1024);
	char *filestart = strstr(data, "File1=");
	if (!filestart)
		return B_ERROR;
		
	char *url = filestart + 6;
	while (*url && *url != '\n')
		*out_url++ = *url++;
		
	*out_url = '\0';
	return B_OK;
}

void 
SCPLSHandler::Rewind()
{
}



void RegisterPlayListHandlers()
{
	PlayListHandler::RegisterHandler(M3UHandler::InstantiateHandler);
	PlayListHandler::RegisterHandler(SCPLSHandler::InstantiateHandler);
}


