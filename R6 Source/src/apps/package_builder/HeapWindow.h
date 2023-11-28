// HeapWindow.h

#include "RWindow.h"
#include "MyDebug.h"

#ifndef _HEAPWINDOW_H
#define _HEAPWINDOW_H

#define MEM_UPDATE 'memU'

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
