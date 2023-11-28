/* Glue Between libbae and the midi kit
**
**
*/

#include <BAE.h>
#include <Path.h>
#include <Synth.h>
#include <SynthUtils.h>
#include <MidiSynth.h>
#include <MidiSynthFile.h>
#include <File.h>
#include <ByteOrder.h>
#include <glue.h>
#include <image.h>
#include <Debug.h>
#include <FindDirectory.h>

#define DYNLOAD 0

static long
translate_error(BAEResult theErr)
{
	switch (theErr)	{
	case BAE_NO_ERROR:					return B_NO_ERROR;
	case BAE_PARAM_ERR:					return B_BAD_VALUE;
	case BAE_MEMORY_ERR:				return B_NO_MEMORY;
	case BAE_BAD_INSTRUMENT:			return B_BAD_INSTRUMENT;
	case BAE_BAD_MIDI_DATA:				return B_BAD_MIDI_DATA;
	case BAE_ALREADY_PAUSED:			return B_ALREADY_PAUSED;
	case BAE_ALREADY_RESUMED:			return B_ALREADY_RESUMED;
	case BAE_NO_SONG_PLAYING:			return B_NO_SONG_PLAYING;
	case BAE_TOO_MANY_SONGS_PLAYING:	return B_TOO_MANY_SONGS_PLAYING;
	}
	return B_ERROR;
}

static long
  quality_to_sample_rate (BAEQuality q)
{
	// these aren't all the rates supported by the BAE, but they'll do
	switch (q) {
	case BAE_11K:	return 11025;
	case BAE_22K:	return 22050;
	case BAE_24K:	return 24000;
	case BAE_48K:	return 48000;
	default:		return 44100;
	}  
}

static BAEQuality
  sample_rate_to_quality (long sample_rate)
{
	// these aren't all the rates supported by the BAE, but they'll do
	if (sample_rate < (11025+22050) / 2)
	  return BAE_11K;
	else if (sample_rate < (22050+24000) / 2)
	  return BAE_22K;
	else if (sample_rate < (24000+44100) / 2)
	  return BAE_24K;
	else if (sample_rate < (44100+48000) / 2)
	  return BAE_44K;
	else
	  return BAE_48K;
}

static BAEAudioModifiers
  translate_modifiers (long bytes_per_sample,
					   long channel_count,
					   bool enable_reverb)
{
	return (bytes_per_sample > 1 ? BAE_USE_16 : 0)
	  | (channel_count > 1 ? BAE_USE_STEREO : 0)
		| (enable_reverb ? 0 : BAE_DISABLE_REVERB);
}

/* ------------------------------------------------------------------- */

static BAEOutputMixer *bae_mixer = NULL;
BSynth *be_synth = NULL;

#if DYNLOAD
static BAEOutputMixer *(*new_outputmixer)(void) = NULL;
static BAEMidiSynth *(*new_midisynth)(BAEOutputMixer *mixer) = NULL;
static BAEMidiSong *(*new_midisong)(BAEOutputMixer *mixer) = NULL;

static bool libbae_loaded = false;

static void
load_libbae(void)
{
	PRINT(("load_libbae()\n"));
	if(libbae_loaded) return;
	
	image_id id = load_add_on("/system/lib/libbae.so");
	PRINT(("load_libbae() %d\n",id));
	if(id > 0){
		if(get_image_symbol(id,"instantiate_outputmixer",B_SYMBOL_TYPE_TEXT,(void**) &new_outputmixer)) goto fail;
		if(get_image_symbol(id,"instantiate_midisynth",B_SYMBOL_TYPE_TEXT,(void**) &new_midisynth)) goto fail;
		if(get_image_symbol(id,"instantiate_midisong",B_SYMBOL_TYPE_TEXT,(void**) &new_midisong)) goto fail;
		libbae_loaded = true;
		PRINT(("load_libbae() done\n"));
		return;
fail:
		new_outputmixer = NULL;
		new_midisynth = NULL;
		new_midisong = NULL;
	}
}

static BAEOutputMixer *
NewOutputMixer(void)
{
	if(!libbae_loaded) load_libbae();
	if(new_outputmixer) {
		return new_outputmixer();
	} else {
		return NULL;
	}
}

static BAEMidiSynth *
NewMidiSynth(BAEOutputMixer *mixer)
{
	if(!libbae_loaded) load_libbae();
	if(new_midisynth) {
		return new_midisynth(mixer);
	} else {
		return NULL;
	}
}

static BAEMidiSong *
NewMidiSong(BAEOutputMixer *mixer)
{
	if(!libbae_loaded) load_libbae();
	if(new_midisong) {
		return new_midisong(mixer);
	} else {
		return NULL;
	}
}
#else
static BAEOutputMixer *
NewOutputMixer(void)
{
	return new BAEOutputMixer;
}

static BAEMidiSynth *
NewMidiSynth(BAEOutputMixer *mixer)
{
	return new BAEMidiSynth(mixer);
}

static BAEMidiSong *
NewMidiSong(BAEOutputMixer *mixer)
{
	return new BAEMidiSong(mixer);
}
#endif

/* ------------------------------------------------------------------- */

BSynth::BSynth()
{
	_init();
}


BSynth::BSynth(synth_mode synth)
{
	_init();
	LoadSynthData(synth);
}


BSynth::~BSynth()
{
}

void BSynth::_init()
{
	PRINT(("BSynth::_init()\n"));
	be_synth = this;
	fSRate = Q_44K;
	fInterp = B_LINEAR_INTERPOLATION;
	fModifiers = M_USE_STEREO | M_USE_16;
	fMaxSynthVox = 56;
	fMaxSampleVox = 4;
	fMode = B_NO_SYNTH;
	fReverb = B_REVERB_BALLROOM;
	fModifiers =  translate_modifiers (2, 2, true);
	fLimiter = 8;
	fClientCount = 0;
	fSetupLock = create_sem(1,"Synth Lock");	
	bae_mixer = NewOutputMixer();

	PRINT(("BSynth::_init() bae_mixer = %x\n",bae_mixer));
	if(bae_mixer){
		bae_mixer->Open(NULL,BAE_44K,
			(BAETerpMode)fInterp,
			(BAEReverbType)fReverb,
			fModifiers,
			fMaxSynthVox,fMaxSampleVox,fLimiter);
	}
}

status_t 
BSynth::LoadSynthData(entry_ref *instrumentsFile)
{
	BAEResult err=BAE_ERROR_COUNT;
	acquire_sem(fSetupLock);
	if(bae_mixer)
	{
		BEntry entry(instrumentsFile);
		if(entry.InitCheck()==B_OK)
		{
			err=BAE_BAD_INSTRUMENT;
			if(entry.Exists())
			{
				BPath path;
				entry.GetPath(&path);
				if(path.InitCheck()==B_OK)
					err=bae_mixer->ChangeAudioFile((BAEPathName)path.Path());
			}
		}
	}
	release_sem(fSetupLock);
	return translate_error(err);
}

status_t 
BSynth::LoadSynthData(synth_mode synth)
{
	status_t err;
	entry_ref ref;
	BPath path;

	if (synth == fMode)
		return B_OK;

	switch (synth) {
	case B_SAMPLES_ONLY:
		break;

	case B_BIG_SYNTH:
	case B_LITTLE_SYNTH:
	case B_TINY_SYNTH:
		err = find_directory (B_SYNTH_DIRECTORY, &path);
		if (err != B_OK)
			return err;
		path.Append(synth == B_BIG_SYNTH ? B_BIG_SYNTH_FILE :
					(synth == B_TINY_SYNTH ? B_TINY_SYNTH_FILE :
					 B_LITTLE_SYNTH_FILE));
		err = get_ref_for_path(path.Path(), &ref);
		if (err != B_OK)
			return err;
		break;

	default:
		return B_BAD_VALUE;
	}
	err=LoadSynthData(&ref);
	if(err == B_OK)
		fMode = synth;
	return err;
}

synth_mode 
BSynth::SynthMode(void)
{
	return fMode;
}

void 
BSynth::Unload(void)
{
	// OBSOLETE
	// It is not possible to unload the synth data in the current Beatnik engine.
}

bool 
BSynth::IsLoaded(void) const
{
	return fMode != B_NO_SYNTH;
}

status_t 
BSynth::SetSamplingRate(int32 sample_rate)
{
	BAEResult theErr;
	BAEQuality q = sample_rate_to_quality (sample_rate);
	if (bae_mixer)
		theErr = bae_mixer->ChangeAudioModes (q, (BAETerpMode)fInterp, fModifiers);
	if(theErr==BAE_NO_ERROR)
		fSRate= q;
	return translate_error(theErr);
}

int32 
BSynth::SamplingRate() const
{
	return quality_to_sample_rate((BAEQuality)fSRate);
}

status_t 
BSynth::SetInterpolation(interpolation_mode interp_mode)
{
	BAEResult theErr;
	if (bae_mixer)
		theErr = bae_mixer->ChangeAudioModes ((BAEQuality)fSRate, (BAETerpMode)interp_mode, fModifiers);
	if(theErr==BAE_NO_ERROR)
		fInterp = interp_mode;
	return translate_error(theErr);
}

interpolation_mode 
BSynth::Interpolation() const
{
	return (interpolation_mode)bae_mixer->GetTerpMode();
}

void 
BSynth::SetReverb(reverb_mode rev_mode)
{
	bae_mixer->SetReverbType((BAEReverbType)rev_mode);
}

reverb_mode 
BSynth::Reverb() const
{
	return (reverb_mode)bae_mixer->GetReverbType();
}

status_t 
BSynth::EnableReverb(bool reverb_enabled)
{
	BAEResult theErr;
	long mods = fModifiers & ~BAE_DISABLE_REVERB;
	if (!reverb_enabled)
		mods |= BAE_DISABLE_REVERB;

	if(bae_mixer)
		theErr = bae_mixer->ChangeAudioModes ((BAEQuality)fSRate, (BAETerpMode)fInterp, mods);
	if (theErr == BAE_NO_ERROR)
		fModifiers = mods;
	return translate_error(theErr);
}

bool 
BSynth::IsReverbEnabled() const
{
    return (fModifiers & BAE_DISABLE_REVERB ? false : true);
}

status_t 
BSynth::SetVoiceLimits(int16 maxSynthVoices, int16 maxSampleVoices, int16 limiterThreshhold)
{
	BAEResult theErr;
	if(bae_mixer)
		theErr = bae_mixer->ChangeSystemVoices(maxSynthVoices, limiterThreshhold, maxSampleVoices);
	
	if (theErr == BAE_NO_ERROR)
	{
		fMaxSynthVox = maxSynthVoices;
		fMaxSampleVox = maxSampleVoices;
		fLimiter = limiterThreshhold;
	}
	return translate_error(theErr);
}

int16 
BSynth::MaxSynthVoices(void) const
{
	return fMaxSynthVox;
}

int16 
BSynth::MaxSampleVoices(void) const
{
	return fMaxSampleVox;
}

int16 
BSynth::LimiterThreshhold(void) const
{
	return fLimiter;
}

void 
BSynth::SetSynthVolume(double theVolume)
{
	bae_mixer->SetMasterVolume(FLOAT_TO_FIXED(theVolume));
}

double 
BSynth::SynthVolume(void) const
{
	return FIXED_TO_FLOAT(bae_mixer->GetMasterVolume());
}

void 
BSynth::SetSampleVolume(double theVolume)
{
	bae_mixer->SetMasterSoundEffectsVolume(FLOAT_TO_FIXED(theVolume));
}

double 
BSynth::SampleVolume(void) const
{
	return FIXED_TO_FLOAT(bae_mixer->GetMasterSoundEffectsVolume());
}

status_t 
BSynth::GetAudio(int16 *pLeft, int16 *pRight, int32 max_samples) const
{
	if(bae_mixer){
		return bae_mixer->GetAudioSampleFrame(pLeft,pRight);
	} else {
		return 0;
	}
}

void 
BSynth::Pause(void)
{
	bae_mixer->DisengageAudio();
}

void 
BSynth::Resume(void)
{
	bae_mixer->ReengageAudio();
}

void 
BSynth::SetControllerHook(int16 controller, synth_controller_hook cback)
{
	// OBSOLETE
	// This hook is no longer present in the current Beatnik engine
	// it moved into BAEMidiSong, which bears no relationship whatsoever
	// with this class.
}

int32 
BSynth::CountClients(void) const
{
	return fClientCount;
}

void BSynth::_ReservedSynth1() {}
void BSynth::_ReservedSynth2() {}
void BSynth::_ReservedSynth3() {}
void BSynth::_ReservedSynth4() {}

/* ------------------------------------------------------------------- */


BMidiSynth::BMidiSynth()
{
	if(!be_synth) new BSynth();

	synth_mode smode;
	smode = be_synth->SynthMode();
	if (smode == B_NO_SYNTH || smode == B_SAMPLES_ONLY)
	{
		be_synth->LoadSynthData(B_BIG_SYNTH);
		
		if (be_synth->SynthMode() != B_BIG_SYNTH)
		{
			be_synth->LoadSynthData(B_LITTLE_SYNTH);
			
			if (be_synth->SynthMode() != B_LITTLE_SYNTH)
			{
				be_synth->LoadSynthData(B_TINY_SYNTH);
				
				if (be_synth->SynthMode() != B_TINY_SYNTH)
				{
					delete be_synth;
					be_synth = NULL;
				}
			}
		}
	}
	
	atomic_add(&be_synth->fClientCount,1);
	fSynth = NewMidiSynth(bae_mixer);
	PRINT(("BMidiSynth::BMidiSynth() - %x\n",fSynth));	
	if(fSynth){
		if(fSynth->Open(true)) PRINT(("oops\n")); //XXX hack
		fSynth->SetQueue(false);
	}
}


BMidiSynth::~BMidiSynth()
{
	delete fSynth;
	atomic_add(&be_synth->fClientCount,-1);
}

status_t 
BMidiSynth::EnableInput(bool enable, bool loadInstruments)
{
	// FIXME
	PRINT(("BMidiSynth::EnableInput(%s,%s)\n",enable?"true":"false",loadInstruments?"true":"false"));
	fInputEnabled=enable;
	return B_OK;
}

bool 
BMidiSynth::IsInputEnabled(void) const
{
	return fInputEnabled;
}

void 
BMidiSynth::SetVolume(double volume)
{
	PRINT(("BMidiSynth::SetVolume\n"));
	if(fSynth)
		fSynth->SetVolume(FLOAT_TO_FIXED(volume));
}

double 
BMidiSynth::Volume(void) const
{
	if(fSynth){
		return fSynth->GetVolume();
	} else {
		return 0;
	}
}

void 
BMidiSynth::SetTransposition(int16 offset)
{
	if(fSynth)
		fSynth->SetPitchOffset(long(offset));
}

int16 
BMidiSynth::Transposition(void) const
{
	if(fSynth)
		return fSynth->GetPitchOffset();
	return 0;
}

void 
BMidiSynth::MuteChannel(int16 channel, bool do_mute)
{
	if(fSynth){
		if(do_mute){
			fSynth->MuteChannel(channel);
		} else {
			fSynth->UnmuteChannel(channel);
		}
	}		
}

void 
BMidiSynth::GetMuteMap(char *pChannels) const
{
	if(fSynth) fSynth->GetChannelMuteStatus(pChannels);
}

void 
BMidiSynth::SoloChannel(int16 channel, bool do_solo)
{
	if(fSynth){
		if(do_solo){
			fSynth->SoloChannel(channel);
		} else {
			fSynth->UnSoloChannel(channel);
		}
	}		
}

void 
BMidiSynth::GetSoloMap(char *pChannels) const
{
	if(fSynth) fSynth->GetChannelSoloStatus(pChannels);
}

status_t 
BMidiSynth::LoadInstrument(int16 instrument)
{
	if(fSynth){
		return fSynth->LoadInstrument(instrument);
	} else {
		return B_ERROR;
	}
}

status_t 
BMidiSynth::UnloadInstrument(int16 instrument)
{
	if(fSynth){
		return fSynth->UnloadInstrument(instrument);
	} else {
		return B_ERROR;
	}
}

status_t 
BMidiSynth::RemapInstrument(int16 from, int16 to)
{
	if(fSynth) {
		return fSynth->RemapInstrument(from,to);
	} else {
		return B_ERROR;
	}
}

void 
BMidiSynth::FlushInstrumentCache(bool startStopCache)
{
	// OBSOLETE
}

uint32 
BMidiSynth::Tick(void) const
{
	if(fSynth){
		return fSynth->GetTick();
	} else {
		return 0;
	}
}

void 
BMidiSynth::NoteOff(uchar channel, uchar note, uchar velocity, uint32 time)
{
	if(fSynth) fSynth->NoteOff(channel-1,note,velocity,time);
}

void 
BMidiSynth::NoteOn(uchar channel, uchar note, uchar velocity, uint32 time)
{
	if(fSynth) fSynth->NoteOn(channel-1,note,velocity,time);
}

void 
BMidiSynth::KeyPressure(uchar channel, uchar note, uchar pressure, uint32 time)
{
	if(fSynth) fSynth->KeyPressure(channel-1,note,pressure,time);
}

void 
BMidiSynth::ControlChange(uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	if(fSynth) fSynth->ControlChange(channel-1,controlNumber,controlValue,time);
}

void 
BMidiSynth::ProgramChange(uchar channel, uchar programNumber, uint32 time)
{
	if(fSynth) fSynth->ProgramChange(channel-1,programNumber,time);
}

void 
BMidiSynth::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	if(fSynth) fSynth->ChannelPressure(channel-1,pressure,time);
}

void 
BMidiSynth::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	if(fSynth) fSynth->PitchBend(channel-1,lsb,msb,time);
}

void 
BMidiSynth::AllNotesOff(bool controlOnly, uint32 time)
{
	if(fSynth) fSynth->AllNotesOff(time);
}

void BMidiSynth::_ReservedMidiSynth1() {}
void BMidiSynth::_ReservedMidiSynth2() {}
void BMidiSynth::_ReservedMidiSynth3() {}
void BMidiSynth::_ReservedMidiSynth4() {}

void 
BMidiSynth::Run()
{
	// no-op
}

/* ------------------------------------------------------------------- */

#define RMF_SUPPORT	0

static bool
is_midi_file(const entry_ref *ref)
{
	BFile file(ref, O_RDONLY);
	int32 magic;
	struct stat st;
	status_t err;

	err = file.Read(&magic, sizeof(magic));
	if (err < 0)
	  return false;
 	magic = B_BENDIAN_TO_HOST_INT32(magic);

	if (magic == 'MThd') {
//	  *type = SONG_TYPE_SMS;
#if RMF_SUPPORT
	} else if (magic == 'IREZ') {
//	  *type = SONG_TYPE_RMF;
#endif
	} else {
	  return false;
	}

	if (file.GetStat(&st) < 0)
	  return false;

	return true;
}

BAEMidiSong *MidiSong(BAEMidiSynth *synth)
{
	BAEMidiSong *song=dynamic_cast<BAEMidiSong*>(synth);
	if(!song)
		debugger("called BMidiSynthFile::LoadFile on something that is not a BMidiSynthFile");
	return  song;
}


BMidiSynthFile::BMidiSynthFile()
{
	PRINT(("BMidiSynthFile::BMidiSynthFile\n"));
	// FIXME
	// delete the baseclass' BAEMidiSynth object
	// should probably add a new private constructor to BMidiSynth
	// that doesn't create this in the first place
	delete fSynth;
	fSynth=NewMidiSong(bae_mixer);
}


BMidiSynthFile::~BMidiSynthFile()
{
	PRINT(("BMidiSynthFile::~BMidiSynthFile\n"));
	// baseclass will delete the BAEMidiSong object
}

status_t 
BMidiSynthFile::LoadFile(const entry_ref *midi_entry_ref)
{
	PRINT(("BMidiSynthFile::LoadFile\n"));
	
	// LoadFromFile takes a long time to realize it doesn't have
	// have a valid MIDI file, so do a quick check first.
	if(is_midi_file(midi_entry_ref))
	{
		BEntry entry(midi_entry_ref);
		if(entry.InitCheck()==B_OK)
		{
			BPath path;
			if(entry.GetPath(&path)==B_OK)
			{
				if(path.InitCheck()==B_OK)
				{
					PRINT(("Loading %s\n",path.Path()));
					if(BAE_NO_ERROR==MidiSong(fSynth)->LoadFromFile((BAEPathName)path.Path()))
						return B_OK;
				}
			}
		}
	}
	return B_ERROR;
}

void 
BMidiSynthFile::UnloadFile(void)
{
	MidiSong(fSynth)->Unload();
}

status_t 
BMidiSynthFile::Start(void)
{
	PRINT(("BMidiSynthFile::Start\n"));
	if(BAE_NO_ERROR==MidiSong(fSynth)->Start())
		return B_OK;
	return B_ERROR;
}

void 
BMidiSynthFile::Stop(void)
{
	MidiSong(fSynth)->Stop(false);
}

void 
BMidiSynthFile::Fade(void)
{
	MidiSong(fSynth)->Stop(true);
}

void 
BMidiSynthFile::Pause(void)
{
	MidiSong(fSynth)->Pause();
}

void 
BMidiSynthFile::Resume(void)
{
	MidiSong(fSynth)->Resume();
}

int32 
BMidiSynthFile::Duration(void) const
{
	return MidiSong(fSynth)->GetTickLength();
}

int32 
BMidiSynthFile::Position(int32 ticks) const
{
	if(BAE_NO_ERROR==MidiSong(fSynth)->SetTickPosition(ticks))
		return B_OK;
	return B_ERROR;
}

int32 
BMidiSynthFile::Seek()
{
	return MidiSong(fSynth)->GetTickPosition();
}

status_t 
BMidiSynthFile::GetPatches(int16 *pArray768, int16 *pReturnedCount) const
{
	// documented as "not working, don't use"
	return B_ERROR;
}


void 
BMidiSynthFile::SetFileHook(synth_file_hook pSongHook, int32 arg)
{
	MidiSong(fSynth)->SetDoneCallback((BAEDoneCallbackPtr)pSongHook,(void*)arg);
}

bool 
BMidiSynthFile::IsFinished(void) const
{
	return MidiSong(fSynth)->IsDone();
}

void 
BMidiSynthFile::ScaleTempoBy(double tempoFactor)
{
	MidiSong(fSynth)->SetMasterTempo(FLOAT_TO_FIXED(tempoFactor));
}

void 
BMidiSynthFile::SetTempo(int32 newTempoBPM)
{
	MidiSong(fSynth)->SetTempoInBeatsPerMinute(newTempoBPM);
}

int32 
BMidiSynthFile::Tempo(void) const
{
	return MidiSong(fSynth)->GetTempoInBeatsPerMinute();
}

void 
BMidiSynthFile::EnableLooping(bool loop)
{
	MidiSong(fSynth)->SetLoopFlag(loop);
}

void 
BMidiSynthFile::MuteTrack(int16 track, bool do_mute)
{
	// documented as "not working, don't use"
	// maybe use: BMidiSynth::MuteChannel(track,do_mute); ???
}

void 
BMidiSynthFile::GetMuteMap(char *pTracks) const
{
	// documented as "not working, don't use"
	// maybe use: BMidiSynth::GetMuteMap(pTracks); ???
}

void 
BMidiSynthFile::SoloTrack(int16 track, bool do_solo)
{
	// documented as "not working, don't use"
	// maybe use: BMidiSynth::SoloChannel(track, do_solo); ???
}

void
BMidiSynthFile::GetSoloMap(char *pTracks) const 
{
	// documented as "not working, don't use"
}

void BMidiSynthFile::_ReservedMidiSynthFile1() {}
void BMidiSynthFile::_ReservedMidiSynthFile2() {}
void BMidiSynthFile::_ReservedMidiSynthFile3() {}


/* ------------------------------------------------------------------- */


/* ------------------------------------------------------------------- */
