// HeapWindow.h

#ifndef HEAP_WINDOW_H
#define HEAP_WINDOW_H

#include "RWindow.h"
#include "MyDebug.h"

#define MEM_UPDATE 'memU'

class BStringView;

class HeapWindow : public RWindow
{
public:
	HeapWindow();
	// ~HeapWindow();
	
virtual void MessageReceived(BMessage *m);
virtual bool QuitRequested();

private:
	BStringView *bytesUsed;
	BStringView *chunksUsed;
	BStringView *bytesFree;
	
	long pastUsed;
	long pastFree;
	long pastChunks;
};

#endif
