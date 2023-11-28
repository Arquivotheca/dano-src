//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

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
#include <USB_printer.h>

#include <support/memadviser.h>

#include "debug.h"
#include "DeskjetBeOS.h"

// HP stuffs
#include "Header.h"
#include "IO_defs.h"
#include "resources.h"
#include "Printer.h"

extern "C" BPrinterAddOn *instantiate_printer_addon(int32 index, BTransportIO* transport, BNode *printer_file)
{
	if (index != 0)	return NULL;
	return static_cast<BPrinterAddOn *>(new BDeskjet(transport, printer_file));
}

extern "C" BPrinterConfigAddOn *instantiate_printer_config_addon(BTransportIO* transport, BNode *printer_file)
{
	return static_cast<BPrinterConfigAddOn *>(new BDeskjetConfig(transport, printer_file));
}



// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

status_t DeskjetTools::ConvertErrorCodes(int hperror) const
{
	switch (hperror)
	{
		case NO_ERROR:				return B_OK;
		case JOB_CANCELED:			return B_CANCELED;
		case SYSTEM_ERROR:			return B_ERROR;
		case ALLOCMEM_ERROR:		return B_NO_MEMORY;
		case NO_PRINTER_SELECTED:	return B_NO_PRINTER;
		case INDEX_OUT_OF_RANGE:	return B_BAD_INDEX;
		case ILLEGAL_RESOLUTION:	return B_BAD_VALUE;
		case NULL_POINTER:			return B_BAD_VALUE;
		case UNSUPPORTED_PRINTER:	return B_NO_PRINTER;
		case UNSUPPORTED_PEN:		return B_BAD_VALUE;
		case TEXT_UNSUPPORTED:		return B_BAD_VALUE;
		case GRAPHICS_UNSUPPORTED:	return B_BAD_VALUE;
		case UNSUPPORTED_FONT:		return B_BAD_VALUE;
		case ILLEGAL_COORDS:		return B_BAD_VALUE;
		case UNSUPPORTED_FUNCTION:	return B_BAD_VALUE;
		case IO_ERROR:				return B_IO_ERROR;
		case BAD_DEVICE_ID:			return B_FILE_ERROR;
		default:					return B_ERROR;
	}
}


// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

BDeskjetConfig::BDeskjetConfig(BTransportIO* transport, BNode *printer)
	: 	BPrinterConfigAddOn(transport, printer),
		fInitCheck(B_OK),
		fDeskjetTools(NULL),
		fSystem(NULL),
		fPrintContext(NULL),
		fPrinterModes(NULL),
		fPrinterModesCount(0),
		fBeToHpPrintModes(NULL)
{
	fDeskjetTools = new DeskjetTools(transport);

	fSystem = new BSystemServices(fDeskjetTools);
	if (fSystem->constructor_error) {
		fInitCheck = fDeskjetTools->ConvertErrorCodes(fSystem->constructor_error);
		return;
	}

	fPrintContext = new PrintContext(fSystem);
	if (fPrintContext->constructor_error) {
		fInitCheck = fDeskjetTools->ConvertErrorCodes(fPrintContext->constructor_error);
		return;
	}		

	int32 deskjetType;
	ssize_t result = Printer()->ReadAttr("DJTYPE", B_INT32_TYPE, 0, &deskjetType, sizeof(int32));
	if ((result == sizeof(int32)) && (deskjetType != -1)) {
		const int hpresult = fPrintContext->SelectDevice((PRINTER_TYPE)deskjetType);
		fInitCheck = fDeskjetTools->ConvertErrorCodes(hpresult);
	}

	// After SelectDevice
	fSystem->InitDevice();

	// Just to set some default values
	Settings().SetSettings(BMessage());	

	// Fetch the printer modes
	fPrintContext->SetInputResolution(150);
	const printer_mode_t *dummy;
	PrinterModes(&dummy);

	// We set our default settings
	fPrintContext->SelectPrintMode(1);	// Default basic mode
	const float resolution = min_c(fPrintContext->EffectiveResolutionX(), fPrintContext->EffectiveResolutionY());
	fPrintContext->SetInputResolution(resolution);

	// compute the paper sizes
	// choose the paper size
	set_paper_format(BPrintPaper::B_LETTER, LETTER, 0);
	set_paper_format(BPrintPaper::B_A4,	A4,	1);
	set_paper_format(BPrintPaper::B_LEGAL, LEGAL, 2);
	set_paper_format(HP_PHOTO_SIZE, PHOTO_SIZE, 3, "Photo size");
    fPrintContext->SetPaperSize(LETTER);
	PaperSelected(0, 0);

	Settings().SetDeviceXdpi(fPrintContext->EffectiveResolutionX());	// printer's resolution
	Settings().SetDeviceYdpi(fPrintContext->EffectiveResolutionY());
}

BDeskjetConfig::~BDeskjetConfig()
{
	delete fPrintContext;
	delete fSystem;
	delete fDeskjetTools;
	delete [] fPrinterModes;
	delete [] fBeToHpPrintModes;
	fPrinterModesCount = 0;
}


int32 BDeskjetConfig::PrinterModes(printer_mode_t const **modes)
{
	if (fPrinterModes) {
		// We already know the printer modes
	 	*modes = fPrinterModes;
		return fPrinterModesCount;
	}

	const uint32 c = fPrintContext->GetModeCount();
	fPrinterModes = new printer_mode_t[c];
	static const char *paper = "Plain paper";
	bool defaultMode = false;

	delete [] fBeToHpPrintModes;
	fBeToHpPrintModes = new int[c];

	int index = 0;
	for (int i=0 ; i<c ; i++)
	{
		if (fPrintContext->SelectPrintMode(i) != NO_ERROR)
			continue;
		fPrinterModes[index].paper = paper;
		fPrinterModes[index].quality = fPrintContext->GetModeName();
		fPrinterModes[index].attributes = printer_mode_t::B_COLOR_ONLY;
		if (i == 0) { // 0 is the grayscale
			fPrinterModes[index].attributes = printer_mode_t::B_BLACK_ONLY;
		} else if (i == 1) { // 1 is the default
			fPrinterModes[index].attributes |= printer_mode_t::B_IS_DEFAULT_PAPER | printer_mode_t::B_IS_DEFAULT_QUALITY;
			defaultMode = true;
		}
		fBeToHpPrintModes[index] = i;
		index++;
	}
	if (index == 0) { // huh! No modes available!
		delete [] fPrinterModes;
		fPrinterModes = NULL;
		delete [] fBeToHpPrintModes;
		fBeToHpPrintModes = NULL;
		return 0;
	}

	if (defaultMode == false) { // We didn't get the default mode
		for (int i=0 ; i<index ; i++) { // Try to find a default color mode 
			if ((fPrinterModes[i].attributes & printer_mode_t::B_BLACK_ONLY) == false) {
				fPrinterModes[i].attributes |= printer_mode_t::B_IS_DEFAULT_PAPER | printer_mode_t::B_IS_DEFAULT_QUALITY;
				defaultMode = true;
			}
		}
	}

	if (defaultMode == false) { // We still don't have a default mode!
		fPrinterModes[0].attributes |= printer_mode_t::B_IS_DEFAULT_PAPER | printer_mode_t::B_IS_DEFAULT_QUALITY;
	}

 	fPrinterModesCount = index;
 	*modes = fPrinterModes;
	return fPrinterModesCount;
}

status_t BDeskjetConfig::PrinterModeSelected(int32 index)
{
	if (fBeToHpPrintModes == NULL)
		return B_BAD_VALUE;
	fPrintContext->SelectPrintMode(fBeToHpPrintModes[index]);
	Settings().SetDeviceXdpi(fPrintContext->EffectiveResolutionX());
	Settings().SetDeviceYdpi(fPrintContext->EffectiveResolutionY());
	Settings().Message().RemoveName("hp:printmode");
	Settings().Message().AddInt32("hp:printmode", fBeToHpPrintModes[index]);
	return B_OK;
}

status_t BDeskjetConfig::PaperSelected(int32 tray, int32 paper)
{
	BPrinterConfigAddOn::PaperSelected(tray, paper);
	Settings().Message().RemoveName("hp:paperformat");
	Settings().Message().AddInt32("hp:paperformat", paper);
	return B_OK;
}

int32 BDeskjetConfig::PaperFormats(int32 tray, BPrintPaper const **papers)
{
	if (tray != 0)
		return 0;
	*papers = fPaperFormats;
	return sizeof(fPaperFormats)/sizeof(fPaperFormats[0]);
}


void BDeskjetConfig::set_paper_format(int id, PAPER_SIZE hpid, int index, const char *name)
{
    fPrintContext->SetPaperSize(hpid);
	float w = fPrintContext->PhysicalPageSizeX() * 25.4f;
	float h = fPrintContext->PhysicalPageSizeY() * 25.4f;
	float l = fPrintContext->PrintableStartX() * 25.4f;
	float t = fPrintContext->PrintableStartY() * 25.4f;
	float r = w - (l + fPrintContext->PrintableWidth() * 25.4f);
	float b = h - (t + fPrintContext->PrintableHeight() * 25.4f);
	fPaperFormats[index].SetTo(id, name, w, h, l, t, r, b);
}


BPrintConfigView *BDeskjetConfig::AddPrinter(const char *printername, BDirectory *spooldir)
{
	return new AddPrinterView(fPrintContext, spooldir);
}




status_t BDeskjetConfig::CleanPrintHeads(bool is_feature_availlable=false)
{
	if (is_feature_availlable == true)
		return B_OK;
	return fDeskjetTools->ConvertErrorCodes(fPrintContext->PerformPrinterFunction(CLEAN_PEN));
}

status_t BDeskjetConfig::PrintNozzleCheckPattern(bool is_feature_availlable=false)
{
	return BPrinterConfigAddOn::PrintNozzleCheckPattern(is_feature_availlable);
}

status_t BDeskjetConfig::PrinterStatus(printer_status_t *status)
{
	if (status == NULL)
		return B_OK;

	status->status = fDeskjetTools->fPrinterStatus;
	status->time_sec = -1;
	status->CMYKcmyk[0] = 0xFF;
	status->CMYKcmyk[1] = 0xFF;
	status->CMYKcmyk[2] = 0xFF;
	status->CMYKcmyk[3] = 0xFF;
	status->CMYKcmyk[4] = 0xFF;
	status->CMYKcmyk[5] = 0xFF;
	status->CMYKcmyk[6] = 0xFF;
	status->CMYKcmyk[7] = 0xFF;
	return B_OK;
}


// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

BDeskjet::BDeskjet(BTransportIO* transport, BNode *printer_file)
	:	BPrinterRasterAddOn(transport, printer_file, 0, 0),
		fInitCheck(B_OK),
		fSystem(NULL),
		fPrintContext(NULL),
		fJob(NULL),
		fTempBuffer(NULL)
{
	fDeskjetTools = new DeskjetTools(transport, this);
}

BDeskjet::~BDeskjet()
{
	// Make sure we freed all memory
	delete fJob;
	delete fPrintContext;
	delete fSystem;
	delete [] fTempBuffer;
	delete fDeskjetTools;
}

status_t BDeskjet::ConvertErrorCodes(int hperror)
{
	return fDeskjetTools->ConvertErrorCodes(hperror);
}

status_t BDeskjet::BeginJob()
{
	BPrinterRasterAddOn::BeginJob();

	Transport()->SetErrorHandler(this);

	// Setup the HP engine
	fSystem = new BSystemServices(fDeskjetTools);
	if (fSystem->constructor_error)
		return ConvertErrorCodes(fSystem->constructor_error);	

	// Make sure to display DISPLAY_PRINTING
	fSystem->DisplayPrinterStatus(DISPLAY_PRINTING);

	return B_OK;
}

status_t BDeskjet::EndJob()
{
	if (fDeskjetTools->fCancelRequested)
	{ // There was an error, reset the printer
		// Soft reset doesn't work with Apollo printers!!
		//Transport()->SoftReset();
	}
	else
	{ // Be sure all the data have been sent
		Transport()->Sync();
	}
	
	// Be sure that the error handler won't be called
	Transport()->SetErrorHandler(NULL);

	// Make sure to display OFFLINE after the end of the printjob
	fSystem->DisplayPrinterStatus(DISPLAY_OFFLINE);

	delete fJob;			fJob = NULL;
	delete fPrintContext;	fPrintContext = NULL;
	delete fSystem;			fSystem = NULL;
	delete [] fTempBuffer;	fTempBuffer = NULL;
	return 	BPrinterRasterAddOn::EndJob();
}

status_t BDeskjet::BeginPage(const print_bitmap_t *bitmap)
{
	if (fTempBuffer == NULL)
	{
		fTempBuffer = new BYTE[bitmap->w * bitmap->x_loop * 3];
		
		// Read the paper format
		int32 paperformat;
		if (Settings().Message().FindInt32("hp:paperformat", &paperformat) != B_OK)
			paperformat = LETTER;
	
		// TODO: Here we could use the HP's internal rescaler (which is filtered), but
		// in this case we must have xscale == yscale > 1
	
		fPrintContext = new PrintContext(fSystem, bitmap->w*bitmap->x_loop, bitmap->w*bitmap->x_loop, (PAPER_SIZE)paperformat);
		if (fPrintContext->constructor_error)
			return ConvertErrorCodes(fPrintContext->constructor_error);	
	
		// Select the printer
		int32 deskjetType;
		ssize_t result = Printer()->ReadAttr("DJTYPE", B_INT32_TYPE, 0, &deskjetType, sizeof(int32));
		if ((result == sizeof(int32)) && (deskjetType != -1))
			fPrintContext->SelectDevice((PRINTER_TYPE)deskjetType);
	
		// After SelectDevice
		fSystem->InitDevice();

		int32 index;
		if (Settings().Message().FindInt32("hp:printmode", &index) != B_OK)
			index = 1;
		fPrintContext->SelectPrintMode(index);	// Default basic mode
		const float resolution = min_c(fPrintContext->EffectiveResolutionX(), fPrintContext->EffectiveResolutionY());
		fPrintContext->SetInputResolution(resolution);
	
		fJob = new Job(fPrintContext);
		if (fJob->constructor_error)
			return ConvertErrorCodes(fJob->constructor_error);	
	}

	return BPrinterRasterAddOn::BeginPage(bitmap);
}

status_t BDeskjet::EndPage()
{
	fJob->NewPage();
	return B_OK;
}

status_t BDeskjet::OutputData(const print_bitmap_t *bitmap)
{
	uint32 *p = bitmap->bits.bits32;
	for (int y=0 ; (y<bitmap->h) && (CanContinue()) ; y++)
	{
		bool notempty = false;
		for (int x=0 ; x<bitmap->w ; x++)
		{
			uint32 pixel = *(p + x*(bitmap->offb_next_pixel/4));
			notempty = notempty || ((pixel & 0x00FFFFFF) != 0x00FFFFFF);	// REVIST: Endienness problem here
			for (int xloop=0 ; xloop < bitmap->x_loop ; xloop++)
			{
				const int offset = (x * bitmap->x_loop + xloop) * 3;
				fTempBuffer[offset+0] = (pixel >> 16) & 0xFF;
				fTempBuffer[offset+1] = (pixel >> 8) & 0xFF;
				fTempBuffer[offset+2] = pixel  & 0xFF;
			}
		}
		for (int yloop=0 ; yloop < bitmap->y_loop ; yloop++)
		{
			int hperr;		
			if ((hperr = fJob->SendRasters(notempty ? fTempBuffer : NULL)) != NO_ERROR)
				return ConvertErrorCodes(hperr);
		}
		p += (bitmap->offb_next_line)/4;
	}	
	return B_OK;
}

status_t BDeskjet::GetSupportedDeviceID(uint32 *count, printer_id_t **ids)
{
	static printer_id_t printerID[] = { {"HEWLETT-PACKARD", NULL}, {"APOLLO", NULL} };
	*count = sizeof(printerID)/sizeof(printerID[0]);
	*ids = printerID;
	return B_OK;
}

status_t BDeskjet::Cancel()
{
	fDeskjetTools->fCancelRequested = true;
	return BPrinterRasterAddOn::Cancel();	
}

bool BDeskjet::Error(status_t& io_error, bool retry)
{
	if (retry == false)
	{ // unrecoverable error
		printer_status_t status;
		status.status = printer_status_t::B_ERROR;
		ReportPrinterStatus(status);
		return false;
	}
	else
	{
		// Never retry, it as already been done.
		return fPrintContext->GetPrinter().Error(retry);
	}
}

// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

AddPrinterView::AddPrinterView(PrintContext *context, BNode *printer)
	: 	BPrintConfigView(BRect(0,0,250,300), "Printer Model", B_FOLLOW_ALL, B_WILL_DRAW),
		fDeskJetType(-1),
		fPrinter(printer)
{
	BRect r = Bounds();
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.InsetBy(8,8);

	fPrinterList = new BListView(r, NULL, B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
	PRINTER_TYPE type;
	fPrinterList->AddItem(new BDJItem("Automatic selection", -1));
	unsigned int currIdx = 0;
	while ((type = context->EnumDevices(currIdx)) != UNSUPPORTED)
	{
		const char *name = context->PrintertypeToString(type);
		fPrinterList->AddItem(new BDJItem(name, (int32)type));
	}
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
	BDJItem *item = static_cast<BDJItem *>(fPrinterList->ItemAt(fPrinterList->CurrentSelection()));
	if (item == NULL)	fDeskJetType = -1;
	else				fDeskJetType = item->fValue;
	ssize_t result = Printer()->WriteAttr("DJTYPE", B_INT32_TYPE, 0, &fDeskJetType, sizeof(int32));
	if (result != sizeof(int32))
		return (status_t)result;
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

// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

BSystemServices::BSystemServices(DeskjetTools *deskjet)
	: 	SystemServices(),
		fDeskjet(deskjet)
{
	#if defined(CAPTURE)
	InitScript("/boot/home/hp_debug.txt", true, false);
	#endif
	constructor_error = InitDeviceComm();
	IOMode.bUSB = ((fDeskjet->Transport()->GetAttributes() & BTransportAddOn::B_TRANSPORT_IS_USB) != 0);
	BYTE status_reg;
	if ((IOMode.bStatus) && (GetStatusInfo(&status_reg) == false))
		IOMode.bStatus = false;		// We need that to avoid problem with File transport for eg.
}

BSystemServices::~BSystemServices()
{
	#if defined(CAPTURE)
	EndScript();
	#endif
}

void BSystemServices::InitDevice()
{
}

BOOL BSystemServices::PrinterIsAlive()
{ // Try to get the DeviceID
	BYTE status_reg;
	if (GetStatusInfo(&status_reg) != B_OK)
		return true; // GetPortStatus() is not availlable, assume the printer is alive
	#define DJ6XX_OFF		(0xF8)
	#define DJ400_OFF		(0xC0)
	#define DJ400_OFF_BOGUS	(0xC8)	// sometimes the DJ400 reports a status byte of C8 when it's turned off
	#define DEVICE_IS_OK(reg) (!((reg == DJ6XX_OFF) || (reg == DJ400_OFF) || (reg == DJ400_OFF_BOGUS)))
	#if defined(DEBUG) && (DBG_MASK & DBG_LVL1)
		if (DEVICE_IS_OK(status_reg))	DBG1("PrinterIsAlive: returning TRUE\n");
		else							DBG1("PrinterIsAlive: returning FALSE\n");
	#endif
	return (DEVICE_IS_OK(status_reg));
}

void BSystemServices::DisplayPrinterStatus(DISPLAY_STATUS ePrinterStatus)
{
	switch (ePrinterStatus)
	{
		case DISPLAY_PRINTING:
			fDeskjet->fPrinterStatus = printer_status_t::B_PRINTING;
			DBG1("BSystemServices::DisplayPrinterStatus: DISPLAY_PRINTING\n");
			break;

		case DISPLAY_BUSY:
			fDeskjet->fPrinterStatus = printer_status_t::B_PRINTING;
			DBG1("BSystemServices::DisplayPrinterStatus: DISPLAY_BUSY\n");
			break;

		case DISPLAY_COMM_PROBLEM:
			DBG1("BSystemServices::DisplayPrinterStatus: DISPLAY_COMM_PROBLEM\n");
			fDeskjet->fPrinterStatus = printer_status_t::B_ERROR;
			break;
		
		case DISPLAY_OUT_OF_PAPER:
			fDeskjet->fPrinterStatus = printer_status_t::B_NO_PAPER;
			DBG1("BSystemServices::DisplayPrinterStatus: DISPLAY_OUT_OF_PAPER\n");
			break;			
		case DISPLAY_TOP_COVER_OPEN:
			fDeskjet->fPrinterStatus = printer_status_t::B_COVER_OPEN;
			DBG1("BSystemServices::DisplayPrinterStatus: DISPLAY_TOP_COVER_OPEN\n");
			break;			
		case DISPLAY_OFFLINE:
			fDeskjet->fPrinterStatus = printer_status_t::B_OFFLINE;
			DBG1("BSystemServices::DisplayPrinterStatus: DISPLAY_OFFLINE\n");
			break;			
		case DISPLAY_NO_COLOR_PEN:
		case DISPLAY_NO_BLACK_PEN:
		case DISPLAY_NO_PENS:
		case DISPLAY_NO_PEN_DJ400:
		case DISPLAY_NO_PEN_DJ600:
			fDeskjet->fPrinterStatus = printer_status_t::B_NO_INK;
			DBG1("BSystemServices::DisplayPrinterStatus: DISPLAY_NO_PENS\n");
			break;			
		case DISPLAY_ERROR_TRAP:
			fDeskjet->fPrinterStatus = printer_status_t::B_PAPER_JAM;
			DBG1("BSystemServices::DisplayPrinterStatus: DISPLAY_ERROR_TRAP\n");
			break;			
		case DISPLAY_PRINTING_CANCELED:
			fDeskjet->fPrinterStatus = printer_status_t::B_OFFLINE;
			DBG1("BSystemServices::DisplayPrinterStatus: DISPLAY_PRINTING_CANCELED\n");
			break;
		case DISPLAY_PHOTO_PEN_WARN:
		case DISPLAY_PRINTER_NOT_SUPPORTED:
		case DISPLAY_CANT_ID_PRINTER:
		default:
			fDeskjet->fPrinterStatus = printer_status_t::B_UNKNOWN;
			DBG1("BSystemServices::DisplayPrinterStatus: DISPLAY_UNKNOWN\n");
	}
	
	if (fDeskjet->Deskjet())
	{ // Send the status to the driver
		printer_status_t status;
		status.status = fDeskjet->fPrinterStatus;
		fDeskjet->Deskjet()->ReportPrinterStatus(status);
	}
}

DRIVER_ERROR BSystemServices::BusyWait(DWORD msec)
{
	if (fDeskjet->Deskjet())
		fDeskjet->Deskjet()->CanContinue();

	if (fDeskjet->fCancelRequested)
		return JOB_CANCELED;
	snooze(msec*1000);
	return NO_ERROR;
}

DRIVER_ERROR BSystemServices::ReadDeviceID(BYTE* strID, int iSize)
{
	BString devid;
	status_t err = B_ERROR;	
	for (int i=0 ; i<5 ; i++) { // sometimes HP printers returns an error here. So just retry!
		if ((err = fDeskjet->Transport()->GetDeviceID(devid)) == B_OK) {
			// Sometimes the Apollo printers return an empty device id! Just retry!
			if (devid.Length() > 0) {
				break;
			}
		}
		// Apparently the 6xx serie needs some time to rest.
		snooze(250000);
	}
	if (err != B_OK)
		return err;
	memcpy(strID, devid.String(), min_c(iSize, devid.Length()+1));
	return NO_ERROR;
}

BYTE* BSystemServices::AllocMem(int iMemSize)
{
	BYTE *buffer;
	if (madv_reserve_memory(iMemSize, "BSystemServices::AllocMem"))
		buffer = (BYTE *)::malloc(iMemSize);
	madv_finished_allocating(iMemSize);
	return buffer;
}

void BSystemServices::FreeMem(BYTE* pMem)
{
	::free((void *)pMem);
}

BOOL BSystemServices::GetStatusInfo(BYTE* bStatReg)
{
	return (fDeskjet->Transport()->GetPortStatus(bStatReg) == B_OK);
}

DRIVER_ERROR BSystemServices::ToDevice(const BYTE* pBuffer, DWORD* wCount)
{
	ssize_t sent = fDeskjet->Transport()->Write((void *)pBuffer, (ssize_t)*wCount);
	if (sent == B_CANCELED)
		return JOB_CANCELED;
	if (sent < 0)
		return IO_ERROR;
	*wCount -= (DWORD)sent;
	return NO_ERROR;
}

DRIVER_ERROR BSystemServices::FromDevice(char* pReadBuff, WORD* wReadCount)
{
	ssize_t read = fDeskjet->Transport()->Read((void *)pReadBuff, (ssize_t)*wReadCount);
	if (read == B_CANCELED)
		return JOB_CANCELED;
	if (read < 0)
		return IO_ERROR;
	*wReadCount -= (WORD)read;
	return NO_ERROR;
}

BOOL BSystemServices::YieldToSystem (void)
{ // function called from time to time. We handle Cancel here.
	return fDeskjet->fCancelRequested;
}

BYTE BSystemServices::GetRandomNumber()
{
	int32 alea = crc_alea;	
	alea <<= 1;
	if (alea < 0)
		alea ^= CRC_ALEA_STEP;
	crc_alea = alea;
	return (alea >> 24);
}

DWORD BSystemServices::GetSystemTickCount(void)
{
	return (DWORD)(system_time()/1000);
}

float BSystemServices::power(float x, float y)
{
	return powf(x,y);
}

DRIVER_ERROR BSystemServices::FlushIO()
{
	if (fDeskjet->Transport()->Sync() != B_OK)
		return IO_ERROR;
	return NO_ERROR;
}

DRIVER_ERROR BSystemServices::AbortIO()
{
	return NO_ERROR;
}

// #######################################################################################
// #pragma mark -
//
// Gross hack here: reimplement HP's Send function. We must not try to handle errors here.

DRIVER_ERROR Printer::Send(const BYTE* pWriteBuff, unsigned long wWriteCount)
{
	if (ErrorTerminationState) 	// don't try any more IO if we previously
		return JOB_CANCELED;	// terminated in an error state

	if (bCheckForCancelButton && 
	    (ulBytesSentSinceCancelCheck >= CANCEL_BUTTON_CHECK_THRESHOLD) )
	{
		ulBytesSentSinceCancelCheck = 0;
		char* tmpStr;
		BYTE DevIDBuffer[DevIDBuffSize];
		DRIVER_ERROR tmpErr = pSS->ReadDeviceID(DevIDBuffer, DevIDBuffSize);
		if(tmpErr)
			return tmpErr;
		if((tmpStr = strstr((char*)DevIDBuffer + 2,"CNCL")))
		{
			DBG1("DJ970 CANCEL BUTTON PRESSED BY USER\n");
			// Since the printer is now just throwing data away, we can bail
            // immediately w/o worrying about finishing the raster so the
            // end-of-job FF will work.
            ErrorTerminationState = TRUE;
			pSS->DisplayPrinterStatus(DISPLAY_PRINTING_CANCELED);
            return JOB_CANCELED;
		}
	}

	DRIVER_ERROR write_error = NO_ERROR;
	DWORD residual = (DWORD)wWriteCount;
	while (residual > 0)	// while still data to send in this request
	{
		const BYTE *pWritePos = (const BYTE *) &(pWriteBuff[(WORD)wWriteCount-residual]);
        write_error = pSS->ToDevice(pWritePos, &residual);
        if (write_error == JOB_CANCELED)
        {
            DBG1("Send: JOB_CANCELED from ToDevice.\n");
            pSS->DisplayPrinterStatus(DISPLAY_PRINTING_CANCELED);
			ErrorTerminationState = true;
			return JOB_CANCELED;
        }
        else if (write_error != NO_ERROR) // Presumably a PORT_ERROR
        {
            DBG1("Send: Error returned from ToDevice.  Bailing...\n");
            ErrorTerminationState = TRUE;
            return IO_ERROR;
        } 
	}
	return write_error;
}




bool Printer::Error(bool retry)
{
	// Try to understand what the problem is and display it

	BYTE status_reg = 0;
	DISPLAY_STATUS eDisplayStatus = DISPLAY_PRINTING;
	if (IOMode.bStatus)
	{
		pSS->GetStatusInfo(&status_reg);	
		eDisplayStatus = ParseError(status_reg);

		switch (eDisplayStatus)
		{
			case DISPLAY_PRINTING_CANCELED:
				// user canceled in an error state,
				// so we don't want to attempt any
				// further communication with the printer		
				ErrorTerminationState = TRUE;		
				pSS->DisplayPrinterStatus(eDisplayStatus);
				return false;
		
			case DISPLAY_ERROR_TRAP:
			case DISPLAY_COMM_PROBLEM:
				// these are unrecoverable cases
				// don't let any more of this job
				// be sent to the printer		
				ErrorTerminationState = TRUE;
				pSS->DisplayPrinterStatus(eDisplayStatus);
				return false;
		
			case DISPLAY_TOP_COVER_OPEN:
				pSS->DisplayPrinterStatus(DISPLAY_TOP_COVER_OPEN);
		
				// wait for top cover to close
				while (eDisplayStatus == DISPLAY_TOP_COVER_OPEN)
				{
					// although we'll leave an incomplete job in the printer,
					// we really need to bail for proper CANCEL response.
					if (pSS->BusyWait((DWORD)500) == JOB_CANCELED)
					{
						ErrorTerminationState = TRUE;
						return false;
					}
					pSS->GetStatusInfo(&status_reg);
					eDisplayStatus = ParseError(status_reg);
				}
		
				// Now that the top is closed, the printer
				// will report OFFLINE for a while.  Displaying
				// OFFLINE might confuse the user, so clear the
				// error and wait for the printer to come online.
				// If it doesn't after a while, report OFFLINE.
				// The delay was determined by experiment.		
				pSS->DisplayPrinterStatus(DISPLAY_PRINTING);
		
				for (int i=0 ; i<10 ; i++)
				{
					pSS->GetStatusInfo(&status_reg);

					// note: we will call ParseError
					// with no error. Since we are just
					// looking for it to leave OFFLINE,
					// that's not a problem (we won't display
					// any error return right now)					
					if (ParseError(status_reg) != DISPLAY_OFFLINE)
						return true;
					
					if (pSS->BusyWait((DWORD)1000) == JOB_CANCELED)
					// Since the top_cover error HAS been handled, we have
					// the opportunity to finish the raster before we hit
					// the next slowpoll threshold.
					{
						DBG1("Send: JOB_CANCELED\n");
						pSS->DisplayPrinterStatus(DISPLAY_PRINTING_CANCELED);
						return false;
					}
				}
				// The top cover is closed but something went wrong
				break;
		
			case DISPLAY_OUT_OF_PAPER:
				pSS->DisplayPrinterStatus(DISPLAY_OUT_OF_PAPER);
				// wait for the user to add paper and
				// press resume
				while (eDisplayStatus == DISPLAY_OUT_OF_PAPER)
				{
					// although we'll leave an incomplete job in the printer,
					// we really need to bail for proper CANCEL response.
					if (pSS->BusyWait((DWORD)500) == JOB_CANCELED)
					{
						ErrorTerminationState = TRUE;
						return false;
					}
					pSS->GetStatusInfo(&status_reg);
					eDisplayStatus = ParseError(status_reg);
				}
				pSS->DisplayPrinterStatus(DISPLAY_PRINTING);
				return true;
		
			case DISPLAY_BUSY:
				if (pSS->BusyWait((DWORD)5000) == JOB_CANCELED)
				{
					ErrorTerminationState = TRUE;
					return false;
				}
				pSS->DisplayPrinterStatus(DISPLAY_BUSY);
				return true;
		
			// other cases need no special handling, display
			// the error and try to continue
			default:
				pSS->DisplayPrinterStatus(eDisplayStatus);
				break;
		}
	}

	if (retry)
	{
		// give printer time to digest the data and check for 'cancel' before
		// the next iteration of the loop
		if (pSS->BusyWait((DWORD)1000) == JOB_CANCELED)
		{
			DBG1("Send: JOB_CANCELED\n");
			pSS->DisplayPrinterStatus(DISPLAY_PRINTING_CANCELED);
			return false;
		}
	    return true;
	}
	
	return false;
}


//#if defined(DEBUG) && (DBG_MASK & DBG_ASSERT)

void hp_assert(const char *a, const char *b, int c)
{
	printf("HP_ASSERT: %s,%s,%d\n", a,b,c);
}

//#endif
