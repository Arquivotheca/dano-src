//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>

#include <BeBuild.h>
#include <OS.h>

#include "PreviewDriver.h"
#include "PreviewWindow.h"

extern "C" BPrinterAddOn *instantiate_printer_addon(int32 index, BTransportIO* transport, BNode *printer_file)
{
	if (index != 0)	return NULL;
	return static_cast<BPrinterAddOn *>(new BPreview(transport, printer_file));
}

extern "C" BPrinterConfigAddOn *instantiate_printer_config_addon(BTransportIO* transport, BNode *printer_file)
{
	return static_cast<BPrinterConfigAddOn *>(new BPreviewConfig(transport, printer_file));
}


// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

BPreview::BPreview(BTransportIO* transport, BNode *printer_file)
	:	BPrinterAddOn(transport, printer_file),
	fPreviewWindow(NULL)
{
}

BPreview::~BPreview()
{
	if (fPreviewWindow->Lock())
		fPreviewWindow->Quit();
}

status_t BPreview::BeginJob()
{
	fPreviewWindow = new PreviewWindow(*this);
	fPreviewWindow->Show();
	fPreviewWindow->Wait();
	return B_OK;
}

status_t BPreview::EndJob()
{
	return B_OK;
}

status_t BPreview::Cancel()
{
	return B_OK;
}

// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

BPreviewConfig::BPreviewConfig(BTransportIO* transport, BNode *printer_file)
	:	BPrinterConfigAddOn(transport, printer_file)
{
	// Just to set some default values
	Settings().SetSettings(BMessage());	

	for (int i=0 ; i<BPrintPaper::B_NB_PAPER_FORMATS ; i++)
	{
		fPaperFormats[i].SetTo(i);
		fPaperFormats[i].SetMargins(3,3,3,5);
		if (i == BPrintPaper::DefaultFormat())
			PaperSelected(0, 1);
	}

	PrinterModeSelected(1);
}

BPreviewConfig::~BPreviewConfig()
{
}

		
int32 BPreviewConfig::PrinterModes(printer_mode_t const **modes)
{
	static const char *paper = "Plain paper";	
	fPrinterModes[0].paper = paper;
	fPrinterModes[0].quality = "72 DPI";
	fPrinterModes[0].attributes = printer_mode_t::B_COLOR_ONLY;
	fPrinterModes[1].paper = paper;
	fPrinterModes[1].quality = "300 DPI";
	fPrinterModes[1].attributes = printer_mode_t::B_COLOR_ONLY | printer_mode_t::B_IS_DEFAULT_PAPER | printer_mode_t::B_IS_DEFAULT_QUALITY;
	fPrinterModes[2].paper = paper;
	fPrinterModes[2].quality = "360 DPI";
	fPrinterModes[2].attributes = printer_mode_t::B_COLOR_ONLY;
	fPrinterModes[3].paper = paper;
	fPrinterModes[3].quality = "600 DPI";
	fPrinterModes[3].attributes = printer_mode_t::B_COLOR_ONLY;
	fPrinterModes[4].paper = paper;
	fPrinterModes[4].quality = "720 DPI";
	fPrinterModes[4].attributes = printer_mode_t::B_COLOR_ONLY;
 	*modes = fPrinterModes;
	return sizeof(fPrinterModes)/sizeof(fPrinterModes[0]);
}

status_t BPreviewConfig::PrinterModeSelected(int32 index)
{
	static int32 resolution[] = {72, 300, 360, 600, 720};
	const int c = sizeof(fPrinterModes)/sizeof(fPrinterModes[0]);
	if ((index < 0) || (index >= c))
		return B_BAD_VALUE;
	Settings().SetDeviceXdpi(resolution[index]);
	Settings().SetDeviceYdpi(resolution[index]);
	return B_OK;
}


int32 BPreviewConfig::PaperFormats(int32 tray, BPrintPaper const **papers)
{
	if (tray != 0)
		return 0;
	*papers = fPaperFormats;
	return sizeof(fPaperFormats)/sizeof(fPaperFormats[0]);
}

status_t BPreviewConfig::Save()
{
	Settings().SetNbCopies(1);
	return B_OK;
}

