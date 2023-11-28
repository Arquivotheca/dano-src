#ifndef TERM_VIEW_H
#define TERM_VIEW_H

#include <View.h>
#include <Region.h>

#include "vt100.h"

class bTermView : public BView, VT100Display
{
	BRegion invalid;
		
	VT100 *vt100;
	int rows, cols;	
	float fw,fh,fa;
	int fd;
	int cursorx, cursory;
	
	int scrolled_to;
	
public:
	void SetFD(int fd);
	bTermView(int width, int height);
	
	void Write(const char *data, size_t len);
	
	virtual void MessageReceived(BMessage *msg);
	virtual	void KeyDown(const char *bytes, int32 numBytes);	
	virtual	void Draw(BRect updateRect);
		
	// from VT100Display
	virtual void SetTitle(const char *title);
	virtual void MoveCursor(int x, int y);
	virtual void InvalidateRegion(int line);
	virtual void InvalidateRegion(int line, int start, int stop);
	virtual void InvalidateRegion(int startline, int stopline);

	void DumpHistory(){
		int fd = open("/boot/home/bterm.dump", O_CREAT|O_WRONLY|O_APPEND, 0666);
		if(fd && vt100) {
			write(fd, "=== bterm dump ===\n", 19);
			vt100->DumpHistory(fd);
		}
	}
};

#endif
