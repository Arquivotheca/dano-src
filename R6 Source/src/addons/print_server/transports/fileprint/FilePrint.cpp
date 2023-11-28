#include "FilePrint.h"
#include <stdio.h>
#include <string.h>
#include <Application.h>
#include <Path.h>
#include <Message.h>
#include <Messenger.h>
#include <Entry.h>


static const char *kTransportName = "Print To File";


extern "C" BTransportAddOn *instantiate_transport_addon(BNode *printer_file)
{
	return (BTransportAddOn *)(new FilePrint(printer_file));
}


// Do not change the order of inheritance for this class -- 
// BLooper MUST appear first, due to its odd destructor, which 
// frees its own memory.  If you put it second, ~FilePrint() 
// will be called twice -- quite nasty. 

FilePrint::FilePrint(BNode *printer)
	: BHandler(), BTransportAddOn(printer),
	ffd(-1),
	fDeviceStatus(B_NO_INIT),
	fAttributes(B_TRANSPORT_IS_SHARABLE)
{
	fDeviceStatus = B_OK;
}

FilePrint::~FilePrint()
{
}

status_t FilePrint::BeginPrintjob(const BMessage& jobInfo)
{
	// We need our own looper here, because
	// we may run out of the print_server's team.
	// REVISIT: we could optimize this by
	// creating our own looper only if we *are* in the be_app thread
	// and use be_app's looper instead.
	BLooper *filePanelLooper = new BLooper("FilePrint");
	filePanelLooper->AddHandler(this);
	filePanelLooper->Run();

	printsem = create_sem(0, "be:transport_file_sem");
	cancel_requested = false;
	valid_output = false;

	const char *filename = jobInfo.FindString(B_JOB_NAME);
	BMessenger target(this);
	BFilePanel save_panel(B_SAVE_PANEL, &target);
	if (filename)
		save_panel.SetSaveText(filename);
	save_panel.Show();

	// wait for user to dismiss file panel...
	acquire_sem(printsem);
	delete_sem(printsem);

	filePanelLooper->Lock();
	filePanelLooper->RemoveHandler(this);
	filePanelLooper->Quit();

	if (CancelRequested())
		fDeviceStatus = B_CANCELED;

	return fDeviceStatus;
}

const char *FilePrint::Name() const
{
	return kTransportName;
}

const char* FilePrint::OutputPath()
{
	return output_path.String();
}

bool FilePrint::CancelRequested()
{
	return cancel_requested;
}

ssize_t FilePrint::Read(void *buffer, size_t size)
{
	return B_NOT_ALLOWED;
}

ssize_t FilePrint::Write(const void *buffer, size_t size)
{
	return fFile.Write(buffer, size);
}

void FilePrint::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case B_SAVE_REQUESTED:
		{
			entry_ref ref;
			msg->FindRef("directory", &ref);
			BDirectory dir(&ref);					
			const char *name = msg->FindString("name");
			BPath path(&dir, name);
			output_path = path.Path();			
			if ((fDeviceStatus = fFile.SetTo(&dir, name, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) == B_OK)
				valid_output = true; // we release the semaphore when we get the B_CANCEL msg
			break;
		}
		
		case B_CANCEL:
		{
			// this message is sent whenever the file panel is hidden
			// so we use valid_output to determine if the user is really
			// trying to cancel or not
			if (!valid_output)
				cancel_requested = true;
			release_sem(printsem);
			break;
		}
		
		default:
			BHandler::MessageReceived(msg);
			break;	
	}
}

status_t FilePrint::ProbePrinters(BMessage *printers)
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

static FilePrint *fileprint = NULL;
BDataIO* init_transport(BMessage *msg, BMessage *reply)
{
	const char *printer_file;
	printer_file = msg->FindString("printer_file");
	BDirectory dir(printer_file);
	if (dir.InitCheck() != B_OK)
		return NULL;
	fileprint = new FilePrint(&dir);
	if (fileprint->InitCheck() == B_OK)
	{
		BMessage jobInfo;
		if (fileprint->BeginPrintjob(jobInfo) != B_OK)
			return NULL;
		reply->what = 'okok';
		reply->AddBool("bidirectional", fileprint->IsReadable());
		reply->AddString(B_TRANSPORT_NAME, fileprint->Name());
		reply->AddInt32(B_TRANSPORT_ATTRIBUTES, (int32)fileprint->GetAttributes());
		return fileprint;
	}
	else if (fileprint->InitCheck() == B_CANCELED)
	{
		reply->what = 'canc';
		return fileprint;
	}
	exit_transport();
	return (fileprint = NULL);
}

bool exit_transport()
{
	fileprint->EndPrintjob();
	delete fileprint;
	fileprint = NULL;
	return true;
}
