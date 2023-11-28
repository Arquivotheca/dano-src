// ---------------------------------------------------------------------------
/*
	SoundCheckWindow.cpp
	
	Copyright (c) 2001 Be Inc. All Rights Reserved.
	
*/
// ---------------------------------------------------------------------------
#include "miniplay.h"

#include "SoundCheckWindow.h"
#include "settings.h"
#include "ValidApp.h"
#include "Test.h"


#include <Window.h>
#include <TextControl.h>
#include <TextView.h>
#include <CheckBox.h>
#include <MessageFilter.h>
#include <Alert.h>
#include <SoundPlayer.h>
#include <math.h>
#include <Button.h>
#include <String.h>
#include <MediaTrack.h>
#include <MediaFile.h>
#include <OS.h>
#include <Entry.h>

#include <stdio.h>
#include <Debug.h>

// ---------------------------------------------------------------------------

float phase = 0;
float phase_inc = 2*M_PI*1000.0/44100.0;
BMediaTrack* track;
bool trackDone = false;

// ---------------------------------------------------------------------------

void
play_func(void* /* cookie */, void* buf, size_t size, const media_raw_audio_format& /* fmt */)
{
	if (track != 0) {
		int64 count = 0;
		if (track->ReadFrames(buf, &count) != B_OK) {
			// Just try to start over... since they can cancel out at any time, this won't hurt anything
			// this allows us to loop even with bad files (like SonySoundTest07.mp3)
			int64 f = 0;
			track->SeekToFrame(&f);
			memset(buf, 0, size);
		}
		return;
	}
	int16 * s = (int16 *)buf;
	int cnt = size/4;
	while (cnt-- > 0) {
		int16 v = int16(32000 * sin(phase));
		phase += phase_inc;
		if (phase >= 2*M_PI) phase -= 2*M_PI;
		s[0] = v;
		s[1] = v;
		s += 2;
	}
}

// ---------------------------------------------------------------------------

void
play_func_left(void* cookie, void* data, size_t size, const media_raw_audio_format& fmt)
{
	ASSERT(fmt.format == 0x2);
	ASSERT(fmt.channel_count == 2);

	play_func(cookie, data, size, fmt);
	
	// Nuke the right channel (Jon says... super-cheezy way to silence one channel)
	if (trackDone == false) {
		short* s = (short *)data;
		s += 1;
		int cnt = size/4;
		while (cnt-- > 0) {
			*s = 0;
			s += 2;
		}
	}
}

// ---------------------------------------------------------------------------

void
play_func_right(void* cookie, void* data, size_t size, const media_raw_audio_format& fmt)
{
	ASSERT(fmt.format == 0x2);
	ASSERT(fmt.channel_count == 2);

	play_func(cookie, data, size, fmt);

	// Nuke the left channel (Jon says... super-cheezy way to silence one channel)
	if (trackDone == false) {
		short* s = (short *)data;
		int cnt = size/4;
		while (cnt-- > 0) {
			*s = 0;
			s += 2;
		}
	}
}

// ---------------------------------------------------------------------------

SoundControl::SoundControl(const BString& setting, PlayHook hook)
{
	fMediaFile = NULL;
	fPlayer = NULL;
	track = NULL;
	trackDone = false;

	// See if we can open the file specified for this test
	char buf[200] = "";
	const char* fileName = get_setting(setting.String(), buf, 200);
	if (fileName != NULL) {
		// if we don't have a full path, prepend our current directory back into fileName
		if (fileName[0] != '/') {
			BString fullName(ValidApp::s_current_directory);
			fullName += "/";
			fullName += fileName;
			strcpy(buf, fullName.String());
			fileName = buf;
		}
		entry_ref ref;
		bool file_ok = true;
		if (get_ref_for_path(fileName, &ref) == B_OK) {
			fMediaFile = new BMediaFile(&ref);
			track = fMediaFile->TrackAt(0);
			media_format fmt;
			fmt.type = B_MEDIA_RAW_AUDIO;
			if ((track->DecodedFormat(&fmt) < 0) || (fmt.type != B_MEDIA_RAW_AUDIO)) {
				file_ok = false;
			}
		}
		else {
			file_ok = false;
		}
		
		if (file_ok == false) {
			fprintf(stderr, "%s: not recognized\n", fileName);
			delete fMediaFile;
			track = 0;
			fileName = NULL;
		}
	}

	if (fileName) {
		fSoundName = fileName;
	}
	else {
		fSoundName = "tone";
	}

	media_raw_audio_format fmt(media_raw_audio_format::wildcard);
	if (track != 0) {
		media_format mfmt;
		track->DecodedFormat(&mfmt);
		fmt = mfmt.u.raw_audio;
	}
	else {
		fmt.frame_rate = 44100.0;
		fmt.channel_count = 0x2;
		fmt.format = 0x2;
		fmt.buffer_size = 2048;
	}
	
	fPlayer = new BSoundPlayer(&fmt, "Test Sound", hook);
	
	// Make sure we are at maximum volume.
	fPlayer->SetVolume(1.0);
}

// ---------------------------------------------------------------------------

SoundControl::~SoundControl()
{
	delete fMediaFile;
	delete fPlayer;
}

// ---------------------------------------------------------------------------

void
SoundControl::Play()
{
	fPlayer->SetHasData(true);
	fPlayer->Start();
}

// ---------------------------------------------------------------------------

void
SoundControl::Stop()
{
	fPlayer->Stop();
}

// ---------------------------------------------------------------------------
// EnterFilter - a helper class for SoundCheckWindow
// Pressing the Enter key is the same as clicking the currently active control
// ---------------------------------------------------------------------------

class EnterFilter : public BMessageFilter {
public:
							EnterFilter(SoundCheckWindow* inWindow);
	virtual	filter_result	Filter(BMessage* message, BHandler** target);

private:
	SoundCheckWindow*		fWindow;
};

EnterFilter::EnterFilter(SoundCheckWindow* inWindow)
			: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN)
{
	fWindow = inWindow;
}

filter_result
EnterFilter::Filter(BMessage* message, BHandler** /* target */)
{
	filter_result result = B_DISPATCH_MESSAGE;
	const char* bytes;
	if (message->FindString("bytes", &bytes) == B_OK && bytes[0] == B_RETURN) {
		fWindow->PostMessage(msgClickShortcut);
		result = B_SKIP_MESSAGE;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
// SoundCheckWindow member functions
// ---------------------------------------------------------------------------

SoundCheckWindow::SoundCheckWindow(BRect rect, const PhaseInfo* phaseInfo, int32 phaseCount) 
				 : TestWindow(rect, "Sound Output", B_TITLED_WINDOW,
							  B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE) 
{
	fPhaseInfo = phaseInfo;
	fPhaseCount = phaseCount;
	fCurrentPhase = -1;
	
	fPhaseControls = new (BCheckBox*)[fPhaseCount];
	
	// Set up sets of buttons for each test, however, only enable the current set	
	float top = 10;	
	for (int i = 0; i < fPhaseCount; i++) {
		BString question = "Correct sound heard in ";
		question += fPhaseInfo[i].speakersUsed;
		question += "?";
		
		fPhaseControls[i] = new BCheckBox(BRect(10, top, 690, top+25), "", question.String(), new BMessage(msgSoundDone));
		AddChild(fPhaseControls[i]);
		top += 35;
	}

	// Add one more set of buttons at the bottom for all done
	top += 20;
	fFailButton = new BButton(BRect(10,top,90,top+30), "fail", "Fail", new BMessage(msgAllFailed));
	AddChild(fFailButton);
	
	AddCommonFilter(new EnterFilter(this));
	
	// The first one seems to be able to unset the MUTE flag, the second one canactually set the volume.
	// There may be a better way, but this stuff is not documented. =P
	
	printf("mav result = %s\n", strerror(mini_adjust_volume(miniMainOut, 1.0, 1.0, VOL_CLEAR_MUTE)));
	printf("msv result = %s\n", strerror(mini_set_volume(miniMainOut, 1.0, 1.0)));
}

// ---------------------------------------------------------------------------

SoundCheckWindow::~SoundCheckWindow()
{
	// make sure the current phase is stopped (doesn't hurt if already stopped)
	this->StopCurrentPhase();

	delete [] fPhaseControls;
}

// ---------------------------------------------------------------------------

void 
SoundCheckWindow::Show() 
{
	this->StartNextPhase();
	TestWindow::Show();
}

// ---------------------------------------------------------------------------

int32 
SoundCheckWindow::FindNextPhase()
{
	// given our current phase, bump the count until we get to one that
	// isn't to be ignored

	for (fCurrentPhase += 1; fCurrentPhase < fPhaseCount; fCurrentPhase++) {		
		// check that we haven't ignored this test - if so, we found the right one
		if (IgnoreTest(fPhaseInfo[fCurrentPhase].settingPrefix) == false) {
			break;
		}
	}
	return fCurrentPhase;
}

// ---------------------------------------------------------------------------

bool 
SoundCheckWindow::IgnoreTest(const char* testName) {
	bool ignore = false;
	const char* str = get_setting(testName);
	if (str != NULL) {
		if (strcasecmp(str, "ignore") == 0 || strcasecmp(str, "no") == 0 ||
			strcasecmp(str, "0") == 0 || strcasecmp(str, "pass") == 0) {
			ignore = true;
		}
	}
	
	return ignore;
}

// ---------------------------------------------------------------------------

void
SoundCheckWindow::StartNextPhase()
{
	// start up the sound, and enable the buttons -- or signal myself to quit because
	// we have played them all

	if (this->FindNextPhase() < fPhaseCount) {
		BString settingName = fPhaseInfo[fCurrentPhase].settingPrefix;
		settingName += ".filename";
		fCurrentSound = new SoundControl(settingName, fPhaseInfo[fCurrentPhase].playHook);
		
		// If we have a prepareMessage, then we display it and wait for the user
		if (fPhaseInfo[fCurrentPhase].prepareMessage) {
			BAlert* prepare = new BAlert("", fPhaseInfo[fCurrentPhase].prepareMessage, "OK");
			SetLargeAlertFont(prepare);
			prepare->Go();
		}
		fCurrentSound->Play();
	}
	else {
		this->PostMessage(msgAllDone);
	}
		
	for (int i = 0; i < fPhaseCount; i++) {
		if (i == fCurrentPhase) {
			BString newLabel = fPhaseControls[i]->Label();
			newLabel += " (";
			newLabel += fCurrentSound->SoundName();
			newLabel += ")";
			fPhaseControls[i]->SetLabel(newLabel.String());
		}
		fPhaseControls[i]->SetEnabled(i == fCurrentPhase);
	}

	// all-fail button is always enabled
	fFailButton->SetEnabled(true);
}

// ---------------------------------------------------------------------------

void
SoundCheckWindow::StopCurrentPhase()
{
	// don't stop a phase if we are already beyond all phases (or already stopped)
	if (fCurrentPhase < fPhaseCount && fCurrentSound != NULL) {
		fCurrentSound->Stop();
		// If we have a cleanupMessage, then we display it and wait for the user
		if (fPhaseInfo[fCurrentPhase].cleanupMessage) {
			BAlert* prepare = new BAlert("", fPhaseInfo[fCurrentPhase].cleanupMessage, "OK");
			SetLargeAlertFont(prepare);
			prepare->Go();
		}
		delete fCurrentSound;
		fCurrentSound = NULL;
	}
}

// ---------------------------------------------------------------------------

void
SoundCheckWindow::MessageReceived(BMessage * msg)
{
	// If the current test failed, then we can just quit
	// otherwise, we go on to do more of the subtests

	switch (msg->what) {
		case msgClickShortcut:
			// user hit "Enter" as a shortcut to clicking the currently enabled control
			fPhaseControls[fCurrentPhase]->SetValue(1);
			// fallthru to msgSoundDone...
		case msgSoundDone:
			this->StopCurrentPhase();
			this->StartNextPhase();
			break;
		case msgSoundFailed:
			this->StopCurrentPhase();
			fail("Sound check (%s) cancelled by operator\n", fPhaseInfo[fCurrentPhase].speakersUsed);
			this->TestDone(false);
			break;
		case msgAllFailed:
			this->StopCurrentPhase();
			fail("Sound check (%s) cancelled by operator\n", fCurrentPhase < fPhaseCount ? fPhaseInfo[fCurrentPhase].speakersUsed : "after all sub-tests");
			this->TestDone(false);
			break;
		case msgAllDone:
			this->TestDone(true);
			break;
		default:
			TestWindow::MessageReceived(msg);
			break;
	}
};
