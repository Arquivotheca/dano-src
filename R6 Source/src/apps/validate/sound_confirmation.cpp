
#include "Test.h"
#include "SoundCheckWindow.h"
#include "ValidApp.h"

// ---------------------------------------------------------------------------
// Internal speaker test
// ---------------------------------------------------------------------------

Test * make_sound_confirmation();

// ---------------------------------------------------------------------------

const int32 kPhaseCount = 3;

PhaseInfo kPhaseInfo[kPhaseCount] = {
	{"sound.left", "left speaker", NULL, NULL, &play_func_left},
	{"sound.right", "right speaker", NULL, NULL, &play_func_right},
	{"sound.phase", "both speakers", NULL, NULL, &play_func}
};

// ---------------------------------------------------------------------------

Test * make_sound_confirmation()
{
	SoundCheckWindow* kkw = new SoundCheckWindow(BRect(25,100,725,300), kPhaseInfo, kPhaseCount);
	return kkw->GetTest();
}

// ---------------------------------------------------------------------------
// External speaker test
// ---------------------------------------------------------------------------

Test* make_external_sound_confirmation();

// ---------------------------------------------------------------------------

static const int32 kExternalPhaseCount = 1;

// external speakers need to be plugged in before running tests...
static PhaseInfo kExternalPhaseInfo[kExternalPhaseCount] = {
	{"external_sound", "external speakers", 
	 NULL, 
	 "Please disconnect external speakers.  Click 'OK' when you are ready to continue.", &play_func},
};

// ---------------------------------------------------------------------------

Test * make_external_sound_confirmation()
{
	SoundCheckWindow* window = new SoundCheckWindow(BRect(25,100,725,225), kExternalPhaseInfo, kExternalPhaseCount);
	return window->GetTest();
}
