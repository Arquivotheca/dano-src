#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <parallel_driver.h>
#include <Message.h>
#include <Entry.h>
#include <Directory.h>
#include <Path.h>
#include <String.h>

#include "parallel_print.h"

// ------------------------------------------------------------------------

static const char *kTransportName = "Parallel Port";

// ------------------------------------------------------------------------

extern "C" BTransportAddOn *instantiate_transport_addon(BNode *printer_file)
{
	return (BTransportAddOn *)(new ParallelPrint(printer_file));
}

ParallelPrint::ParallelPrint(BNode *printer)
	: BTransportAddOn(printer),
	ffd(-1),
	fDeviceStatus(B_NO_INIT),
	fAttributes(B_TRANSPORT_IS_READABLE),
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
	fDevicePath.SetTo(PAR_DEVICE_DIRECTORY);
	fDevicePath.Append(dev_name);

	// Open the device for READING only
	SetAccessMode(O_RDONLY);
}

ParallelPrint::~ParallelPrint()
{
	if (ffd > 0)
		close(ffd);
	free(fDeviceID);
}

status_t ParallelPrint::SetAccessMode(int access_mode)
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

ssize_t ParallelPrint::Read(void *buffer, size_t size)
{
	const ssize_t s = read(ffd, buffer, size);
	return (s < 0) ? errno : s;
}

ssize_t ParallelPrint::Write(const void *buffer, size_t size)
{
	const ssize_t s = write(ffd, buffer, size);
	return (s < 0) ? errno : s;
}



status_t ParallelPrint::GetDeviceID(BString& device_id) const
{
	return GetDeviceID(ffd, device_id);
}

status_t ParallelPrint::GetDeviceID(int ffd, BString& device_id) const
{
	char devID[256];
	int ext_byte = 0x4;
	if ((ioctl(ffd, PAR_REQUEST_DEVICE_ID, (void *)&ext_byte, 0) == B_OK) && (read(ffd, &devID, 256) > 2))
	{
		device_id = devID+2;
		return B_OK;
	}
	return B_ERROR;
}

status_t ParallelPrint::GetPortStatus(uint8 *status) const
{
	return ioctl(ffd, PAR_STATUS, status, 0);
}

status_t ParallelPrint::SoftReset() const
{
	return ioctl(ffd, PAR_RESET, 0, 0);
}


const char *ParallelPrint::Name() const
{
	return kTransportName;
}

status_t ParallelPrint::ProbePrinters(BMessage *printers)
{
	// First some informations on this transport
	printers->MakeEmpty();
	printers->AddString(B_TRANSPORT_NAME, Name());
	printers->AddInt32(B_TRANSPORT_ATTRIBUTES, (int32)GetAttributes());
	return probe_printers(PAR_DEVICE_DIRECTORY, printers);
}


status_t ParallelPrint::probe_printers(const char *device_directory, BMessage *inout)
{
	// First some informations on this transport
	BEntry entry;
	BDirectory dir(device_directory);
	while (dir.GetNextEntry(&entry) == B_OK)
	{
		BPath path;
		entry.GetPath(&path);
		int ffd = open(path.Path(), O_RDONLY);
		if (ffd >= 0)
		{
			BMessage reply;
			reply.AddString(B_TRANSPORT_DEV_UNIQUE_NAME, path.Leaf());
			reply.AddString(B_TRANSPORT_DEV_PATH, path.Path());

			char buffer[256];
			int ext_byte = 0x4;
			if ((ioctl(ffd, PAR_REQUEST_DEVICE_ID, (void *)&ext_byte, 0) == B_OK) &&
				(read(ffd, &buffer, 256) > 2))
			{
				char *devID = buffer + 2;
				reply.AddString(B_TRANSPORT_DEV_ID, devID);
				char *saveptr1;
				char *token = strtok_r(devID, ";", &saveptr1);
				if (token != NULL)
				{
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
						}
						free(a_token);
					} while ((token = strtok_r(NULL, ";", &saveptr1)) != NULL);
				}
			}
			inout->AddMessage(B_TRANSPORT_MSG_PRINTERS, &reply);
			close(ffd);
		}
	}
	return B_OK;
}


// -------------------------------------------------------------
#pragma mark -
// For compatibility with old API

extern "C" _EXPORT BDataIO* init_transport(BMessage *msg, BMessage *reply);
extern "C" _EXPORT bool exit_transport();

static ParallelPrint *pport = NULL;
BDataIO* init_transport(BMessage *msg, BMessage *reply)
{
	const char *printer_file;
	printer_file = msg->FindString("printer_file");
	BDirectory dir(printer_file);
	if (dir.InitCheck() != B_OK)
		return NULL;
	pport = new ParallelPrint(&dir);
	if (pport->InitCheck() == B_OK)
	{
		reply->what = 'okok';
		reply->AddBool("bidirectional", pport->IsReadable());
		reply->AddString(B_TRANSPORT_NAME, pport->Name());
		reply->AddInt32(B_TRANSPORT_ATTRIBUTES, (int32)pport->GetAttributes());
		return pport;
	}
	delete pport;
	return (pport = NULL);
}

bool exit_transport()
{
	delete pport;
	pport = NULL;
	return true;
}
