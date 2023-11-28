#ifndef __ABOUT_BOX__
#define __ABOUT_BOX__

#include <View.h>
#include <Window.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Resources.h>
#include <Autolock.h>

class BBitmap;
class BStringView;
class BTextView;

namespace BTrackerTheft {

enum {
	kResAboutBoxBackground = 1000
	// ** Add resource ID's for other icons
	// used in AboutBox **
};


// *** This is shamelessly stolen from Tracker/Utilities.cpp
// The AboutBox used to be part of Tracker but for open source
// reasons it no longer is. The AboutBox needs some of these
// classes, normally found in the Tracker, to work properly.
// Rather then recode them all, they've just been copy/pasted
// here. Some of the stuff has been abreviated from the Tracker
// implementation. ***
class OffscreenBitmap {
	// a utility class for setting up offscreen bitmaps
public:
	OffscreenBitmap(BRect bounds);
	OffscreenBitmap();
	~OffscreenBitmap();

	BView *BeginUsing(BRect bounds);
	void DoneUsing();
	BBitmap *Bitmap() const;
		// blit this to your view when you are done rendering
	BView *View() const;
		// use this to render your image

private:
	void NewBitmap(BRect frame);
	BBitmap *fBitmap;
};

class FlickerFreeStringView : public BStringView {
	// Adds support for offscreen bitmap drawing for string views that update often
	// this would be better implemented as an option of BStringView	
public:
	FlickerFreeStringView(BRect bounds, const char *name, const char *text);
	virtual ~FlickerFreeStringView();
	virtual void Draw(BRect);
	virtual void AttachedToWindow();
	virtual void SetViewColor(rgb_color);
	virtual void SetLowColor(rgb_color);
	
private:
	OffscreenBitmap *fBitmap;
	rgb_color fViewColor;
	rgb_color fLowColor;
	BBitmap *fOrigBitmap;

	typedef BStringView _inherited;
};

class BImageResources
{
	// convenience class for accessing 
public:
	BImageResources(void *memAddr);
	~BImageResources();
	
	BResources *ViewResources();
	const BResources *ViewResources() const;
	
	status_t FinishResources(BResources *) const;
	
	const void *LoadResource(type_code type, int32 id,
		size_t *outSize) const;
	const void *LoadResource(type_code type, const char *name,
		size_t *outSize) const;
		// load a resource from the Tracker executable, just like the
		// corresponding functions in BResources.  These methods are
		// thread-safe.
	
	status_t GetBitmapResource(type_code type, int32 id, BBitmap** out) const;
		// this is a wrapper around LoadResource(), for retrieving
		// arbitrary bitmaps.  the resource with the given type and
		// id is looked up, and a BBitmap created from it and returned
		// in 'out'.  currently it can only create bitmaps from data
		// that is an archived bitmap object.
		
private:
	image_id find_image(void *memAddr) const;
	
	mutable BLocker fLock;
	BResources fResources;
};

extern BImageResources* GetLibbeResources();

// *** /END of Shameless Tracker theft ***

class AboutWindow : public BWindow {
public:
	AboutWindow();
	~AboutWindow();
	static void RunAboutWindow();
	
private:
	static AboutWindow *oneCopyOnly;

	typedef BWindow _inherited;
};

class Thermometer : public BView {
public:
	Thermometer(BRect, const char *, float maxValue, float initialValue);
	void SetValue(float);
	virtual void Draw(BRect);

private:
	float fMaxValue;
	float fValue;
};

class CreditsAutoScrollView : public BScrollView {
public:
	CreditsAutoScrollView(BRect frame, const char *name, const BFont *font);
	virtual void Pulse();
	virtual void MessageReceived(BMessage *);
	virtual void Show();
	void SetShowSecretText(bool on)
		{ fShowSecretText = on; }

private:
	BTextView *fTextView;
	bool fShowSecretText;
	
	typedef BScrollView _inherited;
};

class LicenseItem : public BView {
public:
	LicenseItem(BRect frame, const char *name, int32 iconId, const char *text, const BFont *font);
	~LicenseItem();
	
	virtual void Draw(BRect rect);

private:
	BString fText;
	BBitmap	*fLogo;
	BTextView *fTextView;
};

class LicenseInfoView : public BView {
public:
	LicenseInfoView(BRect frame, const BFont *font);
	virtual void AttachedToWindow();

private:
	BFont fFont;
};

class AboutView : public BView {
public:
	AboutView(BRect frame, const char *);
	virtual ~AboutView();
	virtual void AttachedToWindow();
	virtual void Pulse();
	virtual void Draw(BRect rect);
	void UpdateInfo();
	virtual void MouseDown(BPoint);

private:
	void ToggleMode(BPoint);
	bool GetUptimeString(BString &string);

	system_info fOldSysInfo;
	BStringView *fMemoryFree;
	BStringView *fUptime;
	bigtime_t fLastUptimeInSeconds;
	Thermometer *fThermometer;
	bool fSysInfoMode;
	BRect fBigIconRect;

	CreditsAutoScrollView *fCreditsView;
	LicenseInfoView *fLegalView;
	BScrollView	*fLegalScroller;
	
	BBitmap	*fBigLogo;
	typedef BView _inherited;
};


} // namespace BTrackerTheft

using namespace BTrackerTheft;

#endif
