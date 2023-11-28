#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <USB_printer.h>
#include <Message.h>
#include <Entry.h>
#include <Directory.h>
#include <Path.h>
#include <String.h>

#include "usb_print.h"


// ------------------------------------------------------------------------

static const char *kTransportName = "USB Port";

// ------------------------------------------------------------------------

extern "C" BTransportAddOn *instantiate_transport_addon(BNode *printer_file)
{
	return (BTransportAddOn *)(new USBPrint(printer_file));
}

USBPrint::USBPrint(BNode *printer)
	: BTransportAddOn(printer),
	ffd(-1),
	fDeviceStatus(B_NO_INIT),
	fAttributes(B_TRANSPORT_IS_READABLE | B_TRAMSPORT_IS_HOT_PLUG | B_TRANSPORT_IS_USB),
	fDeviceID(NULL),
	fCurrentAccessMode(-1)
{
	if (printer == NULL)
	{ // Special case. We don't actually need to communicate with the printer
		fDeviceStatus = B_OK;
		return;
	}

	char dev_name[B_OS_NAME_LENGTH];
	if (printer->ReadAttr(M_ATTR_TRANSPORT_ADDR, B_STRING_TYPE, 0, dev_name, B_OS_NAME_LENGTH) <= 0)
		return;

	// Find the printer corresponding to this dev_name
	if (find_device_path(dev_name, &fDevicePath) != B_OK)
		return;

	// Open the device for READING only
	SetAccessMode(O_RDONLY);
}

USBPrint::~USBPrint()
{
	if (ffd > 0)
		close(ffd);
	free(fDeviceID);
}

status_t USBPrint::SetAccessMode(int access_mode)
{
	if ((access_mode != O_RDONLY) && (access_mode != O_WRONLY) && (access_mode != O_RDWR))
		return B_BAD_VALUE;

	if (access_mode == fCurrentAccessMode)
		return BTransportAddOn::SetAccessMode(fCurrentAccessMode);

	close(ffd);
	fCurrentAccessMode = -1;
	ffd = open(fDevicePath.Path(), access_mode);
	if (ffd < 0)
	{ // We were unable to reopen the device
		fDeviceStatus = (status_t)ffd;
	}
	else
	{ // New access mode accepted
		fDeviceStatus = B_OK;
		fCurrentAccessMode = access_mode;
	}

	if (fDeviceStatus == B_OK)
		SetAccessMode(fCurrentAccessMode);

	return fDeviceStatus;
}

ssize_t USBPrint::Read(void *buffer, size_t size)
{
	const ssize_t s = read(ffd, buffer, size);
	return (s < 0) ? errno : s;
}

ssize_t USBPrint::Write(const void *buffer, size_t size)
{
	const ssize_t s = write(ffd, buffer, size);
	return (s < 0) ? errno : s;
}


status_t USBPrint::GetDeviceID(int ffd, BString& device_id) const
{
	status_t err;
	char devID[USB_PRINTER_DEVICE_ID_LENGTH];
	if ((err = ioctl(ffd, USB_PRINTER_GET_DEVICE_ID, devID, 0)) != B_OK)
		return err;
	device_id = devID;
	return B_OK;
}

status_t USBPrint::GetDeviceID(BString& device_id) const
{
	return GetDeviceID(ffd, device_id);
}

status_t USBPrint::GetPortStatus(uint8 *status) const
{
	status_t err;
	if (status == NULL)
		return B_BAD_VALUE;
	return ioctl(ffd, USB_PRINTER_GET_PORT_STATUS, status, 0);
}

status_t USBPrint::SoftReset() const
{
	return ioctl(ffd, USB_PRINTER_SOFT_RESET, NULL, 0);
}

const char *USBPrint::Name() const
{
	return kTransportName;
}

status_t USBPrint::ProbePrinters(BMessage *printers)
{
	// First some informations on this transport
	printers->MakeEmpty();
	printers->AddString(B_TRANSPORT_NAME, Name());
	printers->AddInt32(B_TRANSPORT_ATTRIBUTES, GetAttributes());
	return probe_printers(USB_DEVICE_DIRECTORY, printers);
}

status_t USBPrint::find_device_path(const char *dev_name, BPath *devpath)
{
	devpath->Unset();

	// Then look for connected devices
	BEntry entry;
	BDirectory dir(USB_DEVICE_DIRECTORY);
	while ((dir.GetNextEntry(&entry) == B_OK) && (devpath->InitCheck() != B_OK))
	{
		BPath path;
		entry.GetPath(&path);
		int ffd = open(path.Path(), O_RDONLY);
		if (ffd >= 0)
		{
			char devID[256];
			BString deviceID(B_EMPTY_STRING);
			GetDeviceID(ffd, deviceID);
			strcpy(devID, deviceID.String());

			char *saveptr1;
			char *token = strtok_r(devID, ";", &saveptr1);
			if (token != NULL)
			{
				BString smfg;
				BString smdl;
				do
				{ // Break the DEVICE_ID string into tokens
					char *a_token = strdup(token);
					char *saveptr2;
					char *key = strtok_r(a_token, ":", &saveptr2);
					char *value = strtok_r(NULL, ":", &saveptr2);
					if ((key) && (value))
					{
						if (!strcmp(key, "MFG"))	smfg = value;
						if (!strcmp(key, "MDL"))	smdl = value;
					}
					free(a_token);
				} while ((token = strtok_r(NULL, ";", &saveptr1)) != NULL);					
				BString unique;
				unique << smfg << "/" << smdl;
				if (unique == dev_name)
					*devpath = path;
			}
			close(ffd);
		}
	}
	
	return devpath->InitCheck();
}

status_t USBPrint::probe_printers(const char *device_directory, BMessage *inout)
{
	// Then look for connected devices
	BEntry entry;
	BDirectory dir(device_directory);
	while (dir.GetNextEntry(&entry) == B_OK)
	{
		BPath path;
		entry.GetPath(&path);
		int ffd = open(path.Path(), O_RDONLY);
		if (ffd >= 0)
		{
			char devID[256];
			BString deviceID(B_EMPTY_STRING);
			if ((GetDeviceID(ffd, deviceID) != B_OK) || (deviceID.String() == B_EMPTY_STRING))
			{
				close(ffd);
				continue;
			}
			
			strcpy(devID, deviceID.String());
	
			BMessage reply;
			reply.AddString(B_TRANSPORT_DEV_PATH, path.Path());
			reply.AddString(B_TRANSPORT_DEV_ID, devID);
			char *saveptr1;
			char *token = strtok_r(devID, ";", &saveptr1);
			if (token != NULL)
			{
				BString smfg;
				BString smdl;
				BString sdes;
				do
				{ // Break the DEVICE_ID string into tokens
					char *a_token = strdup(token);
					char *saveptr2;
					char *key = strtok_r(a_token, ":", &saveptr2);
					char *value = strtok_r(NULL, ":", &saveptr2);
					if ((key) && (value))
					{ // Record <DEVID:key:, value> in the reply BMessage
						char key_name[32];
						sprintf(key_name, "DEVID:%s:", key);
						reply.AddString(key_name, value);
						if (!strcmp(key, "MFG"))				smfg = value;
						if (!strcmp(key, "MDL"))				smdl = value;
						if (!strcmp(key, "DES"))				sdes = value;
						else if (!strcmp(key, "DESCRIPTION"))	sdes = value;
					}
					free(a_token);
				} while ((token = strtok_r(NULL, ";", &saveptr1)) != NULL);					
				BString unique;
				unique << smfg << "/" << smdl;
				reply.AddString(B_TRANSPORT_DEV_UNIQUE_NAME, unique.String());
				
				BString port;
				if (sdes != B_EMPTY_STRING)		port << sdes;
				else							port << smfg << " " << smdl;
				if (port != B_EMPTY_STRING)
					reply.AddString(B_TRANSPORT_DEV_DESCRIPTION, port.String());
			}

			inout->AddMessage(B_TRANSPORT_MSG_PRINTERS, &reply);
			close(ffd);
		}
	}
	return B_OK;
}

status_t USBPrint::Perform(int32 selector, void *data)
{
	if (selector == USB_PRINTER_SET_STATUS_CAPABILITY)
		return ioctl(ffd, USB_PRINTER_SET_STATUS_CAPABILITY, data, 0);
	return BTransportAddOn::Perform(selector, data);
}

// -------------------------------------------------------------
#pragma mark -
// For compatibility with old API

extern "C" _EXPORT BDataIO* init_transport(BMessage *msg, BMessage *reply);
extern "C" _EXPORT bool exit_transport();

static USBPrint *usb_port = NULL;
BDataIO* init_transport(BMessage *msg, BMessage *reply)
{
	const char *printer_file;
	printer_file = msg->FindString("printer_file");
	BDirectory dir(printer_file);
	if (dir.InitCheck() != B_OK)
		return NULL;
	usb_port = new USBPrint(&dir);
	if (usb_port->InitCheck() == B_OK)
	{
		reply->what = 'okok';
		reply->AddBool("bidirectional", usb_port->IsReadable());
		reply->AddString(B_TRANSPORT_NAME, usb_port->Name());
		reply->AddInt32(B_TRANSPORT_ATTRIBUTES, (int32)usb_port->GetAttributes());
		return usb_port;
	}
	delete usb_port;
	return (usb_port = NULL);
}


bool exit_transport()
{
	delete usb_port;
	usb_port = NULL;
	return true;
}
