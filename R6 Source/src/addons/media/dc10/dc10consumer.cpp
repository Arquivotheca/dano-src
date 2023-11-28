#include "dc10consumer.h"
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








dc10consumer::dc10consumer(const uint32 internal_id,
						   const char * devicename,
						   const char *nodename,
						   BMediaAddOn *addon=0) :
		BMediaNode(nodename),
		BMediaEventLooper(),
		BBufferConsumer(B_MEDIA_ENCODED_VIDEO),
		BControllable(),
		mInternalID(internal_id),
		mAddOn(addon)
{
	FUNCTION("dc10consumer::dc10consumer- BEGIN\n");
	
	SetPriority(B_URGENT_DISPLAY_PRIORITY);
	

	// Let the system know we do physical input
	AddNodeKind(B_PHYSICAL_OUTPUT);

	fd = -1;
	if (!devicename) {
		mName = strdup("/dev/video/dc10/0");
	}
	else 
	{
		mName = strdup(devicename);
	}
	mInitCheck = B_OK;
	mRate = 29.97;
	fCodec = NULL;
	fDevice = NULL; 	
	
	firstbuffer = 0;
	mState = 0;
	FUNCTION("dc10consumer::dc10consumer- END\n");
	return;
}

dc10consumer::~dc10consumer()
{
	FUNCTION("dc10consumer::~dc10consumer - BEGIN\n");
	
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
	FUNCTION("dc10consumer::~dc10consumer - END\n");
}




//
// BMediaNode hooks
//

BMediaAddOn *dc10consumer::AddOn(long *cookie) const
{
	FUNCTION("dc10consumer::AddOn\n");
	*cookie = mInternalID;
	return mAddOn;
}

void dc10consumer::NodeRegistered()
{
	FUNCTION("dc10consumer::NodeRegistered %ld 0x%x - BEGIN\n", mInternalID, (void *)this);
			
	if (mInitCheck == B_OK)
	{
		FUNCTION("dc10consumer::NodeRegistered OK\n");
		// set latency defaults
		SetEventLatency((bigtime_t)1000000LL/mRate);
			
			
		// initialize output 
    	mInput.source = media_source::null;
	    mInput.node = Node();
		mInput.destination.port = ControlPort();
		mInput.destination.id = 0;
		
		mInput.format.type = B_MEDIA_ENCODED_VIDEO;
    	mInput.format.u.encoded_video = pjpeg_ntsc_half_format;
    	
    	
		//Initialize();		
		ConstructControlWeb();

		// tell the event looper to start up the thread!
		Run();
	}
	else ERROR("dc10consumer::NodeRegistered - Couldn't open driver!!\n");
	
	FUNCTION("dc10consumer::NodeRegistered END\n");
}


void dc10consumer::Preroll()
{	
	FUNCTION("dc10consumer::Preroll - BEGIN\n");
	return ;
}

status_t dc10consumer::HandleMessage(int32 message,
    								 const void * data,
    								 size_t size)
{
	FUNCTION("dc10consumer::HandleMessage\n");
	if	(	BMediaNode::HandleMessage(message, data, size) &&
			BMediaEventLooper::HandleMessage(message, data, size) &&
			BBufferConsumer::HandleMessage(message, data, size)) {
		HandleBadMessage(message, data, size);
	}

	return B_OK;
}


status_t dc10consumer::RequestCompleted(const media_request_info & /*info*/)
{
    FUNCTION("dc10consumer::RequestCompleted\n");
    return B_OK;
}

//
// BMediaEventLooper hooks
//

void dc10consumer::Start(bigtime_t performance_time)
{
	FUNCTION("dc10consumer::Start\n");
    BMediaEventLooper::Start(performance_time);
}

void dc10consumer::Stop(bigtime_t performance_time,
    					bool immediate)
{
	FUNCTION("dc10consumer::Stop\n");
    BMediaEventLooper::Stop(performance_time, immediate);
}

void dc10consumer::Seek(bigtime_t media_time,
    					bigtime_t performance_time)
{
	FUNCTION("dc10consumer::Seek\n");
    BMediaEventLooper::Seek(media_time, performance_time);
}

void dc10consumer::TimeWarp(bigtime_t at_real_time,
							bigtime_t to_performance_time)
{
	FUNCTION("dc10consumer::timeWarp\n");
    BMediaEventLooper::TimeWarp(at_real_time, to_performance_time);
}

status_t dc10consumer::DeleteHook(BMediaNode * node)
{
	FUNCTION("dc10consumer::DeleteHook\n");
    return BMediaEventLooper::DeleteHook(node);
}

void dc10consumer::HandleEvent(const media_timed_event *event,
							   bigtime_t /*lateness*/,
							   bool /*realTimeEvent*/)
{
	//FUNCTION("dc10consumer::HandleEvent - Begin\n");
	switch(event->type) {
		case BTimedEventQueue::B_START:
			//fCodec->enable();
			break;
	
		case BTimedEventQueue::B_STOP:
			fCodec->disable();
			break;
	
		case BTimedEventQueue::B_WARP:
			
			break;
	
		case BTimedEventQueue::B_TIMER:
			
			break;

		case BTimedEventQueue::B_DATA_STATUS:
		{
			
			break;
		}

		case BTimedEventQueue::B_HANDLE_BUFFER:
		{
            BBuffer * buffer = (BBuffer *) event->pointer;
            if (buffer != NULL) {
                if ((RunState() == BMediaEventLooper::B_STARTED) && (mState == 1))
                {
                	if(mEncoding == 2)
                	{
                		size_t size = 0;
                		if(firstbuffer == 0)
                		{
                			PRINTF("dc10consumerer::HandleEvent buffer first time\n");
                			fDevice->buffer = buffer->Data();
							size = buffer->SizeUsed();
							fDevice->buffer_index = 0;
							if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
							{
								//error in ioctl
								ERROR("dc10consumerer::HandleEvent ioctl error \n");
							}
							write(fDevice->fd,buffer->Data(),size);
						
							fDevice->buffer_index = 1;
							if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
							{
								//error in ioctl
								ERROR("dc10consumerer::HandleEvent ioctl error \n");
							}
							write(fDevice->fd,buffer->Data(),size);
						
							fDevice->buffer_index = 2;
							if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
							{
								//error in ioctl
								ERROR("dc10consumerer::HandleEvent ioctl error \n");
							}
							write(fDevice->fd,buffer->Data(),size);
						
							fDevice->buffer_index = 3;
							if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
							{
								//error in ioctl
								ERROR("dc10consumerer::HandleEvent ioctl error \n");
							}
							write(fDevice->fd,buffer->Data(),size);
						
                			firstbuffer = 1;
                			fCodec->enable();
                		}
                		else
                		{
                			//FUNCTION("dc10consumer::HandleEvent  handle buffers- \n");
               				if(ioctl(fDevice->fd, DC10_GET_BUFFER_INFO, fDevice, sizeof(dc10_config)) < 0)
							{
								//error in ioctl
								ERROR("dc10consumerer::HandleEvent ioctl error \n");
							}
							//FUNCTION("dc10consumer::HandleEvent  handle buffers- 0 %lu\n",fDevice->buffer_info[0]);
							//FUNCTION("dc10consumer::HandleEvent  handle buffers- 1 %lu\n",fDevice->buffer_info[1]);
							//FUNCTION("dc10consumer::HandleEvent  handle buffers- 2 %lu\n",fDevice->buffer_info[2]);
							//FUNCTION("dc10consumer::HandleEvent  handle buffers- 3 %lu\n",fDevice->buffer_info[3]);
                    		if(((fDevice->buffer_info[0] & 1 ) == 1)
                    			&&   ((fDevice->buffer_info[1] & 1 ) == 1))
							{
								//PRINTF("dc10consumerer::HandleEvent buffer 0-1\n");
								fDevice->buffer = buffer->Data();
								size = buffer->SizeUsed();
								fDevice->buffer_index = 0;
								if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
								{
									//error in ioctl
									ERROR("dc10consumerer::HandleEvent ioctl error \n");
								}
								write(fDevice->fd,buffer->Data(),size);
						
								fDevice->buffer_index = 1;
								if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
								{
									//error in ioctl
									ERROR("dc10consumerer::HandleEvent ioctl error \n");
								}
								write(fDevice->fd,buffer->Data(),size);
							}
							if(((fDevice->buffer_info[2] & 1 ) == 1)
                    			&&   ((fDevice->buffer_info[3] & 1 ) == 1))
							{
								//PRINTF("dc10consumerer::HandleEvent buffer 2-3\n");
								fDevice->buffer = buffer->Data();
								size = buffer->SizeUsed();
								fDevice->buffer_index = 2;
								if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
								{
									//error in ioctl
									ERROR("dc10consumerer::HandleEvent ioctl error \n");
								}
								write(fDevice->fd,buffer->Data(),size);
						
								fDevice->buffer_index = 3;
								if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
								{
									//error in ioctl
									ERROR("dc10consumerer::HandleEvent ioctl error \n");
								}
								write(fDevice->fd,buffer->Data(),size);
							}
						}
					}
					if(mEncoding == 1)
					{
						unsigned char * compressed_data = (unsigned char *) buffer->Data();
						size_t offset1,offset2,size1,size2;
						
						if (compressed_data[0] == 'B' && compressed_data[1] == 'M')
						{
							offset1 = 0x52;
						}
						else
						{
							offset1 = 0;
						}	
						offset2 = B_BENDIAN_TO_HOST_INT32(((uint32*)compressed_data)[3]);
						
						size1 =  offset2 -offset1 ;
						size2 = buffer->SizeUsed() - offset1;
						
						if(firstbuffer == 0)
                		{
                			PRINTF("dc10consumerer::HandleEvent buffer first time\n");
             
							fDevice->buffer_index = 0;
							if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
							{
								//error in ioctl
								ERROR("dc10consumerer::HandleEvent ioctl error \n");
							}
							write(fDevice->fd,compressed_data + offset1, size1);
						
							fDevice->buffer_index = 1;
							if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
							{
								//error in ioctl
								ERROR("dc10consumerer::HandleEvent ioctl error \n");
							}
							write(fDevice->fd,compressed_data + offset2, size2);
						
							fDevice->buffer_index = 2;
							if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
							{
								//error in ioctl
								ERROR("dc10consumerer::HandleEvent ioctl error \n");
							}
							write(fDevice->fd,compressed_data + offset1, size1);
						
							fDevice->buffer_index = 3;
							if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
							{
								//error in ioctl
								ERROR("dc10consumerer::HandleEvent ioctl error \n");
							}
							write(fDevice->fd,compressed_data + offset2, size2);
						
                			firstbuffer = 1;
                			fCodec->enable();
                		}
                		else
                		{
                			//FUNCTION("dc10consumer::HandleEvent  handle buffers- \n");
               				if(ioctl(fDevice->fd, DC10_GET_BUFFER_INFO, fDevice, sizeof(dc10_config)) < 0)
							{
								//error in ioctl
								ERROR("dc10consumerer::HandleEvent ioctl error \n");
							}
							//FUNCTION("dc10consumer::HandleEvent  handle buffers- 0 %lu\n",fDevice->buffer_info[0]);
							//FUNCTION("dc10consumer::HandleEvent  handle buffers- 1 %lu\n",fDevice->buffer_info[1]);
							//FUNCTION("dc10consumer::HandleEvent  handle buffers- 2 %lu\n",fDevice->buffer_info[2]);
							//FUNCTION("dc10consumer::HandleEvent  handle buffers- 3 %lu\n",fDevice->buffer_info[3]);
                    		if(((fDevice->buffer_info[0] & 1 ) == 1)
                    			&&   ((fDevice->buffer_info[1] & 1 ) == 1))
							{
								//PRINTF("dc10consumerer::HandleEvent buffer 0-1\n");
				
								fDevice->buffer_index = 0;
								if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
								{
									//error in ioctl
									ERROR("dc10consumerer::HandleEvent ioctl error \n");
								}
								write(fDevice->fd,compressed_data + offset1, size1);
						
								fDevice->buffer_index = 1;
								if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
								{
									//error in ioctl
									ERROR("dc10consumerer::HandleEvent ioctl error \n");
								}
								write(fDevice->fd,compressed_data + offset2, size2);
							}
							if(((fDevice->buffer_info[2] & 1 ) == 1)
                    			&&   ((fDevice->buffer_info[3] & 1 ) == 1))
							{
								//PRINTF("dc10consumerer::HandleEvent buffer 2-3\n");
								fDevice->buffer_index = 2;
								if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
								{
									//error in ioctl
									ERROR("dc10consumerer::HandleEvent ioctl error \n");
								}
								write(fDevice->fd,compressed_data + offset1, size1);
						
								fDevice->buffer_index = 3;
								if(ioctl(fDevice->fd, DC10_WRITE_INDEX, fDevice, sizeof(dc10_config)) < 0)
								{
									//error in ioctl
									ERROR("dc10consumerer::HandleEvent ioctl error \n");
								}
								write(fDevice->fd,compressed_data + offset2, size2);
							}
						}
						
					}
                }
                else PRINTF("ERROR: Consumer not running\n");

                buffer->Recycle();
            }
            break;
		}
		case BTimedEventQueue::B_SEEK:
			PRINTF("ignoring unhandled event B_SEEK\n");
			break;

		case BTimedEventQueue::B_HARDWARE:
		default:
			PRINTF("ignoring unknown event 0x%lx\n", event->type);
			break;
	}
	//FUNCTION("dc10consumer::HandleEvent - End\n");
}

void dc10consumer::SetRunMode(run_mode mode)
{
	FUNCTION("dc10consumer::SetRunMode \n");
	BMediaEventLooper::SetRunMode(mode);
	BMediaNode::SetRunMode(mode);
}

//
// BBufferConsumer hooks
//



status_t dc10consumer::AcceptFormat(const media_destination &/*dest*/, 
									media_format *format)
{
	FUNCTION("dc10consumer::AcceptFormat -Begin\n");

	if (mInitCheck < B_OK)
		return B_NO_INIT;
		
	if (format->type == B_MEDIA_NO_TYPE)
	{
		format->type = B_MEDIA_ENCODED_VIDEO;	
		return B_MEDIA_BAD_FORMAT;
	}
	if (format->type != B_MEDIA_ENCODED_VIDEO)
	{
		return B_MEDIA_BAD_FORMAT;
	}
	
	if (format->type == B_MEDIA_ENCODED_VIDEO)
	{
		if (format->u.encoded_video.forward_history != 0  )
		{
			ERROR("dc10consumer::FormatProposal - format->u.encoded_video.forward_history ERROR\n");
			return B_MEDIA_BAD_FORMAT;
		}
		
		if (format->u.encoded_video.backward_history != 0  )
		{
			ERROR("dc10consumer::FormatProposal - format->u.encoded_video.backward_history ERROR\n");
			return B_MEDIA_BAD_FORMAT;
		}
		
		
		if (format->u.encoded_video.output.display.format != B_NO_COLOR_SPACE)		
		{
			ERROR("dc10consumer::FormatProposal - BAD DISPLAY FORMAT\n");
			return B_MEDIA_BAD_FORMAT;
		}
		
		
		if (format->u.encoded_video.output.orientation != B_VIDEO_TOP_LEFT_RIGHT)
		{
			ERROR("dc10consumer::FormatProposal - BAD ORIENTATION\n");
			return B_MEDIA_BAD_FORMAT;
		}
			
		// First active should be 0
		
		if (format->u.encoded_video.output.first_active != 0)
		{
			ERROR("dc10consumer::FormatProposal - BAD FIRST ACTIVE\n");
			return B_MEDIA_BAD_FORMAT;
		}	
			
			
		// Interlaced
		if (format->u.encoded_video.output.interlace != 1)
		{
			ERROR("dc10consumer::FormatProposal - BAD interlace\n");
			return B_MEDIA_BAD_FORMAT;
		}	
		
		// Field rate
		if ((format->u.encoded_video.output.field_rate < 24.9)
			|| ((format->u.encoded_video.output.field_rate > 25.1) && (format->u.encoded_video.output.field_rate < 29.9))
			||(format->u.encoded_video.output.field_rate > 30.0))
		{
			ERROR("dc10consumer::FormatProposal - BAD FIELD RATE %f\n",format->u.encoded_video.output.field_rate);
			return B_MEDIA_BAD_FORMAT;
		}
		if ((format->u.encoded_video.output.field_rate > 29.9)
			&&(format->u.encoded_video.output.field_rate < 30.0))
		{
			mVideoFormat = FORMAT_NTSC;
			if ((format->u.encoded_video.encoding != pjpeg_ntsc_half_format.encoding )
				&&(format->u.encoded_video.encoding != pjpeg_ntsc_format.encoding ))
			{
				ERROR("dc10consumer::FormatProposal - BAD ENCODING FORMAT %d %d %d\n",
					format->u.encoded_video.encoding,
					pjpeg_ntsc_half_format.encoding,
					pjpeg_ntsc_format.encoding);
				return B_MEDIA_BAD_FORMAT;
			}
			
			if (format->u.encoded_video.encoding == pjpeg_ntsc_half_format.encoding )
			{
				mEncoding = 2;
				if (format->u.encoded_video.output.display.line_count != 240)
				{
					ERROR("dc10consumer::FormatProposal - BAD LINE COUNT\n");
					return B_MEDIA_BAD_FORMAT;
				}
				if (format->u.encoded_video.output.display.line_width != 320)
				{
					ERROR("dc10consumer::FormatProposal - BAD LINE WIDTH\n");
					return B_MEDIA_BAD_FORMAT;
				}
			}
			
			if (format->u.encoded_video.encoding == pjpeg_ntsc_format.encoding )
			{
				mEncoding = 1;
				if (format->u.encoded_video.output.display.line_count != 480)
				{
					ERROR("dc10consumer::FormatProposal - BAD LINE COUNT\n");
					return B_MEDIA_BAD_FORMAT;
				}
				if (format->u.encoded_video.output.display.line_width != 640)
				{
					ERROR("dc10consumer::FormatProposal - BAD LINE WIDTH\n");
					return B_MEDIA_BAD_FORMAT;
				}
			}
		}	
		if ((format->u.encoded_video.output.field_rate > 24.9)
			&&(format->u.encoded_video.output.field_rate < 25.1))
		{
			mVideoFormat = FORMAT_PAL;
			if ((format->u.encoded_video.encoding != pjpeg_pal_half_format.encoding )
				&&(format->u.encoded_video.encoding != pjpeg_pal_format.encoding ))
			{
				ERROR("dc10consumer::FormatProposal - BAD ENCODING FORMAT\n");
				return B_MEDIA_BAD_FORMAT;
			}
			
			if (format->u.encoded_video.encoding == pjpeg_pal_half_format.encoding )
			{
				mEncoding = 2;
				if (format->u.encoded_video.output.display.line_count != 288)
				{
					ERROR("dc10consumer::FormatProposal - BAD LINE COUNT\n");
					return B_MEDIA_BAD_FORMAT;
				}
				if (format->u.encoded_video.output.display.line_width != 384)
				{
					ERROR("dc10consumer::FormatProposal - BAD LINE WIDTH\n");
					return B_MEDIA_BAD_FORMAT;
				}
			}
			
			if (format->u.encoded_video.encoding == pjpeg_pal_format.encoding )
			{
				mEncoding = 1;
				if (format->u.encoded_video.output.display.line_count != 576)
				{
					ERROR("dc10consumer::FormatProposal - BAD LINE COUNT\n");
					return B_MEDIA_BAD_FORMAT;
				}
				if (format->u.encoded_video.output.display.line_width != 768)
				{
					ERROR("dc10consumer::FormatProposal - BAD LINE WIDTH\n");
					return B_MEDIA_BAD_FORMAT;
				}
			}
		}			
	}	
		    
	FUNCTION("dc10consumer::AcceptFormat -End\n");
	return B_OK;
}

status_t
dc10consumer::GetNextInput(int32 *cookie, 
						   media_input *out_input)
{
	FUNCTION("dc10consumer::GetNextInput -Begin\n");
	if (mInitCheck < B_OK)
	{
		ERROR("dc10consumer::GetNextInput mInitCheck\n");
		return B_NO_INIT;
	}
	
	if (!cookie || !out_input)
	{
		ERROR("dc10consumer::GetNextInput pointeur non null\n");
		return B_BAD_VALUE;
	}
	
	if (*cookie != 0) 
	{
		ERROR("dc10consumer::GetNextInput *cookie non 0\n");
		return B_BAD_INDEX;
	}
	*out_input = mInput;
	(*cookie)++;
	FUNCTION("dc10consumer::GetNextInput -End\n");
	return B_OK;
}

void
dc10consumer::DisposeInputCookie(int32 /*cookie*/)
{
	FUNCTION("dc10consumer::DisposeInputCookie\n");
}




void 
dc10consumer::ConstructControlWeb()
{	
	FUNCTION("dc10consumer::ConstructControlWeb - BEGIN\n");
	
	// Create a fresh web
	BParameterWeb *web = new BParameterWeb();	

	// Control tab
	BParameterGroup *controlGroup = web->MakeGroup("Output");	
	
									
	BParameterGroup *outputGroup = controlGroup->MakeGroup("Outputs");
		
	outputGroup->MakeNullParameter(1, B_MEDIA_ENCODED_VIDEO, "This device has no control.", B_GENERIC);
	outputGroup->MakeNullParameter(1, B_MEDIA_ENCODED_VIDEO, "The only formats accepted are :.", B_GENERIC);
	outputGroup->MakeNullParameter(1, B_MEDIA_ENCODED_VIDEO, "P-JPEG 320*240 @ 29.97 fps", B_GENERIC);
	outputGroup->MakeNullParameter(1, B_MEDIA_ENCODED_VIDEO, "P-JPEG 384*288 @ 25.00 fps", B_GENERIC);
	outputGroup->MakeNullParameter(1, B_MEDIA_ENCODED_VIDEO, "M-JPEG 640*480 @ 29.97 fps", B_GENERIC);
	outputGroup->MakeNullParameter(1, B_MEDIA_ENCODED_VIDEO, "M-JPEG 768*576 @ 25.00 fps", B_GENERIC);
	
	SetParameterWeb(web);

	FUNCTION("dc10consumer::ConstructControlWeb - END\n");
}

status_t dc10consumer::GetParameterValue(int32 id, 
										 bigtime_t *last_change, 
										 void */*value*/, 
										 size_t *ioSize)
{
	FUNCTION("dc10consumer::GetParameterValue - Begin\n");
	ERROR("dc10consumer::GetParameterValue - %d  Value: %d  Size: %d\n", id, *((int32 *)value), *ioSize);
	
	if (last_change)
		*last_change = bigtime_t(0);
		
	
	*ioSize = sizeof(int32);
	
	switch(id)
	{
		default:
			return B_BAD_VALUE;
		break;
	}
	FUNCTION("dc10consumer::GetParameterValue - End\n");
	return B_OK;
}

void dc10consumer::SetParameterValue(int32 id, 
									 bigtime_t /*when*/, 
									 const void */*value*/, 
									 size_t /*size*/)
{
	FUNCTION("dc10consumer::SetParameterValue - Begin\n");
	PROGRESS("dc10consumer::SetParameterValue ID: %d  Value: %d %d\n",
		id, (int32)*((int32 *)value), (int32)*((float *)value));
		
	//int32 aValue = *((int32 *)value);
	//float floatValue = *((float *)value);
	
	switch(id)
	{
		default:
			ERROR("dc10consumer::SetParameterValue - BAD ID\n");
		break;
		
	}
	FUNCTION("dc10consumer::SetParameterValue - end\n");
}

status_t dc10consumer::StartControlPanel(BMessenger * outMessenger)
{
	FUNCTION("dc10consumer::StartControlPanel\n");
    return BControllable::StartControlPanel(outMessenger);
}


void dc10consumer::ProducerDataStatus(const media_destination & /*for_whom*/,
								 	  int32 /*status*/, 
								 	  bigtime_t /*at_performance_time*/)
{
	FUNCTION("dc10consumer::ProducerDataStatus\n");
}

status_t dc10consumer::GetLatencyFor(const media_destination &for_whom,
									 bigtime_t *out_latency, 
									 media_node_id *out_timesource)
{
	FUNCTION("dc10consumer::GetLatencyFor - Begin\n");
	if (!out_latency || !out_timesource)
		return B_BAD_VALUE;

	 if (mInput.destination != for_whom)
        return B_MEDIA_BAD_DESTINATION;

    *out_latency = 0LL;

	*out_timesource = TimeSource()->ID();
	FUNCTION("dc10consumer::GetLatencyFor - End\n");

	return B_OK;
}

status_t dc10consumer::Connected(const media_source &/*producer*/, 
								 const media_destination &/*where*/,
								 const media_format &with_format, 
								 media_input */*out_input*/)
{
	FUNCTION("dc10consumer::Connected - Begin\n");

	firstbuffer = 0;
	
	fd=open(mName,O_RDWR);
	if(fd<0)
	{
		ERROR("dc10consumer::dc10producer Erreur :: main ouverture device  %s\n",mName);
		return B_ERROR;
	}		
	else
	{
		PROGRESS("dc10consumer::Connected OK :: %lu \n",fd);
	}

	fCodec = new dc10codec();
	fCodec->setup(fd,MODE_DECOMPRESS);
	fDevice = &fCodec->mDevice; 
	
	//mVideoFormat = FORMAT_NTSC;
	fCodec->decoder_commande(mVideoFormat);
	fCodec->decoder_commande(INPUT_ZR36060);
	fCodec->encoder_commande(mVideoFormat);
	fCodec->encoder_commande(INPUT_ZR36060);
	
	
	fCodec->init(mVideoFormat,
    			 with_format.u.encoded_video.output.display.line_width, 
    			 with_format.u.encoded_video.output.display.line_count,
    			 MODE_DECOMPRESS,
    			 10);
	
	
	
	snooze(1000);
	mState = 1;
	FUNCTION("dc10consumer::Connected - End\n");
	return B_OK;
}

void dc10consumer::Disconnected(const media_source &/*producer*/,
						   const media_destination &/*where*/)
{
	FUNCTION("dc10consumer::Disconnected - Begin\n");
	mState = 0;
	fCodec->encoder_commande(COLOR_MIR);
	
	if(fd >= 0)
	{
		close(fd);
		fd = -1;
	}
	if(fCodec != NULL)
	{
		delete fCodec ;
		fCodec = NULL;
	}
		
	fDevice = NULL;
	FUNCTION("dc10consumer::Disconnected - End\n");
}

status_t dc10consumer::FormatChanged(
		const media_source &/*producer*/, const media_destination &/*consumer*/, 
		int32, const media_format &/*format*/)
{
	FUNCTION("dc10consumer::FormatChanged - Begin\n");
	FUNCTION("dc10consumer::FormatChanged - End\n");
	return B_ERROR;
}

void dc10consumer::BufferReceived(BBuffer *buffer)
{
	//FUNCTION("dc10consumer::BufferReceived - Begin\n");
	 EventQueue()->AddEvent(media_timed_event(
        buffer->Header()->start_time,
        BTimedEventQueue::B_HANDLE_BUFFER,
        buffer,
        BTimedEventQueue::B_RECYCLE_BUFFER));
	//FUNCTION("dc10consumer::BufferReceived- End\n");
}



port_id dc10consumer::ControlPort() const
{
	//FUNCTION("dc10consumer::ControlPort \n");
    return BMediaNode::ControlPort();
}

status_t dc10consumer::SeekTagRequested(const media_destination &destination, 
										bigtime_t /*in_target_time*/,
										uint32 /*in_flags*/, 
										media_seek_tag */*out_seek_tag*/,
										bigtime_t */*out_tagged_time*/, 
										uint32 */*out_flags*/)
{
	FUNCTION("dc10consumer::SeekTagRequested - Begin\n");
	if (destination.id != 0) 
		return B_MEDIA_BAD_DESTINATION;

	PRINTF("dc10consumer::SeekTagRequested -- Unsupported!\n");

	return ENOSYS;
}
