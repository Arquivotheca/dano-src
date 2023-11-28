#include <Control.h>

#include "Attributes.h"
#include "AttributeStream.h"
#include "DrawingTidbits.h"
#include "MediaController.h"
#include "MediaPlayerApp.h"
#include "MultiThumbMediaSlider.h"
#include "PlayerWindow.h"
#include "debug.h"

const bigtime_t kFastSeekIncrement = 250000;

MediaController::MediaController()
	:	BLooper("MediaTrackController controller", B_NORMAL_PRIORITY),
		fInitCheckVal(B_OK),
		fReadyToPlay(false),
		fHasAudio(false),
		fHasVideo(false),
		fDefaultAutoLoop(true),
		fAutoLoop(false),
		fVideoFramePeriod(10000),
		fHalfVolume(false),
		fMuted(false),
		fOriginalVolume(0),
		fDropFrames(true),
		fCurrentState(STATE_INIT),
		fSaveSlot(0),
		fStateChanged(false)
{
}

MediaController::~MediaController()
{
}

void 
MediaController::Quit()
{
	Unlock();
	PostMessage(B_QUIT_REQUESTED);
	Lock();
	BLooper::Quit(); 
}

status_t
MediaController::InitCheck() const
{
	return fInitCheckVal;
}

bigtime_t 
MediaController::Length() const
{
	return fLength;
}

bigtime_t 
MediaController::InPoint() const
{
	return fInPoint;
}

void
MediaController::ChangeInPoint(bigtime_t inpoint, bool loading)
{
	if (inpoint!=fInPoint)
	{
		fInPoint=inpoint;
		if (!loading)
		{
			SetStateChanged();
		}
	}
}

bigtime_t 
MediaController::OutPoint() const
{
	return fOutPoint;
}

void
MediaController::ChangeOutPoint(bigtime_t outpoint, bool loading)
{
	if (outpoint!=fOutPoint)
	{
		fOutPoint=outpoint;
		if (!loading)
		{
			SetStateChanged();
		}
	}
}

bool 
MediaController::AutoLoop() const
{
	return fAutoLoop;
}

void 
MediaController::SetAutoLoop(bool on)
{
	fAutoLoop = on;
}

bool 
MediaController::DefaultAutoLoop() const
{
	return fDefaultAutoLoop;
}

void 
MediaController::SetDefaultAutoLoop(bool on)
{
	fDefaultAutoLoop = on;
}

bool 
MediaController::IsStopped() const
{
	return CurrentState() == STATE_STOPPED;
}

bool 
MediaController::IsPlaying() const
{

	ControlState tmp = CurrentState();
		// avoid having to lock
	return tmp == STATE_PLAYING || tmp == STATE_PAUSE_AFTER_END_POINT_NUDGE;
}

bool
MediaController::ReadyToPlay() const
{
	return fReadyToPlay;
}

bool 
MediaController::HasAudio() const
{
	return fHasAudio;
}

bool 
MediaController::HasVideo() const
{
	return fHasVideo;
}

BRect
MediaController::VideoSize() const
{
	ASSERT(HasVideo());
	return BRect(0, 0, fVideoWidth - 1, fVideoHeight - 1);
}

bigtime_t
MediaController::VideoFramePeriod() const
{
	return fVideoFramePeriod;
}

void 
MediaController::NudgeForward()
{
	bigtime_t pos = Position();
	bigtime_t fileLen = Length();
	if (pos < fileLen - fVideoFramePeriod)
		SetPosition(pos + fVideoFramePeriod);
}

void 
MediaController::NudgeBackward()
{
	bigtime_t pos = Position();
	if (pos > fVideoFramePeriod)
		SetPosition(pos - fVideoFramePeriod);
}

void 
MediaController::ScanForward()
{
	bigtime_t pos = Position();
	bigtime_t fileLen = Length();
	if (pos < fileLen - kFastSeekIncrement)
		SetPosition(pos + kFastSeekIncrement);
}

void 
MediaController::ScanBackward()
{
	bigtime_t pos = Position();
	if (pos > kFastSeekIncrement)
		SetPosition(pos - kFastSeekIncrement);
}

void 
MediaController::SetPosition(bigtime_t pos, bigtime_t when)
{
	BMessage message(kSetPosition);
	message.AddInt64("be:pos", pos);
	message.AddInt64("be:when", when);
	PostMessage(&message);
}


void 
MediaController::StartScrubbing(BMessenger &target)
{
	BMessage message(kSetScrubbing);
	message.AddBool("be:scrub", true);
	message.AddMessenger("be:scrubDriver", target);
	PostMessage(&message);
}

void 
MediaController::StopScrubbing()
{
	BMessage message(kSetScrubbing);
	message.AddBool("be:scrub", false);
	PostMessage(&message);
}

void 
MediaController::OutPointHit()
{
	if (fAutoLoop) {
		Rewind();
#if 0
		if (InPoint() == OutPoint())
			// trapped between in and out point, wait for a bit,
			// can't play anyway
			PauseAfterThumbNudge();
#endif
	} else {
		Stop();
		Done();
	}
}

void 
MediaController::PlayAfterThumbNudge()
{
	Play();
}

bool 
MediaController::IsPaused() const
{
	return CurrentState() == STATE_PAUSED;
}

void 
MediaController::Rewind()
{
	SetPosition(fInPoint);
}

void 
MediaController::GoToEnd()
{
	SetPosition(fOutPoint);
}

void 
MediaController::RestoreState(AttributeStreamNode *stream)
{
	float volume;
	if (stream->Read(kVolumeAttribute, 0, B_FLOAT_TYPE, sizeof(volume), &volume)
		== sizeof(volume)) 
		SetVolume(volume);
	bigtime_t tmpTime;
	if (stream->Read(kPosAttribute, 0, B_INT64_TYPE, sizeof(tmpTime), &tmpTime)
		== sizeof(tmpTime)) 
		SetPosition(tmpTime);
	if (stream->Read(kInPointAttribute, 0, B_INT64_TYPE, sizeof(tmpTime), &tmpTime)
		== sizeof(tmpTime)) 
		SetInPoint(tmpTime);
	if (stream->Read(kOutPointAttribute, 0, B_INT64_TYPE, sizeof(tmpTime), &tmpTime)
		== sizeof(tmpTime))
	{
		// If the outpoint is 0, the attribute was probably erroneously saved
		// by a previous version of the program and it should be ignored.
		if (tmpTime!=0LL)
		{
			SetOutPoint(tmpTime);
		}
	} 

	// preset the default loop mode
	bool loop = false;
	MediaPlayerApp *app = dynamic_cast<MediaPlayerApp *>(be_app);
	if (HasVideo())
		loop = app->LoopMoviesByDefault();
	else
		loop = app->LoopSoundsByDefault();

	bool autoLoopVal;
	if (stream->Read(kAutoLoopAttribute, 0, B_BOOL_TYPE, sizeof(autoLoopVal), &autoLoopVal)
		== sizeof(autoLoopVal) && autoLoopVal != loop) {
		SetDefaultAutoLoop(false);
		loop = autoLoopVal;
	} else 
		SetDefaultAutoLoop(true);

	SetAutoLoop(loop);
	
	bool dropFrames;
	if (stream->Read(kDropFramesAttribute, 0, B_BOOL_TYPE, sizeof(dropFrames),
		&dropFrames) == sizeof(dropFrames))
		SetDropFrames(dropFrames != 0);
	else
		SetDropFrames(true);
	
	SetStateChanged(false);
}

void 
MediaController::SaveState(AttributeStreamNode *stream)
{
	bool fOutPointCorrected;
	float volume = Volume();
	stream->Write(kVolumeAttribute, 0, B_FLOAT_TYPE, sizeof(volume), &volume);

#ifdef SAVE_POS
	bigtime_t pos = Pos();
	stream->Write(kPosAttribute, 0, B_INT64_TYPE, sizeof(pos), &pos);
#endif
	stream->Write(kInPointAttribute, 0, B_INT64_TYPE, sizeof(fInPoint), &fInPoint);
	
	// If the user set the outpoint to 0 and the length of the
	// current media is longer than 0, set it to 1 (microsecond)
	// before saving it, to prevent the RestoreState function from
	// interpreting it as having been saved by a previous media player.
	if (fOutPointCorrected=((fOutPoint==0LL) && (IsContinuous()) || (fLength>0LL)))
	{
		fOutPoint++;
	}
	
	stream->Write(kOutPointAttribute, 0, B_INT64_TYPE, sizeof(fOutPoint), &fOutPoint);

	// Reset the outpoint to 0 if it was corrected, just to make sure
	// that the user interface will stay up-to-date with reality for
	// really, really short files.
	if (fOutPointCorrected)
	{
		fOutPoint=0LL;
	}

	// save autoLoop settings only if they differ from default 
	bool defaultLoop = false;
	MediaPlayerApp *app = dynamic_cast<MediaPlayerApp *>(be_app);
	if (HasVideo())
		defaultLoop = app->LoopMoviesByDefault();
	else
		defaultLoop = app->LoopSoundsByDefault();

	if (DefaultAutoLoop()) {
		AttributeStreamFileNode *fileNode = dynamic_cast<AttributeStreamFileNode *>(stream);
		if (fileNode)
			fileNode->Node()->RemoveAttr(kAutoLoopAttribute);
	} else
		stream->Write(kAutoLoopAttribute, 0, B_BOOL_TYPE, sizeof(fAutoLoop), &fAutoLoop);

	stream->Write(kDropFramesAttribute, 0, B_BOOL_TYPE, sizeof(fDropFrames), &fDropFrames);

	SetStateChanged(false);
}

bool MediaController::StateChanged()
{
	return fStateChanged;
}

void MediaController::SetStateChanged(bool changed)
{
	fStateChanged = changed;
}

void 
MediaController::PrefsChanged()
{
	if (DefaultAutoLoop()) {
		bool defaultLoop = false;
		MediaPlayerApp *app = dynamic_cast<MediaPlayerApp *>(be_app);
		if (HasVideo())
			defaultLoop = app->LoopMoviesByDefault();
		else
			defaultLoop = app->LoopSoundsByDefault();
		
		SetAutoLoop(defaultLoop);
	}
	
	SetStateChanged(true);
}


float
MediaController::GetScrubTargetPosition(BMessenger &scrubTarget, bigtime_t length)
{
	// static, called with an unlocked MediaNodeController

	// peek at the value of the controlling slider
	MessengerAutoLocker lock(&scrubTarget);
	if (!lock.IsLocked())
		return 0;

	BControl *controller = dynamic_cast<BControl *>(scrubTarget.Target(0));
	if (!controller)
		return 0;
	
	float result = controller->Value();
	result /= kSliderScale;

	// while we have the slider locked, update the time value so that the time label
	// draws correctly
	// it has to be done this way because the slider has no way of figuring out what the
	// time value is
	TMultiThumbMediaSlider *multiThumbSlider = dynamic_cast<TMultiThumbMediaSlider *>
		(controller);
	if (multiThumbSlider)
		multiThumbSlider->UpdateMainTime((bigtime_t)((double)result * (double)length));

	return result;
}

void MediaController::Done()
{
	SetState(STATE_STOPPED);
	Invoke(new BMessage(M_DONE_PLAYING));	// notify our target that we are done
}

void 
MediaController::RestoreFullVolume()
{
	if (fHalfVolume) 
		fHalfVolume = false;
	else if (fMuted)
		fMuted = false;
	else
		return;

	SetRawVolume(fOriginalVolume);
}

void
MediaController::SetVolume(float)
{
	// All the work is done in the subclasses
	SetStateChanged(true);
}

void 
MediaController::SetHalfVolume()
{
	fOriginalVolume = RawVolume();
	SetVolume(Volume() / 3);

	fHalfVolume = true;
}

void 
MediaController::SetMuted()
{
	fOriginalVolume = RawVolume();

	SetVolume(0);
	fMuted = true;
}

void
MediaController::GetFileInfo(BString *str)
{
	*str = fDescriptionString;
}

float MediaController::EnabledPortion() const
{
	return 1.0;
}

void MediaController::SetState(ControlState state)
{
#if 0
	const char *stateNames[] =  {
		"init", "loading", "stopped", "playing", "stream_wait", "scrubbing",
		"paused", "seeking", "scanning forward", "scanning backward", "pause after endpoint",
		"cleanup"
	};

	printf("current state is %s\n", stateNames[fCurrentState]);
#endif

	fCurrentState = state;
}

ControlState MediaController::CurrentState() const
{
	return fCurrentState;
}

ControlState MediaController::PopState()
{
	if (fSaveSlot <= 0) {
		TRESPASS();
		fSaveSlot = 1;
	}
	
	return fSavedState[--fSaveSlot];
}

void MediaController::PushState(ControlState state)
{
	if (fSaveSlot >= kStateStackSize) {
		TRESPASS();
		fSaveSlot--;
	}
	
	fSavedState[fSaveSlot++] = state;
}

const char* MediaController::GetConnectionName()
{
	return "";
}

bool MediaController::IsContinuous() const
{
	return true;
}

float MediaController::BufferUtilization() const
{
	return 1.0;
}

bool MediaController::IsBuffering() const
{
	return false;
}

void MediaController::GetStats(player_stats *) const
{
}

bool MediaController::DropFrames() const
{
	return fDropFrames;
}

void MediaController::SetDropFrames(bool drop)
{
	fDropFrames = drop;
}

bool MediaController::NeedsFrameDropping()
{
	return false;
}

void MediaController::GetOutputSizeLimits(float *outMinWidth, float *outMaxWidth, float *outMinHeight, float *outMaxHeight)
{
	*outMinWidth = 0;
	*outMinHeight = 0;
	*outMaxWidth = 1000000.0;
	*outMaxHeight = 1000000.0;
}



