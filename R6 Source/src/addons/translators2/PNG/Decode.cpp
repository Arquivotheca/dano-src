//****************************************************************************************
//
//	File:		Decode.cpp
//
//  Written by:	Matt Bagosian
//	Revised by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "Decode.h"
#include <ByteOrder.h>
#include <DataIO.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <limits.h>

//	PNGToBitmapTranslator statics
const ssize_t PNGToBitmapTranslator::mk_png_check_bytes(8);

PNGToBitmapTranslator::~PNGToBitmapTranslator() {
	if (m_src_png != NULL) {
		png_destroy_read_struct(&m_src_png, (m_src_info != NULL) ? &m_src_info : NULL, NULL);
	}
	
	delete [] m_row_buf;
}

status_t PNGToBitmapTranslator::Status(status_t *err) const {
	status_t error(m_status);
	
	if (error == B_NO_ERROR) {
		error = Translator::Status();
	}
	
	if (err != NULL) {
		*err = error;
	}
	
	return error;
}

status_t PNGToBitmapTranslator::PerformTranslation() {
	status_t err;
	
	if (Status(&err) != B_NO_ERROR) {
		return err;
	}
	
	if ((err = setjmp(m_src_png->jmpbuf)) != B_NO_ERROR) {
		return err;
	}
	
	// We'll set up a row callback function so that we can translage
	// from one source to another without needing the entire image
	png_set_progressive_read_fn(m_src_png, this, static_cast<png_progressive_info_ptr>(PNGCallbackReadInfoBeginning),
		static_cast<png_progressive_row_ptr>(PNGCallbackReadRow),
		static_cast<png_progressive_end_ptr>(PNGCallbackReadInfoEnd));
	
	// BeOS now uses opacity alpha like PNG
	//png_set_invert_alpha(m_src_png);
	
	// Read from the source with our callbacks
	if (m_src->Seek(0, SEEK_SET) != 0) {
		return B_IO_ERROR;
	}
	
	ssize_t bytes_read;
	uint8 *buf;
	
	if ((buf = new uint8[mk_buf_size]) == NULL) {
		return B_NO_MEMORY;
	}
	
	while ((bytes_read = m_src->Read(buf, sizeof (uint8) * mk_buf_size)) > 0) {
		png_process_data(m_src_png, m_src_info, buf, bytes_read);
	}
	
	delete [] buf;
	return (bytes_read < 0) ? B_NO_TRANSLATOR : B_NO_ERROR;
}

void PNGToBitmapTranslator::PNGCallbackReadInfoBeginning(png_structp const png, png_infop const info) {
	status_t err;
	PNGToBitmapTranslator *trnsltr(static_cast<PNGToBitmapTranslator *>(png_get_progressive_ptr(png)));
	png_byte color_type(png_get_color_type(png, info));
	png_byte bit_depth(png_get_bit_depth(png, info));

	// massage the PNG's colorspace into something we can use

	// unpack and expand anything whose pixels
	// fit in one byte
	if (bit_depth <= 8) {
		png_set_expand(png);
		png_set_packing(png);
	}

	// expand any transparent images to add an alpha channel
	if (png_get_valid(png, info, PNG_INFO_tRNS)) {
		png_set_expand(png);
	}

	// we can only handle up to 8 bits per channel
	if (bit_depth > 8) {
		png_set_strip_16(png);
	}
	
	// use little-endian color order (BGR v RGB)
	png_set_bgr(png);

	// B_GRAY8 doesn't work
	// promote multibit grayscale to RGB(A)32
	if (!(color_type & PNG_COLOR_MASK_COLOR)) {
		png_set_gray_to_rgb(png);
	}

	// fill out the 32 bits for images without an alpha channel
	// so we can use B_RGB32
	if (!(color_type & PNG_COLOR_MASK_ALPHA)) {
		png_set_filler(png, 0xff, PNG_FILLER_AFTER);
	}
	
	// We want the library to handle interlacing, don't we?
	png_set_interlace_handling(png);
	
	// Now we need to update our info struct to reflect the
	// transformation changes we've asked it to make
	png_read_update_info(png, info);
	
	// We need to allocate space to keep track of the old row and
	// write out the bitmap header
	ssize_t row_bytes(png_get_rowbytes(png, info));
	delete [] trnsltr->m_row_buf;
	
	if ((trnsltr->m_row_buf = new png_byte[row_bytes]) == NULL) {
		// We're imitating the library's own error handler here
		longjmp(png->jmpbuf, B_NO_MEMORY);
	}
	
	if ((err = trnsltr->WriteHeader()) != B_NO_ERROR) {
		longjmp(png->jmpbuf, err);
	}
	
	// Initialize the destination space
	// This lets people use BPIO's like BMallocIO, where you
	//     can't seek past EOF (which libpng seems to like doing)
	memset(trnsltr->m_row_buf, (png_get_bit_depth(png, info) > 1) ? 0xFF : 0x00, row_bytes);
}

void PNGToBitmapTranslator::PNGCallbackReadRow(png_structp const png, png_bytep const row,
	const png_uint_32 row_num, const int /*pass*/) {

	PNGToBitmapTranslator *trnsltr(static_cast<PNGToBitmapTranslator *>(png_get_progressive_ptr(png)));
	ssize_t row_bytes(png_get_rowbytes(png, trnsltr->m_src_info));
	
	if (row != NULL) {
		// Get the old row
		if (trnsltr->m_dest->Seek(sizeof (TranslatorBitmap) + row_num * row_bytes, SEEK_SET) !=
			sizeof (TranslatorBitmap) + row_num * row_bytes ||
			trnsltr->m_dest->Read(trnsltr->m_row_buf, row_bytes) < row_bytes) {

			// We're imitating the library's own error handler here
			longjmp(png->jmpbuf, B_IO_ERROR);
		}
		
		// Combine the two (this has no effect if the image is not interlaced)
		png_progressive_combine_row(png, trnsltr->m_row_buf, row);
		
		// Write the result
		if (trnsltr->m_dest->Seek(sizeof (TranslatorBitmap) + row_num * row_bytes, SEEK_SET) !=
			sizeof (TranslatorBitmap) + row_num * row_bytes ||
			trnsltr->m_dest->Write(trnsltr->m_row_buf, row_bytes) < row_bytes) {

			longjmp(png->jmpbuf, B_IO_ERROR);
		}
	}
}

void PNGToBitmapTranslator::Init() {
	m_status = B_NO_ERROR;
	m_src_png = NULL;
	m_src_info = NULL;
	m_row_buf = NULL;
	
	png_byte buf[mk_png_check_bytes];
	
	// Try to read the header
	if (m_src->Seek(0, SEEK_SET) != 0 || m_src->Read(buf, mk_png_check_bytes) < mk_png_check_bytes) {
		m_status = B_IO_ERROR;
	}
	// Check the header
	else if (png_sig_cmp(buf, 0, mk_png_check_bytes) != 0) {
		m_status = B_NO_TRANSLATOR;
	}
	// Try to allocate the PNG image data structures
	else if ((m_src_png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL
		|| (m_src_info = png_create_info_struct(m_src_png)) == NULL) {

		if (m_src_png != NULL) {
			png_destroy_read_struct(&m_src_png, (m_src_info != NULL) ? &m_src_info : NULL, NULL);
		}
		
		m_src_png = NULL;
		m_src_info = NULL;
		m_status = B_NO_MEMORY;
	}
}

status_t PNGToBitmapTranslator::WriteHeader() {
	m_dest_bmp_hdr.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
	
	// Set up the image specific items
	// initialize offsets because image may not have an oFFs chunk
	png_int_32 offset_x(0), offset_y(0);
	int offset_units(PNG_OFFSET_PIXEL);

	png_get_oFFs(m_src_png, m_src_info, &offset_x, &offset_y, &offset_units);
	m_dest_bmp_hdr.bounds.left = static_cast<float>(offset_x);
	m_dest_bmp_hdr.bounds.top = static_cast<float>(offset_y);
	
	if (offset_units == PNG_OFFSET_MICROMETER) {
		png_get_pHYs(m_src_png, m_src_info, NULL, NULL, &offset_units);
		m_dest_bmp_hdr.bounds.left = (offset_units == PNG_RESOLUTION_METER) ?
			m_dest_bmp_hdr.bounds.left / 1000000.0 : 0.0;
		m_dest_bmp_hdr.bounds.top = (offset_units == PNG_RESOLUTION_METER) ?
			m_dest_bmp_hdr.bounds.top / 1000000.0 : 0.0;
	}
	
	m_dest_bmp_hdr.bounds.right = png_get_image_width(m_src_png, m_src_info) -
		1.0 + m_dest_bmp_hdr.bounds.left;
	m_dest_bmp_hdr.bounds.bottom = png_get_image_height(m_src_png, m_src_info) -
		1.0 + m_dest_bmp_hdr.bounds.top;
	
	// Try to fix corrupted images or those for which we don't have
	// the proper granularity to express (mainly to compensate for
	// Photoshop fuck-ups)
	if (!m_dest_bmp_hdr.bounds.IsValid()
		|| static_cast<size_t>(m_dest_bmp_hdr.bounds.Width() + 1) != png_get_image_width(m_src_png, m_src_info)
		|| static_cast<size_t>(m_dest_bmp_hdr.bounds.Height() + 1) != png_get_image_height(m_src_png, m_src_info)) {
		m_dest_bmp_hdr.bounds.left = 0.0;
		m_dest_bmp_hdr.bounds.top = 0.0;
		m_dest_bmp_hdr.bounds.right = png_get_image_width(m_src_png, m_src_info) - 1.0;
		m_dest_bmp_hdr.bounds.bottom = png_get_image_height(m_src_png, m_src_info) - 1.0;
	}
	
	m_dest_bmp_hdr.rowBytes = png_get_rowbytes(m_src_png, m_src_info);
	
	// Get the color space
	png_byte color_type(png_get_color_type(m_src_png, m_src_info));
	png_byte bit_depth(png_get_bit_depth(m_src_png, m_src_info));

	if(color_type & PNG_COLOR_MASK_COLOR) {  // RGB, RGB_ALPHA, PALETTE
		m_dest_bmp_hdr.colors = B_RGB32;
		if (color_type & PNG_COLOR_MASK_ALPHA) {
			m_dest_bmp_hdr.colors = B_RGBA32;
		}
		m_dest_bmp_hdr.rowBytes = png_get_image_width(m_src_png, m_src_info) * 4;
	} else {	// GRAY, GRAY_ALPHA
		m_dest_bmp_hdr.colors = B_GRAY8;
		if (bit_depth == 1) {
		    m_dest_bmp_hdr.colors = B_GRAY1;
		}
	}

	// Write out the header
	m_dest_bmp_hdr.dataSize = m_dest_bmp_hdr.rowBytes * png_get_image_height(m_src_png, m_src_info);
	m_dest_bmp_hdr.bounds.left = B_HOST_TO_BENDIAN_FLOAT(m_dest_bmp_hdr.bounds.left);
	m_dest_bmp_hdr.bounds.top = B_HOST_TO_BENDIAN_FLOAT(m_dest_bmp_hdr.bounds.top);
	m_dest_bmp_hdr.bounds.right = B_HOST_TO_BENDIAN_FLOAT(m_dest_bmp_hdr.bounds.right);
	m_dest_bmp_hdr.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT(m_dest_bmp_hdr.bounds.bottom);
	m_dest_bmp_hdr.rowBytes = B_HOST_TO_BENDIAN_INT32(m_dest_bmp_hdr.rowBytes);
	m_dest_bmp_hdr.colors = static_cast<color_space>(B_HOST_TO_BENDIAN_INT32(m_dest_bmp_hdr.colors));
	m_dest_bmp_hdr.dataSize = B_HOST_TO_BENDIAN_INT32(m_dest_bmp_hdr.dataSize);
	ssize_t bmp_size(sizeof (TranslatorBitmap));
	
	if (m_dest->Seek(0, SEEK_SET) != 0	|| m_dest->Write(&m_dest_bmp_hdr, bmp_size) < bmp_size) {
		return B_IO_ERROR;
	}
	
	// Use SetSizeDestination() so that BMallocIO works without initializing
	// the whole stream first, which was yanked for Opera streaming
	return m_dest->SetSize(B_BENDIAN_TO_HOST_INT32(m_dest_bmp_hdr.dataSize) + 32);
}
