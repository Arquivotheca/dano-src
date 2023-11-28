//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef _PREVIEW_DRIVER_H
#define _PREVIEW_DRIVER_H

#include <ListItem.h>
#include <Node.h>
#include <Directory.h>

#include <print/PrinterConfigAddOn.h>
#include <print/PrinterAddOn.h>
#include <print/TransportIO.h>
#include <print/PrintConfigView.h>
#include <print/PrintPaper.h>

class BView;
class PreviewWindow;


// /////////////////////////////////////////////////////////////////////////////////////

class BPreview : public BPrinterAddOn
{
public:
			BPreview(BTransportIO* transport, BNode *printer_file);
	virtual ~BPreview();
private:
	virtual	status_t BeginJob();
	virtual	status_t EndJob();
	virtual	status_t Cancel();
	PreviewWindow *fPreviewWindow;
};


class BPreviewConfig : public BPrinterConfigAddOn
{
public:
			BPreviewConfig(BTransportIO* transport, BNode *printer_file);
	virtual ~BPreviewConfig();
private:
	virtual int32 PrinterModes(printer_mode_t const **modes);
	virtual status_t PrinterModeSelected(int32 index);
	virtual int32 PaperFormats(int32 tray, BPrintPaper const **papers);
	virtual status_t Save();

	printer_mode_t fPrinterModes[5];
	BPrintPaper fPaperFormats[BPrintPaper::B_NB_PAPER_FORMATS];
};

#endif
