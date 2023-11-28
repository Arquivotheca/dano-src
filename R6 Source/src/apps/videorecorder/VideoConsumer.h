//	Copyright (c) 1998-99, Be Incorporated, All Rights Reserved.
//	SMS
/*	VideoConsumer.h	*/

#if !defined(VID_CONSUMER_H)
#define VID_CONSUMER_H

#include <View.h>
#include <Bitmap.h>
#include <Window.h>
#include <MediaNode.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <TranslationKit.h>
#include <BufferConsumer.h>
#include <TimedEventQueue.h>
#include <MediaEventLooper.h>
#include <MediaFormats.h>
#include <MediaDecoder.h>

struct capture_config {
	media_file_format			fileformat;
	media_codec_info			videoencoder;
	media_codec_info			audioencoder;
	float						quality;
	int							interval;
	bool						preview;
//	bool						recordaudio;
	char						filename[64];
	float						audio_jitter;	// sec
};

struct writer_msg_info {
	port_id			port;
	capture_config	config;
#if 0
	//int				width;
	//int				height;
	//color_space		colorspace;
	int					interval;
	bool				preview;
	char				fileNameText[64];

	media_format		format;
	media_file_format	fileformat;
	media_codec_info	encoder;
	float				quality;

	//char			fileformat[64];
	//char			encoder[64];
#endif
};

//#define FTP_INFO			0x60000001
#define WRITER_INFO			0x60000002
#define START_WRITE			0x60000003
#define STOP_WRITE			0x60000004

class VideoConsumer : 
	public BMediaEventLooper,
	public BBufferConsumer
{
public:
						VideoConsumer(
							const char * name,
							BView * view,
							BStringView	* statusLine,
							BMediaAddOn *addon,
							const uint32 internal_id);
						~VideoConsumer();
	
/*	BMediaNode */
public:

	virtual	BMediaAddOn	*AddOn(long *cookie) const;
	virtual void Preroll();
	status_t PrerollStatus;
protected:

	virtual void		NodeRegistered();
	virtual	status_t 	RequestCompleted(
							const media_request_info & info);
							
	virtual	status_t 	HandleMessage(
							int32 message,
							const void * data,
							size_t size);

/*  BMediaEventLooper */
protected:
	virtual void		HandleEvent(
							const media_timed_event *event,
							bigtime_t lateness,
							bool realTimeEvent);

/*	BBufferConsumer */
public:
	
	virtual	status_t	AcceptFormat(
							const media_destination &dest,
							media_format * format);
	virtual	status_t	GetNextInput(
							int32 * cookie,
							media_input * out_input);
							
	virtual	void		DisposeInputCookie(
							int32 cookie);
	
protected:

	virtual	void		BufferReceived(
							BBuffer * buffer);
	
private:

	virtual	void		ProducerDataStatus(
							const media_destination &for_whom,
							int32 status,
							bigtime_t at_media_time);									
	virtual	status_t	GetLatencyFor(
							const media_destination &for_whom,
							bigtime_t * out_latency,
							media_node_id * out_id);	
	virtual	status_t	Connected(
							const media_source &producer,
							const media_destination &where,
							const media_format & with_format,
							media_input * out_input);							
	virtual	void		Disconnected(
							const media_source &producer,
							const media_destination &where);							
	virtual	status_t	FormatChanged(
							const media_source & producer,
							const media_destination & consumer, 
							int32 from_change_count,
							const media_format & format);
							
	status_t		DrawPreview(BBuffer *buffer);
	status_t		ConsumeAudioBuffer(BBuffer *buffer);
	status_t		ConfigureDecoder();
							
/*	implementation */

public:
			status_t	CreateBuffers(
							const media_format & with_format);
							
			void		DeleteBuffers();
							
	void				UpdateStatus(
							char *status);
	
private:

	BStringView	*		mStatusLine;
	uint32				mInternalID;
	BMediaAddOn			*mAddOn;

	bool					mConnectionActive;
	media_input				mVideoIn;
	media_input				mAudioIn;
	media_destination		mDestination;
	bigtime_t				mMyLatency;
	float					mVidRate;
	
	uint8 *mAudioBuffer;
	int mAudioBufferSize;
	int mAudioBufferUsed;
	int mAudioExpandCount;
	int mAudioFramesRepeated;
	int mAudioFramesDropped;
	//BBufferGroup			*mAudioBuffers;

	BWindow					*mWindow;
	BView					*mView;
	BBitmap					*mBitmap[3];
	void					*mBits[3];
	bool					mOurBuffers;
	BBufferGroup			*mBuffers;
	uint32					mBufferMap[3];
	BBitmap					*mPreviewBitmap;
	BMediaBufferDecoder		mDecoder;
	bool					mDecodeFields;
	bool					mBottomField;

	capture_config	mCaptureConfig;

	int mReceiveCount;

	status_t InitMediaFile();
	status_t UninitMediaFile();

	BFile * mFile;
	BMediaFile * mMediaFile;
	BMediaTrack * mVideoTrack;
	media_encode_info mVideoEncodeInfo;
	BMediaTrack * mAudioTrack;
	bool mWriting;
	bigtime_t mWriteStartTime;
	int mAudioFramesWritten;
	int mFramesWritten;
	int mFramesDropped;
};

#endif
