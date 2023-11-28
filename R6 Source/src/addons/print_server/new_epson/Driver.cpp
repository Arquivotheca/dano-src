//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <BeBuild.h>
#include <SupportDefs.h>
#include <Errors.h>
#include <Message.h>
#include <OS.h>
#include <Window.h>
#include <View.h>
#include <Bitmap.h>
#include <Rect.h>
#include <Point.h>
#include <Region.h>
#include <Autolock.h>
#include <ListView.h>
#include <ScrollView.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Entry.h>
#include <AppFileInfo.h>

#include "Driver.h"
#include "DriverEpsonModule.h"
#include "MDefinePrinter.h"
#include "MPrinter.h"
#include "raster.h"
#include "raster360.h"
#include "rasterNSlices.h"

#define DEBUG 0
#if DEBUG
#	define bug		printf
#	define D(_x)	_x
#else
#	define bug		printf
#	define D(_x)
#endif

const char * M_XPRINTER_ADDON_SIGNATURE = "application/x-printer-addon";
static image_id init_printer_add_on(BNode *printer_file, MDefinePrinter **define_printer);
static image_id init_epson_add_on(const char *addon_path, MDefinePrinter **define_printer);


// /////////////////////////////////////////////////////////////////////////////////////


extern "C" BPrinterAddOn *instantiate_printer_addon(int32 index, BTransportIO* transport, BNode *printer_file)
{
	if (index != 0)	return NULL;
	if (printer_file == NULL) { // the printer folder is not availlable yet. We are probably installing the driver.
		// Just return a common instance.
		return static_cast<BPrinterAddOn *>(new BEpsonCommon(transport, printer_file, -1, NULL));
	}
	BPrinterAddOn *instance;
	MDefinePrinter *addon;
	image_id image = init_printer_add_on(printer_file, &addon);
	if (image < 0)
		return NULL;
	if (addon->IsEpsonModule() == false) { // What kind of addon do we need? Our own type, or based on the epson printing system.
		instance = static_cast<BPrinterAddOn *>(new BEpson(transport, printer_file, image, addon));
	} else {
		instance = static_cast<BPrinterAddOn *>(new BEpsonModule(transport, printer_file, image, addon));
	}
	return instance;
}

extern "C" BPrinterConfigAddOn *instantiate_printer_config_addon(BTransportIO* transport, BNode *printer_file)
{
	return static_cast<BPrinterConfigAddOn *>(new BEpsonConfig(transport, printer_file));
}


// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

BEpsonCommon::BEpsonCommon(BTransportIO* transport, BNode *printer_file, image_id image, MDefinePrinter *addon)
	:	BPrinterRasterAddOn(transport, printer_file, 0, 0),
		fAddonImage(image),
		fPrinterDev(addon),
		fPrinter(NULL),
		fPrinterUtils(NULL),
		fStatusUpdateNeeded(true),
		fPreviousTime(0),
		fCancelRequested(false)
{
}

BEpsonCommon::~BEpsonCommon()
{
	delete fPrinter;
	delete fPrinterDev;
	delete fPrinterUtils;
	if (fAddonImage >= 0)
		unload_add_on(fAddonImage);
}

status_t BEpsonCommon::BeginJob()
{
	BPrinterRasterAddOn::BeginJob();

	const float x_factor = ((float)Settings().DeviceXdpi() / (float)Settings().Xdpi());
	const float y_factor = ((float)Settings().DeviceYdpi() / (float)Settings().Ydpi());
	PrinterDef().printer_width		= (uint16)((Settings().DevicePrintableArea().Width() + 1.0f) 	* x_factor);
	PrinterDef().printer_height		= (uint16)((Settings().DevicePrintableArea().Height()+ 1.0f)	* y_factor);
	PrinterDef().printable_top		= (uint16)(Settings().DevicePrintableArea().top					* y_factor);
	PrinterDef().printable_bottom	= (uint16)(Settings().DevicePrintableArea().bottom				* y_factor);
	PrinterDef().expanded 			= false;
	PrinterDef().res				= Settings().Message().FindInt32("epson:res");
	PrinterDef().paper				= Settings().Message().FindInt32("epson:paper");
	PrinterDef().paperformat		= Settings().Message().FindInt32("epson:paperformat");
	PrinterDef().color_mode			= Settings().Message().FindInt32("epson:color_mode");
	PrinterDef().weave_mode			= Settings().Message().FindInt32("epson:weave_mode");
	PrinterDef().speed_mode			= Settings().Message().FindInt32("epson:speed_mode");
	PrinterDef().microdot			= Settings().Message().FindInt32("epson:microdot");
	PrinterDef().hslices			= Settings().Message().FindInt32("epson:hslices");
	PrinterDef().soft_microweave	= Settings().Message().FindInt32("epson:soft_microweave");

	// Make sure to not be larger than the maximum page size
	status_t err;
	tPageFormat min, max;
	if ((err = PrinterDev()->PageLimits(&min, &max)) != B_OK)
		return err;
	// we must scale min/max in the device output resolution
	const float fx = (float)Settings().DeviceXdpi() / 360.0f;
	D(bug("%ld / %ld\n", PrinterDef().printer_width, (long)floor(max.printable_width * fx));)
	if (PrinterDef().printer_width > (uint16)floor(max.printable_width * fx))
		PrinterDef().printer_width = (uint16)floor(max.printable_width * fx);

	// TODO: we should also make sure that the height is correct

	// Get information about this printer
	PrinterDef().printer_info._size = sizeof(PrinterDef().printer_info);
	PrinterDev()->GetPrinterInfo(&PrinterDef().printer_info);

	// Create an MPrinter
	fPrinter = new MPrinter(*this);

	// We need a MPrinterUtils to interrect with the printer
	if (fPrinterUtils == NULL)
		fPrinterUtils = new MPrinterUtils(Transport(), PrinterDev(), &PrinterDef(), false);

	// Update status
	UpdateStatus(true);

	return B_OK;
}

status_t BEpsonCommon::EndJob()
{
	// Update status for the last time and force the status value to ONLINE
	UpdateStatus(true, printer_status_t::B_ONLINE);

	return BPrinterRasterAddOn::EndJob();
}

status_t BEpsonCommon::UpdateStatus(bool force, int force_status_value)
{
	const bigtime_t rightNow = system_time();
	if ((force) || (fStatusUpdateNeeded) || ((rightNow - fPreviousTime) > STATUS_DELAY))
	{ // Update the status first
		BMessage msg;
		status_t result = fPrinterUtils->GetStatus(msg);
		printer_status_t status;
		if ((result == B_OK) && !msg.IsEmpty())
		{ // We got the status and it changed
			fPrinterUtils->ConvertToStatus(msg, &status);
			if (force_status_value != printer_status_t::B_UNKNOWN)
				status.status = force_status_value;
			ReportPrinterStatus(status);
			fStatusUpdateNeeded = false;
			fPreviousTime = rightNow;
		}
		return result;
	}
	return B_OK;
}

status_t BEpsonCommon::GetSupportedDeviceID(uint32 *count, printer_id_t **ids)
{
	static printer_id_t printerID[] = { {"EPSON", NULL} };
	*count = sizeof(printerID)/sizeof(printerID[0]);
	*ids = printerID;
	return B_OK;
}

status_t BEpsonCommon::Cancel()
{
	// don't forget to call the derived class
	// (this will ensure that OutputData() will not be called anymore.
	// EndPage(), Endjob() and EndDocument() _will_ be called
	fCancelRequested = true;
	return BPrinterRasterAddOn::Cancel();
}


// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

BEpson::BEpson(BTransportIO* transport, BNode *printer_file, image_id image, MDefinePrinter *addon)
	:	BEpsonCommon(transport, printer_file, image, addon),
		fDither(NULL)
{
}

BEpson::~BEpson()
{
	delete fDither;
}

status_t BEpson::BeginJob()
{
	Transport()->SetErrorHandler(this);
	status_t err = BEpsonCommon::BeginJob();
	// Create an MPrinter
	if (err == B_OK)
		err = fPrinter->Init();
	return err;
}

status_t BEpson::EndJob()
{
	D(bug("EPSON: EndJob()\n");)

	if (fCancelRequested)
	{ // Either the user requested to Cancel or something went wrong.
		// Don't try to finish the job. But reset the printer.
		Transport()->SoftReset();
	} else {
		// Terminate the job
		fPrinter->Desinit();
		// Wait all the data are transfered to the printer
		Transport()->Sync();
	}

	// Make sure the error handler won't be called anymore (bullet proofing)
	Transport()->SetErrorHandler(NULL);

	return BEpsonCommon::EndJob();
}

status_t BEpson::BeginPage(const print_bitmap_t *bitmap)
{
	if (fDither == NULL)
	{ // fDither not initialized yet
		if (PrinterDef().soft_microweave == 2)	fDither = new MDitherNSlices(*this, bitmap->w, bitmap->offb_next_pixel/4, bitmap->x_loop, bitmap->y_loop);
		else 									fDither = new MDither360	(*this, bitmap->w, bitmap->offb_next_pixel/4, bitmap->x_loop, bitmap->y_loop);
		fDither->InitDithering();
	}

	// Init the dithering routine for this page
	fDither->DoImage();

	return B_OK;
}

status_t BEpson::EndPage()
{
	D(bug("EPSON: EndPage()\n");)

	// Flush the last lines
	status_t result = fDither->StreamBitmapEnd();

	// ... and eject the page
	if (result == B_OK)
		result = fPrinter->FormFeed();

	return result;
}

void BEpson::EveryLine()
{
	// Update status if needed
	UpdateStatus();
}

status_t BEpson::OutputData(const print_bitmap_t *bitmap)
{
	// Initialize the Dithering with current parameters
	fDither->Init(	fPrinter,
					bitmap->bits.bits32,
					bitmap->offb_next_line/4,
					bitmap->h);

	// Send the raster to the printer
	return fDither->StreamBitmap();
}

bool BEpson::Error(status_t& io_error, bool retry)
{
	D(bug("BEpson::ErrorHandler: %s (%srecoverable)\n", strerror(io_error), retry ? "" : "un");)

	if (retry == false)
	{ // Unrecoverrable error
		printer_status_t status;
		status.status = printer_status_t::B_ERROR;
		ReportPrinterStatus(status);
		return false;
	}
	else
	{ // The error is recoverrable
		// Wait a little (1s) to give time to the printer to process data	
		snooze(ERROR_DELAY);
	
		// make sure the user didn't canceled
		if (CanContinue() == false)
			return false;
	
		// TODO: interract with the user
	
		// An error occurred, try to find what happened
		BMessage msg;
		status_t result = fPrinterUtils->GetStatus(msg);
		printer_status_t status;
		if ((result == B_OK) && !msg.IsEmpty())
		{ // We got the status and it changed
			fPrinterUtils->ConvertToStatus(msg, &status);
			ReportPrinterStatus(status);
		}
		
		// Always need to update the status in case of error
		fStatusUpdateNeeded = true;
	
		// Retry
		return true;
	}
}

// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

BEpsonConfig::BEpsonConfig(BTransportIO* transport, BNode *printer, MDefinePrinter *printer_def)
	: 	BPrinterConfigAddOn(transport, printer),
		fAddonImage(-1),
		fPrinterDev(printer_def),
		fPrinterModes(NULL),
		fPaperFormats(NULL),
		fNbPaperFormat(0),
		fNbPrinterModes(0),
		fPrinterUtils(NULL)
{
	fPrinterStatusCached.time_sec = -1;
	fPrinterStatusCached.status = printer_status_t::B_UNKNOWN;
	fPrinterStatusCached.CMYKcmyk[0] = 0xFF;
	fPrinterStatusCached.CMYKcmyk[1] = 0xFF;
	fPrinterStatusCached.CMYKcmyk[2] = 0xFF;
	fPrinterStatusCached.CMYKcmyk[3] = 0xFF;
	fPrinterStatusCached.CMYKcmyk[4] = 0xFF;
	fPrinterStatusCached.CMYKcmyk[5] = 0xFF;
	fPrinterStatusCached.CMYKcmyk[6] = 0xFF;
	fPrinterStatusCached.CMYKcmyk[7] = 0xFF;

	// Just to set some default values
	Settings().SetSettings(BMessage());	

	if (fPrinterDev == NULL)
		fAddonImage = init_printer_add_on(Printer(), &fPrinterDev);	

	if (fPrinterDev)
	{
		// Get information about this printer
		fPrinterDef.printer_info._size = sizeof(fPrinterDef.printer_info);
		fPrinterDev->GetPrinterInfo(&fPrinterDef.printer_info);

		// Build the paper format list
		int index;
		tPageFormat paper;
		for (fNbPaperFormat=0 ; fPrinterDev->Page(&paper, fNbPaperFormat) == B_OK ; fNbPaperFormat++) { }
		fNbPaperFormat--; // We don't use the 1st entry (userdefined paper)
		index = 1;
		fPaperFormats = new BPrintPaper[fNbPaperFormat];
		int myPaperId = 0;
		for (int32 i=0 ; (fPrinterDev->Page(&paper, index) == B_OK) && (i<fNbPaperFormat) ; i++, index++)
		{
			fPaperFormats[i].SetTo(	BPrintPaper::B_START_USERDEF_PAPER,
									paper.PageName,
									BPrintPaper::pixel_to_milimeter(paper.Width(), 360),
									BPrintPaper::pixel_to_milimeter(paper.Height(), 360),
									BPrintPaper::pixel_to_milimeter(paper.left_margin, 360),
									BPrintPaper::pixel_to_milimeter(paper.top_margin, 360),
									BPrintPaper::pixel_to_milimeter(paper.right_margin, 360),
									BPrintPaper::pixel_to_milimeter(paper.bottom_margin, 360));

			fPaperFormats[i].id = BPrintPaper::FindPaperID(fPaperFormats[i]);
			if (fPaperFormats[i].id == BPrintPaper::B_START_USERDEF_PAPER)
			{ // This paper is specific to this printer, give it a unique private id
				fPaperFormats[i].id == BPrintPaper::B_START_USERDEF_PAPER + myPaperId;
				myPaperId++;
			}
		}

		// Build the resolutions list
		const tRes2Paper *res2paperDefault = NULL;
		fPrinterDev->FindRes2Paper(&res2paperDefault, MDefinePrinter::M_DEFAULT);
		const tRes2Paper *res2paper;
		for (fNbPrinterModes=0 ; fPrinterDev->FindRes2Paper(&res2paper, fNbPrinterModes) == B_OK ; fNbPrinterModes++) { }
		fPrinterModes = new printer_mode_t[fNbPrinterModes];
		for (int32 i=0 ; (fPrinterDev->FindRes2Paper(&res2paper, i)==B_OK) && (i < fNbPrinterModes) ; i++)
		{
			const tPrinterRes *res;
			const tPrinterPaper *paper;
			if ((fPrinterDev->Res(&res, res2paper->res_index) == B_OK) &&
				(fPrinterDev->Paper(&paper, res2paper->paper_index) == B_OK))
			{
				fPrinterModes[i].paper = paper->PaperName;
				fPrinterModes[i].quality = res->ResName;			
				fPrinterModes[i].attributes = printer_mode_t::B_COLOR_ONLY;
				fPrinterModes[i].user = i;
				if (res2paper == res2paperDefault)
				{
					fPrinterModes[i].attributes |= (printer_mode_t::B_IS_DEFAULT_QUALITY | printer_mode_t::B_IS_DEFAULT_PAPER);
					PrinterModeSelected(i); // Set the parameters for the default resolution
				}
			}
		}

		// Find and select the default paper
		PaperSelected(0, 0);
		for (int i=0 ; i<fNbPaperFormat ; i++)
			if (fPaperFormats[i].id == BPrintPaper::DefaultFormat())
				PaperSelected(0, i);
	}
}

BEpsonConfig::~BEpsonConfig()
{
	if (fAddonImage >= 0)
	{ // Delete/unload the add_on only if we loaded ourself
		delete fPrinterDev;
		unload_add_on(fAddonImage);
	}
	
	delete [] fPaperFormats;
	delete [] fPrinterModes;
	delete fPrinterUtils;
}

status_t BEpsonConfig::CleanPrintHeads(bool is_feature_availlable)
{
	if (is_feature_availlable)
		return B_OK;
	if (fPrinterUtils == NULL)
		fPrinterUtils = new MPrinterUtils(Transport(), PrinterDev(), &PrinterDef());
	return fPrinterUtils->CleanNozzles();
}

status_t BEpsonConfig::PrintNozzleCheckPattern(bool is_feature_availlable)
{
	if (is_feature_availlable)
		return B_OK;
	if (fPrinterUtils == NULL)
		fPrinterUtils = new MPrinterUtils(Transport(), PrinterDev(), &PrinterDef());
	return fPrinterUtils->CheckNozzles();
}

status_t BEpsonConfig::PrinterStatus(printer_status_t *status)
{
	if (status == NULL)		// We support the status report feature
		return B_OK;

	if (fPrinterUtils == NULL)
		fPrinterUtils = new MPrinterUtils(Transport(), PrinterDev(), &PrinterDef());

	BMessage msg;
	status_t result = fPrinterUtils->GetStatus(msg);
	if (result != B_OK)
		return result;
	if (msg.IsEmpty())	// The status has not changed
	{
		*status = fPrinterStatusCached;
		return B_OK;
	}
	fPrinterUtils->ConvertToStatus(msg, status);
	fPrinterStatusCached = *status;
	return B_OK;
}

int32 BEpsonConfig::PrinterModes(printer_mode_t const **modes)
{
	*modes = fPrinterModes;
	return fNbPrinterModes;
}

int32 BEpsonConfig::PaperFormats(int32 tray, BPrintPaper const **papers)
{
	if (tray != 0)
		return 0;
	*papers = fPaperFormats;
	return fNbPaperFormat;
}

status_t BEpsonConfig::PrinterModeSelected(int32 index)
{
	const int idx = fPrinterModes[index].user;
	const tRes2Paper *res2paper;
	fPrinterDev->FindRes2Paper(&res2paper, idx);

	// We record the selected mode (should not be used actually)
	Settings().Message().RemoveName("epson:printmode");
	Settings().Message().AddInt32("epson:printmode", idx);

	// Resolution index & value
	const tPrinterRes *res;
	fPrinterDev->Res(&res, res2paper->res_index);
	Settings().Message().RemoveName("epson:res");
	Settings().Message().AddInt32("epson:res", res2paper->res_index);
	Settings().SetDeviceXdpi(res->x_dpi);
	Settings().SetDeviceYdpi(res->y_dpi);
	
	// Paper type index
	const tPrinterPaper *paper;
	Settings().Message().RemoveName("epson:paper");
	Settings().Message().AddInt32("epson:paper", res2paper->paper_index);

	// Color mode
	const tColorMode *color;
	Settings().Message().RemoveName("epson:color_mode");
	if (fPrinterDev->ColorMode(&color, 0) == B_OK)
		Settings().Message().AddInt32("epson:color_mode", color->color_mode);

	// Microweave mode
	const tWeaveMode *weave;
	Settings().Message().RemoveName("epson:weave_mode");
	if (fPrinterDev->WeaveMode(&weave, res2paper->microweave_index) == B_OK)
		Settings().Message().AddInt32("epson:weave_mode", weave->weave_mode);
	Settings().Message().RemoveName("epson:soft_microweave");
	Settings().Message().AddInt32("epson:soft_microweave", res2paper->microweave_index);

	// Speed mode
	const tSpeedMode *speed;
	Settings().Message().RemoveName("epson:speed_mode");
	if (fPrinterDev->SpeedMode(&speed, res2paper->speed_index & 0xF) == B_OK)
		Settings().Message().AddInt32("epson:speed_mode", speed->speed_mode);

	// Nb of hslices
	int32 hslices = (res2paper->speed_index >> 4) & 0xF;
	Settings().Message().RemoveName("epson:hslices");
	Settings().Message().AddInt32("epson:hslices", hslices==0 ? 1 : hslices);

	// Microdot mode
	const tMicroDot *micro;
	Settings().Message().RemoveName("epson:microdot");
	if (fPrinterDev->DotSize(&micro, res2paper->microdot_index) == B_OK)
		Settings().Message().AddInt32("epson:microdot", micro->micro_dot);

	return B_OK;
}

status_t BEpsonConfig::PaperSelected(int32 tray, int32 paper)
{
	BPrinterConfigAddOn::PaperSelected(tray, paper);
	Settings().Message().RemoveName("epson:paperformat");
	Settings().Message().AddInt32("epson:paperformat", paper);
	return B_OK;
}


BPrintConfigView *BEpsonConfig::AddPrinter(const char *printername, BDirectory *spooldir)
{
	return new AddPrinterView(spooldir);
}

status_t BEpsonConfig::AddPrinterQuiet(const char *printername, BDirectory *spooldir)
{ // a printer has been added with plug and play
	BPath addon;
	ssize_t err;
	if ((err = printer_added(Printer(), &addon)) != B_OK)
		return err;
	D(bug("EPSON: add-on Plug&Play name found = %s\n", addon.Path());)
	if (addon.InitCheck() != B_OK)
		return B_NO_DRIVER;
	if ((err = Printer()->WriteAttr("epst:addon_path", B_STRING_TYPE, 0, addon.Path(), strlen(addon.Path())+1)) < 0)
		return err;
	return B_OK;
}

status_t BEpsonConfig::printer_added(BNode *printer_file, BPath *addon)
{ // Must configure the printer with default values and NO GUI
	// We have to find the appropriate driver
	addon->Unset();

	status_t result = B_ERROR;
	BString mdl;

	result = Model(mdl);
	D(bug("EPSON: Model = %s (%s)\n", mdl.String(), strerror(result));)

	if (result != B_OK)
		return result;
	char model[256];
	strcpy(model, mdl.String());	// has to be writtable

	result = B_NO_DRIVER;
	BPath path;
	BMessage refs;

	int nbRefs;
	if ((model[0]) && ((nbRefs = FetchAddonsRefs("Epson", &refs)) > 0))
	{ // Get the list of all addons
		char *s;
		char *q;
		char *p = strdup(model);
		for (s=p ; (*s) && (!isdigit(*s)) ; s++) { /* find the first digit */ }
		for (q=s ; (*q) && (isdigit(*q)) ; q++)  { /* put a 0 at the end */ }
		*q=0;
		D(bug("EPSON: Model id = %s\n", q);)

		for (int i=0 ; i<nbRefs ; i++)
		{ // Find the best addon
			entry_ref ref;
			if (refs.FindRef("refs", i, &ref) == B_OK)
			{ // Is this add-on matching?
				BEntry entry(&ref);
				BPath addonPath(&entry);
				D(bug("EPSON: path = %s\n", addonPath.Path());)
				if (strstr(addonPath.Leaf(), s))
				{ // That's it!
					path = addonPath;
					result = B_OK;
					break;
				}
			}
		}

		// Freeing memory is good
		free(p);
	}

	if (result == B_OK)
		*addon = path;

	return result;
}

int BEpsonConfig::FetchAddonsRefs(const char *path, BMessage *refs)
{
	int nbRefs = 0;
	const int pathToSearch[] = {B_USER_ADDONS_DIRECTORY, /*B_COMMON_ADDONS_DIRECTORY,*/ B_BEOS_ADDONS_DIRECTORY};
	const int nbPAthToSearch = sizeof(pathToSearch)/sizeof(pathToSearch[0]);
	refs->MakeEmpty();
	refs->what = B_REFS_RECEIVED;
	for (int i=0 ; i<nbPAthToSearch ; i++)
	{ // Look in all add-ons directories
		BPath addon_path;
		if (find_directory((directory_which)(pathToSearch[i]), &addon_path) != B_OK)
			continue;
		addon_path.Append(path);
		BEntry entry;
		BDirectory dir(addon_path.Path());
		while (dir.GetNextEntry(&entry, false) == B_OK)
		{ // Look into all addons in that directory
			 // Ignore directories
			if (entry.IsDirectory())
				continue;

			entry_ref ref;
			if (entry.GetRef(&ref) == B_OK)
			{
				// TODO: Should not add the ref if we already have it the list 
				if (refs->AddRef("refs", &ref) == B_OK)
				{
					nbRefs++;
				}
			}
		}
	}
	refs->AddInt32("nb_refs", nbRefs);
	return nbRefs;
}

// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

AddPrinterView::AddPrinterView(BNode *printer)
	: 	BPrintConfigView(BRect(0,0,250,300), "Printer Model", B_FOLLOW_ALL, B_WILL_DRAW),
		fPrinter(printer)
{
	BRect r = Bounds();
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.InsetBy(8,8);
	fPrinterList = new BListView(r, NULL, B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
	AddAddons(B_USER_ADDONS_DIRECTORY);
	AddAddons(B_BEOS_ADDONS_DIRECTORY);
	fPrinterList->Select(0);
	AddChild(new BScrollView(NULL, fPrinterList, B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW, false, true));
}

AddPrinterView::~AddPrinterView()
{
	while (fPrinterList->IsEmpty() == false)
		delete fPrinterList->RemoveItem((int32)0);
}

void AddPrinterView::AttachedToWindow()
{
	BRect r = Parent()->Bounds();
	r.InsetBy(8,8);
	MoveTo(r.LeftTop());
	ResizeTo(r.Width(), r.Height());
	fPrinterList->SetTarget(this);
	fPrinterList->SetSelectionMessage(new BMessage('list'));
	BPrintConfigView::AttachedToWindow();
	fPrinterList->MakeFocus();
}

void AddPrinterView::GetMinimumSize(float *width, float *height)
{
	*width = 8*2 + 250;
	*height = 8*2 + B_V_SCROLL_BAR_WIDTH + 80;
}

status_t AddPrinterView::Save()
{
	uint32 printer_index = fPrinterList->CurrentSelection();
	if (printer_index < 0)
		return B_BAD_VALUE;

	BEpsonItem *selected_item = static_cast<BEpsonItem *>(fPrinterList->ItemAt(printer_index));
	ssize_t err = Printer()->WriteAttr("epst:addon_path", B_STRING_TYPE, 0, selected_item->fValue.Path(), strlen(selected_item->fValue.Path())+1);
	if (err < 0)
		return err;

	return B_OK;
}

void AddPrinterView::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case 'list':
		{
			int32 index;
			message->FindInt32("index", &index);
			SetSaveEnabled((index>=0));
			break;
		}
		default:
			BPrintConfigView::MessageReceived(message);
	}
}

status_t AddPrinterView::AddAddons(directory_which which)
{
	BPath path;
	status_t err = find_directory(which, &path);
	if (err == B_OK)
	{
		path.Append("Epson");
		BDirectory dir(path.Path());
		if (dir.InitCheck() == B_OK)
		{
			char name[B_FILE_NAME_LENGTH];
			BEntry entry;
			while (dir.GetNextEntry(&entry) == B_OK)
			{
				BFile file(&entry, B_READ_ONLY);
				BAppFileInfo fileInfo(&file);
				char signature[256];
				if (fileInfo.GetSignature(signature) == B_OK)
				{
					if (strcmp(signature, M_XPRINTER_ADDON_SIGNATURE) == 0)
					{
						entry.GetName(name);
						const int32 nb = fPrinterList->CountItems();
						int32 i;
						for (i=0 ; i<nb ; i++)
						{
							BEpsonItem *item = static_cast<BEpsonItem *>(fPrinterList->ItemAt(i));
							if (strcmp(item->Name(), name) == 0)
								break;
						}
						if (i == nb)
						{
							// Get the realname of the printer
							BPath p;
							entry.GetPath(&p);
							MDefinePrinter *info;
							image_id image = init_epson_add_on(p.Path(), &info);
							if (image >= 0) {
								fPrinterList->AddItem( new BEpsonItem(info->Name(), p, name) );
								delete info;
								unload_add_on(image);
							}
						}
					}
				}
			}
		}
	}
	return B_OK;
}

// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

// ============================================================================================
// Function name: init_printer_add_on
// Purpose: Chargement de l'add_on imprimante, initialisation des ressources
// Input: printer file
// ============================================================================================

image_id init_printer_add_on(BNode *printer_file, MDefinePrinter **define_printer)
{
	image_id addon_image = B_ERROR;
	*define_printer = NULL;
	if (printer_file)
	{
		D(bug("EPSON: We have a Printer()\n");)
		char addon_path[B_FILE_NAME_LENGTH];
		ssize_t err = printer_file->ReadAttr("epst:addon_path", B_STRING_TYPE, 0, addon_path, sizeof(addon_path));
		if (err >= 0)
		{
			addon_image = init_epson_add_on(addon_path, define_printer);
		} else {
			D(bug("EPSON: epson-addon not found!\n");)
		}
	}
	return addon_image;
}


image_id init_epson_add_on(const char *addon_path, MDefinePrinter **define_printer)
{
	image_id addon_image = load_add_on(addon_path);
	D(bug("EPSON: Addon found -> %s\n", addon_path);)
	D(bug("EPSON: load_add_on returned %08lx (%s)\n", addon_image, addon_image<0 ? strerror(addon_image) : "ok");)
	if (addon_image > 0)
	{	
		// Find the instantiation functions
		MDefinePrinter* (* instantiate_MDefinePrinter)(); 
		status_t res = get_image_symbol(addon_image, "instantiate_MDefinePrinter", B_SYMBOL_TYPE_TEXT, (void **)&instantiate_MDefinePrinter);
		if (res == B_OK)
		{ // Instantiate the object
			*define_printer = instantiate_MDefinePrinter();
			D(bug("EPSON: fPrinterDev = %p\n", *define_printer);)
		} else {
			D(bug("EPSON: instantiate_MDefinePrinter not found in  %s\n", addon_path);)
			unload_add_on(addon_image);
			return res;
		}
	}
	return addon_image;
}


