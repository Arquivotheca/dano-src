#if ! defined SETTINGS_INCLUDED
#define SETTINGS_INCLUDED 1

class BMessage;

void InitSettings();
bool SettingsChanged();
BMessage *AcquireSettings();
void ReleaseSettings();
void DefaultSettings();
void SaveSettings();

#endif
