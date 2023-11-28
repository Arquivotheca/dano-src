
#if !defined(JOY_CALIB_H)
#define JOY_CALIB_H

#include <List.h>
#include <Window.h>
#include "JoystickTweaker.h"

class BJoystick;
class BTextView;


class JoyCalib : public BWindow {
public:
		JoyCalib(
				BRect parent,
				BJoystick & stick, 
				BWindow * show);
		~JoyCalib();

virtual	bool QuitRequested();
virtual	void MessageReceived(
				BMessage * message);
virtual	void Pulse();

		BJoystick & Stick();
		void SetInstructions(
				const char * instructions);
		void EnterMode(
				int mode);
		int Mode();

private:

friend class CalibAxis;

		BTextView * m_instructions;
		sem_id m_wait;
		BJoystick & m_stick;
		BList m_calib_views;
		float m_where;
		int16 m_values[MAX_AXES];
		uint32 m_buttons;
		uint8 m_hats[MAX_HATS];
		thread_id m_idler;
		int m_mode;
		BWindow * m_show;
		bigtime_t m_last_click;
		BJoystick::_joystick_info * m_info;
		BJoystick::_joystick_info m_prev_info;
		int16 m_ax_min[MAX_AXES];
		int16 m_ax_max[MAX_AXES];

static	status_t Idle(
				void * data);
};


#endif	/* JOY_CALIB_H */
