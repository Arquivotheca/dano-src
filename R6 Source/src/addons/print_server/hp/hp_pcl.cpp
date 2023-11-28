//--------------------------------------------------------------------
//
//	(c) 1990-2001 Be Incorporated, all rights reserved.
//	Be Incorporated Confidential Information
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

#include "hp_pcl.h"

// /////////////////////////////////////////////////////////////////////////////////////


extern "C" BPrinterAddOn *instantiate_printer_addon(int32 index, BTransportIO* transport, BNode *printer_file)
{
	if (index != 0)	return NULL;
	return static_cast<BPrinterAddOn *>(new BPclDriver(transport, printer_file));
}

extern "C" BPrinterConfigAddOn *instantiate_printer_config_addon(BTransportIO* transport, BNode *printer_file)
{
	return static_cast<BPrinterConfigAddOn *>(new BPclConfig(transport, printer_file));
}

// /////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------
// This is the magic matrix currently used to do the gray scale to
// 1 bit dithering.
//------------------------------------------------------------------

static uint8 ditherTable16[16] = {
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5
};

static uint8 ditherTable64[64] = {
     0, 32,  8, 40,  2, 34, 10, 42, 
    48, 16, 56, 24, 50, 18, 58, 26, 
    12, 44,  4, 36, 14, 46,  6, 38, 
    60, 28, 52, 20, 62, 30, 54, 22, 
     3, 35, 11, 43,  1, 33,  9, 41, 
    51, 19, 59, 27, 49, 17, 57, 25, 
    15, 47,  7, 39, 13, 45,  5, 37, 
    63, 31, 55, 23, 61, 29, 53, 21
};

static uint8 ditherTable256[256];

// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

BPclDriver::BPclDriver(BTransportIO* transport, BNode *printer_file)
	:	BPrinterRasterAddOn(transport, printer_file, 0, 0),
		fCancelRequested(false),
		fTempBuffer(NULL),
		cur_error_vector(NULL),
		next_error_vector(NULL),
		fBandWidth(0)
{
}

BPclDriver::~BPclDriver()
{
	free(next_error_vector);
	free(cur_error_vector);
	free(fTempBuffer);
}

status_t BPclDriver::BeginJob()
{
	Transport()->SetErrorHandler(this);
	BPrinterRasterAddOn::BeginJob();
	return B_OK;
}

status_t BPclDriver::EndJob()
{
	if (fCancelRequested)
	{ // Either the user requested to Cancel or something went wrong.
		// Don't try to finish the job. But reset the printer.
		Transport()->SoftReset();
	}
	else
	{ // Wait all the data are transfered to the printer
		char buffer[32];
		sprintf(buffer, "%cE", 27);	// Reset
		SendOutput(buffer);
		Transport()->Sync();
	}

	// Make sure the error handler won't be called anymore (bullet proofing)
	Transport()->SetErrorHandler(NULL);

	// Free memory
	free(next_error_vector);	next_error_vector = NULL;
	free(cur_error_vector);		cur_error_vector = NULL;
	free(fTempBuffer);			fTempBuffer = NULL;
	return BPrinterRasterAddOn::EndJob();
}

status_t BPclDriver::EndPage()
{
	char buffer[32];
	ssize_t error;
	sprintf(buffer, "%c*rB", 27); // End graphic
	if ((error = SendOutput(buffer)) < B_OK)
		return error;

	return B_OK;
}

status_t BPclDriver::BeginPage(const print_bitmap_t *bitmap)
{
	if (fBandWidth == 0) {
		fBandWidth = (bitmap->w + 7) & ~3;
		fTempBuffer = (uint8 *)malloc(fBandWidth * bitmap->x_loop * 3);
		memset(fTempBuffer, 0xFF, (fBandWidth * bitmap->x_loop * 3));	// (0xFF = white)
		generate_dither_256();
		cur_error_vector = (int32 *)malloc(fBandWidth * sizeof(int32));
		next_error_vector = (int32 *)malloc(fBandWidth * sizeof(int32));
		memset(cur_error_vector, 0, fBandWidth * sizeof(int32));
		memset(next_error_vector, 0, fBandWidth * sizeof(int32));	
	}

	char buffer[32];
	ssize_t error;

	// resolution
	sprintf(buffer, "%cE%c*t%uR", 27, 27, Settings().DeviceXdpi());	// X and Y dpi are the same
	if ((error = SendOutput(buffer)) < B_OK)
		return error;
	
	// page size
	int32 pclformat;
	if ((error = Settings().Message().FindInt32("hppcl:paperformat", &pclformat)) != B_OK)
		return error;
	sprintf(buffer, "%c&l%dA", 27, pclformat);
	if ((error = SendOutput(buffer)) < B_OK)
		return error;

	// page orientation
	sprintf(buffer, "%c&l%dO", 27, 0); // Force to portrait. the rotation is done by the printkit
	if ((error = SendOutput(buffer)) < B_OK)
		return error;
	
	// raster orientation
	sprintf(buffer, "%c*r%dF", 27, 0);
	if ((error = SendOutput(buffer)) < B_OK)
		return error;
	
	// disable top margin & pref skip
	sprintf(buffer, "%c&l0e0L%c9", 27, 27);
	if ((error = SendOutput(buffer)) < B_OK)
		return error;

	// Top margin (in dots)
	// Set the top line to be 75 dots (= 1/4 inch at 300 dpi).
	const float y_factor = ((float)Settings().DeviceYdpi() / (float)Settings().Ydpi());
	const int TopY = (int)(Settings().DevicePrintableArea().top * y_factor);
	
	/* set the initial x-and-y coordinates for raster graphics */
	sprintf(buffer, "%c*p%dX%c*p%dY", 27, 0, 27, TopY);
	if ((error = SendOutput(buffer)) < B_OK)
		return error;

	// Enter graphic
	sprintf(buffer, "%c*r1A", 27); 
	if ((error = SendOutput(buffer)) < B_OK)
		return error;

	sprintf((char *)buffer, "%c*b%dM", 27, 2);	// Graphics Compaction Mode 2
	if ((error = SendOutput(buffer)) < B_OK)
		return error;

	return B_OK;
}

status_t BPclDriver::OutputData(const print_bitmap_t *bitmap)
{
	status_t error = B_OK;
	uint32 *p = bitmap->bits.bits32;
	for (int y=0 ; (y<bitmap->h) && (CanContinue()) && (error==B_OK) ; y++) {
		for (int x=0 ; x<bitmap->w ; x++) {
			const uint32 pixel = *(p + x*(bitmap->offb_next_pixel/4));
			for (int xloop=0 ; xloop < bitmap->x_loop ; xloop++) {
				const int offset = (x * bitmap->x_loop + xloop) * 3;
				fTempBuffer[offset+0] = (pixel >> 16) & 0xFF;
				fTempBuffer[offset+1] = (pixel >> 8) & 0xFF;
				fTempBuffer[offset+2] = pixel & 0xFF;
			}
		}
		for (int yloop=0 ; ((yloop < bitmap->y_loop) && (error==B_OK)) ; yloop++)
			error = Dither((const uint32 *)fTempBuffer);
		p += (bitmap->offb_next_line)/4;
	}	
	return error;
}

status_t BPclDriver::GetSupportedDeviceID(uint32 *count, printer_id_t **ids)
{
	static printer_id_t printerID[] = { {"HEWLETT-PACKARD", NULL} };
	*count = sizeof(printerID)/sizeof(printerID[0]);
	*ids = printerID;
	return B_OK;
}

status_t BPclDriver::Cancel()
{
	// don't forget to call the derived class
	// (this will ensure that OutputData() will not be called anymore.
	// EndPage(), Endjob() and EndDocument() _will_ be called
	fCancelRequested = true;
	return BPrinterRasterAddOn::Cancel();
}

bool BPclDriver::Error(status_t& io_error, bool retry)
{
	if (retry == false)
	{ // Unrecoverrable error
		return false;
	}
	else
	{ // The error is recoverrable
		// Wait a little (0.5s) to give time to the printer to process data	
		snooze(500000);
	
		// make sure the user didn't canceled
		if (CanContinue() == false)
			return false;

		// Retry
		return true;
	}
}

// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark # PCL3 Stuffs #

status_t BPclDriver::Dither(const uint32 *fTempBuffer)
{
	static int line=0;
	ssize_t error;
	char buffer1[32];
	unsigned char buffer[2048];
	unsigned char out_buffer[2048];
	unsigned char *out = out_buffer;
	memset(buffer, 0, sizeof(buffer));
	memset(out_buffer, 0, sizeof(out_buffer));

//	do_line(buffer, fTempBuffer, line++);
//	do_fs_error_line(buffer, fTempBuffer);
	do_burkes_error_line(buffer, fTempBuffer);

	// find the last non zero byte in the buffer... this will avoid doing
	// useless work.
	long cnt = 1;
	for (int k=0; k<fBandWidth/8; k++)
		if (buffer[k])
			cnt = k;
	cnt++;

	// compress that line using PackBits compression.
	const long cnt1 = Compress((void **)&out, sizeof(out_buffer), (void *)buffer, (size_t)cnt, B_RLE_127);
	if (cnt1 < 0)
		return (status_t)cnt1;
	sprintf(buffer1, "%c*b%ldW", 27, cnt1);
	if ((error = SendOutput(buffer1)) < B_OK)
		return error;

	// Send the line to the printer.
	if ((error = SendOutput(out, cnt1)) < B_OK)
		return error;

	return B_OK;
}

void BPclDriver::do_line(uchar *buffer, const uint32 *ptr, long y)
{
	const int32 MAX_INTENSITY = 255*(R_RATIO+G_RATIO+B_RATIO);
	const uint8 *rgb = (const uint8 *)ptr;
	uchar cur;	
	int32 k,intensity;
	int32 xx = 0;
	
	const uint8 *dither = ditherTable64 + (y&0x7)*8;
	for (int i=0 ; i < fBandWidth/8 ; i++) {
		for (k=0, cur=0; k < 8; k++, xx++) {
			intensity =	 R_RATIO * (*rgb++);
			intensity+=	 G_RATIO * (*rgb++);
			intensity+=	 B_RATIO * (*rgb++);
			if ((intensity >= (MAX_INTENSITY-1)) || (intensity > (((int32)dither[xx&0x7])<<18)))
				cur &= ~(0x80 >> k);
			else
				cur |= (0x80 >> k);
		}
		buffer[i] = cur;
	}
}

void BPclDriver::do_fs_error_line(uchar *buffer, const uint32 *ptr)
{
	const int32 MAX_INTENSITY = 255*(R_RATIO+G_RATIO+B_RATIO);
	const int32 HALF_MAX = MAX_INTENSITY/2;
	
	const uint8 *rgb = (const uint8 *)ptr;
	uchar	cur;	
	int32	k;
	int32	last_error = 0;
	int32 	error;
	int32 	error_sum;
	int32 	intensity;
	int32 	xx = 0;

	for (int i=0 ; i < fBandWidth/8 ; i++) {
		for (k=0, cur=0 ; k < 8; k++, xx++) {
			intensity =	 R_RATIO * (*rgb++);
			intensity+=	 G_RATIO * (*rgb++);
			intensity+=	 B_RATIO * (*rgb++);

			int32 val = intensity + cur_error_vector[xx] + last_error;
			if (val > MAX_INTENSITY)	val = MAX_INTENSITY;
			else if(val < 0)			val = 0;
			if (val > HALF_MAX){
				error = (val - MAX_INTENSITY);
			} else {
				cur |= (0x80 >> k);
				error = val;
			}

			error_sum = error;
			error = error>>4;	/* error/16 */
			last_error = 7*error;
			error_sum -= last_error;			
			if (xx != 0){
				next_error_vector[xx-1] += 3*error;
			}
			error_sum -= 3*error;
			next_error_vector[xx] += 5*error;
			error_sum -= 5*error;
			if (xx != fBandWidth - 1) {
				next_error_vector[xx+1] += error_sum;
			}
		}
		buffer[i] = cur;
	}

	int32 * const tmp = cur_error_vector;
	cur_error_vector = next_error_vector;
	next_error_vector = tmp;
	memset(next_error_vector, 0, fBandWidth * sizeof(int32));	
}


void BPclDriver::do_burkes_error_line(uchar *buffer, const uint32 *ptr)
{
	const int32 MAX_INTENSITY = 255*(R_RATIO+G_RATIO+B_RATIO);
	const int32 HALF_MAX = MAX_INTENSITY/2;
	
	const uint8 *rgb = (const uint8 *)ptr;
	uchar	cur;	
	int32	k;
	int32 	error;
	int32 	intensity;
	int32 	xx = 0;

	for (int i=0 ; i < fBandWidth/8 ; i++) {
		for (k=0, cur=0 ; k<8 ; k++, xx++) {
			intensity =	 R_RATIO * (*rgb++);
			intensity+=	 G_RATIO * (*rgb++);
			intensity+=	 B_RATIO * (*rgb++);

			int32 val = intensity + cur_error_vector[xx];
			if (val > MAX_INTENSITY)	val = MAX_INTENSITY;
			else if(val < 0)			val = 0;
			if (val > HALF_MAX) {
				error = (val - MAX_INTENSITY);
			} else {
				cur |= (0x80 >> k);
				error = val;
			}

			next_error_vector[xx] += (error>>2);
			if (xx < fBandWidth-1) {
				cur_error_vector[xx+1] += (error>>2);
				next_error_vector[xx+1] += (error>>3);
				if (xx < fBandWidth-2) {
					cur_error_vector[xx+1] += (error>>3);
					next_error_vector[xx+1] += (error>>4);
				}
			}
			if (xx > 0) {
				next_error_vector[xx-1] += (error>>3);
				if(xx > 1) {
					next_error_vector[xx-2] += (error>>4);
				}
			}
		}
		buffer[i] = cur;
	}

	int32 * const tmp = cur_error_vector;
	cur_error_vector = next_error_vector;
	next_error_vector = tmp;
	memset(next_error_vector, 0, fBandWidth * sizeof(int32));	
}

status_t BPclDriver::SendOutput(const char *data)
{
	return SendOutput((void *)data, strlen(data));
}

status_t BPclDriver::SendOutput(void *data, size_t size)
{
	return Transport()->Write(data, size);
}

void BPclDriver::generate_dither(uint8 *dither, int32 value, int32 level, int32 size)
{
	const int32 half = (size>>(level+1));
	const int32 valueInc = 1<<(level<<1);

	if (half > 1) {
		generate_dither(dither,value,level+1,size);
		generate_dither(dither+half*size+half,value+valueInc,level+1,size);
		generate_dither(dither+half,value+valueInc+valueInc,level+1,size);
		generate_dither(dither+half*size,value+valueInc+valueInc+valueInc,level+1,size);
	} else {
		dither[0] 		= value;
		dither[size+1] 	= value+valueInc;
		dither[1]		= value+valueInc+valueInc;
		dither[size]	= value+valueInc+valueInc+valueInc;
	}
}

void BPclDriver::generate_dither_256()
{
	generate_dither(ditherTable256,0,0,16);
}


// /////////////////////////////////////////////////////////////////////////////////////
// #pragma mark -

BPclConfig::BPclConfig(BTransportIO* transport, BNode *printer_file)
	: 	BPrinterConfigAddOn(transport, printer_file)
{
	// Just to set some default values
	Settings().SetSettings(BMessage());	

	// Supported modes
	const char *paper = "Plain";
	for (int i=0 ; i<3 ; i++)
	{
		fPrinterModes[i].paper = paper;
		fPrinterModes[i].attributes = printer_mode_t::B_BLACK_ONLY;
		fPrinterModes[i].user = i;
	}
	fPrinterModes[0].quality = "Draft";
	fPrinterModes[1].quality = "Normal";
	fPrinterModes[1].attributes |= printer_mode_t::B_IS_DEFAULT_QUALITY | printer_mode_t::B_IS_DEFAULT_PAPER;
	fPrinterModes[2].quality = "High";
	PrinterModeSelected(1); // Set the parameters for the default resolution
	
	// Supported papers
	fPaperFormats[0].SetTo(BPrintPaper::B_LETTER);
	fPaperFormats[1].SetTo(BPrintPaper::B_LEGAL);
	fPaperFormats[2].SetTo(BPrintPaper::B_A4);
	fPaperFormats[3].SetTo(BPrintPaper::B_B5);
	fPaperFormats[4].SetTo(BPrintPaper::B_B);
	fPaperFormats[5].SetTo(BPrintPaper::B_ENVELOPE_10);
	for (int i=0 ; i<6 ; i++)
		fPaperFormats[i].SetMargins(BPrintPaper::inch_to_milimeter(0.25), BPrintPaper::inch_to_milimeter(0.25), BPrintPaper::inch_to_milimeter(0.25), BPrintPaper::inch_to_milimeter(0.25));

	// Find and select the default paper
	PaperSelected(0, 0);
	for (int i=0 ; i<6 ; i++)
		if (fPaperFormats[i].id == BPrintPaper::DefaultFormat())
			PaperSelected(0, i);
}

BPclConfig::~BPclConfig()
{
}

int32 BPclConfig::PrinterModes(printer_mode_t const **modes)
{
	*modes = fPrinterModes;
	return sizeof(fPrinterModes)/sizeof(fPrinterModes[0]);
}

int32 BPclConfig::PaperFormats(int32 tray, BPrintPaper const **papers)
{
	if (tray != 0)
		return 0;
	*papers = fPaperFormats;
	return sizeof(fPaperFormats)/sizeof(fPaperFormats[0]);
}

status_t BPclConfig::PrinterModeSelected(int32 index)
{
	const int idx = fPrinterModes[index].user;

	// We record the selected mode (should not be used actually)
	Settings().Message().RemoveName("hppcl:printmode");
	Settings().Message().AddInt32("hppcl:printmode", idx);

	// *Mandatory* Resolution.
	const int resolutions[3] = {150,300,600};
	Settings().SetDeviceXdpi(resolutions[idx]);
	Settings().SetDeviceYdpi(resolutions[idx]);

	return B_OK;
}

status_t BPclConfig::PaperSelected(int32 tray, int32 paper)
{
	BPrinterConfigAddOn::PaperSelected(tray, paper);
	
	int32 pclformat = 2;
	switch (paper)
	{
		case 0:	pclformat=2; break;
		case 1:	pclformat=3; break;
		case 2:	pclformat=26; break;
		case 3:	pclformat=100; break;
		case 4:	pclformat=6; break;
		case 5:	pclformat=71; break;
	}
	
	Settings().Message().RemoveName("hppcl:paperformat");
	Settings().Message().AddInt32("hppcl:paperformat", pclformat);
	return B_OK;
}


