#include "PPAddOn.h"
#include <File.h>
#include <Resources.h>
#include <Entry.h>
#include <stdio.h>
#include "PrefsAppExport.h"

class TestDynAddOn : public PPAddOn {
public:
	TestDynAddOn(image_id i,PPWindow*w);

	BView* MakeView();

	BBitmap* Icon();
	char* Name();
	char* InternalName();
};

class TestDynView : public BView {
public:
	TestDynView(BRect,PPAddOn*);

	void AllAttached();
	void MessageReceived(BMessage*);
	
	PPAddOn*AddOn;
};

class TestDynAddOn2 : public PPAddOn {
public:
	TestDynAddOn2(image_id i,PPWindow*w);

	BView* MakeView();

	BBitmap* Icon();
	char* Name();
	char* InternalName();
};

class TestDynView2 : public BView {
public:
	TestDynView2(BRect,PPAddOn*);

	void AllAttached();
	void MessageReceived(BMessage*);
	
	PPAddOn*AddOn;
};

