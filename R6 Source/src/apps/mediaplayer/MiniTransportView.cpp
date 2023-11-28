#include <Debug.h>
#include <InterfaceKit.h>

#include "Bitmaps.h"
#include "PlayerWindow.h"
#include "MiniTransportView.h"
#include "VideoView.h"
#include "TransportButton.h"
#include "MediaController.h"
#include "ProgressBar.h"

const int32 kMin = 1;
const int32 kMax = 1024;
const unsigned char *kPressedPlayButtonBitmapBits = kPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kPlayingPlayButtonBitmapBits = kPressedPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kPressedPlayingPlayButtonBitmapBits = kPlayingPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kPausedPlayButtonBitmapBits = kPressedPlayingPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kPressedPausedPlayButtonBitmapBits = kPausedPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
const unsigned char *kDisabledPlayButtonBitmapBits = kPressedPausedPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;

MiniTransportView::MiniTransportView(BRect rect, uint32 resizingMode, bool showModeSwitch)
	:	TransportView(rect, resizingMode)
{
	BRect buttonRect(Bounds());
	buttonRect.SetRightBottom(buttonRect.LeftTop() + kPlayButtonSize);
	buttonRect.OffsetTo(5, 3);

	fPlayButton = new PlayPauseButton(buttonRect, "",
		kPlayButtonBitmapBits, kPressedPlayButtonBitmapBits, kDisabledPlayButtonBitmapBits,
		kPlayingPlayButtonBitmapBits, kPressedPlayingPlayButtonBitmapBits,
		kPausedPlayButtonBitmapBits, kPressedPausedPlayButtonBitmapBits,
		new BMessage(kPlayPause), ' ', 0, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fPlayButton);

	fSliderRect = Bounds();
	fSliderRect.InsetBy(0, 5);
	fSliderRect.left = buttonRect.right + 8;
	fSliderRect.right -= 7;
	
	if (showModeSwitch) {
		BRect twistieRect(Bounds());
		twistieRect.InsetBy(4, 3);
		twistieRect.SetLeftBottom(twistieRect.RightTop() + BPoint(-16, 20));
		KnobSwitch *knobSwitch = new KnobSwitch(twistieRect, "switch", B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		knobSwitch->ResizeToPreferred();
		AddChild(knobSwitch);
		knobSwitch->SetMessage(new BMessage(M_TOGGLE_MINI_MODE));

		fSliderRect.right -= 21;
	}

	fTimeSlider = new TMultiThumbMediaSlider(this, fSliderRect, "Time Slider", "", 0, kSliderScale,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(fTimeSlider);
	fTimeSlider->AddThumbs(0, 0, kSliderScale,
		new BMessage(kStartScrub), new BMessage(kStopScrub), NULL,
		NULL, NULL, new BMessage(kInPointChanged),
		NULL, NULL, new BMessage(kOutPointChanged));			
	fTimeSlider->SetLabelPlacement(kLabelCenterWhileTracking);
	fTimeSlider->SetThumbCollisionDetection(true);
}

void 
MiniTransportView::AttachedToWindow()
{
	fPlayButton->SetTarget(this);
	_inherited::AttachedToWindow();
}

void 
MiniTransportView::SetEnabled(bool enabled)
{
	if (fTimeSlider)
		fTimeSlider->SetEnabled(enabled);

	_inherited::SetEnabled(enabled);
}

void 
MiniTransportView::UpdateButtons()
{
	_inherited::UpdateButtons();
}

void 
MiniTransportView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		default:
			return _inherited::MessageReceived(message);
	}
}


void MiniTransportView::AttachToController(MediaController *controller)
{
	TransportView::AttachToController(controller);

	if (controller == 0)
		return;
		
	if (controller->IsContinuous()){
		if (fTimeSlider) {
			fTimeSlider->RemoveSelf();
			delete fTimeSlider;
			fTimeSlider = 0;
		}
	
		if (fBufferStatus == 0) {
			// Add buffer status
			fBufferStatus = new ProgressBar(fSliderRect, "Buffer Status",
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
			fTimeSlider = new TMultiThumbMediaSlider(this, fSliderRect, "Time Slider", "", 0, kSliderScale,
				B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
			AddChild(fTimeSlider);
			fTimeSlider->AddThumbs(0, 0, kSliderScale,
				new BMessage(kStartScrub), new BMessage(kStopScrub), NULL,
				NULL, NULL, new BMessage(kInPointChanged),
				NULL, NULL, new BMessage(kOutPointChanged));			
			fTimeSlider->SetLabelPlacement(kLabelCenterWhileTracking);
			fTimeSlider->SetThumbCollisionDetection(true);
		}
	}
}

