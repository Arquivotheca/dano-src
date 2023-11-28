// ---------------------------------------------------------------------------
/*
	SoundCheckWindow.h
	
	Copyright (c) 2001 Be Inc. All Rights Reserved.
	
*/
// ---------------------------------------------------------------------------

#include <Window.h>
#include <SoundPlayer.h>
#include <String.h>
#include <MediaTrack.h>
#include <MediaFile.h>

#include "Test.h"

class BCheckBox;

// ---------------------------------------------------------------------------

const uint32 msgSoundDone = 'oned';
const uint32 msgSoundFailed = 'onef';
const uint32 msgAllDone = 'done';
const uint32 msgAllFailed = 'fail';
const uint32 msgClickShortcut = 'clik';

// ---------------------------------------------------------------------------

typedef void (*PlayHook)(void*, void*, size_t, const media_raw_audio_format&);

extern void play_func(void* cookie, void* buf, size_t size, const media_raw_audio_format& fmt);
extern void play_func_left(void* cookie, void* data, size_t size, const media_raw_audio_format& fmt);
extern void play_func_right(void* cookie, void* data, size_t size, const media_raw_audio_format& fmt);

// ---------------------------------------------------------------------------

struct PhaseInfo {
	const char* settingPrefix;
	const char* speakersUsed;
	const char* prepareMessage;
	const char* cleanupMessage;
	PlayHook playHook;
};

// ---------------------------------------------------------------------------

class SoundControl {
public:
					SoundControl(const BString& setting, PlayHook hook);
					~SoundControl();
	void			Play();
	void			Stop();
	BString&		SoundName() { return fSoundName; }
	
private:
	BMediaFile*		fMediaFile;
	BSoundPlayer*	fPlayer;
	BString			fSoundName;
};

// ---------------------------------------------------------------------------
// class SoundCheckWindow
// ---------------------------------------------------------------------------
#include <stdio.h>
class SoundCheckWindow : public TestWindow {
public:
					SoundCheckWindow(BRect rect, const PhaseInfo* phaseInfo, int32 phaseCount);
	virtual 		~SoundCheckWindow();

	virtual void	Show();
	virtual void	MessageReceived(BMessage* msg);

protected:
	int32				fPhaseCount;
	const PhaseInfo*	fPhaseInfo;
	
private:
	int32			FindNextPhase();
	bool			IgnoreTest(const char* testName);
	void			StartNextPhase();
	void			StopCurrentPhase();

	int32			fCurrentPhase;
	BCheckBox**		fPhaseControls;
	BButton*		fFailButton;
	SoundControl*	fCurrentSound;


};
