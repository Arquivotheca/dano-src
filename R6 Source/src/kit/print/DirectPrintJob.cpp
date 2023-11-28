//******************************************************************************
//
//	File:		DirectPrintJob.cpp
//
//	Written by:	Mathias Agopian
//
//	Copyright 2001, Be Incorporated
//
//******************************************************************************


#ifndef _DEBUG_H
#	include <Debug.h>
#endif

#include <BeBuild.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <InterfaceDefs.h>
#include <File.h>
#include <Directory.h>
#include <Path.h>
#include <String.h>
#include <Errors.h>
#include <Messenger.h>

// Our own include file
#include <print/DirectPrintJob.h>
#include <print/PrintJobSettings.h>
#include <print/TransportIO.h>
#include <print/PrinterRasterAddOn.h>

#include "PrintEnv.h"
#include "PrintStream.h"

//------------------------------------------------------------------
// Usefull defines
//------------------------------------------------------------------

#define m _m_rprivate

#define DEBUG 1

#if DEBUG
#	define D(_x)	_x
#else
#	define D(_x)
#endif

#define E(_x)	_x
#define bug		printf


//------------------------------------------------------------------
// Defined types
//------------------------------------------------------------------

#include "PrintJob_private.h"

namespace BPrivate
{	
	struct direct_printjob_private
	{
		PrinterManager *fPrinterManager;
	};
}

using namespace BPrivate;

//------------------------------------------------------------------

BDirectPrintJob::BDirectPrintJob(const char *job_name)
	:	BPrintJob(job_name),
		_m_private(new direct_printjob_private),
		m(*_m_private)
{
	InitObject();
}

BDirectPrintJob::BDirectPrintJob(const BMessage& settings, const char *job_name = NULL)
	:	BPrintJob(settings, job_name),
		_m_private(new direct_printjob_private),
		m(*_m_private)
{
	InitObject();
}

status_t BDirectPrintJob::InitObject()
{
	if (BPrintJob::_m_private->fPrinterManager)
		m.fPrinterManager = BPrintJob::_m_private->fPrinterManager;
	else
		m.fPrinterManager = new PrinterManager(true);
	return B_OK;
}

BDirectPrintJob::~BDirectPrintJob()
{
	if (BPrintJob::_m_private->fPrinterManager == NULL)
		delete m.fPrinterManager;
	delete _m_private;
}

PrinterManager *BDirectPrintJob::printer_manager()
{
	return m.fPrinterManager;
}

//------------------------------------------------------------------
// #pragma mark -

status_t BDirectPrintJob::StartJob(uint32 nbPages, uint32 flags)
{
	// Check the state of the BPrintJob
	if (InitCheck() != B_OK)
		return InitCheck();

	// Make sure user has not aborted the job
	if (CanContinue() == false)
		return B_CANCELED;

	// ********************************************************
	// Lock the printer
	// ********************************************************

	// This mode works only in StandAlone mode.
	if (printer_manager()->is_printer_free(&PrinterNode()) == false)
	{ // Cancel the print-job, since there's nothing else to do.
		CancelJob();
		return B_PRINTER_BUSY;
	}
	// TODO: there's a little race-condition here - should fix it.
	printer_manager()->set_printer_busy(&PrinterNode(), NULL);


	// ********************************************************
	// We must make sure the driver supports the direct mode
	// That is, it must be a BPrinterRaster addon
	// ********************************************************

	status_t err;	
	image_id addon_image = err = load_driver_addon(&PrinterNode(), NULL);
	if (addon_image < 0)
		goto exit;

	{ // We need a block here, for stack allocated objects.
		BPrinterAddOn *(*instantiate_printer_addon)(int32, BTransportIO *, BNode *);
		if ((err = get_image_symbol(addon_image, B_INSTANTIATE_PRINTER_ADDON_FUNCTION, B_SYMBOL_TYPE_TEXT, (void **)&instantiate_printer_addon)) != B_OK) {
			err = B_BAD_DRIVER; // This is an old API driver. Won't work.
			goto exit;
		}
	
		BNode printer_node(PrinterNode());
		BTransportIO transport(&printer_node, NULL, 16*1024); //TODO: Should not hardcode the async buffer!
		if ((err = transport.InitCheck()) != B_OK)
			goto exit;
			
		BPrinterAddOn *addOn;
		if ((addOn = instantiate_printer_addon(0, &transport, &printer_node)) == NULL) { // TODO: should not always use index 0
			err = B_BAD_DRIVER; // This driver has a problem ?!
			goto exit;
		}
		
		BPrinterRasterAddOn *printerAddOn = dynamic_cast<BPrinterRasterAddOn *>(addOn);
		if (printerAddOn == NULL) {
			delete printerAddOn;
			err = B_BAD_DRIVER; // This driver doesn't support DirectPrintJob
			goto exit;
		}
		
		// ********************************************************
		// Here we are safe. The driver is initialized and supports BDirectPrintJob
		// ********************************************************

		// Just so that we know we successfuly started a print-job
		BPrintJob::_m_private->fJobStarted = true;

		// And... print!
		BMessage localSettings = *Settings();
		localSettings.AddInt32("be:nb_pages", nbPages);
		localSettings.AddInt32("be:bitmap_flags", flags);
		localSettings.AddBool("be:direct", true);
		localSettings.AddPointer("be:job", this);
		err = printerAddOn->take_job(&localSettings);

		// Put the BPrintJob in it's original state, ready to be used again!
		BPrintJob::_m_private->re_init();
	}

exit:
	// Free the printer
	printer_manager()->set_printer_free(&PrinterNode());
	TPrintTools::ClearCancel(&PrinterNode());
	if (addon_image >= 0)
		unload_add_on(addon_image);
	return err;
}

//------------------------------------------------------------------
// #pragma mark -

void BDirectPrintJob::CancelJob() {
	BPrintJob::CancelJob();
}
bool BDirectPrintJob::CanContinue() {
	return BPrintJob::CanContinue();
}
status_t BDirectPrintJob::BeginJob(const printer_descriptor_t&) {
	return B_OK;
}
status_t BDirectPrintJob::EndJob() {
	return B_OK;
}
status_t BDirectPrintJob::BeginPage(const uint32 page) {
	return B_OK;
}
status_t BDirectPrintJob::EndPage() {
	return B_OK;
}

//------------------------------------------------------------------
// #pragma mark -

status_t BDirectPrintJob::BeginJob() {
	return B_UNSUPPORTED;
}
status_t BDirectPrintJob::SpoolPage() {
	return B_UNSUPPORTED;
}
status_t BDirectPrintJob::CommitJob() {
	return B_UNSUPPORTED;
}
status_t BDirectPrintJob::SetScale(float) {
	return B_UNSUPPORTED;
}
status_t BDirectPrintJob::DrawView(BView *, BRect, BPoint) {
	return B_UNSUPPORTED;
}
status_t BDirectPrintJob::DrawPictures(BPicture * const *, const BRect *, const BPoint *, const uint32) {
	return B_UNSUPPORTED;
}

//------------------------------------------------------------------
// #pragma mark -

status_t BDirectPrintJob::_ReservedDirectPrintJob1() { return B_ERROR; }
status_t BDirectPrintJob::_ReservedDirectPrintJob2() { return B_ERROR; }
status_t BDirectPrintJob::_ReservedDirectPrintJob3() { return B_ERROR; }
status_t BDirectPrintJob::_ReservedDirectPrintJob4() { return B_ERROR; }
status_t BDirectPrintJob::Perform(int32 code, ...) { return B_ERROR; }

