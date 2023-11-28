#ifndef PPWINDOW_H_
#define PPWINDOW_H_

#include <Window.h>
#include "PPAddOn.h"

class BNode;

class PPAddOnData {
public:
	PPAddOn* AddOn;
	BBitmap* Icon;
	BBitmap* SmallIcon;
	char* Name;
	char* InternalName;
	BView* View;

	PPAddOnData*next;
};

class PPWindow : public BWindow {
public:
	static BArchivable* Instantiate(BMessage*);
	PPWindow(BMessage*);
	bool QuitRequested();

	void WorkspaceActivated(int32,bool);
	void ScreenChanged(BRect,color_space);

	void MessageReceived(BMessage*);

	void LoadAddOns(const char*);
	PPAddOn*LoadAnAddOn(const char*);
	void InitAnAddOn(PPAddOnData*);

	void SwitchToPanel(PPAddOnData*);

	void ReadPrefsFromAttributes(BMessage*,BNode*);
	void WritePrefsToAttributes(BMessage*,BNode*);

	PPAddOnData*AddOnDataForInt(int);
	int IntForAddOn(PPAddOn*);
	PPAddOnData*AddOnDataForName(const char*);

	void ChangeIcon(PPAddOn*,BBitmap*,BBitmap*);
	void UseNewPanel(PPAddOn*);
	void RemovePanel(PPAddOn*);
	void SwitchToPanel(const char*);

	BMessage* preferences;

	PPAddOnData*FirstAddOn;
	PPAddOnData*CurrentAddOn;
};

#endif
