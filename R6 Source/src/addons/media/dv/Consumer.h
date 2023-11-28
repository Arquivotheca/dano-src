#ifndef _DV_BUFFER_CONSUMER_H_
#define _DV_BUFFER_CONSUMER_H_

#define BUFFER_DEPTH		16

#define TOUCH(x) ((void)(x))

#include <media/Buffer.h>
#include <media/BufferConsumer.h>
#include <media/Controllable.h>
#include <media/TimeSource.h>
#include <support/Locker.h>

#include "Node.h"
#include "ieee1394.h"

class FlavorRoster;
class DVAudioEncoder;

class DVBufferConsumer :
	public virtual DVMediaEventLooper,
	public virtual BBufferConsumer,
	public virtual BControllable,
	public virtual BTimeSource
{
public:
					DVBufferConsumer(BMediaAddOn *addon, FlavorRoster *roster,
							const char *name, int32 internal_id,
							int32 bus, uint64 guid, bool PAL);
					~DVBufferConsumer();

virtual	status_t	InitCheck() const { return fInitStatus; }
virtual status_t	SetTo(int32 bus, uint64 guid, bool PAL);
virtual void		NotifyPresence(bool presence);

// BMediaNode
public:
		BMediaAddOn *AddOn(int32 *outInternalID) const;
protected:
virtual	void		NodeRegistered();
virtual	void		Preroll();

// BMediaEventLooper
protected:
	//
	// children overloading HandleEvent (to add functionality as
	// BBufferProducer or BTimeSource) *MUST* call through to
	// DVBufferConsumer::HandleEvent()
	//
virtual	void		HandleEvent(const media_timed_event *event,
							bigtime_t lateness,
							bool realTimeEvent = false);
virtual	void		SetRunMode(run_mode mode);

// BBufferConsumer
private:
virtual	status_t	HandleMessage(
							int32 message, const void *data, size_t size);

virtual	status_t	AcceptFormat(
							const media_destination &dest,
							media_format *format);

virtual	status_t	GetNextInput(
							int32 *cookie,
							media_input *out_input);

virtual	void		DisposeInputCookie(int32 cookie);

virtual	void		BufferReceived(BBuffer *buffer);

virtual	void		ProducerDataStatus(
							const media_destination & for_whom,
							int32 status,
							bigtime_t at_performance_time);

virtual	status_t	GetLatencyFor(
							const media_destination &for_whom,
							bigtime_t *out_latency,
							media_node_id *out_timesource);

virtual	status_t	Connected(
							const media_source &producer,
							const media_destination &where,
							const media_format &with_format,
							media_input *out_input);

virtual	void		Disconnected(
							const media_source &producer,
							const media_destination &where);

virtual	status_t	FormatChanged(
							const media_source &producer,
							const media_destination &consumer, 
							int32 change_tag,
							const media_format &format);

virtual	status_t	SeekTagRequested(
							const media_destination &destination,
							bigtime_t in_target_time,
							uint32 in_flags, 
							media_seek_tag *out_seek_tag,
							bigtime_t *out_tagged_time,
							uint32 *out_flags);

// BControllable
protected:
virtual status_t	GetParameterValue(int32 id, bigtime_t *last_change,
							void *value, size_t *size);
virtual void		SetParameterValue(int32 id, bigtime_t when,
							const void *value, size_t size);

// BTimeSource
private:
		enum event_type {
				TS_START = BTimedEventQueue::B_USER_EVENT,
				TS_STOP
		};

virtual	status_t	TimeSourceOp(const time_source_op_info &op, void *_reserved = NULL);

// user hooks
private:
		status_t	GetLatencyFor(int32 id, bigtime_t *out_latency);
		status_t	SetLatencyFor(int32 id, bigtime_t latency);
		void		ProducerDataStatus(int32 id, int32 status);
		void		TimeSourceOp(time_source_op op, bigtime_t rtime);
		void		HandleStart(bigtime_t ptime);
		void		HandleStop();
		void		HandleTimeWarp(bigtime_t to_performance_time);

// state
private:
		status_t		fInitStatus;

		BMediaAddOn		*fAddOn;
		FlavorRoster	*fFlavorRoster;
		int32			fInternalId;
		bool			fRunning;
		bool			fTimeSourceRunning;
		bigtime_t		fPerformanceTime;

		struct Input {
			bool			connected;
			/* XXX: should store base format in separate area, specialized
			 * one for the connection in input */
			media_format	base_format;
			media_input		input;
		};
		#define			VIDEO_INPUT		0
		#define			AUDIO_32K_INPUT	1
		#define			AUDIO_44K_INPUT	2
		#define			AUDIO_48K_INPUT	3
		#define			TOTAL_INPUTS	4
		Input			fInputs[TOTAL_INPUTS];

static	int32	_playback_thread_(void *data);
		int32			PlaybackThread();
		thread_id		fPlaybackThread;
		
		BTimedEventQueue fBufferEventQueue;
		sem_id			fVideoBufferReceivedSem;
		void			*fBuffers[BUFFER_DEPTH];
		DVAudioEncoder	*fAudioEncoder;

		int32			fBus;
		uint64			fGUID;
		bool			fPAL;
		int				fDriver;
		int				fIsocPort;
		sem_id			fIsocSem;
		ieee1394_area	fLockedAreas[BUFFER_DEPTH];

		double			fVideoBufferDuration;
		double			fAudioBufferDuration;
		bigtime_t		fOverhead;	

		bigtime_t		fLastStateChange;
		int32			fTransportEnabled;
};

#endif
