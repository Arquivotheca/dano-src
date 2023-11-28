#if !defined(__GAME_CONSUMER_H__)
#define __GAME_CONSUMER_H__

#include <MediaEventLooper.h>
#include <BufferConsumer.h>
#include <Controllable.h>
#include <TimeSource.h>

#include <Locker.h>
#include <String.h>

#include "game_audio.h"
#include "game_tools.h"

class BBufferGroup;
class BTimedEventQueue;

namespace BPrivate {

class GameAddOn;
class game_buffer;
class GameParameterMap;

class RTLogContext;

//#pragma mark --- GameConsumer ---
// ------------------------------------------------------------------------ //

class GameConsumer :
	public BMediaEventLooper,
	public BBufferConsumer,
	public BTimeSource,
	public BControllable
{
public:
	virtual ~GameConsumer();
	GameConsumer(
		const char* name,
		GameAddOn* addon,
		const char* devicePath,
		int32 dacID,
		int32 addonID);

	void* operator new(size_t s);
	void operator delete(void* p, size_t s);

	status_t InitCheck() const;
	
	int DeviceFD() const;
	
	// stream queue locking interface
	BLocker& QueueLock() const;

#if __RTLOG
	RTLogContext*				log;
#endif

protected: // BMediaNode
	virtual status_t HandleMessage(
		int32 code,
		const void* data,
		size_t size);

	virtual BMediaAddOn* AddOn(
		int32* outID) const;
	
	virtual void SetRunMode(
		run_mode mode);

	virtual void NodeRegistered();

protected: // BMediaEventLooper
	virtual void HandleEvent(
		const media_timed_event* event,
		bigtime_t howLate,
		bool realTimeEvent =false);
	
protected: // BBufferConsumer
	virtual status_t AcceptFormat(
		const media_destination& destination,
		media_format* ioFormat);
	
	virtual void BufferReceived(
		BBuffer* buffer);
	
	virtual status_t Connected(
		const media_source& source,
		const media_destination& destination,
		const media_format& format,
		media_input* outInput);

	virtual void Disconnected(
		const media_source& source,
		const media_destination& destination);
		
	virtual void DisposeInputCookie(
		int32 cookie);
	
	virtual status_t FormatChanged(
		const media_source& source,
		const media_destination& destination,
		int32 changeTag,
		const media_format& newFormat);
		
	virtual status_t GetLatencyFor(
		const media_destination& destination,
		bigtime_t* outLatency,
		media_node_id* outTimeSource);
		
	virtual status_t GetNextInput(
		int32* ioCookie,
		media_input* outInput);

	virtual void ProducerDataStatus(
		const media_destination& destination,
		int32 status,
		bigtime_t tpWhen);
	
protected: // BTimeSource
	virtual status_t TimeSourceOp(
		const time_source_op_info& op,
		void* _reserved);

protected: // BControllable
	virtual status_t GetParameterValue(
		int32 id,
		bigtime_t* outLastChangeTime,
		void* outValue,
		size_t* ioSize);
		
	virtual void SetParameterValue(
		int32 id,
		bigtime_t changeTime,
		const void* value,
		size_t size);

private:
	class DacInfo;
	class Endpoint;
	class EndpointIterator;
	class Stream;
	
	struct split_channel_entry
	{
		// +em: unlikely to ever get used +++++
		int32		source_channel_count;
		DacInfo*	dac;
		int32		dac_channel_offset;
	};
	
	status_t					_initStatus;
	GameAddOn*					_addon;
	BString						_path;
	int							_fd;
	int32						_dacID;
	int32						_addonID;

	// DAC states
	int32						_dacCount;
	DacInfo*					_dac;
	float						_preferredFrameRate;
	uint32						_preferredSampleFormat;
	int16						_preferredSampleBits;
	uint32						_dacFrameCount;

	// endpoint/multistream set	
	Endpoint*					_endpoints;
	int32						_iteratorCount;
	int32						_nextEndpointID;

	// stream set
	BLocker						_queueLock;
	Stream*						_streams;
	Stream*						_timeSourceStream;

	// timesource state
	bigtime_t					_lastPerfPublished;
	bigtime_t					_lastRealPublished;
	float						_drift;
	
	BTimedEventQueue* const		_perfQueue;
	BTimedEventQueue* const		_realQueue;
	
	GameParameterMap*			_parameterMap;

	// Event handlers.
	void ServiceTimeSource();
	void FlushTimeSourceEvents();

	// ValidateFormat(): specialize and/or correct the given format to
	// be compatible with the provided DACs.
	// Call RefreshDacInfo() first.
	status_t ValidateFormat(
		media_multi_audio_format* ioFormat,
		DacInfo** dacs,
		uint32 dacCount);
		
	status_t ConstrainFrameRate(
		float* ioFrameRate,
		DacInfo** dacs,
		uint32 dacCount) const;
	status_t ConstrainFormat(
		uint32* ioFormat,
		int16* ioValidBits,
		DacInfo** dacs,
		uint32 dacCount) const;

	// figure if and how channels must be split to play the given format.
	// dacs must have room for _dacCount DACInfo pointers;
	// splits must have room for _dacCount split_channel_entry instances.
	// +em: +++++ not likely to get implemented any time soon +++++
	status_t BuildSplitChannelMap(
		const media_multi_audio_format& format,
		DacInfo** dacs,
		uint32* outDacCount,
		split_channel_entry* splits =0,
		uint32* outSplitCount =0);
	
	// configure the given DAC for the given format; only affect qualities
	// not configurable at the stream level.
	status_t SetDacFormat(
		DacInfo* dac,
		const media_multi_audio_format& format);

	// take a snapshot of the status of each DAC, and figure my preferred format.
	void RefreshDacInfo();
	
	// recalculate hardware latency & send change notice if necessary.
	void RefreshLatency();
	
	// figure the best buffer size for this CPU.
	status_t InitFrameCount();
	
	// describe the connection point, or return an error if no
	// streams are available.
	status_t DescribeFreeInput(
		media_input* outInput);
	// describe the given endpoint.
	void DescribeInput(
		Endpoint* endpoint,
		media_input* outInput);

	// stream control.
	status_t CreateStream(
		DacInfo* dac,
		const media_multi_audio_format& format,
		const game_format_spec& streamSpec,
		int32 bufferCount,
		Stream** outStream);
	void DeleteStream(
		Stream* stream);
	void StopAllStreams();
	void FlushStreamEvents(
		Stream* stream);
	void FlushAllStreamEvents();
	
	// endpoint operations.
	status_t FindEndpoint(
		int32 destinationID,
		Endpoint** outEndpoint) const;
	void CreateEndpoint(
		Stream* forStream,
		const media_source& source,
		Endpoint** outEndpoint);
	status_t DeleteEndpoint(
		Endpoint* endpoint);
		
	void ReapEndpoints();
	
	void ClearEndpointsAndStreams();
	
	// parameter web initialization.
	status_t BuildParameterWeb();	
};

}; // BPrivate
#endif // __GAME_CONSUMER_H__
