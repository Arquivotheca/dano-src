#ifndef _PLAY_LIST_HANDLER_H
#define _PLAY_LIST_HANDLER_H

#include <ObjectList.h>

class PlayListHandler;
class BPositionIO;
class PlayListHandlerInfo;

typedef PlayListHandler* (*PlayListInstantiateFunc)(BPositionIO *stream);

struct PlayListHandlerInfo {
	PlayListInstantiateFunc Instantiate;
};

void RegisterPlayListHandlers();

class PlayListHandler {
public:
	static PlayListHandler* InstantiateHandler(BPositionIO *stream);
	static void RegisterHandler(PlayListInstantiateFunc);

	PlayListHandler(BPositionIO *stream);
	virtual ~PlayListHandler();
	virtual status_t GetNextFile(char *out_url, size_t size);	
	virtual void Rewind();
	BPositionIO *Stream();

private:
	BPositionIO *fStream;
	static BObjectList<PlayListHandlerInfo> *fPlayListHandlers;
};

#endif

