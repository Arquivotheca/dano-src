/// HeapWindow.cpp
#include "HeapWindow.h"

#include <StringView.h>
#include <Screen.h>
#include <Button.h>
#include <Application.h>

HeapWindow::HeapWindow()
	: RWindow(BRect(0,0,200,110),"Heap Stats", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_NOT_CLOSABLE)
{
	Lock();
	
	{
		BScreen si(this);	
		MoveTo((si.Frame().right - 205),(si.Frame().bottom-115));
	}
	
	BStringView *labelBytesUsed =
		new BStringView(BRect(10,10,100,24),"labelBytesUsed","Bytes used:");
	AddChild(labelBytesUsed);
	
	bytesUsed = new BStringView(BRect(100,10,200,24),"bytes used",B_EMPTY_STRING);
	AddChild(bytesUsed);
	
	
	
	BStringView *labelChunksUsed =
		new BStringView(BRect(10,30,100,44),"labelChunksUsed","Chunks used:");
	AddChild(labelChunksUsed);
	
	chunksUsed = new BStringView(BRect(100,30,200,44),"chunks used",B_EMPTY_STRING);
	AddChild(chunksUsed);
	
	
	
	BStringView *labelBytesFree =
		new BStringView(BRect(10,50,100,64),"labelBytesFree","Bytes free:");
	AddChild(labelBytesFree);
		
	bytesFree = new BStringView(BRect(100,50,200,64),"bytes free",B_EMPTY_STRING);
	AddChild(bytesFree);
	
	BButton *update = new BButton(BRect(50,80,130,100),"update","Update",
		new BMessage(MEM_UPDATE));
	AddChild(update);
	
	pastUsed = 0;
	pastFree = 0;
	pastChunks = 0;
	
	Show();
	Unlock();
}

void HeapWindow::MessageReceived(BMessage *m)
{
	switch(m->what) {
		case MEM_UPDATE:
		{
			char buf[80];
			long curUsed = mstats().bytes_used;
			long curChunks = mstats().chunks_used;
			long curFree = mstats().bytes_free;
			sprintf(buf,"%d  %d  %d",curUsed,pastUsed,curUsed-pastUsed);
			bytesUsed->SetText(buf);
			sprintf(buf,"%d  %d  %d",curChunks,pastChunks,curChunks-pastChunks);
			chunksUsed->SetText(buf);
			sprintf(buf,"%d  %d  %d",curFree,pastFree,curFree-pastFree);
			bytesFree->SetText(buf);
			
			pastUsed = curUsed;
			pastChunks = curChunks;
			pastFree = curFree;
			
			break;
		}
		default:
			break;
	}
}

bool HeapWindow::QuitRequested()
{
	if (CountWindows() <= 1) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	return TRUE;
}
