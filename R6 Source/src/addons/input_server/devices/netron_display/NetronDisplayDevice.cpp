
#include "NetronDisplayDevice.h"
#include "miniplay.h"
#if DEBUG
#include <string.h>
#endif /* #if DEBUG */
#include <Debug.h>
#include <Autolock.h>

extern "C" _EXPORT BInputServerDevice* instantiate_input_device();

class NetronLooper : public BLooper {
	public:
		NetronLooper(NetronDisplayDevice *owner);
		virtual void     MessageReceived(BMessage *message);
	private:
		NetronDisplayDevice *owner;
};

NetronLooper::NetronLooper(NetronDisplayDevice *owner)
	: BLooper("NetronDisplayLooper", B_NORMAL_PRIORITY, 2)
	, owner(owner)
{
}

void 
NetronLooper::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP: {
			owner->HandlePowerKey(message);
		}
		break;
		case 'PRES':
			owner->EnableKeyMessages();
		break;
		default:
			BLooper::MessageReceived(message);
	}
}


BInputServerDevice *
instantiate_input_device()
{
	return new NetronDisplayDevice();
}

NetronDisplayDevice::NetronDisplayDevice()
	: NetronDisplayControl(0), m_powerOff(false)
{
	SendRawKeys = false;
	SendNormalKeys = false;
	IgnorePowerKey = false;
	if(NetronDisplayControl::InitCheck() == B_NO_ERROR) {
		Looper = new NetronLooper(this);
		Looper->Run();
	}
	else
		Looper = NULL;
}

NetronDisplayDevice::~NetronDisplayDevice()
{
	if(Looper && Looper->Lock())
		Looper->Quit();
}

status_t 
NetronDisplayDevice::InitCheck()
{
	status_t err;
	input_device_ref *device_list[2];
	input_device_ref device = {"NetronDisplay", B_KEYBOARD_DEVICE, NULL};
	err = NetronDisplayControl::InitCheck();
	if(err != B_NO_ERROR)
		return err;
	device_list[0] = &device;
	device_list[1] = NULL;
	
	return RegisterDevices(device_list);
}

status_t 
NetronDisplayDevice::Start(const char *, void *)
{
	SendNormalKeys = true;
	return B_NO_ERROR;
}

status_t 
NetronDisplayDevice::Stop(const char *, void *)
{
	SendNormalKeys = false;
	return B_NO_ERROR;
}

// The kMonitorInformationLength[subcode1] contains the subcode2 value 
// corresponding to subcode1that should be passed
//	sub-code 1	sub-code 2	information
//	00			0x0a		Model name (10 bytes)
//	01			0x08		Serial number (8 bytes)
//	02			0x02		Production week (2 bytes)
//	03			0x04		Production year (4 bytes)

static uint8 kSubCodeMap[] = {0x0a, 0x08, 0x02, 0x04};
static int8 kSubCode1 = 3;
static int8 kSubCode2 = 4;

// power states (see section 2.3.12 of the ECS spec)
static const int8 kPowerStateOn			= 0x02;
static const int8 kPowerStateOff		= 0x03;
static const int8 kPowerStateStandby	= 0x06;

status_t 
NetronDisplayDevice::Control(const char *, void *, uint32 code,
                             BMessage *message)
{
	BAutolock lock(Lock);
	if(code == 'NETR') {
		status_t err;
		BMessage reply_message;
		BMessenger reply_messenger;
		err = message->FindMessage("reply_message", &reply_message);
		if(err != B_NO_ERROR)
			return err;
		err = message->FindMessenger("reply_messenger", &reply_messenger);
		if(err != B_NO_ERROR)
			return err;

		switch(message->what) {
			case 'INIT': {
				RawKeyMessenger = reply_messenger;
				SendRawKeys = true;
				err = B_NO_ERROR;
			} break;

			case 'UNIN': {
				SendRawKeys = false;
				err = B_NO_ERROR;
			} break;
			
			case 'SETD': {
				int32 debug_level;
				err = message->FindInt32("debug_level", &debug_level);
				if(err == B_NO_ERROR)
					SetDebugLevel(debug_level);
			} break;
			
			case 'GETK': {
				uint32 keys;
				keys = GetKeyState();
				reply_message.AddData("data", B_ANY_TYPE, &keys, sizeof(keys));
			} break;

			case 'GETR': {
				int8 reg;
				uint8 val;
				err = message->FindInt8("reg", &reg);
				if(err == B_NO_ERROR)
					err = GetRegister(reg, &val);
				if(err == B_NO_ERROR)
					reply_message.AddData("data", B_ANY_TYPE, &val, 1);
			} break;
	
			case 'GETF': {
				int8 reg;
				uint8 val;
				err = message->FindInt8("reg", &reg);
				if(err == B_NO_ERROR)
					err = GetFactorySetting(reg, &val);
				if(err == B_NO_ERROR)
					reply_message.AddData("data", B_ANY_TYPE, &val, 1);
				break;
			}

			case 'FACB': {	// Restore basic settings
				uint8 packet[] = { 0x81, 0x01, 0x7c, 0x48, 0x00, 0xff };
				err = SendPacket(NETRON_NORMAL_TIMEOUT, packet, sizeof(packet));
				break;
			}
			
			case 'FACG': {	// Restore geometry settings
				uint8 packet[] = { 0x81, 0x01, 0x7c, 0x49, 0x00, 0xff };
				err = SendPacket(NETRON_NORMAL_TIMEOUT, packet, sizeof(packet));
				break;
			}

			case 'SAVE': {	// Save settings to monitor EEPROM
				uint8 packet[] = { 0x81, 0x01, 0x7c, 0x20, 0x00, 0xff };
				err = SendPacket(NETRON_NORMAL_TIMEOUT, packet, sizeof(packet));
				break;
			}
			
			case 'DEGA': {
				uint8 packet[] = { 0x81, 0x01, 0x60, 0x00, 0x01, 0xff };
				err = SendPacket(NETRON_NORMAL_TIMEOUT, packet, sizeof(packet));
				break;
			}
			
			case 'SETR': {
				int8 reg;
				int8 value;
				err = message->FindInt8("reg", &reg);
				if(err == B_NO_ERROR)
					err = message->FindInt8("value", &value);
				if(err == B_NO_ERROR)
					err = SetRegister(reg, value);
			} break;

			case 'SNDR': {
				status_t status;
				err = message->FindInt32("status", &status);
				if(err == B_NO_ERROR)
					if(status == B_NO_ERROR) {
						uint8 packet[] = { 0x90, 0x50, 0xff };
						err = SendRawPacket(packet, sizeof(packet));
					}
					else {
						uint8 packet[] = { 0x90, 0x60, 0x02, 0xff };
						err = SendRawPacket(packet, sizeof(packet));
					}
			} break;
			
			case 'GETM': {	// Get monitor information (see ECS pg 21)
				uint8 packet[] = { 0x81, 0x09, 0x71, 0xFF, 0xFF, 0xff };
				uint8 reply[64];
				ssize_t replylen = sizeof(reply);
				// fill in subcode1 and subcode2 based on what we want to get
				int8 subcode1;
				err = message->FindInt8("subcode", &subcode1);
				if (err == B_NO_ERROR) {
					packet[kSubCode1] = subcode1;
					packet[kSubCode2] = kSubCodeMap[subcode1];
					err = SendPacket(NETRON_NORMAL_TIMEOUT, packet, sizeof(packet),
									 reply, (size_t *)&replylen);
				}
				if(err == B_NO_ERROR) {
					reply_message.AddData("data", B_ANY_TYPE, reply, replylen);
				}
				break;
			}

			case 'GETE': {	// Get ETI (addendum furnished by Kimoto)
				uint8 packet[] = { 0x81, 0x09, 0x60, 0x11, 0xff };
				uint8 reply[64];
				ssize_t replylen = sizeof(reply);
				err = SendPacket(NETRON_NORMAL_TIMEOUT, packet, sizeof(packet),
					             reply, (size_t *)&replylen);
				if(err == B_NO_ERROR) {
					reply_message.AddData("data", B_ANY_TYPE, reply, replylen);
				}
				break;
			}

			case 'SPWR': {	// Set monitor power state (see section 2.3.12 of ECS spec)
				uint8 packet[] = { 0x81, 0x01, 0x00, 0xFF, 0xff };
				uint8 reply[64];
				ssize_t replylen = sizeof(reply);
				// fill in subcode1 based on the state we want to set
				int8 subcode1;
				err = message->FindInt8("subcode", &subcode1);
				if (err == B_NO_ERROR) {
					// only send the command if the subcode is known
					if (subcode1 == kPowerStateOn || subcode1 == kPowerStateOff || subcode1 == kPowerStateStandby) {
						packet[3] = subcode1;
						err = SendPacket(NETRON_NORMAL_TIMEOUT, packet, sizeof(packet),
										 reply, (size_t *)&replylen);

						// set mute if going off; clear mute if going on
						switch (subcode1) {
							case kPowerStateOff:
								SetAudioMuteState(true);
								break;
							case kPowerStateOn:
								if (!UserHasAudioMuted())
									SetAudioMuteState(false);
								break;
						}
					} else {
						err = B_ERROR;
					}
				}
				if(err == B_NO_ERROR) {
					reply_message.AddData("data", B_ANY_TYPE, reply, replylen);
				}
				
				break;
			}
			
			case 'GPWR': {	// Get monitor power state (see section 2.3.12 of ECS spec)
				uint8 packet[] = { 0x81, 0x09, 0x00, 0xff };
				uint8 reply[2];
				ssize_t replylen = sizeof(reply);
				err = SendPacket(NETRON_NORMAL_TIMEOUT, packet, sizeof(packet),
					             reply, (size_t *)&replylen);
				if(err == B_NO_ERROR) {
					reply_message.AddData("data", B_ANY_TYPE, reply, replylen);
				}
				break;
			}
			

			case 'IGNR':
				IgnorePowerKey = true;
				break;
			
			case 'IGNX':
				IgnorePowerKey = false;
				break;

			case 'RAWC': {
				const void  *command;
				ssize_t      commandlen;
				uint8        reply[64];
				ssize_t      replylen;
				err = message->FindData("command", B_ANY_TYPE,
				                        &command, &commandlen);
				if(err == B_NO_ERROR)
					err = message->FindInt32("replylen", &replylen);
				if(err == B_NO_ERROR)
					err = SendPacket(NETRON_NORMAL_TIMEOUT,
					                 (uint8*)command, commandlen,
					                 reply, (size_t *)&replylen);
				if(err == B_NO_ERROR)
					reply_message.AddData("data", B_ANY_TYPE, reply, replylen);

			} break;

			case 'GETS': {
				reply_message.AddData("data", B_ANY_TYPE, &fail_count, 4);
			} break;
			
			default:
				err = B_ERROR;
		}
		reply_message.AddInt32("status", err);
		reply_messenger.SendMessage(&reply_message);
	}
	return B_NO_ERROR;
}

void 
NetronDisplayDevice::HandlePowerKey(BMessage *message)
{
	uint8    current_state[2];
	size_t   reply_len = sizeof(current_state);
	uint8    packet[] = { 0x81, 0x09, 0x00, 0xff };
	uint32   key_code = POWER_KEY;
	status_t err;

	BAutolock lock(Lock);
	err = SendPacket(NETRON_POWER_TIMEOUT, packet, sizeof(packet),
	                 current_state, &reply_len);
	if(err == B_NO_ERROR && reply_len == 2) {
		SERIAL_PRINT(("New powerstate %d, down %d\n", current_state[1],
		         message->what == B_UNMAPPED_KEY_DOWN));
		if (IgnorePowerKey) {
			key_code = 0x102;	/* power on */
		} else {
			switch(current_state[1]) {
				case 2:								/* power on */
					key_code = 0x102;
					m_powerOff = false;
					if (!UserHasAudioMuted())
						SetAudioMuteState(false);
					break; 
				case 3:								/* power off */
					key_code = 0x100;
					m_powerOff = true;
					SetAudioMuteState(true);
					break;
				case 6:								 /* standby */
					key_code = 0x101;
					break;
			}
		}
	}
	SERIAL_PRINT(("send unmapped key message %x, down %d\n", 0x200000 + key_code,
	         message->what == B_UNMAPPED_KEY_DOWN));
	message->AddInt32("key", 0x200000 + key_code);
	EnqueueMessage(new BMessage(*message));
}

status_t 
NetronDisplayDevice::KeyPressed(uint8 key_code, bool down)
{
	if(SendRawKeys) {
		BMessage message('KEYP');
		message.AddInt8("keycode", key_code);
		message.AddBool("down", down);
		RawKeyMessenger.SendMessage(&message);
	}
	else if(SendNormalKeys) {
		BMessage *message;
		if(down)
			message = new BMessage(B_UNMAPPED_KEY_DOWN);
		else
			message = new BMessage(B_UNMAPPED_KEY_UP);
		message->AddInt32("when", system_time());

		if(key_code == POWER_KEY) {
			Looper->PostMessage(message);
			delete message;
		}
		else {
			/* The WEB, MEDIA, and EMAIL keys turn monitor power back on,
			** so unmute audio if appropriate.
			*/
			if (!down && (key_code == WEB_KEY || key_code == MEDIA_KEY
					|| key_code == EMAIL_KEY)) {
				if (!UserHasAudioMuted())
					SetAudioMuteState(false);

				// also, update cached power state
				{
					BAutolock lock(Lock);
					if (m_powerOff)
						m_powerOff = false;
				}
				
				// fall through to the code that enqueues the event msg
			}
			
			// The volume buttons should be ignored if monitor power is off.
			if (key_code == VOLUME_UP_KEY || key_code == VOLUME_DOWN_KEY) {
				BAutolock lock(Lock);
				if (m_powerOff) {
					// return without enqueuing
					return B_NO_ERROR;
				}
			}
			
			message->AddInt32("key", 0x200000 + key_code);
			EnqueueMessage(message);
		}
	}
	return B_NO_ERROR;
}

status_t 
NetronDisplayDevice::TestPattern(uint8 pattern)
{
	if(SendRawKeys) {
		BMessage message('TPAT');
		message.AddInt8("pattern", pattern);
		RawKeyMessenger.SendMessage(&message);
	}
	return B_NO_ERROR;
}

status_t 
NetronDisplayDevice::PowerOnReset()
{
	BMessage message('PRES');
	Looper->PostMessage(&message);
	return B_NO_ERROR;
}

status_t
NetronDisplayDevice::SetAudioMuteState(bool mute)
{
	status_t err;
	
	err = mini_adjust_volume(miniMainOut, 0, 0, mute ? VOL_SET_MUTE : VOL_CLEAR_MUTE);
	SERIAL_PRINT(("NetronDisplayDevice: mini_adjust_volume(%d) "
		"returned %s\n", mute, strerror(err)));
		
	return err;
}

bool
NetronDisplayDevice::UserHasAudioMuted(void)
{
	status_t err;
	float volL;
	bool mute;
	
	err = mini_get_volume(miniMainOut, &volL, NULL, &mute);
	SERIAL_PRINT(("NetronDisplayDevice: mini_get_volume() said:\n"
		"volL: %f\tmute: %d\n"
		"and returned %s\n", volL, mute, strerror(err)));

	if (err == B_OK && mute && volL == 0.0)
		return true;
	else
		return false;
}
