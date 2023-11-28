//
//	Copyright 2000 Be, Incorporated.  All Rights Reserved.
//

#ifndef _CONFIGWINDOW_H_
#define _CONFIGWINDOW_H_

#include <Window.h>

class ConfigView;
class MountView;

class ConfigWindow : public BWindow
{
public:	// ctor, dtor

	ConfigWindow(BRect rect, const char *title, const char *fname);
	~ConfigWindow();

public: // overloaded functions

	void MessageReceived(BMessage *message);
	bool QuitRequested();

private: // helper functions

	void MountState(bool teardown = false);
	void EditState(bool teardown = false);

private: // data members

	float fEm;
	char *fFileName;

	BBox *fBgBox;
	ConfigView *fConfigView;
	MountView *fMountView;
};


#endif /* _CONFIGWINDOW_H_ */
