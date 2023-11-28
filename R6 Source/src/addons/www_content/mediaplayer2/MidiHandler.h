
#include <DataIO.h>

#define X_PLATFORM      X_BE
#include <BAE.h>
#include "GenPriv.h"

class PlaybackEngine;

class MidiHandler
{
	public:
		MidiHandler(PlaybackEngine *engine);
		~MidiHandler();
		status_t SetTo(BDataIO *source);

		status_t InitCheck() const;
		status_t Pause();
		status_t Resume();

		int64    CountFrames() const;
		bigtime_t Duration() const;
		int64    CurrentFrame() const;
		bigtime_t CurrentTime() const;
		status_t SeekTo(int32 to_what, bigtime_t *inout_time, int64 *inout_frame, int32 flags=0);

	private:
		static void OverloadHook(void *);
		static void stop_midi();
		static void	timecallback(void *pReference, unsigned long currentMicroseconds, unsigned long currentMidiClock);
		static void	donecallback(void *pReference);

		PlaybackEngine *fEngine;
		static BAEOutputMixer *bae_mixer;
		static int32 fMidiRefCount;
		static image_id bae_image;
		status_t fStatus;
		BMallocIO fMallocIO;
		BAEMidiSong *bae_song;

		static BAEMidiSong *(*new_midisong)(BAEOutputMixer *mixer);
		static BAEMidiSong *(*new_rmfsong)(BAEOutputMixer *mixer);
		static BAEResult (*setmicrosecondposition)(BAEMidiSong *self, unsigned long ticks);
		static unsigned long (*getmicrosecondposition)(BAEMidiSong *self);
		static unsigned long (*getmicrosecondlength)(BAEMidiSong *self);
		static void			(*songpause)(BAEMidiSong *self);
		static void			(*songresume)(BAEMidiSong *self);
		static void			(*songsettimecallback)(BAEMidiSong *self, BAETimeCallbackPtr pSongCallback, void *pReference);
		static void			(*songsetdonecallback)(BAEMidiSong *self, BAEDoneCallbackPtr pSongCallback, void *pReference);
		static bool*		bae_overload;
		static void			(**bae_overloadhook)(void *);
};

