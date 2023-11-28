//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2001 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <BeBuild.h>
#include <OS.h>
#include <SupportDefs.h>
#include <Errors.h>
#include <Message.h>

#include "DriverEpsonModule.h"
#include "MDefinePrinter.h"
#include "MPrinter.h"

//#define DEBUG 1
#if DEBUG
#	define bug		printf
#	define D(_x)	_x
#else
#	define bug		printf
#	define D(_x)
#endif


// /////////////////////////////////////////////////////////////////////////////////////

BEpsonModule::BEpsonModule(BTransportIO* transport, BNode *printer_file, image_id image, MDefinePrinter *addon)
	:	BEpsonCommon(transport, printer_file, image, addon),
		fGlobal(NULL),
		fTempBuffer(NULL),
		fPairBuffer(NULL),
		fPairIndex(0)
{
}

BEpsonModule::~BEpsonModule()
{
	free((void *)fTempBuffer);
	free((void *)fPairBuffer);
}

status_t BEpsonModule::BeginJob()
{
	Transport()->SetErrorHandler(this);
	status_t err = BEpsonCommon::BeginJob();
	if (err != B_OK)
		return err;
	if ((err = Settings().Message().FindInt32("epson:printmode", &fEpsonPrintMode)) != B_OK)
		return err;
	fEpsonPrintMode += 1;	// Convert to epson's index
	return err;
}

status_t BEpsonModule::EndJob()
{
	D(bug("EPSON: EndJob()\n");)

	if (fCancelRequested)
	{ // Either the user requested to Cancel or something went wrong.
		// Don't try to finish the job. But reset the printer.
		tagSE_END_PARAM p;
		p.sVersion = 1;
		p.pvGlobal = fGlobal;
		p.bFlush = false;
		PrinterDev()->sSEEnd(&p);
		Transport()->SoftReset();
	} else {
		// Terminate the job
		tagSE_END_PARAM p;
		p.sVersion = 1;
		p.pvGlobal = fGlobal;
		p.bFlush = true;
		PrinterDev()->sSEEnd(&p);
		Transport()->Sync();
	}
	fGlobal = NULL;

	// Make sure the error handler won't be called anymore (bullet proofing)
	Transport()->SetErrorHandler(NULL);

	return BEpsonCommon::EndJob();
}

status_t BEpsonModule::BeginPage(const print_bitmap_t *bitmap)
{
	fLineCount = 0;
	if (fGlobal == NULL) {	
		const int printkit_input_res_x = Settings().DeviceXdpi() / bitmap->x_loop;
		const int printkit_input_res_y = Settings().DeviceYdpi() / bitmap->y_loop;
		int epson_input_res_x;
		int epson_input_res_y;
		epson_input_res_y = epson_input_res_x = min_c(printkit_input_res_x, printkit_input_res_y);		
		printkit_to_epson_jump_x = printkit_input_res_x / epson_input_res_x;
		printkit_to_epson_jump_y = printkit_input_res_y / epson_input_res_y;
		printkit_to_epson_loop_x = epson_input_res_x / printkit_input_res_x;
		printkit_to_epson_loop_y = epson_input_res_y / printkit_input_res_y;
		if (printkit_to_epson_jump_x == 0)	printkit_to_epson_jump_x = 1;
		if (printkit_to_epson_jump_y == 0)	printkit_to_epson_jump_y = 1;
		if (printkit_to_epson_loop_x == 0)	printkit_to_epson_loop_x = 1;
		if (printkit_to_epson_loop_y == 0)	printkit_to_epson_loop_y = 1;

		D(bug("scalling : %d,%d - %d,%d\n", printkit_to_epson_jump_x, printkit_to_epson_jump_y, printkit_to_epson_loop_x, printkit_to_epson_loop_y);)

		const float x_factor = ((float)epson_input_res_x / (float)Settings().Xdpi());
		const float y_factor = ((float)epson_input_res_y / (float)Settings().Ydpi());
		tagSE_INIT_PARAM p;
		p.sVersion = 1;
		p.ppvGlobal = &fGlobal;	
		p.SrcPaperSize.x = (short)((Settings().DeviceArea().Width() + 1.0f) * x_factor);
		p.SrcPaperSize.y = (short)((Settings().DeviceArea().Height()+ 1.0f) * y_factor);
		p.SrcPrintArea.x = (short)((Settings().DevicePrintableArea().Width() + 1.0f) * x_factor);
		p.SrcPrintArea.y = (short)((Settings().DevicePrintableArea().Height()+ 1.0f) * y_factor);
		p.SrcMargin.x = 0;
		p.SrcMargin.y = (short)(Settings().DevicePrintableArea().top * y_factor);	
		p.sPaperSizeID = 1;	// TODO: letter
		p.sPrintingMode = fEpsonPrintMode;	
		p.pvPrintOutFunc = (void *)_spool_func;
		p.pvFuncParam = this;

		D(bug("tagSE_INIT_PARAM dump:\n");)
		D(bug("sVersion: %d\n", p.sVersion);)
		D(bug("ppvGlobal: %p\n", p.ppvGlobal);)
		D(bug("SrcPaperSize.x: %d\n", p.SrcPaperSize.x);)
		D(bug("SrcPaperSize.y: %d\n", p.SrcPaperSize.y);)
		D(bug("SrcPrintArea.x: %d\n", p.SrcPrintArea.x);)
		D(bug("SrcPrintArea.y: %d\n", p.SrcPrintArea.y);)
		D(bug("SrcMargin.x: %d\n", p.SrcMargin.x);)
		D(bug("SrcMargin.y: %d\n", p.SrcMargin.y);)
		D(bug("sPaperSizeID: %d\n", p.sPaperSizeID);)
		D(bug("sPrintingMode: %d\n", p.sPrintingMode);)
	
		short result = PrinterDev()->sSEInit(&p);
		(void)result;
		D(bug("sSEInit() = %d\n", (int)result);)

		const size_t size = WIDTHBYTES((bitmap->w * printkit_to_epson_loop_x * 24)/printkit_to_epson_jump_x);
		fTempBuffer = (uint8 *)malloc(size);
		fPairBuffer = (uint8 *)malloc(size*2);
		memset(fPairBuffer, 0xFF, size*2);
	}

	return B_OK;
}

status_t BEpsonModule::EndPage()
{
	status_t result = B_OK;
	// ... and eject the page
	if (result == B_OK)
		result = fPrinter->FormFeed();
	return result;
}

status_t BEpsonModule::OutputData(const print_bitmap_t *bitmap)
{
	uint32 *p = bitmap->bits.bits32;
	for (int y=0 ; (y<bitmap->h) && (CanContinue()) ; y++)
	{
		if (fLineCount < (printkit_to_epson_jump_y-1)) {
			fLineCount++;
			p += (bitmap->offb_next_line)/4;
			continue;
		}
		fLineCount = 0;

		for (int x=0 ; x<bitmap->w ; x += printkit_to_epson_jump_x) {
			uint32 pixel = *(p + x * (bitmap->offb_next_pixel/4));
			for (int xloop=0 ; xloop < printkit_to_epson_loop_x ; xloop++)
			{
				const int offset = ((x/printkit_to_epson_jump_x) * printkit_to_epson_loop_x + xloop) * 3;
				fTempBuffer[offset+0] = (pixel >> 16) & 0xFF;
				fTempBuffer[offset+1] = (pixel >> 8) & 0xFF;
				fTempBuffer[offset+2] = pixel  & 0xFF;
			}
		}
		for (int yloop=0 ; yloop < printkit_to_epson_loop_y ; yloop++) {
			status_t error;
			if ((error = EpsonOut(bitmap, fTempBuffer)) != B_OK)
				return error;
		}
		p += (bitmap->offb_next_line)/4;

		// Update status if needed
		UpdateStatus();
	}	
	return B_OK;
}

status_t BEpsonModule::EpsonOut(const print_bitmap_t *bitmap, const uint8 *buffer)
{
	tagSE_OUT_PARAM param;
	param.sVersion = 1;
	param.pvGlobal = fGlobal;
	param.lWidthBytes = WIDTHBYTES((bitmap->w * printkit_to_epson_loop_x * 24)/printkit_to_epson_jump_x);
	param.lHeight = 2;
	param.pvBits = (void *)fPairBuffer;
	memcpy(fPairBuffer, fPairBuffer+param.lWidthBytes, param.lWidthBytes*(param.lHeight-1));
	memcpy(fPairBuffer+param.lWidthBytes*(param.lHeight-1), buffer, param.lWidthBytes);
	fPairIndex++;
	if (fPairIndex == param.lHeight) {
		fPairIndex = 0;
		if (PrinterDev()->sSEOut(&param) != 0)
			return B_ERROR;
	}
	return B_OK;
}

bool BEpsonModule::_spool_func(void *hParam, char *pBuf, long cbBuf)
{
	BEpsonModule *This = reinterpret_cast<BEpsonModule *>(hParam);
	return This->spool_func(pBuf, cbBuf);
}

bool BEpsonModule::spool_func(char *pBuf, long cbBuf)
{
	return (Transport()->Write((void *)pBuf, (size_t)cbBuf) == cbBuf) ? true : false;
}

bool BEpsonModule::Error(status_t& io_error, bool retry)
{
	(void)io_error;

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
