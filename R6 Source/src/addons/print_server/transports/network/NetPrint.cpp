#include "NetPrint.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <String.h>
#include <Message.h>
#include <Directory.h>
#include <printclient.h>
#include <stdlib.h>
#include <Entry.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Messenger.h>

static const char *kTransportName = "Network";

NetPrint::NetPrint(BNode *printer)
	: BTransportAddOn(printer),
	ffd(-1),
	fDeviceStatus(B_NO_INIT),
	fAttributes(0),
	fDeviceID(NULL)
{
	if (printer == NULL)
	{ // Special case. We don't actually need to communicate with the printer
		fDeviceStatus = B_OK;
		return;
	}

	char dev_name[B_OS_NAME_LENGTH];
	if (printer->ReadAttr(M_ATTR_TRANSPORT_ADDR, B_STRING_TYPE, 0, dev_name, B_OS_NAME_LENGTH) <= 0)
		return;

	BMessenger messenger("application/x-vnd.Be-dahood");
	hoodEnabled = messenger.IsValid();
	if (hoodEnabled)
	{
		ffd = open(dev_name, B_READ_ONLY);
	}
	else
	{
		char tmp[255];
		int count = 0;
		BEntry entry;
		BPath outPath, tempDir;
		find_directory(B_COMMON_TEMP_DIRECTORY, &tempDir);
		while (true)
		{
			outPath = tempDir;
			sprintf(tmp, "atalk_output_%d", count++);
			outPath.Append(tmp);
			entry.SetTo(outPath.Path());
			if (!entry.Exists())
				break;
		}
		ffd = open(outPath.Path(), O_RDWR | O_CREAT | O_TRUNC);
		backendSpool = outPath.Path();				
	}
}

NetPrint::~NetPrint()
{
	if (ffd > 0)
		close(ffd);
	free(fDeviceID);

	if (hoodEnabled == false)
		unlink(backendSpool.String());
}

ssize_t NetPrint::Write(const void* buffer, size_t numBytes)
{
	return write(ffd, buffer, numBytes);
}

ssize_t NetPrint::Read(void* buffer, size_t numBytes)
{
	return B_NOT_ALLOWED;
}

const char *NetPrint::Name() const
{
	return kTransportName;
}

status_t NetPrint::ProbePrinters(BMessage *printers)
{
	// First some informations on this transport
	printers->MakeEmpty();
	printers->AddString(B_TRANSPORT_NAME, Name());
	printers->AddInt32(B_TRANSPORT_ATTRIBUTES, (int32)GetAttributes());
	return B_OK;
}


// -------------------------------------------------------------
#pragma mark -
// For compatibility with old API

extern "C" _EXPORT BDataIO* init_transport(BMessage *msg, BMessage *reply);
extern "C" _EXPORT bool exit_transport();

static NetPrint *nport = NULL;
BDataIO* init_transport(BMessage *msg, BMessage *reply)
{
	const char *printer_file;
	printer_file = msg->FindString("printer_file");
	BDirectory dir(printer_file);
	if (dir.InitCheck() != B_OK)
		return NULL;
	nport = new NetPrint(&dir);
	if (nport->InitCheck() == B_OK)
	{
		reply->what = 'okok';
		reply->AddBool("bidirectional", nport->IsReadable());
		reply->AddString(B_TRANSPORT_NAME, nport->Name());
		reply->AddInt32(B_TRANSPORT_ATTRIBUTES, (int32)nport->GetAttributes());
		return nport;
	}
	delete nport;
	return (nport = NULL);
}

bool exit_transport()
{
	delete nport;
	nport = NULL;
	return true;
}
