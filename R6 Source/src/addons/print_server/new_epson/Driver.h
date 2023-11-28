//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef DRIVER_H
#define DRIVER_H

#include <FindDirectory.h>
#include <Directory.h>
#include <Path.h>
#include <ListItem.h>

#include <print/PrinterAddOnDefs.h>
#include <print/PrinterConfigAddOn.h>
#include <print/PrinterRasterAddOn.h>
#include <print/TransportIO.h>
#include <print/PrintConfigView.h>
#include <print/PrintPaper.h>

#include "MDefinePrinter.h"

class BView;
class BListView;
class MDither;
class MPrinter;
class MPrinterUtils;

// /////////////////////////////////////////////////////////////////////////////////////

class BEpsonCommon : public BPrinterRasterAddOn
{
public:
			BEpsonCommon(BTransportIO* transport, BNode *printer_file, image_id image, MDefinePrinter *addon);
	virtual ~BEpsonCommon();
	tPrinterDef& PrinterDef() { return fPrinterDef; }
	MDefinePrinter *PrinterDev() { return fPrinterDev; }

protected:
	virtual	status_t BeginJob();
	virtual	status_t EndJob();
	status_t UpdateStatus(bool force = false, int force_status_value = printer_status_t::B_UNKNOWN);

	virtual status_t OutputData(const print_bitmap_t *) { return B_OK; };
	virtual status_t GetSupportedDeviceID(uint32 *count, printer_id_t **ids);
	virtual status_t Cancel();
	tPrinterDef fPrinterDef;
	MPrinter *fPrinter;
	MPrinterUtils *fPrinterUtils;
	bool fCancelRequested;
	bool fStatusUpdateNeeded;

private:
	image_id fAddonImage;
	MDefinePrinter *fPrinterDev;
	bigtime_t fPreviousTime;
public:
	enum {
		STATUS_DELAY = 2000000,		// update the status every 2 sec
		ERROR_DELAY = 1000000		// wait 1 sec in case of I/O error
	};
};

// /////////////////////////////////////////////////////////////////////////////////////

class BEpson : public BEpsonCommon, public BTransportIOErrorHandler
{
public:
			BEpson(BTransportIO* transport, BNode *printer_file, image_id image, MDefinePrinter *addon);
	virtual ~BEpson();
	void EveryLine();	// Called by the MDither for every line
private:
	virtual	status_t BeginJob();
	virtual	status_t EndJob();
	virtual status_t EndPage();
	virtual status_t BeginPage(const print_bitmap_t *bitmap);
	virtual status_t OutputData(const print_bitmap_t *bitmap);
	virtual bool Error(status_t& io_error, bool retry);

private:
	MDither *fDither;
};

// /////////////////////////////////////////////////////////////////////////////////////

class BEpsonConfig : public BPrinterConfigAddOn
{
public:
			BEpsonConfig(BTransportIO* transport, BNode *printer_file, MDefinePrinter *printer_def = NULL);
	virtual ~BEpsonConfig();
	tPrinterDef& PrinterDef() { return fPrinterDef; }
	MDefinePrinter *PrinterDev() { return fPrinterDev; }
private:
	virtual BPrintConfigView *AddPrinter(const char *printername, BDirectory *spooldir);
	virtual status_t AddPrinterQuiet(const char *printername, BDirectory *spooldir);
	virtual int32 PrinterModes(printer_mode_t const **modes);
	virtual status_t PrinterModeSelected(int32 index);
	virtual status_t PaperSelected(int32 tray, int32 paper);
	virtual int32 PaperFormats(int32 tray, BPrintPaper const **papers);

	virtual status_t CleanPrintHeads(bool is_feature_availlable=false);
	virtual status_t PrintNozzleCheckPattern(bool is_feature_availlable=false);
	virtual status_t PrinterStatus(printer_status_t *status);

	status_t printer_added(BNode *printer_file, BPath *addon);
	int FetchAddonsRefs(const char *path, BMessage *refs);

	image_id fAddonImage;
	MDefinePrinter *fPrinterDev;
	printer_mode_t *fPrinterModes;
	BPrintPaper *fPaperFormats;
	int32 fNbPaperFormat;
	int32 fNbPrinterModes;
	tPrinterDef fPrinterDef;
	MPrinterUtils *fPrinterUtils;
	printer_status_t fPrinterStatusCached;
};

// /////////////////////////////////////////////////////////////////////////////////////

class AddPrinterView : public BPrintConfigView
{
public:
			AddPrinterView(BNode *printer);
	virtual ~AddPrinterView();

	virtual status_t Save();
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);
	virtual void GetMinimumSize(float *width, float *height);

private:
	status_t AddAddons(directory_which which);
	int compare_string_items(const void *item1, const void *item2);

	class BEpsonItem : public BStringItem
	{ public:
		BEpsonItem(const char *pretty_name, const BPath& v, const char *name)
			: BStringItem(pretty_name), fValue(v), fName(name) { }
		const char *Name() { return fName.String(); }
		BPath fValue;
		BString fName;
	};
	
	BNode *Printer() { return fPrinter; };
	
	BListView *fPrinterList;
	BNode *fPrinter;
};

#endif




