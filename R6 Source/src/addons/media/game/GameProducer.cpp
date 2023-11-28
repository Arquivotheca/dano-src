#include "GameProducer.h"
#include "GameADC.h"
#include "GameAddOn.h"
#include "GameParameterMap.h"

#include "game_tools.h"

#include <Autolock.h>
#include <ParameterWeb.h>
#include <RealtimeAlloc.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace BPrivate;

#define D_TROUBLE(x)	PRINT(x)
#define D_EVENT(x)		PRINT(x)
#define D_METHOD(x)		PRINT(x)
#define D_WARNING(x)	PRINT(x)
#define D_FORMAT(x)		PRINT(x)
#define D_SETTINGS(x)	PRINT(x)

//#pragma mark --- format negotiation helpers ---
// ------------------------------------------------------------------------ //

static int rank_audio_format(
	uint32 format, int16 valid_bits)
{
	switch (format)
	{
		case media_multi_audio_format::B_AUDIO_CHAR:
			return 0;
		case media_multi_audio_format::B_AUDIO_UCHAR:
			return 1;
		case media_multi_audio_format::B_AUDIO_SHORT:
			return 2;
		case media_multi_audio_format::B_AUDIO_INT:
			return 10 + ((valid_bits > 0) ? valid_bits : 32);
		case media_multi_audio_format::B_AUDIO_FLOAT:
			return 64;
		default:
			return 0;
	}
}

static int compare_suggested_formats(
	const media_multi_audio_format& a,
	const media_multi_audio_format& b)
{
	// rank formats by:
	// (1) frame rate
	// (2) (channel count > 1)
	// (3) format: (int8, uint8, short, int18-int32, float)

	if (a.frame_rate > b.frame_rate)
		return 1;
	else
	if (b.frame_rate > a.frame_rate)
		return -1;
	
	if (a.channel_count >= 2 && b.channel_count < 2)
		return 1;
	else
	if (b.channel_count >= 2 && a.channel_count < 2)
		return -1;
	
	int a_rank = rank_audio_format(a.format, a.valid_bits);
	int b_rank = rank_audio_format(b.format, b.valid_bits);
	if (a_rank > b_rank)
		return 1;
	else
	if (b_rank > a_rank)
		return -1;

	return 0;
}

//#pragma mark --- GameProducer::OutputIterator ---
// ------------------------------------------------------------------------ //

class GameProducer::OutputIterator
{
public:
	OutputIterator(GameProducer& in_producer) :
		producer(in_producer),
		adcIndex(0),
		adcCookie(0) {}

	void* operator new(size_t s) { return rtm_alloc(0, s); }
	void operator delete(void* p, size_t s) { rtm_free(p); }

	GameProducer&	producer;	
	int32			adcIndex;
	int32			adcCookie;
};

//#pragma mark --- GameProducer ---
// ------------------------------------------------------------------------ //


GameProducer::~GameProducer()
{
	_addon->StopWatchingSettings(Node(), _path.String());

	// halt node thread
	Quit();
	
	// clean up
	BAutolock _l(_adcLock);
	for (int n = 0; n < _adcCount; n++)
	{
		delete _adc[n];
	}
	rtm_free(_adc);
	if (_fd >= 0)
	{
		close(_fd);
	}
	delete _parameterMap;
}


GameProducer::GameProducer(const char* name, GameAddOn *addon, const char *devicePath, int32 adcID, int32 codecID) :

	// base classes
	BMediaNode(name),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BControllable(),
	BMediaEventLooper(),
	
	// state
	_initStatus(B_ERROR),
	_addon(addon),
	_path(devicePath),
	_fd(-1),
	_adcID(adcID),
	_addonID(codecID),
	_adcLock("BGameProducer Codec Set"),
	_adcCount(0),
	_adc(0),
	_parameterMap(0)
{
	D_METHOD(("GameProducer::GameProducer()\n"));
	AddNodeKind(B_PHYSICAL_INPUT);
	_initStatus = B_OK;
}

status_t 
GameProducer::InitCheck() const
{
	return _initStatus;
}

int 
GameProducer::DeviceFD() const
{
	return _fd;
}

BLocker *
GameProducer::Locker() const
{
	return const_cast<BLocker*>(&_adcLock);
}


//#pragma mark --- BMediaNode ---
// ------------------------------------------------------------------------ //

status_t 
GameProducer::HandleMessage(int32 code, const void *data, size_t size)
{
	D_METHOD(("GameProducer::HandleMessage()\n"));
	switch (code)
	{
		case M_GAMENODE_PARAM_CHANGED:
		{
			gamenode_param_changed_msg& m = *(gamenode_param_changed_msg*)data;
			D_SETTINGS(("GameProducer: M_GAMENODE_PARAM_CHANGED: %x, %x\n",
				m.mixer_id, m.control_id));
			// +++
			// implement to support unbound mixers (for which a single parameter
			// may apply to several codecs.)
			return B_OK;
		}
	}
	return B_ERROR;
}

BMediaAddOn*
GameProducer::AddOn(int32 *outID) const
{
	D_METHOD(("GameProducer::AddOn()\n"));
	*outID = _addonID;
	return _addon;
}

void 
GameProducer::SetRunMode(run_mode mode)
{
	D_METHOD(("GameProducer::SetRunMode()\n"));
	// disallow invalid modes
	if (mode != B_RECORDING)
	{
		D_WARNING(("GameProducer::SetRunMode(): bad run mode %ld\n", mode));
		ReportError(B_NODE_FAILED_SET_RUN_MODE);
		return;
	}
		
	// hand off
	BMediaEventLooper::SetRunMode(mode);
}

void 
GameProducer::NodeRegistered()
{
	D_METHOD(("GameProducer::NodeRegistered()\n"));
	status_t err;
	BAutolock _l(_adcLock);
	
	// open device
	_fd = open(_path.String(), O_RDWR);
	if (_fd < 0)
	{
		D_TROUBLE((
			"GameProducer::NodeRegistered():\n\t"
			"open('%s'): %s\n",
			_path.String(),
			strerror(_fd)));
		return;
	}

	// build single codec description
	_adcCount = 1;
	_adc = (GameADC**)rtm_alloc(0, sizeof(GameADC*));
	_adc[0] = new GameADC(this, _adcID);
	
	// +++++ load configuration
	
	// set up parameter web
	err = BuildParameterWeb();
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameProducer::NodeRegistered():\n\t"
			"BuildParameterWeb(): %s\n",
			strerror(err)));
		return;
	}
	
	// +++++ set latency & priority
	
	Run();
}


//#pragma mark --- BMediaEventLooper ---
// ------------------------------------------------------------------------ //

void 
GameProducer::HandleEvent(
	const media_timed_event *event, bigtime_t howLate, bool realTimeEvent)
{
	D_METHOD(("GameProducer::HandleEvent()\n"));
	switch (event->type)
	{
		case BTimedEventQueue::B_START:
		{
			D_EVENT(("GameProducer: B_START\n"));
			BAutolock _l(_adcLock);
			HandleStart(event);
			break;
		}
		case BTimedEventQueue::B_STOP:
		{
			D_EVENT(("GameProducer: B_STOP\n"));
			BAutolock _l(_adcLock);
			HandleStop(event);
			break;
		}
		default:
			D_EVENT(("GameProducer: 0x%x\n", event->type));
			break;
	}
}

//#pragma mark --- BBufferProducer ---
// ------------------------------------------------------------------------ //

void 
GameProducer::Connect(
	status_t status,
	const media_source &source,
	const media_destination &destination,
	const media_format &format,
	char *ioName)
{
#if DEBUG
	char fmt_buf[256];
	string_for_format(format, fmt_buf, 255);
#endif
	D_METHOD(("GameProducer::Connect()\n"));
	D_FORMAT((
		"Connect():\n\t"
		"format:     %s\n", fmt_buf));

	status_t err;
	// find reserved endpoint in appropriate ADC
	BAutolock _l(_adcLock);
	int32 adcIndex;
	GameADC::EndpointIterator it;
	err = FindEndpoint(source.id, &adcIndex, &it);
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameProducer::Connect():\n\t"
			"endpoint for source 0x%x not found!\n",
			source.id));
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}
	GameADC* adc = _adc[adcIndex];
	// cope with given connection status
	if (status < B_OK)
	{
		err = adc->CancelEndpoint(it);
		if (err < B_OK)
		{
			D_TROUBLE((
				"GameProducer::Connect():\n\t"
				"GameADC::CancelEndpoint(): %s\n",
				strerror(err)));
		}
	}
	else
	{
		err = adc->ConnectEndpoint(it, destination, format.u.raw_audio);
		if (err < B_OK)
		{
			D_TROUBLE((
				"GameProducer::Connect():\n\t"
				"GameADC::ConnectEndpoint(): %s\n",
				strerror(err)));
		}
	}
}

void 
GameProducer::Disconnect(const media_source &source, const media_destination &destination)
{
	D_METHOD(("GameProducer::Disconnect()\n"));
	status_t err;
	BAutolock _l(_adcLock);
	// find the endpoint
	int32 adcIndex;
	GameADC::EndpointIterator it;
	err = FindEndpoint(source.id, &adcIndex, &it);
	if (err < B_OK)
	{
		D_WARNING((
			"GameProducer::Disconnect():\n\t"
			"endpoint for source 0x%x not found: %s\n",
			source.id, strerror(err)));
		return;
	}
	ASSERT(adcIndex >= 0);
	ASSERT(adcIndex < _adcCount);
	ASSERT(_adc[adcIndex]);
	err = _adc[adcIndex]->Disconnect(it);
	if (err < B_OK)
	{
		D_WARNING((
			"GameProducer::Disconnect():\n\t"
			"GameADC::Disconnect(): %s\n",
			strerror(err)));
	}
}

status_t 
GameProducer::DisposeOutputCookie(int32 cookie)
{
	D_METHOD(("GameProducer::DisposeOutputCookie()\n"));
	if (!cookie) return B_ERROR;
	BAutolock _l(_adcLock);

	OutputIterator* it = (OutputIterator*)cookie;
	if (it->adcIndex < _adcCount && it->adcCookie)
	{
		_adc[it->adcIndex]->DisposeOutputCookie(it->adcCookie);
	}
	delete it;
	return B_OK;
}

void 
GameProducer::EnableOutput(const media_source &source, bool enabled, int32 *_deprecated_)
{
	D_METHOD(("GameProducer::EnableOutput()\n"));
	status_t err;
	BAutolock _l(_adcLock);

	int32 adcIndex;
	GameADC::EndpointIterator it;
	err = FindEndpoint(source.id, &adcIndex, &it);
	if (err < B_OK)
	{
		D_WARNING((
			"GameProducer::EnableOutput():\n\t"
			"endpoint for source %ld not found: %s\n",
			source.id, strerror(err)));
		return;
	}
	ASSERT(adcIndex >= 0);
	ASSERT(adcIndex < _adcCount);
	_adc[adcIndex]->Enable(it, enabled);
}

status_t 
GameProducer::FormatChangeRequested(const media_source &source, const media_destination &destination, media_format *ioFormat, int32 *_deprecated_)
{
	D_METHOD(("GameProducer::FormatChangeRequested(not bloody likely)\n"));
	return B_NOT_ALLOWED;
}

status_t 
GameProducer::FormatProposal(const media_source &source, media_format *ioFormat)
{
#if DEBUG
	char fmt_buf[256];
	string_for_format(*ioFormat, fmt_buf, 255);
#endif
	D_METHOD(("GameProducer::FormatProposal()\n"));
	D_FORMAT(("\tin:  %s\n", fmt_buf));
	status_t err;
	if (ioFormat->type != B_MEDIA_RAW_AUDIO)
	{
		return B_MEDIA_BAD_FORMAT;
	}
	if (!GAME_IS_ADC(source.id))
	{
		D_TROUBLE((
			"GameProducer::FormatProposal():\n\t"
			"expected an ADC ID; got 0x%x\n",
			source.id));
		return B_MEDIA_BAD_SOURCE;
	}
	// find the appropriate ADC
	BAutolock _l(_adcLock);
	int32 adcIndex;
	err = FindFreeEndpointADC(source.id, &adcIndex);
	if (err < B_OK)
	{
		D_WARNING((
			"GameProducer::FormatProposal():\n\t"
			"endpoint for source %ld not found: %s\n",
			source.id, strerror(err)));
		return B_BAD_VALUE;
	}
	// validate format
	ASSERT(adcIndex >= 0);
	ASSERT(adcIndex < _adcCount);
	int32 streamID = GAME_NO_ID;
	err = _adc[adcIndex]->ValidateFormat(&ioFormat->u.raw_audio, &streamID);
	D_FORMAT(("\tout: %s\n", fmt_buf));
	// return prognosis
	return err;
}

status_t 
GameProducer::FormatSuggestionRequested(media_type type, int32 quality, media_format *outFormat)
{
	D_METHOD(("GameProducer::FormatSuggestionRequested()\n"));
	if (type != B_MEDIA_RAW_AUDIO)
	{
		return B_MEDIA_BAD_FORMAT;
	}
	outFormat->type = B_MEDIA_RAW_AUDIO;
	outFormat->u.raw_audio = media_multi_audio_format::wildcard;
	
	// find 'best' format of all ADCs	
	BAutolock _l(_adcLock);
	for (int n = 0; n < _adcCount; n++)
	{
		ASSERT(_adc[n]);
		media_multi_audio_format current;
		_adc[n]->GetRequiredFormat(&current);
		if (compare_suggested_formats(current, outFormat->u.raw_audio) > 0)
		{
			outFormat->u.raw_audio = current;
		}
	}
#if DEBUG
	char fmt_buf[256];
	string_for_format(*outFormat, fmt_buf, 255);
#endif
	D_FORMAT(("\tout: %s\n", fmt_buf));
	return B_OK;
}

status_t 
GameProducer::GetLatency(bigtime_t *outLatency)
{
	D_METHOD(("GameProducer::GetLatency()\n"));
	status_t err;
	bigtime_t internal = 0LL;
	// critical section; lock must be released before calling base GetLatency(),
	// since that performs output enumeration and would deadlock.
	{
		BAutolock _l(_adcLock);
		for (int32 n = 0; n < _adcCount; n++)
		{
			bigtime_t current;
			err = _adc[n]->GetInternalLatency(&current);
			if (err < B_OK)
			{
				D_WARNING((
					"GameProducer::GetLatency():\n\t"
					"GameADC::GetInternalLatency(): %s\n",
					strerror(err)));
				continue;
			}
			if (current > internal)
			{
				internal = current;
			}
		}
	}
	// end critical section
	
	bigtime_t downstream = 0LL;
	err = BBufferProducer::GetLatency(&downstream);
	if (err < B_OK)
	{
		D_WARNING((
			"GameProducer::GetLatency():\n\t"
			"BBufferProducer::GetLatency(): %s\n",
			strerror(err)));
	}
	*outLatency = internal + downstream;
	return B_OK;
}

status_t 
GameProducer::GetNextOutput(int32 *ioCookie, media_output *outOutput)
{
	D_METHOD(("GameProducer::GetNextOutput()\n"));
	BAutolock _l(_adcLock);
	OutputIterator* it = (OutputIterator*)*ioCookie;
	status_t err;
	if (!it)
	{
		// initialize iterator
		it = new OutputIterator(*this);
		*ioCookie = (int32)it;
	}
	// search for an output
	while (it->adcIndex < _adcCount)
	{
		GameADC* adc = _adc[it->adcIndex];
		// fetch next output for this codec, if any
		media_multi_audio_format format;
		media_source source;
		media_destination destination;
		err = adc->GetNextOutput(&it->adcCookie, &format, &source, &destination);
		if (err == B_OK)
		{
			outOutput->format.type = B_MEDIA_RAW_AUDIO;
			outOutput->format.u.raw_audio = format;
			outOutput->source = source;
			outOutput->destination = destination;
			outOutput->node = Node();
			return B_OK;
		}
		// clean up and move on
		if (it->adcCookie)
		{
			adc->DisposeOutputCookie(it->adcCookie);
			it->adcCookie = 0;
		}
		++it->adcIndex;
	}
	return B_ERROR;
}

void 
GameProducer::LateNoticeReceived(const media_source &source, bigtime_t howLate, bigtime_t tpWhen)
{
	D_METHOD(("GameProducer::LateNoticeReceived()\n"));
	// whatever.
}

status_t 
GameProducer::PrepareToConnect(
	const media_source &source,
	const media_destination &destination,
	media_format *ioFormat,
	media_source *outSource,
	char *outName)
{
#if DEBUG
	char fmt_buf[256];
	string_for_format(*ioFormat, fmt_buf, 255);
#endif
	D_METHOD(("GameProducer::PrepareToConnect()\n"));
	D_FORMAT((
		"\tPrepareToConnect()\n\t"
		"in format:  %s\n", fmt_buf));

	status_t err;
	// find requested ADC
	BAutolock _l(_adcLock);
	int32 adcIndex;
	err = FindFreeEndpointADC(source.id, &adcIndex);
	if (err < B_OK)
	{
		D_WARNING((
			"GameProducer::PrepareToConnect():\n\t"
			"ADC for source 0x%x not found: %s\n",
			source.id, strerror(err)));
		return err;
	}
	GameADC* adc = _adc[adcIndex];
	// create endpoint
	int32 sourceID;
	err = adc->ReserveEndpoint(
		&ioFormat->u.raw_audio,
		&sourceID);
#if DEBUG
	string_for_format(*ioFormat, fmt_buf, 255);
#endif
	if (err < B_OK)
	{
		D_WARNING((
			"GameProducer::PrepareToConnect():\n\t"
			"GameADC::ReserveEndpoint(): %s\n\t"
			"out format: %s\n",
			strerror(err), fmt_buf));
		return err;
	}
	// build source
	outSource->port = Node().port;
	outSource->id = sourceID;
	// build name
	// +++++ number!
	snprintf(outName, B_MEDIA_NAME_LENGTH, "%s", adc->Name());
	D_FORMAT((
		"PrepareToConnect()\n\t"
		"out format: %s\n", fmt_buf));
	return B_OK;
}

status_t 
GameProducer::SetBufferGroup(const media_source &source, BBufferGroup *group)
{
	D_METHOD(("GameProducer::SetBufferGroup()\n"));
	status_t err;
	BAutolock _l(_adcLock);

	int32 adcIndex;
	GameADC::EndpointIterator it;
	err = FindEndpoint(source.id, &adcIndex, &it);
	if (err < B_OK)
	{
		D_WARNING((
			"GameProducer::SetBufferGroup():\n\t"
			"endpoint for source %ld not found: %s\n",
			source.id, strerror(err)));
		return B_MEDIA_BAD_SOURCE;
	}
	ASSERT(adcIndex >= 0);
	ASSERT(adcIndex < _adcCount);
	return _adc[adcIndex]->SetBufferGroup(it, group);	
}

//#pragma mark --- BControllable ---
// ------------------------------------------------------------------------ //

status_t 
GameProducer::GetParameterValue(int32 id, bigtime_t *outLastChangeTime, void *outValue, size_t *ioSize)
{
	status_t err = _parameterMap ?
		_parameterMap->GetParameterValue(_fd, id, outValue, ioSize) :
		EPERM;
	if (err == B_OK) *outLastChangeTime = 0LL;
	else
	{
		D_TROUBLE((
			"GameProducer::GetParameterValue():\n\t"
			"GetParameterValue(%x): %s\n",
			id, strerror(err)));
	}
	return err;
}

void 
GameProducer::SetParameterValue(int32 id, bigtime_t changeTime, const void *value, size_t size)
{
	status_t err = _parameterMap ?
		_parameterMap->SetParameterValue(_fd, id, value, size) :
		EPERM;
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameProducer::SetParameterValue():\n\t"
			"SetParameterValue(%x): %s\n",
			id, strerror(err)));
	}	
	else
	{
		// notify the addon of the change
		const game_mixer_control * ci = _parameterMap->ControlAt(id & ~GameParameterMap::PARAM_ID_MASK);
		if (!ci)
		{
			D_TROUBLE((
				"GameProducer::SetParameterValue():\n\t"
				"_parameterMap->ControlAt() couldn't find %ld\n", id));
			return;
		}
		err = _addon->CodecParameterChanged(Node(), _path.String(), ci->mixer_id, ci->control_id);
		if (err < B_OK)
		{
			D_TROUBLE((
				"GameProducer::SetParameterValue():\n\t"
				"_addon->CodecParameterChanged(): %s\n", strerror(err)));
		}
	}
}

//#pragma mark --- internal ---
// ------------------------------------------------------------------------ //

status_t 
GameProducer::FindEndpoint(
	int32 sourceID, int32 *outADCIndex, GameADC::EndpointIterator *outIterator)
{
	ASSERT(_adcLock.IsLocked());
	for (int32 n = 0; n < _adcCount; n++)
	{
		ASSERT(_adc[n]);
		if (_adc[n]->FindEndpoint(sourceID, outIterator) == B_OK)
		{
			*outADCIndex = n;
			return B_OK;
		}
	}
	return B_BAD_VALUE;
}

status_t 
GameProducer::FindFreeEndpointADC(int32 sourceID, int32 *outADCIndex)
{
	ASSERT(_adcLock.IsLocked());
	for (int32 n = 0; n < _adcCount; n++)
	{
		ASSERT(_adc[n]);
		if (_adc[n]->ID() == sourceID)
		{
			*outADCIndex = n;
			return B_OK;
		}
	}
	return B_BAD_VALUE;
}

void 
GameProducer::HandleStart(const media_timed_event *event)
{
	D_METHOD(("GameProducer::HandleStart()\n"));
	ASSERT(_adcLock.IsLocked());
	status_t err;
	for (int32 n = 0; n < _adcCount; n++)
	{
		if (!_adc[n]->IsRunning())
		{
			err = _adc[n]->Start();
			if (err < B_OK)
			{
				D_TROUBLE((
					"GameProducer::HandleStart():\n\t"
					"GameADC[%ld]::Start(): %s\n",
					n, strerror(err)));
				continue;
			}
		}
	}
}

void 
GameProducer::HandleStop(const media_timed_event *event)
{
	D_METHOD(("GameProducer::HandleStop()\n"));
	ASSERT(_adcLock.IsLocked());
	status_t err;
	for (int32 n = 0; n < _adcCount; n++)
	{
		if (_adc[n]->IsRunning())
		{
			err = _adc[n]->Stop();
			if (err < B_OK)
			{
				D_TROUBLE((
					"GameProducer::HandleStop():\n\t"
					"GameADC[%ld]::Stop(): %s\n",
					n, strerror(err)));
				continue;
			}
		}
	}
}

status_t 
GameProducer::BuildParameterWeb()
{
	D_METHOD(("GameProducer::BuildParameterWeb()\n"));
	if (_parameterMap) return EPERM;
	status_t err;
	
	G<game_get_info> ggi;
	if (ioctl(_fd, GAME_GET_INFO, &ggi) < 0) return errno;
	
	game_mixer_info* mixerInfo = (game_mixer_info*)alloca(sizeof(game_mixer_info) * ggi.mixer_count);
	if (!mixerInfo) return B_NO_MEMORY;
	memset(mixerInfo, 0, sizeof(game_mixer_info) * ggi.mixer_count);
	for (int n = 0; n < ggi.mixer_count; n++) mixerInfo[n].mixer_id = GAME_MAKE_MIXER_ID(n);

	G<game_get_mixer_infos> ggmi;
	ggmi.info = mixerInfo;
	ggmi.in_request_count = ggi.mixer_count;
	if (ioctl(_fd, GAME_GET_MIXER_INFOS, &ggmi) < 0) return errno;

	// build parameter mapping for mixers we care about
	_parameterMap = new GameParameterMap;

	for (int n = 0; n < ggmi.out_actual_count; n++)
	{
		int32 codec_id = mixerInfo[n].linked_codec_id;
		if (codec_id != GAME_NO_ID && !GAME_IS_ADC(codec_id)) continue;
		err = _parameterMap->AppendMixerControls(_fd, mixerInfo[n]);
		if (err < B_OK)
		{
			D_TROUBLE(("GameProducer::BuildParameterWeb():\n\t"
				"AppendMixerControls('%s'): %s\n", mixerInfo[n].name, strerror(err)));
		}
		err = _addon->StartWatchingSettings(Node(), _path.String(), mixerInfo[n].mixer_id);
		PRINT(("+++ GameProducer::BuildParameterWeb(): watching mixer %x\n",
			mixerInfo[n].mixer_id));
		if (err < B_OK)
		{
			D_TROUBLE(("GameProducer::BuildParameterWeb():\n\t"
				"_addon->StartWatchingSettings('%s'): %s\n", mixerInfo[n].name, strerror(err)));
		}
	}

	// generate the web
	BParameterWeb* web = 0;
	err = _parameterMap->MakeParameterWeb(_fd, &web,
		GameParameterMap::INCLUDE_ADC_MIXERS |
		GameParameterMap::INCLUDE_UNBOUND_MIXERS |
		GameParameterMap::ENABLE_ADVANCED_CONTROLS);
	if (err < B_OK)
	{
		D_TROUBLE(("GameProducer::BuildParameterWeb():\n\t"
			"MakeParameterWeb(): %s\n", strerror(err)));
		delete _parameterMap; _parameterMap = 0;
		return err;
	}

	SetParameterWeb(web);
	return B_OK;

}

