#include <File.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <Entry.h>
#include <Locker.h>
#include <Autolock.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <MediaTrack.h>
#include <Extractor.h>
#include <list>
#include <algorithm>

#include "MidiHandler.h"
#include "BAE_API.h"
#include "GenSnd.h"
#include "PlaybackEngine.h"

//#define DEBUG
#define DEBUG if (0) printf
#define SEEK(x) //printf x
#define SEQ(x) //printf x

static BLocker locker("midi locker");

static bigtime_t g_lastOverload;
static int g_overloadCnt = 0;
static BLocker g_listLock("g_listLock");
static list<MidiHandler *> g_list;

void MidiHandler::stop_midi()
{
	for (list<MidiHandler *>::iterator ptr(g_list.begin());
			ptr != g_list.end(); ptr++) {
		songpause((*ptr)->bae_song);
		(*ptr)->fEngine->Close();
		(*ptr)->fEngine->ReportError("B_MIDI_OVERLOAD");
	}
}

void MidiHandler::OverloadHook(void *)
{
	printf("MIDI overload hook called!\n");
	status_t locked = g_listLock.LockWithTimeout(1000000LL);
	if (locked < 0) {
		//	this should not happen, but you can't be too careful
		//	with 3ed party code involved
		fprintf(stderr, "uh-oh! deadlock in OverloadHook()\n");
	}
	*bae_overload = false;
	//	If we have 6 overloads, each within 3 seconds of the previous one,
	//	we're overloading the system with MIDI and must stop it.
	bigtime_t now = system_time();
	if (now-g_lastOverload < 3000000LL) {
		if (g_overloadCnt++ > 15) {
			printf("MIDI overload detected; stop it!\n");
			stop_midi();
			g_overloadCnt = 0;
#if ALERT_ON_OVERLOAD
			BMessage msg_alert(TB_OPEN_ALERT);
			msg_alert.AddString("itemplate","Alerts/midi.txt");
			msg_alert.AddString("template","midioverload");
			TellTellBrowser(&msg_alert);
#endif
		}
	}
	else {
		g_overloadCnt = 1;
	}
	g_lastOverload = now;
	if (locked >= 0) g_listLock.Unlock();
}


MidiHandler::MidiHandler(PlaybackEngine *engine) :
	bae_song(NULL)
{
	BAutolock autolock(&locker);
	fStatus = B_ERROR;
	DEBUG("%08x: MidiHandler ctor\n",(int)find_thread(NULL));

	fEngine = engine;

	if(fMidiRefCount++ == 0)
	{
		DEBUG("%08x: loading libbae\n",(int)find_thread(NULL));
		bae_image = load_add_on("/system/lib/libbae.so");
		if(bae_image < 0)
		{
			DEBUG("%08x: couldn't load libbae.so!!!\n",(int)find_thread(NULL));
			fMidiRefCount--;
			return;
		}

		BAEOutputMixer *(*new_outputmixer)(void);
		BAEResult (*outputmixeropen)(BAEOutputMixer *self, BAEPathName pAudioPathName = NULL,
										BAEQuality q = BAE_22K,
										BAETerpMode t = BAE_LINEAR_INTERPOLATION,
										BAEReverbType r = BAE_REVERB_TYPE_4,
										BAEAudioModifiers am = (BAE_USE_16 | BAE_USE_STEREO),
										short int maxMidiVoices = 32,
										short int maxSoundVoices = 4,
										short int mixLevel = 8,
										BAE_BOOL engageAudio = TRUE);
	
		if(get_image_symbol(bae_image,"instantiate_outputmixer",B_SYMBOL_TYPE_TEXT,(void**) &new_outputmixer)) return;
		if(get_image_symbol(bae_image,"instantiate_midisong",B_SYMBOL_TYPE_TEXT,(void**) &new_midisong)) return;
		if(get_image_symbol(bae_image,"instantiate_rmfsong",B_SYMBOL_TYPE_TEXT,(void**) &new_rmfsong)) return;
		if(get_image_symbol(bae_image,"Open__14BAEOutputMixerPv10BAEQuality11BAETerpMode13BAEReverbTypelsssc",B_SYMBOL_TYPE_TEXT,(void**) &outputmixeropen)) return;
		if(get_image_symbol(bae_image,"GetMicrosecondPosition__11BAEMidiSong",B_SYMBOL_TYPE_TEXT,(void**) &getmicrosecondposition)) return;
		if(get_image_symbol(bae_image,"SetMicrosecondPosition__11BAEMidiSongUl",B_SYMBOL_TYPE_TEXT,(void**) &setmicrosecondposition)) return;
		if(get_image_symbol(bae_image,"GetMicrosecondLength__11BAEMidiSong",B_SYMBOL_TYPE_TEXT,(void**) &getmicrosecondlength)) return;
		if(get_image_symbol(bae_image,"Pause__11BAEMidiSong",B_SYMBOL_TYPE_TEXT,(void**) &songpause)) return;
		if(get_image_symbol(bae_image,"Resume__11BAEMidiSong",B_SYMBOL_TYPE_TEXT,(void**) &songresume)) return;
		if(get_image_symbol(bae_image,"_bae_overload",B_SYMBOL_TYPE_TEXT,(void**) &bae_overload)) return;
		if(get_image_symbol(bae_image,"_bae_overloadHook",B_SYMBOL_TYPE_TEXT,(void**) &bae_overloadhook)) return;
		if(get_image_symbol(bae_image,"SetTimeCallback__11BAEMidiSongPFPvUlUl_vPv",B_SYMBOL_TYPE_TEXT,(void**) &songsettimecallback)) return;
		if(get_image_symbol(bae_image,"SetDoneCallback__11BAEMidiSongPFPv_vPv",B_SYMBOL_TYPE_TEXT,(void**) &songsetdonecallback)) return;

		*bae_overloadhook = OverloadHook;
		bae_mixer = new_outputmixer();
		outputmixeropen(bae_mixer,(BAEPathName)"/boot/beos/etc/synth/Patches.hsb",BAE_22K,
			(BAETerpMode)BAE_LINEAR_INTERPOLATION,
			(BAEReverbType)BAE_REVERB_TYPE_4,
			M_USE_STEREO | M_USE_16,
			56,4,8);
	}
	fStatus = B_OK;
	DEBUG("%08x: MidiHandler ctor OK\n",(int)find_thread(NULL));

	BAutolock lock(g_listLock);
	g_list.push_back(this);
}

MidiHandler::~MidiHandler()
{
	BAutolock autolock(&locker);
	if(bae_song)
		songsetdonecallback(bae_song, NULL, this);
	delete bae_song;
	bae_song = NULL;
	if(--fMidiRefCount <= 0)
	{
		*bae_overloadhook = NULL;
		delete bae_mixer;
		bae_mixer = NULL;
		if(bae_image >=0)
			unload_add_on(bae_image);
	}
	list<MidiHandler *>::iterator ptr(find(g_list.begin(), g_list.end(), this));
	if (ptr != g_list.end()) {
		g_list.erase(ptr);
	}
}

status_t MidiHandler::InitCheck() const
{
	BAutolock autolock(&locker);
	DEBUG("%08x: MidiHandler::InitCheck: %08lx\n",(int)find_thread(NULL),fStatus);
	return fStatus;
}

status_t MidiHandler::SetTo(BDataIO *source)
{
	BAutolock autolock(&locker);
	DEBUG("%08xMidiHandler::SetTo(BDataIO*): %08lx\n",(int)find_thread(NULL),fStatus);
	if(fStatus) return fStatus;
	fStatus = B_ERROR;

	// copy the MIDI file into memory
	fMallocIO.SetSize(0);
	char buf[1000];
	int32 numread;
	BPositionIO *posIO=dynamic_cast<BPositionIO*>(source);
	if(posIO)
	{
		posIO->Seek(0,SEEK_SET);
		while((numread=posIO->Read(buf,sizeof(buf)))>0)
			fMallocIO.Write(buf,numread);
	}
	else
	{
		while((numread=source->Read(buf,sizeof(buf)))>0)
			fMallocIO.Write(buf,numread);
	}
	
	char *buffer=(char *)fMallocIO.Buffer();
	if(!buffer)
		return B_ERROR; // sometimes happens when we can't read from the source
	if(	buffer[0]=='I' &&
		buffer[1]=='R' &&
		buffer[2]=='E' &&
		buffer[3]=='Z')
		bae_song = new_rmfsong(bae_mixer);
	else
		bae_song = new_midisong(bae_mixer);

	if(fMallocIO.BufferLength() && BAE_NO_ERROR==bae_song->LoadFromMemory(buffer,fMallocIO.BufferLength(),FALSE))
	{
		BAEResult (*songstart)(BAEMidiSong *self, BAE_BOOL useEmbeddedMixerSettings = TRUE, BAE_BOOL autoLevel = FALSE);
		if(get_image_symbol(bae_image,"Start__11BAEMidiSongcc",
			B_SYMBOL_TYPE_TEXT,(void**) &songstart) == B_OK)
		{
			fStatus = B_OK;
			songsetdonecallback(bae_song, donecallback, this);
			songsettimecallback(bae_song, timecallback, this);
			songstart(bae_song);
			DEBUG("%08lx: midi file!\n",find_thread(NULL));
			return fStatus;
		}
	}
	DEBUG("%08lx: Not a MIDI file, buffer is %ld bytes\n",find_thread(NULL),fMallocIO.BufferLength());
	delete bae_song;
	bae_song = NULL;
	fMallocIO.SetSize(0);
	return B_ERROR;
}

status_t MidiHandler::Pause()
{
	BAutolock autolock(&locker);
	DEBUG("%08lx: MidiHandler::Pause\n",find_thread(NULL));
	songpause(bae_song);
	return B_OK;
}

status_t MidiHandler::Resume()
{
	BAutolock autolock(&locker);
	DEBUG("%08lx: MidiHandler::Resume\n",find_thread(NULL));
	songresume(bae_song);
	return B_OK;
}

int64    MidiHandler::CountFrames() const
{
	DEBUG("MidiHandler::CountFrames\n");
	if(fStatus || !bae_song) return -1;
	int64 length = int64(getmicrosecondlength(bae_song))*44100/1000000;
//	DEBUG("length: %Ld\n",length);
	return length;
}

bigtime_t MidiHandler::Duration() const
{
	if(fStatus || !bae_song) return -1;
//	DEBUG("MidiHandler::Duration\n");
	return getmicrosecondlength(bae_song);
}

int64    MidiHandler::CurrentFrame() const
{
//	DEBUG("MidiHandler::CurrentFrame\n");
	if(fStatus || !bae_song) return -1;
	bigtime_t now = getmicrosecondposition(bae_song);
	return int64(now * 44100 / 1000000);
}

bigtime_t MidiHandler::CurrentTime() const
{
//	DEBUG("MidiHandler::CurrentTime\n");
	if(fStatus || !bae_song) return -1;
	return getmicrosecondposition(bae_song);;
}

status_t MidiHandler::SeekTo(int32 to_what, bigtime_t *inout_time, int64 *inout_frame, int32 /*flags*/)
{
//	DEBUG("MidiHandler::SeekToFrame\n");
	if(fStatus || !bae_song) return fStatus;

	bigtime_t whereto;
	if(to_what == B_SEEK_BY_FRAME)
		whereto = bigtime_t(*inout_frame * 1000000 / 44100);
	else
		whereto = *inout_time;

	setmicrosecondposition(bae_song,whereto);
	bigtime_t now = getmicrosecondposition(bae_song);
	*inout_frame = int64(now * 44100 / 1000000);
	*inout_time = now;

	return B_OK;
}

void MidiHandler::timecallback(void *pReference, unsigned long /*currentMicroseconds*/, unsigned long /*currentMidiClock*/)
{
//	DEBUG("MidiHandler::timecallback\n");
	((MidiHandler*)pReference)->fEngine->DoUpdate();
}

void MidiHandler::donecallback(void *pReference)
{
//	DEBUG("MidiHandler::donecallback\n");
	((MidiHandler*)pReference)->fEngine->Close();
}


BAEOutputMixer *MidiHandler::bae_mixer = NULL;
int32 MidiHandler::fMidiRefCount = 0;
image_id MidiHandler::bae_image = -1;

BAEResult (*MidiHandler::setmicrosecondposition)(BAEMidiSong *self, unsigned long ticks);
unsigned long (*MidiHandler::getmicrosecondposition)(BAEMidiSong *self);
unsigned long (*MidiHandler::getmicrosecondlength)(BAEMidiSong *self);
BAEMidiSong *(*MidiHandler::new_midisong)(BAEOutputMixer *mixer);
BAEMidiSong *(*MidiHandler::new_rmfsong)(BAEOutputMixer *mixer);
void (*MidiHandler::songpause)(BAEMidiSong *self);
void (*MidiHandler::songresume)(BAEMidiSong *self);
void (*MidiHandler::songsettimecallback)(BAEMidiSong *self, BAETimeCallbackPtr pSongCallback, void *pReference);
void (*MidiHandler::songsetdonecallback)(BAEMidiSong *self, BAEDoneCallbackPtr pSongCallback, void *pReference);
bool* MidiHandler::bae_overload;
void (**MidiHandler::bae_overloadhook)(void *);

