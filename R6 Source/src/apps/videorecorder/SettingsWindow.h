#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include "Settings.h"

#include <Window.h>

class SettingsWindow : public BWindow {
	public:
		SettingsWindow(BWindow *target, ScalarValueSetting *audiojittersetting);
		void MessageReceived(BMessage*);
		bool QuitRequested();
		
	private:
		BWindow		*fTarget;
		ScalarValueSetting	*fAudioJitterSetting;
		class BSlider *fAudioJitterSlider;
};

#endif

