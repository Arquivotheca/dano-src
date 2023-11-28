#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <media/Buffer.h>
#include <media/ParameterWeb.h>
#ifndef DEBUG
	#define DEBUG 1
#endif
#include <support/Debug.h>

#include "Consumer.h"
#include "DVDefs.h"
#include "Flavor.h"
#include "dva.h"
#include "ieee1394.h"
#include "avc.h"
#include "vcr.h"

#define DEBUG_PREFIX "DVBufferConsumer::"
#include "debug.h"

const int32 P_TRANSPORT_ENABLED = 'xprt';
const bigtime_t DEFAULT_AVC_TIMEOUT = 1000000;

#define BUFFER_SIZE (DV_PAL_ENCODED_FRAME_SIZE + (300 + 50) * 8)

//
// ctor/dtor
//

DVBufferConsumer::DVBufferConsumer(
				BMediaAddOn *addon,	FlavorRoster *roster,
				const char *name, int32 internal_id,
				int32 bus, uint64 guid, bool PAL) :
		BMediaNode(name),
		DVMediaEventLooper(),
		BBufferConsumer(B_MEDIA_ENCODED_VIDEO),
		BControllable(),
		BTimeSource(),

		fInitStatus(B_NO_INIT),
		fAddOn(addon),
		fFlavorRoster(roster),
		fInternalId(internal_id),
		fRunning(false),
		fTimeSourceRunning(false),
		fPerformanceTime(0),

		fVideoBufferReceivedSem(-1),
		fAudioEncoder(NULL),

		fBus(bus),
		fGUID(guid),
		fPAL(PAL),

		fOverhead(0)
{
	AddNodeKind(B_PHYSICAL_OUTPUT);

	for (int32 i=0;i<TOTAL_INPUTS;i++)
		fInputs[i].connected = false;

	fInitStatus = B_OK;

	return;
}

DVBufferConsumer::~DVBufferConsumer()
{
	if (fRunning)
		HandleStop();

	for (int32 i=0;i<TOTAL_INPUTS;i++) {
		if (fInputs[i].connected)
			Disconnected(fInputs[i].input.source, fInputs[i].input.destination);
	}

	fFlavorRoster->RemoveNode(this);

	Quit();
}

status_t DVBufferConsumer::SetTo(int32 bus, uint64 guid, bool PAL)
{
	bool was_running;

	PRINTF(0, ("from %lx|%Lx to %lx|%Lx\n", fBus, fGUID, bus, guid));

	if (fInitStatus < B_OK)
		return fInitStatus;

	was_running = fRunning;

	if (was_running)
		HandleStop();

	fBus = bus;
	fGUID = guid;
	fPAL = PAL;

	/* XXX: if connected, make sure format still makes sense */
		
	if (was_running)
		HandleStart(Now() /* XXX */);

	return B_OK;
}

void DVBufferConsumer::NotifyPresence(bool presence)
{
	TOUCH(presence);
}

//
// BMediaNode hooks
//

BMediaAddOn *
DVBufferConsumer::AddOn(int32 *outInternalID) const
{
	if (outInternalID)
		*outInternalID = fInternalId;
	return fAddOn;
}

void
DVBufferConsumer::NodeRegistered()
{
	if (fInitStatus < B_OK) {
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}

	BParameterWeb *web = new BParameterWeb();
	BParameterGroup *main = web->MakeGroup(Name());
	BDiscreteParameter *state = main->MakeDiscreteParameter(
										P_TRANSPORT_ENABLED,
										B_MEDIA_UNKNOWN_TYPE,
										"Record to Tape",
										B_ENABLE);
	state->AddItem(0, "Disabled");
	state->AddItem(1, "Enabled");

	fLastStateChange = system_time();
	fTransportEnabled = false;
	SetParameterWeb(web);

	fInputs[VIDEO_INPUT].input.node = Node();
	fInputs[VIDEO_INPUT].input.source = media_source::null;
	fInputs[VIDEO_INPUT].input.destination.port = ControlPort();
	fInputs[VIDEO_INPUT].input.destination.id = VIDEO_INPUT;

		fInputs[VIDEO_INPUT].base_format.type = B_MEDIA_ENCODED_VIDEO;
		fInputs[VIDEO_INPUT].base_format.u.encoded_video = fPAL ? dv_pal_format : dv_ntsc_format;
		fInputs[VIDEO_INPUT].base_format.u.encoded_video.encoding =
				media_encoded_video_format::B_ANY;

	fInputs[VIDEO_INPUT].input.format = fInputs[VIDEO_INPUT].base_format;
	strcpy(fInputs[VIDEO_INPUT].input.name, "DV Video");

	fInputs[AUDIO_32K_INPUT].input.node = Node();
	fInputs[AUDIO_32K_INPUT].input.source = media_source::null;
	fInputs[AUDIO_32K_INPUT].input.destination.port = ControlPort();
	fInputs[AUDIO_32K_INPUT].input.destination.id = AUDIO_32K_INPUT;
		fInputs[AUDIO_32K_INPUT].base_format.type = B_MEDIA_RAW_AUDIO;
		fInputs[AUDIO_32K_INPUT].base_format.u.raw_audio =
				media_raw_audio_format::wildcard;
		fInputs[AUDIO_32K_INPUT].base_format.u.raw_audio.frame_rate = 32000;
		// XXX: accept mono inputs as well
		fInputs[AUDIO_32K_INPUT].base_format.u.raw_audio.channel_count = 2;
		fInputs[AUDIO_32K_INPUT].base_format.u.raw_audio.format =
				media_raw_audio_format::B_AUDIO_SHORT;
	fInputs[AUDIO_32K_INPUT].input.format=fInputs[AUDIO_32K_INPUT].base_format;
	strcpy(fInputs[AUDIO_32K_INPUT].input.name, "DV Audio 32k");

	fInputs[AUDIO_44K_INPUT] = fInputs[AUDIO_32K_INPUT];
	fInputs[AUDIO_44K_INPUT].input.destination.id = AUDIO_44K_INPUT;
	fInputs[AUDIO_44K_INPUT].base_format.u.raw_audio.frame_rate = 
		fInputs[AUDIO_44K_INPUT].input.format.u.raw_audio.frame_rate = 44100;
	strcpy(fInputs[AUDIO_44K_INPUT].input.name, "DV Audio 44.1k");

	fInputs[AUDIO_48K_INPUT] = fInputs[AUDIO_32K_INPUT];
	fInputs[AUDIO_48K_INPUT].input.destination.id = AUDIO_48K_INPUT;
	fInputs[AUDIO_48K_INPUT].base_format.u.raw_audio.frame_rate = 
		fInputs[AUDIO_48K_INPUT].input.format.u.raw_audio.frame_rate = 48000;
	strcpy(fInputs[AUDIO_48K_INPUT].input.name, "DV Audio 48k");

	Run();
}

//
// BMediaEventLooper hooks
//

void
DVBufferConsumer::HandleEvent(const media_timed_event *event,
		bigtime_t lateness, bool realTimeEvent)
{
	TOUCH(lateness);

	switch(event->type) {
		case BTimedEventQueue::B_START:
			HandleStart(realTimeEvent ?
					TimeSource()->PerformanceTimeFor(event->event_time) :
					event->event_time);
			break;
	
		case BTimedEventQueue::B_STOP:
			HandleStop();
			break;
	
		case BTimedEventQueue::B_WARP:
			HandleTimeWarp(event->bigdata);
			break;
	
		case BTimedEventQueue::B_TIMER:
			TimerExpired(event->event_time, event->data);
			break;

		case BTimedEventQueue::B_DATA_STATUS:
		{
			ProducerDataStatus(event->data, event->bigdata);
			break;
		}

		case TS_START:
			TimeSourceOp(BTimeSource::B_TIMESOURCE_START, event->event_time);
			break;
	
		case TS_STOP:
			TimeSourceOp(BTimeSource::B_TIMESOURCE_STOP, event->event_time);
			break;
	
		case BTimedEventQueue::B_SEEK:
			PRINTF(-1, ("ignoring unhandled event B_SEEK\n"));
			break;

		case BTimedEventQueue::B_HARDWARE:
		default:
			PRINTF(-1, ("ignoring unknown event 0x%lx\n", event->type));
			break;
	}
}

void
DVBufferConsumer::SetRunMode(run_mode mode)
{
	BMediaEventLooper::SetRunMode(mode);
}

//
// BBufferConsumer hooks
//

status_t
DVBufferConsumer::HandleMessage(int32 message, const void * data, size_t size)
{
	if	(	BMediaNode::HandleMessage(message, data, size) &&
			BMediaEventLooper::HandleMessage(message, data, size) &&
			BBufferConsumer::HandleMessage(message, data, size) &&
			BTimeSource::HandleMessage(message, data, size)) {
		HandleBadMessage(message, data, size);
	}

	return B_OK;
}

status_t
DVBufferConsumer::AcceptFormat(
		const media_destination &dest, media_format *format)
{
	status_t err;

	if (fInitStatus < B_OK)
		return B_NO_INIT;

	if ((dest.id < 0) || (dest.id >= TOTAL_INPUTS))
		return B_BAD_INDEX;

	err = format_is_compatible(*format, fInputs[dest.id].input.format) ?
			B_OK : B_MEDIA_BAD_FORMAT;

	// XXX: specialize formats, check video encoding id

	*format = fInputs[dest.id].input.format;

	return err;
}

status_t
DVBufferConsumer::GetNextInput(int32 *cookie, media_input *out_input)
{
	if (fInitStatus < B_OK)
		return B_NO_INIT;

	if (!cookie || !out_input)
		return B_BAD_VALUE;

	if ((*cookie < 0) || (*cookie >= TOTAL_INPUTS))
		return B_BAD_INDEX;

	if (	(*cookie != 0) &&
			(	(fInputs[AUDIO_32K_INPUT].connected) ||
				(fInputs[AUDIO_44K_INPUT].connected) ||
				(fInputs[AUDIO_48K_INPUT].connected))) {
		while (fInputs[*cookie].connected == false) {
			(*cookie)++;
			if (*cookie >= TOTAL_INPUTS)
				return B_BAD_INDEX;
		}
	}

	*out_input = fInputs[*cookie].input;
	(*cookie)++;

	return B_OK;
}

void
DVBufferConsumer::DisposeInputCookie(int32 cookie)
{
	TOUCH(cookie);
}

void
DVBufferConsumer::BufferReceived(BBuffer *buffer)
{
	int32 id;
	status_t err;

	id = buffer->Header()->destination;

	if ((id < 0) || (id >= TOTAL_INPUTS)) {
		buffer->Recycle();
		return;
	}

	if (!fRunning || !fInputs[id].connected) {
		PRINTF(-1, ("Not running or not connected\n"));
		buffer->Recycle();
		return;
	}

	if (	(id == VIDEO_INPUT) &&
			(buffer->SizeUsed() != 120000)	&&
			(buffer->SizeUsed() != 144000)) {
		PRINTF(-1, ("Bad video buffer received (size %ld)\n", buffer->SizeUsed()));
		buffer->Recycle();
		return;
	}

	media_timed_event event;
	event.event_time = buffer->Header()->start_time;
	event.type = BTimedEventQueue::B_USER_EVENT + id;
	event.pointer = buffer;
	event.cleanup = BTimedEventQueue::B_RECYCLE_BUFFER;

	if (id != VIDEO_INPUT)
		event.data = fInputs[id].input.format.u.raw_audio.byte_order;

	err = fBufferEventQueue.AddEvent(event);
	if (err < B_OK) {
		PRINTF(-1, ("Error adding buffer event (%lx)\n", err));
		buffer->Recycle();
	} else if (id == VIDEO_INPUT) {
		// signal buffer presence for event loop
		release_sem(fVideoBufferReceivedSem);
	}
}

void
DVBufferConsumer::ProducerDataStatus(const media_destination & for_whom,
		int32 status, bigtime_t at_performance_time)
{
	if ((for_whom.id < 0) || (for_whom.id >= TOTAL_INPUTS))
		return;

	media_timed_event event(
			at_performance_time, BTimedEventQueue::B_DATA_STATUS,
			NULL, 0, for_whom.id, status, NULL);
	if (EventQueue()->AddEvent(event) < B_OK) {
		PRINTF(-1, ("error adding data status event\n"));
		ProducerDataStatus(for_whom.id, status);
	}
}

status_t
DVBufferConsumer::GetLatencyFor(const media_destination &for_whom,
		bigtime_t *out_latency, media_node_id *out_timesource)
{
	if (!out_latency || !out_timesource)
		return B_BAD_VALUE;

	if ((for_whom.id < 0) || (for_whom.id >= TOTAL_INPUTS))
		return B_BAD_INDEX;

	BTimeSource *ts = TimeSource();
	*out_timesource = ts ? ts->ID() : (media_node_id)0;

	return GetLatencyFor(for_whom.id, out_latency);
}

status_t
DVBufferConsumer::Connected(
		const media_source &producer, const media_destination &where,
		const media_format &with_format, media_input *out_input)
{
	if (fInitStatus < B_OK)
		return B_NO_INIT;

	if (!out_input)
		return B_BAD_VALUE;

	if (	(where.id < 0) || (where.id >= TOTAL_INPUTS) ||
			(where != fInputs[where.id].input.destination)) {
		PRINTF(-1, ("Bad destination\n"));
		return B_BAD_VALUE;
	}

	if (	(where.id != VIDEO_INPUT) &&
			(fInputs[AUDIO_32K_INPUT].connected ||
			 fInputs[AUDIO_44K_INPUT].connected ||
			 fInputs[AUDIO_48K_INPUT].connected)) {
		PRINTF(-1, ("An audio channel has already been connected\n"));
		return B_MEDIA_ALREADY_CONNECTED;
	}

	if (fInputs[where.id].connected) {
		PRINTF(-1, ("channel already connected\n"));
		return B_MEDIA_ALREADY_CONNECTED;
	}

	fInputs[where.id].input.source = producer;
//! eww, don't do this
	FormatChanged(producer, where, 0, with_format);
	*out_input = fInputs[where.id].input;	// XXX?
	fInputs[where.id].connected = true;

	return B_OK;
}


void
DVBufferConsumer::Disconnected(
		const media_source &producer, const media_destination &where)
{
	if (	(where.id < 0) || (where.id >= TOTAL_INPUTS) ||
			(producer != fInputs[where.id].input.source) ||
			(where != fInputs[where.id].input.destination)) {
 		PRINTF(-1, ("unknown destination %ld/%ld\n", where.port, where.id));
		return;
	}

	if (!fInputs[where.id].connected) {
		PRINTF(-1, ("not connected\n"));
		return;
	}

	if ((where.id != VIDEO_INPUT) && fAudioEncoder) {
		delete fAudioEncoder;
		fAudioEncoder = NULL;
	}

	fInputs[where.id].input.source = media_source::null;
	fInputs[where.id].input.format = fInputs[where.id].base_format;
	fInputs[where.id].connected = false;
}

status_t
DVBufferConsumer::FormatChanged(
		const media_source &producer, const media_destination &consumer, 
		int32, const media_format &format)
{
	if (fInitStatus < B_OK)
		return B_NO_INIT;

	if (	(consumer.id < 0) || (consumer.id >= TOTAL_INPUTS) ||
			(consumer != fInputs[consumer.id].input.destination) ||
			(producer != fInputs[consumer.id].input.source)) {
		PRINTF(0, ("Invalid connection\n"));
		return B_BAD_VALUE;
	}

	// format was already checked by AcceptFormat()

	fInputs[consumer.id].input.format = format;

	if (consumer.id == VIDEO_INPUT) {
		fVideoBufferDuration =
				(double)1e6 / (double)format.u.encoded_video.output.field_rate;
		PRINTF(1, ("buffer duration: %g\n", fVideoBufferDuration));
	} else {
		if (fAudioEncoder)
			fAudioEncoder->SetParameters(
					8 * (format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK),
					format.u.raw_audio.frame_rate,
					format.u.raw_audio.channel_count);
		else
			fAudioEncoder = new DVAudioEncoder(
					8 * (format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK),
					format.u.raw_audio.frame_rate,
					format.u.raw_audio.channel_count);

		fAudioBufferDuration =
				(double)1e6 / (double)format.u.raw_audio.frame_rate;

		if (fAudioEncoder->InitCheck() < B_OK) {
			delete fAudioEncoder;
			fAudioEncoder = NULL;
			return B_ERROR;
		}

		PRINTF(1, ("audio connected at %g\n", format.u.raw_audio.frame_rate));
	}

	return B_OK;
}

status_t
DVBufferConsumer::SeekTagRequested(
		const media_destination &destination, bigtime_t in_target_time,
		uint32 in_flags, media_seek_tag *out_seek_tag,
		bigtime_t *out_tagged_time, uint32 *out_flags)
{
	TOUCH(in_target_time); TOUCH(in_flags);
	TOUCH(out_seek_tag); TOUCH(out_tagged_time);
	TOUCH(out_flags);

	if ((destination.id < 0) || (destination.id >= TOTAL_INPUTS))
		return B_MEDIA_BAD_DESTINATION;

	PRINTF(-1, ("SeekTagRequested -- Unsupported!\n"));

	return ENOSYS;
}

//
// BControllable functions
//

status_t
DVBufferConsumer::GetParameterValue(
    int32 id, bigtime_t *last_change, void *value, size_t *size)
{
	if (id != P_TRANSPORT_ENABLED)
		return B_BAD_VALUE;

	*last_change = fLastStateChange;
	*size = sizeof(int32);
	*((int32 *)value) = fTransportEnabled;

	return B_OK;
}

// XXX: respect when field
void
DVBufferConsumer::SetParameterValue(
    int32 id, bigtime_t when, const void *value, size_t size)
{
	int32 new_state;

	if ((id != P_TRANSPORT_ENABLED) || !value || (size != sizeof(int32)))
		return;

	// XXX: handle case where user changes state while node is running?
	if (fRunning) {
		printf("Sorry, cannot change record mode while node is running\n");
		return;
	}

	new_state = *(int32 *)value;
	if ((new_state != 0) && (new_state != 1))
		return;

    fLastStateChange = when;
    fTransportEnabled = new_state;

	BroadcastNewParameterValue(fLastStateChange, P_TRANSPORT_ENABLED,
			&fTransportEnabled, sizeof(fTransportEnabled));
}

//
// BTimeSource functions
//

status_t
DVBufferConsumer::TimeSourceOp(const time_source_op_info &op, void *)
{
	media_timed_event event;
	time_source_op opp = op.op;

	// BTimedEventQueue::B_START has the same value as
	// BTimeSource::B_TIMESOURCE_START (other collisions exist as well),
	// so we have to remap the event name before we place it in the queue.
	switch (opp) {
		case BTimeSource::B_TIMESOURCE_START :
			event.type = TS_START;
			break;
		case BTimeSource::B_TIMESOURCE_STOP :
		case BTimeSource::B_TIMESOURCE_STOP_IMMEDIATELY :
			event.type = TS_STOP;
			break;
		default :
			PRINTF(-1, ("unknown operation %d\n", opp)); return B_ERROR;
	}

	// short-circuit: skip BTimedEventQueue to avoid a race condition:
	// if the TIMESOURCE_START hasn't posted by the time someone does a
	// GetTime(), B_INFINITE_TIMEOUTs start showing up in read_port_etc()s
	if (	(op.real_time <= BTimeSource::RealTime()) ||
			(opp == B_TIMESOURCE_STOP_IMMEDIATELY)) {
		TimeSourceOp(opp, op.real_time);
		return B_OK;
	}

	event.event_time = op.real_time;
	RealTimeQueue()->AddEvent(event);
	return B_OK;
}

//
// Internal functions
//

void
DVBufferConsumer::HandleStart(bigtime_t ptime)
{
	int32 i;

	TOUCH(ptime);

	if (fInitStatus < B_OK)
		return;

	if (fRunning) {
		PRINTF(0, ("Already started\n"));
		return;
	}

	if (fBus >= 0) {
		fDriver = open_1394(fBus);
		if (fDriver < 0) {
			PRINTF(-1, ("Unable to open 1394 bus (%s)\n", strerror(errno)));
			fInitStatus = errno;
			goto err1;
		}
	
		memset(fBuffers, 0, sizeof(fBuffers[0]) * BUFFER_DEPTH);
		for (i=0;i<BUFFER_DEPTH;i++) {
			fBuffers[i] = malloc_locked(fDriver, &fLockedAreas[i],
					(BUFFER_SIZE + B_PAGE_SIZE-1) & ~(B_PAGE_SIZE-1), B_CONTIGUOUS);
			if (!fBuffers[i]) {
				for (int32 j=0;j<i;j++)
					free_locked(fDriver, &fLockedAreas[j]);
				fInitStatus = B_NO_MEMORY;
				PRINTF(-1, ("unable to allocate buffer %ld\n", i));
				goto err2;
			}
		}

		fVideoBufferReceivedSem = create_sem(0, "DVBufferConsumer count");
		if (fVideoBufferReceivedSem < B_OK) {
			PRINTF(-1, ("unable to create semaphore\n"));
			fInitStatus = fVideoBufferReceivedSem;
			goto err3;
		}
	
		fInitStatus = fFlavorRoster->AcquireBusAndGUID(fBus, fGUID);
		if (fInitStatus < 0) {
			PRINTF(-1, ("Unable to acquire device\n"));
			goto err4;
		}
	
		fIsocSem = create_sem(0, "DVBufferConsumer");
		if (fIsocSem < B_OK) {
			PRINTF(-1, ("unable to create semaphore\n"));
			fInitStatus = fIsocSem;
			goto err5;
		}
	
		fIsocPort = acquire_isochronous_port(fDriver, B_1394_ISOC_WRITE_PORT);
		if (fIsocPort < B_OK) {
			PRINTF(-1, ("failed to acquire isoc port\n"));
			goto err6;
		}

		if (fTransportEnabled) {
			struct avc_subunit vcr;	
			vcr.fd = fDriver;
			vcr.guid = fGUID;
			vcr.type = AVC_TAPE_PLAYER_RECORDER;
			vcr.id = 0;
			vcr_record(&vcr, VCR_RECORD_RECORD, DEFAULT_AVC_TIMEOUT);
		}

		fPlaybackThread = spawn_thread(_playback_thread_,
				"DVBufferConsumer playback", B_REAL_TIME_PRIORITY, this);
		if (fPlaybackThread < B_OK) {
			PRINTF(-1, ("error spawning playback thread\n"));
			goto err7;
		}
		resume_thread(fPlaybackThread);
	}

	fRunning = true;

	return;

err7:
	release_isochronous_port(fDriver, fIsocPort);
err6:
	delete_sem(fIsocSem);
err5:
	fFlavorRoster->ReleaseBusAndGUID(fBus, fGUID);
err4:
	delete_sem(fVideoBufferReceivedSem);
err3:
	for (i=0;i<BUFFER_DEPTH;i++)
		free_locked(fDriver, &fLockedAreas[i]);
err2:
	close_1394(fDriver);
err1:
	return;
}

void
DVBufferConsumer::HandleStop()
{
	if (fInitStatus < B_OK)
		return;

	if (!fRunning) {
		PRINTF(0, ("Not running\n"));
		return;
	}

	if (fBus >= 0) {
		struct ieee1394_istop is;
		int32 i;

		delete_sem(fVideoBufferReceivedSem);
		delete_sem(fIsocSem);
		wait_for_thread(fPlaybackThread, &fPlaybackThread);
	
		// stop isoc transmission
		is.port = fIsocPort;
		is.event.type = B_1394_ISOC_EVENT_NOW;
		ioctl(fDriver, B_1394_STOP_ISOCHRONOUS, &is);
	
		release_isochronous_port(fDriver, fIsocPort);
		delete_sem(fIsocSem);

		if (fTransportEnabled) {
			struct avc_subunit vcr;	
			vcr.fd = fDriver;
			vcr.guid = fGUID;
			vcr.type = AVC_TAPE_PLAYER_RECORDER;
			vcr.id = 0;
			vcr_wind(&vcr, VCR_WIND_STOP, DEFAULT_AVC_TIMEOUT);
		}

		fFlavorRoster->ReleaseBusAndGUID(fBus, fGUID);
		for (i=0;i<BUFFER_DEPTH;i++)
			free_locked(fDriver, &fLockedAreas[i]);
		close_1394(fDriver);
	}

	fRunning = false;
}

void
DVBufferConsumer::HandleTimeWarp(bigtime_t to_performance_time)
{
	if (fInitStatus < B_OK)
		return;

	PRINTF(-1, ("HandleTimeWarp(%Lx)\n", to_performance_time));
}

void 
DVBufferConsumer::Preroll()
{
	struct avc_subunit vcr;

	if (fInitStatus < B_OK)
		return;

	if (!fTransportEnabled)
		return;

	if (fBus < 0)
		return;

	// Only do this if the node hasn't yet been started (otherwise the
	// transport could be paused in the middle of recording).
	if (fRunning)
		return;

	vcr.fd = open_1394(fBus);
	if (vcr.fd < 0)
		return;
	vcr.guid = fGUID;
	vcr.type = AVC_TAPE_PLAYER_RECORDER;
	vcr.id = 0;

	vcr_record(&vcr, VCR_RECORD_PAUSE, DEFAULT_AVC_TIMEOUT);

	for (int32 i=0;i<5;i++) {
		uchar mode, state;
		snooze(100000);
		vcr_transport_state(&vcr, &mode, &state, DEFAULT_AVC_TIMEOUT);
		if ((mode == VCR_RECORD) && (state == VCR_RECORD_PAUSE))
			break;
	}

	close_1394(vcr.fd);
}

status_t 
DVBufferConsumer::GetLatencyFor(int32 id, bigtime_t *out_latency)
{
	TOUCH(id);

	if (fInitStatus < B_OK)
		return B_NO_INIT;

	*out_latency = (bigtime_t)(fVideoBufferDuration * BUFFER_DEPTH + fOverhead);

	return B_OK;	
}

status_t 
DVBufferConsumer::SetLatencyFor(int32 id, bigtime_t latency)
{
	TOUCH(id);

	PRINTF(-1, ("(%Ld)\n", latency));
	if (fInitStatus < B_OK)
		return B_NO_INIT;

	bigtime_t current = (bigtime_t)(fVideoBufferDuration * BUFFER_DEPTH + fOverhead);

	if (current - fOverhead > latency)
		fOverhead = 0;
	else
		fOverhead = latency - (current - fOverhead);

	return B_OK;
}

void
DVBufferConsumer::ProducerDataStatus(int32 id, int32 status)
{
	TOUCH(id); TOUCH(status);
}

struct inject_params {
	bigtime_t		t0, t1;
	DVAudioEncoder	*encoder;
};

BTimedEventQueue::queue_action
inject_audio(media_timed_event *event, void *context)
{
	struct inject_params *p = (struct inject_params *)context;
	BBuffer *buffer;

	buffer = reinterpret_cast<BBuffer *>(event->pointer);
	if (!buffer) {
		ASSERT(0);
		return BTimedEventQueue::B_REMOVE_EVENT;
	}

	switch (event->type - BTimedEventQueue::B_USER_EVENT) {
		case AUDIO_32K_INPUT :
		case AUDIO_44K_INPUT :
		case AUDIO_48K_INPUT :
			break;
		case VIDEO_INPUT :
			return BTimedEventQueue::B_NO_ACTION;	// XXX
		default :
			ASSERT(0);
			buffer->Recycle();
			return BTimedEventQueue::B_REMOVE_EVENT;
	}

	if (event->event_time >= p->t1) {
		PRINTF(10, ("Audio event too late (%Ld >= %Ld)\n", \
				event->event_time, p->t1));
		return BTimedEventQueue::B_DONE;
	}

	// XXX: race condition
#define _C_ 2
	if (p->encoder) {
		int32 retval;

		retval = p->encoder->AddAudioToFrame(
				event->event_time, (int16 *)buffer->Data(),
				buffer->SizeUsed() / sizeof(int16) / _C_, event->data);
		if (retval > 0)
			return BTimedEventQueue::B_DONE;
		if (retval < 0)
			PRINTF(10, ("Dropped audio event\n"));
	}

	buffer->Recycle();

	return BTimedEventQueue::B_REMOVE_EVENT;
}

int32 
DVBufferConsumer::PlaybackThread()
{
	struct ieee1394_dvwrite dv;
	uint32 frame = 0, repeats = 0;
	int32 FirstSent, LastSent;
	const media_timed_event *event;
	bigtime_t first_buffer_time = 0;
	struct inject_params params;
	BBuffer *buffer;
	status_t frame_status;

	// initialize DV write data
	dv.port = fIsocPort;
	dv.sem = fIsocSem;
	dv.pal = fPAL;
	dv.state.seq = 0;
	dv.state.cycle_time = 0;
	dv.state.fraction = 0;

	FirstSent = 0; LastSent = BUFFER_DEPTH - 5;

	if (acquire_sem_etc(fVideoBufferReceivedSem, LastSent + 2, 0, 0) < B_OK)
		return B_OK;

	for (int32 i=0;i<LastSent;i++) {
		event = fBufferEventQueue.FindFirstMatch(
				0, BTimedEventQueue::B_ALWAYS, true,
				BTimedEventQueue::B_USER_EVENT + VIDEO_INPUT);
		if (!event) {
			PRINTF(-1, ("Error getting event (no more?)\n"));
			return B_ERROR;
		}

		// XXX: check event time
		PRINTF(-1, ("Event time: %Ld\n", event->event_time));

		if (i == 0)
			first_buffer_time = event->event_time;

		buffer = reinterpret_cast<BBuffer *>(event->pointer);
		if (!buffer) {
			PRINTF(-1, ("Bad buffer\n"));
			return B_ERROR;
		}
		memcpy(fBuffers[i], buffer->Data(), buffer->SizeUsed());
		buffer->Recycle();

		// XXX: inject buffers
		params.t0 = event->event_time;
		params.t1 = params.t0 + fVideoBufferDuration;
		params.encoder = fAudioEncoder;
		frame_status = (fAudioEncoder) ? fAudioEncoder->InitializeFrame(fBuffers[i], params.t0) : B_ERROR;
		fBufferEventQueue.RemoveEvent(event);
		fBufferEventQueue.DoForEach(inject_audio, &params);
		if (frame_status == B_OK) fAudioEncoder->CompletedFrame();
	}

	// XXX: snooze until proper start time
PRINTF(-1, ("should wait until %Ld (%Ld)\n", RealTimeFor(first_buffer_time, 1000LL), system_time()));

//	if (acquire_sem_etc(fIsocSem, 1, B_ABSOLUTE_TIMEOUT,
//				RealTimeFor(first_buffer_time, 1000LL)) < B_OK)
//		return B_OK;

	// Write frames to the camera
	for (int32 i=0;i<LastSent;i++) {
		dv.buffer = (uchar *)fBuffers[i];
		dv.headers = (uint32 *)((uchar *)dv.buffer + DV_PAL_ENCODED_FRAME_SIZE);
		if (ioctl(fDriver, B_1394_DV_WRITE, &dv, sizeof(dv)) < B_OK) {
			PRINTF(-1, ("error writing frame %ld\n", i));
			return B_ERROR;
		}
	}

	// Publish time
	fPerformanceTime = first_buffer_time;
	PublishTime(fPerformanceTime, BTimeSource::RealTime(),
			fTimeSourceRunning ? 1.0f : 0.0f);

	/* XXX: quick, timeout */

	#if TIME_FRAMES
		bigtime_t now = system_time();
	#endif

	while (acquire_sem(fIsocSem) == B_OK) {
		#if TIME_FRAMES
			if ((frame % 30) == 29)
				printf("time: %Ld\n", system_time() - now);
			now = system_time();
		#endif

		fPerformanceTime += fVideoBufferDuration;		// XXX: roundoff error!
		PublishTime(fPerformanceTime, BTimeSource::RealTime(),
				fTimeSourceRunning ? 1.0f : 0.0f);

		FirstSent = (FirstSent + 1) % BUFFER_DEPTH;
		int32 NextToSend = (LastSent + 1) % BUFFER_DEPTH;

		while ((event = fBufferEventQueue.FindFirstMatch(
				0, BTimedEventQueue::B_ALWAYS, true,
				BTimedEventQueue::B_USER_EVENT + VIDEO_INPUT)) != NULL) {
			buffer = reinterpret_cast<BBuffer *>(event->pointer);
			if (!buffer) {
				PRINTF(-1, ("Bad buffer\n"));
				return B_ERROR;
			}

			// check time
			if (event->event_time < params.t0 + 3000) {
				run_mode m = RunMode();
				PRINTF(10, ("Late video buffer (%Ld, %Ld) %Ld\n", \
						params.t0 + 3000, event->event_time, \
						params.t0 + 3000 - event->event_time));
				if ((m == B_RECORDING) || (m == B_OFFLINE)) {
					PRINTF(-1, ("Using buffer anyway\n"));
				} else {
					NotifyLateProducer(fInputs[VIDEO_INPUT].input.source,
							params.t0 - event->event_time, event->event_time);

					if (m == B_DROP_DATA) {
						PRINTF(-1, ("Dropping buffer\n"));
						buffer->Recycle();
						fBufferEventQueue.RemoveEvent(event);
						continue;
					}
				}
			}

			if (event->event_time > params.t1 + fVideoBufferDuration - 3000) {
				PRINTF(10, ("Missing video frame (%Ld, %Ld) %Ld\n", \
						params.t1, event->event_time, \
						event->event_time - params.t1 - (bigtime_t)fVideoBufferDuration + 3000));
				event = NULL;
				break;
			}
/*
			PRINTF(10, ("Using video buffer # %ld at %Ld (%Ld)\n", \
					buffer->Header()->u.raw_video.field_sequence, \
					event->event_time, params.t1));
*/
			params.t0 = params.t1;
			memcpy(fBuffers[NextToSend], buffer->Data(), buffer->SizeUsed());
			buffer->Recycle();
			fBufferEventQueue.RemoveEvent(event);

			repeats = 0;

			break;
		}

		if (!event) {
			memcpy(fBuffers[NextToSend], fBuffers[LastSent],
				fPAL ? DV_PAL_ENCODED_FRAME_SIZE : DV_NTSC_ENCODED_FRAME_SIZE);

			params.t0 = params.t1;

			repeats++;
			PRINTF(0, ("Repeating frame %ld %ld times\n", \
					frame - repeats, repeats));
		}

		params.t1 = params.t0 + fVideoBufferDuration;
		params.encoder = fAudioEncoder;
		frame_status = (fAudioEncoder) ? fAudioEncoder->InitializeFrame(fBuffers[NextToSend], params.t0) : B_ERROR;
		fBufferEventQueue.DoForEach(inject_audio, &params);
		if (frame_status == B_OK) fAudioEncoder->CompletedFrame();

		dv.buffer = (uchar *)fBuffers[NextToSend];
		dv.headers = (uint32 *)((uchar *)dv.buffer + DV_PAL_ENCODED_FRAME_SIZE);
		if (ioctl(fDriver, B_1394_DV_WRITE, &dv, sizeof(dv)) < B_OK) {
			PRINTF(-1, ("error issuing DV write\n"));
			return B_ERROR;
		}
		LastSent = NextToSend;

		frame++;
	}

	PRINTF(0, ("wrote %ld frames\n", frame));

	// flush remaining events in queue
	fBufferEventQueue.FlushEvents(0, BTimedEventQueue::B_ALWAYS);

	return B_OK;

}

int32 
DVBufferConsumer::_playback_thread_(void *data)
{
	return ((DVBufferConsumer *)data)->PlaybackThread();
}

void
DVBufferConsumer::TimeSourceOp(time_source_op op, bigtime_t rtime)
{
	switch(op) {
		case BTimeSource::B_TIMESOURCE_START:
			fTimeSourceRunning = true;
			PublishTime(fPerformanceTime, rtime, 1.0f);
			break;

		case BTimeSource::B_TIMESOURCE_STOP:
		case BTimeSource::B_TIMESOURCE_STOP_IMMEDIATELY:
			fTimeSourceRunning = false;
			PublishTime(fPerformanceTime, rtime, 0.0f);
			break;

		case BTimeSource::B_TIMESOURCE_SEEK:
			/* XXX: handle */
		default:
			PRINTF(-1, ("%s: unknown timesource op %d\n", Name(), (int)op));
			break;
	}
}
