#ifndef MCONTROLBT848_H
#define MCONTROLBT848_H

#include <MediaNode.h>
#include <MediaEventLooper.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <TimeSource.h>


#include "VideoDefs.h"
#include "Bt848Source.h"

#include <Region.h>

// Things we use in here
class BVideoControls;
class BVideoSource;
class BTuner;
class BAudioMux;
class BVideoMux;
class BVideoCaptureConfig;
class BParameterGroup;
class BMediaAddOn;

namespace BPrivate {

class Settings;
class ScalarValueSetting;
class BooleanValueSetting;
class StringValueSetting;
class EnumeratedStringValueSetting;

}

using namespace BPrivate;

#define MAX_CONNECTIONS 2
#define MAX_BUFFERS 3

enum EVideoInputName
{
	B_CONTROL_NO_VIDEO = 0,
	B_CONTROL_COMPOSITE1,
	B_CONTROL_COMPOSITE2,
	B_CONTROL_COMPOSITE3,
	B_CONTROL_COMPOSITE4,
	B_CONTROL_TUNER_VIDEO,
	B_CONTROL_SVIDEO,
	B_CONTROL_COLORBARS
};

enum EAudioInputName
{
	B_CONTROL_NO_AUDIO = 0,
	B_CONTROL_MUTE,
	B_CONTROL_TUNER_AUDIO,
	B_CONTROL_EXTERNAL_JACK,
	B_CONTROL_INTERNAL_JACK,
	B_CONTROL_RADIO
};

enum EControlID
{
	B_CONTROL_VIDEO_FORMAT = 1L,
	B_CONTROL_TUNER_LOCALE,
	B_CONTROL_TUNER_BRAND,
	B_CONTROL_VIDEO_INPUT_1,
	B_CONTROL_VIDEO_INPUT_2,
	B_CONTROL_VIDEO_INPUT_3,
	B_CONTROL_VIDEO_INPUT_4,
	B_CONTROL_AUDIO_INPUT_1,
	B_CONTROL_AUDIO_INPUT_2,
	B_CONTROL_AUDIO_INPUT_3,
	B_CONTROL_AUDIO_INPUT_4,
	B_CONTROL_MSP_MODE,
	B_CONTROL_AUDIO_MUX_TYPE,
	B_CONTROL_CHANNEL,
	B_CONTROL_VIDEO_INPUT,
	B_CONTROL_AUDIO_INPUT,
	B_CONTROL_AUDIO_MODE,
	B_CONTROL_BRIGHTNESS,
	B_CONTROL_CONTRAST,
	B_CONTROL_SATURATION,
	B_CONTROL_HUE,
	B_CONTROL_LUMA_COMB,
	B_CONTROL_LUMA_CORING,
	B_CONTROL_CHROMA_COMB,
	B_CONTROL_GAMMA_CORRECTION,
	B_CONTROL_ERROR_DIFFUSION,
	B_CONTROL_PLL,
	B_CONTROL_DEFAULT_SIZE,
	B_CONTROL_DEFAULT_COLORSPACE,
	B_CONTROL_I2C_DEVICES,
	B_CONTROL_IS_HITACHI
};

enum Bt848Message
{
	B_BT848_CHANNEL_UP = 0x60000001L,
	B_BT848_CHANNEL_DOWN,
	B_BT848_SCAN_UP,
	B_BT848_SCAN_DOWN,
	B_BT848_FINE_TUNE_UP,
	B_BT848_FINE_TUNE_DOWN,
	B_BT848_TUNE_FREQUENCY,
	B_BT848_TUNE_INDEX,
	
	B_BT848_FRAME_READY = 0x61000001L,
};

struct bt848_thread_info {
	port_id	port;
	Bt848Source *bt848;
	bool	quit;
};

struct bt848_frame_info {
	uint32 index;
};

typedef struct
{
	port_id	port;
	uint32	frequency;
	uint32	index;
	bool	isLocked;
	bool	videoPresent;
} bt848_msg_info;
	

typedef struct 
{
	char	name[15];
	uint32	frequency;
} ChannelInfo;


class BBt848Controllable :
	public BMediaEventLooper,
	public BBufferProducer,
	public BControllable,
	public BTimeSource
{
	public:
									BBt848Controllable(const uint32 internal_id, const char *devicename,
										const char *nodename, BMediaAddOn *addon=0);
									~BBt848Controllable();
									
		status_t					InitCheck() { return mInitCheck; }

	/* BMediaNode */
	public:
		virtual	BMediaAddOn*		AddOn(long *) const;
	protected:
		virtual void				Start(bigtime_t performanceTime);
		virtual void				Stop(bigtime_t performanceTime, bool immediate);
		virtual void				Preroll();
//		virtual void				SetTimeSource(BTimeSource *time_source);
		virtual status_t			HandleMessage(int32 code, const void *msg, size_t size);
		virtual void				NodeRegistered();
		virtual void				SetRunMode(run_mode mode);

	/* BMediaEventLooper */
	private:
		virtual void				HandleEvent(const media_timed_event *event, bigtime_t lateness, bool realTimeEvent);

	/* TimeSource */
	protected:
		virtual	status_t 			TimeSourceOp(const time_source_op_info & op, void * _reserved);
	

	/* BBufferProducer */
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

	/* BControllable */
	protected:
		virtual	status_t 			GetParameterValue(int32 id, bigtime_t * last_change, void * value, size_t * ioSize);
		virtual	void 				SetParameterValue(int32 id, bigtime_t when, const void * value, size_t size);
//		virtual	status_t 			StartControlPanel(BMessenger * out_messenger);

	/* BTimeSource */
//	public:
//		virtual	status_t 			SnoozeUntil(bigtime_t performance_time, bigtime_t with_latency = 0,
//										bool retry_signals = false);		
//	private:
//		virtual	status_t 			RemoveMe(BMediaNode * node);
//		virtual	status_t 			AddMe(BMediaNode * node);
	

	private:
		virtual	status_t 			DeleteHook(BMediaNode * node);
		
		void						HandleStart(bigtime_t performanceTime);
		void						HandleStop(bigtime_t performanceTime);
		void						SendBuffers(BVideoImage *image, uint32 whichIndex);
		
		bigtime_t					fNextCaptureTime;

		void						StartCapture();
		void						RestartCapture(bool resetCounter = false); // opposite from Bt848Source
		void						SwitchCapture();
		void						StopCapture();
		status_t					ReconfigureCapture();

		bool						ReadTvFavorites();
		bool						ReadFmFavorites();
		status_t	 				MapCaptureBuffers(
										BBufferGroup *,
										const media_video_display_info & display,
										BVideoImage **,
										uint32 &);
		void						PrepareBufferMap();
		void						SetUpSettings(
										const char *filename,
										const char *directory);
		void						QuitSettings();
		int32						FindIndex(
										const char *setting_table[],
										const char *setting);
										
		// Constructing the control web
		void						RequestNewWeb();
		void						ConstructControlWeb();
		
		// Our own private Initialization
		void						Initialize();
		void						Uninitialize();

		uint32						mBufferCountF1;
		uint32						mBufferCountF2;
		BVideoImage					**mRingBufferF1;
		BVideoImage					**mRingBufferF2;
		BVideoImage					*mBufferF1[MAX_BUFFERS];
		BVideoImage					*mBufferF2[MAX_BUFFERS];
	
		BBuffer 					*bufferF1[MAX_BUFFERS];
		BBuffer 					*bufferF2[MAX_BUFFERS];
		
		BRegion						mClipRegion;
		
		// Device specifics
		Bt848Source 				*mDevice;
		thread_id					mFrameReadyThread;
		bt848_thread_info			mFrameReadyThreadInfo;
		BTuner						*mTuner;
		BAudioMux					*mAudioMux;
		uint32						mAudioMuxType;
		BVideoMux					*mVideoMux;
		BVideoControls				*mVideoControls;
		BI2CBus						*mI2CBus;
		bool						mHitachi;
	
		// Current settings
		uint32						mAudioSource;
		uint32						mAudioMode;
		uint32						mMspMode;
		status_t					mInitCheck;	
	
		media_format				mVideoFormat; 	/* Our raw_video format */
		media_format				mQueryFormat; 	/* Our wildcard raw_video format */
		
		media_output				mOutputF1;
		media_output				mOutputF2;
		BBufferGroup 				*mBufferGroupF1;
		BBufferGroup 				*mBufferGroupF2;
		BBufferGroup 				*mMappedBuffersF1;
		BBufferGroup 				*mMappedBuffersF2;
		bool						mBuffersMappedF1;
		bool						mBuffersMappedF2;
		bool						mConnectionProposedF1;
		bool						mConnectionProposedF2;	
		bool						mConnectionActiveF1;
		bool						mConnectionActiveF2;	
		bool						mUsingConsumerBuffersF1;
		bool						mUsingConsumerBuffersF2;
		bt848_cliplist				mClipListF1;
		bt848_cliplist				mClipListF2;
		int16						**mActiveClipListF1;
		int16						**mActiveClipListF2;
		
		uint32						mInternalID;
		BMediaAddOn					*mAddOn;
		char						*mName;
				
		uint32						mFrameNumber;
		float						mRate;
		
		volatile bool				mCaptureQuit;
		
		bool						mRunning;
		bool 						mStarting;
		bool 						mStopping;
		bool						mMutedF1;
		bool						mMutedF2;
	
		bigtime_t					mMediaTime;
		bigtime_t					mDownstreamLatency;
			
		uint32						mXSize;
		uint32						mYSize;
		color_space					mColorspace;
		bool						mInterlaced;
		
		EVideoInputName				mVideoInput[4];
		EAudioInputName				mAudioInput[4];
	
	
		bool						mHasTvFavorites;
		bool						mHasFmFavorites;
		
		uint32						mTvFavoritesIndex;
		uint32						mFmFavoritesIndex;
			
		ChannelInfo					mTvFavorites[128];
		ChannelInfo					mFmFavorites[128];
		
		char 						mSettingsFile[32];
		Settings 					*settings;
		
		ScalarValueSetting 			*videoSourceSetting;
		ScalarValueSetting 			*audioSourceSetting;
		ScalarValueSetting 			*audioMuxTypeSetting;
	
		ScalarValueSetting 			*brightnessSetting;
		ScalarValueSetting 			*contrastSetting;
		ScalarValueSetting 			*saturationSetting;
		ScalarValueSetting 			*hueSetting;
		
		BooleanValueSetting 		*pllSetting;
		BooleanValueSetting 		*gammaSetting;
		BooleanValueSetting 		*lumaCoringSetting;
		BooleanValueSetting 		*lumaFilterSetting;
		BooleanValueSetting 		*chromaFilterSetting;
		BooleanValueSetting 		*errorDiffusionSetting;
			
		StringValueSetting 			*channelSetting;
		
		EnumeratedStringValueSetting *videoNameSetting[4];
		EnumeratedStringValueSetting *audioNameSetting[4];
		
		EnumeratedStringValueSetting *audioModeSetting;
		EnumeratedStringValueSetting *mspModeSetting;
	
		EnumeratedStringValueSetting *tunerBrandSetting;
		EnumeratedStringValueSetting *formatSetting;
		EnumeratedStringValueSetting *tunerLocaleSetting;
	
		EnumeratedStringValueSetting *imageSizeSetting;
		EnumeratedStringValueSetting *colorspaceSetting;
	
									BBt848Controllable(const BBt848Controllable &);
									BBt848Controllable & operator = (const BBt848Controllable &);
};



#endif
