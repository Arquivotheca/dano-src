#if !defined(_JOYSTICK_TWEAKER)
#define _JOYSTICK_TWEAKER

#include <Joystick.h>
#include <stdlib.h>
#include "joystick_driver.h"

struct axis_calib_data {
	int16	bottom;
	int16	start_dead;	/* inclusive */
	int16	end_dead;	/* inclusive */
	int16	top;
	int16	bottom_mul;	/* 128 == normal */
	int16	top_mul;	/* 128 == normal */
};

struct BJoystick::_joystick_info {
	_joystick_info()
		{
			memset(&module, 0, sizeof(module));
			memset(&data, 0, sizeof(data)*4);
			memset(file_name, 0, sizeof(file_name));
			memset(axis_names, 0, sizeof(axis_names));
			memset(hat_names, 0, sizeof(hat_names));
			memset(button_names, 0, sizeof(button_names));
			axis_calib_data d = { 0, 0, 0, 0, 128, 128 };
			for (int ix=0; ix<MAX_AXES; ix++) {
				axis_calib[ix] = d;
			}
			button_autofire = 0;
			button_latch = 0;
			prev_read = 0;
			prev_latch = 0;
			prev_auto = 0;
			calibrate = true;
			module.num_sticks = 1;
		}
	~_joystick_info()
		{
			for (int ix=0; ix<module.num_axes; ix++)
				free(axis_names[ix]);
			for (int ix=0; ix<module.num_buttons; ix++)
				free(button_names[ix]);
		}
	_joystick_info(const _joystick_info & other)
		{
			memcpy(this, &other, sizeof(*this));
			for (int ix=0; ix<module.num_axes; ix++)
				if (axis_names[ix]) axis_names[ix] = strdup(axis_names[ix]);
			for (int ix=0; ix<module.num_hats; ix++)
				if (hat_names[ix]) hat_names[ix] = strdup(hat_names[ix]);
			for (int ix=0; ix<module.num_buttons; ix++)
				if (button_names[ix]) button_names[ix] = strdup(button_names[ix]);
		}
	_joystick_info & operator=(const _joystick_info & other)
		{
			memcpy(this, &other, sizeof(*this));
			for (int ix=0; ix<module.num_axes; ix++)
				if (axis_names[ix]) axis_names[ix] = strdup(axis_names[ix]);
			for (int ix=0; ix<module.num_hats; ix++)
				if (hat_names[ix]) hat_names[ix] = strdup(hat_names[ix]);
			for (int ix=0; ix<module.num_buttons; ix++)
				if (button_names[ix]) button_names[ix] = strdup(button_names[ix]);
			return *this;
		}
	joystick_module_info module;
	extended_joystick data;
	extended_joystick data_2;
	extended_joystick data_3;
	extended_joystick data_4;
	char file_name[256];
	char * axis_names[MAX_AXES];
	char * hat_names[MAX_HATS];
	char * button_names[MAX_BUTTONS];
	axis_calib_data axis_calib[MAX_AXES];
	uint32 button_autofire;
	uint32 button_latch;
	uint32 prev_read;
	uint32 prev_latch;
	uint32 prev_auto;
	bool calibrate;
};

class _BJoystickTweaker {
public:
		_BJoystickTweaker(
				BJoystick & stick);
		~_BJoystickTweaker();

		void scan_including_disabled();
		status_t save_config(
				const entry_ref * ref);
		BJoystick::_joystick_info * get_info();

		BJoystick & m_stick;
};

#endif	/* _JOYSTICK_TWEAKER */
