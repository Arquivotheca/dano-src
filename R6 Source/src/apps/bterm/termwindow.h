#ifndef TERM_WINDOW_H
#define TERM_WINDOW_H

#include <Window.h>

#include "termview.h"

class bTermWindow : public BWindow
{
	int fd;
	bTermView *tv;

	static status_t reader(void *data) { return ((bTermWindow*)data)->Reader(); }
	status_t Reader();
	virtual void MessageReceived(BMessage *msg);
			
public:
	bTermWindow(int argc, char *argv[]);
	virtual bool QuitRequested();
	void DumpHistory() { if(tv) tv->DumpHistory(); }
};

#endif
