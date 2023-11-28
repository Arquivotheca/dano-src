// ***********************************************************************
// libprint.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#include <typeinfo>

#include <Autolock.h>
#include <Window.h>
#include <Bitmap.h>
#include <View.h>
#include <Region.h>

#include <print/PrintJobSettings.h>
#include <print/PrinterRasterAddOn.h>
#include <print/PrinterAddOnDefs.h>

#include "PrintStream.h"

#if (PRINTING_FOR_DESKTOP)
#	define		M_BITMAP_SIZE	(16*1024*1024)
#else
#	define		M_BITMAP_SIZE	(256*1024)
#endif

#define SPEED_STATISTICS	1

#define m _rPrivate

namespace BPrivate
{
	struct _printer_raster_addon_data
	{
		BPrinterRasterAddOn::print_bitmap_t BitmapHeader;
		BBitmap *Bitmap;
		BView *View;
		unsigned int NbRasters;
		size_t Memory;
		uint16 printer_width;
		uint16 printer_height;
		bool cancelled;
		float scale_factor;
		bool direct;
		BDirectPrintJob *direct_job;
		#if SPEED_STATISTICS
		bigtime_t process_time;
		size_t process_nb;
		#endif
	};
} using namespace BPrivate;


	
BPrinterRasterAddOn::BPrinterRasterAddOn(BTransportIO* transport, BNode *printer_file, unsigned int nb_rasters, size_t memory)
	: 	BPrinterAddOn(transport, printer_file),
	 	_fPrivate(new _printer_raster_addon_data),
		_rPrivate(*_fPrivate)
{
	m.Bitmap = NULL;
	m.View = NULL;
	m.NbRasters = nb_rasters;
	m.cancelled = false;
	m.direct = false;
	m.direct_job = NULL;
	#if SPEED_STATISTICS
	m.process_time = 0;
	m.process_nb = 0;
	#endif

	// memory = 0 -> Choose the best value.
	// TODO: We should adapt this value depending on the availlable memory
	if (memory == 0)
		memory = M_BITMAP_SIZE;
	m.Memory = memory;
}

BPrinterRasterAddOn::~BPrinterRasterAddOn()
{
	#if SPEED_STATISTICS
	if ((m.process_nb) && (m.process_time))
		printf("BPrinterRasterAddOn: processing speed %lu (x1000) pixel/s\n",
				(uint32)((m.process_nb * 1000.0) / m.process_time));
	#endif
	delete m.Bitmap;
	delete _fPrivate;
}

status_t BPrinterRasterAddOn::BeginJob()
{
	// Find out if we are in direct mode
	m.direct = Settings().Message().FindBool("be:direct");
	if (m.direct) {
		void *p = NULL;
		if ((Settings().Message().FindPointer("be:job", &p) != B_OK) || (!p))
			return B_BAD_VALUE;
		m.direct_job = reinterpret_cast<BDirectPrintJob *>(p);
	}

	//printf("m.direct_job = %p, direct=%d\n", m.direct_job, m.direct);
	status_t result = _setup_bitmap();
	if (result == B_OK) {
		if ((m.direct) && (m.direct_job)) {
			BDirectPrintJob::printer_descriptor_t pd;
			pd.width = m.printer_width;
			pd.height = m.printer_height;
			pd.space = m.Bitmap->ColorSpace();
			pd.aspect_ratio = 1.0; // Always 1.0, we handle the rescalling.
			result = m.direct_job->BeginJob(pd);
		}
	}
	return result;
}

status_t BPrinterRasterAddOn::EndJob()
{
	if ((m.direct) && (m.direct_job)) {
		m.direct_job->EndJob();
	}
	delete m.Bitmap;
	m.Bitmap = NULL;
	return B_OK;
}

status_t BPrinterRasterAddOn::Print(const page_t& page, const int nbCopies)
{
	// Check for 'direct' mode case first
	if (m.direct) {
		return handle_direct_mode(page, nbCopies);
	}
	
	// Make a copy of the bitmap header, because we'll modify it.
	print_bitmap_t bitmap_header = m.BitmapHeader;
	const uint32 nbPictures = page.picture_count;
	BPicture const * const * pictures = page.pictures;
	const BRect *clips = page.clips;
	const BPoint *where = page.points;

	status_t result = B_OK;
	BWindow *p_off_screen_window = m.View->Window();
	if (p_off_screen_window == NULL)
		return B_ERROR;

	// Lock the bitmap's window		
	BAutolock autolocker(p_off_screen_window);
	if (autolocker.IsLocked() == false)
		return B_ERROR;

	// Compute the scale factor
	const float scale_factor = m.scale_factor * Settings().Scale();

	// Set the scale factor
	m.View->SetScale(scale_factor);

	// Compute the clear rect
	BRect clear_rect = m.View->Bounds();

	const bool hmirror = (Settings().Attributes() & B_PRINT_ATTR_H_MIRROR);
	const bool portrait = (Settings().Orientation() == B_PRINT_LAYOUT_PORTRAIT);

	// compute the direction vector (used to process the page)
	int max_lines;
	int inc_lines;
	BPoint dir_vector;
	BPoint start_offset;
	if (portrait)
	{
		dir_vector = m.View->Frame().OffsetToCopy(B_ORIGIN).LeftBottom() + BPoint(0.0f, 1.0f);
		if (!hmirror)	start_offset = BPoint(0, 0);
		else			start_offset = BPoint(0, m.printer_height - (m.View->Frame().IntegerHeight()+1));
		max_lines = m.printer_height;
		inc_lines = (m.View->Frame().IntegerHeight()+1);
	}
	else
	{
		dir_vector = m.View->Frame().OffsetToCopy(B_ORIGIN).RightTop() + BPoint(1.0f, 0.0f);
		if (!hmirror)	start_offset = BPoint(0, 0);
		else			start_offset = BPoint(m.printer_height - (m.View->Frame().IntegerWidth()+1), 0);
		max_lines = m.printer_height; 
		inc_lines = (m.View->Frame().IntegerWidth()+1);
	}

	// Handle multiple copy
	for (int i=0 ; ((i<nbCopies) && (result == B_OK) && (m.cancelled == false)); i++)
	{
		// initialization for the begining of the page
		if ((result = BeginPage(&bitmap_header)) != B_OK)
			break;

		#ifdef _M_SHARPEN_H_
		// Create the sharpening object
		MSharpen sharpen(m.Bitmap);
		#endif

		// Compute and output each raster
		for (int r=0, countLines=0 ; ((r<m.NbRasters) && (result == B_OK) && (m.cancelled == false)) ; r++, countLines += inc_lines)
		{
			// Compute the raster_rect position
			const int raster = (hmirror) ? (-r) : (r);
			BRect raster_rect = ((m.View->Frame().OffsetToCopy(dir_vector * raster)) + start_offset) / scale_factor;
			
			// Erase this raster to white
			const rgb_color cWhite = {255, 255, 255, 255};
			m.View->ConstrainClippingRegion(NULL);
			m.View->SetHighColor(cWhite);
			m.View->FillRect(clear_rect);
			
			// go through the list of pictures
			for (int p=0 ; p<nbPictures ; p++)
			{
				// Calc the rect of the BPicture in the user's coordinate system
				BRect tmp_rect = clips[p];
				tmp_rect.OffsetTo(where[p]);
				BRect clip_rect = (tmp_rect & raster_rect);
				if (clip_rect.IsValid())
				{
					// Then, clip to this intersection, in the BBitmap's coordinate system
					clip_rect.OffsetBy(B_ORIGIN - raster_rect.LeftTop());
	
					// where to render the BPicture
					BPoint tmp_point = where[p] - raster_rect.LeftTop();
	
					// Clip the picture
					clip_rect.left		= (int)(clip_rect.left - 0.5f);
					clip_rect.top		= (int)(clip_rect.top - 0.5f);
					clip_rect.right		= (int)(clip_rect.right + 0.5f);
					clip_rect.bottom	= (int)(clip_rect.bottom + 0.5f);
					BRegion tmp_region;
					tmp_region.Set(clip_rect);
					m.View->ConstrainClippingRegion(&tmp_region);
					m.View->DrawPicture(pictures[p], tmp_point);
				}
			}
	
			// Be sure that the app_server has finished his work
			m.View->Sync();

			// Sharpen the bitmap (if needed)
			#ifdef _M_SHARPEN_H_
			sharpen.Sharpen();
			#endif

			// Send the raster to the printer
			if ((countLines + inc_lines) > max_lines)
				bitmap_header.h = (max_lines - countLines) / bitmap_header.y_integer_scale;

			#if SPEED_STATISTICS
				thread_info info;
				const thread_id me = find_thread(NULL);
				get_thread_info(me, &info);
			#endif

			result = OutputData(&bitmap_header);

			#if SPEED_STATISTICS
				if (result == B_OK) {
					m.process_nb += (bitmap_header.w * bitmap_header.h * bitmap_header.x_loop * bitmap_header.y_loop) / (bitmap_header.x_integer_scale * bitmap_header.y_integer_scale);
					m.process_time -= info.user_time;
					get_thread_info(me, &info);
					m.process_time += info.user_time;
				}
			#endif
		}

		// Stop immediately on error
		if (result != B_OK)
			break;

		// deinitialization for the begining of the page
		if ((result = EndPage()) != B_OK)
			break;
	}

	return result;
}

// ---------------------------------------------

status_t BPrinterRasterAddOn::handle_direct_mode(const page_t& page, uint32 nbCopies)
{
	if ((m.direct_job == NULL) || (m.direct == false))
		return B_BAD_VALUE;

	// Make a copy of the bitmap header, because we'll modify it.
	print_bitmap_t bitmap_header = m.BitmapHeader;


	status_t result = B_OK;
	const bool hmirror = (Settings().Attributes() & B_PRINT_ATTR_H_MIRROR);
	const bool portrait = (Settings().Orientation() == B_PRINT_LAYOUT_PORTRAIT);
	BRect bounds = m.Bitmap->Bounds();

	// compute the direction vector (used to process the page)
	int max_lines;
	int inc_lines;
	BPoint dir_vector;
	BPoint start_offset;
	if (portrait) {
		dir_vector = bounds.OffsetToCopy(B_ORIGIN).LeftBottom() + BPoint(0.0f, 1.0f);
		if (!hmirror)	start_offset = BPoint(0, 0);
		else			start_offset = BPoint(0, m.printer_height - (bounds.IntegerHeight()+1));
		max_lines = m.printer_height;
		inc_lines = (bounds.IntegerHeight()+1);
	} else {
		dir_vector = bounds.OffsetToCopy(B_ORIGIN).RightTop() + BPoint(1.0f, 0.0f);
		if (!hmirror)	start_offset = BPoint(0, 0);
		else			start_offset = BPoint(m.printer_height - (bounds.IntegerWidth()+1), 0);
		max_lines = m.printer_height; 
		inc_lines = (bounds.IntegerWidth()+1);
	}

	// Handle multiple copy
	for (int i=0 ; ((i<nbCopies) && (result == B_OK) && (m.cancelled == false)); i++)
	{
		// initialization for the begining of the page
		if ((result = BeginPage(&bitmap_header)) != B_OK)
			break;

		if ((result = m.direct_job->BeginPage(page.page)) != B_OK)
			break;

		#ifdef _M_SHARPEN_H_
		// Create the sharpening object
		MSharpen sharpen(m.Bitmap);
		#endif

		// Compute and output each raster
		for (int r=0, countLines=0 ; ((r<m.NbRasters) && (result == B_OK) && (m.cancelled == false)) ; r++, countLines += inc_lines)
		{
			// Compute the raster_rect position
			const int raster = (hmirror) ? (-r) : (r);
			BRect raster_rect = ((bounds.OffsetToCopy(dir_vector * raster)) + start_offset);
			
			// Call the client's code
			BDirectPrintJob::bitmap_rect_t r;
			r.page = page.page;
			r.left = (uint32)raster_rect.left;
			r.top = (uint32)raster_rect.top;
			r.width = (uint32)raster_rect.IntegerWidth() + 1;
			r.height = (uint32)raster_rect.IntegerHeight() + 1;
			result = m.direct_job->FillBitmap(r, *(m.Bitmap));
			if (result != B_OK)
				break;

			// Sharpen the bitmap (if needed)
			#ifdef _M_SHARPEN_H_
			sharpen.Sharpen();
			#endif

			// Send the raster to the printer
			if ((countLines + inc_lines) > max_lines)
				bitmap_header.h = (max_lines - countLines) / bitmap_header.y_integer_scale;

			#if SPEED_STATISTICS
				thread_info info;
				const thread_id me = find_thread(NULL);
				get_thread_info(me, &info);
			#endif

			result = OutputData(&bitmap_header);

			#if SPEED_STATISTICS
				if (result == B_OK) {
					m.process_nb += (bitmap_header.w * bitmap_header.h * bitmap_header.x_loop * bitmap_header.y_loop) / (bitmap_header.x_integer_scale * bitmap_header.y_integer_scale);
					m.process_time -= info.user_time;
					get_thread_info(me, &info);
					m.process_time += info.user_time;
				}
			#endif
		}

		// Stop immediately on error
		if (result != B_OK)
			break;

		// deinitialization for the begining of the page
		if ((result = EndPage()) != B_OK)
			break;

		if ((result = m.direct_job->EndPage()) != B_OK)
			break;
	}

	return result;
}

status_t BPrinterRasterAddOn::Cancel()
{
	m.cancelled = true;
	return BPrinterAddOn::Cancel();
}

status_t BPrinterRasterAddOn::BeginPage(const print_bitmap_t *bitmap)
{
	return B_OK;
}

status_t BPrinterRasterAddOn::EndPage()
{
	return B_OK;
}



status_t BPrinterRasterAddOn::_setup_bitmap()
{
	color_space space = B_RGB32;
	uint32 flags = B_BITMAP_ACCEPTS_VIEWS;	
	if (m.direct) {
		flags = Settings().Message().FindInt32("be:bitmap_flags");
	}

	// Make sure this color_space is supported
	uint32 support_flags = 0;
	if (bitmaps_support_space(space, &support_flags) == false)
		return B_BAD_VALUE;

	if (flags & B_BITMAP_ACCEPTS_VIEWS) {
		// Make sure we can attach BView to this color_space if needed
		if ((support_flags & B_BITMAPS_SUPPORT_ATTACHED_VIEWS) == 0)
			return B_BAD_VALUE;
	}

	// Find the offset to the next pixel for this color space
	size_t pixel_chunk, row_alignment, pixels_per_chunk;
	get_pixel_size_for(space, &pixel_chunk, &row_alignment, &pixels_per_chunk);
	if (pixels_per_chunk != 1)
		return B_BAD_VALUE;
	int pixel_length = pixel_chunk;

	// compute the rasters height (and nb of rasters needed)
	if ((m.NbRasters == 0) && (m.Memory == 0))
		return B_BAD_VALUE;

	float rendering_resolution = min_c(Settings().DeviceXdpi(), Settings().DeviceYdpi());
	#if (PRINTING_FOR_DESKTOP == 0)
	// On BeIA never use a high resolution, keep about ~300 dpi
	if (rendering_resolution * 0.01f > 3.0f)
		rendering_resolution = rendering_resolution / ((int)rendering_resolution/300);
	#endif
	const float x_scale = (float)Settings().DeviceXdpi() / rendering_resolution;
	const float y_scale = (float)Settings().DeviceYdpi() / rendering_resolution;
	int xIntegerScale = 1;
	int yIntegerScale = 1;
	m.BitmapHeader.x_loop = (uint16)1;
	m.BitmapHeader.y_loop = (uint16)1;
	if (x_scale >= 1.0f)	m.BitmapHeader.x_loop = (uint16)x_scale;
	else					xIntegerScale = (int)(1.0f/x_scale);
	if (y_scale >= 1.0f)	m.BitmapHeader.y_loop = (uint16)y_scale;
	else					yIntegerScale = (int)(1.0f/y_scale);

	// Get the Device Printable Area
	m.scale_factor = rendering_resolution / (float)Settings().Xdpi();	// Xdpi and Ydpi should be equals
	const int pixWidth	= (int)floor((Settings().DevicePrintableArea().Width()  * m.scale_factor) + 0.5f);
	const int pixHeight	= (int)floor((Settings().DevicePrintableArea().Height() * m.scale_factor) + 0.5f);

	// Store the size the bitmap would have if it has only one raster
	m.printer_width = pixWidth;
	m.printer_height = pixHeight;

	// Break it into rasters
	int	w = pixWidth;
	int h;
	if (m.NbRasters == 0)
	{
		h = (m.Memory / (w * pixel_length));
		if (h > pixHeight)
			h = pixHeight;
		m.NbRasters = (pixHeight + h - 1) / h;
	}
	else
	{
		h = ((pixHeight - 1 + m.NbRasters - 1) / m.NbRasters);
	}

	if (h < 2)
	{ // 2 lines minimum
		h = 2;
		m.NbRasters = (pixHeight + h - 1) / h;
	}

	// Create the BBitmap and its BView
	const bool hmirror = Settings().Attributes() & B_PRINT_ATTR_H_MIRROR;
	const bool vmirror = Settings().Attributes() & B_PRINT_ATTR_V_MIRROR;
	const bool portrait = Settings().Orientation() == B_PRINT_LAYOUT_PORTRAIT;

	BRect rect;
	if (portrait)	rect.Set(0, 0, w-1, h-1);
	else			rect.Set(0, 0, h-1, w-1);
	m.Bitmap = new BBitmap(rect, (flags | B_BITMAP_IS_AREA), space);	
	if (m.direct == false) {
		m.View = new BView(rect, NULL, B_FOLLOW_NONE, B_WILL_DRAW | B_SUBPIXEL_PRECISE);
		m.View->ForceFontAliasing(true);
		m.Bitmap->AddChild(m.View);
	}

	// Update our internal structure
	m.BitmapHeader.bits.bits = m.Bitmap->Bits();
	m.BitmapHeader.space = m.Bitmap->ColorSpace();

	if (portrait)
	{ // Portrait
		m.BitmapHeader.w = m.Bitmap->Bounds().IntegerWidth() + 1;
		m.BitmapHeader.h = m.Bitmap->Bounds().IntegerHeight() + 1;
		m.BitmapHeader.offb_next_pixel = pixel_length;
		m.BitmapHeader.offb_next_line = m.Bitmap->BytesPerRow();

		if (hmirror)
		{ // Horizontal mirror
			m.BitmapHeader.bits.bits8 += (m.Bitmap->BytesPerRow() * m.Bitmap->Bounds().IntegerHeight());
			m.BitmapHeader.offb_next_line = -m.BitmapHeader.offb_next_line;
		}

		if (vmirror)
		{ // Vertical mirror
			m.BitmapHeader.bits.bits8 += (m.Bitmap->Bounds().IntegerWidth() * pixel_length);
			m.BitmapHeader.offb_next_pixel = -m.BitmapHeader.offb_next_pixel;
		}		
	}
	else
	{ // Landscape
		m.BitmapHeader.w = m.Bitmap->Bounds().IntegerHeight() + 1;
		m.BitmapHeader.h = m.Bitmap->Bounds().IntegerWidth() + 1;
		m.BitmapHeader.bits.bits8 += (m.Bitmap->BytesPerRow() * m.Bitmap->Bounds().IntegerHeight());
		m.BitmapHeader.offb_next_pixel = -m.Bitmap->BytesPerRow();
		m.BitmapHeader.offb_next_line = pixel_length;

		if (hmirror)
		{ // Horizontal mirror
			m.BitmapHeader.bits.bits8 += (m.Bitmap->Bounds().IntegerWidth() * pixel_length);
			m.BitmapHeader.offb_next_line = -m.BitmapHeader.offb_next_line;
		}

		if (vmirror)
		{ // Vertical mirror
			m.BitmapHeader.bits.bits8 -= (m.Bitmap->BytesPerRow() * m.Bitmap->Bounds().IntegerHeight());
			m.BitmapHeader.offb_next_pixel = -m.BitmapHeader.offb_next_pixel;
		}
	}
	
	m.BitmapHeader.x_integer_scale = (int16)xIntegerScale;
	m.BitmapHeader.y_integer_scale = (int16)yIntegerScale;
	m.BitmapHeader.w /= xIntegerScale;
	m.BitmapHeader.h /= yIntegerScale;
	m.BitmapHeader.offb_next_pixel *= xIntegerScale;
	m.BitmapHeader.offb_next_line *= yIntegerScale;

Settings().Message().PrintToStream();
m.BitmapHeader.PrintToStream();

	return B_OK;
}

// --------------------------------------------------------
// #pragma mark -

ssize_t BPrinterRasterAddOn::Compress(void **out_buffer_ptr, size_t out_length, const void *in_buffer, size_t in_length, compression_type_t type)
{
	if ((type != B_RLE_127) && (type != B_RLE_128))
		return B_BAD_VALUE;

	const int MAX_SAME = ((type == B_RLE_127) ? (127) : (128));
	int8 cur = 0, prev = 0;
	int nb_same = 0, nb_diff = 0;
	const int8 *pIn = (const int8 *)in_buffer + in_length;
	int8 *pOut = (int8 *)(*out_buffer_ptr) + out_length;
	
	cur = *--pIn;
	*--pOut = cur;
	do {
		prev = cur;
		cur = *--pIn;

		if (cur == prev) {
			if (nb_diff != 0) {
				nb_same = 1;
				*pOut = nb_diff-1;
				*--pOut = cur;
				nb_diff = 0;
			} else  {
				if (nb_same == MAX_SAME) {
					*--pOut = -MAX_SAME;
					*--pOut = cur;
					nb_same = 0;
				} else {
					nb_same++;
				}
			}
		} else {
			if (nb_same != 0) {
				*--pOut = -nb_same;
				*--pOut = cur;
				nb_same = 0;
			} else {
				if (nb_diff == 127) {
					*--pOut = 127;
					*--pOut = cur;
					nb_diff = 0;
				} else {
					*--pOut = cur;
					nb_diff++;
				}
			}
		}

		if ((void *)pOut < *out_buffer_ptr) {
			return B_NO_MEMORY;
		}
	} while((void *)pIn > (void *)in_buffer);
	
	if (nb_same != 0)	*--pOut = -nb_same;
	else				*--pOut = nb_diff;
	
	const size_t size = (((int8 *)(*out_buffer_ptr) + out_length) - (int8 *)pOut);
	*out_buffer_ptr = (void *)pOut;
	return  size;
}

// --------------------------------------------------------
// #pragma mark -

void BPrinterRasterAddOn::print_bitmap_t::PrintToStream(void) const
{
	printf("bits: %p\n", bits);
	printf("space: %lX\n", space);
	printf("width: %d\n", w);
	printf("height: %d\n", h);
	printf("offp: %d\n", offb_next_pixel);
	printf("offl: %d\n", offb_next_line);
	printf("xs: %d\n", x_integer_scale);
	printf("ys: %d\n", y_integer_scale);
	printf("x_loop: %u\n", x_loop);
	printf("y_loop: %u\n", y_loop);
}

status_t BPrinterRasterAddOn::Perform(int32 selector, void * data) { return B_ERROR; }
status_t BPrinterRasterAddOn::_Reserved_BPrinterRasterAddOn_0(int32 arg, ...) { return B_ERROR; }
status_t BPrinterRasterAddOn::_Reserved_BPrinterRasterAddOn_1(int32 arg, ...) { return B_ERROR; }
status_t BPrinterRasterAddOn::_Reserved_BPrinterRasterAddOn_2(int32 arg, ...) { return B_ERROR; }
status_t BPrinterRasterAddOn::_Reserved_BPrinterRasterAddOn_3(int32 arg, ...) { return B_ERROR; }

