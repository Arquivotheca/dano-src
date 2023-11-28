//--------------------------------------------------------------------
//
//	(c) 1990-2001 Be Incorporated, all rights reserved.
//	Be Incorporated Confidential Information
//	
//--------------------------------------------------------------------

#ifndef DRIVER_H
#define DRIVER_H

#include <FindDirectory.h>
#include <Directory.h>
#include <Path.h>
#include <Node.h>

#include <print/PrinterAddOnDefs.h>
#include <print/PrinterConfigAddOn.h>
#include <print/PrinterRasterAddOn.h>
#include <print/TransportIO.h>
#include <print/PrintConfigView.h>
#include <print/PrintPaper.h>

#define R_RATIO 19661  /* 30% */
#define G_RATIO 38666  /* 59% */
#define B_RATIO 7209   /* 11% */


class BPclDriver : public BPrinterRasterAddOn, public BTransportIOErrorHandler
{
public:
			BPclDriver(BTransportIO* transport, BNode *printer_file);
	virtual ~BPclDriver();
private:
	virtual	status_t BeginJob();
	virtual	status_t EndJob();
	virtual status_t EndPage();
	virtual status_t BeginPage(const print_bitmap_t *bitmap);
	virtual status_t OutputData(const print_bitmap_t *bitmap);
	virtual status_t GetSupportedDeviceID(uint32 *count, printer_id_t **ids);
	virtual status_t Cancel();
	virtual bool Error(status_t& io_error, bool retry);

	bool fCancelRequested;
	uint8 *fTempBuffer;

	// PCL stuffs
	status_t Dither(const uint32 *fTempBuffer);
	int32 SendOutput(const char *string);
	int32 SendOutput(void *buffer, size_t size);
	void do_line(uchar *buffer, const uint32 *ptr, long y);
	void do_fs_error_line(uchar *buffer, const uint32 *ptr);
	void do_burkes_error_line(uchar *buffer, const uint32 *ptr);
	static void generate_dither(uint8 *dither, int32 value, int32 level, int32 size);
	static void generate_dither_256();
	int32 *cur_error_vector;
	int32 *next_error_vector;
	int32 fBandWidth;	// width rounded up to bytes boundaries
};


// /////////////////////////////////////////////////////////////////////////////////////

class BPclConfig : public BPrinterConfigAddOn
{
public:
			BPclConfig(BTransportIO* transport, BNode *printer_file);
	virtual ~BPclConfig();
private:
	virtual int32 PrinterModes(printer_mode_t const **modes);
	virtual status_t PrinterModeSelected(int32 index);
	virtual status_t PaperSelected(int32 tray, int32 paper);
	virtual int32 PaperFormats(int32 tray, BPrintPaper const **papers);

	printer_mode_t fPrinterModes[3];
	BPrintPaper fPaperFormats[6];
};


#endif
