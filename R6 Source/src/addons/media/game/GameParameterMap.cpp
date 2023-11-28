class BParameterWeb;
class BParameterGroup;

#include "game_tools.h"
#include "GameParameterMap.h"

#include <Debug.h>
#include <ParameterWeb.h>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#define D_TROUBLE(x)	PRINT(x)
#define D_WARNING(x)	PRINT(x)
#define D_METHOD(x)		//PRINT(x)

using namespace BPrivate;

GameParameterMap::GameParameterMap() :
	_controls(0),
	_controlCount(0)
{
}


GameParameterMap::~GameParameterMap()
{
	D_METHOD(("GameParameterMap::~GameParameterMap()\n"));
	if (_controls) free(_controls);
}

status_t 
GameParameterMap::AppendMixerControls(int fd, const game_mixer_info &mixerInfo)
{
	D_METHOD(("GameParameterMap::AppendMixerControls()\n"));
	status_t err;
	// allocate a chunk
	game_mixer_control* base = 0;
	int32 newCount = _controlCount + mixerInfo.control_count;
	if (_controls)
	{
		game_mixer_control* cp = (game_mixer_control*)realloc(
			_controls, newCount * sizeof(game_mixer_control));
		if (!cp) return B_NO_MEMORY;
		_controls = cp;
		base = _controls + _controlCount * sizeof(game_mixer_control);
	}
	else
	{
		ASSERT(!_controlCount);
		_controls = (game_mixer_control*)malloc(
			newCount * sizeof(game_mixer_control));
		if (!_controls) return B_NO_MEMORY;
		base = _controls;
	}
	// fill the chunk
	G<game_get_mixer_controls> ggmc;
	ggmc.mixer_id = mixerInfo.mixer_id;
	ggmc.control = base;
	ggmc.in_request_count = mixerInfo.control_count;
	err = ioctl(fd, GAME_GET_MIXER_CONTROLS, &ggmc);
	if (err < B_OK) return errno;
	_mixers.push_back(mixer_vec::value_type(_controlCount, mixerInfo));
	_controlCount = newCount;
	return B_OK;
}

int32 
GameParameterMap::CountControls() const
{
	return _controlCount;
}

const game_mixer_control *
GameParameterMap::ControlAt(int32 index) const
{
	return (index < 0 || index >= _controlCount) ? 0 : &_controls[index];
}

status_t 
GameParameterMap::GetIndexFor(int32 controlID, int32* outIndex) const
{
	for (int32 n = 0; n < _controlCount; n++)
	{
		if (_controls[n].control_id == controlID)
		{
			*outIndex = n;
			return B_OK;
		}
	}
	return B_BAD_VALUE;
}

int32 
GameParameterMap::CountMixers() const
{
	return _mixers.size();
}

const game_mixer_info *
GameParameterMap::MixerAt(int32 mixerIndex) const
{
	return (mixerIndex < 0 || mixerIndex >= _mixers.size()) ?
		0 :
		&_mixers[mixerIndex].second;
}

int32 
GameParameterMap::LowerBoundFor(int32 mixerIndex) const
{
	return (mixerIndex < 0 || mixerIndex >= _mixers.size()) ?
		B_BAD_INDEX :
		_mixers[mixerIndex].first;
}

status_t 
GameParameterMap::GetParameterValue(int fd, int32 id, void *outValue, size_t *ioSize)
{
	D_METHOD(("GameParameterMap::GetParameterValue()\n"));
	status_t err;

	const game_mixer_control* ci = ControlAt(id & ~PARAM_ID_MASK);
	if (!ci) return B_BAD_INDEX;
		
	H<game_mixer_control_value> gmcv;
	gmcv.control_id = ci->control_id;
	G<game_get_mixer_control_values> ggmcv;
	ggmcv.mixer_id = ci->mixer_id;
	ggmcv.in_request_count = 1;
	ggmcv.values = &gmcv;
	err = ioctl(fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcv);
	if (err < B_OK) return errno;

	switch (id & PARAM_ID_MASK)
	{
		case PARAM_LEVEL_CONTROL_TYPE:
		case PARAM_INV_LEVEL_CONTROL_TYPE:
		case PARAM_MUTE_CONTROL_TYPE:
		{
			if (gmcv.kind != GAME_MIXER_CONTROL_IS_LEVEL)
			{
				D_WARNING(("GameParameterMap::GetParameterValue():\n\t"
					"control type mismatch; expected GAME_MIXER_CONTROL_IS_LEVEL, got %ld\n",
					gmcv.kind));
			}
			if ((id & PARAM_ID_MASK) == PARAM_MUTE_CONTROL_TYPE)
			{
				if (*ioSize < sizeof(int32)) return B_NO_MEMORY;
				*ioSize = sizeof(int32);
				*(int32*)outValue = int32(gmcv.level.flags & GAME_LEVEL_IS_MUTED);
			}
			else
			{
				G<game_get_mixer_level_info> ggmli;
				ggmli.control_id = gmcv.control_id;
				ggmli.mixer_id = ggmcv.mixer_id;
				err = ioctl(fd, GAME_GET_MIXER_LEVEL_INFO, &ggmli);
				if (err < B_OK)
				{
					D_WARNING(("GameParameterMap::GetParameterValue():\n\t"
						"GAME_GET_MIXER_LEVEL_INFO(%x : %x): %s\n",
						ggmli.mixer_id, ggmli.control_id, strerror(errno)));
					return errno;
				}
				if (*ioSize < sizeof(float) * ggmli.value_count) return B_NO_MEMORY;
				*ioSize = sizeof(float) * ggmli.value_count;
				for (int n = 0; n < ggmli.value_count; n++)
				{
					if ((id & PARAM_ID_MASK) == PARAM_INV_LEVEL_CONTROL_TYPE)
						gmcv.level.values[n] = ggmli.max_value - gmcv.level.values[n];
					((float*)outValue)[n] = float(gmcv.level.values[n]);
				}
			}
			break;
		}
		case PARAM_MUX_CONTROL_TYPE:
			if (gmcv.kind != GAME_MIXER_CONTROL_IS_MUX)
			{
				D_WARNING(("GameParameterMap::GetParameterValue():\n\t"
					"control type mismatch; expected GAME_MIXER_CONTROL_IS_MUX, got %ld\n",
					gmcv.kind));
			}
			if (*ioSize < sizeof(int32)) return B_NO_MEMORY;
			*ioSize = sizeof(int32);
			*(int32*)outValue = gmcv.mux.mask;
			break;

		case PARAM_ENABLE_CONTROL_TYPE:
			if (gmcv.kind != GAME_MIXER_CONTROL_IS_ENABLE)
			{
				D_WARNING(("GameParameterMap::GetParameterValue():\n\t"
					"control type mismatch; expected GAME_MIXER_CONTROL_IS_ENABLE, got %ld\n",
					gmcv.kind));
			}
			if (*ioSize < sizeof(int32)) return B_NO_MEMORY;
			*ioSize = sizeof(int32);
			*(int32*)outValue = gmcv.enable.enabled;
			break;
	}

	return B_OK;
}

status_t 
GameParameterMap::SetParameterValue(int fd, int32 id, const void *value, size_t size)
{
	D_METHOD(("GameParameterMap::SetParameterValue()\n"));
	status_t err;

	const game_mixer_control* ci = ControlAt(id & ~PARAM_ID_MASK);
	if (!ci) return B_BAD_INDEX;
		
	H<game_mixer_control_value> gmcv;
	gmcv.control_id = ci->control_id;
	G<game_get_mixer_control_values> ggmcv;
	ggmcv.mixer_id = ci->mixer_id;
	ggmcv.in_request_count = 1;
	ggmcv.values = &gmcv;
	err = ioctl(fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcv);
	if (err < B_OK) return errno;

	switch (id & PARAM_ID_MASK)
	{
		case PARAM_LEVEL_CONTROL_TYPE:
		case PARAM_INV_LEVEL_CONTROL_TYPE:
		case PARAM_MUTE_CONTROL_TYPE:
		{
			if ((id & PARAM_ID_MASK) == PARAM_MUTE_CONTROL_TYPE)
			{
				if (size != sizeof(int32)) return B_NO_MEMORY;
				gmcv.level.flags = *(int32*)value ?
					(gmcv.level.flags | GAME_LEVEL_IS_MUTED) :
					(gmcv.level.flags & ~GAME_LEVEL_IS_MUTED);
			}
			else
			{
				G<game_get_mixer_level_info> ggmli;
				ggmli.control_id = gmcv.control_id;
				ggmli.mixer_id = ggmcv.mixer_id;
				err = ioctl(fd, GAME_GET_MIXER_LEVEL_INFO, &ggmli);
				if (err < B_OK)
				{
					PRINT((
						"set_game_parameter_value():\n\t"
						"GAME_GET_MIXER_LEVEL_INFO(%hx : %hx): %s\n",
						ggmli.mixer_id, ggmli.control_id, strerror(err)));
					return errno;
				}
				if (size != sizeof(float) * ggmli.value_count) return B_NO_MEMORY;
				for (int n = 0; n < ggmli.value_count; n++)
				{
					gmcv.level.values[n] = int16(((float*)value)[n]);
					if ((id & PARAM_ID_MASK) == PARAM_INV_LEVEL_CONTROL_TYPE)
						gmcv.level.values[n] = ggmli.max_value - gmcv.level.values[n];
				}
			}
			break;
		}
		case PARAM_MUX_CONTROL_TYPE:
			if (size != sizeof(int32)) return B_NO_MEMORY;
			gmcv.mux.mask = *(int32*)value;
			break;
		case PARAM_ENABLE_CONTROL_TYPE:
			if (size != sizeof(int32)) return B_NO_MEMORY;
			gmcv.enable.enabled = *(int32*)value;
			break;
	}
	G<game_set_mixer_control_values> gsmcv;
	gsmcv.mixer_id = ggmcv.mixer_id;
	gsmcv.in_request_count = 1;
	gsmcv.values = &gmcv;
	err = ioctl(fd, GAME_SET_MIXER_CONTROL_VALUES, &gsmcv);
	return (err < B_OK) ? errno : B_OK;
}

status_t 
GameParameterMap::MakeParameterWeb(int fd, BParameterWeb **outWeb, uint32 flags)
{
	D_METHOD(("GameParameterMap::MakeParameterWeb()\n"));
	if (!(flags &
		(INCLUDE_DAC_MIXERS | INCLUDE_ADC_MIXERS | INCLUDE_UNBOUND_MIXERS)))
	{
		D_TROUBLE(("GameParameterMap::MakeParameterWeb():\n\t"
			"no mixer bindings specified.\n"));
		return EPERM;
	}

	status_t err;
	BParameterWeb* web = new BParameterWeb();
	
	if (!CountMixers())
	{
		BParameterGroup* group = web->MakeGroup("null");
		group->MakeNullParameter(
			0, B_MEDIA_NO_TYPE, "This device has no controls.", B_GENERIC);
	}
	
	// add groups for codec-bound mixers	
	if (flags & (INCLUDE_DAC_MIXERS | INCLUDE_ADC_MIXERS))
	for (int32 n = 0; n < CountMixers(); n++)
	{
		const game_mixer_info* mi = MixerAt(n);
		ASSERT(mi);
		if (!((flags & INCLUDE_DAC_MIXERS) && GAME_IS_DAC(mi->linked_codec_id)) &&
			!((flags & INCLUDE_ADC_MIXERS) && GAME_IS_ADC(mi->linked_codec_id)))
			continue;
		BParameterGroup* group = web->MakeGroup(mi->name);
		err = PopulateGroup(group, fd, n, flags);
		if (err < B_OK)
		{
			D_WARNING((
				"GameParameterMap::MakeParameterWeb():\n\t"
				"PopulateGroup('%s'): %s\n",
				mi->name, strerror(err)));
			continue;
		}
	}
	// add groups for unbound mixers
	if (flags & INCLUDE_UNBOUND_MIXERS)
	for (int32 n = 0; n < CountMixers(); n++)
	{
		const game_mixer_info* mi = MixerAt(n);
		ASSERT(mi);
		if (mi->linked_codec_id != GAME_NO_ID) continue;
		BParameterGroup* group = web->MakeGroup(mi->name);
		err = PopulateGroup(group, fd, n, flags);
		if (err < B_OK)
		{
			D_WARNING((
				"GameParameterMap::MakeParameterWeb():\n\t"
				"PopulateGroup('%s'): %s\n",
				mi->name, strerror(err)));
			continue;
		}
	}
	*outWeb = web;
	return B_OK;
}

status_t 
GameParameterMap::PopulateGroup(
	BParameterGroup *mainGroup, int fd, int32 mixerIndex,
	uint32 flags)
{
	D_METHOD(("GameParameterMap::PopulateGroup()\n"));
	status_t err;

	const game_mixer_info* mi = MixerAt(mixerIndex);
	ASSERT(mi);

	BParameterGroup* channelGroup = mainGroup->MakeGroup("channels");
	BParameterGroup* auxGroup = mainGroup->MakeGroup("aux");

	for (int32 n = LowerBoundFor(mixerIndex); n < CountControls(); n++)
	{
		const game_mixer_control* ci = ControlAt(n);
		ASSERT(ci);
		if (ci->mixer_id != mi->mixer_id) break;
		if (ci->parent_id != GAME_NO_ID && ci->parent_id != ci->control_id) continue;
		if (!(flags & ENABLE_ADVANCED_CONTROLS) &&
			(ci->flags & GAME_MIXER_CONTROL_ADVANCED)) continue;

		BParameterGroup* group;
		if (ci->flags & GAME_MIXER_CONTROL_AUXILIARY)
		{
			group = auxGroup->MakeGroup("");
		}
		else
		{
			group = channelGroup->MakeGroup("");
		}
		ASSERT(group);
		err = PopulateRecurse(group, fd, n, 0, flags, 0);
		if (err < B_OK) return err;
	}

	return B_OK;
	
}

status_t 
GameParameterMap::PopulateRecurse(
	BParameterGroup *group, int fd, int32 controlIndex, const char *parentLabel,
	uint32 flags, int recurseCount)
{
	D_METHOD(("GameParameterMap::PopulateRecurse()\n"));
	status_t err;

	const game_mixer_control* ci = ControlAt(controlIndex);
	ASSERT(ci);

	if (recurseCount++ > 20)
	{
		D_TROUBLE(("GameParameterMap::PopulateRecurse():\n\t"
			"make_codec_control_recurse(): structure too deep (current: %ld, parent: %ld)\n",
			ci->control_id, ci->parent_id));
		return B_ERROR;
	}

	// make appropriate parameter(s)
	char label[32];
	switch (ci->kind)
	{
		case GAME_MIXER_CONTROL_IS_LEVEL:
			err = PopulateLevelControl(group, fd, controlIndex, parentLabel, label);
			break;
		case GAME_MIXER_CONTROL_IS_MUX:
			err = PopulateMuxControl(group, fd, controlIndex, parentLabel, label);
			break;
		case GAME_MIXER_CONTROL_IS_ENABLE:
			err = PopulateEnableControl(group, fd, controlIndex, parentLabel, label);
			break;
		default:
			err = B_ERROR;
	}
	if (err < B_OK) return err;
	
	// recurse to child controls
	for (int n = 0; n < CountControls(); n++)
	{
		const game_mixer_control* cci = ControlAt(n);
		ASSERT(cci);
		if (cci->parent_id != ci->control_id) continue;
		if (cci->control_id == ci->control_id) continue;
		if (!(flags & ENABLE_ADVANCED_CONTROLS) &&
			(cci->flags & GAME_MIXER_CONTROL_ADVANCED)) continue;

		err = PopulateRecurse(group, fd, n, label, flags, recurseCount);
		if (err < B_OK) return err;
	}

	return B_OK;
}

status_t 
GameParameterMap::PopulateLevelControl(
	BParameterGroup *group, int fd, int32 controlIndex, const char *parentLabel,
	char *outLabel)
{
	D_METHOD(("GameParameterMap::PopulateLevelControl()\n"));
	
	const game_mixer_control* ci = ControlAt(controlIndex);
	ASSERT(ci);
	ASSERT(ci->kind == GAME_MIXER_CONTROL_IS_LEVEL);
	status_t err;

	G<game_get_mixer_level_info> ggmli;
	ggmli.control_id = ci->control_id;
	ggmli.mixer_id = ci->mixer_id;
	err = ioctl(fd, GAME_GET_MIXER_LEVEL_INFO, &ggmli);
	if (err < B_OK) return errno;

	strcpy(outLabel, ggmli.label);
	
	BParameter* prev = 0;
	
	if (ci->parent_id == GAME_NO_ID)
	{
		BNullParameter* label = group->MakeNullParameter(
			controlIndex | PARAM_LABEL_TYPE,
			B_MEDIA_RAW_AUDIO,
			ggmli.label,
			B_WEB_BUFFER_INPUT);
		prev = label;
	}
	
	bool makeMute = true;	
	const char* kind = B_GAIN;
	if (ci->flags & GAME_LEVEL_IS_PAN)
	{
		kind = B_BALANCE;
		makeMute = false;
	}
	else
	if (ci->flags & GAME_LEVEL_IS_EQ)
	{
		kind = B_EQUALIZATION;
		makeMute = false;
	}
	else switch (ggmli.type)
	{
		case GAME_LEVEL_AC97_MASTER:
			kind = B_MASTER_GAIN;
			break;
		case GAME_LEVEL_AC97_TREBLE:
		case GAME_LEVEL_AC97_BASS:
			kind = B_EQUALIZATION;
			makeMute = false;
			break;
		case GAME_LEVEL_AC97_3D_DEPTH:
		case GAME_LEVEL_AC97_3D_CENTER:
			kind = B_LEVEL;
			makeMute = false;
			break;
	}
	
	if (makeMute && (ggmli.flags & GAME_LEVEL_HAS_MUTE))
	{
		BDiscreteParameter* mute = group->MakeDiscreteParameter(
			controlIndex | PARAM_MUTE_CONTROL_TYPE,
			B_MEDIA_RAW_AUDIO,
			"Mute",
			B_MUTE);
		if (prev) mute->AddInput(prev);
		prev = mute;
	}
	
	int32 id = controlIndex;
	float factor = (ggmli.min_value == ggmli.max_value) ? 0 :
		(ggmli.max_value_disp - ggmli.min_value_disp) / float(ggmli.max_value - ggmli.min_value);
	float offset;
	if (factor < 0)
	{
		factor = -factor;
		offset = ggmli.max_value_disp;
		id |= PARAM_INV_LEVEL_CONTROL_TYPE;
	}
	else 
	{
		offset = ggmli.min_value_disp;
		id |= PARAM_LEVEL_CONTROL_TYPE;
	}
	BContinuousParameter* control = group->MakeContinuousParameter(
		id,
		B_MEDIA_RAW_AUDIO,
		(parentLabel && strlen(ggmli.label) && strcmp(ggmli.label, parentLabel)) ? ggmli.label : 0,
		kind,
		(ggmli.flags & GAME_LEVEL_VALUE_IN_DB) ? "dB" : 0,
		ggmli.min_value, ggmli.max_value, 1.0);
	control->SetChannelCount(
		ggmli.value_count);
	control->SetResponse(
		BContinuousParameter::B_LINEAR,
		factor, offset);
	
	if (prev) control->AddInput(prev);
	
	return B_OK;
}

status_t 
GameParameterMap::PopulateMuxControl(
	BParameterGroup *group, int fd, int32 controlIndex, const char *parentLabel,
	char *outLabel)
{
	D_METHOD(("GameParameterMap::PopulateMuxControl()\n"));

	const game_mixer_control* ci = ControlAt(controlIndex);
	ASSERT(ci);
	ASSERT(ci->kind == GAME_MIXER_CONTROL_IS_MUX);
	status_t err;

	G<game_get_mixer_mux_info> ggmmi;
	ggmmi.control_id = ci->control_id;
	ggmmi.mixer_id = ci->mixer_id;
	ggmmi.in_request_count = 32;
	ggmmi.items = (game_mux_item*)alloca(sizeof(game_mux_item) * ggmmi.in_request_count);
	err = ioctl(fd, GAME_GET_MIXER_MUX_INFO, &ggmmi);
	if (err < B_OK) return errno;

	strcpy(outLabel, ggmmi.label);
	BDiscreteParameter* mux = group->MakeDiscreteParameter(
		controlIndex | PARAM_MUX_CONTROL_TYPE,
		B_MEDIA_RAW_AUDIO,
		ggmmi.label,
		B_INPUT_MUX);
	for (int n = 0; n < ggmmi.out_actual_count; n++)
	{
		mux->AddItem(ggmmi.items[n].mask, ggmmi.items[n].name);
	}
	
	return B_OK;
}

status_t 
GameParameterMap::PopulateEnableControl(
	BParameterGroup *group, int fd, int32 controlIndex, const char *parentLabel,
	char *outLabel)
{
	D_METHOD(("GameParameterMap::PopulateEnableControl()\n"));

	const game_mixer_control* ci = ControlAt(controlIndex);
	ASSERT(ci);
	ASSERT(ci->kind == GAME_MIXER_CONTROL_IS_ENABLE);
	status_t err;

	G<game_get_mixer_enable_info> ggmei;
	ggmei.control_id = ci->control_id;
	ggmei.mixer_id = ci->mixer_id;
	err = ioctl(fd, GAME_GET_MIXER_ENABLE_INFO, &ggmei);
	if (err < B_OK) return errno;

	strcpy(outLabel, ggmei.label);
	BDiscreteParameter* enable = group->MakeDiscreteParameter(
		controlIndex | PARAM_ENABLE_CONTROL_TYPE,
		B_MEDIA_RAW_AUDIO,
		ggmei.label,
		B_ENABLE);

	return B_OK;
}
