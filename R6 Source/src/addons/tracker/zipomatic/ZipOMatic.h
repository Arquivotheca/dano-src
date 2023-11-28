#ifndef __ZIP_O_MATIC__
#define __ZIP_O_MATIC__

// Dedicated to GUIZip

#include <Application.h>

const uint32 kWindowClosed = 'wcls';

class ZipOWindow;

class ZipOMaticApp : public BApplication {
public:
	ZipOMaticApp(const char *);

protected:
	virtual void ReadyToRun();
	virtual void RefsReceived(BMessage *);
	virtual void MessageReceived(BMessage *);

private:
	int32 windowCount;
		// used to stagger windows and quit properly

	typedef BApplication _inherited;
};

#endif
