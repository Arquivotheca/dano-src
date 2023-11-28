
#include "controls.h"

#include <Message.h>
#include <MenuField.h>
#include <ChannelSlider.h>
#include <StringView.h>
#include <Slider.h>




AuxLevelControl::AuxLevelControl(const game_get_mixer_level_info &level) :
	BView(BRect(0,0,127,77), "auxLevel",
		B_FOLLOW_NONE, B_WILL_DRAW),
	GameMixerItem(level.control_id, level.mixer_id),
	info(level)
{
	BMessage cch('cch!');
	cch.AddInt32("mixer", level.mixer_id);
	cch.AddInt32("level", level.control_id);
	m_slider = new BSlider(BRect(0,0,127,57), "AuxGain", level.label,
		new BMessage(cch), level.min_value, level.max_value);
	AddChild(m_slider);
	char str_min[64], str_max[64];
	sprintf(str_min, level.disp_format, level.min_value_disp);
	sprintf(str_max, level.disp_format, level.max_value_disp);
	if (level.min_value_disp > level.max_value_disp) {
		m_slider->SetLimitLabels(str_max, str_min);
	}
	else {
		m_slider->SetLimitLabels(str_min, str_max);
	}
	flip = level.min_value_disp > level.max_value_disp;
	pivot = level.max_value+level.min_value;
	m_disp = new BStringView(BRect(0,58,127,77), "disp", "");
	m_disp->SetAlignment(B_ALIGN_CENTER);
	AddChild(m_disp);
}

void 
AuxLevelControl::ReadValue(int fd)
{
	GameMixerItem::ReadValue(fd);
	int32 v;
	if (flip) {
		v = pivot-value.level.values[0];
	}
	else {
		v = value.level.values[0];
	}
	m_slider->SetValue(v);
	UpdateText();
}

void 
AuxLevelControl::ApplyValue(int fd)
{
	int32 v;
	v = m_slider->Value();
	if (flip) {
		value.level.values[0] = pivot-v;
	}
	else {
		value.level.values[0] = v;
	}
	value.level.flags = 0;
	GameMixerItem::ApplyValue(fd);
	UpdateText();
}

void 
AuxLevelControl::UpdateText()
{
	char str[64];
	float f = info.min_value_disp+((info.max_value_disp-info.min_value_disp)*
		(value.level.values[0]-info.min_value))/info.max_value;
	sprintf(str, info.disp_format, f);
	m_disp->SetText(str);
}

void 
AuxLevelControl::AttachedToWindow()
{
	BView * p = Parent();
	if (p != 0) {
		SetViewColor(p->ViewColor());
		m_disp->SetViewColor(ViewColor());
	}
}


LevelControl::LevelControl(const game_get_mixer_level_info &level) :
	BView(BRect(0,0,63,(level.flags & GAME_LEVEL_HAS_MUTE) ? 231 : 211), "level",
		B_FOLLOW_NONE, B_WILL_DRAW),
	GameMixerItem(level.control_id, level.mixer_id),
	info(level)
{
	count = level.value_count;
	float top = (level.flags & GAME_LEVEL_HAS_MUTE) ? 192 : 0;
	BMessage cch('cch!');
	cch.AddInt32("mixer", level.mixer_id);
	cch.AddInt32("level", level.control_id);
	m_slider = new BChannelSlider(BRect(0,0,63,191), "Gain", level.label,
		new BMessage(cch), level.value_count);
	AddChild(m_slider);
	if (top > 0) {
		cch.AddBool("mute", true);
		m_mute = new BCheckBox(BRect(0,top,63,top+19), "Mute", "Mute",
			new BMessage(cch));
		AddChild(m_mute);
		top = 212;
	}
	else {
		m_mute = 0;
		top = 192;
	}
	m_slider->SetLimits(level.min_value, level.max_value);
	char str_min[64], str_max[64];
	sprintf(str_min, level.disp_format, level.min_value_disp);
	sprintf(str_max, level.disp_format, level.max_value_disp);
	if (level.min_value_disp > level.max_value_disp) {
		m_slider->SetLimitLabels(str_max, str_min);
	}
	else {
		m_slider->SetLimitLabels(str_min, str_max);
	}
	flip = level.min_value_disp > level.max_value_disp;
	pivot = level.max_value+level.min_value;
	m_disp = new BStringView(BRect(0,top,63,top+19), "disp", "");
	AddChild(m_disp);
}

void 
LevelControl::ReadValue(int fd)
{
	GameMixerItem::ReadValue(fd);
	int32 v[8];
	if (flip) {
		for (int ix=0; ix<count; ix++) {
			v[ix] = pivot-value.level.values[ix];
		}
	}
	else {
		for (int ix=0; ix<count; ix++) {
			v[ix] = value.level.values[ix];
		}
	}
	m_slider->SetValue(0, count, v);
	if (m_mute) {
		if (value.level.flags & GAME_LEVEL_IS_MUTED) {
			m_mute->SetValue(1);
		}
		else {
			m_mute->SetValue(0);
		}
	}
	UpdateText();
}

void 
LevelControl::ApplyValue(int fd)
{
	int32 v[8];
	m_slider->GetValue(v, 0, count);
	if (flip) {
		for (int ix=0; ix<count; ix++) {
			value.level.values[ix] = pivot-v[ix];
		}
	}
	else {
		for (int ix=0; ix<count; ix++) {
			value.level.values[ix] = v[ix];
		}
	}
	if (m_mute) {
		value.level.flags = (m_mute->Value() ? GAME_LEVEL_IS_MUTED : 0);
	}
	else {
		value.level.flags = 0;
	}
	GameMixerItem::ApplyValue(fd);
	UpdateText();
}

void
LevelControl::UpdateText()
{
	char str[64];
	float f = info.min_value_disp+((info.max_value_disp-info.min_value_disp)*
		(value.level.values[0]-info.min_value))/info.max_value;
	sprintf(str, info.disp_format, f);
	m_disp->SetText(str);
}

void 
LevelControl::AttachedToWindow()
{
	BView * p = Parent();
	if (p != 0) {
		SetViewColor(p->ViewColor());
		m_disp->SetViewColor(ViewColor());
	}
}


namespace controls_cpp {

class MuxMessage : public BMessage {
public:
		MuxMessage(int16 mixer, int16 control) : BMessage('cch!') {
			AddInt32("mixer", mixer);
			AddInt32("mux", control);
		}
};

class EnableMessage : public BMessage {
public:
		EnableMessage(int16 mixer, int16 control) : BMessage('cch!') {
			AddInt32("mixer", mixer);
			AddInt32("enable", control);
		}
};

}
using namespace controls_cpp;

MuxControl::MuxControl(const game_get_mixer_mux_info &mux) :
	BOptionPopUp(BRect(0,0,127,23), "mux", mux.label,
		new MuxMessage(mux.mixer_id, mux.control_id), true),
	GameMixerItem(mux.control_id, mux.mixer_id)
{
	MenuField()->SetDivider(63);
	for (int ix=0; ix<mux.out_actual_count; ix++) {
		AddOptionAt(mux.items[ix].name, mux.items[ix].mask, ix);
	}
}

void 
MuxControl::ReadValue(int fd)
{
	GameMixerItem::ReadValue(fd);
	SetValue(value.mux.mask);
}

void 
MuxControl::ApplyValue(int fd)
{
	value.mux.mask = Value();
	GameMixerItem::ApplyValue(fd);
}



EnableControl::EnableControl(const game_get_mixer_enable_info &enable) :
	BCheckBox(BRect(0,0,127,23), "enable", enable.label,
		new EnableMessage(enable.mixer_id, enable.control_id)),
	GameMixerItem(enable.control_id, enable.mixer_id)
{
}

void 
EnableControl::ReadValue(int fd)
{
	GameMixerItem::ReadValue(fd);
	SetValue(value.enable.enabled);
}

void 
EnableControl::ApplyValue(int fd)
{
	value.enable.enabled = Value();
	GameMixerItem::ApplyValue(fd);
}

