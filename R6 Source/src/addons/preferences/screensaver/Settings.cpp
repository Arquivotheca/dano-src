#include "Settings.h"
#include "Benaphore.h"
#include "ssdefs.h"

#include <Message.h>
#include <Screen.h>
#include <Path.h>
#include <File.h>
#include <FindDirectory.h>

static Benaphore	settingmutex("setting mutex");
static BMessage		settings;
static BMessage		originalsettings;

void InitSettings()
{
	// load preferences
	BPath	prefs_name;
	BFile	prefs_file;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &prefs_name) == B_OK &&
		prefs_name.Append(SETTINGS_FILE_NAME) == B_OK &&
		prefs_file.SetTo(prefs_name.Path(), B_READ_ONLY) == B_OK)
	{
		if(settings.Unflatten(&prefs_file) != B_OK)
			settings.MakeEmpty();
	}

	originalsettings = settings;
}

bool SettingsChanged()
{
	return false;	// TODO make this work
}

BMessage *AcquireSettings()
{
	settingmutex.Lock();
	return &settings;
}

void ReleaseSettings()
{
	settingmutex.Unlock();
}

void DefaultSettings()
{
	// settings
	BMessage *m = AcquireSettings();

	if(! m->HasInt32(kWindowTab))
		m->AddInt32(kWindowTab, 0);
	if(! m->HasString(kModuleName))
		m->AddString(kModuleName, "");
	if(! m->HasInt32(kTimeFlags))
		m->AddInt32(kTimeFlags, 0x00);
	if(! m->HasInt32(kTimeFade))
		m->AddInt32(kTimeFade, 120);
	if(! m->HasInt32(kTimeStandby))
		m->AddInt32(kTimeStandby, 120);
	if(! m->HasInt32(kTimeSuspend))
		m->AddInt32(kTimeSuspend, 120);
	if(! m->HasInt32(kTimeOff))
		m->AddInt32(kTimeOff, 120);
	if(! m->HasInt32(kCornerNow))
		m->AddInt32(kCornerNow, -1);
	if(! m->HasInt32(kCornerNever))
		m->AddInt32(kCornerNever, -1);
	if(! m->HasBool(kLockEnable))
		m->AddBool(kLockEnable, false);
	if(! m->HasInt32(kLockDelay))
		m->AddInt32(kLockDelay, 120);
	if(! m->HasString(kLockPassword))
		m->AddString(kLockPassword, "");
	if(! m->HasString(kModuleName))
		m->AddString(kModuleName, "");
	if(! m->HasString(kLockMethod))
		m->AddString(kLockMethod, "custom");
	if(! m->HasString(kLockPassword))
		m->AddString(kLockPassword, "");

	ReleaseSettings();
}

void SaveSettings()
{
	BMessage *m = AcquireSettings();

	// save preferences
	BPath	prefs_name;
	BFile	prefs_file;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &prefs_name) == B_OK &&
		prefs_name.Append(SETTINGS_FILE_NAME) == B_OK &&
		prefs_file.SetTo(prefs_name.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE) == B_OK)
	{
		m->Flatten(&prefs_file);
	}

	ReleaseSettings();
}
