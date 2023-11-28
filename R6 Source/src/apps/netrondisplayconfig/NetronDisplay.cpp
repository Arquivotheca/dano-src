#include "NetronDisplay.h"
#include <Input.h>
#include <stdio.h>

#include <Looper.h>

class MessageLooper : public BLooper {
	public:
		virtual void MessageReceived(BMessage *message);
		NetronDisplay *display;
};
		

NetronDisplay::NetronDisplay(int debug_level)
{
	MessageLooper *msglooper;
	Looper = NULL;
	init_status = B_ERROR;

	InputDevice = find_input_device("NetronDisplay");
	if(InputDevice == NULL) {
		printf("NetronDisplay-- find_input_device returned NULL\n");
		return;
	}
	msglooper = new MessageLooper;
	if(msglooper == NULL) {
		printf("NetronDisplay-- MessageLooper is NULL\n");
		return;
	}
	msglooper->display = this;
	Looper = msglooper;
	if(Looper->Run() < B_NO_ERROR) {
		printf("NetronDisplay-- Looper->Run() is ERROR\n");
		return;
	}
	if(debug_level >= 0) {
		BMessage msg('SETD');
		msg.AddInt32("debug_level", debug_level);
		SendCommand(&msg);
	}
	init_status = B_OK;
}


NetronDisplay::~NetronDisplay()
{
	if(Looper) {
		Looper->Lock();
		Looper->Quit();
	}
	delete InputDevice;
}

static void
process_reply_message(BMessage *message)
{
	sem_id    reply_sem;
	status_t  status = B_ERROR;
	void     *dest_data_ptr;
	size_t    data_len;
//	uint32    request_type; ?unused? KCC

	if(message->FindInt32("reply_sem", &reply_sem) != B_NO_ERROR)
		return;

	message->FindInt32("status", &status);
	if(status != B_NO_ERROR)
		goto err;

	if(message->FindInt32("reply_data_len", (int32*)&data_len) != B_NO_ERROR)
		goto err;

	if(message->FindPointer("reply_data_ptr", &dest_data_ptr) != B_NO_ERROR)
		goto err;

	if(data_len != 0) {
		const void *src_data_ptr;
		ssize_t src_data_len;
		if(message->FindData("data", B_ANY_TYPE, &src_data_ptr,
		                     &src_data_len) != B_NO_ERROR)
			goto err;
		if(src_data_len != (ssize_t)data_len)
			goto err;
		memcpy(dest_data_ptr, src_data_ptr, data_len);
	}
	release_sem(reply_sem);
	return;
err:
	delete_sem(reply_sem);
}

void 
MessageLooper::MessageReceived(BMessage *message)
{
	//printf("NetronDisplay::MessageReceived\n");
	//message->PrintToStream();
	switch(message->what) {
		case 'KEYP': {
			int8 key_code;
			bool down;
			if(message->FindInt8("keycode", &key_code) == B_NO_ERROR &&
			   message->FindBool("down", &down) == B_NO_ERROR)
				display->KeyPressed(key_code, down);
			} break;
		case 'TPAT': {
			int8 pattern;
			if(message->FindInt8("pattern", &pattern) == B_NO_ERROR)
				display->TestPattern(pattern);
			} break;
		case 'REPL': {
			process_reply_message(message);
			} break;
		default:
			BLooper::MessageReceived(message);
	}
}

status_t 
NetronDisplay::SendCommand(BMessage *msg, void *reply_data,
                           size_t reply_data_len)
{
	status_t   err;
	sem_id     reply_sem;
	BMessage  reply_msg('REPL');

	reply_sem = create_sem(0, "NetronDisplayGetRegisterReply");
	if(reply_sem < 0) {
		err = reply_sem;
		goto err0;
	}
	reply_msg.AddInt32("request_type", msg->what);
	reply_msg.AddInt32("reply_sem", reply_sem);
	reply_msg.AddPointer("reply_data_ptr", reply_data);
	reply_msg.AddInt32("reply_data_len", reply_data_len);
	msg->AddMessage("reply_message", &reply_msg);
	msg->AddMessenger("reply_messenger", BMessenger(NULL, Looper));
	
	err = InputDevice->Control('NETR', msg);
	if(err != B_NO_ERROR)
		goto err1;
	//err = acquire_sem_etc(reply_sem, 1, B_RELATIVE_TIMEOUT, 1000000); /* debug only, timeout is not safe */
	err = acquire_sem(reply_sem);
err1:
	delete_sem(reply_sem);
err0:
	return err;
}


status_t 
NetronDisplay::GetRegister(uint8 reg, uint8 *value)
{
	BMessage message('GETR');
	message.AddInt8("reg", reg);
	return SendCommand(&message, value, 1);
}

status_t 
NetronDisplay::SetRegister(uint8 reg, uint8 value)
{
	BMessage message('SETR');
	message.AddInt8("reg", reg);
	message.AddInt8("value", value);
	return SendCommand(&message);
}

status_t 
NetronDisplay::GetKeyState(uint32 *keys)
{
	BMessage message('GETK');
	return SendCommand(&message, keys, 4);
}


status_t 
NetronDisplay::SendReply(status_t status)
{
	BMessage message('SNDR');
	message.AddInt32("status", status);
	return SendCommand(&message);
}

status_t 
NetronDisplay::GetStats(uint32 *fail_count)
{
	BMessage message('GETS');
	return SendCommand(&message, fail_count, 4);
}

status_t
NetronDisplay::RecallFactorySettings(bool basic, bool geometry)
{
	status_t err1 = B_NO_ERROR;
	status_t err2 = B_NO_ERROR;
	if(basic) {
		BMessage message('FACB');
		err1 = SendCommand(&message);
	}
	if(geometry) {
		BMessage message('FACG');
		err2 = SendCommand(&message);
	}
	return err1 != B_NO_ERROR ? err1 : err2;
}

status_t
NetronDisplay::ActivateDegauss()
{
	BMessage message('DEGA');
	return SendCommand(&message);
}

status_t
NetronDisplay::SaveSettings()
{
	BMessage message('SAVE');
	return SendCommand(&message);
}

status_t
NetronDisplay::GetFactorySetting(uint8 reg, uint8 *value)
{
	BMessage message('GETF');
	message.AddInt8("reg", reg);
	return SendCommand(&message, value, 1);
}

status_t 
NetronDisplay::KeyPressed(uint8 key_code, bool down)
{
	(void)key_code;
	(void)down;
	
	return B_NO_ERROR;
}

status_t 
NetronDisplay::TestPattern(uint8 pattern)
{
	return B_NO_ERROR;
}

