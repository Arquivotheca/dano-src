#ifndef __MEDIA_CONTROLLER__
#define __MEDIA_CONTROLLER__

#include <Entry.h>
#include <Invoker.h>
#include <Looper.h>
#include <String.h>

namespace BPrivate {

class AttributeStreamNode;

}
using namespace BPrivate;

class VideoView;

const int32 kSliderScale = 1000000;

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
const int32 kStateStackSize = 4;

enum ControlState {
	STATE_INIT,
	STATE_LOADING,
	STATE_STOPPED,
	STATE_PLAYING,
	STATE_STREAM_WAIT,
	STATE_SCRUBBING,
	STATE_PAUSED,
	STATE_SEEKING,
	STATE_SCANNING_FORWARD,
	STATE_SCANNING_BACKWARD,
	STATE_PAUSE_AFTER_END_POINT_NUDGE,
	STATE_CLEANUP
};


struct player_stats {
	bool has_video;
	int64 frames_played;
	int64 frames_dropped;
	int using_overlay;
	int num_buffers;
	
	float audio_decode_time;
	float video_decode_time;
	bigtime_t video_display_time;
	bigtime_t total_play_time;
	
	double raw_data_rate;

	int is_network_stream;
	double connection_rate;
};


class MediaController : public BLooper, public BInvoker {

// BInvoker so that we can notify a target about state changes,
// such as hitting the movie end

public:
	virtual void Close() = 0;

	virtual status_t ConnectVideoOutput(VideoView *) = 0;

	status_t InitCheck() const;

	virtual const char* GetConnectionName();

	bool HasVideo() const;
	bool HasAudio() const;
	
	bigtime_t Length() const;
	BRect VideoSize() const;
	bigtime_t VideoFramePeriod() const;

	virtual bigtime_t Position() const = 0;
	virtual void SetPosition(bigtime_t pos, bigtime_t when = 0);

	bigtime_t InPoint() const;
	virtual void SetInPoint(bigtime_t, bool tracking = false) = 0;
	void ChangeInPoint(bigtime_t inpoint, bool loading = false); // sets state-change flag except when loading

	bigtime_t OutPoint() const;
	virtual void SetOutPoint(bigtime_t, bool tracking = false) = 0;
	void ChangeOutPoint(bigtime_t outpoint, bool loading = false); // sets state-change flag except when loading

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

	virtual float Volume() = 0;
	virtual void SetVolume(float);

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
	bool StateChanged();
	void PrefsChanged();

	void GetFileInfo(BString *);

	virtual float EnabledPortion() const;
	virtual bool IsContinuous() const;
	virtual float BufferUtilization() const;
	virtual bool IsBuffering() const;

	virtual void GetStats(player_stats *stats) const;

	bool DropFrames() const;
	void SetDropFrames(bool drop);
	virtual bool NeedsFrameDropping();
	
	virtual void GetOutputSizeLimits(float *outMinWidth, float *outMaxWidth,
		float *outMinHeight, float *outMaxHeight);

protected:
	MediaController();
	virtual ~MediaController();
	virtual void Quit();

	void Done();

	virtual float RawVolume() const = 0;
	virtual void SetRawVolume(float) = 0;

	void OutPointHit();
		// loop or stop
	virtual void PauseAfterThumbNudge() = 0;
	virtual void PlayAfterThumbNudge();
		// we are done pausing, start playin again

	float GetScrubTargetPosition(BMessenger &, bigtime_t);

	void PushState(ControlState);
	ControlState PopState();
	void SetState(ControlState);
	ControlState CurrentState() const;

	status_t fInitCheckVal;
	bool fReadyToPlay;
	bool fHasAudio;
	bool fHasVideo;
	bigtime_t fLength;
	bool fDefaultAutoLoop;
	bool fAutoLoop;


	float fVideoWidth;
	float fVideoHeight;
	bigtime_t fVideoFramePeriod;
	bool fHalfVolume;
	bool fMuted;
	float fOriginalVolume;
	BString fDescriptionString;
	bool fDropFrames;

private:

	void SetStateChanged(bool changed=true);

	ControlState fCurrentState;
	ControlState fSavedState[kStateStackSize];
	int fSaveSlot;

	bool fStateChanged; // true if SaveState() needs to be done
	bigtime_t fInPoint;
	bigtime_t fOutPoint;
};

#endif
