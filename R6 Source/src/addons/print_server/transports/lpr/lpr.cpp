#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <Message.h>
#include <Entry.h>
#include <Directory.h>
#include <Path.h>
#include <String.h>


#include "lpr.h"

#define HOST_ADDRESS	"lpr:address"
#define QUEUE_NAME		"lpr:Qname"
#define USER_NAME		"lpr:user"

// ------------------------------------------------------------------------

static const char *kTransportName = "Line Printer (Network)";

// ------------------------------------------------------------------------

extern "C" BTransportAddOn *instantiate_transport_addon(BNode *printer_file)
{
	return (BTransportAddOn *)(new LPRPrint(printer_file));
}

LPRPrint::LPRPrint(BNode *printer)
	: BTransportAddOn(printer),
	fPrinterNode(printer),
	fDeviceStatus(B_OK),
	fAttributes(B_TRANSPORT_IS_SHARABLE | B_TRANSPORT_IS_NETWORK)
{
}

LPRPrint::~LPRPrint()
{
	// Make sure to remove the temporary file
	if (fJobName.Length())
	{
		BEntry entry(fSpoolFilePath.Path());
		entry.Remove();
	}
}

status_t LPRPrint::BeginPrintjob(const BMessage& jobInfos)
{
	size_t read;
	const size_t l = 256;
	char address[256], queue[256], user[256];
	if ((read = fPrinterNode->ReadAttr(HOST_ADDRESS, B_STRING_TYPE, 0, address, l-1)) <= 0) return B_ERROR;
	address[read] = 0;
	if ((read = fPrinterNode->ReadAttr(QUEUE_NAME, B_STRING_TYPE, 0, queue, l-1)) <= 0) return B_ERROR;
	queue[read] = 0;	
	if ((read = fPrinterNode->ReadAttr(USER_NAME, B_STRING_TYPE, 0, user, l-1)) <= 0) return B_ERROR;
	user[read] = 0;

	fLpdAddr = BNetAddress(address, 515);
	fQueueName = queue;
	fUserName = user;

	// Create Spool File
	const char *jobName = jobInfos.FindString(B_JOB_NAME);
	if (jobName == NULL)
		jobName = "unknown";
	fJobName = jobName;
	
	BString spoolPath = fJobName;
	spoolPath << "@" << user << "@" << address << "@" << (uint32)system_time();
	fSpoolFilePath.SetTo("/tmp", spoolPath.String());

	fSpoolFile.SetTo(fSpoolFilePath.Path(), B_READ_WRITE | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	if (fSpoolFile.InitCheck() != B_OK)
		return fSpoolFile.InitCheck();

	return B_OK;
}

status_t LPRPrint::EndPrintjob()
{
	BNetDebug::Enable(false);

	// Receive Spool job
	char *buffer = NULL;
	BNetEndpoint endPoint;
	status_t result = ReceiveAPrinterJob(endPoint);
	if (result != B_OK)
		return result;

	// job number
	unsigned int jobNumber = system_time() % 1000;
	char jobNumberString[8];
	sprintf(jobNumberString, "%03u", jobNumber % 1000);

	// Get the hostname
	char hostName[32];
	memset(hostName, 0, sizeof(hostName));
	if (gethostname(hostName, sizeof(hostName)) != B_OK)
		strcpy(hostName, "BeOS");

	{ // Send control file		
		BString cfAHeader;
		BString cfAFile;

		cfAFile << "H" << hostName << "\n";
		cfAFile << "P" << fUserName << "\n";
		cfAFile << "J" << fJobName << "\n";
		cfAFile << "l" << "dfA" << jobNumberString << hostName << "\n";
		cfAHeader << cfAFile.Length() << " " << "cfA" << jobNumberString << hostName << "\n";

		BNetBuffer cfABuffer;
		cfABuffer.AppendUint8(2);
		cfABuffer.AppendData(cfAHeader.String(), cfAHeader.Length());
		endPoint.Send(cfABuffer);
		if (((result = endPoint.Error()) != B_OK) || ((result = Acknowledge(endPoint)) != B_OK))
			goto exit;

		cfABuffer = BNetBuffer();
		cfABuffer.AppendString(cfAFile.String());
		endPoint.Send(cfABuffer);
		if (((result = endPoint.Error()) != B_OK) || ((result = Acknowledge(endPoint)) != B_OK))
			goto exit;
	}

	{ // Send data file
		fSpoolFile.Seek(0, SEEK_SET);
		off_t size;
		fSpoolFile.GetSize(&size);
		
		BString dfAHeader;
		dfAHeader << size << " " << "dfA" << jobNumberString << hostName << "\n";

		BNetBuffer dfABuffer;
		dfABuffer.AppendUint8(3);
		dfABuffer.AppendData(dfAHeader.String(), dfAHeader.Length());
		endPoint.Send(dfABuffer);
		if (((result = endPoint.Error()) != B_OK) || ((result = Acknowledge(endPoint)) != B_OK))
			goto exit;

		size_t length = 1024*1024;
		buffer = (char *)malloc(length);
		if (buffer == NULL) {
			length = 1024;
			buffer = (char *)malloc(length);
			if (buffer == NULL) {
				result = B_NO_MEMORY;
				goto exit;
			}
		}
		
		size_t read;
		do {
			if ((read = fSpoolFile.Read(buffer, length)) > 0) {
				endPoint.Send(buffer, read);
				if ((result = endPoint.Error()) != B_OK)
					goto exit;
			}
		} while (read == length);
		
		if (result == B_OK)
			result = Acknowledge(endPoint);
	}

exit:
	endPoint.Close();
	free(buffer);

	if (result == B_OK)
	{ // Print waiting jobs
		PrintAnyWaitingJob(endPoint);
	}

	return result;
}



ssize_t LPRPrint::Write(const void *buffer, size_t size)
{
	return fSpoolFile.Write(buffer, size);
}

ssize_t LPRPrint::Read(void *, size_t)
{
	return B_NOT_ALLOWED;
}

const char *LPRPrint::Name() const
{
	return kTransportName;
}

BPrintConfigView *LPRPrint::GetConfigView()
{
	return new LPRConfigView(fPrinterNode);
}

status_t LPRPrint::ProbePrinters(BMessage *printers)
{
	// First some informations on this transport
	printers->MakeEmpty();
	printers->AddString(B_TRANSPORT_NAME, Name());
	printers->AddInt32(B_TRANSPORT_ATTRIBUTES, (int32)GetAttributes());
	return B_OK;
}

// -------------------------------------------------------------
// #pragma mark -

status_t LPRPrint::Acknowledge(BNetEndpoint &endpoint)
{
	status_t result;
	char ack;
	int32 size = endpoint.Receive(&ack, 1);
	if ((result = endpoint.Error()) != B_OK)
		return result;
	if (ack != 0)
		return B_ERROR;
	return B_OK;
}

status_t LPRPrint::ReceiveAPrinterJob(BNetEndpoint& endPoint)
{
	status_t result = endPoint.Connect(fLpdAddr);
	if (result != B_OK)
		return result;
	BNetBuffer sendBuffer;
	sendBuffer.AppendUint8(2);
	sendBuffer.AppendData(fQueueName.String(), fQueueName.Length());
	sendBuffer.AppendUint8('\n');
	int32 size = endPoint.Send(sendBuffer);
	if ((result = endPoint.Error()) == B_OK)
		result = Acknowledge(endPoint);
	if (result != B_OK)
		endPoint.Close();
	return result;
}

status_t LPRPrint::PrintAnyWaitingJob(BNetEndpoint& endPoint)
{
	status_t result = endPoint.Connect(fLpdAddr);
	if (result != B_OK)
		return result;
	BNetBuffer sendBuffer;
	sendBuffer.AppendUint8(1);
	sendBuffer.AppendData(fQueueName.String(), fQueueName.Length());
	sendBuffer.AppendUint8('\n');
	int32 size = endPoint.Send(sendBuffer);
	result = endPoint.Error();
	endPoint.Close();
	return result;
}

// -------------------------------------------------------------
// #pragma mark -

LPRConfigView::LPRConfigView(BNode *printer)
	:	BPrintConfigView(BRect(0,0,250,300), "LPR Configuration", B_FOLLOW_ALL, B_WILL_DRAW),
		fPrinter(printer)
{
}

void LPRConfigView::AttachedToWindow()
{
	BPrintConfigView::AttachedToWindow();
	BRect r = Parent()->Bounds();
	r.InsetBy(8,8);
	MoveTo(r.LeftTop());
	ResizeTo(r.Width(), r.Height());

	fTCHostName = new BTextControl(BRect(0,0,250,20), NULL, "IP Address:", "", new BMessage('modi'));
	fTCHostName->ResizeToPreferred();
	fTCHostName->SetDivider(80);
	AddChild(fTCHostName);
	fTCHostName->ResizeTo(250, fTCHostName->Frame().Height());
	fTCHostName->MakeFocus();
	fTCHostName->SetTarget(this);
	fTCHostName->SetModificationMessage(new BMessage('modi'));

	fTCQueueName = new BTextControl(BRect(0,fTCHostName->Frame().bottom + 4,250, 0), NULL, "Queue name:", "lp", new BMessage('modi'));
	fTCQueueName->ResizeToPreferred();
	fTCQueueName->SetDivider(80);
	AddChild(fTCQueueName);
	fTCQueueName->ResizeTo(250, fTCQueueName->Frame().Height());
	fTCQueueName->SetTarget(this);
	fTCQueueName->SetModificationMessage(new BMessage('modi'));

	fTCUserName = new BTextControl(BRect(0,fTCQueueName->Frame().bottom + 4,250, 0), NULL, "Username:", "", new BMessage('modi'));
	fTCUserName->ResizeToPreferred();
	fTCUserName->SetDivider(80);
	AddChild(fTCUserName);
	fTCUserName->ResizeTo(250, fTCUserName->Frame().Height());
	fTCUserName->SetTarget(this);
	fTCUserName->SetModificationMessage(new BMessage('modi'));

	SetSaveEnabled(false);
}

void LPRConfigView::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case 'modi':
			if ((fTCHostName->Text() && *(fTCHostName->Text())!=0)	&&
				(fTCQueueName->Text() && *(fTCQueueName->Text())!=0) &&
				(fTCUserName->Text() && *(fTCUserName->Text())!=0))
				SetSaveEnabled(true);
			else
				SetSaveEnabled(false);
			break;
		default:
			BPrintConfigView::MessageReceived(message);
	}
}

status_t LPRConfigView::Save()
{
printf("LPRConfigView::save\n");
	size_t l;
	const char *t;
	t = fTCHostName->Text();
	if ((!t) || ((l = strlen(t)) == 0)) {
		fTCHostName->MakeFocus();
		return B_BAD_VALUE;
	}
	if (Printer()->WriteAttr(HOST_ADDRESS, B_STRING_TYPE, 0, t, l+1) != l+1) return B_ERROR;

	t = fTCQueueName->Text();
	if ((!t) || ((l = strlen(t)) == 0)) {
		fTCQueueName->MakeFocus();
		return B_BAD_VALUE;
	}
	if (Printer()->WriteAttr(QUEUE_NAME, B_STRING_TYPE, 0, t, l+1)!= l+1) return B_ERROR;

	t = fTCUserName->Text();
	if ((!t) || ((l = strlen(t)) == 0)) {
		fTCUserName->MakeFocus();
		return B_BAD_VALUE;
	}
	if (Printer()->WriteAttr(USER_NAME, B_STRING_TYPE, 0, t, l+1) != l+1) return B_ERROR;

	return B_OK;
}

void LPRConfigView::GetMinimumSize(float *width, float *height)
{
	*width = 8 + 250;
	*height = 8 + fTCUserName->Frame().bottom - fTCHostName->Frame().top;
}



// -------------------------------------------------------------
// #pragma mark -
// For compatibility with old API

extern "C" _EXPORT BDataIO* init_transport(BMessage *msg, BMessage *reply);
extern "C" _EXPORT bool exit_transport();

static LPRPrint *port = NULL;
BDataIO* init_transport(BMessage *msg, BMessage *reply)
{
	const char *printer_file;
	printer_file = msg->FindString("printer_file");
	BDirectory dir(printer_file);
	if (dir.InitCheck() != B_OK)
		return NULL;
	port = new LPRPrint(&dir);
	if (port->InitCheck() == B_OK)
	{
		BMessage jobInfo;
		if (port->BeginPrintjob(jobInfo) != B_OK)
			return NULL;
		reply->what = 'okok';
		reply->AddBool("bidirectional", port->IsReadable());
		reply->AddString(B_TRANSPORT_NAME, port->Name());
		reply->AddInt32(B_TRANSPORT_ATTRIBUTES, (int32)port->GetAttributes());
		return port;
	}
	delete port;
	return (port = NULL);
}


bool exit_transport()
{
	port->EndPrintjob();
	delete port;
	port = NULL;
	return true;
}
