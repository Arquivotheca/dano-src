
#if !defined(controls_h)
#define controls_h

#include <OptionPopUp.h>
#include <CheckBox.h>
#include <string.h>

#include "game_audio2.h"

class BStringView;
class BChannelSlider;
class BSlider;

template<class T> class G : public T {
public:
	G() { memset(this, 0, sizeof(*this)); info_size = sizeof(*this); }
};

template<class T> class H : public T {
public:
	H() { memset(this, 0, sizeof(*this)); }
};

#define C(x) do { status_t s = (x); if (s < 0) { perror(#x); exit(1); } } while(0)

//	These should have some interface in common that makes them
//	find their way back to their control(s).
class GameMixerItem {
public:
							GameMixerItem(
								int16 control,
								int16 mixer) {
								value.mixer_id = mixer;
								value.control_id = control;
							}
		virtual void		ReadValue(int fd) {
								G<game_get_mixer_control_values> ggmcvs;
								ggmcvs.in_request_count = 1;
								ggmcvs.values = &value;
								ggmcvs.mixer_id = value.mixer_id;
								C(ioctl(fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcvs));
							}
		virtual void		ApplyValue(int fd) {
								G<game_set_mixer_control_values> gsmcvs;
								gsmcvs.in_request_count = 1;
								gsmcvs.mixer_id = value.mixer_id;
								gsmcvs.values = &value;
								C(ioctl(fd, GAME_SET_MIXER_CONTROL_VALUES, &gsmcvs));
							}
protected:
		game_mixer_control_value value;
};

class AuxLevelControl :
	public BView,
	public GameMixerItem
{
public:
							AuxLevelControl(
								const game_get_mixer_level_info & level);
		virtual void		ReadValue(int fd);
		virtual void		ApplyValue(int fd);
		void				UpdateText();
		virtual void		AttachedToWindow();
private:
		BSlider * m_slider;
		bool flip;
		int16 pivot;
		game_get_mixer_level_info info;
		BStringView * m_disp;
};

class LevelControl : public BView, public GameMixerItem {
public:
							LevelControl(
								const game_get_mixer_level_info & level);
		virtual void		ReadValue(int fd);
		virtual void		ApplyValue(int fd);
		void				UpdateText();
		virtual void		AttachedToWindow();
private:
		int count;
		BCheckBox * m_mute;
		BChannelSlider * m_slider;
		bool flip;
		int16 pivot;
		game_get_mixer_level_info info;
		BStringView * m_disp;
};

class MuxControl : public BOptionPopUp, public GameMixerItem {
public:
							MuxControl(
								const game_get_mixer_mux_info & level);
		virtual void		ReadValue(int fd);
		virtual void		ApplyValue(int fd);
};

class EnableControl : public BCheckBox, public GameMixerItem {
public:
							EnableControl(
								const game_get_mixer_enable_info & level);
		virtual void		ReadValue(int fd);
		virtual void		ApplyValue(int fd);
};


#endif	//	controls_h
