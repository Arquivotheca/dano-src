
#include "Bt848Controllable.h"

#include "bt848addon.h"
#include "VideoDefs.h"
#include "Bt848Source.h"
#include "AudioMux.h"
#include "VideoMux.h"
#include "ClipList.h"
#include "VideoImage.h"
#include "VideoConversions.h"
#include "VideoSource.h"
#include "VideoDefs.h"
#include "bt848_driver.h"

#include <Errors.h>
#include <OS.h>
#include <StopWatch.h>
#include <scheduler.h>

#include <Buffer.h>
#include <BufferGroup.h>
#include <BufferProducer.h>
#include <MediaTheme.h>
#include <ParameterWeb.h>
#include <TimeSource.h>
#include <MediaDefs.h>

#include <assert.h>
#include <malloc.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "TrackerSettings.h"

#define DO_NOTHING(x...)

#if DEBUG
#define PRINTF printf
#define DEBUGARG(ident) ident
#define DEBUGVAR(ident, initexpr) ident=initexpr
#else
#define PRINTF DO_NOTHING
#define DEBUGARG(ident)
#define DEBUGVAR(ident, initexpr) (void)initexpr
#endif

#define FUNCTION	DO_NOTHING //PRINTF
#define ERROR		PRINTF
#define PROGRESS	PRINTF
#define PULSE		DO_NOTHING //PRINTF
#define LOOP		DO_NOTHING //PRINTF
			
#define DEFAULT_TIMEOUT 200000L

#define MAX_CLIP_LINE 80

#define M1 ((double)1000000.0)

#if 0
static uint16	msp_read(BI2CBus *bus, uint16 device, uint16 address);
static void		PrintVideoDisplay(const struct media_video_display_info &display);
#endif

#if DEBUG
static void		PrintVideoFormat(const media_raw_video_format aFormat);
#endif

static uint32	bitsPerPixel(uint32 cSpace);
static void		msp_reset(BI2CBus *bus);
static void		msp_write(BI2CBus *bus, uint16 device, uint16 address, uint data);
static void		msp_setmode(BI2CBus *bus, uint16 type);
static void		msp_setvolume(BI2CBus *bus, int32 left, int32 right);
static void		reset_hitachi(Bt848Source *device);
static void		write_hitachi(Bt848Source *device,  uint8 address, uint8 data);
static uint8	read_hitachi(Bt848Source *device,  uint8 address);

#define MSP		0x80
#define MSP_DEM	0x10
#define MSP_DFP	0x12

const char *kVideoFormat[] = {
	"Unknown",
	"NTSC-M",
	"NTSC-J",
	"PAL-BDGHI",
	"PAL-M ",
	"PAL-N",
	"SECAM",
	0
};

const char *kTunerLocale[] = {
	"Unknown",				//0
	"Australia Air",		//1
	"Brazil Air",			//2
	"China Air",			//3
	"Europe Air",			//4
	"Europe Cable",			//5
	"France Air",			//6
	"France Cable",			//7
	"Great Britain Air",	//8
	"Great Britain Cable",	//9
	"Japan Air",			//10
	"Japan Cable",			//11
	"US Air",				//12
	"US Cable",				//13
	"US Cable HRC",			//14
	"FM Radio",				//15
	0
};

const char *kTunerBrand[] = {
	"No Tuner",
	"Alps",
	"Panasonic",
	"Philips",
	"Temic",
	0
};

const char *kVideoSourceName[] = {
	"None",
	"Composite 1",
	"Composite 2",
	"Composite 3",
	"Composite 4",
	"Tuner",
	"SVideo",
	"Camera",
	0
};

const char *kAudioSourceName[] = {
	"None",
	"Mute",
	"Tuner",
	"External Jack",
	"Internal Jack",
	"Radio",
	"Microphone",
	0
};

const char *kAudioMode[] = {
	"Main",
	"Second",
	"Both",
	0
};

const char *kMspMode[] = {
	"US NTSC FM",
	"PAL B/G FM",
	"PAL B/G NICAM",
	"PAL I NICAM",
	"FM RADIO",
	"Satellite FM Mono",
	"AM (type 1)",
	"AM (type 2)",
	0
};

#define MSP_MODE_NTSC_FM		0
#define MSP_MODE_PAL_BG_FM		1
#define MSP_MODE_PAL_BG_NICAM	2
#define MSP_MODE_PAL_I_NICAM	3
#define MSP_MODE_FM_RADIO		4
#define MSP_MODE_SAT_FM_MONO	5
#define MSP_MODE_AM_A			6
#define MSP_MODE_AM_B			7

const char *kImageSize[] = {
	"768x576",
	"720x576",
	"720x480",
	"640x480",
	"352x240",
	"320x240",
	"160x120",
	0
};

const char *kColorspace[] = {
	"8 Bits/Pixel (Gray)",
	"15 Bits/Pixel",
	"16 Bits/Pixel",
	"32 Bits/Pixel",
	0
};

static status_t
frame_ready_thread(void *data)
{
	status_t err = B_NO_ERROR;
	bt848_thread_info *ti = (bt848_thread_info *)data;
	//printf("frame_ready_thread started, port = %d\n", ti->port);
	do {
		bt848_frame_info info;
		//printf("frame_ready_thread waiting for frame\n");
		err = ti->bt848->WaitForFrame(100000, &info.index);
		if(err == B_NO_ERROR) {
			//printf("frame_ready_thread: got frame index %d\n", info.index);
			err = write_port_etc(ti->port, B_BT848_FRAME_READY, &info, sizeof(info),
			                     B_TIMEOUT, 32000);
		}
		else {
			//printf("frame_ready_thread: no frame\n");
		}
	} while((err == B_NO_ERROR || err == B_TIMED_OUT) && !ti->quit);
	//printf("frame_ready_thread done, quit = %d, err = %s\n", ti->quit, strerror(err));

	return B_NO_ERROR;
}


BBt848Controllable::BBt848Controllable(const uint32 internal_id, const char *devicename, const char *nodename, BMediaAddOn *addon) :
	BMediaNode(nodename),
	BMediaEventLooper(),
	BBufferProducer(B_MEDIA_RAW_VIDEO),
	BTimeSource(),
	fNextCaptureTime(0),
	mBufferCountF1(MAX_BUFFERS),
	mBufferCountF2(MAX_BUFFERS),
	mTuner(0),
	mAudioMux(0),
	mVideoControls(0),
	mHitachi(false),
	mAudioSource(0),
	mInitCheck(-1),
	mBufferGroupF1(0),
	mBufferGroupF2(0),
	mBuffersMappedF1(false),
	mBuffersMappedF2(false),
	mConnectionProposedF1(false),
	mConnectionProposedF2(false),
	mConnectionActiveF1(false),
	mConnectionActiveF2(false),
	mUsingConsumerBuffersF1(false),
	mUsingConsumerBuffersF2(false),
	mInternalID(internal_id),
	mAddOn(addon),
	mFrameNumber(0),
	mRate(29.97),
	mCaptureQuit(false),
	mMutedF1(false),
	mMutedF2(false),
	mMediaTime(0),
	mDownstreamLatency(20000),
	mHasTvFavorites(false),
	mHasFmFavorites(false),
	mTvFavoritesIndex(0),
	mFmFavoritesIndex(0),
	settings(NULL)
{
	FUNCTION("BBt848Controllable - BEGIN\n");
	
	mFrameReadyThread = -1;

	SetPriority(B_URGENT_DISPLAY_PRIORITY);
	
	for (int i = 0; i < B_VIDEO_V_MAX; i++)
		mClipListF1[i] = NULL;
		
	for (int i = 0; i < B_VIDEO_V_MAX; i++)
		mClipListF2[i] = NULL;

	// Let the system know we do physical input
	AddNodeKind(B_PHYSICAL_INPUT);

	// create the Bt848Source
	mDevice = 0;
	if (!devicename) {
		mName = strdup("/dev/video/bt848/0");
		mDevice = new Bt848Source("/dev/video/bt848/0");
	}
	else {
		mName = strdup(devicename);
		PROGRESS("BBt848Controllable - Creating %s\n", mName);
		mDevice	= new Bt848Source(devicename);
	}
	if(mDevice == NULL)
		return;
	mInitCheck = mDevice->InitCheck();
	if(mInitCheck != B_NO_ERROR) {
		ERROR("BBt848Controllable::NodeRegistered - Couldn't create Bt848Source object!!\n");
		goto err1;
	}

	mFrameReadyThreadInfo.port = ControlPort();
	mFrameReadyThreadInfo.bt848 = mDevice;
	mFrameReadyThreadInfo.quit = false;
	mFrameReadyThread = spawn_thread((thread_func)frame_ready_thread,
	                                 "frame ready thread",
	                                 B_URGENT_DISPLAY_PRIORITY, &mFrameReadyThreadInfo);

	if(mFrameReadyThread < 0) {
		mInitCheck = mFrameReadyThread;
		goto err1;
	}
	
	resume_thread(mFrameReadyThread);
	return;

err1:
	delete mDevice;
	mDevice = NULL;
	return;
}

BBt848Controllable::~BBt848Controllable()
{
	FUNCTION("~BBt848Controllable - BEGIN\n");

	if (mInitCheck == B_OK) {
		StopCapture();
		
		QuitSettings();
	
		// close the bt848 video source
	
		if(mFrameReadyThread >= 0) {
			mFrameReadyThreadInfo.quit = true;
			status_t dummy;
			wait_for_thread(mFrameReadyThread, &dummy);
		}
	
		delete mDevice;

		Uninitialize();

		Quit();
	}

	FUNCTION("~BBt848Controllable - END\n");
}

BMediaAddOn *
BBt848Controllable::AddOn(long *cookie) const
{
	FUNCTION("BBt848Controllable::AddOn\n");
	*cookie = mInternalID;
	return mAddOn;
}

void 
BBt848Controllable::Start(bigtime_t performanceTime)
{
	FUNCTION("BBt848Controllable::Start @ %Ld\n", performanceTime);
	BMediaEventLooper::Start(performanceTime);
}

void 
BBt848Controllable::Stop(bigtime_t performanceTime, bool immediate)
{
//	Calling TimeSource() from the destructor is bad
//	FUNCTION("BBt848Controllable::Stop @ %Ld\n", immediate ? TimeSource()->Now() : performanceTime);
	BMediaEventLooper::Stop(performanceTime, immediate);
}

void 
BBt848Controllable::Preroll()
{
	/* DO NOTHING */
}


//void 
//BBt848Controllable::SetTimeSource(BTimeSource *time_source)
//{
//}

status_t 
BBt848Controllable::HandleMessage(int32 message, const void * data, size_t /*size*/)
{
	FUNCTION("BBt848Controllable::HandleMessage - BEGIN\n");
	status_t status = B_OK;
	bt848_msg_info info;
	
	switch (message)
	{
		case B_BT848_FRAME_READY: {
			uint32 whichIndex = ((bt848_frame_info *)data)->index;
			//printf("B_BT848_FRAME_READY index %d\n", whichIndex);
			BVideoImage *image = 0;
			image = mDevice->GetFrame(whichIndex);
			if(image && image->FrameNumber() != mFrameNumber)
				SendBuffers(image, whichIndex);
			} break;
	
		case B_BT848_CHANNEL_UP:
			if (mTuner)
			{
				StopCapture();
				info.index = mTuner->NextChannel();
				RestartCapture();
				info.frequency = mTuner->CurrentFrequency();
				info.isLocked = mTuner->TunerLocked();
				info.videoPresent = ((Bt848Source *)mDevice)->VideoPresent();
				write_port(((bt848_msg_info *)data)->port, message, (void *)&info, sizeof(bt848_msg_info)); 
			}
			break;
		case B_BT848_CHANNEL_DOWN:
			if (mTuner)
			{
				StopCapture();
				info.index = mTuner->PreviousChannel();
				RestartCapture();
				info.frequency = mTuner->CurrentFrequency();
				info.isLocked = mTuner->TunerLocked();
				info.videoPresent = ((Bt848Source *)mDevice)->VideoPresent();
				write_port(((bt848_msg_info *)data)->port, message, (void *)&info, sizeof(bt848_msg_info)); 
			}
			break;
		case B_BT848_SCAN_UP:
			if (mTuner)
			{
				StopCapture();
				info.index = mTuner->ScanUp();
				RestartCapture();
				info.frequency = mTuner->CurrentFrequency();
				info.isLocked = mTuner->TunerLocked();
				info.videoPresent = ((Bt848Source *)mDevice)->VideoPresent();
				write_port(((bt848_msg_info *)data)->port, message, (void *)&info, sizeof(bt848_msg_info)); 
			}
			break;
		case B_BT848_SCAN_DOWN:
			if (mTuner)
			{
				StopCapture();
				info.index = mTuner->ScanDown();
				RestartCapture();
				info.frequency = mTuner->CurrentFrequency();
				info.isLocked = mTuner->TunerLocked();
				info.videoPresent = ((Bt848Source *)mDevice)->VideoPresent();
				write_port(((bt848_msg_info *)data)->port, message, (void *)&info, sizeof(bt848_msg_info)); 
			}
			break;
		case B_BT848_FINE_TUNE_UP:
			if (mTuner)
			{
				info.frequency = mTuner->FineTuneUp();
				info.index = mTuner->CurrentIndex();
				info.isLocked = mTuner->TunerLocked();
				info.videoPresent = ((Bt848Source *)mDevice)->VideoPresent();
				write_port(((bt848_msg_info *)data)->port, message, (void *)&info, sizeof(bt848_msg_info)); 
			}
			break;
		case B_BT848_FINE_TUNE_DOWN:
			if (mTuner)
			{
				info.frequency = mTuner->FineTuneDown();
				info.index = mTuner->CurrentIndex();
				info.isLocked = mTuner->TunerLocked();
				info.videoPresent = ((Bt848Source *)mDevice)->VideoPresent();
				write_port(((bt848_msg_info *)data)->port, message, (void *)&info, sizeof(bt848_msg_info)); 
			}
			break;
		case B_BT848_TUNE_FREQUENCY:
			if (mTuner)
			{
				StopCapture();
				mTuner->Tune(((bt848_msg_info *)data)->frequency);
				RestartCapture();
				info.isLocked = mTuner->TunerLocked();
				info.videoPresent = ((Bt848Source *)mDevice)->VideoPresent();
				write_port(((bt848_msg_info *)data)->port, message, (void *)&info, sizeof(bt848_msg_info)); 
			}
			break;
		case B_BT848_TUNE_INDEX:
			if (mTuner)
			{
				StopCapture();
				mTuner->Tune(mTuner->FrequencyFor(((bt848_msg_info *)data)->index));
				RestartCapture();
				info.isLocked = mTuner->TunerLocked();
				info.videoPresent = ((Bt848Source *)mDevice)->VideoPresent();
				write_port(((bt848_msg_info *)data)->port, message, (void *)&info, sizeof(bt848_msg_info)); 
			}
			break;
		default:
			status = B_ERROR;
			break;
	}

	FUNCTION("BBt848Controllable::HandleMessage - END\n");
	return status;
}

void 
BBt848Controllable::NodeRegistered()
{
	FUNCTION("BBt848Controllable::NodeRegistered %ld %p - BEGIN\n", mInternalID, (void *)this);
	
	// get settings from settings file
	// generate settings file name
	strcpy(mSettingsFile,"settings");
	mSettingsFile[8] = mName[strlen(mName)-1];
	mSettingsFile[9] = 0;
	PROGRESS("BBt848Controllable::NodeRegistered - Reading Settings file: %s\n", mSettingsFile);			
	SetUpSettings((const char *)mSettingsFile, (const char *)"Media/bt848");				

	if (mInitCheck == B_OK)
	{
		// set latency defaults
		SetEventLatency((bigtime_t)1000000LL/mRate);
			
		Initialize();		
		ConstructControlWeb();

		// tell the event looper to start up the thread!
		Run();
	}
	else
	{
		ERROR("BBt848Controllable::NodeRegistered - Couldn't open driver!!\n");
	}
}

void 
BBt848Controllable::SetRunMode(run_mode mode)
{
	BMediaEventLooper::SetRunMode(mode);
	BMediaNode::SetRunMode(mode);
}

status_t 
BBt848Controllable::DeleteHook(BMediaNode *node)
{
	return BMediaEventLooper::DeleteHook(node);
}

void 
BBt848Controllable::HandleEvent(const media_timed_event *event, bigtime_t DEBUGARG(lateness), bool realTimeEvent)
{
	LOOP("BBt848Controllable::HandleEvent: lateness: %Ld\n", lateness);

//	bigtime_t realTime = BTimeSource::RealTime();

	switch(event->type)
	{
		case BTimedEventQueue::B_START:
		{
			bigtime_t performanceTime = event->event_time;
			if (realTimeEvent)
				performanceTime = TimeSource()->PerformanceTimeFor(event->event_time);
			HandleStart(performanceTime);
			break;
		}

		case BTimedEventQueue::B_STOP:
		{
			bigtime_t performanceTime = event->event_time;
			if (realTimeEvent)
				performanceTime = TimeSource()->PerformanceTimeFor(event->event_time);
			HandleStop(performanceTime);
			break;
		}


		case BTimedEventQueue::B_SEEK:
		case BTimedEventQueue::B_WARP:
			break;
		
		default:
			break;
	}
}

status_t 
BBt848Controllable::TimeSourceOp(const time_source_op_info &op, void */*_reserved*/)
{
	FUNCTION("BBt848Controllable::TimeSourceOp: 0x%x %Ld %Ld @ %Ld\n",
		op.op, op.real_time, op.performance_time, BTimeSource::RealTime());


	switch(op.op)
	{
		case BTimeSource::B_TIMESOURCE_START:
		{
			media_timed_event event(op.real_time, BTimedEventQueue::B_START);
			RealTimeQueue()->AddEvent(event);
			break;
		}
		
		case BTimeSource::B_TIMESOURCE_STOP:
		{
			media_timed_event event(op.real_time, BTimedEventQueue::B_STOP);
			RealTimeQueue()->AddEvent(event);
			break;
		}
			
		case BTimeSource::B_TIMESOURCE_STOP_IMMEDIATELY:
		{
			media_timed_event event(0, BTimedEventQueue::B_STOP);
			DispatchEvent(&event, 0, true);
			break;
		}	
		
		case BTimeSource::B_TIMESOURCE_SEEK:
		{
			media_timed_event event(op.real_time, BTimedEventQueue::B_SEEK);
			event.bigdata = op.performance_time;
			RealTimeQueue()->AddEvent(event);
			break;
		}
		default:
			return B_ERROR;
	
	}

	return B_OK;
}

void 
BBt848Controllable::HandleStart(bigtime_t performanceTime)
{
	FUNCTION("BBt848Controllable::HandleStart: %Ld @ %Ld\n", performanceTime, BTimeSource::RealTime());
	if (RunState() == BMediaEventLooper::B_STARTED)
		return;

	mFrameNumber = 0;
	mMediaTime = 0;
	
	StartCapture();

	if (mOutputF1.destination != media_destination::null)
		SendDataStatus(B_DATA_AVAILABLE, mOutputF1.destination, performanceTime);

	if (mOutputF2.destination != media_destination::null)
		SendDataStatus(B_DATA_AVAILABLE, mOutputF2.destination, performanceTime);
	
	PublishTime(mMediaTime, TimeSource()->RealTimeFor(performanceTime, 0), 1.0);

}

void 
BBt848Controllable::HandleStop(bigtime_t performanceTime)
{
	FUNCTION("BBt848Controllable::HandleStop: %Ld @ %Ld\n", performanceTime, BTimeSource::RealTime());
	if (RunState() == BMediaEventLooper::B_STOPPED)
		return;

	// actually stop the capture
	StopCapture();
	
	if (mOutputF1.destination != media_destination::null)
		SendDataStatus(B_DATA_NOT_AVAILABLE, mOutputF1.destination, performanceTime);
		
	if (mOutputF2.destination != media_destination::null)
		SendDataStatus(B_DATA_NOT_AVAILABLE, mOutputF2.destination, performanceTime);
		
	PublishTime(mMediaTime, TimeSource()->RealTimeFor(performanceTime, 0), 0);
}

void 
BBt848Controllable::SendBuffers(BVideoImage *image, uint32 whichIndex)
{
	//bigtime_t t1 = system_time();
	status_t err;
	mFrameNumber++;
	if (image->FrameNumber() != mFrameNumber)
	{
		// Trouble here! We haven't managed to keep up with
		// the capture.  Fix our local frame number with
		// the real value from the driver
		ERROR("Frame# mismatch: %ld %ld\n", image->FrameNumber(), mFrameNumber);
		mFrameNumber = image->FrameNumber();
	}

	if ( mConnectionActiveF1 && !mMutedF1) 
	{
		BBuffer *buf = bufferF1[whichIndex];
	//printf("Requesting F1 buffer #%d @ %08x\n", whichIndex, buf);
		LOOP("Requesting F1 buffer #%p\n", buf);
		err = mBufferGroupF1->RequestBuffer(buf, 0);
		if (err == B_OK)
		{
			LOOP("Received F1 buffer #%p size %ld x %ld\n",
				buf, mOutputF1.format.u.raw_video.display.bytes_per_row, mOutputF1.format.u.raw_video.display.line_count);

			// Filling in the critical buffer information
			buf->Header()->type = B_MEDIA_RAW_VIDEO;
			buf->Header()->size_used = 	mOutputF1.format.u.raw_video.display.bytes_per_row *
										mOutputF1.format.u.raw_video.display.line_count;
			buf->Header()->u.raw_video.field_sequence = mFrameNumber;
			buf->Header()->u.raw_video.field_number = 0;
			buf->Header()->u.raw_video.pulldown_number = 0;
			buf->Header()->u.raw_video.first_active_line = 0;
			buf->Header()->u.raw_video.line_count = mOutputF1.format.u.raw_video.display.line_count;
			buf->Header()->start_time = TimeSource()->PerformanceTimeFor(image->Timestamp());
			
					
			// Send the buffer		
			LOOP("Sending buffer %p\n", buf);
			status_t status;
			if ((status=SendBuffer(buf, mOutputF1.destination)) != B_OK)
			{
				// If SendBuffer fails,we need to recycle it
				ERROR("BBt848Controllable::CaptureRun - SENDBUFFER FAILED 0x%lx\n", status);
				buf->Recycle();
			}
		}
		else
		{
			ERROR("BBt848Controllable::CaptureRun - COULDN'T ACQUIRE F1 BUFFER #%ld, %s\n",
		           whichIndex, strerror(err));
		}
	}


	if ( mConnectionActiveF2 && !mMutedF2 )
	{
		BBuffer *buf = bufferF2[whichIndex];
		LOOP("Requesting F2 buffer #%p\n", buf);
		err = mBufferGroupF2->RequestBuffer(buf, 0);
		if (err == B_OK)
		{
			LOOP("Received F2 buffer #%p size %ld x %ld\n", buf, mOutputF2.format.u.raw_video.display.bytes_per_row, mOutputF2.format.u.raw_video.display.line_count);

			// Filling in the critical buffer information
			buf->Header()->type = B_MEDIA_RAW_VIDEO;
			buf->Header()->size_used = 	mOutputF2.format.u.raw_video.display.bytes_per_row *
										mOutputF2.format.u.raw_video.display.line_count;
			buf->Header()->u.raw_video.field_sequence = mFrameNumber;
			buf->Header()->u.raw_video.field_number = 1;
			buf->Header()->u.raw_video.pulldown_number = 0;
			buf->Header()->u.raw_video.first_active_line = 0;
			buf->Header()->u.raw_video.line_count = mOutputF2.format.u.raw_video.display.line_count;
			
			buf->Header()->start_time = TimeSource()->PerformanceTimeFor(image->Timestamp() + (uint32)500000./mRate);
					
			// Send the buffer		
			LOOP("Sending F2 buffer %p\n", buf);
			status_t status;
			if ((status=SendBuffer(buf, mOutputF2.destination)) != B_OK)
			{
				// If SendBuffer fails,we need to recycle it
				ERROR("BBt848Controllable::CaptureRun - SENDBUFFER F2 FAILED 0x%lx\n", status);
				buf->Recycle();
			}

		}
		else
		{
			ERROR("BBt848Controllable::CaptureRun - COULDN'T ACQUIRE F2 BUFFER #%ld, %s\n",
		           whichIndex, strerror(err));
		}
	}
	
	mMediaTime = (bigtime_t)((mFrameNumber * 1000000LL)/mRate);
	PublishTime(mMediaTime, image->Timestamp(), 1.0);
	//bigtime_t t2 = system_time();
	//printf("SendBuffers took %Ld us\n", t2-t1);
}


void 
BBt848Controllable::StartCapture()
{
	if (mDevice)
	{
		if (mConnectionActiveF1 && !mMutedF1 || mConnectionActiveF2 && !mMutedF2 )
		{
			mDevice->StartCapture();
		}
	}
}

void 
BBt848Controllable::RestartCapture(bool resetFrameCounter)
{
	if (mDevice && RunState() == B_STARTED)
	{
		if (mConnectionActiveF1 && !mMutedF1 || mConnectionActiveF2 && !mMutedF2)
		{
			mDevice->StartCapture(resetFrameCounter);
		}
	}
}

void 
BBt848Controllable::SwitchCapture()
{
	if(mDevice && RunState() == B_STARTED &&
	   (mConnectionActiveF1 && !mMutedF1 || mConnectionActiveF2 && !mMutedF2)) {
		uint32 index;
		if(mDevice->SwitchCapture(&index) != B_NO_ERROR) {
			ERROR("BBt848Controllable::SwitchCapture - switch failed, force\n");
			mDevice->StopCapture();
			mDevice->StartCapture(false);
		}
		else {
			BVideoImage *image;
			image = mDevice->GetFrame(index);
			if(image && image->Status() != (status_t)BT848_CAPTURE_TIMEOUT) {
				SendBuffers(image, index);
			}
		}
	}
}

void 
BBt848Controllable::StopCapture()
{
	if (mDevice && RunState() == B_STARTED)
	{
		bool activeF1 = mConnectionActiveF1 && !mMutedF1;
		bool activeF2 = mConnectionActiveF2 && !mMutedF2;
		if(activeF1 || activeF2) {
			uint32 index;
			BVideoImage *image;
			image = mDevice->NextFrameWithTimeout(65000, &index);
			if(image && image->Status() != (status_t)BT848_CAPTURE_TIMEOUT) {
				SendBuffers(image, index);
			}
		}
		mDevice->StopCapture();
	}
}

status_t 
BBt848Controllable::ReconfigureCapture()
{
	status_t err = B_NO_ERROR;
	if(!mDevice)
		return B_ERROR;

	bool activeF1 = mConnectionActiveF1 && !mMutedF1;
	bool activeF2 = mConnectionActiveF2 && !mMutedF2;

	//printf("mBufferCountF1 %d mBufferCountF2 %d\n", mBufferCountF1, mBufferCountF2);
	//printf("activeF1 %d activeF2 %d\n", activeF1, activeF2);

	if(activeF1 && !activeF2)
		err = mDevice->ConfigureCapture(mRingBufferF1, mActiveClipListF1, mBufferCountF1, NULL);
	else if(!activeF1 && activeF2)
		err = mDevice->ConfigureCapture(mRingBufferF2, mActiveClipListF2, mBufferCountF2, NULL);
	else if(activeF1 && activeF2) {
		uint32 buffercount = MIN(mBufferCountF1, mBufferCountF2);
		err = mDevice->ConfigureCapture(mRingBufferF1, mActiveClipListF1, buffercount, NULL,
		                                mRingBufferF2, mActiveClipListF2);
	}
	return err;
}

status_t 
BBt848Controllable::FormatSuggestionRequested(media_type type, int32 quality, media_format *format)
{
	FUNCTION("BBt848Controllable::FormatSuggestionRequested\n");

	if (type == B_MEDIA_NO_TYPE)
		type = B_MEDIA_RAW_VIDEO;
	
	if (type == B_MEDIA_RAW_VIDEO)
		*format = mVideoFormat;
	else
		return B_MEDIA_BAD_FORMAT;
		
	quality = quality;
	
	return B_OK;
}

status_t 
BBt848Controllable::FormatProposal(const media_source &/*output*/, media_format *format)
{
	FUNCTION("BBt848Controllable::FormatProposal - BEGIN\n");

	uint32 maxX, maxY;
	
	if (mDevice)
		if ((mDevice->VideoFormat() == B_NTSC_M) ||
			(mDevice->VideoFormat() == B_NTSC_J) )
		{
			maxX = 720;
			maxY = 480;
			mRate = 29.97;
		}
		else
		{
			maxX = 768;
			maxY = 576;
			mRate = 25.00;
		}
	else
		{
			ERROR("BBt848Controllable::FormatProposal - NO DEVICE ERROR\n");
			return B_MEDIA_BAD_FORMAT;
		}
		
	if (format->type == B_MEDIA_NO_TYPE)
	{
		// if not specified
		// return error if a second connection is attempted and the first is interlaced
		if (mConnectionActiveF1 && mInterlaced)
		{
			ERROR("BBt848Controllable::FormatProposal - SECOND CONNECTION NOT AVAILABLE\n");
			return B_MEDIA_BAD_FORMAT;
		}
		else
		// otherwise propose the default format
		{
			*format = mVideoFormat;
			return B_OK;
		}
	}
	
	if (format->type == B_MEDIA_RAW_VIDEO)
	{
#if DEBUG
		PrintVideoFormat(format->u.raw_video);
#endif
		
		// first handle field rate
		// since the node either interlaces or
		// splits the fields into two streams, the field rate 
		// is always the frame rate (ie either 29.97 or 25.0)
		if (format->u.raw_video.field_rate == media_raw_video_format::wildcard.field_rate)
		{
			format->u.raw_video.field_rate = mRate;
		}
		else 
			if (format->u.raw_video.field_rate != mRate)
			{
				ERROR("BBt848Controllable::FormatProposal - FIELD RATE ERROR\n");
				return B_MEDIA_BAD_FORMAT;
			}
			
		// We can do 8, 15, 16, and 32-bit.  So as long as they are proposing
		// one of these, then we'll let it stand.  If they propose wildcard,
		// then we'll suggest the default
		if (format->u.raw_video.display.format == media_raw_video_format::wildcard.display.format)
			format->u.raw_video.display.format = mColorspace;
		else
			if ((format->u.raw_video.display.format != B_GRAY8) &&
				(format->u.raw_video.display.format != B_RGB15) &&
				(format->u.raw_video.display.format != B_RGB16) &&
				(format->u.raw_video.display.format != B_RGB24) &&
				(format->u.raw_video.display.format != B_RGB32) &&
				(format->u.raw_video.display.format != B_RGB15_BIG) &&
				(format->u.raw_video.display.format != B_RGB16_BIG) &&
				(format->u.raw_video.display.format != B_RGB32_BIG) &&
				(format->u.raw_video.display.format != B_YCbCr411) &&
				(format->u.raw_video.display.format != B_YCbCr422))				
			{
				format->u.raw_video.display.format = mColorspace;
				ERROR("BBt848Controllable::FormatProposal - BAD DISPLAY FORMAT\n");
				return B_MEDIA_BAD_FORMAT;
			}
		
		// We can only support one orientation: B_VIDEO_TOP_LEFT_RIGHT.
		if (format->u.raw_video.orientation == media_raw_video_format::wildcard.orientation)
			format->u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		else 
			if (format->u.raw_video.orientation != B_VIDEO_TOP_LEFT_RIGHT)
			{
				format->u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
				ERROR("BBt848Controllable::FormatProposal - BAD ORIENTATION\n");
				return B_MEDIA_BAD_FORMAT;
			}
			
		// First active should be 0
		if (format->u.raw_video.first_active == media_raw_video_format::wildcard.first_active)
			format->u.raw_video.first_active = 0;
		else 
			if (format->u.raw_video.first_active != 0)
			{
				ERROR("BBt848Controllable::FormatProposal - BAD FIRST ACTIVE\n");
				return B_MEDIA_BAD_FORMAT;
			}

		if (!mConnectionActiveF1)
		{
			PROGRESS("BBt848Controllable::FormatProposal - FIRST CONNECTION\n");
			// this is the first connection
			
			//
			// line count
			//
			if (format->u.raw_video.display.line_count == media_raw_video_format::wildcard.display.line_count)
			{
				// if wildcard
				// propose default line count
				format->u.raw_video.display.line_count = mYSize;
			}
			else
				if ((format->u.raw_video.display.line_count < 1) || (format->u.raw_video.display.line_count > maxY))
					// if illegal line count return error
				{
					format->u.raw_video.display.line_count = maxY;
				    if(format->u.raw_video.display.line_width > maxX)
						format->u.raw_video.display.line_width = maxX;
					//printf("changed line count %ld\n", format->u.raw_video.display.line_count);
					ERROR("BBt848Controllable::FormatProposal - BAD LINE COUNT\n");
					return B_MEDIA_BAD_FORMAT;
				}
					// otherwise accept the proposed line count
					
			// remember if the accepted line count requires interlace (and thus prohibits a second connection)
			if (format->u.raw_video.display.line_count > maxY/2)
				mInterlaced = true;
			else
				mInterlaced = false;

			//
			// line width & bytes per row
			//
			if (format->u.raw_video.display.line_width == media_raw_video_format::wildcard.display.line_width)
			{
				// if wildcard
				// propose default line width
				format->u.raw_video.display.line_width = mXSize;
				format->u.raw_video.display.bytes_per_row = 
					(mXSize * bitsPerPixel(format->u.raw_video.display.format)+4)/8;
			}
			else
				if ((format->u.raw_video.display.line_width < 1)    || 
				    (format->u.raw_video.display.line_width > maxX))
				    // if illegal line width return error
				{
					ERROR("BBt848Controllable::FormatProposal - BAD LINE WIDTH OR ROW BYTES\n");
					return B_MEDIA_BAD_FORMAT;
				}
					// otherwise accept as is
				
			//
			// interlace
			//
			if (format->u.raw_video.interlace == media_raw_video_format::wildcard.interlace)
			{
				// if wildcard
				// set interlace as appropriate for line count
				format->u.raw_video.interlace = mInterlaced ? 2 : 1;
			}
			else 
				// if improperly set retun error
				if (((format->u.raw_video.interlace == 1) && mInterlaced)  ||
					((format->u.raw_video.interlace == 2) && !mInterlaced) ||
					 (format->u.raw_video.interlace < 1)                   ||
					 (format->u.raw_video.interlace > 2))
				{				 
					ERROR("BBt848Controllable::FormatProposal - BAD INTERLACE\n");
					return B_MEDIA_BAD_FORMAT;
				}
				// otherwise accept as is
		}
		else
		{
			if (!mConnectionActiveF2)
			{
				PROGRESS("BBt848Controllable::FormatProposal - SECOND CONNECTION\n");
				// this is the second connection
				
				if (mInterlaced)
					// second connection not possible if first is interlaced
					return B_MEDIA_BAD_FORMAT;
				else
				{
					//
					// line count
					//
					if (format->u.raw_video.display.line_count == media_raw_video_format::wildcard.display.line_count)
					{
						// if the first connection doesn't require interlace
						// allow a CIF or smaller connection
						format->u.raw_video.display.line_count = mYSize <= maxY/2 ? mYSize : maxY/2;
					}
					else
						if ((format->u.raw_video.display.line_count < 1) || (format->u.raw_video.display.line_count > maxY/2))
							// if illegal line count return error
							return B_MEDIA_BAD_FORMAT;
							// otherwise accept as is

					//
					// line width & bytes per row
					//
					if (format->u.raw_video.display.line_width == media_raw_video_format::wildcard.display.line_width)
					{
						// if wildcard
						// propose default line width
						format->u.raw_video.display.line_width = mXSize;
						format->u.raw_video.display.bytes_per_row = 
							(mXSize * bitsPerPixel(format->u.raw_video.display.format)+4)/8;
					}
					else
						if ((format->u.raw_video.display.line_width < 1)    || 
						    (format->u.raw_video.display.line_width > maxX))
						    // if illegal line width return error
							return B_MEDIA_BAD_FORMAT;
							// otherwise accept as is

					//
					// interlace
					//
					if (format->u.raw_video.interlace == media_raw_video_format::wildcard.interlace)
					{
						// if wildcard
						// set interlace to non-interlaced
						format->u.raw_video.interlace = 1;
					}
					else 
						// if improperly set retun error
						if (format->u.raw_video.interlace != 1)					 
							return B_MEDIA_BAD_FORMAT;
						// otherwise accept as is
				}					
			}
		}	

		//NEED TO HANDLE CHECKING/SETTING THE FOLLOWING:
		//mVideoFormat.u.raw_video.display.bytes_per_row = 320*2;		
		//mVideoFormat.u.raw_video.pixel_width_aspect = 4;
		//mVideoFormat.u.raw_video.pixel_height_aspect = 3;

#if DEBUG
		PrintVideoFormat(format->u.raw_video);
#endif
		return B_OK;
	}

	return B_MEDIA_BAD_FORMAT;
}

status_t 
BBt848Controllable::FormatChangeRequested(const media_source &/*source*/, const media_destination &/*dest*/, media_format */*io_format*/, int32 */*out_change_count*/)
{
	FUNCTION("BBt848Controllable::FormatChangeRequested\n");
	return B_ERROR;
}

status_t 
BBt848Controllable::GetNextOutput(int32 *cookie, media_output *out_destination)
{
	FUNCTION("BBt848Controllable::GetNextOutput\n");

	if (mInitCheck != B_OK)
	{
		ERROR("BBt848Controllable::GetNextOutput - DEVICE NOT INITIALIZED\n");
		return B_ERROR;
	}

	switch (*cookie)
	{
		case 0:
			PROGRESS("BBt848Controllable::GetNextOutput #0\n");
			*out_destination = mOutputF1;
			(*cookie)++;
			return B_OK;
		case 1:
			if (!mInterlaced)
			{
				PROGRESS("BBt848Controllable::GetNextOutput #1\n");
				*out_destination = mOutputF2;
				(*cookie)++;
				return B_OK;
			}
			else
			{
				ERROR("BBt848Controllable::GetNextOutput - BAD INDEX\n");
				return B_BAD_INDEX;
			}
		case 2:
		default:
			ERROR("BBt848Controllable::GetNextOutput - BAD INDEX\n");
			return B_BAD_INDEX;
	}
}

status_t 
BBt848Controllable::DisposeOutputCookie(int32 cookie)
{
	FUNCTION("BBt848Controllable::DisposeOutputCookie - BEGIN\n");
	
	cookie = cookie;	
	return B_OK;
}

status_t 
BBt848Controllable::SetBufferGroup(const media_source &for_source, BBufferGroup *group)
{
	FUNCTION("BBt848Controllable::SetBufferGroup port = %08lx id = %ld\n", for_source.port, for_source.id);
	
	StopCapture();
	
	switch(for_source.id)
	{
		case 1:
			PROGRESS("BBt848Controllable::SetBufferGroup - SOURCE #1\n");
			mBuffersMappedF1 = false;

			// if we're currently using buffers
			// obtained from a previous SetBufferGroup
			// we need to delete the old BVideoImages
			if(mBufferGroupF1)
				delete mBufferGroupF1;
			PROGRESS("BBt848Controllable::SetBufferGroup - DELETING PREVIOUS F1 RING BUFFER\n");
			for (int ctr=0; ctr < (int)mBufferCountF1; ctr++)
				if (mBufferF1[ctr] && mBufferF1[ctr]->IsValid())
				{
					delete mBufferF1[ctr];
					mBufferF1[ctr] = 0;
				}

			if (group == 0)
			{
				PROGRESS("BBt848Controllable::SetBufferGroup - RECLAIM ALL F1 BUFFERS\n");
				// we got here due to a reclaim all buffers
				mUsingConsumerBuffersF1 = false;
				//  create some stinkin buffers!!!!!
			}
			else
			{
				PROGRESS("BBt848Controllable::SetBufferGroup - MAPPING F1 CAPTURE BUFFERS\n");
				mBufferGroupF1 = group;
				mRingBufferF1 = mBufferF1;
				if (MapCaptureBuffers(mBufferGroupF1,
						mOutputF1.format.u.raw_video.display,
						mRingBufferF1, mBufferCountF1) != B_NO_ERROR)
				{
					ERROR("BBt848Controllable::SetBufferGroup - MapCaptureBuffers failed\n");
					mBufferGroupF1 = NULL;
					return B_ERROR;
				}
				mUsingConsumerBuffersF1 = true;
			}

			break;
		case 2:
			PROGRESS("BBt848Controllable::SetBufferGroup - SOURCE #2\n");
			mBuffersMappedF2 = false;
			// if we're currently using buffers
			// obtained from a previous SetBufferGroup
			// we need to delete the old BVideoImages
			if(mBufferGroupF2)
				delete mBufferGroupF2;
			PROGRESS("BBt848Controllable::SetBufferGroup - DELETING PREVIOUS F2 RING BUFFER\n");
			for (int ctr=0; ctr < (int)mBufferCountF2; ctr++)
				if (mBufferF2[ctr] && mBufferF2[ctr]->IsValid())
				{
					delete mBufferF2[ctr];
					mBufferF2[ctr] = 0;
				}

			if (group == 0)
			{
				PROGRESS("BBt848Controllable::SetBufferGroup - RECLAIM ALL F2 BUFFERS\n");
				// we got here due to a reclaim all buffers
				mUsingConsumerBuffersF2 = false;
				//  create some stinkin buffers!!!!!
			}
			else
			{
				PROGRESS("BBt848Controllable::SetBufferGroup - MAPPING F2 CAPTURE BUFFERS\n");
				mBufferGroupF2 = group;
				mRingBufferF2 = mBufferF2;
				if (MapCaptureBuffers(mBufferGroupF2,
						mOutputF2.format.u.raw_video.display,
						mRingBufferF2, mBufferCountF2) != B_NO_ERROR)
				{
					ERROR("BBt848Controllable::SetBufferGroup - MapCaptureBuffers failed\n");
					mBufferGroupF2 = NULL;
					return B_ERROR;
				}
				mUsingConsumerBuffersF2 = true;
			}
			break;
			
		default:
			ERROR("BBt848Controllable::SetBufferGroup - BAD SOURCE\n");
			RestartCapture();
			return B_MEDIA_BAD_SOURCE;			
	}	

	PrepareBufferMap();

	status_t status = ReconfigureCapture();
		
	if (status != B_OK) {
		ERROR("BBt848Controllable::SetBufferGroup - CONFIGURE CAPTURE FAILED\n");
	}
	else {
		RestartCapture();
	}
	
	FUNCTION("BBt848Controllable::SetBufferGroup - END\n");
	return status;
}

static status_t
UpdateCaptureBuffers(BBufferGroup *aGroup,
                     const media_video_display_info & display,
                     BVideoImage ** ringBuffer, uint32 numBuffs)
{
	int32 BPP = bitsPerPixel(display.format);
	int32 offset = ((int32)display.line_offset) * display.bytes_per_row +
	               ((int32)display.pixel_offset)*BPP/8;
	//printf("offset %d < %d %d\n", offset, display.line_offset, display.pixel_offset);
	if(aGroup == NULL) {
		ERROR("BBt848Controllable::UpdateCaptureBuffers - no buffer group\n");
		return B_ERROR;
	}
	if(ringBuffer == NULL) {
		ERROR("BBt848Controllable::UpdateCaptureBuffers - no ringBuffer\n");
		return B_ERROR;
	}
	
	int32 buffCount=0;
	status_t status = aGroup->CountBuffers(&buffCount);
	BBuffer ** buffList = NULL;
	if(status == B_NO_ERROR) {
		if((int32)numBuffs != buffCount) {
			ERROR("BBt848Controllable::UpdateCaptureBuffers - buffer count mismatch\n");
			return B_ERROR;
		}
		buffList = new BBuffer * [buffCount];
		status = aGroup->GetBufferList(buffCount, buffList);
	}
	for (int i = 0; i < (int)numBuffs; i++) {
		ringBuffer[i]->SetImageSize(BPoint(display.line_width, display.line_count));
		ringBuffer[i]->SetColorSpace(display.format);
		ringBuffer[i]->SetBytesPerRow(display.bytes_per_row);
		//pixel_offset;
		//line_offset;
		if(status == B_NO_ERROR) {
			PROGRESS("BBt848Controllable::VideoClippingChanged - "
			         "set F1 buffer #%d -> %p+%ld\n", i,
			         buffList[i]->Data(), offset);
			ringBuffer[i]->SetLogical(true);
			ringBuffer[i]->SetBuffer((char *)buffList[i]->Data()+offset);
		}
	}
	delete [] buffList;
	return B_NO_ERROR;
}

static status_t
MediaClipListToBt848ClipList(int16 num_shorts, int16 *clip_data,
                             bt848_cliplist &bt848clip, int width, int height)
{
	if(clip_data == 0) {
		ERROR("BBt848Controllable::VideoClippingChanged - no clip\n");
		return B_ERROR;
	}
	if(clip_data[0] != 0 || clip_data[1] != 0) {
		ERROR("BBt848Controllable::VideoClippingChanged - offsets not supported\n");
		return B_ERROR;
	}
	int16 *clip_data_end = clip_data+num_shorts;
	int16 *srcline = clip_data+2;
	int16 *srcptr = srcline;
	int linenum = 0;
	int dropped_regions = 0;
	if(*srcptr < 0) {
		ERROR("BBt848Controllable::VideoClippingChanged - bad clip data\n");
		return B_ERROR;
	}
	while(srcptr < clip_data_end) {
		int16 pairval = *srcptr;
		int outcount = 1;
		if(pairval < 0) {
			outcount = -pairval;
		}
		else {
			srcline = srcptr;
		}
		int16 *srcdata = srcline;
		while(outcount > 0) {
			srcdata = srcline;
			int16 pairs = *srcdata++ / 2;
			int i = 0;
			int currx = 0;
			if(linenum >= B_VIDEO_V_MAX) {
				ERROR("BBt848Controllable::VideoClippingChanged - clip data too big\n");
				return B_ERROR;
			}
			if(srcdata + pairs*2 > clip_data_end) {
				ERROR("BBt848Controllable::VideoClippingChanged - bad clip data\n");
				return B_ERROR;
			}
			if(pairs == 0) {
				bt848clip[linenum][i++] = -width;
				currx = width;
			}
			while(pairs > 0) {
				int16 start = *srcdata++;
				int16 end = *srcdata++;
				if(start < 0 || end < start || width <= end) {
					ERROR("BBt848Controllable::VideoClippingChanged - bad clip data\n");
					return B_ERROR;
				}
				if(bt848clip[linenum] == NULL) {
					ERROR("BBt848Controllable::VideoClippingChanged - bad output\n");
					return B_ERROR;
				}
				if(start < currx) {
					ERROR("BBt848Controllable::VideoClippingChanged - "
					      "bad clip data start %d < last end %d\n", start, currx);
					return B_ERROR;
				}
				if(i > MAX_CLIP_LINE-4) {
					dropped_regions++;
				}
				else {
					if(start > currx)
						bt848clip[linenum][i++] = -(start-currx);
					bt848clip[linenum][i++] = (end-start+1);
					currx = end+1;
				}
				pairs--;
			}
			if(currx < width) {
				bt848clip[linenum][i++] = -(width-currx);
			}
			bt848clip[linenum][i++] = 0;
			
			if(i > MAX_CLIP_LINE) {
				ERROR("BBt848Controllable::VideoClippingChanged - "
				      "cliplist overrun used %d bytes of %d\n", i, MAX_CLIP_LINE);
				return B_ERROR;
			}
			linenum++;
			outcount--;
		}
		if(pairval < 0)
			srcptr++;
		else {
			srcptr = srcdata;
		}
	}
	while(linenum < height) {
		bt848clip[linenum][0] = -width;
		bt848clip[linenum][1] = 0;
		linenum++;
	}
	if(dropped_regions > 0) {
		ERROR("BBt848Controllable::VideoClippingChanged - "
		      "cliplist full, dropped %d visible line segments\n", dropped_regions);
	}

	return B_NO_ERROR;
}

status_t 
BBt848Controllable::VideoClippingChanged(const media_source &for_source, int16 num_shorts, int16 *clip_data, const media_video_display_info &display, int32 */*out_from_change_count*/)
{
	FUNCTION("BBt848Controllable::VideoClippingChanged\n");
	status_t err = B_NO_ERROR;

	PROGRESS("BBt848Controllable::VideoClippingChanged - CONVERTING CLIP DATA\n");

	switch(for_source.id)
	{
		case 1:
			PROGRESS("BBt848Controllable::VideoClippingChanged - MAPPING CONNECTION 1\n");
			err = MediaClipListToBt848ClipList(num_shorts, clip_data,
			                                   mClipListF1,
			                                   display.line_width,
			                                   display.line_count);
			if(err != B_NO_ERROR)
				return err;
			mActiveClipListF1 = mClipListF1;
			mOutputF1.format.u.raw_video.display = display;
			UpdateCaptureBuffers(mBufferGroupF1, display, mRingBufferF1, mBufferCountF1);
			if(!mMutedF1) {
				err = ReconfigureCapture();
				if (err == B_OK) {
					SwitchCapture();
				}
				else
				{
					ERROR("BBt848Controllable::VideoClippingChanged - CONFIGURE CAPTURE FAILED\n");
				}
			}
		break;
		
		case 2:
			PROGRESS("BBt848Controllable::VideoClippingChanged - MAPPING CONNECTION 2\n");
			err = MediaClipListToBt848ClipList(num_shorts, clip_data,
			                                   mClipListF2,
			                                   display.line_width,
			                                   display.line_count);
			if(err != B_NO_ERROR)
				return err;
			mActiveClipListF2 = mClipListF2;
			mOutputF2.format.u.raw_video.display = display;
			UpdateCaptureBuffers(mBufferGroupF2, display, mRingBufferF2, mBufferCountF2);
			if(!mMutedF2) {
				err = ReconfigureCapture();
				if (err == B_OK) {
					SwitchCapture();
				}
				else
				{
					ERROR("BBt848Controllable::VideoClippingChanged - CONFIGURE CAPTURE FAILED\n");
				}
			}
		break;
		
		default:
			PROGRESS("BBt848Controllable::VideoClippingChanged - MAPPING NOTHING SOURCE = %ld\n", for_source.id);
			RestartCapture();
			return B_ERROR;
	}

	return err;
}

status_t 
BBt848Controllable::GetLatency(bigtime_t *out_latency)
{
	BBufferProducer::GetLatency(out_latency);
	(*out_latency) += (bigtime_t)(1000000LL/mRate) + SchedulingLatency();
	return B_OK;
}

status_t 
BBt848Controllable::PrepareToConnect(const media_source &/*what*/, const media_destination &/*where*/, media_format *format, media_source *out_source, char *io_name)
{
	FUNCTION("BBt848Controllable::PrepareToConnect\n");

	strcpy(io_name, "Bt848 Input 1");
	
	if (mInitCheck != B_OK)
	{
		ERROR("BBt848Controllable::PrepareToConnect - DEVICE NOT OPENED\n");
		return B_ERROR;
	}

	if (!mConnectionProposedF1)
	{
		PROGRESS("BBt848Controllable::PrepareToConnect - CONNECTION 1\n");
		*out_source = mOutputF1.source;
		mOutputF1.format = *format;
		mConnectionProposedF1 = true;
		mUsingConsumerBuffersF1 = false;
		mActiveClipListF1 = NULL;
		mMutedF1 = false;
	}
	else
		if (!mInterlaced && !mConnectionProposedF2)
		{
			PROGRESS("BBt848Controllable::PrepareToConnect - CONNECTION 2\n");
			*out_source = mOutputF2.source;
			mOutputF2.format = *format;
			mConnectionProposedF2 = true;
			mUsingConsumerBuffersF2 = false;
			mActiveClipListF2 = NULL;
			mMutedF2 = false;
		}
		else
		{
			ERROR("BBt848Controllable::PrepareToConnect - NONEXISTENT CONNECTION\n");
			return B_ERROR;
		}
		
	
	return B_OK;
}

void 
BBt848Controllable::Connect(status_t error, const media_source &source, const media_destination &destination, const media_format &/*format*/, char *io_name)
{
	FUNCTION("BBt848Controllable::Connect - BEGIN Source ID#%ld Destination ID#%ld\n", source.id, destination.id);

	uint32 i = 0;
	
	if (error < B_OK)
	{
		ERROR("BBt848Controllable::Connect - ERROR NOTIFICATION\n");
		if(source.id == mOutputF1.source.id) {
			mConnectionProposedF1 = false;
		}
		if(source.id == mOutputF2.source.id) {
			mConnectionProposedF2 = false;
		}
		return;
	}
	
	bool notConnected = !(mConnectionActiveF1 || mConnectionActiveF2);
	
	StopCapture();
	
	if (!mConnectionActiveF1)
	{
		PROGRESS("BBt848Controllable::Connect - CONNECTION 1\n");
		mOutputF1.destination = destination;
		mConnectionActiveF1 = true;
		if (!mUsingConsumerBuffersF1)
		{
			PROGRESS("BBt848Controllable::Connect - USING PRODUCER BUFFERS, CONNECTION 1\n");
			// create an F1 buffer group if the consumer hasn't supplied one
			if (!mBufferGroupF1)
				mBufferGroupF1 = new BBufferGroup();

			mRingBufferF1 = mBufferF1;
			if (mBufferGroupF1 && mBufferGroupF1->InitCheck() == B_OK)
			{
				// Now, create the video buffers and for each
				// one create a BBuffer and add it to the BufferGroup
				for (i = 0; i < MAX_BUFFERS; i++)
				{
					PROGRESS("BBt848Controllable::Connect - %ld x %ld\n", mOutputF1.format.u.raw_video.display.line_width, mOutputF1.format.u.raw_video.display.line_count);
					mBufferF1[i] = new BVideoImage(BPoint(mOutputF1.format.u.raw_video.display.line_width, mOutputF1.format.u.raw_video.display.line_count),
												mOutputF1.format.u.raw_video.display.format,
												B_INTERLEAVED,
												B_BUFFER_TOP_TO_BOTTOM,
												true);
				
					if (mBufferF1[i] && mBufferF1[i]->IsValid())
					{						
						buffer_clone_info info;
						if ((info.area = area_for(mBufferF1[i]->Buffer())) >= B_OK)
						{
							info.offset = 0;
							info.size = ((size_t)ceil(mBufferF1[i]->ImageSize().y)) * mBufferF1[i]->BytesPerRow();
							info.flags = i;
							info.buffer = 0;
				
							if ((mBufferGroupF1->AddBuffer(info)) != B_OK)
							{
								ERROR("BBt848Controllable::Connect - ERROR ADDING BUFFER TO F1 GROUP\n");
								mRingBufferF1 = 0;
								break;
							}
						}
						else
						{
							ERROR("BBt848Controllable::Connect - ERROR IN AREA_FOR\n");
							mRingBufferF1 = 0;
							break;
						}
					}
					else 
					{
						ERROR("BBt848Controllable::Connect - ERROR CREATING VIDEO RING BUFFER\n");
						mRingBufferF1 = 0;
						break;
					}
				}
			}
			else
			{
				ERROR("BBt848Controllable::Connect - ERROR CREATING F1 BUFFER GROUP\n");
				mRingBufferF1 = 0;
			}
			
			if (!mRingBufferF1)
			{
				for (uint32 j = 0; j < i; j++)
					if (mBufferF1[j]) {
						delete mBufferF1[j];
						mBufferF1[j] = NULL;
					}
				delete mBufferGroupF1;
				mBufferGroupF1 = 0;
				return;
			}
		}
	}
	else
		if (!mInterlaced && !mConnectionActiveF2)
		{
			PROGRESS("BBt848Controllable::Connect - CONNECTION 2\n");
			mOutputF2.destination = destination;
			mConnectionActiveF2 = true;
			if (!mUsingConsumerBuffersF2)
				{
				// create an F2 buffer group if the consumer hasn't supplied one
				if (!mBufferGroupF2)
					mBufferGroupF2 = new BBufferGroup();
	
				mRingBufferF2 = mBufferF2;
				if (mBufferGroupF2 && mBufferGroupF2->InitCheck() == B_OK)
				{
					// Now, create the video buffers and for each
					// one create a BBuffer and add it to the BufferGroup
					for (i = 0; i < MAX_BUFFERS; i++)
					{
						PROGRESS("BBt848Controllable::Connect - %ld x %ld\n", mOutputF2.format.u.raw_video.display.line_width, mOutputF2.format.u.raw_video.display.line_count);
						mBufferF2[i] = new BVideoImage(BPoint(mOutputF2.format.u.raw_video.display.line_width, mOutputF2.format.u.raw_video.display.line_count),
													mOutputF2.format.u.raw_video.display.format,
													B_INTERLEAVED,
													B_BUFFER_TOP_TO_BOTTOM,
													true);
					
						if (mBufferF2[i] && mBufferF2[i]->IsValid())
						{						
							buffer_clone_info info;
							if ((info.area = area_for(mBufferF2[i]->Buffer())) >= B_OK)
							{
								info.offset = 0;
								info.size = ((size_t)ceil(mBufferF2[i]->ImageSize().y)) * mBufferF2[i]->BytesPerRow();
								info.flags = i;
								info.buffer = 0;
					
								if ((mBufferGroupF2->AddBuffer(info)) != B_OK)
								{
									ERROR("BBt848Controllable::Connect - ERROR ADDING BUFFER TO F2 GROUP\n");
									mRingBufferF2 = 0;
									break;
								}
							}
							else
							{
								ERROR("BBt848Controllable::Connect - ERROR IN AREA_FOR\n");
								mRingBufferF2 = 0;
								break;
							}
						}
						else 
						{
							ERROR("BBt848Controllable::Connect - ERROR CREATING F2 VIDEO RING BUFFER\n");
							mRingBufferF2 = 0;
							break;
						}
					}
				}
				else
				{
					ERROR("BBt848Controllable::Connect - ERROR CREATING F2 BUFFER GROUP\n");
					mRingBufferF2 = 0;
				}
				
				if (!mRingBufferF2)
				{
					for (uint32 j = 0; j < i; j++)
						if (mBufferF2[j]) {
							delete mBufferF2[j];
							mBufferF2[j] = NULL;
						}
					delete mBufferGroupF2;
					mBufferGroupF2 = 0;
					return;
				}
	
			}	
		}
		else
		{
			ERROR("BBt848Controllable::Connect - NONEXISTENT CONNECTION\n");
			return;
		}
		

	PrepareBufferMap();

	//printf("bt848: connect cliplist %p\n", mActiveClipListF1);
	status_t status = ReconfigureCapture();

	if (status != B_OK) {
		ERROR("BBt848Controllable::Connect - CONFIGURE CAPTURE FAILED\n");
		return;
	}
	
	// enable audio from the selected source
	if (mHitachi)
		mDevice->SetGpio(0x00001f);
	else 
		if (mAudioMux)	
			mAudioMux->SetSource(mAudioSource);

	
	strcpy(io_name, Name());
	
	// redetermine and set latencies	
	bigtime_t downstreamLatency = 0;
	media_node_id id;
	FindLatencyFor(destination, &downstreamLatency, &id);
	
	if (downstreamLatency > mDownstreamLatency)
		mDownstreamLatency = downstreamLatency;
	
	bigtime_t currentEventLatency = EventLatency();
	bigtime_t duration = (bigtime_t)(1000000LL/mRate);
	
	if (currentEventLatency < duration + mDownstreamLatency)
		SetEventLatency(mDownstreamLatency + duration);

	SetInitialLatency(duration);

	RestartCapture(notConnected);

	FUNCTION("BBt848Controllable::Connect - END\n");
}

void 
BBt848Controllable::Disconnect(const media_source & DEBUGARG(source), const media_destination &destination)
{
	FUNCTION("BBt848Controllable::Disconnect - BEGIN Source ID#%ld Destination ID#%ld\n", source.id, destination.id);

	if ((mOutputF1.destination != destination) && (mOutputF2.destination != destination))
	{
		ERROR("BBt848Controllable::Disconnect - NOT OUR CONNECTION\n");	
		return;
	}

	StopCapture();

	if (mConnectionActiveF1 && (mOutputF1.destination == destination))
	{
		PROGRESS("BBt848Controllable::Disconnect - Disconnecting #1\n");
		mOutputF1.destination = media_destination::null;
		mConnectionProposedF1 = false;
		mConnectionActiveF1 = false;
		mBuffersMappedF1 = false;
		if (mBufferGroupF1)
			delete mBufferGroupF1;
		mBufferGroupF1 = NULL;

		PROGRESS("BBt848Controllable::Disconnect - Deleting F1 VideoImages\n");	
		for (int i = 0; i < (int)mBufferCountF1; i++)
			if (mBufferF1[i] && mBufferF1[i]->IsValid())
			{
				delete mBufferF1[i];
				mBufferF1[i] = NULL;
			}

	}
	else
		if (mConnectionActiveF2 && (mOutputF2.destination == destination))
		{
			PROGRESS("BBt848Controllable::Disconnect - Disconnecting #2\n");
			mOutputF2.destination = media_destination::null;
			mConnectionProposedF2 = false;
			mConnectionActiveF2 = false;
			mBuffersMappedF2 = false;
			if (mBufferGroupF2)
				delete mBufferGroupF2;
			mBufferGroupF2 = NULL;

			PROGRESS("BBt848Controllable::Disconnect - Deleting F2 VideoImages\n");	
			for (int i = 0; i < (int)mBufferCountF2; i++)
				if (mBufferF2[i] && mBufferF2[i]->IsValid())
				{
					delete mBufferF2[i];
					mBufferF2[i] = NULL;
				}
		}
		else
		{
			ERROR("BBt848Controllable::Disconnect - CONNECTION NOT ACTIVE\n");	
		}
	
	// tidy up if all connections are closed
	if (!mConnectionActiveF1 && !mConnectionActiveF2)
	{
		// save our settings
		PROGRESS("BBt848Controllable::Disconnect() - SAVING SETTINGS TO %s\n", mSettingsFile);
		settings->SaveSettings(false);
	
		//mute audio
		if (mHitachi)
			mDevice->SetGpio(0x20001f);
		else
			if (mAudioMux)
				mAudioMux->Mute();
		
	}
	else {
		status_t status = ReconfigureCapture();
		
		if (status == B_OK) {
			RestartCapture();
		}
		else {
			ERROR("BBt848Controllable::Disconnect - CONFIGURE CAPTURE FAILED\n");
		}		
		
	}
	
	FUNCTION("BBt848Controllable::Disconnect - END\n");
	return;
}

void 
BBt848Controllable::LateNoticeReceived(const media_source &/*what*/, bigtime_t /*how_much*/, bigtime_t /*performance_time*/)
{
	FUNCTION("BBt848Controllable::LateNoticeReceived\n");
}

void 
BBt848Controllable::EnableOutput(const media_source &what, bool enabled, int32 */*change_tag*/)
{
	FUNCTION("BBt848Controllable::EnableOutput\n");
	status_t err;
	
	switch (what.id)
	{
		case 1:
			//printf("bt848 enable output 1: %d\n", enabled);
			if(!mConnectionActiveF2 || mMutedF2)
				StopCapture();
			mMutedF1 = !enabled;
			err = ReconfigureCapture();
			if(err != B_NO_ERROR) {
				ERROR("BBt848Controllable::EnableOutput - ReconfigureCapture FAILED\n");
				StopCapture();
			}
			else {
				if(mConnectionActiveF2 && !mMutedF2)
					SwitchCapture();
				else
					RestartCapture();
			}

			if (mOutputF1.destination != media_destination::null)
			{
				SendDataStatus(mMutedF1 ? B_DATA_NOT_AVAILABLE : B_DATA_AVAILABLE, 
					mOutputF1.destination, TimeSource()->Now());
			}
			break;
		case 2:
			if(!mConnectionActiveF1 || mMutedF1)
				StopCapture();
			mMutedF2 = !enabled;
			err = ReconfigureCapture();
			if(err != B_NO_ERROR) {
				ERROR("BBt848Controllable::EnableOutput - ReconfigureCapture FAILED\n");
				StopCapture();
			}
			else {
				if(mConnectionActiveF1 && !mMutedF1)
					SwitchCapture();
				else
					RestartCapture();
			}
			if (mOutputF2.destination != media_destination::null)
			{
				SendDataStatus(mMutedF2 ? B_DATA_NOT_AVAILABLE : B_DATA_AVAILABLE, 
					mOutputF2.destination, TimeSource()->Now());
			}
			break;
		default:
			ERROR("BBt848Controllable::EnableOutput - NON_EXISTENT CONNECTION\n");
			break;
	}
	
}

status_t 
BBt848Controllable::SetPlayRate(int32 /*numer*/, int32 /*denom*/)
{
	return B_ERROR;
}

void 
BBt848Controllable::AdditionalBufferRequested(const media_source &/*source*/, media_buffer_id /*prev_buffer*/, bigtime_t /*prev_time*/, const media_seek_tag */*prev_tag*/)
{
	//it's good to feel wanted
}

status_t 
BBt848Controllable::GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *ioSize)
{
	ERROR("BBt848Controllable::GetParameterValue - %ld  Value: %ld  Size: %ld\n", id, *((int32 *)value), *ioSize);
	
	if (last_change)
		*last_change = bigtime_t(0);
		
	int32 *aValue = ((int32 *)value);
	
	*ioSize = sizeof(int32);
	
	switch(id)
	{
		case B_CONTROL_VIDEO_INPUT_1:
			*aValue = (int32)mVideoInput[0];			
		break;

		case B_CONTROL_VIDEO_INPUT_2:
			*aValue = (int32)mVideoInput[1];			
		break;
		
		case B_CONTROL_VIDEO_INPUT_3:
			*aValue = (int32)mVideoInput[2];			
		break;
		
		case B_CONTROL_VIDEO_INPUT_4:
			*aValue = (int32)mVideoInput[3];			
		break;
		
		case B_CONTROL_AUDIO_INPUT_1:
			*aValue = (int32)mAudioInput[0];			
		break;
		
		case B_CONTROL_AUDIO_INPUT_2:
			*aValue = (int32)mAudioInput[1];			
		break;
		
		case B_CONTROL_AUDIO_INPUT_3:
			*aValue = (int32)mAudioInput[2];			
		break;
		
		case B_CONTROL_AUDIO_INPUT_4:
			*aValue = (int32)mAudioInput[3];			
		break;

		case B_CONTROL_AUDIO_MUX_TYPE:
			*aValue = mAudioMuxType;			
		break;
		
		case B_CONTROL_BRIGHTNESS:
			if (mVideoControls)
				*(float*)aValue = mVideoControls->Brightness();
		break;
		
		case B_CONTROL_CONTRAST:
			if (mVideoControls)
				*(float*)aValue = mVideoControls->Contrast();
		break;
		
		case B_CONTROL_HUE:
			if (mVideoControls)
				*(float*)aValue = mVideoControls->Hue();
		break;
		
		case B_CONTROL_SATURATION:
			if (mVideoControls)
				*(float*)aValue = mVideoControls->Saturation();
		break;
		
		case B_CONTROL_PLL:
			if (mDevice)
				*aValue = mDevice->Pll();
		break;

		case B_CONTROL_GAMMA_CORRECTION:
			if (mVideoControls)
				*aValue = mVideoControls->GammaCorrectionRemoval();
		break;

		case B_CONTROL_ERROR_DIFFUSION:
			if (mVideoControls)
				*aValue = mVideoControls->ErrorDiffusion();
		break;

		case B_CONTROL_LUMA_CORING:
			if (mVideoControls)
				*aValue = mVideoControls->LumaCoring();
		break;

		case B_CONTROL_LUMA_COMB:
			if (mVideoControls)
				*aValue = mVideoControls->LumaCombFilter();
		break;

		case B_CONTROL_CHROMA_COMB:
			if (mVideoControls)
				*aValue = mVideoControls->ChromaCombFilter();
		break;

		case B_CONTROL_AUDIO_INPUT:
			if (mAudioMux)
				// we use the local source variable so we don't save the mute condition
				*aValue = mAudioSource;
		break;
		
		case B_CONTROL_AUDIO_MODE:
			*aValue = mAudioMode;
		break;
		
		case B_CONTROL_MSP_MODE:
			*aValue = mMspMode;
		break;
		
		case B_CONTROL_VIDEO_INPUT:
			if (mVideoMux)
				*aValue = mVideoMux->Source() & 0x3;
		break;

		case B_CONTROL_TUNER_BRAND:
			if (mTuner)
				*aValue =  ((Bt848Source *)mDevice)->TunerBrand(); 
		break;
		
		case B_CONTROL_TUNER_LOCALE:
			if (mTuner)
				*aValue = mTuner->TunerLocale();
		break;
		
		case B_CONTROL_CHANNEL:
			if (mTuner)
			{
				if ((mTuner->TunerLocale() == B_FM_RADIO) && mHasFmFavorites)
					*aValue = mFmFavoritesIndex;
				else
				{
					if (mHasTvFavorites)
						*aValue = mTvFavoritesIndex;
					else
						*aValue = mTuner->CurrentIndex();
				}					
			}
		break;
		
		case B_CONTROL_VIDEO_FORMAT:
			if (mDevice)
				*aValue = mDevice->VideoFormat();
		break;
		
		case B_CONTROL_DEFAULT_SIZE:
			switch(mXSize)
			{
				case 768:
					*aValue = 0;
					break;
				case 720:
					if (mYSize == 576)
						*aValue = 1;
					else
						*aValue = 2;
					break;
				case 640:
					*aValue = 3;
					break;
				case 352:
					*aValue = 4;
					break;
				case 320:
					*aValue = 5;
					break;
				case 160:
					*aValue = 6;
					break;
				default:
					*aValue = 5;
					break;
			}		
		break;
		
		case B_CONTROL_DEFAULT_COLORSPACE:
			switch(mColorspace)
			{
				case B_GRAY8:
					*aValue = 0;
					break;
				case B_RGB15:
					*aValue = 1;
					break;
				case B_RGB16:
					*aValue = 2;
					break;
				case B_RGB32:
					*aValue = 3;
					break;
				default:
					*aValue = 2;
					break;
			}		
		break;
		
		case B_CONTROL_IS_HITACHI:
			*aValue = (mHitachi ? 1 : 0);
		break;

		default:
			return B_BAD_VALUE;
		break;
	}
	
	return B_OK;
}

void 
BBt848Controllable::SetParameterValue(int32 id, bigtime_t /*when*/, const void *value, size_t /*size*/)
{
	PROGRESS("BBt848Controllable::SetParameterValue ID: %ld  Value: %ld %ld\n",
		id, (int32)*((int32 *)value), (int32)*((float *)value));
		
	int32 aValue = *((int32 *)value);
	float floatValue = *((float *)value);
	
	switch(id)
	{
		case B_CONTROL_VIDEO_INPUT_1:
			mVideoInput[0] = (EVideoInputName)aValue;			
			videoNameSetting[0]->ValueChanged(kVideoSourceName[mVideoInput[0]]);
			RequestNewWeb();
		break;

		case B_CONTROL_VIDEO_INPUT_2:
			mVideoInput[1] = (EVideoInputName)aValue;			
			videoNameSetting[1]->ValueChanged(kVideoSourceName[mVideoInput[1]]);
			RequestNewWeb();
		break;

		case B_CONTROL_VIDEO_INPUT_3:
			mVideoInput[2] = (EVideoInputName)aValue;			
			videoNameSetting[2]->ValueChanged(kVideoSourceName[mVideoInput[2]]);
			RequestNewWeb();
		break;

		case B_CONTROL_VIDEO_INPUT_4:
			mVideoInput[3] = (EVideoInputName)aValue;			
			videoNameSetting[3]->ValueChanged(kVideoSourceName[mVideoInput[3]]);
			RequestNewWeb();
		break;

		case B_CONTROL_AUDIO_INPUT_1:
			mAudioInput[0] = (EAudioInputName)aValue;			
			audioNameSetting[0]->ValueChanged(kAudioSourceName[mAudioInput[0]]);
			if (strcmp(kAudioSourceName[1], kAudioSourceName[mAudioInput[0]]) == 0)
				mAudioMux->SetMute(0);
			RequestNewWeb();
		break;

		case B_CONTROL_AUDIO_INPUT_2:
			mAudioInput[1] = (EAudioInputName)aValue;			
			audioNameSetting[1]->ValueChanged(kAudioSourceName[mAudioInput[1]]);
			if (strcmp(kAudioSourceName[1], kAudioSourceName[mAudioInput[1]]) == 0)
				mAudioMux->SetMute(1);
			RequestNewWeb();
		break;

		case B_CONTROL_AUDIO_INPUT_3:
			mAudioInput[2] = (EAudioInputName)aValue;			
			audioNameSetting[2]->ValueChanged(kAudioSourceName[mAudioInput[2]]);
			if (strcmp(kAudioSourceName[1], kAudioSourceName[mAudioInput[2]]) == 0)
				mAudioMux->SetMute(2);
			RequestNewWeb();
		break;

		case B_CONTROL_AUDIO_INPUT_4:
			mAudioInput[3] = (EAudioInputName)aValue;			
			audioNameSetting[3]->ValueChanged(kAudioSourceName[mAudioInput[3]]);
			if (strcmp(kAudioSourceName[1], kAudioSourceName[mAudioInput[3]]) == 0)
				mAudioMux->SetMute(3);
			RequestNewWeb();
		break;

		case B_CONTROL_BRIGHTNESS:
			if (mVideoControls)
			{
				mVideoControls->SetBrightness(floatValue);
				brightnessSetting->ValueChanged(floatValue);
			}
		break;
		
		case B_CONTROL_CONTRAST:
			if (mVideoControls)
			{
				mVideoControls->SetContrast(floatValue);
				contrastSetting->ValueChanged(floatValue);
			}
		break;
		
		case B_CONTROL_HUE:
			if (mVideoControls)
			{
				mVideoControls->SetHue(floatValue);
				hueSetting->ValueChanged(floatValue);
			}
		break;
		
		case B_CONTROL_SATURATION:
			if (mVideoControls)
			{
				mVideoControls->SetSaturation(floatValue);
				saturationSetting->ValueChanged(floatValue);
			}
		break;
		
		case B_CONTROL_PLL:
			if (mDevice)
			{
				mDevice->SetPll(aValue);
				pllSetting->ValueChanged(aValue);
			}
		break;

		case B_CONTROL_GAMMA_CORRECTION:
			if (mVideoControls)
			{
				mVideoControls->SetGammaCorrectionRemoval(aValue);
				gammaSetting->ValueChanged(aValue);
			}
		break;

		case B_CONTROL_ERROR_DIFFUSION:
			if (mVideoControls)
			{
				mVideoControls->SetErrorDiffusion(aValue);
				errorDiffusionSetting->ValueChanged(aValue);
			}
		break;

		case B_CONTROL_LUMA_CORING:
			if (mVideoControls)
			{
				mVideoControls->SetLumaCoring(aValue);
				lumaCoringSetting->ValueChanged(aValue);
			}
		break;

		case B_CONTROL_LUMA_COMB:
			if (mVideoControls)
			{
				mVideoControls->SetLumaCombFilter(aValue);
				lumaFilterSetting->ValueChanged(aValue);
			}
		break;

		case B_CONTROL_CHROMA_COMB:
			if (mVideoControls)
			{
				mVideoControls->SetChromaCombFilter(aValue);
				chromaFilterSetting->ValueChanged(aValue);
			}
		break;

		case B_CONTROL_AUDIO_INPUT:
			if (mAudioMux)
			{
				mAudioSource = (uint32)aValue;		
				mAudioMux->SetSource(mAudioSource);
				audioSourceSetting->ValueChanged(mAudioSource);
			}			
		break;

		case B_CONTROL_AUDIO_MUX_TYPE:
			if (mAudioMux)
			{
				if(!mI2CBus->I2CDevicePresent(0x40))
					mAudioMuxType = (uint32)aValue;	
				else
					if (aValue != 4 && aValue != 5)
						mAudioMuxType = (uint32)aValue;
					else	
						mAudioMuxType = 0;
					
				// set the old selection bits to default (high)
				mAudioMux->SetSource(0x03);
				mAudioMux->SetShift(mAudioMuxType);
				mAudioMux->SetSource(mAudioSource);
				audioMuxTypeSetting->ValueChanged(mAudioMuxType);
			}			
		break;

		case B_CONTROL_MSP_MODE:
			if (mDevice)
			{
				PROGRESS("Set MSP Mode to %ld\n", aValue);
				mMspMode = aValue;
				msp_setmode(mI2CBus, mMspMode);
				mspModeSetting->ValueChanged(kMspMode[mMspMode]);
			}
		break;
		
		case B_CONTROL_AUDIO_MODE:
			if (mDevice && mHitachi)
			{
				PROGRESS("Set Audio Mode to %ld\n", aValue);
				mAudioMode = (uint32)aValue;
				switch((uint32)aValue)
				{
					case 0:
						mDevice->SetGpio(0x00001f);
					break;
					
					case 1:
						mDevice->SetGpio(0xc0001f);
					break;
					
					case 2:
						mDevice->SetGpio(0x40001f);
					break;
					
					default:
						ERROR("ERROR -- BAD AUDIO MODE\n");
					break;
					
				}
				audioModeSetting->ValueChanged(kAudioMode[aValue]);
			}
		break;
				
		case B_CONTROL_VIDEO_INPUT:
			if (mVideoMux)
			{
				if(aValue >= 0 && aValue < 4 &&
				   strcmp(kVideoSourceName[6], videoNameSetting[aValue]->Value()) == 0)
					mVideoMux->SetSource((uint32)aValue + 4);
				else
					mVideoMux->SetSource((uint32)aValue);
				videoSourceSetting->ValueChanged(aValue);
			}
		break;

		case B_CONTROL_TUNER_BRAND:
			if (mTuner)
			{
				((Bt848Source *)mDevice)->SetTunerBrand((bt848_tuner_mfg)aValue);
				
				if ( (mTuner->TunerLocale() == B_FM_RADIO) && mHasFmFavorites)
						mTuner->Tune(mFmFavorites[mFmFavoritesIndex].frequency);
				else
				{
					if (mHasTvFavorites)
						mTuner->Tune(mTvFavorites[mTvFavoritesIndex].frequency);
					else
						mTuner->TuneIndex(mTuner->CurrentIndex());
				}
				
				mVideoMux->SetSource(mVideoMux->Source());
				if (!mHitachi)
					mAudioMux->SetSource(mAudioMux->Source());
				tunerBrandSetting->ValueChanged(kTunerBrand[aValue]);
			}
		break;
		
		case B_CONTROL_TUNER_LOCALE:
			if (mTuner)
			{
				mTuner->SetTunerLocale((tuner_locale)aValue);
				
				if ( (mTuner->TunerLocale() == B_FM_RADIO) && mHasFmFavorites)
						mTuner->Tune(mFmFavorites[mFmFavoritesIndex].frequency);
				else
				{
					if (mHasTvFavorites)
						mTuner->Tune(mTvFavorites[mTvFavoritesIndex].frequency);
					else
						mTuner->TuneIndex(mTuner->CurrentIndex());
				}
				
				mVideoMux->SetSource(mVideoMux->Source());
				if (!mHitachi)
					mAudioMux->SetSource(mAudioMux->Source());
				tunerLocaleSetting->ValueChanged(kTunerLocale[aValue]);
				RequestNewWeb();
			}
		break;
		
		case B_CONTROL_CHANNEL:
			if (mTuner)
			{
				if (mHitachi)
					mDevice->SetGpio(0x20001f);
				else
				{
					if (mAudioMux)
					{
						mAudioSource = mAudioMux->Source();
						mAudioMux->Mute();
					}
				}
				
				StopCapture();
				
				if ((mTuner->TunerLocale() == B_FM_RADIO) && mHasFmFavorites)
				{
					mFmFavoritesIndex = aValue;
					mTuner->Tune(mFmFavorites[mFmFavoritesIndex].frequency);
					channelSetting->ValueChanged(mFmFavorites[mFmFavoritesIndex].name);
				}
				else
				{
					if (mHasTvFavorites)
					{
						mTvFavoritesIndex = aValue;
						mTuner->Tune(mTvFavorites[mTvFavoritesIndex].frequency);
						channelSetting->ValueChanged(mTvFavorites[mTvFavoritesIndex].name);
					}
					else
					{
						mTuner->TuneIndex(aValue);
						channelSetting->ValueChanged(mTuner->ChannelNameForIndex(aValue));
					}
				}

				snooze(50000);

				RestartCapture();
				
				if (mHitachi)
					mDevice->SetGpio(0x00001f);
				else
					if (mAudioMux)
						mAudioMux->SetSource(mAudioSource);
						
								
			}
		break;
		
		case B_CONTROL_VIDEO_FORMAT:
			if (mDevice)
			{
				mDevice->SetVideoFormat((video_format)aValue);
				mVideoMux->SetSource(mVideoMux->Source());
				formatSetting->ValueChanged(kVideoFormat[aValue]);
				RequestNewWeb();
			}
		break;
		
		case B_CONTROL_DEFAULT_SIZE:
			switch(aValue)
			{
				case 0:
					mXSize = 768;
					mYSize = 576;
					mInterlaced = true;
					break;
				case 1:
					mXSize = 720;
					mYSize = 576;
					mInterlaced = true;
					break;
				case 2:
					mXSize = 720;
					mYSize = 480;
					mInterlaced = true;
					break;
				case 3:
					mXSize = 640;
					mYSize = 480;
					mInterlaced = true;
					break;
				case 4:
					mXSize = 352;
					mYSize = 240;
					mInterlaced = false;
					break;
				case 5:
					mXSize = 320;
					mYSize = 240;
					mInterlaced = false;
					break;
				case 6:
					mXSize = 160;
					mYSize = 120;
					mInterlaced = false;
					break;
				default:
					mXSize = 320;
					mYSize = 240;
					mInterlaced = false;
					break;
			}		
			imageSizeSetting->ValueChanged(kImageSize[aValue]);
		break;
		
		case B_CONTROL_DEFAULT_COLORSPACE:
			switch(aValue)
			{
				case 0:
					mColorspace = B_GRAY8;
					break;
				case 1:
					mColorspace = B_RGB15;
					break;
				case 2:
					mColorspace = B_RGB16;
					break;
				case 3:
					mColorspace = B_RGB32;
					break;
				default:
					mColorspace = B_RGB16;
					break;
			}		
			colorspaceSetting->ValueChanged(kColorspace[aValue]);
		break;
		
		default:
			ERROR("BBt848Controllable::SetParameterValue - BAD ID\n");
		break;
		
	}
}


bool 
BBt848Controllable::ReadTvFavorites()
{
	FUNCTION("BBt848Controllable::ReadTvFavorites\n");
	FILE *file;
	file = fopen("/boot/home/config/settings/Media/bt848/tvfavorites", "r");
	if (!file) {
		ERROR("BBt848Controllable::ReadTvFavorites - NO TV FAVORITES\n");
		return false;
	}
	
	int32 i = 0;
//	int32 n = -1;
	char s[80];
	
	while (fgets(s, 80, file) != NULL)
	{
		if (s[0] == '#')
			continue;
			
		if (sscanf(s, "%s %ld", mTvFavorites[i].name, &mTvFavorites[i].frequency) == 2)
		{
			PROGRESS("%s %ld\n", mTvFavorites[i].name, mTvFavorites[i].frequency);
			if (i++ == 128)
				break;
		}
		else 
		{
			mTvFavorites[i].frequency = 0;
			break;
		}
	}	
	
	fclose(file);
	FUNCTION("BBt848Controllable::ReadTvFavorites - END\n");
	return true;
}

bool 
BBt848Controllable::ReadFmFavorites()
{
	FUNCTION("BBt848Controllable::ReadFmFavorites\n");
	FILE *file;
	file = fopen("/boot/home/config/settings/Media/bt848/fmfavorites", "r");
	if (!file) {
		ERROR("BBt848Controllable::ReadTvFavorites - NO FM FAVORITES\n");
		return false;
	}
	
	int32 i = 0;
//	int32 n = -1;
	char s[80];
	
	while (fgets(s, 80, file) != NULL)
	{
		if (s[0] == '#')
			continue;
			
		if (sscanf(s, "%s %ld", mFmFavorites[i].name, &mFmFavorites[i].frequency) == 2)
		{
			PROGRESS("%s %ld\n", mFmFavorites[i].name, mFmFavorites[i].frequency);
			if (i++ == 128)
				break;
		}
		else 
		{
			mFmFavorites[i].frequency = 0;
			break;
		}
	}	
	
	fclose(file);
	FUNCTION("BBt848Controllable::ReadFmFavorites - END\n");
	return true;
}

status_t
BBt848Controllable::MapCaptureBuffers(BBufferGroup *aGroup, const media_video_display_info & display,
										 BVideoImage ** ringBuffer, uint32 &numBuffs)
{
	FUNCTION("BBt848Controllable::MapCaptureBuffers - BEGIN\n");
	
	numBuffs = 0;
	DEBUGVAR(uint32 rowBytes, display.bytes_per_row);
	PROGRESS("BBt848Controllable::MapCaptureBuffers - rowbytes = %ld\n",rowBytes);
	uint32 BPP = bitsPerPixel(display.format);
	PROGRESS("BBt848Controllable::MapCaptureBuffers - bits per pixel = %ld\n",BPP);
	
	// First let's find out how many buffers
	// we have in the group
	int32 buffCount=0;
	status_t status = aGroup->CountBuffers(&buffCount);
	if ((B_OK != status) || (buffCount < 1) || buffCount > MAX_BUFFERS)
	{
		// If we got a error, then we don't want to do 
		// any more processing.
		ERROR("BBt848Controllable::MapCaptureBuffers - Error from CountBuffers (0x%lx) Num: %ld\n", status, buffCount);
		return B_ERROR;
	}
	PROGRESS("BBt848Controllable::MapCaptureBuffers - buffer count = %ld\n",buffCount);
	
	// Get a handle on the buffers that were allocated in 
	// the buffer group.
	BBuffer ** buffList = new BBuffer * [buffCount];
	status = aGroup->GetBufferList(buffCount, buffList);

	// Now create BVideoImage objects to match each of
	// the buffers that are in the group.
	for (int i = 0; i < buffCount; i++)
	{
		PROGRESS("BBt848Controllable::MapCaptureBuffers - Handling buffer #%d\n", i);							
		ringBuffer[i] = new BVideoImage(BPoint(display.line_width, display.line_count),
									display.format,
									B_INTERLEAVED,
									B_BUFFER_TOP_TO_BOTTOM,
									false);
		
		if (ringBuffer[i])
		{		
			PROGRESS("BBt848Controllable::MapCaptureBuffers - Add VideoImage %p to ring buffer\n", ringBuffer[i]);							
			ringBuffer[i]->SetImageSize(BPoint(display.line_width, display.line_count));
			ringBuffer[i]->SetColorSpace(display.format);
			ringBuffer[i]->SetLogical(true);
			ringBuffer[i]->SetBytesPerRow(display.bytes_per_row);
			int32 offset = (display.line_offset * display.bytes_per_row) + (display.pixel_offset*BPP+4)/8;
			PROGRESS("BBt848Controllable::MapCaptureBuffers - buffer %ld base = %p\n",buffList[i]->ID(), (char *)buffList[i]->Data());
			ringBuffer[i]->SetBuffer((char *)buffList[i]->Data()+offset);
		}
		else
		{
			for (int j = 0; j < i; j++)
				if (ringBuffer[j])
					delete ringBuffer[j];

			numBuffs = 0;
			delete [] buffList;

			ERROR("BBt848Controllable::MapCaptureBuffers - ERROR CONFIGURING BUFFER\n");
			return B_ERROR;		
		}
	}
	
	numBuffs = buffCount;
	delete [] buffList;
	
	FUNCTION("BBt848Controllable::MapCaptureBuffers - END\n");
	
	return B_OK;
}

void 
BBt848Controllable::PrepareBufferMap()
{
	int32 buffCount = 0;

	// Build mapping from ring buffer index to buffers in BufferList for F1
	if (mConnectionActiveF1 && !mBuffersMappedF1)
	{
		mBuffersMappedF1 = false;
		
		for (int i = 0; i < MAX_BUFFERS; i++) bufferF1[i] = 0;		
		if (mBufferGroupF1->CountBuffers(&buffCount) == B_OK)
		{					
			BBuffer ** buffList = new BBuffer * [buffCount];	
			for (int i = 0; i < buffCount; i++) buffList[i] = 0;
			if (mBufferGroupF1->GetBufferList(buffCount, buffList) == B_OK)
				for (int i = 0; i < buffCount; i++)
				{
					if (buffList[i] != NULL)
					{
						bufferF1[i] = buffList[i];
						PROGRESS(" i = %d buffer = %p\n", i, bufferF1[i]);
					}
					else
					{
						ERROR("BBt848Controllable::CaptureRun ERROR MAPPING RING BUFFER\n");
					}
				}
			else
			{
				ERROR("BBt848Controllable::CaptureRun ERROR IN GET BUFFER LIST\n");
			}
			delete [] buffList;
		}
		else
		{
			ERROR("BBt848Controllable::CaptureRun ERROR IN COUNT BUFFERS\n");
		}
		mBuffersMappedF1 = true;
		mBufferCountF1 = buffCount;
	}
	
	// Build mapping from ring buffer index to buffers in BufferList for F2
	if (mConnectionActiveF2 && !mBuffersMappedF2)
	{
		mBuffersMappedF2 = false;
		
		for (int i = 0; i < MAX_BUFFERS; i++) bufferF2[i] = 0;		
		if (mBufferGroupF2->CountBuffers(&buffCount) == B_OK)
		{					
			BBuffer ** buffList = new BBuffer * [buffCount];	
			for (int i = 0; i < buffCount; i++) buffList[i] = 0;
			if (mBufferGroupF2->GetBufferList(buffCount, buffList) == B_OK)
				for (int i = 0; i < buffCount; i++)
				{
					if (buffList[i] != NULL)
					{
						bufferF2[i] = buffList[i]
						PROGRESS(" i = %d buffer = %p\n", i, bufferF2[i]);
					}
					else
					{
						ERROR("BBt848Controllable::CaptureRun ERROR MAPPING RING BUFFER\n");
					}
				}
			else
			{
				ERROR("BBt848Controllable::CaptureRun ERROR IN GET BUFFER LIST\n");
			}
			delete [] buffList;
		}
		else
		{
			ERROR("BBt848Controllable::CaptureRun ERROR IN COUNT BUFFERS\n");
		}

		mBuffersMappedF2 = true;
		mBufferCountF2 = buffCount;
	}
}

void 
BBt848Controllable::SetUpSettings(const char *filename, const char *directory)
{
//PROGRESS("in BBt848Controllable::SetUpSettings\n");
	settings = new Settings(filename, directory);

	settings->Add(videoNameSetting[0] = new EnumeratedStringValueSetting("VideoInput1", "Composite 1", kVideoSourceName,
			"video1 input name expected", "unrecognized video1 input name"));
	settings->Add(videoNameSetting[1] = new EnumeratedStringValueSetting("VideoInput2", "SVideo", kVideoSourceName,
			"video2 input name expected", "unrecognized video2 input name"));
	settings->Add(videoNameSetting[2] = new EnumeratedStringValueSetting("VideoInput3", "Tuner", kVideoSourceName,
			"video3 input name expected", "unrecognized video3 input name"));
	settings->Add(videoNameSetting[3] = new EnumeratedStringValueSetting("VideoInput4", "Composite 2", kVideoSourceName,
			"video4 input name expected", "unrecognized video4 input name"));

	settings->Add(audioNameSetting[0] = new EnumeratedStringValueSetting("AudioInput1", "Tuner", kAudioSourceName,
			"audio1 input name expected", "unrecognized audio1 input name"));
	settings->Add(audioNameSetting[1] = new EnumeratedStringValueSetting("AudioInput2", "Internal Jack", kAudioSourceName,
			"audio2 input name expected", "unrecognized audio2 input name"));
	settings->Add(audioNameSetting[2] = new EnumeratedStringValueSetting("AudioInput3", "External Jack", kAudioSourceName,
			"audio3 input name expected", "unrecognized audio3 input name"));
	settings->Add(audioNameSetting[3] = new EnumeratedStringValueSetting("AudioInput4", "Mute", kAudioSourceName,
			"audio4 input name expected", "unrecognized audio4 input name"));

	settings->Add(audioModeSetting = new EnumeratedStringValueSetting("AudioMode", "Main", kAudioMode,
			"audio mode expected", "unrecognized audio mode specified"));
	settings->Add(mspModeSetting = new EnumeratedStringValueSetting("MspMode", "US NTSC FM", kMspMode,
			"msp mode expected", "unrecognized msp mode specified"));
			
	settings->Add(tunerBrandSetting = new EnumeratedStringValueSetting("TunerBrand", "Philips", kTunerBrand,
			"tuner brand expected", "unrecognized tuner brand specified"));
	settings->Add(formatSetting = new EnumeratedStringValueSetting("Format", "NTSC-M", kVideoFormat,
			"format specification expected", "unrecognized format specified"));
	settings->Add(tunerLocaleSetting = new EnumeratedStringValueSetting("RFLocale", "US Cable", kTunerLocale,
			"RF locale specification expected", "unrecognized RF locale specified"));

	settings->Add(imageSizeSetting = new EnumeratedStringValueSetting("ImageSize", "320x240", kImageSize,
			"image size specification expected", "unrecognized image size specified"));
	settings->Add(colorspaceSetting = new EnumeratedStringValueSetting("Colors", "16 Bits/Pixel", kColorspace,
			"colors specification expected", "unrecognized colors specified"));

	settings->Add(videoSourceSetting = new ScalarValueSetting("VideoSource", 2,
			"video source expected", "video source out of bounds", 0, 3));
	settings->Add(audioSourceSetting = new ScalarValueSetting("AudioSource", 0,
			"audio source expected", "audio source out of bounds", 0, 3));
	settings->Add(audioMuxTypeSetting = new ScalarValueSetting("AudioMuxType", 0,
			"audio mux type expected", "audio mux type out of bounds", 0, 21));

	settings->Add(brightnessSetting = new ScalarValueSetting("Brightness", 0,
			"brightness expected", "brightness out of bounds", -100, 100));
	settings->Add(contrastSetting = new ScalarValueSetting("Contrast", 0,
			"contrast expected", "contrast out of bounds", -100, 100));
	settings->Add(saturationSetting = new ScalarValueSetting("Saturation", 0,
			"saturation expected", "saturation out of bounds", -100, 100));
	settings->Add(hueSetting = new ScalarValueSetting("Hue", 0,
			"hue expected", "hue out of bounds", -100, 100));

	settings->Add(pllSetting = new BooleanValueSetting("PLL", 0));
	settings->Add(gammaSetting = new BooleanValueSetting("GammaCorrection", 1));
	settings->Add(lumaCoringSetting = new BooleanValueSetting("LumaCoring", 1));
	settings->Add(lumaFilterSetting = new BooleanValueSetting("LumaCombFilter", 0));
	settings->Add(chromaFilterSetting = new BooleanValueSetting("ChromaCombFilter", 1));
	settings->Add(errorDiffusionSetting = new BooleanValueSetting("ErrorDifusion", 1));
			
	settings->Add(channelSetting = new StringValueSetting("Channel", "5",
		"channel name expected", ""));

	settings->TryReadingSettings();
}

void 
BBt848Controllable::QuitSettings()
{
	FUNCTION("BBt848Controllable::QuitSettings\n");
	if(settings) {
		settings->SaveSettings(false);
		delete settings;
	}
}

int32 
BBt848Controllable::FindIndex(const char *setting_table[], const char *setting)
{
	for ( int32 i = 0; setting_table[i] != 0; i++)
	{
		if (strcmp(setting_table[i], setting) == 0)
		{
			return i;
		}
	}
	return 0;
}

void 
BBt848Controllable::RequestNewWeb()
{
	ConstructControlWeb();
}

void 
BBt848Controllable::ConstructControlWeb()
{
//	uint32 i;
	
	FUNCTION("Bt848Controllable::ConstructControlWeb - BEGIN\n");
	
	// Create a fresh web
	BParameterWeb *web = new BParameterWeb();	

	// Control tab
	BParameterGroup *controlGroup = web->MakeGroup("Controls");	
									
		BParameterGroup *inputGroup = controlGroup->MakeGroup("Inputs");
			BDiscreteParameter * channel = inputGroup->MakeDiscreteParameter(B_CONTROL_CHANNEL, B_MEDIA_NO_TYPE, "Channel:", B_TUNER_CHANNEL);
				if ((mTuner->TunerLocale() == B_FM_RADIO) && mHasFmFavorites)
				{
					int i = 0;
					while (mFmFavorites[i].frequency != 0)
					{
						channel->AddItem(i, mFmFavorites[i].name);
						i++;
					}
				}
				else
				{
					if (mHasTvFavorites)
					{
						int i = 0;
						while (mTvFavorites[i].frequency != 0)
						{
							channel->AddItem(i, mTvFavorites[i].name);
							i++;
						}
					}
					else
					{
						for (int i = 0; i < (int)mTuner->NumberChannels(); i++)
							channel->AddItem(i, mTuner->ChannelNameForIndex(i));
					}					
				}

			BDiscreteParameter * videoInput = inputGroup->MakeDiscreteParameter(B_CONTROL_VIDEO_INPUT, B_MEDIA_NO_TYPE, "Video Input:", B_INPUT_MUX);
				videoInput->AddItem(0, kVideoSourceName[mVideoInput[0]]);
				videoInput->AddItem(1, kVideoSourceName[mVideoInput[1]]);
				videoInput->AddItem(2, kVideoSourceName[mVideoInput[2]]);
				videoInput->AddItem(3, kVideoSourceName[mVideoInput[3]]);
				
			if (!mHitachi)
			{
				BDiscreteParameter * audioInput = inputGroup->MakeDiscreteParameter(B_CONTROL_AUDIO_INPUT, B_MEDIA_NO_TYPE, "Audio Input:", B_INPUT_MUX);
					audioInput->AddItem(0, kAudioSourceName[mAudioInput[0]]);
					audioInput->AddItem(1, kAudioSourceName[mAudioInput[1]]);
					audioInput->AddItem(2, kAudioSourceName[mAudioInput[2]]);
					audioInput->AddItem(3, kAudioSourceName[mAudioInput[3]]);
			}
			else
			{
				BDiscreteParameter * audioMode = inputGroup->MakeDiscreteParameter(B_CONTROL_AUDIO_MODE, B_MEDIA_NO_TYPE, "Audio:", B_INPUT_MUX);
					for (int i = 0; kAudioMode[i] != 0; i++)
						audioMode->AddItem(i,kAudioMode[i]);
			}

		BParameterGroup *lumaGroup = controlGroup->MakeGroup("Luma");
			/*BContinuousParameter * brightness =*/ lumaGroup->MakeContinuousParameter(B_CONTROL_BRIGHTNESS,
				 B_MEDIA_NO_TYPE, "Brightness", "BRIGHTNESS", "", -100, 100, 1);
			/*BContinuousParameter * contrast =*/ lumaGroup->MakeContinuousParameter(B_CONTROL_CONTRAST,
				 B_MEDIA_NO_TYPE, "Contrast", "CONTRAST", "", -100, 100, 1);
		BParameterGroup *chromaGroup = controlGroup->MakeGroup("Chroma");
			/*BContinuousParameter * saturation =*/ chromaGroup->MakeContinuousParameter(B_CONTROL_SATURATION,
				 B_MEDIA_NO_TYPE, "Saturation", "SATURATION", "", -100, 100, 1);
			// Hue control is only possible for NTSC
			if ((mDevice) &&
				((mDevice->VideoFormat() == B_NTSC_M) ||
				 (mDevice->VideoFormat() == B_NTSC_J)))
			{
				/*BContinuousParameter * hue =*/ chromaGroup->MakeContinuousParameter(B_CONTROL_HUE,
					 B_MEDIA_NO_TYPE, "Hue", "HUE", "", -100, 100, 1);
			}

	// Options Setup tab
	BParameterGroup *optionsSetupGroup = web->MakeGroup("Options");
		
		BParameterGroup *nodeDefaultsGroup = optionsSetupGroup->MakeGroup("Node Defaults");			
			BDiscreteParameter * defaultSize = nodeDefaultsGroup->MakeDiscreteParameter(B_CONTROL_DEFAULT_SIZE, B_MEDIA_RAW_VIDEO, "Default Image Size:", B_RESOLUTION);
				for (int i = 0; kImageSize[i] != 0; i++)
					defaultSize->AddItem(i,kImageSize[i]);
			BDiscreteParameter * defaultColorspace = nodeDefaultsGroup->MakeDiscreteParameter(B_CONTROL_DEFAULT_COLORSPACE, B_MEDIA_RAW_VIDEO, "Default Colors:", B_COLOR_SPACE);
				for (int i = 0; kColorspace[i] != 0; i++)
					defaultColorspace->AddItem(i,kColorspace[i]);

		BParameterGroup *filterGroup = optionsSetupGroup->MakeGroup("Image Filters");		
			if (mDevice && mDevice->DeviceID() == 848)
				// luma comb only on the 848
				/*BDiscreteParameter * lumaCombFilter =*/ filterGroup->MakeDiscreteParameter(B_CONTROL_LUMA_COMB, B_MEDIA_NO_TYPE, "Luma Comb", B_ENABLE);
			/*BDiscreteParameter * lumaCoring =*/ filterGroup->MakeDiscreteParameter(B_CONTROL_LUMA_CORING, B_MEDIA_NO_TYPE, "Luma Coring", B_ENABLE);
			/*BDiscreteParameter * chromaCombFilter =*/ filterGroup->MakeDiscreteParameter(B_CONTROL_CHROMA_COMB, B_MEDIA_NO_TYPE, "Chroma Comb", B_ENABLE);
			/*BDiscreteParameter * gammaCorrection =*/ filterGroup->MakeDiscreteParameter(B_CONTROL_GAMMA_CORRECTION, B_MEDIA_NO_TYPE, "Gamma", B_ENABLE);
			/*BDiscreteParameter * errorDiffusion =*/ filterGroup->MakeDiscreteParameter(B_CONTROL_ERROR_DIFFUSION, B_MEDIA_NO_TYPE, "Error Diffusion", B_ENABLE);

					
	// Names Setup tab
	BParameterGroup *namingSetupGroup = web->MakeGroup("Input Names");
	if (!mHitachi)
	{
		
		BParameterGroup *videoMuxGroup = namingSetupGroup->MakeGroup("Video Input Names");
			BDiscreteParameter * video1 = videoMuxGroup->MakeDiscreteParameter(B_CONTROL_VIDEO_INPUT_1, B_MEDIA_NO_TYPE, "Video 1 is:", B_INPUT_MUX);
				for (int i = 0; kVideoSourceName[i] != 0; i++)
					video1->AddItem(i,kVideoSourceName[i]);
			BDiscreteParameter * video2 = videoMuxGroup->MakeDiscreteParameter(B_CONTROL_VIDEO_INPUT_2, B_MEDIA_NO_TYPE, "Video 2 is:", B_INPUT_MUX);
				for (int i = 0; kVideoSourceName[i] != 0; i++)
					video2->AddItem(i,kVideoSourceName[i]);
			BDiscreteParameter * video3 = videoMuxGroup->MakeDiscreteParameter(B_CONTROL_VIDEO_INPUT_3, B_MEDIA_NO_TYPE, "Video 3 is:", B_INPUT_MUX);
				for (int i = 0; kVideoSourceName[i] != 0; i++)
					video3->AddItem(i,kVideoSourceName[i]);
			BDiscreteParameter * video4 = videoMuxGroup->MakeDiscreteParameter(B_CONTROL_VIDEO_INPUT_4, B_MEDIA_NO_TYPE, "Video 4 is:", B_INPUT_MUX);
				for (int i = 0; kVideoSourceName[i] != 0; i++)
					video4->AddItem(i,kVideoSourceName[i]);
	}

	BParameterGroup *audioMuxGroup = namingSetupGroup->MakeGroup("Audio Input Names");
		BDiscreteParameter * audio1 = audioMuxGroup->MakeDiscreteParameter(B_CONTROL_AUDIO_INPUT_1, B_MEDIA_NO_TYPE, "Audio 1 is:", B_INPUT_MUX);
			for (int i = 0; kAudioSourceName[i] != 0; i++)
				audio1->AddItem(i,kAudioSourceName[i]);
		BDiscreteParameter * audio2 = audioMuxGroup->MakeDiscreteParameter(B_CONTROL_AUDIO_INPUT_2, B_MEDIA_NO_TYPE, "Audio 2 is:", B_INPUT_MUX);
			for (int i = 0; kAudioSourceName[i] != 0; i++)
				audio2->AddItem(i,kAudioSourceName[i]);
		BDiscreteParameter * audio3 = audioMuxGroup->MakeDiscreteParameter(B_CONTROL_AUDIO_INPUT_3, B_MEDIA_NO_TYPE, "Audio 3 is:", B_INPUT_MUX);
			for (int i = 0; kAudioSourceName[i] != 0; i++)
				audio3->AddItem(i,kAudioSourceName[i]);
		BDiscreteParameter * audio4 = audioMuxGroup->MakeDiscreteParameter(B_CONTROL_AUDIO_INPUT_4, B_MEDIA_NO_TYPE, "Audio 4 is:", B_INPUT_MUX);
			for (int i = 0; kAudioSourceName[i] != 0; i++)
				audio4->AddItem(i,kAudioSourceName[i]);

	// Hardware Setup tab
	BParameterGroup *hwGroup = web->MakeGroup("Hardware Setup");

		BParameterGroup *videoGroup = hwGroup->MakeGroup("Hardware");			
			BDiscreteParameter * videoFormat = videoGroup->MakeDiscreteParameter(B_CONTROL_VIDEO_FORMAT, B_MEDIA_RAW_VIDEO, "Video Format:", B_VIDEO_FORMAT);
				for (int i = 0; kVideoFormat[i] != 0; i++)
					videoFormat->AddItem(i,kVideoFormat[i]);
			BDiscreteParameter * tunerBrand = videoGroup->MakeDiscreteParameter(B_CONTROL_TUNER_BRAND, B_MEDIA_NO_TYPE, "Tuner Brand: ", B_INPUT_MUX);	
				for (int i = 0; kTunerBrand[i] != 0; i++)
					tunerBrand->AddItem(i,kTunerBrand[i]);
			BDiscreteParameter * tunerLocale = videoGroup->MakeDiscreteParameter(B_CONTROL_TUNER_LOCALE, B_MEDIA_NO_TYPE, "Tuner Locale:", B_INPUT_MUX);	
				for (int i = 0; kTunerLocale[i] != 0; i++)
					tunerLocale->AddItem(i,kTunerLocale[i]);

			if ((mDevice) &&
				(!(mDevice->VideoFormat() == B_NTSC_M) &&
				 !(mDevice->VideoFormat() == B_NTSC_J)))
				/*BDiscreteParameter * pllEnable =*/ videoGroup->MakeDiscreteParameter(B_CONTROL_PLL, B_MEDIA_NO_TYPE, "Use Phase Lock Loop", B_ENABLE);

		BParameterGroup *debugGroup = hwGroup->MakeGroup("Debug Info");			
		//BNullParameter * x = debugGroup->MakeNullParameter(B_CONTROL_BOGUS, B_MEDIA_NO_TYPE, "Hardware Detected on your card", B_WEB_LOGICAL_INPUT);
			BDiscreteParameter * audioMuxType = debugGroup->MakeDiscreteParameter(B_CONTROL_AUDIO_MUX_TYPE, B_MEDIA_NO_TYPE, "Audio Mux Type:", B_INPUT_MUX);
				for (int i = 0; i < 23; i++)
				{
					char s[32];
					sprintf(s,"%d", i);
					audioMuxType->AddItem(i,s);			
				}
			if (mI2CBus->I2CDevicePresent(0x40))
			{			
				BDiscreteParameter * mspMode = debugGroup->MakeDiscreteParameter(B_CONTROL_MSP_MODE, B_MEDIA_NO_TYPE, "Audio Format: ", B_INPUT_MUX);	
					for (int i = 0; kMspMode[i] != 0; i++)
						mspMode->AddItem(i,kMspMode[i]);
			}

			BDiscreteParameter * I2CDevices = debugGroup->MakeDiscreteParameter(B_CONTROL_I2C_DEVICES, B_MEDIA_NO_TYPE, "Debug Info:", B_INPUT_MUX);
				char s[32];
				sprintf(s,"Bt%ld", mDevice->DeviceID());
				I2CDevices->AddItem(0,s);
			
				if (!mHitachi)
				{
					for (int i = 1; i < 127; i++)
					{
						if (mI2CBus && mI2CBus->I2CDevicePresent(i))
						{
							switch (i)
							{
								case 0x40:
									sprintf(s,"MSP34XX @ %02x", i);
									break;
							
								case 0x41:
									sprintf(s,"TDA8425 @ %02x", i);
									break;
							
								case 0x50:
									sprintf(s,"EEPROM @ %02x", i);
									break;
							
								case 0x5b:
									sprintf(s,"TDA9850 @ %02x", i);
									break;
							
								case 0x60:
								case 0x61:
								case 0x62:
								case 0x63:
									sprintf(s,"Tuner @ %02x", i);
									break;
								default:
									sprintf(s,"Unknown @ %02x", i);
									break;
							}
							I2CDevices->AddItem(i,s);
						}
					}
				}						
		
	SetParameterWeb(web);

	FUNCTION("Bt848Controllable::ConstructControlWeb - END\n");
}

void 
BBt848Controllable::Initialize()
{
	FUNCTION("BBt848Controllable_Initialize - BEGIN\n");

	if ((mInitCheck == B_OK) && mDevice)
	{		
		// initialize ring buffers for F1 & F2
		mRingBufferF1 = mBufferF1;
		mRingBufferF2 = mBufferF2;
		for (uint32 i = 0; i < MAX_BUFFERS; i++)
		{
			mBufferF1[i] = NULL;
			mBufferF2[i] = NULL;
		}
		
		mHasTvFavorites = ReadTvFavorites();
		mHasFmFavorites = ReadFmFavorites();

		mTuner = mDevice->Tuner();
		mAudioMux = mDevice->AudioMux();
		mVideoMux = mDevice->VideoMux();
		mVideoControls = mDevice->VideoControls();
		mI2CBus = mDevice->I2CBus();
		
		// test for Hitachi
		reset_hitachi(mDevice);
	
		if (read_hitachi(mDevice, 0) == 0x00)
		{
			mHitachi = true;
			mDevice->GpioOutEnable(0xf0001f);
			mDevice->SetGpio(0x00001f);

			write_hitachi(mDevice, 0, 0x20);
			write_hitachi(mDevice, 0, 0xb6);
			write_hitachi(mDevice, 1, 0x66);
			write_hitachi(mDevice, 1, 0x87);
			write_hitachi(mDevice, 2, 0x0f);
			write_hitachi(mDevice, 3, 0xff);
			// mute audio
			mDevice->SetGpio(0x20001f);
			
			mAudioMux->SetShift(32);
		}
		else
		{
			mHitachi = false;
			mDevice->GpioOutEnable(0xffffff);

			// hw reset MSP34XX
			mDevice->SetGpio(0x000020);
			mDevice->SetGpio(0x000000);
			snooze(2500);
			mDevice->SetGpio(0x000020);

			snooze(2500);
			if (mI2CBus->I2CDevicePresent(0x40))
			{
				msp_reset(mI2CBus);
				mMspMode = FindIndex(kMspMode, mspModeSetting->Value());
				msp_setmode(mI2CBus, mMspMode);
				msp_setvolume(mI2CBus, 65535, 65535);
			}
		}
		
				
		// set controls from settings file

		mDevice->SetPll(pllSetting->Value());
		mDevice->SetVideoFormat((video_format)FindIndex(kVideoFormat, formatSetting->Value()));

		if (mTuner)
		{
			((Bt848Source *)mDevice)->SetTunerBrand((bt848_tuner_mfg)FindIndex(kTunerBrand, tunerBrandSetting->Value()));
			mTuner->SetTunerLocale((tuner_locale)FindIndex(kTunerLocale, tunerLocaleSetting->Value()));

			if ((mTuner->TunerLocale() == B_FM_RADIO) && mHasFmFavorites)
			{
				mFmFavoritesIndex = 0;
				mTuner->Tune(mFmFavorites[mFmFavoritesIndex].frequency);
			}
			else
			{
				if (mHasTvFavorites)
				{
					mTvFavoritesIndex = 0;
					mTuner->Tune(mTvFavorites[mTvFavoritesIndex].frequency);
				}
				else
					mTuner->Tune((char *)channelSetting->Value());
			}
		}
		
		if (mVideoControls)
		{
			mVideoControls->SetBrightness(brightnessSetting->Value());
			mVideoControls->SetContrast(contrastSetting->Value());
			mVideoControls->SetSaturation(saturationSetting->Value());
			mVideoControls->SetHue(hueSetting->Value());
			mVideoControls->SetGammaCorrectionRemoval(gammaSetting->Value());
			mVideoControls->SetErrorDiffusion(errorDiffusionSetting->Value());
			mVideoControls->SetLumaCoring(lumaCoringSetting->Value());
			mVideoControls->SetLumaCombFilter(lumaFilterSetting->Value());
			mVideoControls->SetChromaCombFilter(chromaFilterSetting->Value());
		}
		
		if (mVideoMux)
		{
			// set up labels for video inputs
			for (int i = 0; i < 4; i++)
				mVideoInput[i] = (EVideoInputName)FindIndex(kVideoSourceName, videoNameSetting[i]->Value());
			mVideoMux->SetSource(videoSourceSetting->Value());
		}
			

		if (!mHitachi && mAudioMux)
		{

			mAudioMuxType = audioMuxTypeSetting->Value();
			mAudioMux->SetShift(mAudioMuxType);
			
			// set up labels for audio inputs
			for (int i = 0; i < 4; i++)
			{
				mAudioInput[i] = (EAudioInputName)FindIndex(kAudioSourceName, audioNameSetting[i]->Value());
				// note which input is mute for later use
				if (strcmp(kAudioSourceName[1], audioNameSetting[i]->Value()) == 0)
					mAudioMux->SetMute(i);
			}
				
			// remember the audio setting
			mAudioSource = audioSourceSetting->Value();
			
			// and mute
			mAudioMux->Mute();
		}					
			
		mAudioMode = FindIndex(kAudioMode, audioModeSetting->Value());
			
		// Setup the default raw_video format
		// If these are changed at all, then careful
		// attention must be paid to the ConfigureCapture() method
	
		switch(FindIndex(kImageSize, imageSizeSetting->Value()))
		{
			case 0:
				mXSize = 768;
				mYSize = 576;
				mInterlaced = true;
				break;
			case 1:
				mXSize = 720;
				mYSize = 576;
				mInterlaced = true;
				break;
			case 2:
				mXSize = 720;
				mYSize = 480;
				mInterlaced = true;
				break;
			case 3:
				mXSize = 640;
				mYSize = 480;
				mInterlaced = true;
				break;
			case 4:
				mXSize = 352;
				mYSize = 240;
				mInterlaced = false;
				break;
			case 5:
				mXSize = 320;
				mYSize = 240;
				mInterlaced = false;
				break;
			case 6:
				mXSize = 160;
				mYSize = 120;
				mInterlaced = false;
				break;
			default:
				mXSize = 320;
				mYSize = 240;
				mInterlaced = false;
				break;
		}
		
		switch(FindIndex(kColorspace, colorspaceSetting->Value()))
		{
			case 0:
				mColorspace = B_GRAY8;
				break;
			case 1:
				mColorspace = B_RGB15;
				break;
			case 2:
				mColorspace = B_RGB16;
				break;
			case 3:
				mColorspace = B_RGB32;
				break;
			default:
				mColorspace = B_RGB16;
				break;
		}

		mQueryFormat.type = B_MEDIA_RAW_VIDEO;
		mQueryFormat.u.raw_video = media_raw_video_format::wildcard;
	
		mVideoFormat.type = B_MEDIA_RAW_VIDEO;
		mVideoFormat.u.raw_video.field_rate = media_raw_video_format::wildcard.field_rate;
		mVideoFormat.u.raw_video.display.format = mColorspace;
		mVideoFormat.u.raw_video.interlace = mInterlaced ? 2 : 1;
		mVideoFormat.u.raw_video.first_active = 0;
		mVideoFormat.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		mVideoFormat.u.raw_video.pixel_width_aspect = 1;
		mVideoFormat.u.raw_video.pixel_height_aspect = 1;
		mVideoFormat.u.raw_video.display.line_width = mXSize; 
		mVideoFormat.u.raw_video.display.line_count = mYSize;
		mVideoFormat.u.raw_video.display.bytes_per_row = 
			(mVideoFormat.u.raw_video.display.line_width *
			bitsPerPixel(mVideoFormat.u.raw_video.display.format)+4)/8;
		mVideoFormat.u.raw_video.display.pixel_offset = 0;
		mVideoFormat.u.raw_video.display.line_offset = 0;	

		// build a default media_output for each potential connection
		mOutputF1.node = Node();
		mOutputF1.source.port = ControlPort();
		mOutputF1.source.id = 1;
		FormatSuggestionRequested(B_MEDIA_RAW_VIDEO, 0, &mOutputF1.format);
		// allocate storage for clip_list
		for (int j = 0; j < B_VIDEO_V_MAX; j++)
			mClipListF1[j] = new int16[MAX_CLIP_LINE];	
		// Make sure the cliplist represents this region	
//		BRect frame1(0,0,
//			mVideoFormat.u.raw_video.display.line_width - 1,
//			mVideoFormat.u.raw_video.display.line_count - 1);
		mClipRegion.Include(BRect(0,0,
			mVideoFormat.u.raw_video.display.line_width - 1,
			mVideoFormat.u.raw_video.display.line_count - 1));
		//ClipListFromRegion(&mClipRegion, frame1, mClipListF1);
		
		// build a default media_output for each potential connection
		mOutputF2.node = Node();
		mOutputF2.source.port = ControlPort();
		mOutputF2.source.id = 2;
		FormatSuggestionRequested(B_MEDIA_RAW_VIDEO, 0, &mOutputF2.format);
		// allocate storage for clip_list
		for (int j = 0; j < B_VIDEO_V_MAX; j++)
			mClipListF2[j] = new int16[MAX_CLIP_LINE];	
		// Make sure the cliplist represents this region	
//		BRect frame2(0,0,
//			mVideoFormat.u.raw_video.display.line_width - 1,
//			mVideoFormat.u.raw_video.display.line_count - 1);
		mClipRegion.Include(BRect(0,0,
			mVideoFormat.u.raw_video.display.line_width - 1,
			mVideoFormat.u.raw_video.display.line_count - 1));
		//ClipListFromRegion(&mClipRegion, frame2, mClipListF2);		
	
	}
	else
	{
		ERROR("BBt848Controllable::Initialize - COULDN'T INITIALIZE NODE\n");
	}
	
	FUNCTION("BBt848Controllable::Initialize - END\n");
}

void 
BBt848Controllable::Uninitialize()
{
	FUNCTION("BBt848Controllable::Uninitialize - BEGIN\n");

	for (int i = 0; i < B_VIDEO_V_MAX; i++)
		delete [] mClipListF1[i];
		
	for (int i = 0; i < B_VIDEO_V_MAX; i++)
		delete [] mClipListF2[i];
		
}

//---------------------------------------------------------------

static uint32
bitsPerPixel(uint32 cSpace)
{
	uint32 results=0;
	
	switch(cSpace)
	{
		case B_RGB32:
		case B_RGBA32:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
			return 32;
		break;
		
		case B_RGB24:
		case B_RGB24_BIG:
			return 24;
		break;
		
		case B_RGB16:
		case B_RGB15:
		case B_RGBA15:
		case B_RGB16_BIG:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
			return 16;

		case B_YCbCr422:
			return 16;
		break;
		
		case B_YCbCr411:
			return 12;
		break;
		
		case B_CMAP8:
		case B_GRAY8:
			return 8;
		break;
	}
	
	return results;
}

//---------------------------------------------------------------

#if 0
static void
PrintVideoDisplay(const struct media_video_display_info &display)
{
	printf("Display Information\n");
	printf("      Color: %d\n", display.format);
	printf("      Width: %d\n", display.line_width);
	printf("     Height: %d\n", display.line_count);
	printf("       XOff: %d\n", display.pixel_offset);
	printf("       YOff: %d\n", display.line_offset);
	printf(" Bytes Per Row: %d\n", display.bytes_per_row);
}
#endif

//---------------------------------------------------------------

#if DEBUG
static void
PrintVideoFormat(const media_raw_video_format aFormat)
{
	printf("Raw Video Format\n");
	printf(" Field Rate: %g\n", aFormat.field_rate);
	printf("      Color: 0x%04x\n", aFormat.display.format);
	printf("  Interlace: %d\n", aFormat.interlace);
	printf("      Width: %d\n", aFormat.display.line_width);
	printf("     Height: %d\n", aFormat.display.line_count);
	printf("      First: %d\n", aFormat.first_active);
	printf("       Last: %d\n", aFormat.last_active);
	printf("     Orient: %d\n", aFormat.orientation);
	printf(" Bytes Per Row: %d\n", aFormat.display.bytes_per_row);
	printf("  Width Aspect: %d\n", aFormat.pixel_width_aspect);
	printf(" Height Aspect: %d\n", aFormat.pixel_height_aspect);
}
#endif

//---------------------------------------------------------------

struct msp_dem_init {
	int32 fir1[6];
	int32 fir2[6];
	int32 dco1;
	int32 dco2;
	int32 ad_cv;
	int32 mode_reg;
	int32 dfp_src;
	int32 dfp_matrix;
};

//---------------------------------------------------------------

#define MSP_CARRIER(freq) ((int)((float)(freq/18.432)*(1<<24)))

static msp_dem_init msp_init_data[] = {
	/* US Terrestial FM-mono and FM-stereo */
	{ {  3, 18, 27, 48, 66, 72 }, {  3, 18, 27, 48, 66, 72 },
	  MSP_CARRIER(4.72421), MSP_CARRIER(4.5),
	  0x00d0, 0x0480,   0x0000, 0x3000},

	/* Terrestial FM-mono and FM-stereo*/
	{ {  3, 18, 27, 48, 66, 72 }, {  3, 18, 27, 48, 66, 72 },
	  MSP_CARRIER(5.74218), MSP_CARRIER(5.5),
	  0x00d0, 0x0480,   0x0000, 0x3000},

	/* NICAM B/G, D/K */
	{ { -2, -8, -10, 10, 50, 86 }, {  3, 18, 27, 48, 66, 72 },
	  MSP_CARRIER(5.5), MSP_CARRIER(5.5),
	  0x00d0, 0x0040,   0x0120, 0x3000},

	/* NICAM I */
	{ {  2, 4, -6, -4, 40, 94 }, {  3, 18, 27, 48, 66, 72 },
	  MSP_CARRIER(6.0), MSP_CARRIER(6.0),
	  0x00d0, 0x0040,   0x0120, 0x3000},
	  
	/* FM Radio */
	{ { -8, -8, 4, 6, 78, 107 }, { -8, -8, 4, 6, 78, 107 },
	  MSP_CARRIER(10.7), MSP_CARRIER(10.7),
	  0x00d0, 0x0480, 0x0020, 0x3000 },

	/* Sat FM-mono */
	{ {  1,  9, 14, 24, 33, 37 }, {  3, 18, 27, 48, 66, 72 },
	  MSP_CARRIER(6.5), MSP_CARRIER(6.5),
	  0x00c6, 0x0480,   0x0000, 0x3000},

	/* AM (for carrier detect / msp3400) */
	{ { 75, 19, 36, 35, 39, 40 }, { 75, 19, 36, 35, 39, 40 },
	  MSP_CARRIER(5.5), MSP_CARRIER(5.5),
	  0x00d0, 0x0500,   0x0020, 0x3000},

	/* AM (for carrier detect / msp3410) */
	{ { -1, -1, -8, 2, 59, 126 }, { -1, -1, -8, 2, 59, 126 },
	  MSP_CARRIER(5.5), MSP_CARRIER(5.5),
	  0x00d0, 0x0100,   0x0020, 0x3000},

};

//---------------------------------------------------------------

static void
msp_reset(BI2CBus *bus)
{	
	if (bus->I2CDevicePresent(0x40))
	{
		PROGRESS("MSP FOUND!\n");
		snooze(2000);
		bus->I2CStart();
		bus->I2CSendByte(MSP,2000);
		bus->I2CSendByte(0x00,0);
		bus->I2CSendByte(0x80,0);
		bus->I2CSendByte(0x00,0);
		bus->I2CStop();
		snooze(2000);
	
		bus->I2CStart();
		bus->I2CSendByte(MSP,2000);
		bus->I2CSendByte(0x00,0);
		bus->I2CSendByte(0x00,0);
		bus->I2CSendByte(0x00,0);
		bus->I2CStop();
		snooze(2000);
	}
}

//---------------------------------------------------------------

#if 0
static uint16
msp_read(BI2CBus *bus, uint16 device, uint16 address)
{
	uint16 data = 0;
	
	bus->I2CStart();
	bus->I2CSendByte(MSP,2000);
	bus->I2CSendByte(device + 1    , 0);
	bus->I2CSendByte(address >> 8  , 0);
	bus->I2CSendByte(address & 0xff, 0);
	bus->I2CStart();
	bus->I2CSendByte(MSP+1,2000);
	data |= bus->I2CReadByte(false) << 8;
	data |= bus->I2CReadByte(true);
	bus->I2CStop();

	return data;
}
#endif

//---------------------------------------------------------------

static void
msp_write(BI2CBus *bus, uint16 device, uint16 address, uint data)
{

	bus->I2CStart();
	bus->I2CSendByte(MSP           ,2000);
	bus->I2CSendByte(device        , 0);
	bus->I2CSendByte(address >> 8  , 0);
	bus->I2CSendByte(address & 0xff, 0);
	bus->I2CSendByte(data >> 8     , 0);
	bus->I2CSendByte(data & 0xff   , 0);
	bus->I2CStop();
}

//---------------------------------------------------------------

static void
msp_setcarrier(BI2CBus *bus, int dco1, int dco2)
{
	msp_write(bus, MSP_DEM, 0x0093, dco1 & 0xfff);
	msp_write(bus, MSP_DEM, 0x009b, dco1 >> 12);
	msp_write(bus, MSP_DEM, 0x00a3, dco2 & 0xfff);
	msp_write(bus, MSP_DEM, 0x00ab, dco2 >> 12);
}

//---------------------------------------------------------------

static void
msp_setmode(BI2CBus *bus, uint16 type)
{
	msp_write(bus,MSP_DEM, 0x00bb,			/* ad_cv */
			  msp_init_data[type].ad_cv);
    
	for (int i = 5; i >= 0; i--)			/* fir 1 */
		msp_write(bus,MSP_DEM, 0x0001,
			      msp_init_data[type].fir1[i]);
    
	msp_write(bus,MSP_DEM, 0x0005, 0x0004); /* fir 2 */
	msp_write(bus,MSP_DEM, 0x0005, 0x0040);
	msp_write(bus,MSP_DEM, 0x0005, 0x0000);
	for (int i = 5; i >= 0; i--)
		msp_write(bus,MSP_DEM, 0x0005,
			       msp_init_data[type].fir2[i]);
    
	msp_write(bus,MSP_DEM, 0x0083,     /* MODE_REG */
		       msp_init_data[type].mode_reg);
    
	msp_setcarrier(bus, msp_init_data[type].dco1,
			    msp_init_data[type].dco2);
    
	msp_write(bus,MSP_DEM, 0x0056, 0); /*LOAD_REG_1/2*/

	msp_write(bus, MSP_DFP, 0x0008,
		       msp_init_data[type].dfp_src);
	msp_write(bus, MSP_DFP, 0x0009,
		       msp_init_data[type].dfp_src);
	msp_write(bus, MSP_DFP, 0x000a,
		       msp_init_data[type].dfp_src);
	msp_write(bus, MSP_DFP, 0x000e,
		       msp_init_data[type].dfp_matrix);

	if (type == MSP_MODE_PAL_BG_NICAM ||
		type == MSP_MODE_PAL_I_NICAM)
		msp_write(bus, MSP_DFP, 0x0010, 0x3000);
}

//---------------------------------------------------------------

static void
msp_setvolume(BI2CBus *bus, int32 left, int32 right)
{
	int32 vol,val,balance;

	vol     = (left > right) ? left : right;
	val     = (vol * 0x73 / 65535) << 8;
	balance = 0;
	if (vol > 0)
		balance = ((right-left) * 127) / vol;

	msp_write(bus, MSP_DFP, 0x0000, val); /* loudspeaker */
	msp_write(bus, MSP_DFP, 0x0006, val); /* headphones  */
	msp_write(bus, MSP_DFP, 0x0007, val ? 0x4000 : 0);
	msp_write(bus, MSP_DFP, 0x0001, balance << 8);
}

//---------------------------------------------------------------

static void
reset_hitachi(Bt848Source *device)
{
	uint32	audio_setting = 0x200000;
	device->GpioOutEnable(0xf0001f);
	device->SetGpio(audio_setting | 0x1f);
	device->SetGpio(audio_setting | 0x17);
	snooze(10);
	device->SetGpio(audio_setting | 0x1f);
}

//---------------------------------------------------------------

static void
write_hitachi(Bt848Source *device, uint8 address, uint8 data)
{
	uint32	audio_setting = 0x200000;

	device->GpioOutEnable(0xf0ff1f);
	device->SetGpio(audio_setting | 0x1e | address << 8);
	device->SetGpio(audio_setting | 0x0e | address << 8);
	device->SetGpio(audio_setting | 0x0c | address << 8);
	snooze(10);
	device->SetGpio(audio_setting | 0x0e | address << 8);
	device->SetGpio(audio_setting | 0x1e);

	device->SetGpio(audio_setting | 0x1f | data << 8);
	device->SetGpio(audio_setting | 0x0f | data << 8);
	device->SetGpio(audio_setting | 0x0d | data << 8);
	snooze(10);
	device->SetGpio(audio_setting | 0x0f | data << 8);
	device->SetGpio(audio_setting | 0x1f);
	device->GpioOutEnable(0xf0001f);
}

//---------------------------------------------------------------

static uint8
read_hitachi(Bt848Source *device,  uint8 address)
{
	uint32	audio_setting = 0x200000;
	uint8 data;
	
	device->GpioOutEnable(0xf0ff1f);
	device->SetGpio(audio_setting | 0x1e | address << 8);
	device->SetGpio(audio_setting | 0x0e | address << 8);
	device->SetGpio(audio_setting | 0x0c | address << 8);
	snooze(10);
	device->SetGpio(audio_setting | 0x0e | address << 8);
	device->SetGpio(audio_setting | 0x1e | address << 8);
	device->GpioOutEnable(0xf0001f);

	device->SetGpio(audio_setting | 0x1f);
	device->SetGpio(audio_setting | 0x0f);
	device->SetGpio(audio_setting | 0x0b);
	snooze(10);
	data = (uint8) (device->Gpio() >> 8);
	device->SetGpio(audio_setting | 0x0f);
	device->SetGpio(audio_setting | 0x1f);
	return data;
}

