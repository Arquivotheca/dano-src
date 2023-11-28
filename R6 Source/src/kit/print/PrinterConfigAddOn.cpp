// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************


#include <stdio.h>
#include <stdlib.h>

#include <support/memadviser.h>

#include <print/PrinterConfigAddOn.h>
#include <pr_server.h>

namespace BPrivate
{
	struct _printer_config_addon_data
	{
		BNode *printer;
		BTransportIO* transport;
		BPrintJobEditSettings settings;
		uint32 page_count;
		uint32 current_page;
		uint32 selection;
		BPrintPaper paper_formats[4];
	};
} using namespace BPrivate;




BPrinterConfigAddOn::BPrinterConfigAddOn(BTransportIO* transport, BNode *printer)
	: 	_fPrivate(new _printer_config_addon_data),
		m(*_fPrivate)
{
	m.current_page = 0;
	m.page_count = 0;
	m.selection = false;
	m.paper_formats[0].SetTo(BPrintPaper::B_LEGAL,		NULL, 215.9f, 355.6f, 5.0f, 5.0f, 7.0f, 20.0f);
	m.paper_formats[1].SetTo(BPrintPaper::B_LETTER,		NULL, 215.9f, 279.4f, 5.0f, 5.0f, 7.0f, 20.0f);
	m.paper_formats[2].SetTo(BPrintPaper::B_A4,			NULL, 210.0f, 297.0f, 5.0f, 5.0f, 7.0f, 20.0f);
	m.paper_formats[3].SetTo(BPrintPaper::B_EXECUTIVE,	NULL, 184.1f, 266.7f, 5.0f, 5.0f, 7.0f, 20.0f);
	BPrinterConfigAddOn::PrinterChanged(transport, printer, BMessage());
}

BPrinterConfigAddOn::~BPrinterConfigAddOn()
{
	delete _fPrivate;
}

status_t BPrinterConfigAddOn::InitCheck()
{
	return B_OK;
}

BPrintConfigView *BPrinterConfigAddOn::AddPrinter(const char *printername, BDirectory *spooldir)
{
	return NULL;
}

status_t BPrinterConfigAddOn::AddPrinterQuiet(const char *printername, BDirectory *spooldir)
{
	return B_OK;
}

status_t BPrinterConfigAddOn::AddPanes(BPrintPanel *panel)
{
	return B_OK;
}
	
status_t BPrinterConfigAddOn::PrinterChanged(BTransportIO* transport, BNode *printer, const BMessage& settings)
{
	m.transport = transport;
	m.printer = printer;
	if (settings.IsEmpty() == false)
	{
		m.settings.SetSettings(settings);
	}
	else
	{
		// TODO: we can also update Settings() with (in this order)
		// - the default settings for this printer
		// - the default settings of this driver
		// - the default settings of the framework
	}
	return B_OK;
}

status_t BPrinterConfigAddOn::SetSettings(const BMessage& settings)
{
	m.settings.SetSettings(settings);
	return B_OK;
}

status_t BPrinterConfigAddOn::SetDocumentSettings(const BMessage& settings)
{
	m.page_count = settings.FindInt32("be:page_count");
	m.current_page = settings.FindInt32("be:current_page");
	m.selection = settings.FindBool("be:selection");
	return B_OK;
}

status_t BPrinterConfigAddOn::Save()
{
	return B_OK;
}

int32 BPrinterConfigAddOn::PrinterModes(const printer_mode_t **modes)
{
	modes = NULL;
	return 0;
}

status_t BPrinterConfigAddOn::PrinterModeSelected(int32 index)
{
	return B_BAD_VALUE;
}



int32 BPrinterConfigAddOn::CountTrays()
{
	return 1;
}

const char *BPrinterConfigAddOn::TrayName(int32 index)
{
	if (index != 0)
		return NULL;
	return "Cut sheet feeder";
}

int32 BPrinterConfigAddOn::PaperFormats(int32 tray, BPrintPaper const **papers)
{
	if (tray != 0)
		return 0;
	*papers = m.paper_formats;
	return (sizeof(m.paper_formats) / sizeof(m.paper_formats[0]));
}

status_t BPrinterConfigAddOn::PaperSelected(int32 tray, int32 paper)
{
	if (tray != 0)
		return B_BAD_VALUE;

	const BPrintPaper *papers;
	PaperFormats(tray, &papers);
	BPrintPaper p = papers[paper];

	// the physical paper size is left untouched
	BRect paper_rect(0, 0, p.PaperWidth(), p.PaperHeight());
	paper_rect.left			= BPrintPaper:: milimeter_to_pixel(paper_rect.left);
	paper_rect.top			= BPrintPaper:: milimeter_to_pixel(paper_rect.top);
	paper_rect.right		= BPrintPaper:: milimeter_to_pixel(paper_rect.right);
	paper_rect.bottom		= BPrintPaper:: milimeter_to_pixel(paper_rect.bottom);
	BRect printable_rect;
	printable_rect = p.PrintableRect();
	printable_rect.left		= BPrintPaper::milimeter_to_pixel(printable_rect.left);
	printable_rect.top		= BPrintPaper::milimeter_to_pixel(printable_rect.top);
	printable_rect.right	= BPrintPaper::milimeter_to_pixel(printable_rect.right);
	printable_rect.bottom	= BPrintPaper::milimeter_to_pixel(printable_rect.bottom);
	Settings().SetDeviceArea(paper_rect);			// this is the 'physical' paper size
	Settings().SetDevicePrintableArea(printable_rect);


	// On the contrary the logical paper size gets its layout modified
	if (Settings().Orientation() == B_PRINT_LAYOUT_LANDSCAPE)
		p.SetLandscape();
	if (Settings().Attributes() & B_PRINT_ATTR_H_MIRROR)
		p.SetHMirror(true);
	if (Settings().Attributes() & B_PRINT_ATTR_V_MIRROR)
		p.SetVMirror(true);

	paper_rect = BRect(0, 0, p.PaperWidth(), p.PaperHeight());
	paper_rect.left			= BPrintPaper:: milimeter_to_pixel(paper_rect.left);
	paper_rect.top			= BPrintPaper:: milimeter_to_pixel(paper_rect.top);
	paper_rect.right		= BPrintPaper:: milimeter_to_pixel(paper_rect.right);
	paper_rect.bottom		= BPrintPaper:: milimeter_to_pixel(paper_rect.bottom);
	printable_rect = p.PrintableRect();
	printable_rect.left		= BPrintPaper::milimeter_to_pixel(printable_rect.left);
	printable_rect.top		= BPrintPaper::milimeter_to_pixel(printable_rect.top);
	printable_rect.right	= BPrintPaper::milimeter_to_pixel(printable_rect.right);
	printable_rect.bottom	= BPrintPaper::milimeter_to_pixel(printable_rect.bottom);
	Settings().SetPaperRect(paper_rect);			// this is the 'logical' paper size
	Settings().SetPrintableRect(printable_rect);

	Settings().SetXdpi(72);		// the papers are exprimed in 1/72th of an inch
	Settings().SetYdpi(72);

	return B_OK;
}


status_t BPrinterConfigAddOn::CleanPrintHeads(bool is_feature_availlable)
{
	if (is_feature_availlable)
		return B_NOT_ALLOWED;
	return B_BAD_VALUE;
}

status_t BPrinterConfigAddOn::PrintNozzleCheckPattern(bool is_feature_availlable)
{
	if (is_feature_availlable)
		return B_NOT_ALLOWED;
	return B_BAD_VALUE;
}

status_t BPrinterConfigAddOn::PrinterStatus(printer_status_t *status)
{
	return B_NOT_ALLOWED;
}

BNode *BPrinterConfigAddOn::Printer() const
{
	return m.printer;
}

BTransportIO *BPrinterConfigAddOn::Transport() const
{
	return m.transport;
}

status_t BPrinterConfigAddOn::Model(BString& mdl)
{
	char model[256];
	status_t err = m.printer->ReadAttr(PSRV_PRINTER_ATTR_MDL, B_STRING_TYPE, 0, model, sizeof(model)-1);
	if (err < 0)
		return err;
	model[err] = 0;
	mdl = model;
	return B_OK;
}

const uint32 BPrinterConfigAddOn::PageCount() const
{
	return m.page_count;
}

const uint32 BPrinterConfigAddOn::CurrentPage() const
{
	return m.current_page;
}

bool BPrinterConfigAddOn::DocSelection() const
{
	return m.selection;
}

BPrintJobEditSettings& BPrinterConfigAddOn::Settings() const
{
	return m.settings;
}

// ************************************************************
// memory allocation
// ************************************************************
// #pragma mark -

void *BPrinterConfigAddOn::malloc(size_t size)
{
	void *buffer;
	if (madv_reserve_memory(size, "BPrinterAddOn::malloc"))
		buffer = ::malloc(size);
	madv_finished_allocating(size);
	return buffer;
}

void BPrinterConfigAddOn::free(void *ptr)
{
	::free(ptr);
}

void *BPrinterConfigAddOn::realloc(void *ptr, size_t size)
{
	return ::realloc(ptr, size);
}

// --------------------------------------------------------
// #pragma mark -

status_t BPrinterConfigAddOn::Perform(int32 selector, void * data) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_0(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_1(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_2(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_3(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_4(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_5(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_6(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_7(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_8(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_9(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_10(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_11(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_12(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_13(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_14(int32 arg, ...) { return B_ERROR; }
status_t BPrinterConfigAddOn::_Reserved_BPrinterConfigAddOn_15(int32 arg, ...) { return B_ERROR; }

