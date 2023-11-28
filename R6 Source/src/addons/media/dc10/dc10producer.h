#ifndef DC10PRODUCER_H
#define DC10PRODUCER_H

#include <MediaNode.h>
#include <MediaEventLooper.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <TimeSource.h>

#include <dc10_driver.h>
#include "definition.h"
#include "dc10codec.h"


#define MAX_CONNECTIONS 1
#define MAX_BUFFERS 3

enum DC10Message
{	
	DC10_FRAME_READY = 0x61000001L,
	DC10_SUSPEND_THREAD, 
	DC10_GO 
};
enum EVideoInputName
{
	B_CONTROL_NO_VIDEO = 0,
	B_CONTROL_COMPOSITE,
	B_CONTROL_SVIDEO,
	B_CONTROL_COLORBARS
};




enum EControlID
{
	B_CONTROL_VIDEO_INPUT = 1L,
	B_CONTROL_VIDEO_STANDARD_IN,
	B_CONTROL_VIDEO_BIT_RATE,
	B_BRIGHTNESS,
	B_CONTRAST,
	B_SATURATION,
	B_HUE
};

struct dc10_thread_info {
	port_id	port;
	dc10_config * dc10;
	bool	quit;
	bool   running;
	BBuffer ** fBuf; 
};

class dc10producer :
	public BMediaEventLooper,
	public BBufferProducer,
	public BControllable,
	public BTimeSource
{
	public:
									dc10producer(const uint32 internal_id,
												 const char * devicename,
												 const char *nodename, 
												 BMediaAddOn *addon=0);
									~dc10producer();
									
		status_t					InitCheck() { return mInitCheck; }

	// BMediaNode 
	public:
		virtual	BMediaAddOn*		AddOn(long *) const;
		virtual status_t 					InitCheck(const char ** outFailureText);
	protected:
		virtual void				Start(bigtime_t performanceTime);
		virtual void				Stop(bigtime_t performanceTime, bool immediate);
		virtual void				Preroll();
//		virtual void				SetTimeSource(BTimeSource *time_source);
		virtual status_t			HandleMessage(int32 code, const void *msg, size_t size);
		virtual void				NodeRegistered();
		virtual void				SetRunMode(run_mode mode);

	// BMediaEventLooper 
	private:
		virtual void				HandleEvent(const media_timed_event *event, bigtime_t lateness, bool realTimeEvent);

	// TimeSource 
	protected:
		virtual	status_t 			TimeSourceOp(const time_source_op_info & op, void * _reserved);
	
	// BBufferProducer 
	protected:
		virtual status_t			FormatSuggestionRequested(media_type type, int32 quality, media_format *format);
		virtual status_t			FormatProposal(const media_source &output, media_format *format);
		virtual status_t			FormatChangeRequested(const media_source &source, const media_destination &dest,
										media_format *io_format, int32 *out_change_count);
		virtual status_t			GetNextOutput(int32 *cookie, media_output *out_output);
		virtual status_t			DisposeOutputCookie(int32 cookie);
		virtual status_t			SetBufferGroup(const media_source &for_source, BBufferGroup *group);
		virtual status_t			VideoClippingChanged(const media_source &for_source, int16 num_shorts,
										int16 *clip_data, const media_video_display_info &display,
										int32 * out_from_change_count);
		virtual status_t			GetLatency(bigtime_t *out_latency);
		virtual status_t			PrepareToConnect(const media_source &what, const media_destination &where,
										media_format *format, media_source *out_source, char *out_name);
		virtual void				Connect(status_t error, const media_source &source,
										const media_destination &dest, const media_format &format,
										char * io_name);
		virtual void				Disconnect(const media_source &what, const media_destination &where);
		virtual void				LateNoticeReceived(const media_source &what, bigtime_t how_much,
										bigtime_t performance_time);
		virtual void				EnableOutput(const media_source &what, bool enabled, int32 *change_tag);
		virtual status_t			SetPlayRate(int32 numer, int32 denom);
		virtual	void 				AdditionalBufferRequested(const media_source & source, media_buffer_id prev_buffer,
										bigtime_t prev_time, const media_seek_tag * prev_tag);


	// BControllable 
	protected:
		virtual	status_t 			GetParameterValue(int32 id, bigtime_t * last_change, void * value, size_t * ioSize);
		virtual	void 				SetParameterValue(int32 id, bigtime_t when, const void * value, size_t size);
		virtual	status_t 			StartControlPanel(BMessenger * out_messenger);


	
		// Constructing the control web
		void						RequestNewWeb();
		void						ConstructControlWeb();
		
		void 						CreateBufferGroup(void);
		void 						DeleteBufferGroup(void);

		// Device specifics
		//dc10_config					mDevice;
		dc10_config					*fDevice;
		int32 						fd;
		char *						mName;		
		
		thread_id					mFrameReadyThread;
		dc10_thread_info			mFrameReadyThreadInfo; 
	
		status_t					mInitCheck;	
	
		EVideoInputName				mVideoInput;
		int32						mVideoStandardIn;
		
		uint32						mInternalID;
		BMediaAddOn					*mAddOn;
		
		bigtime_t					mMediaTime;
		bigtime_t					mDownstreamLatency;
		
		media_output        		mOutput;
    	float						mRate;
		
		BBufferGroup 				*fBufferGroup1;
		BBufferGroup 				*fBufferGroup;
	

		
		bool						mInterlaced;
	
		uint32 						mSize;
		uint32 						mFrameNumber;	
		float 						mBitRate;
		
		uint32 						mXSize,mYSize;	
		color_space					mColorSpace;		
		
		dc10codec					*fCodec;
		
		float 						mBrightness;
		float						mContrast;
		float						mSaturation;
		float						mHue;
		
};



#endif
