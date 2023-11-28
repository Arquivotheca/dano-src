#ifndef _PLAYBACK_ENGINE_H
#define _PLAYBACK_ENGINE_H

#include <Locker.h>
#include <Protocol.h>
#include <list>

class IOAdapter;
class BSoundPlayer;
class BMediaFile;
class BMediaTrack;
class MidiHandler;
struct media_raw_audio_format;

using namespace Wagner;

typedef void (*NotifyFunc)(void *data);
typedef void (*ErrorFunc)(void *data, const char *error);
typedef void (*PropertyFunc)(void *data, const char *prop);

class PlaybackEngine {
friend class MidiHandler;
public:
	PlaybackEngine();
	~PlaybackEngine();
	status_t Start(const URL &url, bool sync = false);
	void Stop();
	void TogglePause();
	const URL& GetUrl() const;
	void SetStopNotify(NotifyFunc, void*);
	void SetUpdateNotify(NotifyFunc, void*);
	void SetErrorNotify(ErrorFunc, void*);
	void SetPropertyNotify(PropertyFunc, void*);
	void ReportError(const char *error);
	void NotifyProperty(const char *prop);
	float GetCompletion();
	int32 GetElapsedTime();
	int32 GetTotalTime();
	const char *GetStreamName() const;
	const char *GetStreamGenre() const;
	const char *GetHomePage() const;
	bool DidNameChange();
	bool IsBuffering() {return fRebuffering;}
	bool IsOpening() {return fOpening;}

private:
	static int32 AsyncOpenStart(void *eng);
	static long DoNextURL(void *data);
	status_t NextURL();
	inline void ReadBufferedAudio(void *buffer, size_t bufferSize);
	static void SoundPlayerHook(void *controller, void *buffer, size_t bufferSize,
		const media_raw_audio_format &);
	static void ProcessMeta(void *engine, const char *name, const char *value);
	void Close();
	status_t InitializeStream();
	void DecodeAndBufferAudio();
	void DoUpdate(bool force=false);

	typedef list<URL> url_list;
	url_list fURLList;

	int32 fContentLength;
	size_t fAudioBufferSize;
	IOAdapter *fIOAdapter;
	BMediaFile *fMediaFile;
	MidiHandler *fMidiHandler;
	thread_id fDecoderThread;
	BSoundPlayer *fSoundPlayer;
	int32 fAudioFrameSize;
	unsigned int fAudioZeroValue;
	BMediaTrack *fAudioTrack;
	Protocol *fProtocol;
	bool fStopFlag;
	bool fPaused;
	bool fNameChanged;
	bool fStopNotified;
	NotifyFunc fStopNotify;
	void *fStopData;
	bigtime_t fLastNotify;
	NotifyFunc fUpdateHook;
	void *fUpdateData;
	ErrorFunc fErrorHook;
	void *fErrorData;
	PropertyFunc fPropertyHook;
	void *fPropertyData;
	int32 fDecodedBufferAvailable;
	sem_id fFreeBufAvail;
	sem_id fInitializeWait;
	char *fRawAudioBuffer;
	int32 fReadPos;
	int32 fWritePos;
	int32 fBufferCount;
	int32 fNumFramesOutput;
	bool fEndOfStream;
	bool fOpening;
	long fRebuffering;
	BString fStreamName;
	BString fStreamGenre;
	BString fStreamUrl;
};

#endif
