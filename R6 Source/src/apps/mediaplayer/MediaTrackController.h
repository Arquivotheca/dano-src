#ifndef __MEDIA_TRACK_CONTROLLER__
#define __MEDIA_TRACK_CONTROLLER__

#include <Locker.h>
#include "MediaController.h"
#include "StreamHandler.h"

class BMediaFile;
class BMediaTrack;
class BSoundPlayer;
class HTTPStream;
class URL;
class media_format;
class PlayListHandler;
struct media_raw_audio_format;

class MediaTrackController : public MediaController {
public:
	static MediaController *Open(const URL &url, const char *cookies, const BMessenger &target,
		status_t *error = 0);

	// It is assumed that this will be called from the window thread.
	virtual void Close();

	virtual void DispatchMessage(BMessage*, BHandler*);
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
	
	virtual float EnabledPortion() const;
	virtual const char *GetConnectionName();
	virtual bool IsContinuous() const;
	virtual float BufferUtilization() const;
	virtual bool IsBuffering() const;
	virtual void GetStats(player_stats *stats) const;
	virtual void GetOutputSizeLimits(float *outMinWidth, float *outMaxWidth,
		float *outMinHeight, float *outMaxHeight);

	
protected:
	MediaTrackController();
	virtual status_t SetupTracks(BString *failingFormatDescription);
	
	virtual void PauseAfterThumbNudge();

	status_t SetVideoTrack(BMediaTrack *, const media_format *);
	status_t SetAudioTrack(const char *, BMediaTrack *, const media_format *);

	void AddAudioDecodeTime(bigtime_t);
	virtual float RawVolume() const;
	virtual void SetRawVolume(float);

	void _SetInPoint(bigtime_t);
	void _SetOutPoint(bigtime_t);

	virtual status_t ReadyToLoop(loop_state* outState);
	bool ExecuteLoop(loop_state* outState);
	
private:
	static void InterruptSocketRead(int);
	status_t SetTo(const URL &url, const char *cookies, BString *failingFormatDescription);

	bool IsDataAvailable(bigtime_t atTime, bigtime_t timeNeeded);
	void StartPlaying();
	void Cleanup();

	void SetBitmap(BBitmap*, bool usingOverlay = false);
	void DecodeAudio(void *buffer, size_t bufferSize);
	static void SoundPlayerHook(void *castTospc, void *buffer, size_t bufferSize, const media_raw_audio_format &);

	void SeekSoundTo(bigtime_t);
	void SeekToTime(bigtime_t);
	void AbortPendingSeek();

	// Common state
	BMediaFile *fMediaFile;
	BString fFileName;
	StreamHandler *fRawDataStream;
	bigtime_t fStartTime;
	PlayListHandler *fPlayListHandler;
	int32 fFormatBitRate;

	// Video State	
	BMediaTrack *fVideoTrack;
	BLocker fVideoViewLock;
	VideoView *fVideoView;
	BBitmap *fBitmaps[3];
	int fNumBitmaps;
	int fCurrentDecodeBitmap;
	bigtime_t fDecodedFramePresentationTime;
	BMessenger fScrubTarget;
	bigtime_t fLogicalMediaTime;
	bool fUsingOverlay;
	bool fIsContinuousStream;

	// Audio State
	BMediaTrack *fAudioTrack;
	BSoundPlayer *fSoundPlayer;
	bigtime_t fAudioLatency;
	mutable BLocker fSoundPlayerLock;
	unsigned int fAudioZeroValue;
	void *fAudioDumpingBuffer;
	int32 fAudioFrameSize;
	bool fAudioThreadInitialized;
	
	bigtime_t fLastAudioMediaTime;
	bigtime_t fLastAudioSystemTime;
	
	// Statistics
	player_stats fStats;
	bigtime_t fLastPlayUpdate;
	int fCurrentFrame;
	bigtime_t fLastVideoDecodeSample;
	bigtime_t fLastAudioDecodeSample;
	
	// Looper state
	bool fLooping;
	float fLastScrubPosition;
	
	typedef MediaController _inherited;
};

#endif
