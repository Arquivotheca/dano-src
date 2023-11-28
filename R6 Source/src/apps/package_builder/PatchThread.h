
// PatchThread.h
#include "MThread.h"

#ifndef _PATCHTHREAD_H
#define _PATCHTHREAD_H

class FilePatcher;
class PackWindow;
class ArchivePatchItem;

class PatchThread : public MThread
{
public:
	PatchThread(entry_ref &o, entry_ref &n,
				PackWindow *_pw,
				BMessage *_looperMsg,
				BLooper *_fileLooper,
				ArchivePatchItem *_arcItem,
				BMessenger _viewMess);
	
	~PatchThread();
	virtual long	Execute();
	
private:
	FilePatcher *patcher;
	BMessage	*looperMsg;
	BLooper		*fileLooper;
	PackWindow	*wind;
	ArchivePatchItem	*arcItem;
	BMessenger	viewMess;
};

#endif
