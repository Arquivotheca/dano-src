#ifndef QUICKRES_QUICK_RES_H	// :)
#define QUICKRES_QUICK_RES_H

#include <experimental/DocApplication.h>

#include <String.h>
#include <Locker.h>
#include <Message.h>
#include <Rect.h>

class QuickResApp : public DocApplication
{
public:
	QuickResApp();
	virtual void ReadyToRun();
	virtual	void AboutRequested();

private:
	BLocker		settingslock;
	BMessage	settings;
	BMessenger	fAboutWindow;
	bool		firstframe;
};

#endif
