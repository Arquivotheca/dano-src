#ifndef __PREFERENCES__
#define __PREFERENCES__

#include <String.h>
#include <Window.h>

#include "TrackerSettings.h"
#include "SettingsHandler.h"

class BCheckBox;
class BButton;
class BRadioButton;

class CheckBoxSettingItem;
class BackgroundVolumeSettingItem;

class MediaPlayerSettings : public Settings {
public:
	MediaPlayerSettings();
	~MediaPlayerSettings();
	
	void RunSettingsDialog(BPoint pos);

	const CheckBoxSettingItem *AutoPlay() const;
	const CheckBoxSettingItem *QuitWhenDoneWithSounds() const;
	const CheckBoxSettingItem *QuitWhenDoneWithMovies() const;
	const CheckBoxSettingItem *LoopSoundsByDefault() const;
	const CheckBoxSettingItem *LoopMoviesByDefault() const;
	const BackgroundVolumeSettingItem *BackgroundVolume() const;
	const BooleanValueSetting *MediaNodeDefault() const;
	const CheckBoxSettingItem* EnableHardwareOverlays() const;
	
private:
	CheckBoxSettingItem *fAutoPlay;
	CheckBoxSettingItem *fQuitWhenDoneWithSounds;
	CheckBoxSettingItem *fQuitWhenDoneWithMovies;
	CheckBoxSettingItem *fLoopSoundsByDefault;
	CheckBoxSettingItem *fLoopMoviesByDefault;
	BackgroundVolumeSettingItem *fBackgroundVolumeSettings;
	BooleanValueSetting *fMediaNodeDefault;
	CheckBoxSettingItem *fEnableHardwareOverlay;

	friend class PrefsWindow;
};

class PrefsWindow : public BWindow {
public:
	PrefsWindow(MediaPlayerSettings *, BPoint pos);
	
protected:
	virtual void MessageReceived(BMessage *);
	void UpdateItems();
	
private:
	MediaPlayerSettings *target;
	typedef BWindow _inherited;
};

// move to Settings.h

class CheckBoxSettingItem : public BooleanValueSetting {
public:
	CheckBoxSettingItem(const char *name, const char *title, bool defaultValue);
	virtual ~CheckBoxSettingItem() {}
	
	virtual void SetPreferenceItemToValue();
	virtual void SetValueToPreferenceItem();
	virtual bool AddPreferenceItem(BView *parentView, BPoint where);

protected:
	virtual const char *Title()
		{ return title.String(); }


protected:
	BCheckBox *check;
	BString title;

	friend class PrefsWindow;
};

class BackgroundVolumeSettingItem : public EnumeratedStringValueSetting {
public:
	BackgroundVolumeSettingItem(const char *name);

	enum Values {
		kFullVolume,
		kHalfVolume,
		kMuted
	};
	
	Values EnumValue() const;
	
	virtual void SetPreferenceItemToValue();
	virtual void SetValueToPreferenceItem();
	virtual bool AddPreferenceItem(BView *parentView, BPoint where);

protected:


protected:
	BRadioButton *fullRadio;
	BRadioButton *halfRadio;
	BRadioButton *mutedRadio;
};
#endif
