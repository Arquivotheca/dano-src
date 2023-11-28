#ifndef NETRONDISPLAYCONTROL_H
#define NETRONDISPLAYCONTROL_H

#include <OS.h>
#include <Locker.h>
#include <termios.h>

#define NETRON_NORMAL_TIMEOUT 250000
#define NETRON_POWER_TIMEOUT 1700000

class NetronDisplayControl {
	public:
		enum OldNetronKeyCode {
			OLD_NO_KEY = 0x00,
			OLD_POWER_KEY = 0x01,
			OLD_VOLUME_UP = 0x02,
			OLD_VOLUME_DOWN = 0x03,
			OLD_EMAIL_KEY = 0x05,
			OLD_WEB_KEY = 0x06,
			OLD_CALENDAR_KEY = 0x07,
			OLD_IRADIO_KEY = 0x08,
			OLD_MENU_KEY = 0x09
		};
		enum NetronKeyCode {
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
		NetronDisplayControl(int debug_level = 0);
		virtual ~NetronDisplayControl();
		status_t InitCheck() { return init_status; }
		status_t EnableKeyMessages();
		status_t SendRawPacket(uint8 *packet, size_t packet_len); /* no response */
		status_t SendPacket(bigtime_t timeout, uint8 *packet, size_t packet_len,
		                    uint8 *response = NULL, size_t *response_len = NULL);

		status_t GetRegister(uint8 reg, uint8 *value);
		status_t SetRegister(uint8 reg, uint8 value);
		
		status_t GetFactorySetting(uint8 reg, uint8 *value);
		
		int fail_count;
		uint32 GetKeyState();
		void SetDebugLevel(int new_debug_level);
	protected:
		virtual status_t KeyPressed(uint8 key_code, bool down);
		virtual status_t TestPattern(uint8 pattern);
		virtual status_t PowerOnReset();
	private:
		status_t InitDevice(const char * const name);
		status_t ReadPacket();
		void     ReadPackets();
		void     ProcessNewKeyState(uint32 new_key_state);
		status_t ParseResponsePacket(uint8 *data, size_t data_len);
		status_t ParseRequestPacket(uint8 *data, size_t data_len);

		static int32 input_loop(void *data);

		status_t        init_status;
		struct termios  t;
		int             serial_fd;
		thread_id       input_thread;
		port_id         response_port;
		uint32          key_state;
		bool            old_display_protocol;
		int             debug_level;
		bool            waiting_for_keymap_request;
		int             pending_byte;
		BLocker         send_lock;
};

#endif
