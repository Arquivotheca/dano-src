/*******************************************************************************
/
/	File:			SoundRecordNode.h
/
/   Description:	Record sound from some sound-producing Node.
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#if !defined(SOUND_RECORD_NODE_H)
#define SOUND_RECORD_NODE_H

#include <BufferConsumer.h>

class _SoundRecordNode :
	public BBufferConsumer
{
public:
		_SoundRecordNode(
				const char * name,
				void (*RecordFunc)(void * cookie, bigtime_t timestamp, const void * data, size_t size, const media_raw_audio_format & format) = NULL,
				void (*NotifyFunc)(void * cookie, int32 code, ...) = NULL,
				void * cookie = NULL);
		~_SoundRecordNode();

		status_t SetHooks(
				void (*RecordFunc)(void * cookie, bigtime_t timestamp, const void * data, size_t size, const media_raw_audio_format & format) = NULL,
				void (*NotifyFunc)(void * cookie, int32 code, ...) = NULL,
				void * cookie = NULL);

virtual	port_id ControlPort() const;

virtual	BMediaAddOn* AddOn(
				int32 * internal_id) const;	/* Who instantiated you -- or NULL for app class */

		//	for the notify func
		enum {
			B_WILL_START = 1,		//	performance_time
			B_WILL_STOP,			//	performance_time immediate
			B_WILL_SEEK,			//	performance_time media_time
			B_WILL_TIMEWARP,		//	real_time performance_time
			B_PRODUCER_DATA_STATUS,	//	status performance_time
			B_CONNECTED,			//	name (char*)
			B_DISCONNECTED,			//
			B_FORMAT_CHANGED,		//	media_raw_audio_format*
			B_NODE_DIES,			//	node will die!
			B_OP_TIMED_OUT,			//	timeout that expired
			B_HOOKS_CHANGED			//	
		};

protected:

virtual	void Start(
				bigtime_t performance_time);
virtual	void Stop(
				bigtime_t performance_time,
				bool immediate);
virtual	void Seek(
				bigtime_t media_time,
				bigtime_t performance_time);
virtual	void SetRunMode(
				run_mode mode);
virtual	void TimeWarp(
				bigtime_t at_real_time,
				bigtime_t to_performance_time);
virtual	void Preroll();
virtual	void SetTimeSource(
				BTimeSource * time_source);
virtual	status_t HandleMessage(
				int32 message,
				const void * data,
				size_t size);

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
				bigtime_t at_media_time);
virtual	status_t GetLatencyFor(
				const media_destination & for_whom,
				bigtime_t * out_latency,
				media_node_id * out_timesource);
virtual	status_t Connected(
				const media_source & producer,	/* here's a good place to request buffer group usage */
				const media_destination & where,
				const media_format & with_format,
				media_input * out_input);
virtual	void Disconnected(
				const media_source & producer,
				const media_destination & where);
	/* The notification comes from the upstream producer, so he's already cool with */
	/* the format; you should not ask him about it in here. */
virtual	status_t FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 from_change_count,
				const media_format & format);

private:

virtual	status_t _Reserved_SoundRecordNode_0(void *, ...);
virtual	status_t _Reserved_SoundRecordNode_1(void *, ...);
virtual	status_t _Reserved_SoundRecordNode_2(void *, ...);
virtual	status_t _Reserved_SoundRecordNode_3(void *, ...);
virtual	status_t _Reserved_SoundRecordNode_4(void *, ...);
virtual	status_t _Reserved_SoundRecordNode_5(void *, ...);
virtual	status_t _Reserved_SoundRecordNode_6(void *, ...);
virtual	status_t _Reserved_SoundRecordNode_7(void *, ...);

		void (*_mRecordHook)(void * cookie, bigtime_t time, const void * data, size_t size, const media_raw_audio_format & format);
		void (*_mNotifyHook)(void * cookie, int32 cause, ...);
		void * _mCookie;
		media_input _mInput;
		thread_id _mThread;
		port_id _mPort;
		bigtime_t _mTimeout;

		uint32 _reserved_data_[16];

		//	Functions to calculate timing values. OK to override.
		//	Latency is returned to producer; Timeout is passed to
		//	call to read_port_etc() in service thread loop.
virtual	bigtime_t Timeout();
virtual	bigtime_t Latency();

		//	Functions called when no hooks are installed.
		//	OK to override instead of installing hooks.
virtual	void Record(
				bigtime_t time,
				const void * data,
				size_t size,
				const media_raw_audio_format & format);
virtual	void Notify(
				int32 cause,
				...);

static	status_t ThreadEntry(
				void *);
		void ServiceThread();
};


#endif	//	SOUND_RECORD_NODE_H

