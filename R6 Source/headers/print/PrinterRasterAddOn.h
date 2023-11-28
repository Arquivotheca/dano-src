// ***********************************************************************
// libpraster.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************


#ifndef _PRINTER_RASTER_ADDON_H_
#define _PRINTER_RASTER_ADDON_H_


#include <stdio.h>
#include <Node.h>
#include <Message.h>
#include <Picture.h>
#include <Rect.h>
#include <Point.h>
#include <GraphicsDefs.h>
#include <SupportDefs.h>

#include <print/PrinterAddOn.h>
#include <print/DirectPrintJob.h>

class BTransportIO;

namespace BPrivate
{
	struct _printer_raster_addon_data;	
}

class BPrinterRasterAddOn : public BPrinterAddOn
{
public:
			BPrinterRasterAddOn(BTransportIO* transport, BNode *printer_file, unsigned int nb_rasters = 0, size_t memory = 256*1024);
	virtual ~BPrinterRasterAddOn();

	enum compression_type_t
	{
		B_RLE_128 = 0,	// Packed runs can be 128 bytes (eg: ESC/P2)
		B_RLE_127 = 1	// Runs are limited to 127 bytes (eg: PCL)
	};
	ssize_t Compress(void **out_buffer_ptr, size_t out_length, const void *in_buffer, size_t in_length, compression_type_t type);

protected:

	struct print_bitmap_t
	{
		union
		{
			void *bits;
			uint8 *bits8;
			uint16 *bits16;
			uint32 *bits32;
		} bits;	
		color_space space;
		int w;
		int h;
		int offb_next_pixel;
		int offb_next_line;
		int16 x_integer_scale;
		int16 y_integer_scale;
		uint16 x_loop;
		uint16 y_loop;
		uint32 rsvr[8];
		void PrintToStream(void) const;
	};

	// The main hook
	virtual status_t 	OutputData(const print_bitmap_t *bitmap) = 0;

	// Note: folowing hook must be called by the derived class
	virtual	status_t	BeginJob();
	virtual status_t	EndJob();
	virtual	status_t	BeginPage(const print_bitmap_t *bitmap);
	virtual status_t	EndPage();
	virtual status_t	Cancel();

	// For implementation, you shouldn't need to override this method.
	virtual status_t	Print(const page_t& page, const int nbCopies = 1);


private:
	BPrinterRasterAddOn(const BPrinterRasterAddOn &);
	BPrinterRasterAddOn& operator = (const BPrinterRasterAddOn &);
	status_t handle_direct_mode(const page_t& page, uint32 nbCopies);
	virtual status_t _Reserved_BPrinterRasterAddOn_0(int32 arg, ...);
	virtual status_t _Reserved_BPrinterRasterAddOn_1(int32 arg, ...);
	virtual status_t _Reserved_BPrinterRasterAddOn_2(int32 arg, ...);
	virtual status_t _Reserved_BPrinterRasterAddOn_3(int32 arg, ...);
	virtual status_t Perform(int32 selector, void *data);

	status_t _setup_bitmap(void);

private:
	BPrivate::_printer_raster_addon_data *_fPrivate;
	BPrivate::_printer_raster_addon_data& _rPrivate;
	uint32 _reserved_BPrinterConfigAddOn_[4];
	
	friend struct BPrivate::_printer_raster_addon_data;
};

#endif

