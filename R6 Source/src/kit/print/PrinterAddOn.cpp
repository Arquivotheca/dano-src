// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#include <stdio.h>

#include <support/String.h>
#include <support/memadviser.h>
#include <support/Binder.h>

#include <print/PrintJobEditSettings.h>
#include <print/PrinterAddOn.h>
#include <print/PrinterAddOnDefs.h>
#include <print/TransportIO.h>

#include "PrintEnv.h"
#include "PrintStream.h"

// Always use the binder for now
#if (PRINTING_FOR_DESKTOP)
#	define USE_BINDER_FOR_STATUS	1
#else
#	define USE_BINDER_FOR_STATUS	1
#endif

// ************************************************************
// Defines
// ************************************************************

#define m _rPrivate


// ************************************************************
// Private classes
// ************************************************************

namespace BPrivate
{

// Our private data
struct _printer_addon_data
{
	_printer_addon_data()
		: 	fPrinter(NULL),
			fTransport(NULL),
			fStream(NULL),
			fCancelRequested(false)
	{
	}

	BPrintJobEditSettings	fSettings;
	BNode *					fPrinter;
	BTransportIO *			fTransport;
	BPrintStream *			fStream;
	bool					fCancelRequested;
};

} using namespace BPrivate;

// ************************************************************
// ************************************************************
BPrinterAddOn::BPrinterAddOn(BTransportIO* transport, BNode *printer_file)
	: 	_fPrivate(new _printer_addon_data),
		_rPrivate(*_fPrivate)
{
	m.fTransport = transport;
	m.fPrinter = printer_file;
}

// ************************************************************
// ************************************************************
BPrinterAddOn::~BPrinterAddOn(void)
{
	delete m.fStream;
	delete _fPrivate;
}

// ************************************************************
// ************************************************************
status_t BPrinterAddOn::InitCheck()
{
	return B_OK;
}

// ************************************************************
// ************************************************************
status_t BPrinterAddOn::GetDeviceID(BString& device_id) const
{
	if (m.fTransport)
		return m.fTransport->GetDeviceID(device_id);
	return B_NOT_ALLOWED;
}

// ************************************************************
// ************************************************************
BTransportIO *BPrinterAddOn::Transport() const
{
	return m.fTransport;
}

// ************************************************************
// ************************************************************
BNode *BPrinterAddOn::Printer() const
{
	return m.fPrinter;
}

// ************************************************************
// ************************************************************
const BPrintJobSettings& BPrinterAddOn::Settings() const
{
	return m.fSettings;
}

// ************************************************************
// ************************************************************
bool BPrinterAddOn::CanContinue()
{
	if (m.fStream == NULL)		// TakeJob has not been called yet! Should not be there. continue anyway.
		return true;

	if (TPrintTools::CancelRequested(Printer(), m.fStream->SpoolFile()))
	{
		Cancel();
		return false;
	}

	return true;
}

// ************************************************************
// ************************************************************

void BPrinterAddOn::ReportPrinterStatus(const printer_status_t& status)
{
	const char *STATUS[] = {"B_UNKNOWN", "B_ONLINE", "B_OFFLINE", "B_PRINTING", "B_CLEANING", "B_PAPER_JAM", "B_NO_PAPER", "B_NO_INK", "B_ERROR", "B_COVER_OPEN"};
	const char *s = STATUS[status.status+1];

#if (USE_BINDER_FOR_STATUS)
	BString p = Settings().PrinterName();
	BinderNode::property printerProperty = BinderNode::Root() / "service" / "printing" / p.String();
	if (printerProperty->IsValid())
	{ // Binder is valid
		int inkLevel = 0xFF;
		for (int i=0 ; i<8 ; i++)
		{ // Find the smalest inklevel
			if (status.CMYKcmyk[i] <= 100)
				inkLevel = (inkLevel < status.CMYKcmyk[i]) ? (inkLevel) : (status.CMYKcmyk[i]);
		}
		printerProperty["Status"] = s;
		printerProperty["InkLevel"] = (double)inkLevel;
		//printf("BPrinterAddOn::ReportPrinterStatus(%s) through Binder\n", s);
	}
	else
#endif
	{ // TODO: here we should take some action to display the current printer status
		//printf("BPrinterAddOn::ReportPrinterStatus(%s)\n", s);
	}
}

// ************************************************************
// ************************************************************
status_t BPrinterAddOn::TakeJob(const BMessage *msg)
{ // Stream initialized from a BMessage
	return take_job(NULL, msg);
}

status_t BPrinterAddOn::TakeJob(BFile *spool_file)
{ // Stream initialized from the spool file
	return take_job(spool_file, NULL);
}

status_t BPrinterAddOn::take_job(BFile *spool_file, const BMessage *msg)
{
	status_t result;

	if (msg)
	{ // Create the stream to the spooled data
		m.fStream = new BPrintStream(msg);
	}
	else if (spool_file)
	{ // Create the stream to the spooled data
		m.fStream = new BPrintStream(spool_file);
	}
	else
	{ // Invalid parameter
		return B_BAD_VALUE;
	}

	// Initialize the settings from the spool file/msg
	m.fSettings.SetSettings(m.fStream->Settings());
	m.fCancelRequested = false;

	// Correct the number of page to print (the user wants to print all pages)
	const uint32 first = Settings().FirstPage();
	uint32 last = Settings().LastPage();
	if (last > (first + m.fStream->NbPages() - 1)) {
		last = first + m.fStream->NbPages() - 1;
		m.fSettings.SetLastPage(last);
	}

	// ---- generate and print the page

	// Begin a document
	if ((result = BeginDocument()) != B_OK)
		return result;
		
		// Use a page iterator to go through pages
		const uint32 attr		= Settings().Attributes();
		const uint32 nbcopies	= Settings().NbCopies();
		bool assembled			= (attr & B_PRINT_ATTR_ASSEMBLED);
		bool reversed			= (attr & B_PRINT_ATTR_REVERSE);

		// Begin a job
		if ((result = BeginJob()) != B_OK)
			return result;
		
		PageIterator iter(first, last, nbcopies, assembled, reversed);
		do
		{
			int32 page, copies;
			if (iter.Next(&page, &copies) != B_OK)
				break;
			page_t page_data;
			if ((result = GetPage(page, &page_data)) == B_OK)
				result = Print(page_data, copies); // Print the page
			PutPage(&page_data);
		} while ((m.fCancelRequested==false) && (result == B_OK));
	
		// End the job
		EndJob();

	// End the document
	EndDocument();

	// the job has been user-cancelled
	if (m.fCancelRequested)
		return B_CANCELED;

	return result;
}

status_t BPrinterAddOn::take_job(const BMessage *msg)
{
	status_t result;
	if (!msg)
		return B_BAD_VALUE;

	// We don't have a stream in 'direct'.
	m.fStream = NULL;

	// Initialize the settings
	m.fSettings.SetSettings(*msg);
	m.fCancelRequested = false;

	const uint32 nbPages = Settings().Message().FindInt32("be:nb_pages");
	const uint32 first = Settings().FirstPage();
	uint32 last = Settings().LastPage();
	if (last > (first + nbPages - 1)) {
		last = first + nbPages - 1;
		m.fSettings.SetLastPage(last);
	}
	// ---- generate and print the page

	// Begin a document
	if ((result = BeginDocument()) != B_OK)
		return result;
		
		// Use a page iterator to go through pages
		const uint32 attr		= Settings().Attributes();
		const uint32 nbcopies	= Settings().NbCopies();
		bool assembled			= (attr & B_PRINT_ATTR_ASSEMBLED);
		bool reversed			= (attr & B_PRINT_ATTR_REVERSE);

		// Begin a job
		if ((result = BeginJob()) != B_OK)
			return result;
		
		PageIterator iter(first, last, nbcopies, assembled, reversed);
		do
		{
			int32 page, copies;
			if (iter.Next(&page, &copies) != B_OK)
				break;
			page_t page_data;
			if ((result = GetPage(page, &page_data)) == B_OK)
				result = Print(page_data, copies); // Print the page
			PutPage(&page_data);
		} while ((m.fCancelRequested==false) && (result == B_OK));
	
		// End the job
		EndJob();

	// End the document
	EndDocument();

	// the job has been user-cancelled
	if (m.fCancelRequested)
		return B_CANCELED;

	return result;
}


// ************************************************************
status_t BPrinterAddOn::GetPage(const uint32 page_number, page_t *page)
{
	if (page == NULL)
		return B_BAD_VALUE;

	page->picture_count = 0;
	page->pictures = NULL;
	page->clips = NULL;
	page->points = NULL;
	page->page = page_number;

	if (m.fStream == NULL)
		return B_OK; // We are in 'direct' mode. no stream.

	if (m.fStream->GotoPage(page_number) != B_OK)
		return B_BAD_VALUE;

	const uint32 cp = m.fStream->NbPictures();
	if (cp <= 0)
		return B_ERROR;
	
	// Temporary datas
	page->picture_count = cp;
	page->points			= new BPoint[page->picture_count];
	page->clips				= new BRect[page->picture_count];
	page->pictures			= new BPicture *[page->picture_count];
	for (uint32 p=0 ; p<page->picture_count ; p++)
		page->pictures[p] = 0;

	// Read the pictures and stuffs
	for (uint32 p=0 ; (p<page->picture_count) ; p++)
	{
		m.fStream->GotoPicture(p);
		m.fStream->ReadPoints(page->points + p);
		m.fStream->ReadClips(page->clips + p);
		page->pictures[p] = m.fStream->ReadPicture();
	}

	return B_OK;
}

// ************************************************************
status_t BPrinterAddOn::PutPage(page_t *page)
{
	if (m.fStream == NULL)
		return B_OK; // We are in 'direct' mode. no stream.

	if (page == NULL)
		return B_BAD_VALUE;

	for (uint32 p=0 ; p<page->picture_count ; p++)
		m.fStream->GetRidOfPicture(page->pictures[p]);
	delete [] page->pictures;	page->pictures = NULL;
	delete [] page->points;		page->points = NULL;
	delete [] page->clips;		page->clips = NULL;
	page->picture_count = 0;
	return B_OK;
}

// ************************************************************
status_t BPrinterAddOn::Cancel()
{
	m.fCancelRequested = true;
	return B_OK;
}

// ************************************************************
status_t BPrinterAddOn::Print(const page_t& page, const int nbCopies)
{
	return B_OK;
}

// ************************************************************
status_t BPrinterAddOn::BeginJob() {
	return B_OK;
}

status_t BPrinterAddOn::EndJob() {
	return B_OK;
}

status_t BPrinterAddOn::BeginDocument() {
	return B_OK;
}

status_t BPrinterAddOn::EndDocument() {
	return B_OK;
}

status_t BPrinterAddOn::GetSupportedDeviceID(uint32 *count, printer_id_t **ids)
{
	*count = 0;
	return B_OK;
}


// ************************************************************
// memory allocation
// ************************************************************
// #pragma mark -

void *BPrinterAddOn::malloc(size_t size)
{
	void *buffer;
	if (madv_reserve_memory(size, "BPrinterAddOn::malloc"))
		buffer = ::malloc(size);
	madv_finished_allocating(size);
	return buffer;
}

void BPrinterAddOn::free(void *ptr)
{
	::free(ptr);
}

void *BPrinterAddOn::realloc(void *ptr, size_t size)
{
	return ::realloc(ptr, size);
}

// #pragma mark -

status_t BPrinterAddOn::Perform(int32 selector, void * data)
{
	return B_ERROR;
}

status_t BPrinterAddOn::_Reserved_BPrinterAddOn_0(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_1(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_2(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_3(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_4(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_5(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_6(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_7(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_8(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_9(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_10(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_11(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_12(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_13(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_14(int32 arg, ...) { return B_ERROR; }
status_t BPrinterAddOn::_Reserved_BPrinterAddOn_15(int32 arg, ...) { return B_ERROR; }

