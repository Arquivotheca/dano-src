/*
	Copyright 2000, Be Incorporated.  All Rights Reserved.
*/

#ifndef _CONFIGAPP_H_
#define _CONFIGAPP_H_

#include <Application.h>

class ConfigWindow;

extern const char *STR_APP_SIG;
extern const char *STR_APP_NAME;

class ConfigApp : public BApplication
{
public:	// construction/destruction

	ConfigApp(const char *);
	~ConfigApp();

public:	// overloaded functions

	void AboutRequested();
	void MessageReceived(BMessage*);
	bool QuitRequested();
	void ReadyToRun();

private: // data members

	ConfigWindow *fWindow;
	char *fFileName;
};

#endif /* _CONFIGAPP_H_ */
