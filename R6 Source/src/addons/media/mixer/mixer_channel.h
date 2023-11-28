#ifndef MIXER_CHANNEL_H
#define MIXER_CHANNEL_H

#include <stdio.h>
#include <MediaDefs.h>
#include <ParameterWeb.h>
#include <new>

#ifdef __MWERKS__
	#define BAD_ALLOC bad_alloc
#else
	#define BAD_ALLOC std::bad_alloc
#endif

#define CHANGE_MEMORY 16
#define MAX_CHANNELS 2	//more to come!

class BBuffer;
class _resampler_base;
struct channel_setting;

class mixchannel
{
public:
			mixchannel(uint32 id, port_id port,class BMixer *mixer_parent);
			~mixchannel();
			
			uint32 ID();
			void SetName(const char *name);
			const char * Name();
			void SetGain(int channel, float gain);
			float Gain();
			float Gain(int channel);
			void SetLevel(int count,
							const float *levels);
			void Level(int count,
						float *levels);
			void SetPeak(int count,
						const float *peaks);
			void Peak(int count,
						float *peaks);
			void SetMute(bool mute);
			bool Mute();
			void SetSolo(bool solo);
			bool Solo();
			void SetPan(float pan);
			float Pan();
			float PanAndGain(int channel);
			
			BParameter *MakeWebControls(BParameterGroup *group,
										bool advanced = false);
			status_t GetParameterValue(int32 id,
										bigtime_t *last_change,
										void *value,
										size_t *ioSize);
			void SetParameterValue(int32 id,
									bigtime_t when,
									const void *value,
									size_t size);
			bool HasSetting();
			void GetSetting(channel_setting *setting);
			
			void SubmitChange(const media_format & with_format,
							  const media_format & mix_format);
			const media_format & Format();
			_resampler_base * Resampler();

			void Connected(const media_source & producer,
							const media_destination & where,
							const media_format & with_format,
							media_input * out_input,
							const media_format & mix_format);
			void Disconnected();
			media_destination Destination();
			media_source Source();
			
			int32 GetDataStatus();
			void SetDataStatus(int32 status, bigtime_t at_media_time);
			bool WaitingForData(bigtime_t mix_end_time, uint32 mix_frame_count);
			bool IsSynchronous();
			int64 FrameCount();
			void IncrementFrameCount(int64 count);
			void ZeroFrameCount();
			
			mixchannel *mNext;
			
			void *operator new (size_t) throw (BAD_ALLOC);
			void *operator new[] (size_t) throw (BAD_ALLOC);
			void operator delete (void *) throw ();
			void operator delete[] (void *) throw ();

			bigtime_t mLastMixTime;
			uint32 mPrepIn;			
			uint32 mBuffIn;
			bool   mChain;			
			
private:
			_resampler_base * NewResampler(const media_format & with_format,
										   const media_format & mix_format);
			bool CheckSynchronicity(const media_format & with_format,
									const media_format & mix_format);
			uint32 	mID;
			uint32 	mParameterMask;
			bigtime_t 	mLastChange;	//for parameters
			float 	mGain; /* 0 is off, 1.0 is full, higher is possible, but may clip! */
			float 	mMultiGain[MAX_CHANNELS];
			float 	mLevel[MAX_CHANNELS]; /* level is averaged per buffer */
			float 	mPeak[MAX_CHANNELS]; /* peak is absolute per buffer */
			bool 	mMute; /*true == muted */
			bool 	mSolo; /*true == solo'd*/
			/* pan is pan for mono, balance for stereo */
			float 	mPan; /*1.0 is full left, -1.0 is full right, 0 is center */
			float 	mMultiPan[MAX_CHANNELS];
			char 	mName[B_MEDIA_NAME_LENGTH];
	        media_destination 	mInputDestination; 	// This is my Input
	        media_source 		mInputSource;		// Who is connected to the input
			media_format 	mFormat;
			_resampler_base *	mResampler;
			bool 	mSynchronous;
			int64	mFrameCount;	//number of frames played
			int64	mFrameCountOffset;	//difference between this and mixer output
			int32 mStatus;			//ProducerDataStatus
			int32 mNewStatus;		//Pre-change ProducerDataStatus
			bigtime_t mNextStatusTime;	//time to change ProducerDataStatus
			
			BMixer	*mMixerParent;

private:
static	status_t deferred_save(void *);
		void SaveLevels();
};

#endif
			
