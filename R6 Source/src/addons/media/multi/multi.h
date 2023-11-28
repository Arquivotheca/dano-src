/* multi.h */
/* the prototype of the future */

/* to use a mutli_audio driver */
/* open the driver */
/* get the multi_description */
/* count the ins and outs */
/* set the enabled channels */
/* set the format */
/* OPTIONALLY get the mix */
/* get the buffers */
/* do multi_buffer_exchange */

#ifndef MULTI_H
#define MULTI_H

#include "multi_addon.h"
#include "channel.h"

#include <Buffer.h>
#include <BufferConsumer.h>
#include <BufferGroup.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <MediaAddOn.h>
#include <MediaEventLooper.h>
#include <MediaNode.h>
#include <multi_audio.h>
#include <RealtimeAlloc.h>
#include <TimedEventQueue.h>
#include <TimeSource.h>

class BMediaAddOn;
class WriteFifo;

class BMultiNode :
	public BMediaEventLooper,
	public BBufferConsumer,
	public BBufferProducer,
	public BTimeSource,
	public BControllable
{
public:
/* MultiNode */
virtual		~BMultiNode();
			BMultiNode(const char *name,
					BMultiAudioAddon *addon,
					int32 flavor,
					int fd,
					multi_setup setup);

/* from MediaNode */
virtual	BMediaAddOn* AddOn(
				int32 * internal_id) const;
virtual	void SetRunMode(
				run_mode mode);
virtual	void Preroll();
virtual	void SetTimeSource(
				BTimeSource * time_source);
virtual	status_t HandleMessage(
				int32 message,
				const void * data,
				size_t size);
virtual	status_t RequestCompleted(
				const media_request_info & info);
virtual	void NodeRegistered();
				
/* from MediaEventLooper */
virtual void HandleEvent(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false);
				
/* from BufferConsumer */
virtual	status_t AcceptFormat(
				const media_destination & dest,
				media_format * format);
virtual	status_t GetNextInput(
				int32 * cookie,
				media_input * out_input);
virtual	void DisposeInputCookie(
				int32 cookie);
virtual	void BufferReceived(
				BBuffer * buffer);
virtual	void ProducerDataStatus(
				const media_destination & for_whom,
				int32 status,
				bigtime_t at_performance_time);
virtual	status_t GetLatencyFor(
				const media_destination & for_whom,
				bigtime_t * out_latency,
				media_node_id * out_timesource);
virtual	status_t Connected(
				const media_source & producer,
				const media_destination & where,
				const media_format & with_format,
				media_input * out_input);
virtual	void Disconnected(
				const media_source & producer,
				const media_destination & where);
virtual	status_t FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 change_tag,
				const media_format & format);

/* from BufferProducer */
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
				int32 * _deprecated_);
virtual	status_t GetNextOutput(
				int32 * cookie,
				media_output * out_output);
virtual	status_t DisposeOutputCookie(
				int32 cookie);	/* dug - this should be a pointer n'est-ce pas */
virtual	status_t SetBufferGroup(
				const media_source & for_source,
				BBufferGroup * group);
virtual	status_t GetLatency(
				bigtime_t * out_lantency);
virtual	status_t PrepareToConnect(
				const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char * out_name);
virtual	void Connect(
				status_t error, 
				const media_source & source,
				const media_destination & destination,
				const media_format & format,
				char * io_name);
virtual	void Disconnect(
				const media_source & what,
				const media_destination & where);
virtual	void LateNoticeReceived(
				const media_source & what,
				bigtime_t how_much,
				bigtime_t performance_time);
virtual	void EnableOutput(
				const media_source & what,
				bool enabled,
				int32 * _deprecated_);

/* from TimeSource */
virtual	status_t TimeSourceOp(
				const time_source_op_info & op,
				void * _reserved);

/* from Controllable */
virtual	status_t GetParameterValue(
				int32 id,
				bigtime_t * last_change,
				void * value,
				size_t * ioSize);
virtual	void SetParameterValue(
				int32 id,
				bigtime_t when,
				const void * value,
				size_t size);

private:
/* implementation */
		bool IsUpstreamNode(const media_destination *dest);
		status_t CanDoFormatChange(const media_destination *dest, media_format *format);
		void ApplySetParameter(int32 id, const void *value);
		void MakeParameterWeb();
		void GetSettings();
		void SaveSettings();
		void GetDescription();
		void SetupEnable();
		void SetupFormats();
		void SetupBuffers();
		void SetupOutputs();
		void SetupInputs();
		void SetupMix();
		void SendBuffers();
		void EnableAllChannels();
		void DisableAllChannels();
		void EnableChannels(int32 channel, int32 count);
		void DisableChannels(int32 channel, int32 count);
		void SetGlobalFormat();
		void SetEnabledChannels();
		void WriteZeros(MultiChannel *ch);
		void UseBuffer(BBuffer *buffer);
		void StartDMA();
		void StopDMA();
		status_t MultiBufferExchange(bool starting = false);

		uint32 convert_multi_format_to_media_format(uint32 format);
		uint32 convert_media_format_to_multi_format(uint32 format);
		float convert_multi_rate_to_media_rate(uint32 rate, float cvsr);
		uint32 convert_media_rate_to_multi_rate(float rate);
		
		void string_for_multi_format(int32 format, char *str);
		void string_for_multi_lock_sources(int32 lock_source, char *str, int32 lock_data);
		void string_for_multi_timecode_sources(int32 timecode_source, char *str);
		
		BMultiAudioAddon *mAddon;
		int32		mFlavorID;
		int			mDriver;
		multi_setup	mSetup;
		int32		mTotalChannels;
		int32		mIOChannels;
		int32		mFirstOutputChannelID;
		int32		mFirstInputChannelID;
		uint32		mBufferSize;			/* frames */
		int32		mConnectedOutputs;
		int32		mConnectedInputs;
		int32		mBufferCount;
		int32		mRecordIndex;		
		int32		mPlaybackIndex;
		int64		mPlayedCount;
		int64		mBufferInfoPlayedFramesCount;
		float		mDrift;
		BTimedEventQueue *mEventQ;
		BTimedEventQueue *mRTQ;
		EndPointMap mOutputs;
		EndPointMap mInputs;
		media_format mRecordFormat;
		media_format mPlaybackFormat;
		multi_description 	mDescription;
		multi_channel_info 	*mChannelInfo;
		multi_format_info	mFormatInfo;
		multi_buffer_list	mBufferList;
		buffer_desc			**mRecordBufferDescriptions;
		buffer_desc			**mPlaybackBufferDescriptions;
		multi_buffer_info	mBufferInfo;
		multi_channel_enable mChannelEnable;
		multi_mix_channel_info mMixChannelInfo;
		multi_mix_control_info mMixControlInfo;
		multi_mix_value_info   mMixValueInfo;
		multi_mix_connection_info mMixConnectionInfo;
		bigtime_t	mNextExchangeTime;
		bigtime_t 	mRealStartTime;
		bigtime_t 	mPerfStartTime;
		bigtime_t	mBufferDuration;
		bigtime_t	mLastTime;
		bigtime_t 	mChocolateSaltyBallsFactor;		/* Fudge */
				
};

#endif /* MULTI_H */
