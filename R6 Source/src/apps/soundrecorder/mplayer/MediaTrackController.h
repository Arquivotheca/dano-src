#ifndef __MEDIA_TRACK_CONTROLLER__
#define __MEDIA_TRACK_CONTROLLER__

#define __USE_MEDIA_TRACK__
#ifdef __USE_MEDIA_TRACK__

#include "MediaController.h"

class BMediaFile;
class BMediaTrack;
class BSoundPlayer;

class AudioOutput {
public :
	AudioOutput(BMediaTrack *track, const char *name);
	~AudioOutput();

	status_t InitCheck() const;
	BMediaTrack *Track() const;

	status_t SeekToTime(bigtime_t *);
	status_t Play();
	status_t Stop();
	bool IsPlaying() const;
	bigtime_t TrackTimebase() const;
	
	float Volume() const;
	void SetVolume(float);
private :
	static void AudioPlay(void *, void *, size_t, const media_raw_audio_format &);
	
	void Lock();
	void Unlock();
	
	bool isPlaying;
	int32 frameSize;
	int32 lock_count;
	int32 channelCount;
	int8 default_data;
	uint32 frame_size;
	float frame_rate;
	uint32 buffer_size;
	sem_id lock_sem;
	BMediaTrack *track;
	BSoundPlayer *player;
	bigtime_t perfTime;
	bigtime_t trackTime;
	status_t fInitCheckValue;
};

class MediaTrackController : public MediaController {
public:
	static MediaController *Open(const entry_ref *, BLooper *target,
		status_t *error = 0, bool debug = false);
	
	
	virtual void Delete(BLooper *owner);

	virtual void MessageReceived(BMessage*);
	
	virtual status_t ConnectVideoOutput(VideoView *);
	
	virtual bigtime_t Position() const;
	
	virtual void SetInPoint(bigtime_t, bool tracking = false);
	virtual void SetOutPoint(bigtime_t, bool tracking = false);

	virtual void Play();
	virtual void Stop();
	virtual void Pause();

	virtual void NudgeForward();
	virtual void NudgeBackward();
	virtual void ScanForward();
	virtual void ScanBackward();
	virtual void DoneSkipping();

	virtual void BumpInPointAndRewind();
	virtual void BumpOutPointAndGoToEnd();

	virtual float Volume();
	virtual void SetVolume(float);

protected:
	MediaTrackController(const entry_ref *, bool debug);
	virtual ~MediaTrackController();

	static bool CanHandle(const entry_ref *);

	virtual status_t SetTo(const entry_ref *);
	
	virtual void PauseAfterThumbNudge();

	status_t SetVideoTrack(BMediaTrack *, const media_format *);
	status_t SetAudioTrack(const char *, BMediaTrack *, const media_format *);

	virtual float RawVolume() const;
	virtual void SetRawVolume(float);

	void _SetInPoint(bigtime_t);
	void _SetOutPoint(bigtime_t);
	
	virtual status_t ReadyToLoop(loop_state* outState);
	bool ExecuteLoop(loop_state* outState);
	
private:
	
	void SeekSoundTo(bigtime_t);
	
	void Reset();
	
	BMediaFile *fMediaFile;
	BMediaTrack *fVideoTrack;
	BMediaTrack *fAudioTrack;
	
	AudioOutput *fAudioOutput;

	BBitmap *fBitmap;
	color_space fBitmapDepth;
	VideoView *fVideoView;

	bigtime_t fCurTime;
	bigtime_t fScrubTime;

	bool fPlaying;
	bool fSnoozing;
	void *fAudioDumpingBuffer;
	
	bool fSeekToTimeNeeded;
	bool fSeekToFrameNeeded;
	bool fScanningForward;
	bool fScanningBackward;
	bigtime_t fSeekTime;
	int64 fSeekFrame;
	
	// Looper state 
	bool fLooping; 
	BMessenger fLocalScrubTarget;
	bool fLocalScrubbing;
	bigtime_t fLocalLength;
	BMediaTrack *fCounterTrack;
	bigtime_t fVideoStartTime;
	bigtime_t fStartTime;
	bigtime_t fCurScrubbing;
	bigtime_t fLastScrubbing;
	int64 fNumFramesToSkip;
	int64 fNumSkippedFrames;
	int32 fNumSkippedInARow;
		// count how many frames we skipped in a sequence
		// every now and then show a frame even if it's behind
	float fLastLocalScrubTargetPosition;
		// used to detect if there was a change in the scrub location
};

#endif

#endif
