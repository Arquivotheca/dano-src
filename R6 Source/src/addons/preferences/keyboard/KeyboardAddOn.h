#include "PPAddOn.h"
#include <File.h>
#include <Resources.h>
#include <Entry.h>
#include <stdio.h>
#include "PrefsAppExport.h"

class KeyboardAddOn : public PPAddOn {
public:
	KeyboardAddOn(image_id i,PPWindow*w);

	BView* MakeView();
	bool UseAddOn();

	BBitmap* Icon();
	BBitmap* SmallIcon();
	char* Name();
	char* InternalName();
	void PanelActivated(bool);
};

class KeyboardView : public BView {
public:
	KeyboardView(BMessage*);

	void AllAttached();
	void AllDetached();
	void MessageReceived(BMessage*m);
	void PanelActivated();

	int32 initial_delay;
	int32 initial_repeat;

	bool EnableRevert(int32 cdelay,int32 crepeat);

	static _EXPORT BArchivable* Instantiate(BMessage*);
};
