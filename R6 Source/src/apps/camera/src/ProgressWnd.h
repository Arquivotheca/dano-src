/*
	ProgressWnd.h
	Download progress display window.
	Allows user to cancel the download by releasing a
	semaphore that is provided in the constructor.
*/

#ifndef PROGRESSWND_H
#define PROGRESSWND_H

#include <Window.h>

enum {
	PIC_PROGRESS	= 'prog',
	ALL_PROGRESS	= 'pral',
};

class ProgressWnd : public BWindow {
public:
	ProgressWnd(sem_id cancelSem, int32 numPics);
	~ProgressWnd();

	void MessageReceived(BMessage *msg);
private:
	int32	fFrameID, fTotal;
	char	fCurName[64];
	sem_id	fCancel;
};

#endif
