#ifndef PPADDON_H_
#define PPADDON_H_

#include <image.h>
#include <View.h>
#include <Bitmap.h>

class BResources;
class PPWindow;

class PPAddOn {
public:
	PPAddOn(image_id i,PPWindow*w);
	virtual ~PPAddOn();

	virtual bool UseAddOn();	// returns true if this add-on must be used, or false if it must be skipped (e.g. because
								// the hardware is not there)

	virtual BView* MakeView()=0; // returns a new'ed BView.

	virtual BBitmap* Icon()=0;	// returns a new'ed (typically 32*32) BBitmap.

	virtual BBitmap* SmallIcon();	// returns a new'ed (typically 16x16) BBitmap. Defaults to calling Icon().

	virtual char* Name()=0;	// returns a string. the string belongs to the add-on.
	virtual char* InternalName()=0;	// returns a string. the string belongs to the add-on.

	virtual void LoadPrefs(BMessage*);	// allows the add-on to get its preferences from a prefs file.
										// called before MakeView. The BMessage belongs to the library and must not be
										// deleted by the add-on.

	virtual void SavePrefs(BMessage*);	// allows the add-on to save its preferences. The BMessage belongs to the library
										// and must not be deleted by the add-on. Also the best place to save extra
										// preferences.

	virtual bool QuitRequested(); // gives the panel a chance to refuse quitting

	virtual void PanelActivated(bool);	// notifies the panel when it is selected/deselected. this is done while
										// the view is hidden

	void ChangeIcon(BBitmap*,BBitmap*); // the add-on can use this function to change its icon

	void UseNewPanel(PPAddOn*);	// the add-on can use this function to create a sub-panel
	void RemovePanel(PPAddOn*); // the add-on can use this to remove a panel (possibly itself).

	void SwitchToPanel(const char* =NULL); // switch to the named panel. NULL=switch to this panel.

	image_id MyIid;
	PPWindow* Window;

	PPAddOn* NextAddOn;
	BResources* Resources;

private:
	void InitResources();

};

extern "C" {
	_EXPORT PPAddOn* get_addon_object(image_id i,PPWindow*w);
}

#endif
