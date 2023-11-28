/*	BMixer.h	*/

#ifndef MIXER_H
#define MIXER_H

#include <BufferConsumer.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <BufferGroup.h>
#include <Locker.h>
#include <TimedEventQueue.h>
#include <string.h>
#include <time.h>

#include "mixer_channel.h"
#include "save_info.h"

#define MAX_CHANNEL_COUNT 2 //outputs - more to come!
#define MIN_CHANNEL_STRIPS 4 //inputs
#define MAX_CHANNEL_STRIPS 256 //inputs

#define B_LAUNCH_MIXER_CONTROL 'BLMX'

class BMediaAddOn;
class BLocker;
class BMixer;

struct channel_setting
{
	char name[B_FILE_NAME_LENGTH];
	float gain[MAX_CHANNEL_COUNT];
	bool mute;
};

struct antiglitch_struct {
	BMixer * mixer;
	mixchannel * mchannel;
	media_timed_event * event;
};	

class BMixer : public BBufferConsumer, public BBufferProducer, public BControllable
{
public:
	BMixer(char *name, int32 id, uint32 nBuffers, uint32 nChannels, BMediaAddOn *addon);		
	~BMixer();
	
	uint32 		ResamplerType();
	port_id		SavePortID();
	time_t		SaveTimestamp();
	save_map	*SaveMap();
	BLocker		*SaveLock();
	
	enum
	{
		SAVE_LEVELS	= 0,
		SAVE_RESAMPLER_TYPE
	};

// from BMediaNode
virtual	port_id ControlPort() const;
virtual	BMediaAddOn * AddOn(
				int32 *internal_id) const;

protected:
virtual	void Start(
				bigtime_t performance_time);
virtual	void Stop(
				bigtime_t performance_time, bool immediate);
virtual void Seek(
				bigtime_t performance_time,
				bigtime_t media_time);
virtual	void TimeWarp(
				bigtime_t at_real_time,
				bigtime_t to_performance_time);
status_t HandleMessage(
				int32 message,
				const void * data,
				size_t size);
virtual void NodeRegistered();

// from BControllable
status_t StartControlPanel(
				BMessenger * out_messenger);
status_t GetParameterValue(
				int32 id,
				bigtime_t *last_change,
				void *value,
				size_t *ioSize);
void SetParameterValue(
				int32 id,
				bigtime_t when,
				const void *value,
				size_t size);

//	from BBufferConsumer 
public:
virtual	void BufferReceived(
				BBuffer * buffer);
virtual	status_t AcceptFormat(
				const media_destination & dest,
				media_format * format);
virtual	status_t GetNextInput(
				int32 * cookie,
				media_input * out_input);
virtual	void DisposeInputCookie(
				int32 cookie);
virtual	status_t FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 from_change_count,
				const media_format & format);
virtual void  ProducerDataStatus(
				const media_destination & for_whom,
				int32 status,
				bigtime_t at_media_time);
virtual status_t GetLatencyFor(
				const media_destination &,
				bigtime_t *out_latency,
				media_node_id * out_timesource);

virtual	status_t Connected(
				const media_source & producer,
				const media_destination & where,
				const media_format & with_format,
				media_input * out_input);
virtual	void Disconnected(
				const media_source & producer,
				const media_destination & where);

// from	BBufferProducer
public:
virtual	status_t FormatSuggestionRequested(
				media_type type,
				int32 quality,
				media_format * format);
virtual	status_t FormatProposal(
				const media_source & output,
				media_format * format);
virtual	status_t FormatChangeRequested(
				const media_source & source,
				const media_destination & destination,
				media_format * io_format,
				int32 * out_change_count);
virtual	void LateNoticeReceived(
				const media_source & what,
				bigtime_t how_much,
				bigtime_t performance_time);
virtual	status_t GetNextOutput(
				int32 * cookie,
				media_output * out_destination);
virtual	status_t DisposeOutputCookie(
				int32 cookie);
virtual	status_t SetBufferGroup(
				const media_source & for_source,
				BBufferGroup * group);
virtual	status_t PrepareToConnect(
				const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char *out_name);
virtual	void Connect(		/* be sure to call BBufferProducer::Connect()! */
				status_t error, 
				const media_source & what,
				const media_destination & where,
				const media_format & format,
				char * out_name);
virtual	void Disconnect(	/* be sure to call BMediaNode::Disconnect()! */
				const media_source & what,
				const media_destination & where);
virtual	void EnableOutput(
				const media_source & what,
				bool enabled,
				int32 * change_tag);


//	implementation
protected:
static	status_t MixThreadEntry(void * data);
		void MixRun();

		void HandleEvent();
		void SetupMixBuffer();
		void MixBuffer(BBuffer *buffer);
		void OfflineCheck();
		status_t SendMixBuffer();
		
static	status_t WebThreadEntry(void * data);
		void WebRun();
		void MakeParameterWeb();
			
		void WriteSettingsFile(
				const channel_setting *list,
				const int32 & count);
		int32 ReadSettingsFile(
				channel_setting *list,
				const int32 &count);
		void SaveSettings();
		int CheckForSettings(
				char *name);
		
		void WaitForChannels(
				uint32 mix_frame_count);
		void WaitForStart();
		status_t WaitForMessage(
				bigtime_t wait_until);

		int32 FindNextEmptyChannel(
				int32 index = 1);
		void MakeChannels(
				mixchannel **next,
				int32 count = 4);
inline	mixchannel *GetChannel(int32 dest){
			/* check to be sure the channel is in range */
			/* we don't allow connections on 0 */
			if(dest <= 0 || dest > MAX_CHANNEL_STRIPS-1)
				return NULL;
			else
				return mChannelArray[dest];
		};
		bigtime_t ChannelStartTime(mixchannel * ch, bigtime_t the_time);
		void AntiGlitch(mixchannel * ch);
static	BTimedEventQueue::queue_action _find_channel_event(media_timed_event *event, void *context);

		media_format mOutputFormat;
		media_format mMixFormat;

		bigtime_t mMixBufferDuration;
		double mMixFrameDuration;
		uint32 mMixFrameSize;
		uint32 mMixFrameCount;
		uint64 mMixFrameTotal;
		bool mMixBufFresh;
		
		BBufferGroup *mOurGroup;
		BBufferGroup *mOutputGroup;
		BBuffer *mOutputBuffer;
		BBufferGroup *mMixGroup;
		BBuffer *mMixBuffer;
		
		thread_id mMixThread;
		thread_id mWebThread;
		
		char mMessage[B_MEDIA_MESSAGE_SIZE];
		port_id mControlPort;

		mixchannel	*mMaster;
		mixchannel 	*mChannelOne;				// The list of Channels
		mixchannel 	**mChannelArray;
		uint32 mConnectedCount;
		BTimedEventQueue mEventQueue;
		BTimedEventQueue mBufferQueue;
		
		media_source mOutputSource;				// This is my Output
		media_destination mOutputDestination;	// Who is connected to the output
		
		bool mActive;
		bool mRunning;
		bool mOffline;
		bool mOutputEnabled;
		bool mHookedUp;
		bool mMixing;
		
		int32 mDataStatus;
		int32 mChannelDataStatus;
		
		bigtime_t mStartTime;					// Time to start
		bigtime_t mMixStartTime;
		bigtime_t mNextMixTime;					// Time for next buffer
		bigtime_t mTimeToSend;
		bigtime_t mDownStreamLatency;			// Latency of downstream nodes
		bigtime_t mMixLatency;					// How long it takes to mix
		bigtime_t mSendLatency;					// How long it takes to send
		bigtime_t mSchedulingLatency;
				
		BLocker 	mLock;
					
		BMediaAddOn *mAddon;
		int32		mFlavorID;
		
		channel_setting *mMixSettingList;
		int32 		mWatchForSettings;

		bigtime_t 	mLastParameterChangeTime;		//last time a parameter changed
		
		uint32		mResamplerType;
		thread_id	mSaveThreadID;
		port_id		mSavePortID;
		save_map	mSaveMap;
		time_t		mSaveTimestamp;
		BLocker		mSaveLock;
		
		sem_id 		mSettingsLoadedSem;			// this is only valid inside the
												// constructor
		
static	status_t	save_thread(void *m);
};

#endif /* MIXER_H */
