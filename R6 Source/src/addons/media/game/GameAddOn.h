#if !defined(__GAME_ADDON_H__)
#define __GAME_ADDON_H__

#include <Locker.h>
#include <MediaAddOn.h>
#include <String.h>
#include "game_audio.h"

#include <vector>

class BList;

namespace BPrivate {

// common node messages
enum gamenode_message_t
{
	M_GAMENODE_PARAM_CHANGED		= 0x60000000,
	M_GAMENODE_FIRST_PRIVATE_MESSAGE
};

struct gamenode_param_changed_msg
{
	int32 mixer_id;
	int32 control_id;
};

class GameAddOn : public BMediaAddOn
{
public:
	GameAddOn(image_id image);
	~GameAddOn();
	
	status_t InitCheck(
		const char** outFailureText);
	int32 CountFlavors();
	status_t GetFlavorAt(
		int32 index,
		const flavor_info** outInfo);
	BMediaNode* InstantiateNodeFor(
		const flavor_info* info,
		BMessage* config,
		status_t* outError);

	status_t StartWatchingSettings(
		media_node node,
		const char * device,
		int32 mixer_id);
	
	status_t StopWatchingSettings(
		media_node node,
		const char * device);

	status_t RefreshSettings(
		const char * device);

	status_t CodecParameterChanged(
		media_node fromNode,
		const char * device,
		int32 mixer_id,
		int32 param_id);

private:
	class codec_info;
	class mixer_state;
	class device_state;

	// device enumeration
	status_t		_initStatus;
	BList*			_codecs;
	int32			_nextInternalID;

	void ClearCodecs();

	status_t ScanDevices(
		char* path);
	status_t ImportCodecs(
		const char* path);
	status_t MakeInputFlavor(
		const game_get_info* device,
		const codec_info* codec,
		flavor_info* outFlavor);
	status_t MakeOutputFlavor(
		const game_get_info* device,
		const codec_info* info,
		flavor_info* outFlavor);

	// settings guts
	BLocker					_settingsLock;
	std::vector<device_state *>	_deviceState;
	thread_id				_settingsThread;
	sem_id					_settingsSem;
	
	status_t InitDeviceState(const char * devicePath);
	void ClearDeviceState();
	
	status_t StartSettingsThread();
	status_t StopSettingsThread();
	
	static status_t SettingsThreadEntry(void * cookie);
	void SettingsThread();
	
	void WriteSettings(device_state * device);
	void ReadSettings(device_state * device);
};

}; // BPrivate
#endif //__GAME_ADDON_H__
