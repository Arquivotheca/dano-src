#ifndef NETRONDISPLAY_H
#define NETRONDISPLAY_H

#include <SupportDefs.h>

class NetronKeyCode {
	public:
		enum KeyCode {
			POWER_KEY       = 0,
			EMAIL_KEY       = 1,
			MEDIA_KEY       = 2,
			WEB_KEY         = 3,
			VOLUME_UP_KEY   = 4,
			VOLUME_DOWN_KEY = 5,
			
			/* keys on old hardware */
			CALENDAR_KEY = 8,
			IRADIO_KEY = 9,
			MENU_KEY = 10
		};
};

class NetronDisplay {
	public:
		enum power_state {
			POWER_ON		= 0x02,
			POWER_OFF		= 0x03,
			POWER_STANDBY	= 0x06,
		};

		NetronDisplay(int debug_level = -1);
		virtual ~NetronDisplay();
		status_t InitCheck() { return init_status; }
		status_t GetRegister(uint8 reg, uint8 *value);
		status_t SetRegister(uint8 reg, uint8 value);
		status_t GetKeyState(uint32 *keys);
		status_t SendReply(status_t status);
		status_t GetStats(uint32 *fail_count);
		status_t RecallFactorySettings(bool basic = true, bool geometry = true);
		status_t ActivateDegauss();
		status_t GetFactorySetting(uint8 reg, uint8 *value);
		status_t SaveSettings();
		status_t SetPowerState(power_state state);
		status_t GetPowerState(power_state *state);
		void     SetIgnorePowerKey(bool ignore);
	protected:
		virtual status_t KeyPressed(uint8 key_code, bool down);
		virtual status_t TestPattern(uint8 pattern);
		status_t init_status;

	private:
		void     RetrieveRevision();


		status_t SendCommand(class BMessage *msg, void *reply_data = NULL,
		                     size_t reply_data_len = 0);
		
		class BInputDevice *InputDevice;
		class BLooper *Looper;

	friend class MessageLooper;
};

#endif
