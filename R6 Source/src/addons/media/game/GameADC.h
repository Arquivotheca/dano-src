#if !defined(__GAME_ADC_H__)
#define __GAME_ADC_H__

#include <MediaDefs.h>
#include <MediaNode.h>
#include <OS.h>

#include "game_audio.h"
#include "game_tools.h"

class BLocker;
class BBufferGroup;

namespace BPrivate {

class GameProducer;

// locking note: the shared lock must be acquired before calling any public GameADC methods.
class GameADC
{
public:
	class EndpointIterator;
private:

public:
	GameADC(
		GameProducer* node,
		int32 adc_id);
	~GameADC();

	void* operator new(size_t s);
	void operator delete(void* p, size_t s);
	
	int32 ID() const;
	int DeviceFD() const;
	const char* Name() const;
	BLocker* Locker() const;
	GameProducer* Node() const;
	
	// thread control
	status_t Start();
	bool IsRunning();
	status_t Stop(bool wait =false);
	
	// endpoint iteration
	status_t GetNextOutput(
		int32* ioCookie,
		media_multi_audio_format* outFormat,
		media_source* outSource,
		media_destination* outDestination);
	status_t DisposeOutputCookie(
		int32 cookie);

	// format validation/specialization:
	// - specialize wildcards
	// - correct invalid fields (& return B_MEDIA_BAD_FORMAT)
	// - return a stream ID if the format is valid for only one stream, or
	//   GAME_NO_ID otherwise.
	status_t ValidateFormat(
		media_multi_audio_format* ioFormat,
		int32* outStreamID) const;
	void GetRequiredFormat(
		media_multi_audio_format* oFormat) const;
	void GetPreferredFormat(
		const media_multi_audio_format& required,
		media_multi_audio_format* oPreferred) const;
	status_t FindNearestStreamFormat(
		media_multi_audio_format* ioFormat,
		int32* outStreamID) const;	

	// connection management

	status_t FindEndpoint(
		int32 sourceID,
		EndpointIterator* outEndpoint =0) const;

	status_t ReserveEndpoint(
		media_multi_audio_format* ioFormat,
		int32* outSourceID);
	status_t ConnectEndpoint(
		const EndpointIterator& endpoint,
		const media_destination& destination,
		const media_multi_audio_format& format);
	// invalidates 'endpoint'
	status_t CancelEndpoint(
		const EndpointIterator& endpoint);
	// invalidates 'endpoint'
	status_t Disconnect(
		const EndpointIterator& endpoint);
	status_t Enable(
		const EndpointIterator& endpoint,
		bool enabled);
	status_t SetBufferGroup(
		const EndpointIterator& endpoint,
		BBufferGroup* group);
		
	status_t GetInternalLatency(
		bigtime_t* outLatency);

private:
	class Stream;
	class Endpoint;

	const int					_fd;
	GameProducer*				_node;
	game_codec_info				_info;
	BLocker* const				_lock;

	// current format of the ADC (different formats may be supported
	// on an individual-stream basis)
	media_multi_audio_format	_format;

	// new-connection endpoint; when a connection is formed a
	// corresponding Endpoint is added to the appropriate stream.
	Endpoint*					_freeEndpoint;
	// stream list
	Stream*						_streams;

	uint16						_nextEndpointOrdinal;	
	bool						_running;
	
	// count of currently active output iterators (must maintain
	// stream/endpoint set consistency during iteration -- adding
	// nodes is okay, but deleting isn't.)
	int32						_iteratorCount;

	// initialize the new-connection endpoint
	void InitFreeEndpoint(
		int32 adcID);

	// set the ADC format.  if streamPrecedence is true, only change
	// each codec format field if that value cannot be set at the
	// stream level.
	status_t SetCodecFormat(
		const media_multi_audio_format& format,
		bool streamPrecedence =false);

	// refresh general ADC info (fetch _info and extract format)
	status_t RefreshCodecInfo();

	// terminate all streams and clean up
	void ClearStreams();

	// delete an endpoint (or schedule for deletion if iterators are active)		
	status_t RemoveEndpoint(
		const EndpointIterator& endpoint);

	// delete streams and/or endpoints that were removed during iteration
	void Reap();

	int32 MakeEndpointID();

public:
	class EndpointIterator
	{
	public:
		EndpointIterator();
		EndpointIterator(const EndpointIterator& clone);
		EndpointIterator& operator=(const EndpointIterator& clone);
		
		void* operator new(size_t s);
		void operator delete(void* p, size_t s);
		
		bool IsValid() const;
		bool IsValidEndpoint() const;
		
	private:
		friend class GameADC;
		EndpointIterator(GameADC::Stream* s);
		EndpointIterator(GameADC::Stream* s, GameADC::Endpoint* e);
		
		void Advance();

		GameADC::Stream*	stream;
		GameADC::Endpoint*	endpoint;
	};
};

}; //BPrivate
#endif //__GAME_ADC_H__
