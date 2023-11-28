/*	VideoConsumer.h	*/

#if !defined(VID_CONSUMER_H)
#define VID_CONSUMER_H

#include <View.h>
#include <Bitmap.h>
#include <Window.h>
#include <MediaNode.h>
#include <BufferConsumer.h>

#include "TimedEventQueue.h"

#define MAX_CONNECTIONS 16

class VideoConsumer : public BBufferConsumer
{
public:
						VideoConsumer(
							const uint32 internal_id,
							const char * name,
							BMediaAddOn *addon);
						~VideoConsumer();
	
/*	BMediaNode */
public:
	
	virtual	BMediaAddOn	*AddOn(long *cookie) const;
	virtual	port_id		ControlPort() const;
	
private:
	
	virtual	void		Start(
							bigtime_t performance_time);						
	virtual	void		Stop(
							bigtime_t performance_time,
							bool immediate);						
	virtual	void		Seek(
							bigtime_t media_time,
							bigtime_t performance_time);						
	virtual	void		TimeWarp(
							bigtime_t at_real_time,
							bigtime_t to_performance_time);
protected:

#ifndef R4_MK_API
	virtual	status_t 	RequestCompleted(
							const media_request_info & info);
#endif
							
	virtual	status_t 		HandleMessage(
								int32 message,
								const void * data,
								size_t size);

	
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
							int32 status, bigtime_t at_media_time);									
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
							
/*	implementation */

public:

			void		CreateWindow(
							const char *name,
							const uint32 connection,
							const media_format & with_format);
							
			void		DeleteWindow(
							const uint32 connection);
							
			status_t	CreateBuffers(
							const uint32 connection,
							const media_format & with_format);
							
			void		DeleteBuffers(
							const uint32 connection);
							
	static	status_t	vRun(
							void *data);
														
	void				DisplayThread(
							void);

private:

	uint32				mInternalID;
	BMediaAddOn			*mAddOn;
	thread_id			mDisplayThread;
	port_id				mPort;

	volatile bool		mDisplayQuit;
	
	bool 				mRunning;

	bigtime_t 			mMyLatency;
	
	BTimedEventQueue		*mEventQueue;

	enum { NAME_SIZE = 64 };
	uint32					mConnectionCount;
	bool					mConnectionActive[MAX_CONNECTIONS];
	media_source			mSource[MAX_CONNECTIONS];
	media_destination		mDestination[MAX_CONNECTIONS];
	media_raw_video_format	mFormat[MAX_CONNECTIONS];
	char					mName[MAX_CONNECTIONS][NAME_SIZE];
	BWindow					*mWindow[MAX_CONNECTIONS];
	BView					*mView[MAX_CONNECTIONS];
	BBitmap					*mBitmap[MAX_CONNECTIONS][3];
	bool					mOurBuffers[MAX_CONNECTIONS];
	BBufferGroup			*mBuffers[MAX_CONNECTIONS];
	uint32					mBufferMap[MAX_CONNECTIONS][3];		
};

class VideoWindow : public BWindow
{
public:
						VideoWindow (BRect frame,
							const char *title,
							window_type type,
							uint32 flags);
						~VideoWindow();

	virtual	bool		QuitRequested();
	virtual void		MessageReceived(BMessage *message);
};

#endif /* MY_CONSUMER_H */
