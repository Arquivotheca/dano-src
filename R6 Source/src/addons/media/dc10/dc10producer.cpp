#include "dc10producer.h"
#include "jpegdefs.h"

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

#include <ByteOrder.h>

#include <assert.h>
#include <malloc.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>



#define PRINTF(x...)
#define FUNCTION 	PRINTF 
#define ERROR		PRINTF
#define PROGRESS	PRINTF
#define PULSE		PRINTF
#define LOOP		PRINTF



static status_t
frame_ready_thread(void *data)
{
	status_t err = B_NO_ERROR;
	dc10_thread_info *ti = (dc10_thread_info *)data;
	uint32 i;
	dc10_config * pconfig;
	
	
	pconfig=(dc10_config *) ti->dc10;
		
	
	do {
		if(ti->running)
		{			
			pconfig->timeout = 100000;
			if(ioctl(pconfig->fd, DC10_SYNC, pconfig, sizeof(dc10_config)) < 0)
			{
				//error in ioctl
				FUNCTION("THREAD : erreur ioctl\n");
			}
			if(pconfig->capture_status ==  DC10_CAPTURE_OK) 
			{	
				if((pconfig->mEnabled) && (pconfig->mode == 1) )
				{	
					//FUNCTION("THREAD : buffer receive\n");
					if((pconfig->frame_number & 1) == 0)
					{
						err = write_port_etc(ti->port, DC10_FRAME_READY, pconfig, sizeof(dc10_config),
			            	B_TIMEOUT, 32000);
					}							
				}
			}
			else 
			{
				printf("frame_ready_thread: no frame\n");
				for(i=0;i<4;i++)
				{
					if((pconfig->buffer_info[i]&1)==1)
					{
						//printf("frame_ready_thread: no frame mais frame\n");
						pconfig->buffer_reset_index = i;
						if(ioctl(pconfig->fd, DC10_SET_BUFFER, pconfig, sizeof(dc10_config)) < 0)
						{
							//error in ioctl
							ERROR("dc10producer::HandleMessage ioctl error \n");
						}
					}
				}		
			}
			
			
		}
		else
		{
			write_port_etc(ti->port, DC10_SUSPEND_THREAD, pconfig, sizeof(dc10_config),B_TIMEOUT, 32000);
			
		}
	} while(!ti->quit);
		return B_NO_ERROR;
}


dc10producer::dc10producer(const uint32 internal_id, const char * devicename, const char *nodename, BMediaAddOn *addon) :
	BMediaNode(nodename),
	BMediaEventLooper(),
	BBufferProducer(B_MEDIA_RAW_VIDEO),
	BTimeSource(),
	mInternalID(internal_id),
	mAddOn(addon),
	fBufferGroup1(NULL),
	fBufferGroup(NULL)	
{
	FUNCTION("dc10producer::dc10producer- BEGIN\n");
	
	mMediaTime = 0;
	mDownstreamLatency = 20000;
	mVideoStandardIn = 0;
	
	SetPriority(B_URGENT_DISPLAY_PRIORITY);
	

	// Let the system know we do physical input
	AddNodeKind(B_PHYSICAL_INPUT);

	// create the dc10Source

	if (!devicename) {
		mName = strdup("/dev/video/dc10/0");
	}
	else 
	{
		mName = strdup(devicename);
	}
	fd = -1;
	/*
	fd=open(mName,O_RDWR);
	if(fd<0)
	{
		ERROR("dc10producer::dc10producer Erreur :: main ouverture device   ?%s?%s?%s?\n",mName,devicename,nodename);
		return ;
	}		
	else
	{
		PROGRESS("dc10producer::dc10producer OK :: %d \n",fd);
	}

	fCodec = new dc10codec();
	fCodec->setup(fd);
	fDevice = &fCodec->mDevice; 	
	*/
	
	fCodec = NULL;
	fDevice = NULL;
	mRate = 29.97;

    mInitCheck = B_OK;
	
	
	FUNCTION("dc10producer::dc10producer- END\n");
	return;


}




dc10producer::~dc10producer()
{
	FUNCTION("dc10producer::~dc10producer - BEGIN\n");
	
	
	fDevice = NULL;
	if(fd >= 0)
	{
		close(fd);
		fd = -1;
	}
	if(fCodec != NULL)
	{
		delete fCodec;
		fCodec = NULL;
	}
	Quit();
	FUNCTION("dc10producer::~dc10producer - END\n");
}


status_t dc10producer::InitCheck(const char ** /*outFailureText*/ = NULL)
{
	FUNCTION("dc10producer::initcheck\n");
	return B_OK;
}

//BControllable
void 
dc10producer::RequestNewWeb()
{
	ConstructControlWeb();
}

void 
dc10producer::ConstructControlWeb()
{	
	FUNCTION("dc10producer::ConstructControlWeb - BEGIN\n");
	
	// Create a fresh web
	BParameterWeb *web = new BParameterWeb();	

	// Control tab
	BParameterGroup *controlGroup = web->MakeGroup("Input");	
									
	BParameterGroup *inputGroup = controlGroup->MakeGroup("Inputs");
		

	BDiscreteParameter * videoInput = inputGroup->MakeDiscreteParameter(B_CONTROL_VIDEO_INPUT, 
																		B_MEDIA_NO_TYPE,
																		"Video Input:",
																		B_INPUT_MUX);
				videoInput->AddItem(0, "COMPOSITEE");
				videoInput->AddItem(1, "VHS");
	BDiscreteParameter * videoStandardIn = inputGroup->MakeDiscreteParameter(B_CONTROL_VIDEO_STANDARD_IN,
																			   B_MEDIA_NO_TYPE, 
																			   "Video Sandard:", 
																			   B_INPUT_MUX);
				videoStandardIn->AddItem(0, "NTSC");
				videoStandardIn->AddItem(1, "PAL/SECAM");		
	
	
	BContinuousParameter * videoBitRate = NULL;
	videoBitRate = inputGroup->MakeContinuousParameter(B_CONTROL_VIDEO_BIT_RATE, 
													   B_MEDIA_NO_TYPE,
													   " Compression Factor         :",
													   B_QUALITY,
													   "",
													   0,
													   1,
													   0.01);
	BContinuousParameter * videoBrightness = NULL;
	videoBrightness = inputGroup->MakeContinuousParameter(B_BRIGHTNESS, 
													   B_MEDIA_NO_TYPE,
													   "Brightness",
													   B_QUALITY,
													   "",
													   -100,
													   100,
													   0.01);
	BContinuousParameter * videoContrast = NULL;
	videoContrast = inputGroup->MakeContinuousParameter(B_CONTRAST, 
													   B_MEDIA_NO_TYPE,
													   "Contrast",
													   B_QUALITY,
													   "",
													   -100,
													   100,
													   0.01);
													   
	BContinuousParameter * videoSaturation = NULL;
	videoSaturation = inputGroup->MakeContinuousParameter(B_SATURATION, 
													   B_MEDIA_NO_TYPE,
													   "Saturation",
													   B_QUALITY,
													   "",
													   -100,
													   100,
													   0.01);	
																			   
	if(mVideoStandardIn == 0)
	{
		BContinuousParameter * videoHue = NULL;
		videoHue = inputGroup->MakeContinuousParameter(B_HUE, 
													   B_MEDIA_NO_TYPE,
													   "Hue",
													   B_QUALITY,
													   "",
													   -100,
													   100,
													   0.01);	
	}
	
	SetParameterWeb(web);

	float fvalue = 0.5;
	void * pvalue = (void *) (&fvalue);
	SetParameterValue(3,TimeSource()->Now(), pvalue, 4);
	fvalue = 0;
	SetParameterValue(4,TimeSource()->Now(), pvalue, 4);
	SetParameterValue(5,TimeSource()->Now(), pvalue, 4);
	SetParameterValue(6,TimeSource()->Now(), pvalue, 4);
	//SetParameterValue(3,TimeSource()->Now(), pvalue, 4);
	FUNCTION("dc10producer::ConstructControlWeb - END\n");
}

status_t 
dc10producer::GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *ioSize)
{
	ERROR("dc10producer::GetParameterValue - %d  Value: %d  Size: %d\n", id, *((int32 *)value), *ioSize);
	
	if (last_change)
		*last_change = bigtime_t(0);
		
	int32 *aValue = ((int32 *)value);
	float *floatValue = ((float *)value);
	
	*ioSize = sizeof(int32);
	
	switch(id)
	{
		case B_CONTROL_VIDEO_INPUT:
			*aValue = (int32)mVideoInput;
			*ioSize = sizeof(int32);			
		break;	
		case B_CONTROL_VIDEO_STANDARD_IN:
			*aValue = (int32)mVideoStandardIn;
			*ioSize = sizeof(int32);			
		break;		
		break;
		case B_CONTROL_VIDEO_BIT_RATE:
			*floatValue = (float)mBitRate;			
			*ioSize = sizeof(float);
		break;
		case B_BRIGHTNESS:
			*floatValue = (float) mBrightness;
			*ioSize = sizeof(float);
		break;
		case B_CONTRAST:
			*floatValue = (float) mContrast;
			*ioSize = sizeof(float);
		break;
		case B_SATURATION:
			*floatValue = (float) mSaturation;
			*ioSize = sizeof(float);
		break;
		case B_HUE:
			*floatValue = (float) mHue;
			*ioSize = sizeof(float);
		break;
		default:
			return B_BAD_VALUE;
		break;
	}
	
	return B_OK;
}

void 
dc10producer::SetParameterValue(int32 id, bigtime_t /*when*/, const void *value, size_t /*size*/)
{
	PROGRESS("dc10producer::SetParameterValue ID: %d  Value: %d %d\n",
		id, (int32)*((int32 *)value), (int32)*((float *)value));
		
	int32 aValue = *((int32 *)value);
	float floatValue = *((float *)value);
	
	switch(id)
	{
		case B_CONTROL_VIDEO_INPUT:
			mVideoInput=(EVideoInputName)aValue;
		break;
		case B_CONTROL_VIDEO_STANDARD_IN:
			mVideoStandardIn=aValue;
			RequestNewWeb();
		break;
		case B_CONTROL_VIDEO_BIT_RATE:
			mBitRate=floatValue;
			PROGRESS("dc10producer::SetParameterValue ID: %d  Value: %f\n",id,floatValue);
		break;
		case B_BRIGHTNESS:
			mBrightness = floatValue;
			
			if(fCodec != NULL)
			{
				if( fDevice->decoder_ad  !=0)
				{
					fCodec->brightness(mBrightness);
				}
			}
			
			PROGRESS("dc10producer::SetParameterValue ID: %d  Value: %f\n",id,floatValue);
		break;
		case B_CONTRAST:
			mContrast = floatValue;
			if(fCodec != NULL)
			{
				if( fDevice->decoder_ad  !=0)
				{
					fCodec->contrast(mContrast);
				}
			}
			PROGRESS("dc10producer::SetParameterValue ID: %d  Value: %f\n",id,floatValue);
		break;
		case B_SATURATION:
			mSaturation = floatValue;
			if(fCodec != NULL)
			{
				if( fDevice->decoder_ad  !=0)
				{
					fCodec->saturation(mSaturation);
				}
			}
			PROGRESS("dc10producer::SetParameterValue ID: %d  Value: %f\n",id,floatValue);
		break;
		case B_HUE:
			mHue = floatValue;
			if(fCodec != NULL)
			{
				if( fDevice->decoder_ad  !=0)
				{
					fCodec->hue(mHue);
				}
			}
			PROGRESS("dc10producer::SetParameterValue ID: %d  Value: %f\n",id,floatValue);
		break;
		default:
			ERROR("dc10producer::SetParameterValue - BAD ID\n");
		break;
		
	}
}

status_t
dc10producer::StartControlPanel(
    BMessenger * outMessenger)
{
    return BControllable::StartControlPanel(outMessenger);
}




//BMediNode

BMediaAddOn *
dc10producer::AddOn(long *cookie) const
{
	FUNCTION("dc10producer::AddOn\n");
	*cookie = mInternalID;
	return mAddOn;
}

void 
dc10producer::Start(bigtime_t performanceTime)
{
	FUNCTION("dc10producer::Start @ %Ld\n", performanceTime);
	BMediaEventLooper::Start(performanceTime);
}

void 
dc10producer::Stop(bigtime_t performanceTime, bool immediate)
{
//	Calling TimeSource() from the destructor is bad
	FUNCTION("dc10producer::Stop @ %Ld\n", immediate ? TimeSource()->Now() : performanceTime);
	BMediaEventLooper::Stop(performanceTime, immediate);
}

void 
dc10producer::Preroll()
{
	FUNCTION("dc10producer::Preroll\n");
	/* DO NOTHING */
}


//void 
//Bdc10Controllable::SetTimeSource(BTimeSource *time_source)
//{
//}

status_t 
dc10producer::HandleMessage(int32 message, const void * /*data*/, size_t /*size*/)
{
	//FUNCTION("dc10producer::HandleMessage - BEGIN\n");
	status_t status = B_OK;
	uint32 i;
	//uint32 num[4];
	int32 nb;
	void * pbuf;
	unsigned char * pbuff;
	uint32 * pbufff; 
	BBuffer *BufferToGo;
	
	switch (message)
	{	
		case DC10_FRAME_READY: 	
			{
				nb = -1;			
				if(mSize == 1)
				{
					if((fDevice->buffer_info[0] & 1 ) == 1)
					{
						nb = 0;
					}
					if((fDevice->buffer_info[2] & 1 ) == 1)
					{
						nb = 2;
					}
				}
				
				if(mSize == 2)
				{
					if(((fDevice->buffer_info[0] & 1 ) == 1)&&((fDevice->buffer_info[1] & 1 ) == 1))
					{
						nb = 0;
					}
					if(((fDevice->buffer_info[2] & 1 ) == 1)&&((fDevice->buffer_info[3] & 1 ) == 1))
					{
						nb = 2;
					}
				}
				
				if (nb == -1)
					return B_ERROR;
				
				
				//FUNCTION("dc10producer::HandleMessage - error frame number %d %d\n",mFrameNumber,mDevice.frame_number/2);
				bigtime_t temp=1000;
				size_t size = 0;
				size = (fDevice->buffer_info[nb] & ((1<<23)-1))/2;
				
				if(mSize == 2)
				{
					size += ((fDevice->buffer_info[nb+1] & ((1<<23)-1))/2);
					size += 0x52;
				}
				
				
				if ( ( ((fDevice->buffer_info[nb] & ((1<<23)-1))/2) > (256*1024) ) 
				||   ( ((fDevice->buffer_info[nb+1] & ((1<<23)-1))/2) > (256*1024) ) )
				{
					ERROR("dc10producer::HandleMessage BUFFER overflow\n");
				}
				BufferToGo=fBufferGroup->RequestBuffer(size,temp);
				
				if(BufferToGo != NULL)
				{	
					//FUNCTION("dc10producer::HandleMessage - copy buffer \n");								
					mFrameNumber = fDevice->frame_number / 2 ;
					BufferToGo->Header()->type = B_MEDIA_ENCODED_VIDEO;
					BufferToGo->Header()->size_used = 	size;
					BufferToGo->Header()->u.encoded_video.field_flags = B_MEDIA_KEY_FRAME;
					BufferToGo->Header()->u.encoded_video.forward_history = 0;
					BufferToGo->Header()->u.encoded_video.backward_history = 0;
					BufferToGo->Header()->u.encoded_video.field_sequence = fDevice->frame_number/2;
					BufferToGo->Header()->u.encoded_video.field_number = 0;
					BufferToGo->Header()->u.encoded_video.pulldown_number = 0;
					BufferToGo->Header()->u.encoded_video.first_active_line = 0;
					BufferToGo->Header()->u.encoded_video.line_count = mOutput.format.u.encoded_video.output.display.line_count;
					//FUNCTION("Time\n");
					//buf->Header()->start_time = TimeSource()->PerformanceTimeFor(mDevice.time_stamp);
					BufferToGo->Header()->start_time = TimeSource()->PerformanceTimeFor(fDevice->time_stamp);
			
					pbuf = BufferToGo->Data();

					size_t nnb=nb;
					
					if(mSize == 1)					
					{
						read(fDevice->fd,pbuf,nnb);
					}
					
					if(mSize == 2)					
					{
						pbuff = (unsigned char *) pbuf;
						pbuff[0] = 'B';
						pbuff[1] = 'M';
						
						pbufff = (uint32 *) pbuf;
						//FUNCTION("dc10 offset  %d\n",((fDevice->buffer_info[nnb] & ((1<<23)-1)) / 2) + 0x52);
						pbufff[3] = B_HOST_TO_BENDIAN_INT32 (((fDevice->buffer_info[nnb] & ((1<<23)-1)) / 2) + 0x52 );
					
						pbuff += 0x52;
						read(fDevice->fd,(void*)pbuff,nnb);
						
						pbuff = (unsigned char *) pbuf;
						pbuff += 0x52;
						pbuff += ((fDevice->buffer_info[nnb] & ((1<<23)-1)) / 2);
						nnb += 1;
						read(fDevice->fd,(void*)pbuff,nnb);
					}		
					// Send the buffer		
					//FUNCTION("Sending buffer %08x\n", buf);
					status_t status;
					if ((status=SendBuffer(BufferToGo, mOutput.destination)) != B_OK)
					{
						// If SendBuffer fails,we need to recycle it
						ERROR("DC10producer::CaptureRun - SENDBUFFER FAILED 0x%x\n", status);
						BufferToGo->Recycle();
					}
					else
					{
						//FUNCTION("send buffer ok %d\n",fDevice->frame_number/2);
					}
			
					mMediaTime = (bigtime_t)((mFrameNumber * 1000000LL)/mRate);
					PublishTime(mMediaTime, fDevice->time_stamp, 1.0);
				}
				else
				{
					//FUNCTION("send buffer kko \n");
					
				}
			
			
						
				
				for(i=0;i<4;i++)
				{
					if((fDevice->buffer_info[i]&1)==1)
					{
						fDevice->buffer_reset_index = i;
						if(ioctl(fDevice->fd, DC10_SET_BUFFER, fDevice, sizeof(dc10_config)) < 0)
						{
							//error in ioctl
							ERROR("dc10producer::HandleMessage ioctl error \n");
						}
						
					}
				}			
				
			}
			break;
		case DC10_SUSPEND_THREAD: 	
			{
				//FUNCTION("dc10producer::HandleMessage - DC1_SUSPEND_THREAD \n");
				suspend_thread(mFrameReadyThread);	
			}
			break;
		
		default:
			FUNCTION("dc10producer::HandleMessage - No Message\n");
			status = B_ERROR;
			break;
	}

	//FUNCTION("dc10producer::HandleMessage - END\n");
	return status;
}

void 
dc10producer::NodeRegistered()
{
	FUNCTION("dc10producer::NodeRegistered %ld 0x%x - BEGIN\n", mInternalID, (void *)this);
			
	if (mInitCheck == B_OK)
	{
		FUNCTION("dc10producer::NodeRegistered OK\n");
		// set latency defaults
		SetEventLatency((bigtime_t)1000000LL/mRate);
			
			
		// initialize output 
    	mOutput.destination = media_destination::null;
    	mOutput.format.type = B_MEDIA_ENCODED_VIDEO;
    	mOutput.format.u.encoded_video = pjpeg_ntsc_half_format;
	    mOutput.node = Node();
		mOutput.source.port = ControlPort();
		printf("port %ld\n",mOutput.source.port);
		mOutput.source.id = 0;
		
		//Initialize();		
		ConstructControlWeb();

		// tell the event looper to start up the thread!
		Run();
	}
	else ERROR("dc10producer::NodeRegistered - Couldn't open driver!!\n");
}

void 
dc10producer::SetRunMode(run_mode mode)
{
	FUNCTION("dc10producer::SetRunMode -BEGIN\n");
	BMediaEventLooper::SetRunMode(mode);
	BMediaNode::SetRunMode(mode);
}



status_t 
dc10producer::TimeSourceOp(const time_source_op_info &op, void */*_reserved*/)
{
	FUNCTION("dc10producer::TimeSourceOp: 0x%x %Ld %Ld @ %Ld\n",
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
dc10producer::HandleEvent(const media_timed_event *event, bigtime_t /*lateness*/, bool realTimeEvent)
{
	LOOP("dc10producer::HandleEvent: lateness: %Ld\n", lateness);

	//bigtime_t realTime = BTimeSource::RealTime();

	switch(event->type)
	{
		case BTimedEventQueue::B_START:
		{
			if(fd<0)
			{
				FUNCTION("dc10producer::HandleEvent fd < 0\n");
				return;
			}
			FUNCTION("dc10producer::HandleEvent B_START\n");
			
			bigtime_t performanceTime = event->event_time;
			if (realTimeEvent)
				performanceTime = TimeSource()->PerformanceTimeFor(event->event_time);
			if (RunState() == BMediaEventLooper::B_STARTED)
			{
				FUNCTION("dc10producer::HandleEvent B_STARTED\n");
				break;
			}
			
			mFrameNumber = 0;
			mMediaTime = 0;
			
			if(ioctl(fDevice->fd, DC10_RESET_FRAME_NUMBER, fDevice, sizeof(dc10_config)) < 0)
			{
				//error in ioctl
				return ;
			}			

			if (mOutput.destination != media_destination::null)
				SendDataStatus(B_DATA_AVAILABLE, mOutput.destination, performanceTime);

			PublishTime(mMediaTime, TimeSource()->RealTimeFor(performanceTime, 0), 1.0);
			
			
			
			mFrameReadyThreadInfo.running = true;
			resume_thread(mFrameReadyThread);
			
			fDevice->mode=1;
			if(ioctl(fDevice->fd, DC10_SET_MODE, fDevice, sizeof(dc10_config)) < 0)
			{
				//error in ioctl
				return ;
			}
			
			fCodec->enable();
			
			FUNCTION("dc10producer::HandleEvent thread lance\n");
			break;
		}

		case BTimedEventQueue::B_STOP:
		{
			FUNCTION("dc10producer::HandleEvent B_STOP\n");
			if(fd<0)
			{
				FUNCTION("dc10producer::HandleEvent fd < 0\n");
				return;
			}
			
			fCodec->disable();
			
			fDevice->mode=0;
			if(ioctl(fDevice->fd, DC10_SET_MODE, fDevice, sizeof(dc10_config)) < 0)
			{	
				//error in ioctl
				return ;
			}
			// actually stop the capture
			
			bigtime_t performanceTime = event->event_time;
			if (realTimeEvent)
				performanceTime = TimeSource()->PerformanceTimeFor(event->event_time);
			if (RunState() == BMediaEventLooper::B_STOPPED)
			{
				FUNCTION("dc10producer::HandleEvent B_STOPPED\n");
				break;
			}
			
	
			if (mOutput.destination != media_destination::null)
				SendDataStatus(B_DATA_NOT_AVAILABLE, mOutput.destination, performanceTime);
		
		
			PublishTime(mMediaTime, TimeSource()->RealTimeFor(performanceTime, 0), 0);
			
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
dc10producer::FormatSuggestionRequested(media_type type, int32 quality, media_format *format)
{
	FUNCTION("dc10producer::FormatSuggestionRequested\n");

	if (type == B_MEDIA_NO_TYPE)
		type = B_MEDIA_ENCODED_VIDEO;

	quality = quality;
	
	
	if (format == NULL)
        return B_BAD_VALUE;

    if (type != B_MEDIA_ENCODED_VIDEO)
        return B_MEDIA_BAD_FORMAT;

	fCodec->decoder_init(mVideoInput);
	if (fDevice->video_format == FORMAT_NTSC)
    {
    	format->u.encoded_video = pjpeg_ntsc_half_format;
    }
    if ((fDevice->video_format == FORMAT_PAL)||
		(fDevice->video_format == FORMAT_SECAM))
	{
		format->u.encoded_video = pjpeg_pal_half_format;
    }
    
	return B_OK;
}

status_t 
dc10producer::FormatProposal(const media_source &/*output*/, media_format *format)
{
	FUNCTION("dc10producer::FormatProposal - BEGIN\n");
	
	uint32 maxX, maxY,etat0, etat1, etat2,etat3;
	
	maxX = 0;
	
	
	if(mVideoStandardIn == 0)
	{
		maxX = 640;
		maxY = 480;
		mRate = 29.97;
		mXSize = 320;
		mYSize = 240;
	}
	if(mVideoStandardIn == 1)

	{
		maxX = 768;
		maxY = 576;
		mRate = 25.00;
		mXSize = 384;
		mYSize = 288;
		
	}
	
	if (maxX  == 0)
		return B_MEDIA_BAD_FORMAT;
			
	if (format->type == B_MEDIA_NO_TYPE)
	{
		format->type = B_MEDIA_ENCODED_VIDEO;	
		if (fDevice->video_format == FORMAT_NTSC)
		{
			format->u.encoded_video = pjpeg_ntsc_half_format;
		}
		if ((fDevice->video_format == FORMAT_PAL)||
			(fDevice->video_format == FORMAT_SECAM))
		{
			format->u.encoded_video = pjpeg_pal_half_format;
		}
		return B_OK;
		
	}
	
	if (format->type == B_MEDIA_ENCODED_VIDEO)
	{
		if (format->u.encoded_video.forward_history != 0  )
		{
			ERROR("dc10producer::FormatProposal - format->u.encoded_video.forward_history ERROR\n");
			return B_MEDIA_BAD_FORMAT;
		}
		
		if (format->u.encoded_video.backward_history != 0  )
		{
			ERROR("dc10producer::FormatProposal - format->u.encoded_video.backward_history ERROR\n");
			return B_MEDIA_BAD_FORMAT;
		}
		
		if (format->u.encoded_video.output.display.format == media_raw_video_format::wildcard.display.format)
			format->u.encoded_video.output.display.format = B_NO_COLOR_SPACE;
		else
			if (format->u.encoded_video.output.display.format != B_NO_COLOR_SPACE)		
			{
				format->u.encoded_video.output.display.format = B_NO_COLOR_SPACE;
				ERROR("dc10producer::FormatProposal - BAD DISPLAY FORMAT\n");
				return B_MEDIA_BAD_FORMAT;
			}
		
		// We can only support one orientation: B_VIDEO_TOP_LEFT_RIGHT.
		if (format->u.encoded_video.output.orientation == media_raw_video_format::wildcard.orientation)
			format->u.encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		else 
			if (format->u.encoded_video.output.orientation != B_VIDEO_TOP_LEFT_RIGHT)
			{
				format->u.encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT;
				ERROR("dc10producer::FormatProposal - BAD ORIENTATION\n");
				return B_MEDIA_BAD_FORMAT;
			}
			
		// First active should be 0
		if (format->u.encoded_video.output.first_active == media_raw_video_format::wildcard.first_active)
			format->u.encoded_video.output.first_active = 0;
		else 
			if (format->u.encoded_video.output.first_active != 0)
			{
				ERROR("dc10producer::FormatProposal - BAD FIRST ACTIVE\n");
				return B_MEDIA_BAD_FORMAT;
			}	
			
			
		
		// Field rate
		if (format->u.encoded_video.output.field_rate == media_raw_video_format::wildcard.field_rate)
			format->u.encoded_video.output.field_rate = mRate;
		else 
			if (format->u.encoded_video.output.field_rate != mRate)
			{
				ERROR("dc10producer::FormatProposal - BAD FIELD RATE\n");
				return B_MEDIA_BAD_FORMAT;
			}	
			
			
		// Interlaced
		if (format->u.encoded_video.output.interlace == media_raw_video_format::wildcard.interlace)
			format->u.encoded_video.output.interlace = 1;
		else 
			if (format->u.encoded_video.output.interlace != 1)
			{
				ERROR("dc10producer::FormatProposal - BAD interlace\n");
				return B_MEDIA_BAD_FORMAT;
			}	
			
			
		
			
		etat0 = 0;
		etat1 = 0;
		etat2 = 0;	
		
		etat3 = 0;	
		if (format->u.encoded_video.encoding == media_encoded_video_format::B_ANY )
		{
			etat0 ++;
			etat3 ++;
		}
		if (format->u.encoded_video.encoding == pjpeg_ntsc_half_format.encoding )
		{
			etat1 ++;
			etat3 ++;
		}
		if (format->u.encoded_video.encoding == pjpeg_ntsc_format.encoding )
		{
			etat2 ++;
			etat3 ++;
		}
		if (etat3 == 0 )
		{
			ERROR("BAD FORMAT\n");
			return B_MEDIA_BAD_FORMAT;
		}		
		
		etat3 = 0;	
		if (format->u.encoded_video.output.display.line_count == media_raw_video_format::wildcard.display.line_count)
		{
			etat0 ++;
			etat3 ++;
		}
		if (format->u.encoded_video.output.display.line_count == mYSize)
		{
			etat1 ++;
			etat3 ++;
		}
		if (format->u.encoded_video.output.display.line_count == (2 * mYSize))
		{
			etat2 ++;
			etat3 ++;
		}
		if (etat3 == 0 )
		{
			ERROR("BAD FORMAT\n");
			return B_MEDIA_BAD_FORMAT;
		}
		
		etat3 = 0;	
		if (format->u.encoded_video.output.display.line_width == media_raw_video_format::wildcard.display.line_width)
		{
			etat0 ++;
			etat3 ++;
		}
		if (format->u.encoded_video.output.display.line_width == mXSize)
		{
			etat1 ++;
			etat3 ++;
		}
		if (format->u.encoded_video.output.display.line_width == (2 * mXSize))
		{
			etat2 ++;
			etat3 ++;
		}
		if (etat3 == 0 )
		{
			ERROR("BAD FORMAT\n");
			return B_MEDIA_BAD_FORMAT;
		}
		
		
		
		
		
		
		if ( (etat1 != 0) && (etat2 != 0) )
		{
			ERROR("BAD FORMAT\n");
			return B_MEDIA_BAD_FORMAT;
		}
		
		if(etat1 != 0)
		{
			if (format->u.encoded_video.encoding == media_encoded_video_format::B_ANY )
			{
				 format->u.encoded_video.encoding = pjpeg_ntsc_half_format.encoding ;
			}
			if (format->u.encoded_video.output.display.line_count == media_raw_video_format::wildcard.display.line_count)
			{
				format->u.encoded_video.output.display.line_count = mYSize;
			}
			if (format->u.encoded_video.output.display.line_width == media_raw_video_format::wildcard.display.line_width)
			{
				format->u.encoded_video.output.display.line_width = mXSize;
			}
		}
		
		if(etat2 != 0)
		{
			if (format->u.encoded_video.encoding == media_encoded_video_format::B_ANY )
			{
				 format->u.encoded_video.encoding = pjpeg_ntsc_format.encoding ;
			}
			if (format->u.encoded_video.output.display.line_count == media_raw_video_format::wildcard.display.line_count)
			{
				format->u.encoded_video.output.display.line_count = 2*mYSize;
			}
			if (format->u.encoded_video.output.display.line_width == media_raw_video_format::wildcard.display.line_width)
			{
				format->u.encoded_video.output.display.line_width = 2*mXSize;
			}
			FUNCTION("M_JPEG \n");
		}
		
		if((etat1 == 0) && (etat2 == 0))
 		{
			format->u.encoded_video.encoding = pjpeg_ntsc_half_format.encoding ;
			format->u.encoded_video.output.display.line_count = mYSize;
			format->u.encoded_video.output.display.line_width = mXSize;
		}
	}
    
	FUNCTION("dc10producer::FormatProposal - END OK\n");
	return B_OK;
	
	
	
}

status_t 
dc10producer::FormatChangeRequested(const media_source &/*source*/, 
									const media_destination &/*dest*/, 
									media_format */*io_format*/, 
									int32 */*out_change_count*/)
{
	FUNCTION("dc10producer::FormatChangeRequested\n");
	return B_ERROR;
}

status_t 
dc10producer::GetNextOutput(int32 *cookie,
							media_output *out_destination)
{
	FUNCTION("dc10producer::GetNextOutput\n");

	if (mInitCheck != B_OK)
	{
		ERROR("dc10producer::GetNextOutput - DEVICE NOT INITIALIZED\n");
		return B_ERROR;
	}

	switch (*cookie)
	{
		case 0:
			PROGRESS("dc10producer::GetNextOutput #0\n");
			*out_destination = mOutput;
			(*cookie)++;
			return B_OK;
		default:
			ERROR("dc10producer::GetNextOutput - BAD INDEX\n");
			return B_BAD_INDEX;
	}
}

status_t 
dc10producer::DisposeOutputCookie(int32 /*cookie*/)
{
	FUNCTION("dc10producer::DisposeOutputCookie - BEGIN\n");
	return B_OK;
}

status_t 
dc10producer::SetBufferGroup(const media_source &for_source, BBufferGroup *group)
{
	FUNCTION("dc10producer::SetBufferGroup port = %08x id = %d\n", for_source.port, for_source.id);
	
	
    if (for_source != mOutput.source)
        return B_MEDIA_BAD_SOURCE;

    // release the previous buffer group 
    if (fBufferGroup1 != NULL) {
        DeleteBufferGroup();
    }

    fBufferGroup = group;

    // allocate new buffer group 
    if (fBufferGroup == NULL) 
    {
    	FUNCTION("dc10producer::SetBufferGroup create  buffer\n");
        CreateBufferGroup();
        fBufferGroup = fBufferGroup1;  
        status_t err=fBufferGroup->InitCheck();
		if(err<0)
		{
			FUNCTION("dc10producer::SetBufferGroup BufferGroup->InitCheck error\n");
			DeleteBufferGroup();
			return err;
		}  
		
    }
    else
    {
    	FUNCTION("dc10producer::SetBufferGroup  buffergroup given by consumer \n");
    	status_t err=fBufferGroup->InitCheck();
		if(err<0)
		{
			FUNCTION("dc10producer::SetBufferGroup BufferGroup->InitCheck error\n");
			return err;
		}
		int32 nbbuf;
		fBufferGroup->CountBuffers(&nbbuf);
		FUNCTION("dc10producer::SetBufferGroup BufferGroup->Count %d\n",nbbuf);
	}	
    FUNCTION("dc10producer::SetBufferGroup - END\n");
    return B_OK;
    
    
    
    
}


status_t 
dc10producer::VideoClippingChanged(const media_source &/*for_source*/, int16 /*num_shorts*/, int16 */*clip_data*/, const media_video_display_info &/*display*/, int32 */*out_from_change_count*/)
{
	FUNCTION("dc10producer::VideoClippingChanged\n");
	return B_NO_ERROR;
}

status_t 
dc10producer::GetLatency(bigtime_t *out_latency)
{
	BBufferProducer::GetLatency(out_latency);
	(*out_latency) += (bigtime_t)(1000000LL/mRate) + SchedulingLatency();
	return B_OK;
}

status_t 
dc10producer::PrepareToConnect(const media_source &what, 
							   const media_destination &/*where*/,
							   media_format *format,
							   media_source *out_source,
							   char *io_name)
{
	FUNCTION("dc10producer::PrepareToConnect\n");
	fd=open(mName,O_RDWR);
	if(fd<0)
	{
		ERROR("dc10producer::dc10producer Erreur :: main ouverture device  %s\n",mName);
		return B_ERROR;
	}		
	else
	{
		PROGRESS("dc10producer::dc10producer OK :: %d \n",fd);
	}

	fCodec = new dc10codec();
	fCodec->setup(fd,MODE_COMPRESS);
	fDevice = &fCodec->mDevice; 

	snooze(1000);

	//find format of video in			
	fCodec->decoder_init(mVideoInput);
	
	if((fDevice->video_format != FORMAT_NTSC)
		&&(fDevice->video_format != FORMAT_PAL)
		&&(fDevice->video_format != FORMAT_SECAM))
	{
			ERROR("dc10producer::PrepareToConnect - NO video in\n");
			return B_ERROR;
	}
	
	if ((fDevice->video_format == FORMAT_NTSC)
		&& (mVideoStandardIn == 1))
	{
			ERROR("dc10producer::PrepareToConnect - NTSC IN\n");
			return B_ERROR;
	}
	if (((fDevice->video_format == FORMAT_PAL)||(fDevice->video_format == FORMAT_SECAM))
		&& (mVideoStandardIn == 0))
	{
			ERROR("dc10producer::PrepareToConnect - PAL/SECAM IN\n");
			return B_ERROR;
	}
	
			
	fCodec->brightness(mBrightness);
	fCodec->saturation(mSaturation);
	fCodec->contrast(mContrast);
	if(fDevice->video_format == FORMAT_NTSC)
	{
		fCodec->hue(mHue);		
	}
	
	strcpy(io_name, "dc10 Input 1");
	
	if (mInitCheck != B_OK)
	{
		ERROR("dc10producer::PrepareToConnect - DEVICE NOT OPENED\n");
		return B_ERROR;
	}
	
    if (mOutput.source != what)
    {
    	ERROR("dc10producer::PrepareToConnect - B_MEDIA_BAD_SOURCE\n");
        return B_MEDIA_BAD_SOURCE;
	}
	
    if (mOutput.destination != media_destination::null)
    {
    	ERROR("dc10producer::PrepareToConnect - B_MEDIA_ALREADY_CONNECTED\n");
        return B_MEDIA_ALREADY_CONNECTED;
	}
	
    if (out_source == NULL || io_name == NULL)
    {
    	ERROR("dc10producer::PrepareToConnect - BAD_VALUE \n");
    	return B_BAD_VALUE;
	}
	
	/*
	if (!format_is_compatible(*format, mOutput.format)) {
		*format = mOutput.format;
		return B_MEDIA_BAD_FORMAT;
	}
	*/

    
    *out_source = mOutput.source;
    mOutput.format = *format;
   	strncpy(io_name, mOutput.name, B_MEDIA_NAME_LENGTH);
   
	return B_OK;
}

void 
dc10producer::Connect(status_t error,
					  const media_source &/*source*/,
					  const media_destination &destination, 
					  const media_format &format, 
					  char *io_name)
{
	FUNCTION("dc10producer::Connect - BEGIN Source ID#%d Destination ID#%d\n", source.id, destination.id);
 

    

    if (error != B_OK) {
        // unreserve the connection 
        mOutput.format.type = B_MEDIA_ENCODED_VIDEO ;
        mOutput.format.u.encoded_video =  pjpeg_ntsc_half_format;
        mOutput.destination = media_destination::null;
        
        FUNCTION("dc10producer::Connect error != B_OK\n");
        return;
    }

    // reserve the connection
    FUNCTION("dc10producer::Connect reserve the connection\n"); 
    mOutput.destination = destination;
    mOutput.format = format;
    strncpy(io_name, mOutput.name, B_MEDIA_NAME_LENGTH);
    
   	mSize=0;
    if( mOutput.format.u.encoded_video.encoding == pjpeg_ntsc_format.encoding )
    {
    	mSize = 2;
    	FUNCTION("dc10producer::Connect Full size\n");
    }
    if( mOutput.format.u.encoded_video.encoding == pjpeg_ntsc_half_format.encoding )
    {
    	mSize = 1;
    	FUNCTION("dc10producer::Connect Half size\n");
    }
    
   	

    // create the buffer group 
    FUNCTION("dc10producer::Connect Buffer group\n"); 
    if (fBufferGroup == NULL) {
        CreateBufferGroup();
        fBufferGroup = fBufferGroup1;
    }

	FUNCTION("dc10producer::Connect duration, time ...\n"); 
	/* set the duration of the node's buffers */
    int32 numBuffers;
    fBufferGroup->CountBuffers(&numBuffers);
    SetBufferDuration((1000000LL * numBuffers) / mRate);
    
    
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

   FUNCTION("dc10producer::Connect configure video out\n"); 
   
    	
    //set zoran36067 config
	FUNCTION("dc10producer::Connect set_video \n"); 
    
    fCodec->init(fDevice->video_format,
    			 mOutput.format.u.encoded_video.output.display.line_width, 
    			 mOutput.format.u.encoded_video.output.display.line_count,
    			 MODE_COMPRESS,
    			 (uint32)(mBitRate*100.0));
    			 
	FUNCTION("dc10producer::set_up_buffers \n"); 						
   	
 
   
   	FUNCTION("dc10producer::connect - thread\n");
	mFrameReadyThreadInfo.port = ControlPort();
	mFrameReadyThreadInfo.dc10 = fDevice;
	mFrameReadyThreadInfo.quit = false;
	mFrameReadyThreadInfo.running = false;
	
	mFrameReadyThread = spawn_thread((thread_func)frame_ready_thread,
	                                 "frame ready thread",
	                                 B_URGENT_DISPLAY_PRIORITY, &mFrameReadyThreadInfo);

	if(mFrameReadyThread < 0) {
		mInitCheck = mFrameReadyThread;
		FUNCTION("dc10producer::connect - thread ko\n");
		return;
	}
	else
	{
		FUNCTION("dc10producer::connect - thread ok\n");
	}
	
	resume_thread(mFrameReadyThread);
	

    // enable output 
    fDevice->mEnabled = true;

	
	Start(TimeSource()->Now());

	FUNCTION("dc10producer::Connect - END\n");
}

void 
dc10producer::Disconnect(const media_source &/*source*/, const media_destination &destination)
{
	status_t status;
	FUNCTION("dc10producer::Disconnect - BEGIN Source ID#%d Destination ID#%d\n", source.id, destination.id);
	
	Stop(TimeSource()->Now(),true);
	
	if (mOutput.destination != destination) 
	{
		ERROR("dc10producer::Disconnect - NOT OUR CONNECTION\n");	
		return;
	}

	//StopCapture();

	if ((mOutput.destination == destination) && (fd >=0)) 
	{
		fDevice->mode=0;
		if(ioctl(fDevice->fd, DC10_SET_MODE, fDevice, sizeof(dc10_config)) < 0)
		{
			//error in ioctl
			return ;
		}
		// actually stop the capture
		
		
		
		
		
		FUNCTION("dc10producer::disconnect - thread ok\n");
		mFrameReadyThreadInfo.quit = true;
		resume_thread(mFrameReadyThread);
    	wait_for_thread(mFrameReadyThread, &status);
		FUNCTION("dc10producer::disconnect - thread done\n");
		
		    
    	
    
		PROGRESS("dc10producer::Disconnect - Disconnecting #1\n");
		mOutput.destination = media_destination::null;
		mOutput.format.type = B_MEDIA_ENCODED_VIDEO;
		mOutput.format.u.encoded_video = pjpeg_ntsc_half_format;
		/* release buffer group */
    	fBufferGroup = NULL;
    	if (fBufferGroup1 != NULL) {
        	DeleteBufferGroup();
    	}
		
		
	}	
	snooze(1000);
	
	
		
	if(fd >= 0)
	{
		fCodec->decoder_commande(fDevice->video_format);
		fCodec->decoder_commande(INPUT_ZR36060);
		fCodec->encoder_commande(fDevice->video_format);
		fCodec->encoder_commande(INPUT_ZR36060);
		fCodec->encoder_commande(COLOR_MIR);
		
		close(fd);
		fd = -1;
	}
	if(fCodec != NULL)
	{
		delete fCodec ;
		fCodec = NULL;
	}
		
	fDevice = NULL;
	

	FUNCTION("dc10producer::Disconnect - END\n");
	return;
}

void 
dc10producer::LateNoticeReceived(const media_source &/*what*/, bigtime_t /*how_much*/, bigtime_t /*performance_time*/)
{
	FUNCTION("dc10producer::LateNoticeReceived\n");
}

void 
dc10producer::EnableOutput(const media_source &what, bool enabled, int32 */*change_tag*/)
{
	FUNCTION("dc10producer::EnableOutput\n");
	  if (mOutput.source == what)
       fDevice->mEnabled  = enabled;
}

status_t 
dc10producer::SetPlayRate(int32 /*numer*/, int32 /*denom*/)
{
	return B_ERROR;
}

void 
dc10producer::AdditionalBufferRequested(const media_source &/*source*/, media_buffer_id /*prev_buffer*/, bigtime_t /*prev_time*/, const media_seek_tag */*prev_tag*/)
{
	//it's good to feel wanted
}


void dc10producer::CreateBufferGroup(void)
{
	FUNCTION("dc10producer::CreateBufferGroup create new buffers\n");
    if (fBufferGroup1 != NULL) 
    {
        DeleteBufferGroup();
    }

    // 3 buffers allocated
    fBufferGroup1 = new BBufferGroup(2*256*1024, 3, B_ANY_ADDRESS, B_FULL_LOCK);

}

void dc10producer::DeleteBufferGroup(void)
{
    if (fBufferGroup1 != NULL) {
        delete fBufferGroup1;
        fBufferGroup1 = NULL;
    }
}

