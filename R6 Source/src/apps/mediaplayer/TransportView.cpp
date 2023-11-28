#include <Debug.h>
#include <MenuItem.h>
#include <MessageQueue.h>
#include <PopUpMenu.h>

#include "DrawingTidbits.h"
#include "MediaPlayerApp.h"
#include "MediaController.h"
#include "PlayerWindow.h"
#include "TransportButton.h"
#include "TransportView.h"
#include "VolumeSlider.h"
#include "MessageRunner.h"
#include "ProgressBar.h"


const uint32 kPulse = 'puls';

const bigtime_t kUpdateInterval = 200000;

TransportView::TransportView(BRect rect, uint32 resizingMode)
	:	BBox(rect, "TransportView", resizingMode, B_WILL_DRAW 
			| B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE, B_PLAIN_BORDER),
		fPlayButton(0),
		fTimeSlider(0),
		fBufferStatus(0),
		fVolumeSlider(0),
		fController(0)
{
}

void TransportView::AttachToController(MediaController *controller)
{
	if (fController && controller == 0)
		fController->StopScrubbing();

	fController = controller;
	UpdateCommon();
	if (!Window()->IsLocked())
		return;
	
	if (fTimeSlider)
		fTimeSlider->SetTarget(this);

	if (!Controller())
		SetEnabled(false);	
}

MediaController *
TransportView::Controller() const
{
	return fController;
}

void 
TransportView::UpdateButtons()
{
	if (!Controller())
		return;

	if (fPlayButton) {
		// force play/pause button to show correct state
		// OK to call these repeatedly
		if (Controller()->IsPlaying())
			fPlayButton->SetPlaying();
		else if (Controller()->IsStopped())
			fPlayButton->SetStopped();
		else if (Controller()->IsPaused())
			fPlayButton->SetPaused();
	}
}

void 
TransportView::MouseDown(BPoint where)
{
	uint32 buttons;
	GetMouse(&where, &buttons);
	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		BPopUpMenu *menu = new BPopUpMenu("contextMenu", false, false);
		PlayerWindow *window = dynamic_cast<PlayerWindow *>(Window());
		window->BuildContextMenu(menu);
		menu->SetAsyncAutoDestruct(true);
		menu->Go(ConvertToScreen(where), true, false, true);
	}
}

void 
TransportView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kPulse:
			Pulse();
			break;
			
		case kBeginning:
			if (Controller())
				Controller()->Rewind();
			break;

		case kStop:
			if (Controller()) {
				if (Controller()->IsContinuous())
					Window()->PostMessage(B_QUIT_REQUESTED);
						// Just close the window is this is a continuous stream
				else
					Controller()->Stop();
			}
			
			UpdateButtons();
			break;

		case kPlayPause:
			if (Controller()) {
				if (Controller()->IsPlaying()) {
					if (!fController->IsContinuous())
						Controller()->Pause();
				} else
					Controller()->Play();
			}
			UpdateButtons();
			break;

		case kPlay:
			if (Controller())
				Controller()->Play();
			UpdateButtons();
			break;

		case kEnd:
			if (Controller())
				Controller()->GoToEnd();
			break;
		
		case kStartScrub:
			if (Controller() && fTimeSlider) {
				// defeat auto-quit when the user adjusts the out point
				MediaPlayerApp *app = dynamic_cast<MediaPlayerApp *>(be_app);
				if (app)
					app->SuppressAutoQuit();

				BMessenger control(fTimeSlider, Looper());
				Controller()->StartScrubbing(control);
			}
			break;

		case kStopScrub:
			if (Controller())
				Controller()->StopScrubbing();
			break;

		case kInPointChanged:
			if (Controller() && fTimeSlider) {
				bigtime_t newPoint = (bigtime_t)(Controller()->Length()
					* fTimeSlider->Position(kLeftThumb));

				for (;;) {
					// make sure whe don't have a bunch of messages queued up
					// - this would cause the thumb to do a little dance
					BMessage *others = Window()->MessageQueue()->FindMessage(kInPointChanged, 0);
					if (!others)
						break;
					Window()->MessageQueue()->RemoveMessage(others);
					delete others;
				}

				Controller()->SetInPoint(newPoint, true);
				fTimeSlider->UpdateTime(newPoint, kLeftThumb);
			}
			break;
			
		case kOutPointChanged:
			if (Controller() && fTimeSlider) {
				// defeat auto-quit when the user adjusts the out point
				MediaPlayerApp *app = dynamic_cast<MediaPlayerApp *>(be_app);
				if (app)
					app->SuppressAutoQuit();

				bigtime_t newPoint = (bigtime_t)(Controller()->Length()
					* fTimeSlider->Position(kRightThumb));

				for (;;) {
					// make sure whe don't have a bunch of messages queued up
					// - this would cause the thumb to do a little dance
					BMessage *others = Window()->MessageQueue()->FindMessage(kOutPointChanged, 0);
					if (!others)
						break;
					Window()->MessageQueue()->RemoveMessage(others);
					delete others;
				}

				Controller()->SetOutPoint(newPoint, true);
				fTimeSlider->UpdateTime(newPoint, kRightThumb);
			}
			break;

		case kFastForward:
			if (Controller())
				Controller()->ScanForward();
		
			break;
			
		case kRewind:
			if (Controller())
				Controller()->ScanBackward();

			break;
		
		case kDoneSkipping:
			if (Controller())
				Controller()->DoneSkipping();

			break;

		case kBumpAndRewind:
			if (Controller())
				Controller()->BumpInPointAndRewind();

			break;

		case kBumpAndGoToEnd:
			if (Controller())
				Controller()->BumpOutPointAndGoToEnd();

			break;
	
		case kNudgeForward:
			if (Controller())
				Controller()->NudgeForward();
			
			break;
		
		case kNudgeBackward:
			if (Controller())
				Controller()->NudgeBackward();

			break;
			
		case kVolumeChanged:
			if (fVolumeSlider && Controller() && Controller()->HasAudio())
				Controller()->SetVolume(fVolumeSlider->Position());

			break;

		case M_NUDGE_VOLUME_UP:
			if (Controller() && Controller()->HasAudio()) 
				Controller()->SetVolume(Controller()->Volume() + 0.1);

			break;
			
		case M_NUDGE_VOLUME_DOWN:
			if (Controller() && Controller()->HasAudio()) 
				Controller()->SetVolume(Controller()->Volume() - 0.1);

			break;

		default:
			_inherited::MessageReceived(message);
			break;
	}
}

void TransportView::Pulse()
{
	UpdateCommon();
}

void
TransportView::UpdateCommon()
{
	if (!Controller())
		return;

	ASSERT(Looper()->Thread() == -1 || Looper()->Thread() == find_thread(NULL));
	// only if the above is true can we do the lock
	// unroll/roll safely without even having to check if
	// the window got re-locked fine

	double totalLength = (double)Controller()->Length();
	bigtime_t position = Controller()->Position();
	bigtime_t inPoint = Controller()->InPoint();
	bigtime_t outPoint = Controller()->OutPoint();
	float enabledPortion = Controller()->EnabledPortion();

	if (fTimeSlider) {
		fTimeSlider->UpdatePosition((double)position / totalLength, kMainThumb);
		if (!fTimeSlider->TrackingThumb(kLeftThumb) && !fTimeSlider->TrackingThumb(kRightThumb)) {
			fTimeSlider->UpdatePosition((double)inPoint / totalLength, kLeftThumb);
			fTimeSlider->UpdatePosition((double)outPoint / totalLength, kRightThumb);
		}
	
		if (!fTimeSlider->TrackingThumb(kMainThumb))
			fTimeSlider->UpdateTime(position, kMainThumb);
		if (!fTimeSlider->TrackingThumb(kLeftThumb))
			fTimeSlider->UpdateTime(inPoint, kLeftThumb);
		if (!fTimeSlider->TrackingThumb(kRightThumb))
			fTimeSlider->UpdateTime(outPoint, kRightThumb);
	
		fTimeSlider->SetEnabledPortion(enabledPortion);
	}

	if (fBufferStatus)
		fBufferStatus->SetValue(Controller()->BufferUtilization(), Controller()->IsBuffering());

	UpdateButtons();
}

void 
TransportView::SetEnabled(bool enabled)
{
	if (fPlayButton)
		fPlayButton->SetEnabled(enabled);
}

void TransportView::AttachedToWindow()
{
	_inherited::AttachedToWindow();
	if (Controller() == 0)
		SetEnabled(false);	

	fPulser = new BMessageRunner(BMessenger(this), new BMessage(kPulse),
		kUpdateInterval);
}

void 
TransportView::DetachedFromWindow()
{
	_inherited::DetachedFromWindow();
	delete fPulser;
	fPulser = 0;
}

