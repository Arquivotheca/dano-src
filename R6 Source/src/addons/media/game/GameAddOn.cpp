
#include "GameAddOn.h"
#include "GameConsumer.h"
#include "GameProducer.h"

#include "game_audio.h"

#include <Debug.h>
#include <Autolock.h>
#include <List.h>
#include <MediaRoster.h>
#include <Path.h>
#include <String.h>
#include <FindDirectory.h>

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace BPrivate;

#define D_TROUBLE(x)	PRINT(x)
#define D_METHOD(x)		PRINT(x)
#define D_WARNING(x)	PRINT(x)
#define D_SETTINGS(x)	PRINT(x)

//#pragma mark --- constants ---
// ------------------------------------------------------------------------ //

static const char* const game_dev_root = "/dev/audio/game";

const uint32 DEF_FLAVA_FLAGS		= B_FLAVOR_IS_GLOBAL;

const int32 SETTINGS_FILE_MAGIC		= 'g0na';
const int32 SETTINGS_FILE_VERSION	= 1;

//#pragma mark --- add-on hook ---
// ------------------------------------------------------------------------ //

extern "C" _EXPORT BMediaAddOn* make_media_addon(image_id image)
{
	status_t err = media_realtime_init_image(image, B_MEDIA_REALTIME_AUDIO);
	if (err < B_OK)
	{
		D_WARNING((
			"make_media_addon():\n\t"
			"media_realtime_init_image(): %s\n", strerror(err)));
	}
	return new BPrivate::GameAddOn(image);
}

//#pragma mark --- codec_info ---
// ------------------------------------------------------------------------ //

struct GameAddOn::codec_info
{
	char				path[B_PATH_NAME_LENGTH];
	game_codec_info		info;
	flavor_info			flavor;
};

//#pragma mark --- mixer_state ---
// ------------------------------------------------------------------------ //

class GameAddOn::mixer_state
{
public:
	mixer_state(int32 id) : _mixerID(id) {}
	
	int32 MixerID() const { return _mixerID; }
	
	status_t Import(int fd);
	ssize_t FlattenedSize() const;
	ssize_t Flatten(void * buffer, ssize_t size) const;

	ssize_t Unflatten(const void * buffer, ssize_t size);
	status_t Export(int fd);

private:
	friend class GameAddOn::device_state;
	const int32							_mixerID;
	std::vector<game_mixer_control_value>	_values;
	std::vector<media_node>					_observers;
};

status_t 
GameAddOn::mixer_state::Import(int fd)
{
	status_t err = B_OK;
	_values.clear();
	G<game_get_mixer_infos> ggmis;
	H<game_mixer_info> gmi;
	ggmis.info = &gmi;
	ggmis.in_request_count = 1;
	gmi.mixer_id = _mixerID;
	if (ioctl(fd, GAME_GET_MIXER_INFOS, &ggmis) < 0) {
		err = errno;
		fprintf(stderr, "error: ioctl(GAME_GET_MIXER_INFOS): %s\n", strerror(err));
		return err;
	}
	if (!gmi.control_count) return B_OK;
	std::vector<game_mixer_control> controls;
	controls.resize(gmi.control_count);
	G<game_get_mixer_controls> ggmcs;
	ggmcs.control = &controls[0];
	ggmcs.in_request_count = gmi.control_count;
	ggmcs.mixer_id = _mixerID;
	if (ioctl(fd, GAME_GET_MIXER_CONTROLS, &ggmcs) < 0) {
		err = errno;
		fprintf(stderr, "error: ioctl(GAME_GET_MIXER_CONTROLS): %s\n", strerror(err));
		return err;
	}
	G<game_get_mixer_control_values> ggmcvs;
	ggmcvs.in_request_count = gmi.control_count;
	_values.resize(ggmcvs.in_request_count);
	ggmcvs.values = &_values[0];
	ggmcvs.mixer_id = _mixerID;
	for (int ix=0; ix<gmi.control_count; ix++) {
		ggmcvs.values[ix].control_id = ggmcs.control[ix].control_id;
		ggmcvs.values[ix].mixer_id = _mixerID;	//	should be unnecessary
	}
	if (ioctl(fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcvs) < 0) {
		err = errno;
		fprintf(stderr, "error: ioctl(GAME_GET_MIXER_CONTROL_VALUES)\n");
	}
	return err;
}

ssize_t 
GameAddOn::mixer_state::FlattenedSize() const
{
	return sizeof(_mixerID)+sizeof(int)+sizeof(game_mixer_control_value)*_values.size();
}

ssize_t 
GameAddOn::mixer_state::Flatten(void *buffer, ssize_t size) const
{
	ssize_t t = FlattenedSize();
	if (t > size) return B_NO_MEMORY;
	char * s = (char *)buffer;
	memcpy(s, &_mixerID, sizeof(_mixerID));
	s += sizeof(_mixerID);
	int c = _values.size();
	memcpy(s, &c, sizeof(c));
	s += sizeof(c);
	memcpy(s, &_values[0], sizeof(game_mixer_control_value)*c);
	ASSERT(s <= (((char *)buffer)+size));
	return t;
}

ssize_t 
GameAddOn::mixer_state::Unflatten(const void *buffer, ssize_t size)
{
	if (size < sizeof(_mixerID)+sizeof(int))
	{
		D_TROUBLE((
			"GameAddOn::mixer_state::Unflatten(): corrupt buffer\n"));
		return B_BAD_VALUE;
	}
	int c;
	const char * pos = (const char *)buffer;
	int32 id;
	memcpy(&id, pos, sizeof(id));
	pos += sizeof(id);
	if (id != _mixerID)
	{
		D_TROUBLE((
			"GameAddOn::mixer_state::Unflatten(): expected mixer ID %ld, got %ld\n", _mixerID, id));
		return B_BAD_VALUE;
	}
	memcpy(&c, pos, sizeof(c));
	pos += sizeof(c);
	if (c > 200 || c < 0) {
		D_TROUBLE((
			"GameAddOn::mixer_state::Unflatten(): goofy control count %ld\n", c));
		return B_BAD_VALUE;
	}
	if (size < sizeof(_mixerID)+sizeof(int)+sizeof(game_mixer_control_value)*c) {
		D_TROUBLE((
			"GameAddOn::mixer_state::Unflatten(): truncated buffer\n"));
		return B_BAD_VALUE;
	}
	_values.resize(c);
	memcpy(&_values[0], pos, sizeof(game_mixer_control_value)*c);
	return sizeof(_mixerID)+sizeof(int)+sizeof(game_mixer_control_value)*c;
}

status_t 
GameAddOn::mixer_state::Export(int fd)
{
	G<game_set_mixer_control_values> gsmcvs;
	gsmcvs.mixer_id = _mixerID;
	gsmcvs.in_request_count = _values.size();
	gsmcvs.values = &_values[0];
	status_t err = B_OK;
	if (!gsmcvs.in_request_count) return B_OK;
	if (ioctl(fd, GAME_SET_MIXER_CONTROL_VALUES, &gsmcvs) < 0) {
		err = errno;
		fprintf(stderr, "error: ioctl(GAME_SET_MIXER_CONTROL_VALUES)\n");
	}
	return err;
}

//#pragma mark --- device_state ---
// ------------------------------------------------------------------------ //

class GameAddOn::device_state
{
public:
	device_state(const char * devicePath);
	~device_state();

	const char * Path() const { return _path; }
	const char * SettingsFileName() const { return _settingsFileName.String(); }
	
	void SetDirty(bool dirty) { _dirty = dirty; }
	bool IsDirty() const { return _dirty; }

	status_t AddObserver(media_node node, int32 mixerID);
	void RemoveObserver(media_node node);
	int32 CountObservers() const;
	
	void BroadcastChange(media_node fromNode, int32 mixerID, int32 controlID);

	status_t Import();
	ssize_t FlattenedSize() const;
	ssize_t Flatten(void * buffer, ssize_t size) const;

	ssize_t Unflatten(const void * buffer, ssize_t size);
	status_t Export();
	status_t Export(int32 mixerID);

private:
	char					_path[B_PATH_NAME_LENGTH];
	BString					_settingsFileName;
	bool					_dirty;
	int						_fd;
	std::vector<mixer_state *>	_mixers;
	int32					_observerCount;
};


GameAddOn::device_state::device_state(const char *devicePath) :
	_dirty(false),
	_fd(-1),
	_observerCount(0)
{
	strcpy(_path, devicePath); // +++ o so trusting
	_settingsFileName = devicePath + strlen(game_dev_root) + 1;
	_settingsFileName.ReplaceAll('/', '_');
	_settingsFileName += "_settings";
	_fd = open(_path, O_RDWR);
	if (_fd < 0)
	{
		D_TROUBLE((
			"GameAddOn::device_state::device_state(%s):\n\t"
			"open(): %s\n",
			_path, strerror(_fd)));
	}
	G<game_get_info> ggi;
	if (ioctl(_fd, GAME_GET_INFO, &ggi) < 0)
	{
		D_TROUBLE((
			"GameAddOn::device_state::device_state(%s):\n\t"
			"ioctl(GAME_GET_INFO): %s\n", strerror(errno)));
		return;
	}
	for (int n = 0; n < ggi.mixer_count; n++)
	{
		_mixers.push_back(new mixer_state(GAME_MAKE_MIXER_ID(n)));
	}
}

GameAddOn::device_state::~device_state()
{
	if (_fd >= 0) close(_fd);
	for (int n = _mixers.size(); n; n--)
	{
		delete _mixers[n-1];
	}
}

status_t 
GameAddOn::device_state::AddObserver(media_node node, int32 mixerID)
{
	mixer_state * m = 0;
	for (int n = 0; !m && n < _mixers.size(); n++)
	{
		if (_mixers[n]->MixerID() == mixerID)
		{
			m = _mixers[n];
		}
	}
	if (!m)
	{
		D_TROUBLE((
			"GameAddOn::device_state::AddObserver(): no mixer %ld\n",
			mixerID));
		return B_BAD_VALUE;
	}
	m->_observers.push_back(node);
	_observerCount++;
	return B_OK;
}

void 
GameAddOn::device_state::RemoveObserver(media_node node)
{
	for (int n = 0; n < _mixers.size(); n++)
	{
		std::vector<media_node>& v = _mixers[n]->_observers;
		for (int nn = v.size(); nn; nn--)
		{
			if (v[nn-1] == node)
			{
				v.erase(v.begin()+nn-1);
				_observerCount--;
			}
		}
	}
}

int32 
GameAddOn::device_state::CountObservers() const
{
	return _observerCount;
}

void 
GameAddOn::device_state::BroadcastChange(media_node fromNode, int32 mixerID, int32 controlID)
{
	mixer_state * m = 0;
	for (int n = 0; !m && n < _mixers.size(); n++)
	{
		if (_mixers[n]->MixerID() == mixerID) m = _mixers[n];
	}
	if (!m)
	{
		D_TROUBLE((
			"GameAddOn::device_state::BroadcastChange(): no mixer %ld\n",
			mixerID));
		return;
	}
	gamenode_param_changed_msg msg = { mixerID, controlID };
	for (int n = 0; n < m->_observers.size(); n++)
	{
		if (m->_observers[n].node == fromNode.node) continue;
		// +++ timeout?
		write_port(
			m->_observers[n].port, M_GAMENODE_PARAM_CHANGED,
			&msg, sizeof(msg));
	}
}

status_t 
GameAddOn::device_state::Import()
{
	status_t err = B_OK;
	for (int n = 0; n < _mixers.size(); n++)
	{
		err = _mixers[n]->Import(_fd);
		if (err < B_OK) return err;
	}
	return B_OK;
}

ssize_t 
GameAddOn::device_state::FlattenedSize() const
{
	ssize_t t = sizeof(int);
	for (int n = 0; n < _mixers.size(); n++)
	{
		t += _mixers[n]->FlattenedSize();
	}
	return t;	
}

ssize_t 
GameAddOn::device_state::Flatten(void *buffer, ssize_t size) const
{
	ssize_t ss = FlattenedSize();
	if (ss > size) return B_NO_MEMORY;
	char * s = (char *)buffer;
	int c = _mixers.size();
	memcpy(s, &c, sizeof(c));
	s += sizeof(c);
	status_t err;
	for (int ix=0; ix<_mixers.size(); ix++)
	{
		ss = _mixers[ix]->FlattenedSize();
		err = _mixers[ix]->Flatten(s, ss);
		if (err < 0)
		{
			D_TROUBLE((
				"GameAddOn::device_state::Flatten():\n\t"
				"mixer_state[%d]::Flatten(): %s\n", ix, strerror(err)));
			return err;
		}
		s += err;
	}
	ASSERT(s <= (((char *)buffer)+size));
	return s-(char *)buffer;
}

ssize_t 
GameAddOn::device_state::Unflatten(const void *buffer, ssize_t size)
{
	if (size < sizeof(int)) return B_BAD_VALUE;
	int c;
	const char * pos = (const char *)buffer;
	const char * e = pos+size;
	memcpy(&c, pos, sizeof(int));
	pos += sizeof(int);
	if (c != _mixers.size())
	{
		// +++ is this overly strict?
		// should we attempt to handle settings files with fewer than the current mixer count,
		// assuming that gamedriver modules will 'push' new mixer IDs after previously existing ones?
		D_TROUBLE((
			"GameAddOn::device_state::Unflatten(): mixer count %ld, expected %ld\n", c, _mixers.size()));
		return B_BAD_VALUE;
	}
	status_t err;
	for (int ix=0; ix<c; ix++) {
		err = _mixers[ix]->Unflatten(pos, e-pos);
		if (err < 0)
		{
			D_TROUBLE((
				"GameAddOn::device_state::Unflatten():\n\t"
				"mixer_state[%d]::Unflatten(): %s\n", err, strerror(err)));
			return err;
		}
		pos += err;
	}
	return pos - (const char *)buffer;
}

status_t 
GameAddOn::device_state::Export()
{
	status_t err;
	for (int n = 0; n < _mixers.size(); n++)
	{
		err = _mixers[n]->Export(_fd);
		if (err < B_OK) return err;
	}
	return B_OK;
}

status_t 
GameAddOn::device_state::Export(int32 mixerID)
{
	status_t err;
	for (int n = 0; n < _mixers.size(); n++)
	{
		if (_mixers[n]->MixerID() == mixerID)
		{
			return _mixers[n]->Export(_fd);
		}
	}
	return B_BAD_VALUE;			
}



//#pragma mark --- BMediaAddOn ---
// ------------------------------------------------------------------------ //

GameAddOn::GameAddOn(image_id image) :
	BMediaAddOn(image),
	_initStatus(B_ERROR),
	_codecs(0),
	_nextInternalID(0),
	_settingsLock("GameAddOn::_settingsLock"),
	_settingsThread(-1),
	_settingsSem(-1)
{
	D_METHOD(("GameAddOn::GameAddOn()\n"));
	_codecs = new BList;
	if (!_codecs)
	{
		_initStatus = B_NO_MEMORY;
		return;
	}

	char pathBuffer[B_PATH_NAME_LENGTH];
	strcpy(pathBuffer, game_dev_root);
	_initStatus = ScanDevices(pathBuffer);
}

GameAddOn::~GameAddOn()
{
	D_METHOD(("GameAddOn::~GameAddOn()\n"));
	ClearCodecs();
	status_t err = StopSettingsThread();
	if (err < B_OK)
	{
		D_TROUBLE(("StopSettingsThread(): %s\n", strerror(err)));
	}
	ClearDeviceState();
}

status_t 
GameAddOn::InitCheck(const char **outFailureText)
{
	D_METHOD(("GameAddOn::InitCheck()\n"));
	*outFailureText = strerror(_initStatus);
	return _initStatus;
}

int32 
GameAddOn::CountFlavors()
{
	D_METHOD(("GameAddOn::CountFlavors()\n"));
	return _codecs ? _codecs->CountItems() : 0;
}

status_t 
GameAddOn::GetFlavorAt(int32 index, const flavor_info **outInfo)
{
	D_METHOD(("GameAddOn::GetFlavorAt(%ld)\n", index));
	if (index < 0 || index >= CountFlavors()) return B_BAD_INDEX;
	ASSERT(_codecs);
	*outInfo = &((codec_info*)_codecs->ItemAt(index))->flavor;

	ASSERT(*outInfo);
	return B_OK;
}

BMediaNode *
GameAddOn::InstantiateNodeFor(const flavor_info *info, BMessage *config, status_t* outError)
{
	D_METHOD(("GameAddOn::InstantiateNodeFor(%ld: %s)\n",
		info->internal_id, info->out_format_count ? "OUTPUT" : "INPUT"));
	status_t err;
	if (!_codecs)
	{
		*outError = B_BAD_INDEX;
		return 0;
	}
	codec_info* i = 0;
	for (int n = 0; n < _codecs->CountItems(); n++)
	{
		if (((codec_info*)_codecs->ItemAt(n))->flavor.internal_id == info->internal_id)
		{
			i = (codec_info*)_codecs->ItemAt(n);
			break;
		}
	}	
	if (!i)
	{
		*outError = B_BAD_INDEX;
		return 0;
	}
	
	if (!GAME_IS_DAC(i->info.codec_id) &&
		!GAME_IS_ADC(i->info.codec_id))
	{
		D_TROUBLE((
			"GameAddOn::InstantiateNodeFor():\n\t"
			"not a codec ID: %hx\n",
			i->info.codec_id));
		*outError = B_BAD_VALUE;
		return 0;
	}

	// load settings if we haven't already (this will also start the settings thread
	// if it wasn't running)
	err = InitDeviceState(i->path);
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameAddOn::InstantiateNodeFor():\n\t"
			"InitDeviceState(): %s\n", strerror(err)));
		// +++ should this be fatal?
	}
	
	if (GAME_IS_DAC(i->info.codec_id))
	{
		GameConsumer* node = new GameConsumer(
			i->info.name,
			this,
			i->path,
			i->info.codec_id,
			info->internal_id);
		err = node->InitCheck();
		if (err < B_OK)
		{
			D_TROUBLE((
				"GameAddOn::InstantiateNodeFor():\n\t"
				"GameConsumer::InitCheck(): %s\n",
				strerror(err)));
			*outError = err;
			node->Release();
			node = 0;
		}
		return node;
	}
	else
	{
		GameProducer* node = new GameProducer(
			i->info.name,
			this,
			i->path,
			i->info.codec_id,
			info->internal_id);
		err = node->InitCheck();
		if (err < B_OK)
		{
			D_TROUBLE((
				"GameAddOn::InstantiateNodeFor():\n\t"
				"GameProducer::InitCheck(): %s\n",
				strerror(err)));
			*outError = err;
			node->Release();
			node = 0;
		}
		return node;
	}
}

//#pragma mark --- settings observation API ---
// ------------------------------------------------------------------------ //

status_t 
GameAddOn::StartWatchingSettings(media_node node, const char *device, int32 mixer_id)
{
	BAutolock _l(_settingsLock);
	device_state * d = 0;
	for (int n = 0; !d && n < _deviceState.size(); n++)
	{
		if (!strcmp(_deviceState[n]->Path(), device)) d = _deviceState[n];
	}
	if (!d)
	{
		D_TROUBLE((
			"GameAddOn::StartWatchingSettings():\n\t"
			"no state entry for device %s!\n",
			device));
		return B_BAD_VALUE;
	}
	d->AddObserver(node, mixer_id);
	return B_OK;
}

status_t 
GameAddOn::StopWatchingSettings(media_node node, const char *device)
{
	BAutolock _l(_settingsLock);
	device_state * d = 0;
	int deviceIndex;
	for (deviceIndex = 0; deviceIndex < _deviceState.size(); deviceIndex++)
	{
		if (!strcmp(_deviceState[deviceIndex]->Path(), device))
		{
			d = _deviceState[deviceIndex];
			break;
		}
	}
	if (!d)
	{
		D_TROUBLE((
			"GameAddOn::StopWatchingSettings():\n\t"
			"no state entry for device %s!\n",
			device));
		return B_BAD_VALUE;
	}
	d->RemoveObserver(node);
	if (!d->CountObservers())
	{
		// device is no longer in use; make sure the settings are saved,
		// then delete the state object
		WriteSettings(d);
		delete d;
		_deviceState.erase(_deviceState.begin() + deviceIndex);
	}
	return B_OK;
}

status_t 
GameAddOn::RefreshSettings(const char *device)
{
	BAutolock _l(_settingsLock);
	device_state * d = 0;
	int deviceIndex;
	for (deviceIndex = 0; deviceIndex < _deviceState.size(); deviceIndex++)
	{
		if (!strcmp(_deviceState[deviceIndex]->Path(), device))
		{
			d = _deviceState[deviceIndex];
			break;
		}
	}
	if (!d)
	{
		D_TROUBLE((
			"GameAddOn::RefreshSettings():\n\t"
			"no state entry for device %s!\n",
			device));
		return B_BAD_VALUE;
	}
	status_t err = d->Export();
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameAddOn::RefreshSettings():\n\t"
			"Export(): %s\n",
			 strerror(err)));
	}
	return err;
}

status_t 
GameAddOn::CodecParameterChanged(media_node fromNode, const char *device, int32 mixer_id, int32 param_id)
{
	BAutolock _l(_settingsLock);
	// look up state entry
	device_state * d = 0;
	for (int n = 0; !d && n < _deviceState.size(); n++)
	{
		if (!strcmp(_deviceState[n]->Path(), device)) d = _deviceState[n];
	}
	if (!d)
	{
		D_TROUBLE((
			"GameAddOn::CodecParameterChanged():\n\t"
			"no state entry for device %s!\n",
			device));
		return B_BAD_VALUE;
	}
	// mark it dirty
	d->SetDirty(true);
	// broadcast the change
	d->BroadcastChange(fromNode, mixer_id, param_id);
	return B_OK;
}


//#pragma mark --- internals ---
// ------------------------------------------------------------------------ //

void 
GameAddOn::ClearCodecs()
{
	D_METHOD(("GameAddOn::ClearCodecs()\n"));
	if (!_codecs) return;

	for (int n = _codecs->CountItems(); n > 0; n--)
	{
		codec_info* i = (codec_info*)_codecs->ItemAt(n-1);
		ASSERT(i);
		delete i;
	}
	delete _codecs;
	_codecs = 0;
}


status_t 
GameAddOn::ScanDevices(char *path)
{
	D_METHOD(("GameAddOn::ScanDevices()\n"));
	status_t err;

	char* tail = path + strlen(path);
	DIR* dir = opendir(path);
	if (!dir)
	{
		D_TROUBLE((
			"GameAddOn::ScanDevices():\n\t"
			"not a directory: '%s'\n",
			path));
		return B_BAD_VALUE;
	}
	
	struct dirent* entry;
	while ((entry = readdir(dir)) != 0)
	{
		if (!strcmp(entry->d_name, ".") ||
			!strcmp(entry->d_name, "..")) continue;

		strcpy(tail, "/");
		strcat(tail, entry->d_name);
		
		struct stat stbuf;
		if (stat(path, &stbuf)) continue;
		if (S_ISDIR(stbuf.st_mode))
		{
			ScanDevices(path);
			continue;
		}
		
		ImportCodecs(path);
	}

	closedir(dir);
	return B_OK;
}

status_t 
GameAddOn::ImportCodecs(const char *path)
{
	D_METHOD(("GameAddOn::ImportCodecs()\n"));
	status_t err;
	int fd = open(path, O_RDWR);
	if (fd < 0)
	{
		D_WARNING((
			"GameAddOn::ImportCodecs():\n\t"
			"failed to open '%s': %s\n",
			path, strerror(fd)));
		return fd;
	}

	// get general device stats
	G<game_get_info> ggi;
	err = ioctl(fd, GAME_GET_INFO, &ggi);
	if (err < B_OK)
	{
		D_WARNING((
			"GameAddOn::ImportCodecs():\n\t"
			"GAME_GET_INFO on '%s': %s\n",
			path, strerror(err)));
		// bail
		close(fd);
		return err;
	}
	
	// get info on each ADC
	int count = ggi.adc_count;
	if (count > 0)
	{
		game_codec_info* adcInfo = new game_codec_info[count];
		memset(adcInfo, 0, sizeof(game_codec_info) * count);		
		for (int n = 0; n < count; n++) adcInfo[n].codec_id = GAME_MAKE_ADC_ID(n);
		G<game_get_codec_infos> ggai;
		ggai.in_request_count = count;
		ggai.info = adcInfo;		
		err = ioctl(fd, GAME_GET_CODEC_INFOS, &ggai);
		if (err < B_OK)
		{
			D_WARNING((
				"GameAddOn::ImportCodecs():\n\t"
				"GAME_GET_CODEC_INFOS (ADC) on '%s': %s\n",
				path, strerror(err)));
		}
		count = ggai.out_actual_count;
		codec_info* adc = 0;
		if (count > 0) adc = new codec_info[count];
		for (int n = 0; n < count; n++)
		{
			strcpy(adc[n].path, path);
			adc[n].info = adcInfo[n];
			MakeInputFlavor(&ggi, &adc[n], &adc[n].flavor);
			if (!_codecs) _codecs = new BList;
			ASSERT(_codecs);
			_codecs->AddItem(adc + n);
		}
	}

	// get info on each DAC
	count = ggi.dac_count;
	if (count > 0)
	{
		game_codec_info* dacInfo = new game_codec_info[count];
		memset(dacInfo, 0, sizeof(game_codec_info) * count);		
		for (int n = 0; n < count; n++) dacInfo[n].codec_id = GAME_MAKE_DAC_ID(n);
		G<game_get_codec_infos> ggai;
		ggai.in_request_count = count;
		ggai.info = dacInfo;		
		err = ioctl(fd, GAME_GET_CODEC_INFOS, &ggai);
		if (err < B_OK)
		{
			D_WARNING((
				"GameAddOn::ImportCodecs():\n\t"
				"GAME_GET_CODEC_INFOS (DAC) on '%s': %s\n",
				path, strerror(err)));
		}
		count = ggai.out_actual_count;
		codec_info* dac = 0;
		if (count > 0) dac = new codec_info[count];
		for (int n = 0; n < count; n++)
		{
			strcpy(dac[n].path, path);
			dac[n].info = dacInfo[n];
			MakeOutputFlavor(&ggi, &dac[n], &dac[n].flavor);
			if (!_codecs) _codecs = new BList;
			ASSERT(_codecs);
			_codecs->AddItem(dac + n);
		}
	}
	
	close(fd);
	return B_OK;
}

status_t 
GameAddOn::MakeInputFlavor(const game_get_info* device, const codec_info *info, flavor_info *outFlavor)
{
	outFlavor->flavor_flags = DEF_FLAVA_FLAGS;
	outFlavor->possible_count = 1;
	
	media_format* format = new media_format;
	format->type = B_MEDIA_RAW_AUDIO;
	format->u.raw_audio = media_multi_audio_format::wildcard;
	
	outFlavor->internal_id = _nextInternalID++;
	outFlavor->kinds = B_BUFFER_PRODUCER | B_PHYSICAL_INPUT | B_CONTROLLABLE;
	outFlavor->in_format_count = 0;
	outFlavor->in_formats = 0;
	outFlavor->in_format_flags = 0;
	outFlavor->out_format_count = 1;
	outFlavor->out_formats = format;
	outFlavor->out_format_flags = 0;
	outFlavor->name = strdup(info->info.name);
	outFlavor->info = strdup(device->vendor);
	
	return B_OK;
}

status_t 
GameAddOn::MakeOutputFlavor(const game_get_info* device, const codec_info *info, flavor_info *outFlavor)
{
	outFlavor->flavor_flags = DEF_FLAVA_FLAGS;
	outFlavor->possible_count = 1;
	
	media_format* format = new media_format;
	format->type = B_MEDIA_RAW_AUDIO;
	format->u.raw_audio = media_multi_audio_format::wildcard;

	outFlavor->internal_id = _nextInternalID++;
	outFlavor->kinds = B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT | B_TIME_SOURCE | B_CONTROLLABLE;
	outFlavor->out_format_count = 0;
	outFlavor->out_formats = 0;
	outFlavor->out_format_flags = 0;
	outFlavor->in_format_count = 1;
	outFlavor->in_formats = format;
	outFlavor->in_format_flags = 0;
	outFlavor->name = strdup(info->info.name);
	outFlavor->info = strdup(device->vendor);

	return B_OK;
}

status_t 
GameAddOn::InitDeviceState(const char *devicePath)
{
	BAutolock _l(_settingsLock);
	device_state * d = 0;
	for (int n = 0; !d && n < _deviceState.size(); n++)
	{
		if (!strcmp(_deviceState[n]->Path(), devicePath)) d = _deviceState[n];
	}
	if (!d)
	{
		d = new device_state(devicePath);
		ReadSettings(d);
		_deviceState.push_back(d);
	}
	return StartSettingsThread();
}

void 
GameAddOn::ClearDeviceState()
{
	BAutolock _l(_settingsLock);
	for (int n = 0; n < _deviceState.size(); n++)
	{
		delete _deviceState[n];
	}
	_deviceState.clear();	
}

status_t 
GameAddOn::StartSettingsThread()
{
	if (_settingsThread >= 0) return B_OK;
	if (_settingsSem >= 0) delete_sem(_settingsSem);
	_settingsSem = create_sem(0, "GameAddOn::_settingsSem");
	if (_settingsSem < 0) return _settingsSem;
	_settingsThread = spawn_thread(
		&SettingsThreadEntry, "GameAddOn::_settingsThread",
		B_LOW_PRIORITY, this);
	if (_settingsThread < 0) return _settingsThread;
	return resume_thread(_settingsThread);
}

status_t 
GameAddOn::StopSettingsThread()
{
	if (_settingsThread < 0) return _settingsThread;
	delete_sem(_settingsSem);
	status_t err;
	while (wait_for_thread(_settingsThread, &err) == B_INTERRUPTED) {}
	_settingsSem = -1;
	_settingsThread = -1;
	return B_OK;
}


// static
status_t 
GameAddOn::SettingsThreadEntry(void *cookie)
{
	((GameAddOn*)cookie)->SettingsThread();
	return B_OK;
}

void 
GameAddOn::SettingsThread()
{
	while (acquire_sem_etc(_settingsSem, 1, B_TIMEOUT, 10500000LL) == B_TIMED_OUT)
	{
		BAutolock _l(_settingsLock);
		for (int n = 0; n < _deviceState.size(); n++)
		{
			if (_deviceState[n]->IsDirty())
			{
				WriteSettings(_deviceState[n]);
				_deviceState[n]->SetDirty(false);
			}
		}
	}
}

void 
GameAddOn::WriteSettings(device_state *device)
{
	status_t err;
	BPath path;
	if (find_directory(B_COMMON_SETTINGS_DIRECTORY, &path, true) < B_OK) return;
	path.Append("Media");
	path.Append(device->SettingsFileName());
	D_SETTINGS(("WriteSettings(): about to write %s\n", path.Path()));
	err = device->Import();
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameAddOn::WriteSettings():\n\t"
			"Import(): %s\n", strerror(err)));
		return;
	}
	int fd = open(path.Path(), O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd < 0)
	{
		D_TROUBLE((
			"GameAddOn::WriteSettings():\n\t"
			"open(): %s\n", strerror(err)));
		return;
	}
	int32 v = SETTINGS_FILE_MAGIC;
	write(fd, &v, sizeof(v));
	v = SETTINGS_FILE_VERSION;
	write(fd, &v, sizeof(v));
	ssize_t size = device->FlattenedSize();
	write(fd, &size, sizeof(size));
	void * data = malloc(size);
	if (!data)
	{
		D_TROUBLE((
			"GameAddOn::WriteSettings():\n\t"
			"out of memory.\n"));
	}
	err = device->Flatten(data, size);
	if (err < B_OK)
	{
		free(data);
		close(fd);
		D_TROUBLE((
			"GameAddOn::WriteSettings():\n\t"
			"device->Flatten(): %s\n", strerror(err)));
		return;
	}
	write(fd, data, size);
	free(data);
	close(fd);
}

void 
GameAddOn::ReadSettings(device_state *device)
{
	status_t err;
	BPath path;
	void * data = 0;
	if (find_directory(B_COMMON_SETTINGS_DIRECTORY, &path, true) < B_OK) return;
	path.Append("Media");
	path.Append(device->SettingsFileName());
	D_SETTINGS(("ReadSettings(): about to read %s\n", path.Path()));
	int fd = open(path.Path(), O_RDONLY);
	if (fd < 0)
	{
		D_SETTINGS(("ReadSettings(): %s not found\n", path.Path()));
		return;
	}
	int32 magic[2];
	if (read(fd, magic, 8) < 8 ||
		magic[0] != SETTINGS_FILE_MAGIC ||
		magic[1] != SETTINGS_FILE_VERSION)
	{
		goto corrupt;
	}
	ssize_t size;
	if (read(fd, &size, sizeof(size)) < sizeof(size) ||
		size < 0)
	{
		goto corrupt;
	}
	data = malloc(size);
	if (!data)
	{
		D_TROUBLE((
			"GameAddOn::ReadSettings():\n\t"
			"out of memory.\n"));
	}
	if (read(fd, data, size) < size)
	{
		goto corrupt;
	}
	device->Unflatten(data, size);
	free(data);
#if 1
	// +++ fails on emu10k
	err = device->Export();
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameAddOn::ReadSettings():\n\t"
			"device->Export(): %s\n", strerror(err)));
	}
#endif
	close(fd);
	return;
		
corrupt:
	fprintf(stderr,
		"GameAddOn: corrupt settings file '%s'\n", path.Path());
	if (data) free(data);
	close(fd);
	return;
}

