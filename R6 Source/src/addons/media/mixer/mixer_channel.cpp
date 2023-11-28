#include "mixer.h"
#include "mixer_channel.h"
#include <Buffer.h>
#include <FindDirectory.h>
#include <Locker.h>
#include <math.h>
#include <ParameterWeb.h>
#include <Path.h>
#include <resampler.h>
#include <resampler_zero.h>
#include <resampler_linear.h>
#include <resampler_trilinear.h>
#include <resampler_cubic.h>
#include <resampler_sinc.h>
#include <resampler_sinc_lowpass.h>
#include <rt_alloc.h>
#include <stdio.h>
#include <time.h>

#if B_HOST_IS_LENDIAN
#define ENDIAN B_MEDIA_LITTLE_ENDIAN
#else
#define ENDIAN B_MEDIA_BIG_ENDIAN
#endif

#if !NDEBUG
#define PRINTF printf
#else
#define PRINTF
#endif

#define SETTINGS(string)	//PRINTF##string
#define DBUG(string)		//PRINTF##string
#define PARAMS(string)	 	//PRINTF##string
#define FORMATS(string)	 	//PRINTF##string

#define pi_over_four M_PI/4

//parameters
#define CH_PAN 1
#define CH_SOLO 2
#define CH_MUTE 3
#define CH_GAIN 4
#define CH_NAME 100
#define CH_INFO 300

#define GAIN_BOTTOM -60
#define GAIN_TOP 18

rtm_pool *_channel_heap = 0;

class RTHeap {
public:

	RTHeap() {
		rtm_create_pool(&_channel_heap, 200 * 1024);
	}
	
	~RTHeap() {
		rtm_delete_pool(_channel_heap);
	}
} heapDeleter;

mixchannel::mixchannel(uint32 id, port_id port,BMixer *mixer_parent = NULL)
{
	mFormat.type = B_MEDIA_RAW_AUDIO;
	mFormat.u.raw_audio.frame_rate = media_raw_audio_format::wildcard.frame_rate;
	mFormat.u.raw_audio.channel_count = media_raw_audio_format::wildcard.channel_count;
	mFormat.u.raw_audio.format = media_raw_audio_format::wildcard.format;
	mFormat.u.raw_audio.byte_order = ENDIAN;
	mFormat.u.raw_audio.buffer_size = 0;
	/* set some defaults */
	mFormat.u.raw_audio.channel_count = 2;
	mFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	mFormat.u.raw_audio.frame_rate = 96000;

	mID = id;
	mParameterMask = mID << 16;
	DBUG(("channel ID: %x, param_id: %x\n", mID, mParameterMask));

	mInputSource = media_source::null;
	mInputDestination.port = port;
	mInputDestination.id = mID;
	mGain = 1.0; /* 0 is off, 1.0 is full, higher is possible, but may clip! */
	for(int i=0; i<MAX_CHANNELS; i++){
		mMultiGain[i] = 1.0;
		mMultiPan[i] = 0.707;
		mLevel[i] = 0.0;
		mPeak[i] = 0.0;
	}	

	mLastMixTime = 0LL;
	mPrepIn = 0;
	mBuffIn = 0;
	mChain  = FALSE;

	mMixerParent = mixer_parent;	

	mMute = false; /*true == muted */
	mSolo = false; /*true == solo'd*/
	/* pan is panning for mono, balance for stereo */
	mPan = 0.0; /*1.0 is full left, -1.0 is full right, 0.0 is center */

	mSynchronous = true;
	mFrameCount = 0;	//number of frames played
	mFrameCountOffset = 0;	//difference between this and mixer output
	mStatus = 0;			//ProducerDataStatus
	mNewStatus = 0;		//Pre-change ProducerDataStatus
	mNextStatusTime = 0;	//time to change ProducerDataStatus
	
	mResampler = NULL;

	sprintf(mName, "Free");
	
	mNext = NULL;
}

mixchannel::~mixchannel()
{
}

uint32
mixchannel::ID()
{
	return mID;
}

void
mixchannel::SetName(const char *name)
{
	strcpy(mName, name);
	mMixerParent->SaveLock()->Lock();
	save_info info;
	strncpy(info.name, name, 64);
	info.name[63] = 0;
	SETTINGS(("looking for settings for %s\n", info.name));
	save_map::iterator ptr(mMixerParent->SaveMap()->find(info));
	if (ptr != mMixerParent->SaveMap()->end()) {
		SETTINGS(("found!!!!!!\n"));
		info = *ptr;
		mMultiGain[0] = info.gain[0];
		mMultiGain[1] = info.gain[1];
		SetPan(info.pan);	//	will automatically save
	}
	mMixerParent->SaveLock()->Unlock();
}

void
mixchannel::SaveLevels()
{
	save_info info;
	strncpy(info.name, mName, 64);
	info.name[63] = 0;
	info.gain[0] = mMultiGain[0];
	info.gain[1] = mMultiGain[1];
	info.pan = mPan;
	info.time = mMixerParent->SaveTimestamp();
	if (write_port_etc(	mMixerParent->SavePortID(), 
						BMixer::SAVE_LEVELS, 
						&info, 
						sizeof(info), 
						B_TIMEOUT, 
						10000LL)) 
	{
		SETTINGS(("MixerVolumes: SaveLevels(%s) failed\n", info.name));
	}
}

const char *
mixchannel::Name()
{
	return mName;
}

void
mixchannel::SetGain(int channel, float gain)
{
	mGain = gain;
	if(channel < MAX_CHANNELS && channel >= 0){
		mMultiGain[channel] = gain;
	}
	SaveLevels();
}

float
mixchannel::Gain()
{
	return mGain;
}

float
mixchannel::Gain(int channel)
{
	return mMultiGain[channel];
}

void
mixchannel::SetLevel(int count, const float *levels)
{
	if(((uint32) count) > mFormat.u.raw_audio.channel_count){
		count = mFormat.u.raw_audio.channel_count;
	}
	for(int i=0; i<count; i++){
		mLevel[i] = levels[i];
	}	
}

void
mixchannel::Level(int count, float *levels)
{
	if(((uint32) count) > mFormat.u.raw_audio.channel_count){
		count = mFormat.u.raw_audio.channel_count;
	}
	for(int i=0; i<count; i++){
		levels[i] = mLevel[i];
	}
}

void
mixchannel::SetPeak(int count, const float *peaks)
{
	if(((uint32)count) > mFormat.u.raw_audio.channel_count){
		count = mFormat.u.raw_audio.channel_count;
	}
	for(int i=0; i<count; i++){
		mPeak[i] = peaks[i];
	}

}

void
mixchannel::Peak(int count, float *peaks)
{
	if(((uint32) count) > mFormat.u.raw_audio.channel_count){
		count = mFormat.u.raw_audio.channel_count;
	}
	for(int i=0; i<count; i++){
		peaks[i] = mPeak[i];
	}
}

void
mixchannel::SetMute(bool mute)
{
	mMute = mute;
	SaveLevels();
}

bool
mixchannel::Mute()
{
	return mMute;
}

void
mixchannel::SetSolo(bool solo)
{
	mSolo = solo;
}

bool
mixchannel::Solo()
{
	return mSolo;
}

void
mixchannel::SetPan(float pan)
{
	mPan = pan;
	float p = pi_over_four * mPan;
	mMultiPan[0] = 0.707 * (cos(p) - sin(p)); //reversed because of sliders
	mMultiPan[1] = 0.707 * (cos(p) + sin(p));
	for(int i=2; i<MAX_CHANNELS; i++){
		mMultiPan[i] = 1;
	}
	SaveLevels();
}

float
mixchannel::Pan()
{
	return mPan;
}

float
mixchannel::PanAndGain(int channel)
{
	return (mMultiGain[channel] * mMultiPan[channel]);
}

bool
mixchannel::HasSetting()
{
	return false;
}

void
mixchannel::GetSetting(channel_setting */*setting*/)
{
}

void
mixchannel::SubmitChange(const media_format & with_format,
						const media_format & mix_format)
{
	mFormat = with_format;
	mSynchronous = CheckSynchronicity(with_format, mix_format);
	
	if (NULL != mResampler)
		delete mResampler;
		
	mResampler = NewResampler(with_format, mix_format);
}

const media_format &
mixchannel::Format()
{
	return mFormat;
}

_resampler_base *
mixchannel::Resampler()
{
	return mResampler;
}

_resampler_base *
mixchannel::NewResampler(const media_format & with_format,
						const media_format & mix_format)
{
	_resampler_base *r;
	uint32			resampler_type;
	
	//
	// Get the resampler type
	// 
	if (NULL != mMixerParent)
	{
		resampler_type = mMixerParent->ResamplerType();
	}
	else
	{
		resampler_type = RESAMPLER_LINEAR;
	}
	
	//
	// RESAMPLER_SINC_LOWPASS should only be used for
	// downsampling.  Map RESAMPLER_SINC_LOWPASS to 
	// RESAMPLER_SINC if necessary.
	//
	if ((RESAMPLER_SINC_LOWPASS == resampler_type) &&
		(with_format.u.raw_audio.frame_rate <= mix_format.u.raw_audio.frame_rate))
	{
		resampler_type = RESAMPLER_SINC;
	}
	
	//
	// Giant switch statement from Heck!!!
	//
	switch (resampler_type)
	{
		case RESAMPLER_ZERO :
		default :
		{		
			switch(with_format.u.raw_audio.format){
				case media_raw_audio_format::B_AUDIO_UCHAR:
					r = new resampler_zero<uint8, float>(with_format.u.raw_audio.frame_rate,
														mix_format.u.raw_audio.frame_rate,
														with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					r = new resampler_zero<int32, float>(with_format.u.raw_audio.frame_rate,
														mix_format.u.raw_audio.frame_rate,
														with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_FLOAT:
					r = new resampler_zero<float, float>(with_format.u.raw_audio.frame_rate,
														mix_format.u.raw_audio.frame_rate,
														with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
				default:
					r = new resampler_zero<int16, float>(with_format.u.raw_audio.frame_rate,
														mix_format.u.raw_audio.frame_rate,
														with_format.u.raw_audio.channel_count);
					break;
			}
			
			break;
		} // RESAMPLER_ZERO
	
		case RESAMPLER_LINEAR :
		{		
			switch(with_format.u.raw_audio.format){
				case media_raw_audio_format::B_AUDIO_UCHAR:
					r = new resampler_linear<uint8, float>(	with_format.u.raw_audio.frame_rate,
															mix_format.u.raw_audio.frame_rate,
															with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					r = new resampler_linear<int32, float>(	with_format.u.raw_audio.frame_rate,
															mix_format.u.raw_audio.frame_rate,
															with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_FLOAT:
					r = new resampler_linear<float, float>(	with_format.u.raw_audio.frame_rate,
															mix_format.u.raw_audio.frame_rate,
															with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
				default:
					r = new resampler_linear<int16, float>(	with_format.u.raw_audio.frame_rate,
															mix_format.u.raw_audio.frame_rate,
															with_format.u.raw_audio.channel_count);
					break;
			}
			
			break;
		} // RESAMPLER_LINEAR
		
		case RESAMPLER_TRILINEAR :
		{		
			switch(with_format.u.raw_audio.format){
				case media_raw_audio_format::B_AUDIO_UCHAR:
					r = new resampler_trilinear<uint8, float>(	with_format.u.raw_audio.frame_rate,
																mix_format.u.raw_audio.frame_rate,
																with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					r = new resampler_trilinear<int32, float>(	with_format.u.raw_audio.frame_rate,
																mix_format.u.raw_audio.frame_rate,
																with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_FLOAT:
					r = new resampler_trilinear<float, float>(	with_format.u.raw_audio.frame_rate,
																mix_format.u.raw_audio.frame_rate,
																with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
				default:
					r = new resampler_trilinear<int16, float>(	with_format.u.raw_audio.frame_rate,
																mix_format.u.raw_audio.frame_rate,
																with_format.u.raw_audio.channel_count);
					break;
			}
			
			break;
		} // RESAMPLER_TRILINEAR
		
		case RESAMPLER_CUBIC :
		{		
			switch(with_format.u.raw_audio.format){
				case media_raw_audio_format::B_AUDIO_UCHAR:
					r = new resampler_cubic<uint8, float>(	with_format.u.raw_audio.frame_rate,
															mix_format.u.raw_audio.frame_rate,
															with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					r = new resampler_cubic<int32, float>(	with_format.u.raw_audio.frame_rate,
															mix_format.u.raw_audio.frame_rate,
															with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_FLOAT:
					r = new resampler_cubic<float, float>(	with_format.u.raw_audio.frame_rate,
															mix_format.u.raw_audio.frame_rate,
															with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
				default:
					r = new resampler_cubic<int16, float>(	with_format.u.raw_audio.frame_rate,
															mix_format.u.raw_audio.frame_rate,
															with_format.u.raw_audio.channel_count);
					break;
			}
			
			break;
		} // RESAMPLER_CUBIC
		
		case RESAMPLER_SINC :
		{		
			switch(with_format.u.raw_audio.format){
				case media_raw_audio_format::B_AUDIO_UCHAR:
					r = new resampler_sinc<uint8, float>(	with_format.u.raw_audio.frame_rate,
															mix_format.u.raw_audio.frame_rate,
															with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					r = new resampler_sinc<int32, float>(	with_format.u.raw_audio.frame_rate,
															mix_format.u.raw_audio.frame_rate,
															with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_FLOAT:
					r = new resampler_sinc<float, float>(	with_format.u.raw_audio.frame_rate,
															mix_format.u.raw_audio.frame_rate,
															with_format.u.raw_audio.channel_count);
					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
				default:
					r = new resampler_sinc<int16, float>(	with_format.u.raw_audio.frame_rate,
															mix_format.u.raw_audio.frame_rate,
															with_format.u.raw_audio.channel_count);
					break;
			}
			
			break;
		} // RESAMPLER_SINC
		
		case RESAMPLER_SINC_LOWPASS :
		{		
			switch(with_format.u.raw_audio.format){
				case media_raw_audio_format::B_AUDIO_UCHAR:
					r = new resampler_sinc_lowpass<uint8, float>
							(	
								with_format.u.raw_audio.frame_rate,
								mix_format.u.raw_audio.frame_rate,
								with_format.u.raw_audio.channel_count
							);
					break;
				case media_raw_audio_format::B_AUDIO_INT:
					r = new resampler_sinc_lowpass<int32, float>
							(	
								with_format.u.raw_audio.frame_rate,
								mix_format.u.raw_audio.frame_rate,
								with_format.u.raw_audio.channel_count
							);
					break;
				case media_raw_audio_format::B_AUDIO_FLOAT:
					r = new resampler_sinc_lowpass<float, float>
							(	
								with_format.u.raw_audio.frame_rate,
								mix_format.u.raw_audio.frame_rate,
								with_format.u.raw_audio.channel_count
							);

					break;
				case media_raw_audio_format::B_AUDIO_SHORT:
				default:
					r = new resampler_sinc_lowpass<int16, float>
							(	
								with_format.u.raw_audio.frame_rate,
								mix_format.u.raw_audio.frame_rate,
								with_format.u.raw_audio.channel_count
							);
					break;
			}
			
			break;
		} // RESAMPLER_SINC_LOWPASS

	} // end of giant switch statement from Heck!!!
	
	return r;
} // mixchannel::NewResampler

void
mixchannel::Connected(const media_source & producer,
					const media_destination & /*where*/,
					const media_format & with_format,
					media_input * out_input,
					const media_format & mix_format)
{
	mInputSource = producer;
	mFormat = with_format;
	SetName(out_input->name);
	mFormat = with_format;
	FORMATS(("FORMAT : frame_rate : %f\n", with_format.u.raw_audio.frame_rate));
	FORMATS(("FORMAT : format : 0x%x\n", with_format.u.raw_audio.format));
	FORMATS(("FORMAT : channels : %d\n", with_format.u.raw_audio.channel_count));
	
	if (NULL != mResampler) 
		delete mResampler;
		
	mResampler = NewResampler(with_format, mix_format);
	mSynchronous = CheckSynchronicity(with_format, mix_format);
	//fillin returned input
	out_input->source = mInputSource;
	out_input->destination = mInputDestination;
}

void
mixchannel::Disconnected()
{
	mFormat.type = B_MEDIA_RAW_AUDIO;
	mFormat.u.raw_audio.frame_rate = media_raw_audio_format::wildcard.frame_rate;
	mFormat.u.raw_audio.channel_count = media_raw_audio_format::wildcard.channel_count;
	mFormat.u.raw_audio.format = media_raw_audio_format::wildcard.format;
	mFormat.u.raw_audio.byte_order = ENDIAN;
	mFormat.u.raw_audio.buffer_size = 0;
	
	mFormat.u.raw_audio.channel_count = 2;
	mFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	mFormat.u.raw_audio.frame_rate = 96000;

	mInputSource = media_source::null;

	mGain = 1.0; /* 0 is off, 1.0 is full, higher is possible, but may clip! */
	for(int i=0; i<MAX_CHANNELS; i++){
		mMultiGain[i] = 1.0;
		mMultiPan[i] = 0.707;
		mLevel[i] = 0.0;
		mPeak[i] = 0.0;
	}	
	mMute = false; /*true == muted */
	mSolo = false; /*true == solo'd*/
	mPan = 0.0; /*1.0 is full left, -1.0 is full right, 0.0 is center */

	sprintf(mName, "Free");
}

media_destination
mixchannel::Destination()
{
	return mInputDestination;
}

media_source
mixchannel::Source()
{
	return mInputSource;
}

int32 
mixchannel::GetDataStatus()
{
	return mStatus;
}

void 
mixchannel::SetDataStatus(int32 status, bigtime_t at_media_time)
{
	mNewStatus = status;
	mNextStatusTime = at_media_time;
	if (status == B_DATA_AVAILABLE) {
		mResampler->clear();	

		mPrepIn = 0;
		mBuffIn = 0;
		mChain  = FALSE;
	}
}

bool
mixchannel::WaitingForData(bigtime_t mix_end_time, uint32 mix_frame_count)
{
	if(mix_end_time > mNextStatusTime){
		mStatus = mNewStatus;
	}
	if(mStatus == B_DATA_AVAILABLE && mFrameCount < mix_frame_count){
		return true;
	}else{
		return false;
	}
}

bool
mixchannel::CheckSynchronicity(const media_format & with_format,
								const media_format & mix_format)
{
	uint32 buffer_frame_count = with_format.u.raw_audio.buffer_size /
			((with_format.u.raw_audio.format & 0xf) * with_format.u.raw_audio.channel_count);
	uint32 mix_frame_count = mix_format.u.raw_audio.buffer_size /
			((mix_format.u.raw_audio.format & 0xf) * mix_format.u.raw_audio.channel_count);
	if ((double)mix_format.u.raw_audio.frame_rate*(double)buffer_frame_count ==
		(double)with_format.u.raw_audio.frame_rate*(double)mix_frame_count) {
		return true;
	}else{
		return false;
	}	
}

bool 
mixchannel::IsSynchronous()
{
	return mSynchronous;
}

int64 
mixchannel::FrameCount()
{
	return mFrameCount;
}

void 
mixchannel::IncrementFrameCount(int64 count)
{
	mFrameCount += count;
}

void 
mixchannel::ZeroFrameCount()
{
	mFrameCount = 0;
}

BParameter *
mixchannel::MakeWebControls(BParameterGroup *group, bool /*advanced*/)
{
	BParameter *top = NULL;
	char outname[B_MEDIA_NAME_LENGTH];
	sprintf(outname, "%ld", mInputDestination.id);	
	BParameterGroup *g = group->MakeGroup(outname);

	BNullParameter *in = g->MakeNullParameter(mParameterMask+CH_NAME, B_MEDIA_RAW_AUDIO, mName, B_WEB_BUFFER_INPUT);
	char bit[8];
	if(mFormat.u.raw_audio.format == media_raw_audio_format::B_AUDIO_FLOAT){
		sprintf(bit, "float");
	}else{
		sprintf(bit, "%ldbit", (mFormat.u.raw_audio.format & 0x7) * 8);
	}
	sprintf(outname, "%gkHz %s", mFormat.u.raw_audio.frame_rate/1000, bit);
	BNullParameter * bits = g->MakeNullParameter(mParameterMask+CH_INFO, B_MEDIA_RAW_AUDIO, outname, B_GENERIC);
	bits->AddInput(in);
	top = bits;
	BDiscreteParameter *mute = g->MakeDiscreteParameter(mParameterMask+CH_MUTE, B_MEDIA_RAW_AUDIO, "Mute", B_MUTE);
	DBUG(("channel mute: %x\n", mParameterMask+3));
	BContinuousParameter * gain = g->MakeContinuousParameter(mParameterMask+CH_GAIN, B_MEDIA_RAW_AUDIO, "Gain", 
		B_GAIN, "dB", GAIN_BOTTOM, GAIN_TOP, 1.0);
	DBUG(("channel gain: %x\n", mParameterMask+CH_GAIN));
	gain->SetChannelCount(mFormat.u.raw_audio.channel_count);
	gain->AddInput(mute);
	if(mFormat.u.raw_audio.channel_count <= 1){
		BContinuousParameter * pan = g->MakeContinuousParameter(mParameterMask+CH_PAN, B_MEDIA_RAW_AUDIO, "Pan",
													B_BALANCE, "L R", -1.0, 1.0, 0.02);
		DBUG(("channel pan: %x\n", mParameterMask+1));
		pan->AddInput(top);
		top = pan;
	}
	mute->AddInput(top);
	return (BParameter *)gain;
}

status_t 
mixchannel::GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *ioSize)
{
	PARAMS(("mixchannel::GetParameterValue - id: %x  Size: %d\n", id, *ioSize));
	
	*last_change = mLastChange;
	
	switch(id - mParameterMask) {
		case CH_PAN:{
			float *aValue = ((float *)value);
			*ioSize = sizeof(float);
			*aValue = Pan();
			PARAMS(("channel %d pan %f\n", mID, *aValue));
			return B_OK;
		break;}
		case CH_SOLO:{
			bool *aValue = ((bool *)value);
			*ioSize = sizeof(bool);
			*aValue = Solo();
			PARAMS(("channel %d solo %d\n", mID, *aValue));
			return B_OK;
		break;}
		case CH_MUTE:{
			bool *aValue = ((bool *)value);
			*ioSize = sizeof(bool);
			*aValue = Mute();
			PARAMS(("channel %d mute %d\n", mID, *aValue));
			return B_OK;
		break;}
		case CH_GAIN:{
			int32 count = mFormat.u.raw_audio.channel_count;
			*ioSize = sizeof(float) * count;
			for(int i=0; i<count; i++){
				float x = mMultiGain[i];
				//nice curves for the fader above and below 0dB
				if (x > 1.0) {
					x = sqrt((x-1)/7*GAIN_TOP*GAIN_TOP);
				}
				else {
					x = (GAIN_BOTTOM - 1) * (1 - 2 * x + x * x);
				}
				((float *)value)[i] = x;
				PARAMS(("gain%d : %f\n", i, x));
			}
			return B_OK;
		break;}
		default:
			PARAMS(("ERROR mixchannel couldn't find parameter for id: %x\n", id));
			return B_ERROR;
		break;						
	}
}

void 
mixchannel::SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size)
{
	PARAMS(("mixchannel::SetParameterValue - id: %x  size: %d\n", id, size));
	
	mLastChange = when;

	switch(id - mParameterMask) {
		case CH_PAN:{
			float *aValue = (float *)value;
			PARAMS(("set channel %d pan %f\n", mID, *aValue));
			SetPan(*aValue);
		break;}
		case CH_SOLO:{
			bool *aValue = (bool *)value;
			PARAMS(("set channel %d solo %d\n", mID, *aValue));
			SetSolo(*aValue);
		break;}
		case CH_MUTE:{
			PARAMS(("set channel %d mute %d\n", mID, *((int *)value)));
			SetMute(*((int *)value));
		break;}
		case CH_GAIN:{
			int32 count = mFormat.u.raw_audio.channel_count;
			float *f;
			if(size == sizeof(f[0])){
				PARAMS(("setting them all to the first value\n"));
				PARAMS(("size should be %d\n", sizeof(f[0]) * count));
				f = ((float *)value);
				for(int i=0; i<MAX_CHANNELS; i++){
					//nice curves for the fader above and below 0dB
					if (*f < 0) {
						SetGain(i, 1.0 - sqrt(-*f)/sqrt((float)1-GAIN_BOTTOM));
					}
					else {
						SetGain(i, 1.0 + (7 * *f * *f)/(GAIN_TOP * GAIN_TOP));
					}
					PARAMS(("set gain[%d] %f\n", i, mMultiGain[i]));
				}
			}else{
				for(int i=0; i<count; i++){
					f = ((float *)value)+ i;
					//nice curves for the fader above and below 0dB
					if (*f < 0) {
						SetGain(i, 1.0 - sqrt(-*f)/sqrt((float)1-GAIN_BOTTOM));
					}
					else {
						SetGain(i, 1.0 + (7 * *f * *f)/(GAIN_TOP * GAIN_TOP));
					}
					PARAMS(("set gain[%d] %f\n", i, mMultiGain[i]));
				}					
			}
		break;}
		default:
			PARAMS(("ERROR mixchannel couldn't find parameter for id: %x\n", id));
		break;						
	}
	return;
}

void *mixchannel::operator new (size_t size) throw (BAD_ALLOC)
{
	void *ret = rtm_alloc(_channel_heap, size);
	if (!ret)
		throw std::bad_alloc();		

	return ret;
}


void *mixchannel::operator new[] (size_t size) throw (BAD_ALLOC)
{
	void *ret = rtm_alloc(_channel_heap, size);
	if (!ret)
		throw std::bad_alloc();		

	return ret;
}

void mixchannel::operator delete (void *ptr) throw ()
{
	rtm_free(ptr);
}


void mixchannel::operator delete[] (void *ptr) throw ()
{
	rtm_free(ptr);
}

