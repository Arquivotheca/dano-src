#include "PPAddOn.h"
#include <File.h>
#include <Resources.h>
#include <Entry.h>
#include <stdio.h>
#include "PrefsAppExport.h"

class JoystickAddOn : public PPAddOn {
public:
	JoystickAddOn(image_id i,PPWindow*w);

	BView* MakeView();
	bool UseAddOn();

	BBitmap* Icon();
	BBitmap* SmallIcon();
	char* Name();
	char* InternalName();
};

class JoystickView : public BView {
public:
	static BArchivable* Instantiate(BMessage*);

	JoystickView(BMessage*);

	void AllAttached();
	void MessageReceived(BMessage*m);

	void Probe(int32 portnb);
	void Calibrate(int32 currentport);

	void AddJoystickList(BPath*path,BList*list);

	bool CheckStickMatch(const char* path,const char* stickname);

	BList*settings_list;
	BList*detected_list;
	BList*detected_file_list;
	int32 currentport;
};

class JoystickWindow : public BWindow {
public:
	JoystickWindow(BJoystick*);	

	static int32 _CalibThread(void*p) { return ((JoystickWindow*)p)->CalibThread(); }
	int32 CalibThread();
	BJoystick* calibjoystick;
	BView*topview;
};

class AxisView : public BView {
public:
	AxisView(BRect);
	void Draw(BRect);
	void EnableMinMax(bool);
	void SetValue(float);

	float currentvalue;
	float minvalue,maxvalue;
	bool minmaxenabled;
};

class ButtonView : public BView {
public:
	ButtonView(BRect);
	void Draw(BRect);
	void SetValue(bool);

	bool buttonpressed;
};

class HatView : public BView {
public:
	HatView(BRect);
	void Draw(BRect);
	void SetValue(uint8);

	uint8 currentvalue;
};
