// ***********************************************************************
// libprint.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _PRINTER_ADDON_H_
#define _PRINTER_ADDON_H_

#include <BeBuild.h>
#include <String.h>
#include <Node.h>
#include <DataIO.h>
#include <File.h>
#include <Picture.h>
#include <Rect.h>
#include <Point.h>
#include <Message.h>
#include <SupportDefs.h>

#include <print/PrintJobSettings.h>
#include <print/PrinterAddOnDefs.h>


class BPrinterAddOn;
class BPrintStream;
class BTransportIO;
class BPrintConfigView;


// Must return a pointer to a newly created BPrinterAddOn.
// Or NULL if there is no more driver availlable after the passed index.
// (index=0 should never return NULL).
extern "C" BPrinterAddOn *instantiate_printer_addon(int32 index, BTransportIO* transport, BNode *printer_file);

#define B_INSTANTIATE_PRINTER_ADDON_FUNCTION	"instantiate_printer_addon"

namespace BPrivate
{
	struct _printer_addon_data;	
}

class BPrinterAddOn
{
public:
			BPrinterAddOn(BTransportIO* transport, BNode *printer_file);
	virtual	~BPrinterAddOn();
	virtual status_t Perform(int32 selector, void *data);
	virtual status_t InitCheck();	// != B_OK if there was an error in the ctor

	// Return a list of supported printers. If mdl is empty the driver
	// will match for all printers from the manufacturer 'mfg'
	struct printer_id_t	{
		const char *mfg;	// Cannot be NULL
		const char *mdl;	// Can be NULL, in this case the matching is done only on mfg
		uint32 reserved[2];	// RFU
	};
	virtual status_t GetSupportedDeviceID(uint32 *count, printer_id_t **ids);

public:
	struct page_t
	{
		uint32 picture_count;
		BPicture **pictures;
		BRect *clips;
		BPoint *points;
		uint32 reserved0;
		uint32 page;
		uint32 reserved[8];
	};

	// Each GetPage call must be balanced by its counterpart PutPage
	status_t GetPage(const uint32 page_number, page_t *page);
	status_t PutPage(page_t *page);

	status_t GetDeviceID(BString& device_id) const;
	BTransportIO *Transport() const;
	BNode *Printer() const;

	// Returns the current settings
	const BPrintJobSettings& Settings() const;

	// The derived class must call this method on a regular basis to know if the job
	// should continue or if it has been canceled
	bool CanContinue();
	
	// Call this function on a regular basis to let the system know about the printer status
	// Should be called when the status changed.
	void ReportPrinterStatus(const printer_status_t& status);
	

	// Memory allocation. These functions know about the memory adviser.
	// Use them for big allocations.
	void *malloc(size_t size);
	void free(void *ptr);
	void *realloc(void *ptr, size_t size);

protected:
	// Called when the user canceled the job. Implementation should expect this method
	// to be called from CanContinue()
	virtual status_t 	Cancel();

	// Called by the framework
	virtual	status_t	BeginDocument();
	virtual	status_t	EndDocument();
	virtual	status_t	BeginJob();
	virtual status_t	EndJob();
	
	// Called by the framework for each page to print
	virtual status_t	Print(const page_t& page, const int nbCopies = 1);

private:
	BPrinterAddOn(const BPrinterAddOn &);
	BPrinterAddOn& operator = (const BPrinterAddOn &);

	friend class BPrintJob;
	friend class BDirectPrintJob;
	friend class BServer;
			status_t	TakeJob(const BMessage *msg);
			status_t	TakeJob(BFile *spool_file);

	status_t take_job(BFile *spool_file, const BMessage *msg);
	status_t take_job(const BMessage *msg);
	virtual status_t _Reserved_BPrinterAddOn_0(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_1(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_2(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_3(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_4(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_5(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_6(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_7(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_8(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_9(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_10(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_11(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_12(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_13(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_14(int32 arg, ...);
	virtual status_t _Reserved_BPrinterAddOn_15(int32 arg, ...);

private:
	BPrivate::_printer_addon_data *_fPrivate;
	BPrivate::_printer_addon_data& _rPrivate;
	uint32 _reserved_BPrinterAddOn_[4];
};

#endif
