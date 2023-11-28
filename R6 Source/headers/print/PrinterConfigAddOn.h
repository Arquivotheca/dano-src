// ***********************************************************************
// libprint.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _PRINTER_CONFIG_ADDON_H_
#define _PRINTER_CONFIG_ADDON_H_

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

#include <print/PrintJobEditSettings.h>
#include <print/TransportIO.h>
#include <print/PrintPaper.h>
#include <print/PrinterAddOnDefs.h>

class BPrinterConfigAddOn;
class BPrintConfigView;
class BPrintPanel;

namespace BPrivate
{
	struct _printer_config_addon_data;	
}

// Must return a pointer to a newly created BPrinterConfigAddOn.
extern "C" BPrinterConfigAddOn *instantiate_printer_config_addon(BTransportIO* transport, BNode *printer_file);

#define B_INSTANTIATE_PRINTER_CONFIG_ADDON_FUNCTION	"instantiate_printer_config_addon"



class BPrinterConfigAddOn
{
public:
			BPrinterConfigAddOn(BTransportIO* transport, BNode *printer_file);
	virtual	~BPrinterConfigAddOn();
	virtual status_t InitCheck();	// != B_OK if there was an error in the ctor

	/////////////////////////////////////////////////////////////////////
	// Called when a printer is added. Let a chance to the driver to configure the new printer
	virtual BPrintConfigView *AddPrinter(const char *printername, BDirectory *spooldir);
	virtual status_t AddPrinterQuiet(const char *printername, BDirectory *spooldir);

	// Let a chance to the driver to add its own panes to the BPrintPanel
	virtual status_t AddPanes(BPrintPanel *panel);
	
	// The user selected another printer
	// This inherited method _must_ be called by the derived class
	// updates values returned by Printer(), Transport() and Settings()
	virtual status_t PrinterChanged(BTransportIO* transport, BNode *printer, const BMessage& settings);
	
	// Called by the framework to set the current settings
	virtual status_t SetSettings(const BMessage& settings);

	// Called by the framework to set the document settings
	// implement this hook to update the GUI if needed
	// *Must* call the base version
	virtual status_t SetDocumentSettings(const BMessage& settings);

	// Called by the framework to inform the driver that the user saved the settings
	// it's its last chance to update the current Settings() with the content of the panels
	virtual status_t Save();

	/////////////////////////////////////////////////////////////////////
	// Query the driver's paper types/resolution
	struct printer_mode_t
	{
		printer_mode_t(const char *p=NULL, const char *q=NULL, uint32 a=0)
			: paper(p), quality(q), attributes(a) { }
		enum
		{ // Attributes of this mode
			B_MASK_ALL			= 0xFFFFFFFF,
			B_BLACK_ONLY		= 0x00000001,
			B_COLOR_ONLY		= 0x00000002,
			B_BLACK_COLOR		= 0x00000003,
			B_IS_DEFAULT_PAPER	= 0x00000004,
			B_IS_DEFAULT_QUALITY= 0x00000008,
		};
		const char *paper;
		const char *quality;
		uint32 attributes;
		uint32 user;		// Do what you want with this value
		uint32 _reserved[4];
	};
	virtual int32 PrinterModes(printer_mode_t const **modes);
	virtual status_t PrinterModeSelected(int32 index);

	/////////////////////////////////////////////////////////////////////
	// Methods to query the driver's paper sizes and capabilities
	virtual int32 CountTrays();
	virtual const char *TrayName(int32 index);
	virtual int32 PaperFormats(int32 tray, BPrintPaper const **papers);	
	virtual status_t PaperSelected(int32 tray, int32 paper);
	
	/////////////////////////////////////////////////////////////////////
	// Generic printer-driver's special features
	// When theses functions are called with is_feature_availlable=true
	// the function must return immediately WITHOUT performing any action on
	// the printer and return B_OK if the feature is availlable or B_NOT_ALLOWED if not.
	// If the feature is availlable, the same function will be called with
	// is_feature_availlable=false, in this case the action MUST be performed

	virtual status_t CleanPrintHeads(bool is_feature_availlable=false);
	virtual status_t PrintNozzleCheckPattern(bool is_feature_availlable=false);
		
	// status CAN be NULL, in this case just return B_OK if you support status report
	virtual status_t PrinterStatus(printer_status_t *status);


	/////////////////////////////////////////////////////////////////////
	BTransportIO *Transport() const;
	BNode *Printer() const;
	status_t Model(BString& mdl);

	// These are valid after SetDocumentSettings() has been called
	const uint32 PageCount() const;
	const uint32 CurrentPage() const;
	bool DocSelection() const;

	// Called by the framework to get the settings back
	// (it will be called _after_ BPrintConfigView::Save() is called
	BPrintJobEditSettings& Settings() const;
	
	// Memory allocation. These functions know about the memory adviser.
	// Use them for big allocations.
	void *malloc(size_t size);
	void free(void *ptr);
	void *realloc(void *ptr, size_t size);

private:
	BPrinterConfigAddOn(const BPrinterConfigAddOn &);
	BPrinterConfigAddOn& operator = (const BPrinterConfigAddOn);

	virtual status_t _Reserved_BPrinterConfigAddOn_0(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_1(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_2(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_3(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_4(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_5(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_6(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_7(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_8(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_9(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_10(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_11(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_12(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_13(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_14(int32 arg, ...);
	virtual status_t _Reserved_BPrinterConfigAddOn_15(int32 arg, ...);
	virtual status_t Perform(int32 selector, void *data);

private:
	BPrivate::_printer_config_addon_data *_fPrivate;
	BPrivate::_printer_config_addon_data& m;
	uint32 _reserved_BPrinterConfigAddOn_[4];
};

#endif
