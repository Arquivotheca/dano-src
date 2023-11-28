#include <Control.h>
#include <AutoLock.h>

#include "Attributes.h"
#include "AttributeStream.h"
#include "DrawingTidbits.h"
#include "MediaController.h"
//#include "MediaPlayerApp.h"
#include "MultiThumbMediaSlider.h"
//#include "PlayerWindow.h"

// some internal message what fields
const uint32 kRescheduleWakeup = 'rsch';
const uint32 kPlay = 'play';
const uint32 kPause = 'paus';
const uint32 kStop = 'stop';
const uint32 kSetAutoLoop = 'auto';
const uint32 kSetPosition = 'spos';
const uint32 kSetOutPoint = 'soup';
const uint32 kSetInPoint = 'sinp';
const uint32 kScrubTo = 'scrb';
const uint32 kSetScrubbing = 'sscr';
const uint32 kBumpAndRewind = 'bprw';
const uint32 kBumpAndGoToEnd = 'bpge';
const uint32 kNudgeForward = 'ngfw';
const uint32 kNudgeBackward = 'ngbk';
const uint32 kScanForward = 'scfw';
const uint32 kScanBackward = 'scbk';
const uint32 kDoneScanning = 'dnsc';

const bigtime_t kFastSeekIncrement = 250000;

MediaController::MediaController(const entry_ref *ref, bool debug)
	:	BLooper("MediaTrackController controller", B_NORMAL_PRIORITY),
		fRef(*ref),
		fInitCheckVal(B_OK),
		fReadyToPlay(false),
		fHasAudio(false),
		fHasVideo(false),
		fDefaultAutoLoop(true),
		fAutoLoop(false),
		fPlayState(kStopped),
		fScrubbing(false),
		fVideoFramePeriod(10000),
		fHalfVolume(false),
		fMuted(false),
		fOriginalVolume(0),
		fDebugOutput(debug)
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

const entry_ref *
MediaController::Ref() const
{
	return &fRef;
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

bigtime_t 
MediaController::OutPoint() const
{
	return fOutPoint;
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
	return fPlayState == kStopped;
}

bool 
MediaController::IsPlaying() const
{
	PlayState tmp = fPlayState;
		// avoid having to lock
	return tmp == kPlaying || tmp == kPausedAfterEndPointNudge;
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

		if (InPoint() == OutPoint())
			// trapped between in and out point, wait for a bit,
			// can't play anyway
			PauseAfterThumbNudge();

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
	return fPlayState == kPaused;
}

bool 
MediaController::IsDone() const
{
	return fPlayState == kDone;
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
		SetOutPoint(tmpTime);


	// preset the default loop mode
	bool loop = false;
	/*MediaPlayerApp *app = dynamic_cast<MediaPlayerApp *>(be_app);
	if (HasVideo())
		loop = app->LoopMoviesByDefault();
	else
		loop = app->LoopSoundsByDefault();
	*/
	bool autoLoopVal;
	if (stream->Read(kAutoLoopAttribute, 0, B_BOOL_TYPE, sizeof(autoLoopVal), &autoLoopVal)
		== sizeof(autoLoopVal) && autoLoopVal != loop) {
		SetDefaultAutoLoop(false);
		loop = autoLoopVal;
	} else 
		SetDefaultAutoLoop(true);

	SetAutoLoop(loop);
}

void 
MediaController::SaveState(AttributeStreamNode *stream)
{
	/*float volume = Volume();
	stream->Write(kVolumeAttribute, 0, B_FLOAT_TYPE, sizeof(volume), &volume);

#ifdef SAVE_POS
	bigtime_t pos = Pos();
	stream->Write(kPosAttribute, 0, B_INT64_TYPE, sizeof(pos), &pos);
#endif
	stream->Write(kInPointAttribute, 0, B_INT64_TYPE, sizeof(fInPoint), &fInPoint);
	stream->Write(kOutPointAttribute, 0, B_INT64_TYPE, sizeof(fOutPoint), &fOutPoint);

	// save autoLoop settings only if they differ from default 
	// bool defaultLoop = false;
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
	*/
}

void 
MediaController::PrefsChanged()
{
	/*
	if (DefaultAutoLoop()) {
		bool defaultLoop = false;
		MediaPlayerApp *app = dynamic_cast<MediaPlayerApp *>(be_app);
		if (HasVideo())
			defaultLoop = app->LoopMoviesByDefault();
		else
			defaultLoop = app->LoopSoundsByDefault();
		
		SetAutoLoop(defaultLoop);
	}
	*/
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


void
MediaController::Done()
{
	fPlayState = kDone;
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

void
MediaController::Debug(const char *fmt, ...)
{
	va_list ap; 
	va_start(ap, fmt); 
	vfprintf(stderr, fmt, ap);
	va_end(ap); 		
}

void 
_ThrowOnError(status_t error, char *DEBUG_ONLY(file), int32 DEBUG_ONLY(line))
{
	if (error != B_OK) {
		PRINT(("failing %s at %s:%d\n", strerror(error), file, line));
		throw error;
	}
}
