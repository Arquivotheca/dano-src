//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2001 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef DRIVER_EPSON_MODULE_H
#define DRIVER_EPSON_MODULE_H

#include <FindDirectory.h>
#include <Directory.h>
#include <Path.h>

#include <print/PrinterAddOnDefs.h>
#include <print/PrinterConfigAddOn.h>
#include <print/PrinterRasterAddOn.h>
#include <print/TransportIO.h>
#include <print/PrintConfigView.h>
#include <print/PrintPaper.h>

#include "MDefinePrinter.h"
#include "Driver.h"

class BView;
class MPrinter;
class MPrinterUtils;

// /////////////////////////////////////////////////////////////////////////////////////

class BEpsonModule : public BEpsonCommon, public BTransportIOErrorHandler
{
public:
			BEpsonModule(BTransportIO* transport, BNode *printer_file, image_id image, MDefinePrinter *addon);
	virtual ~BEpsonModule();
private:
	virtual	status_t BeginJob();
	virtual	status_t EndJob();
	virtual status_t EndPage();
	virtual status_t BeginPage(const print_bitmap_t *bitmap);
	virtual status_t OutputData(const print_bitmap_t *bitmap);
	virtual bool Error(status_t& io_error, bool retry);
private:
	status_t EpsonOut(const print_bitmap_t *bitmap, const uint8 *buffer);
	static bool _spool_func(void *hParam, char *pBuf, long cbBuf);
	bool spool_func(char *pBuf, long cbBuf);
	void *fGlobal;
	int32 fEpsonPrintMode;
	uint8 *fTempBuffer;
	uint8 *fPairBuffer;
	int fPairIndex;
	int printkit_to_epson_jump_x;
	int printkit_to_epson_jump_y;
	int printkit_to_epson_loop_x;
	int printkit_to_epson_loop_y;
	int fLineCount;
};

#endif
