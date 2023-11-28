#include <Debug.h>
#include <InterfaceKit.h>
#include <MediaKit.h>
#include <TimeSource.h>
#include <stdio.h>

#include "Bitmaps.h"
#include "PlayerWindow.h"
#include "LargeTransportView.h"
#include "MediaController.h"
#include "MultiThumbMediaSlider.h"
#include "StopWatch.h"
#include "TransportButton.h"
#include "VolumeSlider.h"
#include "VideoView.h"
#include "ProgressBar.h"

const float kButtonSpacing = 3;
const bigtime_t kVolumeUpdatePeriod = 500000;

const int32 kSkipBitmapWidth = 40;
const int32 kSkiptBitmapHeight = 22;
const BPoint kSkipButtonSize(kSkipBitmapWidth - 1, kSkiptBitmapHeight - 1);

LargeTransportView::LargeTransportView(BRect rect, uint32 resizingMode)
	:	TransportView(rect, resizingMode),
		fStopButton(0),
		fFastForwardButton(0),
		fRewindButton(0),
		lastVolumeUpdate(0)
{

const unsigned char *kPressedPlayButtonBitmapBits = kPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kPlayingPlayButtonBitmapBits = kPressedPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kPressedPlayingPlayButtonBitmapBits = kPlayingPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kPausedPlayButtonBitmapBits = kPressedPlayingPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kPressedPausedPlayButtonBitmapBits = kPausedPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kDisabledPlayButtonBitmapBits = kPressedPausedPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;

const unsigned char *kPressedStopButtonBitmapBits = kStopButtonBitmapBits + kStopBitmapWidth * kStopBitmapHeight;
const unsigned char *kDisabledStopButtonBitmapBits = kPressedStopButtonBitmapBits + kStopBitmapWidth * kStopBitmapHeight;

const unsigned char *kPressedSkipBackBitmapBits = kSkipBackBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;
const unsigned char *kSkippingSkipBackBitmapBits = kPressedSkipBackBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;
const unsigned char *kPressedSkippingSkipBackBitmapBits = kSkippingSkipBackBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;
const unsigned char *kDisabledSkipBackBitmapBits = kPressedSkippingSkipBackBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;

const unsigned char *kPressedSkipForwardBitmapBits = kSkipForwardBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;
const unsigned char *kSkippingSkipForwardBitmapBits = kPressedSkipForwardBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;
const unsigned char *kPressedSkippingSkipForwardBitmapBits = kSkippingSkipForwardBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;
const unsigned char *kDisabledSkipForwardBitmapBits = kPressedSkippingSkipForwardBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;

 	BRect buttonRect(Bounds());
	buttonRect.InsetBy(4, 4);
	
	buttonRect.SetRightTop(buttonRect.LeftBottom() + BPoint(kSkipButtonSize.x, -kSkipButtonSize.y));

	fRewindButton = new TransportButton(buttonRect, "",
		kSkipBackBitmapBits, kPressedSkippingSkipBackBitmapBits, kDisabledSkipBackBitmapBits,
		new BMessage(kNudgeBackward), 0, new BMessage(kRewind), new BMessage(kDoneSkipping),
		100000, B_LEFT_ARROW, 0, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fRewindButton);

	buttonRect.OffsetTo(buttonRect.right + kButtonSpacing, buttonRect.top);
	buttonRect.SetRightBottom(buttonRect.LeftTop() + kStopButtonSize);
	fStopButton = new TransportButton(buttonRect, "",
		kStopButtonBitmapBits, kPressedStopButtonBitmapBits, kDisabledStopButtonBitmapBits,
		new BMessage(kStop), 0, 0, 0, 0, '.' , 0, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fStopButton);
	
	buttonRect.OffsetTo(buttonRect.right + kButtonSpacing, buttonRect.top);
	buttonRect.SetRightBottom(buttonRect.LeftTop() + kPlayButtonSize);
	fPlayButton = new PlayPauseButton(buttonRect, "",
		kPlayButtonBitmapBits, kPressedPlayButtonBitmapBits, kDisabledPlayButtonBitmapBits,
		kPlayingPlayButtonBitmapBits, kPressedPlayingPlayButtonBitmapBits,
		kPausedPlayButtonBitmapBits, kPressedPausedPlayButtonBitmapBits,
		new BMessage(kPlayPause), ' ', 0, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fPlayButton);

	
	buttonRect.OffsetTo(buttonRect.right + kButtonSpacing, buttonRect.top);
	buttonRect.SetRightBottom(buttonRect.LeftTop() + kSkipButtonSize);
	fFastForwardButton = new TransportButton(buttonRect, "",
		kSkipForwardBitmapBits, kPressedSkippingSkipForwardBitmapBits, kDisabledSkipForwardBitmapBits,
		new BMessage(kNudgeForward), 0, new BMessage(kFastForward), new BMessage(kDoneSkipping), 100000,
		B_RIGHT_ARROW, 0, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fFastForwardButton);

	BRect controlRect(Bounds());
	controlRect.InsetBy(10, 2);
	controlRect.right -= 18;
	fTimeSlider = new TMultiThumbMediaSlider(this, controlRect, "Time Slider", "", 0, kSliderScale,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(fTimeSlider);
	fTimeSlider->AddThumbs(0, 0, kSliderScale,
		new BMessage(kStartScrub), new BMessage(kStopScrub), NULL,
		NULL, NULL, new BMessage(kInPointChanged),
		NULL, NULL, new BMessage(kOutPointChanged));			
	fTimeSlider->SetLabelPlacement(kLabelTop);
	fTimeSlider->SetThumbCollisionDetection(true);

	controlRect = fTimeSlider->Frame();
	controlRect.top = controlRect.bottom + 8;
	controlRect.bottom = controlRect.top + 1;
	controlRect.left = buttonRect.right + 7;
	controlRect.right = controlRect.left + 85;
	fVolumeSlider = new TVolumeSlider(controlRect, new BMessage(kVolumeChanged), 0, 1000,
		TVolumeSlider::NewVolumeWidget(),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fVolumeSlider->SetModificationMessage(new BMessage(kVolumeChanged));
	AddChild(fVolumeSlider);
	fVolumeSlider->ResizeToPreferred();
	
	BRect twistieRect(Bounds());
	twistieRect.InsetBy(4, 11);
	twistieRect.SetLeftBottom(twistieRect.RightTop() + BPoint(-16, 20));
	KnobSwitch *knobSwitch = new KnobSwitch(twistieRect, "switch", B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	knobSwitch->ResizeToPreferred();
	knobSwitch->SetValue(1);
	AddChild(knobSwitch);
	knobSwitch->SetMessage(new BMessage(M_TOGGLE_MINI_MODE));
}

void 
LargeTransportView::AttachedToWindow()
{
	if (fPlayButton)
		fPlayButton->SetTarget(this);
	if (fStopButton)
		fStopButton->SetTarget(this);
	if (fRewindButton)
		fRewindButton->SetTarget(this);
	if (fFastForwardButton)
		fFastForwardButton->SetTarget(this);
	if (fVolumeSlider)
		fVolumeSlider->SetTarget(this);

	_inherited::AttachedToWindow();
}

void 
LargeTransportView::SetEnabled(bool enabled)
{
	bool isContinuous = false;
	if (Controller())
		isContinuous = Controller()->IsContinuous();

	if (fStopButton)
		fStopButton->SetEnabled(enabled);
	if (fRewindButton)
		fRewindButton->SetEnabled(enabled && !isContinuous);
	if (fFastForwardButton)
		fFastForwardButton->SetEnabled(enabled && !isContinuous);

	if (fVolumeSlider)
		fVolumeSlider->SetEnabled(enabled);

// sliders currently don't handle disabled state
	if (fTimeSlider)
		fTimeSlider->SetEnabled(enabled);

	_inherited::SetEnabled(enabled);
}

void LargeTransportView::AttachToController(MediaController *controller)
{
	TransportView::AttachToController(controller);

	if (controller == 0)
		return;

	if (controller->IsContinuous()) {
		if (fTimeSlider) {
			// Remove time slider
			fTimeSlider->RemoveSelf();
			delete fTimeSlider;
			fTimeSlider = 0;
		}
		
		if (fBufferStatus == 0) {
			// Add buffer status
			BRect controlRect(Bounds());
			controlRect.InsetBy(10, 2);
			controlRect.right -= 18;
			controlRect.bottom = controlRect.top + 17;
			controlRect.OffsetBy(0, 10);
			fBufferStatus = new ProgressBar(controlRect, "Buffer Status",
				B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
			AddChild(fBufferStatus);
		}
	} else {
		if (fBufferStatus != 0) {
			// Remove buffer status
			fBufferStatus->RemoveSelf();
			delete fBufferStatus;
			fBufferStatus = 0;
		}

		if (fTimeSlider == 0) {
			// Add time slider
			BRect controlRect(Bounds());
			controlRect.InsetBy(10, 2);
			controlRect.right -= 18;
			fTimeSlider = new TMultiThumbMediaSlider(this, controlRect, "Time Slider", "", 0, kSliderScale,
				B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
			AddChild(fTimeSlider);
			fTimeSlider->AddThumbs(0, 0, kSliderScale,
				new BMessage(kStartScrub), new BMessage(kStopScrub), NULL,
				NULL, NULL, new BMessage(kInPointChanged),
				NULL, NULL, new BMessage(kOutPointChanged));			
			fTimeSlider->SetLabelPlacement(kLabelTop);
			fTimeSlider->SetThumbCollisionDetection(true);
		}
	}
}



void 
LargeTransportView::UpdateCommon()
{
	bigtime_t now = system_time();
	if (Controller() && Controller()->HasAudio() && now - lastVolumeUpdate > kVolumeUpdatePeriod) {
		// for now have to do lame polling for volume value
#if xDEBUG
		BStopWatch stopWatch("timeToCallVolume");
#endif
		float updatedVolume = Controller()->Volume();
		fVolumeSlider->ValueUpdated(updatedVolume);
		lastVolumeUpdate = now;
	}
	
	_inherited::UpdateCommon();
}


void 
LargeTransportView::MessageReceived(BMessage *message)
{
	switch (message->what) {			
		default:
			_inherited::MessageReceived(message);
	}
}

void 
LargeTransportView::UpdateButtons()
{
	_inherited::UpdateButtons();
}

