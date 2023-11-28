#if !defined(__GAME_PARAMETER_MAP_H__)
#define __GAME_PARAMETER_MAP_H__

#include "game_audio.h"
#include <vector>

namespace BPrivate {

class GameParameterMap {
public:
	enum parameter_id_type_t {
		PARAM_LABEL_TYPE				= 0x10000000,
		PARAM_LEVEL_CONTROL_TYPE		= 0x20000000,
		PARAM_INV_LEVEL_CONTROL_TYPE	= 0x30000000,
		PARAM_MUTE_CONTROL_TYPE			= 0x40000000,
		PARAM_MUX_CONTROL_TYPE			= 0x50000000,
		PARAM_ENABLE_CONTROL_TYPE		= 0x60000000,
		PARAM_TOGGLE_ADVANCED_CONTROLS	= 0x70000000,
		
		PARAM_ID_MASK					= 0xf0000000
	};
	
	GameParameterMap();
	~GameParameterMap();

	status_t AppendMixerControls(int fd, const game_mixer_info& mixerInfo);

	int32 CountControls() const;
	const game_mixer_control* ControlAt(int32 index) const;
	status_t GetIndexFor(int32 controlID, int32* outIndex) const;
	
	int32 CountMixers() const;
	const game_mixer_info* MixerAt(int32 mixerIndex) const;
	int32 LowerBoundFor(int32 mixerIndex) const;

	enum make_web_flag_t {
		INCLUDE_DAC_MIXERS			= 1<<0,
		INCLUDE_ADC_MIXERS			= 1<<1,
		INCLUDE_UNBOUND_MIXERS		= 1<<2,
		ENABLE_ADVANCED_CONTROLS	= 1<<3
	};
	status_t MakeParameterWeb(int fd, class BParameterWeb** outWeb, uint32 flags);

	status_t GetParameterValue(int fd, int32 id, void* outValue, size_t* ioSize);
	status_t SetParameterValue(int fd, int32 id, const void* value, size_t size);

private:
	game_mixer_control*		_controls;
	int32					_controlCount;
	typedef std::vector< std::pair<int32, game_mixer_info> >	mixer_vec;
	mixer_vec				_mixers;
	
	status_t PopulateGroup(
		class BParameterGroup* group,
		int fd,
		int32 mixerIndex,
		uint32 flags);

	status_t PopulateRecurse(
		class BParameterGroup* group,
		int fd,
		int32 controlIndex,
		const char* parentLabel,
		uint32 flags,
		int recurseCount);

	status_t PopulateLevelControl(
		class BParameterGroup* group,
		int fd,
		int32 controlIndex,
		const char* parentLabel,
		char* outLabel);

	status_t PopulateMuxControl(
		class BParameterGroup* group,
		int fd,
		int32 controlIndex,
		const char* parentLabel,
		char* outLabel);

	status_t PopulateEnableControl(
		class BParameterGroup* group,
		int fd,
		int32 controlIndex,
		const char* parentLabel,
		char* outLabel);
};

}; // BPrivate
#endif //__GAME_PARAMETER_MAP_H__
