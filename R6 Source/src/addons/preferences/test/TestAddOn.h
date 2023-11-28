#include "PPAddOn.h"
#include <File.h>
#include <Resources.h>
#include <Entry.h>
#include <stdio.h>
#include "PrefsAppExport.h"

class TestAddOn : public PPAddOn {
public:
	TestAddOn(image_id i,PPWindow*w);

	BView* MakeView();
	bool UseAddOn();

	BBitmap* Icon();
	char* Name();
	char* InternalName();

	void LoadPrefs(BMessage*);
	void SavePrefs(BMessage*);
	bool QuitRequested();
	void PanelActivated(bool);
};

class TestView : public BView {
public:
	TestView(BMessage*);

static BArchivable* Instantiate(BMessage*);
};
