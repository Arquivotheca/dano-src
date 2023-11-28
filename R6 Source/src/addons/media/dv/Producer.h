#ifndef _DV_BUFFER_PRODUCER_H
#define _DV_BUFFER_PRODUCER_H

#include <kernel/OS.h>
#include <media/BufferProducer.h>
#include <media/Controllable.h>
#include <media/MediaDefs.h>
#include <media/MediaNode.h>

#include "Node.h"

#include "ieee1394.h"

// transport states
// XXX should this enum go in ParameterWeb.h?
enum {
	T_NO_DEVICE,
	T_REWIND,
	T_FRAME_REWIND,
	T_STOP,
	T_PLAY,
	T_PAUSE,
	T_FRAME_FORWARD,
	T_FAST_FORWARD
};

class FlavorRoster;

class DVBufferProducer :
	public virtual DVMediaEventLooper,
	public virtual BBufferProducer,
	public virtual BControllable
{
public:
					DVBufferProducer(BMediaAddOn *addon, FlavorRoster *roster,
							const char *name, int32 internal_id,
							/* bus < 0 --> unspecified */
							int32 bus, uint64 guid, bool PAL);
virtual				~DVBufferProducer();

virtual	status_t	SetTo(int32 bus, uint64 guid, bool PAL);
virtual	status_t	InitCheck() const { return fInitStatus; }
virtual	void		NotifyPresence(bool presence);

private:
		status_t			fInitStatus;

		int32				fInternalID;
		BMediaAddOn *		fAddOn;
		FlavorRoster *		fFlavorRoster;

		media_output		fOut;
		bool				fPresent;
		bool				fRunning;
		bool				fConnected;
		bool				fEnabled;

		int					fDriver;
		int32				fBus;
		uint64				fGUID;
		bool				fPAL;

		BBufferGroup		*fBufferGroup;

static	int32				_frame_assembler_(void *data);
		int32				FrameAssembler();
		thread_id			fAssemblyThread;

		uint32				fFieldCount;
		uchar				fIsocChannel;
		ieee1394_area		fLockedArea;
		int32				fIsocPort;
		sem_id				fIsocPacket;
		uint32				*fIsocBuffer;
		ieee1394_iio_hdr	*fIsocHeader;

		status_t	RealStart();
		status_t	RealStop();

		status_t	HandleStart();
		status_t	HandleStop();

		void		CurrentTransportState();
		void		SwitchTransportState(int32 new_state, bigtime_t when);
		int32		fTransportMode;
		int32		fTransportState;
		bigtime_t	fLastStateChange;

/* BMediaNode */
public:
virtual	BMediaAddOn	*AddOn(int32 * internal_id) const;
virtual	status_t 	HandleMessage(int32 message, const void *data,
							size_t size);		
protected:	
virtual	void 		Preroll();
virtual	void 		NodeRegistered();


/* BMediaEventLooper */
protected:
virtual void		HandleEvent(const media_timed_event *event,
							bigtime_t lateness, bool realTimeEvent = false);
virtual void		CleanUpEvent(const media_timed_event *event);
		
/* BBufferProducer */									
protected:
virtual	status_t	FormatSuggestionRequested(media_type type, int32 quality,
							media_format * format);
virtual	status_t 	FormatProposal(const media_source &output,
							media_format *format);
virtual	status_t	FormatChangeRequested(const media_source &source,
							const media_destination &destination,
							media_format *io_format, int32 *_deprecated_);
virtual	status_t 	GetNextOutput(int32 * cookie, media_output * out_output);
virtual	status_t	DisposeOutputCookie(int32 cookie);
virtual	status_t	SetBufferGroup(const media_source &for_source,
							BBufferGroup * group);
virtual	status_t 	VideoClippingChanged(const media_source &for_source,
							int16 num_shorts, int16 *clip_data,
							const media_video_display_info &display,
							int32 * _deprecated_);
virtual	status_t	GetLatency(bigtime_t * out_latency);
virtual	status_t	PrepareToConnect(const media_source &what,
							const media_destination &where,
							media_format *format,
							media_source *out_source, char *out_name);
virtual	void		Connect(status_t error, const media_source &source,
							const media_destination &destination,
							const media_format & format, char *io_name);
virtual	void 		Disconnect(const media_source & what,
							const media_destination & where);
virtual	void 		LateNoticeReceived(const media_source & what,
							bigtime_t how_much, bigtime_t performance_time);
virtual	void 		EnableOutput(const media_source & what, bool enabled,
							int32 * _deprecated_);
virtual	status_t	SetPlayRate(int32 numer,int32 denom);
virtual	void 		AdditionalBufferRequested(const media_source & source,
							media_buffer_id prev_buffer, bigtime_t prev_time,
							const media_seek_tag * prev_tag);
virtual	void		LatencyChanged(const media_source & source,
							const media_destination & destination,
							bigtime_t new_latency, uint32 flags);

/* BControllable */									
protected:
virtual status_t	GetParameterValue(int32 id, bigtime_t *last_change,
							void *value, size_t *size);
virtual void		SetParameterValue(int32 id, bigtime_t when,
							const void *value, size_t size);
};

#endif
