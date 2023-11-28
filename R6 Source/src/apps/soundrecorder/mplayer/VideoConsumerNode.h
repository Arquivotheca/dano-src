#ifndef __VIDEO_CONSUMER_NODE__
#define __VIDEO_CONSUMER_NODE__

#include <OS.h>
#include <BufferConsumer.h>

class VideoView;

const int kNumBuffers = 1;

class VideoConsumerNode : public BBufferConsumer {
public:

	VideoConsumerNode(const char *name, VideoView *, media_node*, bool debug = false);
	~VideoConsumerNode();

protected:

	void ControlLoop();
	static int32 StartControlLoop(void*);

	virtual port_id ControlPort() const;
	virtual BMediaAddOn* AddOn(int32*) const;
	virtual	status_t AcceptFormat(const media_destination & dest,
			media_format * format);	
	virtual	status_t GetNextInput(int32 *cookie,
			media_input * out_input);
	virtual	void DisposeInputCookie(int32 cookie);
	virtual	void BufferReceived(BBuffer * buffer);
	virtual	void ProducerDataStatus(
			const media_destination & for_whom,
			int32 status, bigtime_t at_media_time);
	virtual	status_t GetLatencyFor(
			const media_destination & for_whom,
			bigtime_t * out_latency,
			media_node_id * out_timesource);
	virtual	status_t Connected(const media_source & producer,	
			const media_destination & where,
			const media_format & with_format,
			media_input * out_input);
	virtual	void Disconnected(const media_source & producer,
			const media_destination & where);
	virtual	status_t FormatChanged(const media_source & producer,
			const media_destination & consumer, 
			int32 from_change_count,
			const media_format & format);
	virtual	void Stop(bigtime_t performance_time,
			bool immediate);

private:

	void Debug(const char *fmt, ...);

	port_id				fControlPort;
	thread_id 			fControlLoopThread;
	VideoView 			*fOutputView;
	bool				fUsingBitmapBufferGroup;
	
	BBitmap 			*fOutputBitmap[kNumBuffers];
	int32				fBitmapID[kNumBuffers];

	BTimeSource			*fTimeSource;
	BTimedEventQueue	*fEventQueue;
	bool				fConnected;
	bool				fStopping;
	bigtime_t			fStopTime;
	BBufferGroup		*fBufferGroup;
	bool				fKeepGoing;
	bool				fDebugOutput;
	bool				fStopped;
};

#endif
