#include <File.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <Entry.h>
#include <Locker.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <MediaTrack.h>
#include <Extractor.h>

#define X_PLATFORM      X_BE
#include "MidiDetractor.h"
#include "BAE.h"
#include "BAE_API.h"
#include "GenSnd.h"


//#define DEBUG printf
#define DEBUG if (0) printf
#define SEEK(x) //printf x
#define SEQ(x) //printf x

const int32 BLOCKSIZEINFRAMES = 1024;

extern "C" const char * mime_type_detractor = "audio/x-midi";

extern "C" Detractor* instantiate_detractor()
{
	return new MidiDetractor;
}


MidiDetractor::MidiDetractor()
{
	fCurrentFrame = -1;
	fTempFrames = 0;
	fStatus = B_ERROR;

	bae_image = load_add_on("/system/lib/libbae.so");
	if(bae_image < 0)
	{
		return;
	}
	BAEOutputMixer *(*new_outputmixer)(void);
	BAEMidiSong *(*new_midisong)(BAEOutputMixer *mixer);
	int (*releaseaudio)(void *context);
	BAEResult (*outputmixeropen)(BAEOutputMixer *self, BAEPathName pAudioPathName = NULL,
									BAEQuality q = BAE_22K,
									BAETerpMode t = BAE_LINEAR_INTERPOLATION,
									BAEReverbType r = BAE_REVERB_TYPE_4,
									BAEAudioModifiers am = (BAE_USE_16 | BAE_USE_STEREO),
									short int maxMidiVoices = 56,
									short int maxSoundVoices = 4,
									short int mixLevel = 8,
									BAE_BOOL engageAudio = TRUE);

	if(get_image_symbol(bae_image,"instantiate_outputmixer",B_SYMBOL_TYPE_TEXT,(void**) &new_outputmixer)) return;
	if(get_image_symbol(bae_image,"instantiate_midisong",B_SYMBOL_TYPE_TEXT,(void**) &new_midisong)) return;
	if(get_image_symbol(bae_image,"BAE_ReleaseAudioCard",B_SYMBOL_TYPE_TEXT,(void**) &releaseaudio)) return;
	if(get_image_symbol(bae_image,"Open__14BAEOutputMixerPv10BAEQuality11BAETerpMode13BAEReverbTypelsssc",B_SYMBOL_TYPE_TEXT,(void**) &outputmixeropen)) return;
	if(get_image_symbol(bae_image,"GetMicrosecondPosition__11BAEMidiSong",B_SYMBOL_TYPE_TEXT,(void**) &getmicrosecondposition)) return;
	if(get_image_symbol(bae_image,"SetMicrosecondPosition__11BAEMidiSongUl",B_SYMBOL_TYPE_TEXT,(void**) &setmicrosecondposition)) return;
	if(get_image_symbol(bae_image,"BAE_GetMaxSamplePerSlice",B_SYMBOL_TYPE_TEXT,(void**) &getmaxsampleperslice)) return;
	if(get_image_symbol(bae_image,"BAE_BuildMixerSlice",B_SYMBOL_TYPE_TEXT,(void**) &buildmixerslice)) return;
	if(get_image_symbol(bae_image,"GetMicrosecondLength__11BAEMidiSong",B_SYMBOL_TYPE_TEXT,(void**) &getmicrosecondlength)) return;

	bae_mixer = new_outputmixer();
	
	BEntry synth("/etc/synth/Patches300.hsb");
	if (synth.Exists())
	{	
		outputmixeropen(bae_mixer,(BAEPathName)"/etc/synth/Patches300.hsb",BAE_44K,
			(BAETerpMode)BAE_LINEAR_INTERPOLATION,
			(BAEReverbType)BAE_REVERB_TYPE_4,
			M_USE_STEREO | M_USE_16,
			56,4,8);
	}
	else
	{
		outputmixeropen(bae_mixer,(BAEPathName)"/etc/synth/Patches.hsb",BAE_44K,
			(BAETerpMode)BAE_LINEAR_INTERPOLATION,
			(BAEReverbType)BAE_REVERB_TYPE_4,
			M_USE_STEREO | M_USE_16,
			56,4,8);
	}

	releaseaudio(NULL);
	bae_song = new_midisong(bae_mixer);
	fStatus = B_OK;
}

MidiDetractor::~MidiDetractor()
{
	delete bae_song;
	delete bae_mixer;
	if(bae_image >=0)
		unload_add_on(bae_image);
}

status_t MidiDetractor::InitCheck() const
{
//	printf("MidiDetractor::InitCheck\n");
	return fStatus;
}

status_t MidiDetractor::SetTo(const entry_ref *ref)
{
	if(fStatus) return fStatus;
//	printf("MidiDetractor::SetTo(entry_ref)\n");
	return SetTo(new BFile(ref,B_READ_ONLY));
}

status_t MidiDetractor::SetTo(BDataIO *source)
{
//	printf("MidiDetractor::SetTo(BDataIO*)\n");
	if(fStatus) return fStatus;
	fStatus = B_ERROR;
	fCurrentFrame = -1;

	// copy the MIDI file into memory
	fMallocIO.SetSize(0);
	char buf[1000];
	int32 numread;
	BPositionIO *posIO=dynamic_cast<BPositionIO*>(source);

													 
	if(posIO)
		posIO->Seek(0,SEEK_SET);
	while((numread=source->Read(buf,sizeof(buf)))>0)
	{
		if(fMallocIO.BufferLength()==0)
		{
    		if( buf[0] != 'M' ||
	            buf[1] != 'T' ||
		        buf[2] != 'h' ||
		        buf[3] != 'd')
			{
				return B_ERROR;
			}
		}
		fMallocIO.Write(buf,numread);
	}
	
	if(fMallocIO.BufferLength() && BAE_NO_ERROR==bae_song->LoadFromMemory(fMallocIO.Buffer(),fMallocIO.BufferLength(),FALSE))
	{
		BAEResult (*songstart)(BAEMidiSong *self, BAE_BOOL useEmbeddedMixerSettings = TRUE, BAE_BOOL autoLevel = FALSE);
		if(get_image_symbol(bae_image,"Start__11BAEMidiSongcc",
			B_SYMBOL_TYPE_TEXT,(void**) &songstart) == B_OK)
		{
			songstart(bae_song);
			fCurrentFrame = 0;
			fStatus = B_OK;
			return fStatus;
		}
	}
//	printf("Not a MIDI file, buffer is %d bytes\n",fMallocIO.BufferLength());
	fMallocIO.SetSize(0);
	return B_ERROR;
}

status_t MidiDetractor::GetFileFormatInfo(media_file_format *mfi) const
{
//	printf("MidiDetractor::GetFileFormatInfo\n");
	if(fStatus) return fStatus;
    strcpy(mfi->mime_type,      "audio/x-midi");
    strcpy(mfi->pretty_name,    "MIDI File");
    strcpy(mfi->short_name,     "MIDI");
    strcpy(mfi->file_extension, "mid");

    mfi->family = B_ANY_FORMAT_FAMILY;

    mfi->capabilities = media_file_format::B_READABLE              |
                        media_file_format::B_IMPERFECTLY_SEEKABLE  |
                        media_file_format::B_PERFECTLY_SEEKABLE    |
                        media_file_format::B_KNOWS_RAW_AUDIO       |
                        media_file_format::B_KNOWS_ENCODED_AUDIO;

	return B_OK;
}

const char* MidiDetractor::Copyright(void) const
{
//	printf("MidiDetractor::Copyright\n");
	if(fStatus) return NULL;
	return "Copyright 2000 Be, Inc.";
}

int32 MidiDetractor::CountTracks() const
{
//	printf("MidiDetractor::CountTracks\n");
	if(fStatus) return -1;
	return 1;
}

status_t MidiDetractor::GetCodecInfo(int32 tracknum, media_codec_info *mci) const
{
//	printf("MidiDetractor::GetCodecInfo\n");
	if(fStatus) return fStatus;
	if(tracknum!=0) return B_BAD_INDEX;
	strcpy(mci->pretty_name, "MIDI File");
	strcpy(mci->short_name, "MIDI");
	return B_OK;
}

status_t MidiDetractor::EncodedFormat(int32 tracknum, media_format *out_format) const
{
//	printf("MidiDetractor::EncodedFormat\n");
	if(fStatus) return fStatus;
	if(tracknum!=0) return B_BAD_INDEX;
	out_format->type=B_MEDIA_ENCODED_AUDIO;
    out_format->u.encoded_audio.output.frame_rate = 44100;
	out_format->u.encoded_audio.output.channel_count = 2;
	out_format->u.encoded_audio.encoding = (media_encoded_audio_format::audio_encoding)0;
    out_format->u.encoded_audio.bit_rate = 0;
    out_format->u.encoded_audio.output.buffer_size = BLOCKSIZEINFRAMES*4;
	return B_OK;
}

status_t MidiDetractor::DecodedFormat(int32 tracknum, media_format *inout_format)
{
//	printf("MidiDetractor::DecodedFormat\n");
	if(fStatus) return fStatus;
	if(tracknum!=0) return B_BAD_INDEX;
	inout_format->type=B_MEDIA_RAW_AUDIO;
	inout_format->u.raw_audio.frame_rate = 44100;
	inout_format->u.raw_audio.channel_count = 2;
	inout_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	inout_format->u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
	inout_format->u.raw_audio.buffer_size = BLOCKSIZEINFRAMES*4;
	return B_OK;
}

int64    MidiDetractor::CountFrames(int32 tracknum) const
{
	if(fStatus) return -1;
	if(tracknum!=0) return -1;
//	printf("MidiDetractor::CountFrames\n");
	int64 length = int64(getmicrosecondlength(bae_song))*44100/1000000;
//	printf("length: %Ld\n",length);
	return length;
}

bigtime_t MidiDetractor::Duration(int32 tracknum) const
{
	if(fStatus) return -1;
	if(tracknum!=0) return -1;
//	printf("MidiDetractor::Duration\n");
	return getmicrosecondlength(bae_song);
}

int64    MidiDetractor::CurrentFrame(int32 tracknum) const
{
//	printf("MidiDetractor::CurrentFrame\n");
	if(fStatus) return -1;
	if(tracknum!=0) return -1;
	return fCurrentFrame;
}

bigtime_t MidiDetractor::CurrentTime(int32 tracknum) const
{
//	printf("MidiDetractor::CurrentTime\n");
	if(fStatus) return -1;
	if(tracknum!=0) return -1;
	return fCurrentFrame*1000000/44100;
}

status_t MidiDetractor::ReadFrames(int32 tracknum, void *out_buffer, int64 *out_frameCount, media_header *mh = NULL)
{
//	printf("MidiDetractor::ReadFrames\n");
	if(fStatus) return fStatus;
	if(tracknum!=0) return B_BAD_INDEX;
	const int32 numframes = BLOCKSIZEINFRAMES;

	*out_frameCount = numframes;
	fCurrentFrame+=numframes;
	size_t size=numframes*4;

	int frs = size/4;
	int msps = getmaxsampleperslice();
	int bpf = 4;
	if (frs < msps) {
		//	we need to re-buffer from the synth
		if (fTempFrames <= 0) {
			buildmixerslice(NULL, fTempData, msps*bpf, msps);
			fTempFrames += msps;
		}
		memcpy(out_buffer, &fTempData[(msps-fTempFrames)*bpf], bpf*frs);
		fTempFrames -= frs;
	}
	else while (frs > 0) {
		//	we generate more than one slice at a time
		buildmixerslice(NULL, out_buffer, size, msps);
		int d = msps*bpf;
		out_buffer = ((char *)out_buffer)+d;
		size -= d;
		frs -= msps;
	}

	if(mh)
		mh->start_time = bigtime_t(fCurrentFrame * 1000000.0 / 44100);

	return B_OK;
}
							   
status_t MidiDetractor::SeekTo(int32 tracknum, int32 to_what, bigtime_t *inout_time, int64 *inout_frame, int32 /*flags*/)
{
//	printf("MidiDetractor::SeekToFrame\n");
	if(fStatus) return fStatus;
	if(tracknum!=0) return B_BAD_INDEX;

	bigtime_t whereto;
	if(to_what == B_SEEK_BY_FRAME)
		whereto = bigtime_t(*inout_frame * 1000000 / 44100);
	else
		whereto = *inout_time;

	setmicrosecondposition(bae_song,whereto);
	bigtime_t now = getmicrosecondposition(bae_song);
	fCurrentFrame = int64(now * 44100 / 1000000);
	*inout_frame = fCurrentFrame;
	*inout_time = now;

	return B_OK;
}

status_t MidiDetractor::FindKeyFrameForFrame(int32 tracknum, int64 */*inout_frame*/, int32 /*flags*/) const
{
	if(fStatus) return fStatus;
	if(tracknum!=0) return B_BAD_INDEX;

	printf("MidiDetractor::FindKeyFrameForFrame\n");
	return B_OK;
}

