#if !defined(__GAME_PRODUCER_H__)
#define __GAME_PRODUCER_H__

#include <MediaEventLooper.h>
#include <BufferProducer.h>
#include <Controllable.h>

#include <Locker.h>
#include <String.h>

#include "GameADC.h"
#include "game_audio.h"

namespace BPrivate {

class GameAddOn;
class GameParameterMap;

class GameProducer :
	public BMediaEventLooper,
	public BBufferProducer,
	public BControllable
{
public:
	virtual ~GameProducer();
	GameProducer(
		const char* name,
		GameAddOn* addon,
		const char* devicePath,
		int32 adcID,
		int32 internalID);
	
	status_t InitCheck() const;
	int DeviceFD() const;
	BLocker* Locker() const;

	// expose BBufferProducer::SendBuffer
	status_t SendBuffer(BBuffer* buffer, const media_destination& destination)
		{ return BBufferProducer::SendBuffer(buffer, destination); }
	
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
	
protected: // BBufferProducer

	virtual void Connect(
		status_t status,
		const media_source& source,
		const media_destination& destination,
		const media_format& format,
		char* ioName);
	
	virtual void Disconnect(
		const media_source& source,
		const media_destination& destination);
	
	virtual status_t DisposeOutputCookie(
		int32 cookie);
	
	virtual void EnableOutput(
		const media_source& source,
		bool enabled,
		int32* _deprecated_);
		
	virtual status_t FormatChangeRequested(
		const media_source& source,
		const media_destination& destination,
		media_format* ioFormat,
		int32* _deprecated_);
		
	virtual status_t FormatProposal(
		const media_source& source,
		media_format* ioFormat);
		
	virtual status_t FormatSuggestionRequested(
		media_type type,
		int32 quality,
		media_format* outFormat);
		
	virtual status_t GetLatency(
		bigtime_t* outLatency);
		
	virtual status_t GetNextOutput(
		int32* ioCookie,
		media_output* outOutput);
	
	virtual void LateNoticeReceived(
		const media_source& source,
		bigtime_t howLate,
		bigtime_t tpWhen);
	
	virtual status_t PrepareToConnect(
		const media_source& source,
		const media_destination& destination,
		media_format* ioFormat,
		media_source* outSource,
		char* outName);
		
	virtual status_t SetBufferGroup(
		const media_source& source,
		BBufferGroup* group);
	
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
	class OutputIterator;
	friend class OutputIterator;

	status_t				_initStatus;
	GameAddOn*				_addon;
	BString					_path;
	int						_fd;
	int32					_adcID;
	int32					_addonID;

	BLocker					_adcLock;
	int32					_adcCount;
	GameADC**				_adc;
	
	GameParameterMap*		_parameterMap;

	// requires _adcLock
	status_t FindEndpoint(
		int32 sourceID,
		int32* outADCIndex,
		GameADC::EndpointIterator* outIterator);
	status_t FindFreeEndpointADC(
		int32 sourceID,
		int32* outADCIndex);
		
	void HandleStart(
		const media_timed_event *event);
	void HandleStop(
		const media_timed_event *event);

	// parameter web initialization
	status_t BuildParameterWeb();	
};

}; // BPrivate
#endif //__GAME_PRODUCER_H__
