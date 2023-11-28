#ifndef __MEDIA_CONTROLLER__
#define __MEDIA_CONTROLLER__

#include <Entry.h>
#include <Invoker.h>
#include <Looper.h>
#include <String.h>

class AttributeStreamNode;
class VideoView;

const int32 kSliderScale = 1000000;

class MediaController : public BLooper, public BInvoker {

// BInvoker so that we can notify a target about state changes,
// such as hitting the movie end

public:
	virtual void Delete(BLooper *owner) = 0;

	virtual status_t ConnectVideoOutput(VideoView *) = 0;

	status_t InitCheck() const;

	const entry_ref *Ref() const;

	bool HasVideo() const;
	bool HasAudio() const;
	
	bigtime_t Length() const;
	BRect VideoSize() const;
	bigtime_t VideoFramePeriod() const;

	virtual bigtime_t Position() const = 0;
	virtual void SetPosition(bigtime_t pos, bigtime_t when = 0);

	bigtime_t InPoint() const;
	virtual void SetInPoint(bigtime_t, bool tracking = false) = 0;
	
	bigtime_t OutPoint() const;
	virtual void SetOutPoint(bigtime_t, bool tracking = false) = 0;

	bool AutoLoop() const;
	void SetAutoLoop(bool);

	bool DefaultAutoLoop() const;
	void SetDefaultAutoLoop(bool);

	bool IsPlaying() const;
	virtual void Play() = 0;

	bool IsStopped() const;
	virtual void Stop() = 0;
	
	bool IsPaused() const;
	virtual void Pause() = 0;

	virtual void Rewind();
	virtual void GoToEnd();

	virtual void BumpInPointAndRewind() = 0;
	virtual void BumpOutPointAndGoToEnd() = 0;

	virtual void NudgeForward();
	virtual void NudgeBackward();
	virtual void ScanForward();
	virtual void ScanBackward();
	virtual void DoneSkipping()
		{}

	bool IsDone() const;
	
	virtual float Volume() = 0;
	virtual void SetVolume(float) = 0;

	// support for muting video when window deactivated
	void RestoreFullVolume();
	void SetHalfVolume();
	void SetMuted();

	virtual void StartScrubbing(BMessenger &target);
		// start scrubbing; pass the messenger that the MediaNodeController will
		// lock into for the scrub
	virtual void StopScrubbing();

	bool ReadyToPlay() const;
		// returns false while still opening the file

	void RestoreState(AttributeStreamNode *);
	void SaveState(AttributeStreamNode *);
	void PrefsChanged();

	void GetFileInfo(BString *);

protected:
	MediaController(const entry_ref *, bool debug = false);
	virtual ~MediaController();
	virtual void Quit();

	virtual status_t SetTo(const entry_ref *) = 0;
	void Done();

	virtual float RawVolume() const = 0;
	virtual void SetRawVolume(float) = 0;

	void OutPointHit();
		// loop or stop
	virtual void PauseAfterThumbNudge() = 0;
	virtual void PlayAfterThumbNudge();
		// we are done pausing, start playin again

	float GetScrubTargetPosition(BMessenger &, bigtime_t);

	void Debug(const char *fmt, ...);

	entry_ref fRef;
	status_t fInitCheckVal;
	bool fReadyToPlay;
	bool fHasAudio;
	bool fHasVideo;
	bigtime_t fLength;
	bigtime_t fInPoint;
	bigtime_t fOutPoint;
	bool fDefaultAutoLoop;
	bool fAutoLoop;
	
	enum PlayState {
		kPlaying,
		kPaused,
		kPausedAfterEndPointNudge,
		kStopped,
		kDone
	};
	PlayState fPlayState;

	bool fScrubbing;

	float fVideoWidth;
	float fVideoHeight;
	bigtime_t fVideoFramePeriod;
	bool fHalfVolume;
	bool fMuted;
	float fOriginalVolume;
	BString fDescriptionString;
	bool fDebugOutput;
};

const uint32 M_DONE_PLAYING = 'done';

#if DEBUG
#define ThrowOnError(error) _ThrowOnError(error, __FILE__, __LINE__)

#else
#define ThrowOnError(x) _ThrowOnError(x)
#endif

void _ThrowOnError(status_t, char * = 0, int32 = 0);

#endif
