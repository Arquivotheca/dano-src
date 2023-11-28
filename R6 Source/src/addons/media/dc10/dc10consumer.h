#ifndef DC10CONSUMER_H_
#define DC10CONSUMER_H_

#include <MediaNode.h>
#include <MediaEventLooper.h>
#include <media/Buffer.h>
#include <media/BufferConsumer.h>
#include <media/Controllable.h>
#include <media/TimeSource.h>


#include <dc10_driver.h>
#include "definition.h"
#include "dc10codec.h"


enum EControl2ID
{
	B_CONTROL_VIDEO_OUTPUT = 1L
};


class dc10consumer :
	public virtual BMediaEventLooper,
	public virtual BBufferConsumer,
	public virtual BControllable
{
public:
					dc10consumer(const uint32 internal_id,
								 const char * devicename,
								 const char *nodename,
								 BMediaAddOn *addon=0);
					~dc10consumer();

		status_t	InitCheck()  { return mInitCheck; }


// BMediaNode
public:
		BMediaAddOn *AddOn(int32 *outInternalID) const;
protected:
virtual	void		NodeRegistered();
virtual	void		Preroll();
virtual port_id 	ControlPort() const;

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
virtual void 		Start(bigtime_t performance_time);
virtual void 		Stop(bigtime_t performance_time,
    					 bool immediate);
virtual void 		Seek(bigtime_t media_time,
    					 bigtime_t performance_time);
virtual void 		TimeWarp(bigtime_t at_real_time,
							 bigtime_t to_performance_time);
virtual status_t 	DeleteHook(BMediaNode * node);
							    					

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

virtual void        ProducerDataStatus(const media_destination & for_whom,
								 	   int32 status, 
								 	   bigtime_t at_performance_time);

virtual status_t	GetLatencyFor(const media_destination &for_whom,
								  bigtime_t *out_latency, 
								  media_node_id *out_timesource);

virtual status_t    Connected(const media_source &producer, 
							  const media_destination &where,
		 					  const media_format &with_format, 
							  media_input *out_input);


virtual void 		Disconnected(const media_source &producer,
						   		 const media_destination &where);


virtual status_t    FormatChanged(const media_source &producer, 
								  const media_destination &consumer, 
								  int32,
								  const media_format &format);
								  
virtual void		BufferReceived(BBuffer *buffer);

virtual status_t 	RequestCompleted(const media_request_info & info);
								  


virtual status_t 	SeekTagRequested(const media_destination &destination, 
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
virtual	status_t 	StartControlPanel(BMessenger * out_messenger);



// Constructing the control web
		void						RequestNewWeb();
		void						ConstructControlWeb();
		
	

		// Device specifics
		
		dc10_config					*fDevice;
		dc10codec					*fCodec;
		char *						mName;		
		status_t					mInitCheck;	
		int32						mVideoOutput;
		uint32						mInternalID;
		BMediaAddOn					*mAddOn;
		int32						fd;
		media_input        			mInput;		
		float 						mRate;
		int32 						mVideoFormat;
		int32 						mEncoding;
		int32 						firstbuffer;
		uint32 						mState;
		
};

#endif
