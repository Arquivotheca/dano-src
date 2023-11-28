#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

#include <media/Buffer.h>
#include <media/BufferGroup.h>
#include <media/ParameterWeb.h>
#include <media/TimeSource.h>

#ifndef DEBUG
	#define DEBUG 1
#endif
#include <support/Debug.h>

#define TOUCH(x) ((void)(x))

#include "Producer.h"
#include "DVDefs.h"
#include "Flavor.h"

#include "ieee1394.h"
#include "avc.h"
#include "vcr.h"

#define DEBUG_PREFIX "DVBufferProducer::"
#include "debug.h"

const int32 P_TRANSPORT_STATE = 1;
const bigtime_t	DEFAULT_AVC_TIMEOUT = 1000000;

static status_t isoc_read_request(
		int fd, int32 port, uchar channel, int32 buffers, uint32 len,
		void *base, ieee1394_iio_hdr *hdr, sem_id sem);

static status_t isoc_read_cancel(int fd, uint32 port);

const int32		ISOC_BUFFER_COUNT	= 256;
const int32		ISOC_PACKET_SIZE	= 488;
const bigtime_t	NODE_LATENCY 		= 5000LL;

DVBufferProducer::DVBufferProducer(
		BMediaAddOn *addon, FlavorRoster *roster,
		const char *name, int32 internal_id,
		int32 bus, uint64 guid, bool PAL)
  :	BMediaNode(name),
	DVMediaEventLooper(),
	BBufferProducer(B_MEDIA_ENCODED_VIDEO),
	BControllable()
{
	status_t err;

	fInternalID = internal_id;
	fAddOn = addon;
	fFlavorRoster = roster;
	fInitStatus = B_NO_INIT;

	/* XXX: create this in SetTo? */
	fBufferGroup = new BBufferGroup(DV_PAL_ENCODED_FRAME_SIZE, 8);
	if (fBufferGroup->InitCheck() < B_OK) {
		err = fBufferGroup->InitCheck();
		delete fBufferGroup;
		goto err1;
	}

	if (bus >= 0) {
		fDriver = open_1394(bus);
		if (fDriver < B_OK) {
			printf("Unable to open 1394 driver\n");
			err = fDriver;
			goto err2;
		}
		fPresent = true;
	} else {
		fDriver = -1;
		fPresent = false;
	}
	fBus = bus;
	fGUID = guid;
	fPAL = PAL;
	fIsocChannel = 0x3f;

	fRunning = false;
	fConnected = false;
	fEnabled = false;

	AddNodeKind(B_PHYSICAL_INPUT);

	fInitStatus = B_OK;
	return;

//err3:
//	close_1394(fDriver);
err2:
	delete fBufferGroup;
err1:
	fInitStatus = err;
	return;
}


DVBufferProducer::~DVBufferProducer()
{
	if (fInitStatus < B_OK)
		return;

	if (fConnected)
		Disconnect(fOut.source, fOut.destination);

	if (fRunning)
		HandleStop();

	fFlavorRoster->RemoveNode(this);

	close_1394(fDriver);
	delete fBufferGroup;
}

/* XXX: format of default node should be same as last one, not always NTSC */
status_t
DVBufferProducer::SetTo(int32 bus, uint64 guid, bool PAL)
{
	bool format_changed = (fPAL != PAL);

	PRINTF(0, ("from %lx|%Lx to %lx|%Lx\n", fBus, fGUID, bus, guid));

	if ((bus == fBus) && (guid == fGUID)) {
		PRINTF(0, ("bus and guid identical\n"));
		return B_OK;
	}

	bool was_running = fRunning;
	if (was_running)
		HandleStop();

	if (fDriver >= 0) {
		ASSERT(bus < 0);
		close_1394(fDriver);
	} else {
		ASSERT(bus >= 0);
	}

	fDriver = -1;
	if (bus >= 0) {
		fDriver = open_1394(bus);
		if (fDriver < B_OK) {
			printf("Unable to open 1394 driver\n");
			return fDriver;
		}
	}

	fBus = bus;
	fGUID = guid;
	fPAL = PAL;
	fIsocChannel = 0x3f;

	fOut.format.u.encoded_video = fPAL ? dv_pal_format : dv_ntsc_format;

	/* notify others if PAL-ness changes */
	if (format_changed && fConnected) {
		media_format format = fOut.format;
		PRINTF(-1, ("Proposing format change\n"));
		ProposeFormatChange(&format, fOut.destination);
	}

	if (was_running)
		HandleStart();

	return B_OK;
}

void
DVBufferProducer::NotifyPresence(bool present)
{
	if (fPresent == present)
		return;

	fPresent = present;

	if (fInitStatus < B_OK)
		return;

	if (!present)
		fTransportState = T_NO_DEVICE;
	else
		CurrentTransportState();
	fLastStateChange = system_time();

	BroadcastNewParameterValue(fLastStateChange, P_TRANSPORT_STATE,
				&fTransportState, sizeof(fTransportState));				
}

/* RealStart() and RealStop() exist to work around problems where an app
 * leaves the node running but unconnected. The rule is to only "run" when
 * the node has been both started and connected. If either one of these
 * conditions is false, don't do anything.
 *
 * XXX: there are still some issues to work out with timing while the node is
 *      running but disconnected.
 */
status_t
DVBufferProducer::RealStart(void)
{
	status_t err;
	size_t locked_size;

	PRINTF(0, ("(%s)\n", Name()));

	if (fBus < 0)
		return B_OK;

	if (fFlavorRoster->AcquireBusAndGUID(fBus, fGUID) < B_OK) {
		PRINTF(0, ("already capturing\n"));
		return EALREADY;
	}

	locked_size = (sizeof(ieee1394_iio_hdr) * ISOC_BUFFER_COUNT)
						+ (ISOC_PACKET_SIZE * ISOC_BUFFER_COUNT);
	locked_size = (locked_size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE-1);

	fIsocBuffer = (uint32 *)malloc_locked(fDriver, &fLockedArea,
										locked_size, B_CONTIGUOUS);
	if (fIsocBuffer == NULL) {
		err = ENOMEM;
		goto err1;
	}
	memset(fIsocBuffer, 0, locked_size);
	fIsocHeader = (ieee1394_iio_hdr *)((uchar *)fIsocBuffer +
					(ISOC_PACKET_SIZE * ISOC_BUFFER_COUNT));

	fIsocPacket = create_sem(0, "isochronous packet");
	if (fIsocPacket < B_OK) {
		err = fIsocPacket;
		goto err2;
	}

	/* XXX: priority? */
	fAssemblyThread = spawn_thread(_frame_assembler_, "DV frame assembler",
									B_REAL_TIME_PRIORITY, this);
	if (fAssemblyThread < B_OK) {
		err = fAssemblyThread;
		goto err3;
	}
	resume_thread(fAssemblyThread);

	fIsocPort = acquire_isochronous_port(fDriver, B_1394_ISOC_READ_PORT);
	if (fIsocPort < B_OK) {
		err = fIsocPort;
		goto err4;
	}
	
	err = isoc_read_request(
			fDriver, fIsocPort, fIsocChannel, ISOC_BUFFER_COUNT,
			ISOC_PACKET_SIZE, fIsocBuffer, fIsocHeader, fIsocPacket);
	if (err < B_OK)
		goto err5;

	fFieldCount = 0;

	return B_OK;

err5:
	release_isochronous_port(fDriver, fIsocPort);
err4:
	delete_sem(fIsocPacket);
	{ status_t _; wait_for_thread(fAssemblyThread, &_); }
	// kill_thread(fAssemblyThread);
err3:
	delete_sem(fIsocPacket);
err2:
	free_locked(fDriver, &fLockedArea);
err1:
	return err;
}

status_t
DVBufferProducer::RealStop(void)
{
	status_t _;

	PRINTF(0, ("(%s)\n", Name()));

	if (fBus < 0)
		return B_OK;

	isoc_read_cancel(fDriver, fIsocPort);
	release_isochronous_port(fDriver, fIsocPort);

	delete_sem(fIsocPacket);
	wait_for_thread(fAssemblyThread, &_);

	free_locked(fDriver, &fLockedArea);

	fFlavorRoster->ReleaseBusAndGUID(fBus, fGUID);

	return B_OK;
}

status_t
DVBufferProducer::HandleStart()
{
	PRINTF(0, ("(%s)\n", Name()));

	if (fInitStatus < B_OK)
		return fInitStatus;

	if (fRunning) {
		PRINTF(-1, ("Already running\n"));
		return EALREADY;
	}

	if (fConnected) {
		status_t err = RealStart();
		if (err < B_OK)
			return err;
	}

	fRunning = true;
	return B_OK;
}

status_t
DVBufferProducer::HandleStop()
{
	PRINTF(0, ("(%s)\n", Name()));

	if (fInitStatus < B_OK)
		return fInitStatus;

	if (!fRunning) {
		PRINTF(-1, ("Not running\n"));
		return B_NO_INIT;
	}

	if (fConnected) {
		status_t err = RealStop();
		if (err < B_OK)
			return err;
	}

	fRunning = false;
	return B_OK;
}

int32 
DVBufferProducer::_frame_assembler_(void *data)
{
	return ((DVBufferProducer *)data)->FrameAssembler();
}

int32 
DVBufferProducer::FrameAssembler()
{
	BBuffer *buffer;
	uchar *frame;
	uint32 i, framenum, framepos;
	uint32 DV_ENCODED_FRAME_SIZE =
			fPAL ? DV_PAL_ENCODED_FRAME_SIZE : DV_NTSC_ENCODED_FRAME_SIZE;
	bool last_result;

	i = 0;	
	framenum = 0;
	framepos = 0;

	buffer = fBufferGroup->RequestBuffer(DV_ENCODED_FRAME_SIZE,0LL);
	frame = buffer ? (uchar *)buffer->Data() : NULL;

	last_result = true;

	while (1) {
		uint32 j, x, y, *packet;

		/* XXX: The SendDataStatus handling is broken in the case where you
		 * connect, disconnect, and reconnect to a running node. */
		if (acquire_sem_etc(fIsocPacket, 1, B_TIMEOUT, 0LL) == B_OK) {
			printf("quick (frame %ld) %lx\n", framenum, fIsocPacket);
		} else
		if (last_result == false) {
			if (acquire_sem(fIsocPacket) < B_OK)
				break;
		} else {
			status_t err = acquire_sem_etc(fIsocPacket, 1, B_TIMEOUT, 100000LL);
			if (err == B_TIMED_OUT) {
				if (fEnabled)
					SendDataStatus(B_DATA_NOT_AVAILABLE, fOut.destination,
							TimeSource()->PerformanceTimeFor(system_time()));
				last_result = false;
				continue;
			} else if (err < B_OK)
				break;
		}

		if (last_result == false) {
			if (fEnabled)
				SendDataStatus(B_DATA_AVAILABLE, fOut.destination,
						TimeSource()->PerformanceTimeFor(system_time()));
			last_result = true;
		}

		i %= ISOC_BUFFER_COUNT;

		for (j=0;j<ISOC_BUFFER_COUNT/2;j++,i++) {
			if (	(fIsocHeader[i].status != 8) &&
					(fIsocHeader[i].status != 488)) {
				printf("%Lx: Bad status (%lx)\n",
						fIsocHeader[i].buffer_num, fIsocHeader[i].status);
				continue;
			}

			packet = (uint32 *)((uchar *)fIsocBuffer + i * ISOC_PACKET_SIZE);

			x = B_BENDIAN_TO_HOST_INT32(packet[0]);
			y = B_BENDIAN_TO_HOST_INT32(packet[1]);
		
			#define DBS ((x >> 16) & 0xff)
			if (	(x & 0xc0000000) || ((y & 0xe0000000) != 0x80000000) ||
					(DBS != 120)) {
				printf("bad header: %lx %lx\n", x, y);
				continue;
			}

			if (fIsocHeader[i].status != 488)
				continue;

			uchar *h = (uchar *)(packet + 2);
			if (!(h[0] >> 5) && !(h[1] >> 3)) { /* DIF block 0? */
				if (framenum++ == 0)
					;
				else if (framepos < DV_ENCODED_FRAME_SIZE)
					printf("underflow (frame %ld, %ld)\n", framenum, framepos);
				else if (framepos > DV_ENCODED_FRAME_SIZE)
					printf("overflow (frame %ld, %ld)\n", framenum, framepos);
				else if (framepos == DV_ENCODED_FRAME_SIZE) {
					if (buffer) {
						media_header *h = buffer->Header();
						h->type = B_MEDIA_ENCODED_VIDEO;
						h->time_source = TimeSource()->ID();
						h->size_used = DV_ENCODED_FRAME_SIZE;
						h->start_time = TimeSource()->PerformanceTimeFor(
								system_time() + EventLatency() +
								SchedulingLatency());
						h->file_pos = 0;
						h->orig_size = 0;
						h->data_offset = 0;
						h->u.encoded_video.field_flags = B_MEDIA_KEY_FRAME;
						h->u.encoded_video.forward_history = 0;
						h->u.encoded_video.backward_history = 0;
						h->u.encoded_video.unused_mask = 0;
						h->u.encoded_video.field_gamma = 0;
						h->u.encoded_video.field_sequence = fFieldCount++;
						h->u.encoded_video.field_number = 0;
						h->u.encoded_video.first_active_line = 0;
						h->u.encoded_video.line_count = 480;

						if (!fRunning || !fEnabled ||
								(SendBuffer(buffer, fOut.destination) < B_OK))
							buffer->Recycle();
					}

					buffer = fBufferGroup->RequestBuffer(
									DV_ENCODED_FRAME_SIZE, 0LL);
					frame = buffer ? (uchar *)buffer->Data() : NULL;

					if (!buffer)
						PRINTF(1, ("RequestBuffer failed\n"));
				}
				framepos = 0;
			}

			if (frame && (framepos + 480 <= DV_ENCODED_FRAME_SIZE))
				memcpy(frame + framepos, packet + 2, 480);
			framepos += 480;
		}
	}

	if (buffer) buffer->Recycle();

	return B_OK;
}

BMediaAddOn *
DVBufferProducer::AddOn(int32 *internal_id) const
{
	if (internal_id)
		*internal_id = fInternalID;
	return fAddOn;
}

status_t 
DVBufferProducer::HandleMessage(int32 message, const void *data, size_t size)
{
	TOUCH(message); TOUCH(data); TOUCH(size);

	return ENOSYS;
}

void 
DVBufferProducer::Preroll()
{
}

void 
DVBufferProducer::NodeRegistered()
{
	if (fInitStatus != B_OK) {
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}

	{
		BParameterWeb *web = new BParameterWeb();
		BParameterGroup *main = web->MakeGroup(Name());
		BDiscreteParameter *state = main->MakeDiscreteParameter(
										P_TRANSPORT_STATE,
										B_MEDIA_ENCODED_VIDEO, // ???
										"Transport State",
										"SimpleTransport");
										//B_SIMPLE_TRANSPORT);
		state->AddItem(T_REWIND, "Rewind");
		state->AddItem(T_FRAME_REWIND, "Frame Rewind");
		state->AddItem(T_STOP, "Stop");
		state->AddItem(T_PLAY, "Play");
		state->AddItem(T_PAUSE, "Pause");
		state->AddItem(T_FRAME_FORWARD, "Frame Forward");
		state->AddItem(T_FAST_FORWARD, "Fast Forward");
		
		fLastStateChange = system_time();
		CurrentTransportState();

		SetParameterWeb(web);
	}

	fOut.node = Node();
	fOut.source.port = ControlPort();
	fOut.source.id = 0;
	fOut.destination = media_destination::null;
    fOut.format.type = B_MEDIA_ENCODED_VIDEO;
	fOut.format.u.encoded_video = fPAL ? dv_pal_format : dv_ntsc_format;
	strcpy(fOut.name, Name());	

//	SetPriority(B_DISPLAY_PRIORITY);
	Run();
}

void 
DVBufferProducer::HandleEvent(const media_timed_event *event,
		bigtime_t lateness, bool realTimeEvent)
{
	TOUCH(lateness); TOUCH(realTimeEvent);

	switch(event->type)
	{
		case BTimedEventQueue::B_START:
			HandleStart();
			break;
		case BTimedEventQueue::B_STOP:
			HandleStop();
			break;

		case BTimedEventQueue::B_SEEK:
		case BTimedEventQueue::B_PARAMETER:
		case BTimedEventQueue::B_WARP:
		case BTimedEventQueue::B_HANDLE_BUFFER:
		case BTimedEventQueue::B_DATA_STATUS:
		default:
			break;
	}
}

void 
DVBufferProducer::CleanUpEvent(const media_timed_event *event)
{
	/* nothing is required */

	TOUCH(event);
}

status_t 
DVBufferProducer::FormatSuggestionRequested(
		media_type type, int32 quality, media_format *format)
{
	if (type != B_MEDIA_ENCODED_VIDEO)
		return B_MEDIA_BAD_FORMAT;

	TOUCH(quality);

	*format = fOut.format;
	return B_OK;
}

status_t 
DVBufferProducer::FormatProposal(const media_source &output, media_format *format)
{
	status_t err;

	if (!format)
		return B_BAD_VALUE;

	if (output != fOut.source)
		return B_MEDIA_BAD_SOURCE;
	
	err = format_is_compatible(*format, fOut.format) ?
			B_OK : B_MEDIA_BAD_FORMAT;
	*format = fOut.format;
	return err;
		
}

status_t 
DVBufferProducer::FormatChangeRequested(const media_source &source,
		const media_destination &destination, media_format *io_format,
		int32 *_deprecated_)
{
	TOUCH(destination); TOUCH(io_format); TOUCH(_deprecated_);
	if (source != fOut.source)
		return B_MEDIA_BAD_SOURCE;
		
	// we probably don't support any format changes
	return B_ERROR;	
}

status_t 
DVBufferProducer::GetNextOutput(int32 *cookie, media_output *out_output)
{
	if (!out_output)
		return B_BAD_VALUE;

	if ((*cookie) != 0)
		return B_BAD_INDEX;
	
	*out_output = fOut;
	(*cookie)++;
	return B_OK;
}

status_t 
DVBufferProducer::DisposeOutputCookie(int32 cookie)
{
	TOUCH(cookie);

	return B_OK;
}

status_t 
DVBufferProducer::SetBufferGroup(const media_source &for_source,
		BBufferGroup *group)
{
	TOUCH(for_source); TOUCH(group);

	return B_ERROR;
}

status_t 
DVBufferProducer::VideoClippingChanged(const media_source &for_source,
		int16 num_shorts, int16 *clip_data,
		const media_video_display_info &display, int32 *_deprecated_)
{
	TOUCH(for_source); TOUCH(num_shorts); TOUCH(clip_data);
	TOUCH(display); TOUCH(_deprecated_);

	return B_ERROR;
}

status_t 
DVBufferProducer::GetLatency(bigtime_t *out_latency)
{
	*out_latency = EventLatency() + SchedulingLatency();
	return B_OK;
}

status_t 
DVBufferProducer::PrepareToConnect(const media_source &source,
		const media_destination &destination, media_format *format,
		media_source *out_source, char *out_name)
{
	PRINTF(0, ("(%s)\n", Name()));

	if (fConnected) {
		PRINTF(-1, ("Already connected\n"));
		return EALREADY;
	}

	if (source != fOut.source)
		return B_MEDIA_BAD_SOURCE;
	
	if (destination == media_destination::null) {
		PRINTF(-1, ("Trying to connect to media_destination::null\n"));
		return B_MEDIA_BAD_DESTINATION;
	}
		
	if (!format_is_compatible(*format, fOut.format)) {
		*format = fOut.format;
		return B_MEDIA_BAD_FORMAT;
	}

	// reserve the connection
	fOut.destination = destination;

	// in case this is not already the case
	*format = fOut.format;
	*out_source = fOut.source;
	strcpy(out_name, fOut.name);

	return B_OK;
}

void 
DVBufferProducer::Connect(status_t error, const media_source &source,
		const media_destination &destination, const media_format &format,
		char *io_name)
{
	PRINTF(0, ("(%s)\n", Name()));

	if (fConnected) {
		PRINTF(-1, ("Already connected\n"));
		return;
	}

	if (	(source != fOut.source) || (error < B_OK) ||
			!const_cast<media_format &>(format).Matches(&fOut.format)) {
		PRINTF(0, ("Connect error\n"));
		return;
	}

	if (destination == media_destination::null) {
		PRINTF(-1, ("Trying to connect to media_destination::null\n"));
		return;
	}

	fOut.destination = destination;
	strcpy(io_name, fOut.name);
	
	// get the latency
	bigtime_t latency = 0;
	media_node_id tsID = 0;
	FindLatencyFor(fOut.destination, &latency, &tsID);
	SetEventLatency(latency + NODE_LATENCY);

	if (fRunning) {
		status_t err = RealStart();
		if (err < B_OK)
			return;
	}

	fConnected = true;
	fEnabled = true;
}

void 
DVBufferProducer::Disconnect(const media_source &source,
		const media_destination &destination)
{
	PRINTF(0, ("(%s)\n", Name()));

	if (!fConnected) {
		PRINTF(-1, ("Not connected\n"));
		return;
	}

	if ((source != fOut.source) || (destination != fOut.destination)) {
		PRINTF(0, ("Bad source or destination\n"));
		return;
	}

	fOut.destination = media_destination::null;

	fConnected = false;
	fEnabled = false;

	if (fRunning)
		RealStop();
}

void 
DVBufferProducer::LateNoticeReceived(const media_source &source,
		bigtime_t how_much, bigtime_t performance_time)
{
	TOUCH(source); TOUCH(how_much); TOUCH(performance_time);
}

void 
DVBufferProducer::EnableOutput(const media_source &source, bool enabled,
		int32 *_deprecated_)
{
	TOUCH(_deprecated_);

	if (source != fOut.source)
		return;

	fEnabled = enabled;
}

status_t 
DVBufferProducer::SetPlayRate(int32 numer, int32 denom)
{
	TOUCH(numer); TOUCH(denom);

	return B_ERROR;
}

void 
DVBufferProducer::AdditionalBufferRequested(const media_source &source,
		media_buffer_id prev_buffer, bigtime_t prev_time,
		const media_seek_tag *prev_tag)
{
	TOUCH(source); TOUCH(prev_buffer); TOUCH(prev_time); TOUCH(prev_tag);
}

void 
DVBufferProducer::LatencyChanged(const media_source &source,
		const media_destination &destination, bigtime_t new_latency,
		uint32 flags)
{
	TOUCH(source); TOUCH(destination); TOUCH(new_latency); TOUCH(flags);
}

status_t 
DVBufferProducer::GetParameterValue(
	int32 id, bigtime_t *last_change, void *value, size_t *size)
{
	if (id != P_TRANSPORT_STATE)
		return B_BAD_VALUE;

	*last_change = fLastStateChange;
	*size = sizeof(int32);
	CurrentTransportState();
	*((int32 *)value) = fTransportState;

	return B_OK;
}

// XXX: respect when field
void
DVBufferProducer::SetParameterValue(
	int32 id, bigtime_t when, const void *value, size_t size)
{
	if ((id != P_TRANSPORT_STATE) || !value || (size != sizeof(int32)))
		return;

	SwitchTransportState(*(int32 *)value, when);
}

void
DVBufferProducer::CurrentTransportState()
{
	status_t err = B_ERROR; /* silence bad warning */
	uchar mode, state;
	struct avc_subunit vcr;

	if (!fPresent) {
		fTransportState = T_NO_DEVICE;
		fTransportMode = VCR_WIND;		// ???
		return;
	}

	vcr.fd = fDriver;
	vcr.guid = fGUID;
	vcr.type = AVC_TAPE_PLAYER_RECORDER;
	vcr.id = 0;

	for (int32 retry=0;retry<5;retry++) {
		err = vcr_transport_state(&vcr, &mode, &state, 1000000);
		if (err >= B_OK)
			break;
	}
	
//	printf("CurrentTransportState: AV/C mode=0x%x, state=0x%x\n", mode, state);
	
	if (err < B_OK) {
		fTransportState = T_STOP;
		fTransportMode = VCR_WIND;
		return;  // ???
	}

	if (mode == VCR_WIND) {
		fTransportMode = VCR_WIND;
		fTransportState = T_STOP;

		if (state == VCR_WIND_STOP)
			fTransportState = T_STOP;
		else if (	(state == VCR_WIND_HIGH_SPEED_REWIND) ||
					(state == VCR_WIND_REWIND))
			fTransportState = T_REWIND;
		else if (state == VCR_WIND_FAST_FORWARD)
			fTransportState = T_FAST_FORWARD;

		return;
	} else if (mode == VCR_PLAY) {
		fTransportMode = VCR_PLAY;

		if (state == VCR_PLAY_NEXT_FRAME)
			fTransportState = T_FRAME_FORWARD;
		else if (	(state >= VCR_PLAY_SLOWEST_FORWARD) &&
					(state <= VCR_PLAY_SLOW_FORWARD_1))
			fTransportState = T_FAST_FORWARD;
		else if (	(state >= VCR_PLAY_FAST_FORWARD_1) &&
					(state <= VCR_PLAY_FASTEST_FORWARD))
			fTransportState = T_FAST_FORWARD;
		else if (state == VCR_PLAY_PREVIOUS_FRAME)
			fTransportState = T_FRAME_REWIND;
		else if (	(state >= VCR_PLAY_SLOWEST_REVERSE) &&
					(state <= VCR_PLAY_REVERSE))
			fTransportState = T_REWIND;
		else if (	(state == VCR_PLAY_REVERSE_PAUSE) ||
					(state == VCR_PLAY_FORWARD_PAUSE))
			fTransportState = T_PAUSE;
		else
			fTransportState = T_PLAY;

		return;
	}
	
	fTransportState = T_STOP;
	fTransportMode = VCR_WIND;
	return;  // ???
}

void 
DVBufferProducer::SwitchTransportState(int32 new_state, bigtime_t when)
{
	struct avc_subunit vcr;

	if (!fPresent) {
		new_state = T_NO_DEVICE;
		fTransportMode = VCR_WIND;
		goto done;
	}

	CurrentTransportState();

	vcr.fd = fDriver;
	vcr.guid = fGUID;
	vcr.type = AVC_TAPE_PLAYER_RECORDER;
	vcr.id = 0;

	// no change
	if (new_state == fTransportState)
		goto done;

	switch(new_state) {
		case T_REWIND:
			if (fTransportMode == VCR_PLAY)
				vcr_play(&vcr, VCR_PLAY_FASTEST_REVERSE, DEFAULT_AVC_TIMEOUT);
			else
				vcr_wind(&vcr, VCR_WIND_REWIND, DEFAULT_AVC_TIMEOUT);
			break;

		case T_STOP:
			fTransportMode = VCR_WIND;
			vcr_wind(&vcr, VCR_WIND_STOP, DEFAULT_AVC_TIMEOUT);
			break;

		case T_PLAY:
			fTransportMode = VCR_PLAY;
			vcr_play(&vcr, VCR_PLAY_FORWARD, DEFAULT_AVC_TIMEOUT);
			break;

		case T_PAUSE:
			fTransportMode = VCR_PLAY;
			vcr_play(&vcr, VCR_PLAY_FORWARD_PAUSE, DEFAULT_AVC_TIMEOUT);
			break;

		case T_FAST_FORWARD:
			if (fTransportMode == VCR_PLAY)
				vcr_play(&vcr, VCR_PLAY_FASTEST_FORWARD, DEFAULT_AVC_TIMEOUT);
			else
				vcr_wind(&vcr, VCR_WIND_FAST_FORWARD, DEFAULT_AVC_TIMEOUT);
			break;

		case T_FRAME_FORWARD:
			if (fTransportState == T_PAUSE)
				vcr_play(&vcr, VCR_PLAY_NEXT_FRAME, DEFAULT_AVC_TIMEOUT);
			else
				vcr_play(&vcr, VCR_PLAY_FORWARD_PAUSE, DEFAULT_AVC_TIMEOUT);
			fTransportMode = VCR_PLAY;
			new_state = T_PAUSE;
			break;

		case T_FRAME_REWIND:
			if (fTransportState == T_PAUSE)
				vcr_play(&vcr, VCR_PLAY_PREVIOUS_FRAME, DEFAULT_AVC_TIMEOUT);
			else
				vcr_play(&vcr, VCR_PLAY_FORWARD_PAUSE, DEFAULT_AVC_TIMEOUT);
			fTransportMode = VCR_PLAY;
			new_state = T_PAUSE;
			break;
	}

done:
	// update our internal state
	fLastStateChange = when;
	fTransportState = new_state;

	// tell everyone else
	BroadcastNewParameterValue(fLastStateChange, P_TRANSPORT_STATE,
				&fTransportState, sizeof(fTransportState));				
}

static status_t
isoc_read_request(int fd, int32 port, uchar channel, int32 buffers,
		uint32 len, void *base, struct ieee1394_iio_hdr *hdr,
		sem_id sem)
{
	struct ieee1394_iio iio;
	struct ieee1394_iio_buffer b[ISOC_BUFFER_COUNT];
	struct iovec vec[ISOC_BUFFER_COUNT];
	int i;

	ASSERT(buffers <= ISOC_BUFFER_COUNT);

	iio.port = port;
	iio.flags = 0;
	iio.channel = channel;
	iio.num_buffers = buffers;
	iio.buffers = &b[0];

	for (i=0;i<buffers;i++) {
		b[i].hdr = &hdr[i];
		b[i].veclen = 1;
		b[i].vec = &vec[i];
		b[i].tagmask = B_1394_ISOCHRONOUS_TAG_01;
		b[i].start_event.type = B_1394_ISOC_EVENT_NOW;
		b[i].start_event.cookie = 0;
		if (((i + 1) % (buffers/2)) == 0)
			b[i].sem = sem;
		else
			b[i].sem = -1;

		vec[i].iov_len = len;
		vec[i].iov_base = (uchar *)base + i * len;
	}

	return ioctl(fd, B_1394_QUEUE_ISOCHRONOUS, &iio, sizeof(iio));
}

static status_t
isoc_read_cancel(int fd, uint32 port)
{
	struct ieee1394_istop is;
	is.port = port;
	is.event.type = B_1394_ISOC_EVENT_NOW;
	return ioctl(fd, B_1394_STOP_ISOCHRONOUS, &is);
}
