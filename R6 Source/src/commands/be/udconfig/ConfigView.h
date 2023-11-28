//
// Copyright 2000 Be, Incorporated.  All Rights Reserved.
//

#ifndef _CONFIG_VIEW_H_
#define _CONFIG_VIEW_H_

#include <View.h>

class BList;
class BMessage;
class BInvoker;
class BScrollView;
class ListEditView;
class SettingsFile;

class ConfigView : public BView {
public:
	ConfigView(BRect frame, const char *name, uint32 resizingMode, uint32 flags, const char *fname, BInvoker *invoker);
	~ConfigView();

	status_t InitCheck();

	void AttachedToWindow();
	void AllAttached();
	void FrameResized(float width, float height);

	void MessageReceived(BMessage *msg);

private:
	void AddFileName(BRect&);
	void AddListEditView(BRect&);
	void AddButtons(BRect&);

	void FixScrollBar();

	void EnableButtons(bool enabled);

	void Save();

private:
	float fEm;
	status_t fErr;

	char *fFileName;
	BInvoker *fDoneInvoker;
	SettingsFile *fSettingsFile;
	BList *fSettingsList;
	ListEditView *fListEditView;

	BScrollView *fScrollView;
	float fListHeight;
	float fItemHeight;
	float fScrollHeight;
};

#endif //_CONFIG_VIEW_H_
