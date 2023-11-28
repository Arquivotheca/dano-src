/******************************************************************************
/
/	File:			Input.cpp
/
/	Copyright 1998, Be Incorporated, All Rights Reserved.
/
******************************************************************************/

#include <Input.h>
#include <input_server_private.h>
#include <List.h>
#include <Message.h>

#include <malloc.h>
#include <string.h>


const char *kInputServerSig = "application/x-vnd.Be-input_server";



BInputDevice*
find_input_device(
	const char *name)
{
	BMessage reply;
	BMessage command(IS_FIND_DEVICES);
	command.AddString(IS_DEVICE_NAME, name);

	if (_control_input_server_(&command, &reply) != B_NO_ERROR)
		return (NULL);

	BInputDevice *device = new BInputDevice();
	device->set_name_and_type(reply.FindString(IS_DEVICE_NAME), 
							  (input_device_type)reply.FindInt32(IS_DEVICE_TYPE));
	
	return (device);	
}


status_t
get_input_devices(
	BList	*list)
{
	list->MakeEmpty();	

	BMessage reply;
	BMessage command(IS_FIND_DEVICES);

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_NO_ERROR)
		return (err);
	
	type_code	theType = B_STRING_TYPE;
	int32		numDevices = 0;
	if (reply.GetInfo(IS_DEVICE_NAME, &theType, &numDevices) != B_NO_ERROR)
		return (err);

	for (int32 i = 0; i < numDevices; i++) {
		BInputDevice *device = new BInputDevice();
		device->set_name_and_type(reply.FindString(IS_DEVICE_NAME, i),
								  (input_device_type)reply.FindInt32(IS_DEVICE_TYPE, i));
		list->AddItem(device);
	}

	return (B_NO_ERROR);
}


status_t
watch_input_devices(
	BMessenger	target,
	bool		start)
{
	BMessage reply;
	BMessage command(IS_WATCH_DEVICES);
	command.AddMessenger(IS_WATCH_TARGET, target);
	command.AddBool(IS_WATCH_START, start);

	return (_control_input_server_(&command, &reply));
}


status_t
_control_input_server_(
	BMessage	*command,
	BMessage	*reply)
{
	static BMessenger sMessenger;

	if (!sMessenger.IsValid())
		sMessenger = BMessenger(kInputServerSig);

	status_t err = sMessenger.SendMessage(command, reply);
	if (err == B_NO_ERROR) {
		if (reply->FindInt32(IS_STATUS, &err) != B_NO_ERROR)
			return (B_ERROR);
	}

	return (err);	
}


BInputDevice::BInputDevice()
{
	fName = NULL;
	fType = B_UNDEFINED_DEVICE;
}


BInputDevice::~BInputDevice()
{
	free(fName);
}


const char*
BInputDevice::Name() const
{
	return (fName);
}


input_device_type
BInputDevice::Type() const
{
	return (fType);
}


bool
BInputDevice::IsRunning() const
{
	if (fName == NULL)
		return (false);

	BMessage reply;
	BMessage command(IS_DEVICE_RUNNING);
	command.AddString(IS_DEVICE_NAME, fName);

	return (_control_input_server_(&command, &reply) == B_NO_ERROR);
}


status_t
BInputDevice::Start()
{
	if (fName == NULL)
		return (B_ERROR);

	BMessage reply;
	BMessage command(IS_START_DEVICES);
	command.AddString(IS_DEVICE_NAME, fName);

	return (_control_input_server_(&command, &reply));
}


status_t
BInputDevice::Stop()
{
	if (fName == NULL)
		return (B_ERROR);

	BMessage reply;
	BMessage command(IS_STOP_DEVICES);
	command.AddString(IS_DEVICE_NAME, fName);

	return (_control_input_server_(&command, &reply));
}


status_t
BInputDevice::Control(
	uint32		code,
	BMessage	*message)
{
	if (fName == NULL)
		return (B_ERROR);

	BMessage reply;
	BMessage command(IS_CONTROL_DEVICES);
	command.AddString(IS_DEVICE_NAME, fName);
	command.AddInt32(IS_CONTROL_CODE, code);
	command.AddMessage(IS_CONTROL_MESSAGE, message);
	message->MakeEmpty();

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_NO_ERROR)
		return (err);

	reply.FindMessage(IS_CONTROL_MESSAGE, message);

	return (B_NO_ERROR);
}


status_t
BInputDevice::Start(
	input_device_type	type)
{
	BMessage reply;
	BMessage command(IS_START_DEVICES);
	command.AddInt32(IS_DEVICE_TYPE, type);

	return (_control_input_server_(&command, &reply));
}


status_t
BInputDevice::Stop(
	input_device_type	type)
{
	BMessage reply;
	BMessage command(IS_STOP_DEVICES);
	command.AddInt32(IS_DEVICE_TYPE, type);

	return (_control_input_server_(&command, &reply));
}


status_t
BInputDevice::Control(
	input_device_type	type,
	uint32				code,
	BMessage			*message)
{
	BMessage reply;
	BMessage command(IS_CONTROL_DEVICES);
	command.AddInt32(IS_DEVICE_TYPE, type);
	command.AddInt32(IS_CONTROL_CODE, code);
	command.AddMessage(IS_CONTROL_MESSAGE, message);
	message->MakeEmpty();

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_NO_ERROR)
		return (err);

	reply.FindMessage(IS_CONTROL_MESSAGE, message);

	return (B_NO_ERROR);
}


void
BInputDevice::set_name_and_type(
	const char			*name,
	input_device_type	type)
{
	free(fName);
	fName = NULL;

	if (name != NULL)
		fName = strdup(name);
	fType = type;
}

