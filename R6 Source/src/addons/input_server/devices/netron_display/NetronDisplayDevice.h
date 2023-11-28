#ifndef NETRONDISPLAYDEVICE_H
#define NETRONDISPLAYDEVICE_H

#include <InputServerDevice.h>
#include <Looper.h>
#include <Locker.h>
#include "NetronDisplayControl.h"

class NetronLooper;

class NetronDisplayDevice
	: public BInputServerDevice
	, public NetronDisplayControl {
	public:
		NetronDisplayDevice();
		virtual ~NetronDisplayDevice();
		virtual status_t InitCheck();
		virtual status_t Start(const char *device, void *cookie);
		virtual status_t Stop(const char *device, void *cookie);
		virtual status_t Control(const char	*device, void *cookie, uint32 code, 
		                         BMessage *message);
		void             HandlePowerKey(BMessage *message);
	protected:
		virtual status_t KeyPressed(uint8 key_code, bool down);
		virtual status_t TestPattern(uint8 pattern);
		virtual status_t PowerOnReset();
	private:
		status_t SetAudioMuteState(bool mute);
		bool UserHasAudioMuted(void);
				
		bool           SendNormalKeys;
		bool           SendRawKeys;
		bool           IgnorePowerKey;
		bool           m_powerOff;
		BMessenger     RawKeyMessenger;
		NetronLooper  *Looper;
		BLocker        Lock;
		//status_t init_status;
};

#endif

