//****************************************************************************************
//
//	File:		Encode.cpp
//
//  Written by:	Matt Bagosian
//	Revised by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "Encode.h"
#include "PNGTranslator.h"
#include <ByteOrder.h>
#include <DataIO.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <limits.h>

// Initialize the BitmapToPNGTranslator private static const members
const ssize_t BitmapToPNGTranslator::mk_png_check_bytes(8);
const uint32 BitmapToPNGTranslator::mk_bmp_trnsfrm_none(0);
const uint32 BitmapToPNGTranslator::mk_bmp_trnsfrm_24_to_8c(1);
const uint32 BitmapToPNGTranslator::mk_bmp_trnsfrm_24_to_8g(mk_bmp_trnsfrm_24_to_8c << 1);
const uint32 BitmapToPNGTranslator::mk_bmp_trnsfrm_32_to_16g(mk_bmp_trnsfrm_24_to_8g << 1);
const uint32 BitmapToPNGTranslator::mk_bmp_trnsfrm_32_to_24(mk_bmp_trnsfrm_32_to_16g << 1);

BitmapToPNGTranslator::~BitmapToPNGTranslator() {
	// Get rid of any palette we allocated
	if (m_dest_png != NULL
		&& m_dest_info != NULL) {
		png_colorp plte(NULL);
		int num_colors;
		png_get_PLTE(m_dest_png, m_dest_info, &plte, &num_colors);
		
		if (plte != NULL) {
			png_free(m_dest_png, plte);
		}
	}
	
	if (m_dest_png != NULL) {
		png_destroy_write_struct(&m_dest_png, (m_dest_info != NULL) ? &m_dest_info : NULL);
	}
	
	delete [] m_row_buf;
	delete prefs;
}

status_t BitmapToPNGTranslator::Status(status_t *a_err) const {
	status_t err(m_status);
	
	if (err == B_NO_ERROR) {
		err = Translator::Status();
	}
	
	if (a_err != NULL) {
		*a_err = err;
	}
	
	return err;
}

status_t BitmapToPNGTranslator::PerformTranslation() {
	status_t err;
	
	if (Status(&err) != B_NO_ERROR || (err = setjmp(m_dest_png->jmpbuf)) != B_NO_ERROR) {
		return err;
	}
	
	png_set_write_fn(m_dest_png, this, static_cast<png_rw_ptr>(PNGCallbackWrite),
		static_cast<png_flush_ptr>(PNGCallbackFlush));
	
	// Write out the PNG image info
	if ((err = WriteHeader()) != B_NO_ERROR) {
		return err;
	}
	
	// We want to make sure the channels are in the right order and in
	// the right place
	switch (m_src_bmp_hdr.colors) {
		case B_RGBA32:
		case B_RGB32:
		case B_RGB24:
			png_set_bgr(m_dest_png);
			break;
		
		case B_RGBA32_BIG:
			png_set_swap_alpha(m_dest_png);
			break;
		
		default:
			break;
	}
	
	// In 1-bit images, we represent a black pixel as a 1, and a
	// white one as a 0
	png_set_invert_mono(m_dest_png);
	
	// Write out the image
	size_t num_passes(png_set_interlace_handling(m_dest_png));
	ssize_t row_bytes(m_src_bmp_hdr.rowBytes);
	
	for (size_t i(0); i < num_passes; i++) {
		// Start at the right spot
		if (m_src->Seek(sizeof (TranslatorBitmap), SEEK_SET) != sizeof (TranslatorBitmap)) {
			return B_IO_ERROR;
		}
		
		for (size_t j(0); j < m_src_bmp_hdr.bounds.Height() + 1.0; j++) {
			// Punch it through, baby
			if (m_src->Read(m_row_buf, m_src_bmp_hdr.rowBytes) < row_bytes) {
				return B_IO_ERROR;
			}
			
			// Make the necessary adjustments
			if ((err = PerformTransformations()) != B_NO_ERROR) {
				return err;
			}
			
			// Write out the row
			png_write_rows(m_dest_png, &m_row_buf, 1);
		}
	}
	png_write_end(m_dest_png, NULL);
	
	return B_NO_ERROR;
}

bool BitmapToPNGTranslator::RGBCallbackIsPalettable(void *a_data, const png_color *a_color) {
	Palette *plte(static_cast<Palette *>(a_data));
	size_t plte_index(0);
	
	// Try to find the color in the table
	while (plte_index < plte->m_used && !(plte->m_plte[plte_index].red == a_color->red
		&& plte->m_plte[plte_index].green == a_color->green
		&& plte->m_plte[plte_index].blue == a_color->blue))	{

		plte_index++;
	}
	
	// If it's a new color try to add it
	if (plte_index >= plte->m_used)	{
		if (plte->m_used >= plte->m_size) {
			return false;
		}
		
		plte->m_plte[plte->m_used++] = *a_color;
	}
	
	return true;
}

void BitmapToPNGTranslator::TransformRowRGB24toCMAP8(png_byte *a_row, size_t *a_row_bytes,
	const png_color *a_plte, const size_t a_plte_size, const size_t a_red_chnl,
	const size_t a_green_chnl, const size_t a_blue_chnl) {

	size_t orig_row_bytes(*a_row_bytes);
	*a_row_bytes /= 3;
	
	for (size_t i(0); i < *a_row_bytes; i++) {
		size_t j(0);
		
		while (j < UCHAR_MAX + 1 && j < a_plte_size
			&& !(a_plte[j].red == a_row[i * 3 + a_red_chnl]
			&& a_plte[j].green == a_row[i * 3 + a_green_chnl]
			&& a_plte[j].blue == a_row[i * 3 + a_blue_chnl])) {

			j++;
		}
		
		a_row[i] = static_cast<png_byte>(j % a_plte_size);
	}
	
	memset(&a_row[*a_row_bytes], 0x00, orig_row_bytes - *a_row_bytes);
}

void BitmapToPNGTranslator::TransformRowRGB24toGRAY8(png_byte *a_row, size_t *a_row_bytes) {
	size_t orig_row_bytes(*a_row_bytes);
	*a_row_bytes /= 3;
	int avg;
	
	for (size_t i(0); i < *a_row_bytes; i++) {
		avg = 0;
		avg += a_row[i * 3 + 0];
		avg += a_row[i * 3 + 1];
		avg += a_row[i * 3 + 2];
		avg /= 3;
		a_row[i] = avg;
	}
	
	memset(&a_row[*a_row_bytes], 0x00, orig_row_bytes - *a_row_bytes);
}

void BitmapToPNGTranslator::TransformRowRGB32toGRAY16(png_byte *a_row, size_t *a_row_bytes,
	const size_t a_alpha_chnl) {

	size_t orig_row_bytes(*a_row_bytes);
	*a_row_bytes /= 2;
	int avg;
	
	for (size_t i(0); i < *a_row_bytes;	i += 2) {
		avg = 0;
		
		for (size_t j(0); j < 4; j++) {
			avg += (j == a_alpha_chnl) ? 0 : a_row[i * 2 + j];
		}
		
		avg /= 3;
		a_row[i] = avg;
		a_row[i + 1] = a_row[i * 2 + a_alpha_chnl];
	}
	
	memset(&a_row[*a_row_bytes], 0x00, orig_row_bytes - *a_row_bytes);
}

void BitmapToPNGTranslator::TransformRowRGB32toRGB24(png_byte *a_row, size_t *a_row_bytes,
	const size_t a_dead_chnl) {

	size_t orig_row_bytes(*a_row_bytes);
	*a_row_bytes /= 4;
	
	for (size_t i(0); i < *a_row_bytes * 4;	i++) {
		a_row[i - (i + a_dead_chnl) / 4] = a_row[i];
	}
	
	*a_row_bytes *= 3;
	memset(&a_row[*a_row_bytes], 0x00, orig_row_bytes - *a_row_bytes);
}

bool BitmapToPNGTranslator::CheckRGBAttribute(RGBTestFunc const a_func, void *a_func_arg) {
	// Answer for the obvious cases
	bool four_chnls(false), bgr(true);
	
	switch (m_src_bmp_hdr.colors) {
		case B_RGBA32:
		case B_RGB32:
			four_chnls = true;
			bgr = true;
			break;
		case B_RGBA32_BIG:
		case B_RGB32_BIG:
			four_chnls = true;
			bgr = false;
			break;
		case B_RGB24:
			four_chnls = false;
			bgr = true;
			break;
		case B_RGB24_BIG:
			four_chnls = false;
			bgr = false;
			break;
		default:
			return false;
	}
	
	// Start at the right spot
	ssize_t bmp_size(sizeof (TranslatorBitmap));
	
	if (m_src->Seek(bmp_size, SEEK_SET) != bmp_size) {
		return false;
	}
	
	// Go through each pixel looking for colors
	ssize_t row_bytes(m_src_bmp_hdr.rowBytes);
	size_t base_scalar(four_chnls ? 4 : 3), red_offset(bgr ? 2 : (four_chnls ? 1 : 0)),
		green_offset(bgr ? 1 : (four_chnls ? 2 : 1)), blue_offset(bgr ? 0 : (four_chnls ? 3 : 2));
	
	for (size_t i(0); i < m_src_bmp_hdr.bounds.Height() + 1.0; i++) {
		if (m_src->Read(m_row_buf, row_bytes) != row_bytes) {
			return false;
		}
		
		for (size_t j(0); j < m_src_bmp_hdr.rowBytes / base_scalar; j++) {
			// Get the color of the current pixel
			png_color tmp;
			tmp.red = m_row_buf[j * base_scalar + red_offset];
			tmp.green = m_row_buf[j * base_scalar + green_offset];
			tmp.blue = m_row_buf[j * base_scalar + blue_offset];
			
			// If it's a new color try to add it
			if (!(*a_func)(a_func_arg, &tmp)) {
				return false;
			}
		}
	}
	
	return true;
}

void BitmapToPNGTranslator::Init() {
	prefs = new Prefs("PNGTranslatorSettings");

	status_t err;
	m_status = B_NO_ERROR;
	m_dest_png = NULL;
	m_dest_info = NULL;
	m_row_buf = NULL;
	m_bmp_trnsfrms = mk_bmp_trnsfrm_none;
	
	// Try to read the bitmap header
	if ((err = ReadSourceBitmapHeader()) != B_NO_ERROR) {
		m_status = err;
	}
	// Try to allocate space for the row (this assumes nothing will be
	// added to the end of the row)
	else if ((m_row_buf = new png_byte[m_src_bmp_hdr.rowBytes]) == NULL) {
		m_status = B_NO_MEMORY;
	}
	// Try to allocate the PNG image data structures
	else if ((m_dest_png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL
		|| (m_dest_info = png_create_info_struct(m_dest_png)) == NULL) {
		if (m_dest_png != NULL) {
			png_destroy_write_struct(&m_dest_png, (m_dest_info != NULL) ? &m_dest_info : NULL);
		}
		
		m_dest_png = NULL;
		m_dest_info = NULL;
		m_status = B_NO_MEMORY;
	}
}

bool BitmapToPNGTranslator::IsAlphaUnused() {
	// Answer for the obvious cases
	size_t alpha_chnl(3);
	
	switch (m_src_bmp_hdr.colors) {
		case B_RGB32:
		case B_RGB32_BIG:
		case B_RGB24:
		case B_RGB24_BIG:
		case B_CMAP8:
		case B_GRAY8:
		case B_GRAY1:
			return true;
		// We must venture forth
		case B_RGBA32:
			alpha_chnl = 3;
			break;
		case B_RGBA32_BIG:
			alpha_chnl = 0;
			break;
		default:
			return false;
	}
	
	// Start at the right spot
	ssize_t bmp_size(sizeof (TranslatorBitmap));
	
	if (m_src->Seek(bmp_size, SEEK_SET) != bmp_size) {
		return false;
	}
	
	// Go through each row looking for even a hint of alpha
	ssize_t row_bytes(m_src_bmp_hdr.rowBytes);
	
	for (size_t i(0); i < m_src_bmp_hdr.bounds.Height() + 1.0; i++) {
		if (m_src->Read(m_row_buf, row_bytes) != row_bytes) {
			return false;
		}
		
		for (size_t j(alpha_chnl); j < m_src_bmp_hdr.rowBytes; j += 4) {
			if (m_row_buf[j] != 0xff) {
				return false;
			}
		}
	}
	
	return true;
}

status_t BitmapToPNGTranslator::PerformTransformations() {
	size_t mod_row_bytes(m_src_bmp_hdr.rowBytes);
	
	// Perform any and all transformations necessary
	if (m_bmp_trnsfrms & mk_bmp_trnsfrm_32_to_24) {
		switch (m_src_bmp_hdr.colors) {
			case B_RGBA32:
			case B_RGB32:
				TransformRowRGB32toRGB24(m_row_buf, &mod_row_bytes, 0);
				break;
			case B_RGBA32_BIG:
			case B_RGB32_BIG:
				TransformRowRGB32toRGB24(m_row_buf, &mod_row_bytes, 3);
				break;
			default:
				return B_ILLEGAL_DATA;
		}
	}
	
	if (m_bmp_trnsfrms & mk_bmp_trnsfrm_32_to_16g) {
		// Check to make sure no transformations have been made yet
		if (m_bmp_trnsfrms & mk_bmp_trnsfrm_32_to_24) {
			return B_ILLEGAL_DATA;
		}
		
		switch (m_src_bmp_hdr.colors) {
			case B_RGBA32:
				TransformRowRGB32toGRAY16(m_row_buf, &mod_row_bytes, 0);
				break;
			case B_RGBA32_BIG:
				TransformRowRGB32toGRAY16(m_row_buf, &mod_row_bytes, 3);
				break;
			default:
				return B_ILLEGAL_DATA;
		}
	} else if (m_bmp_trnsfrms & mk_bmp_trnsfrm_24_to_8g) {
		switch (m_src_bmp_hdr.colors) {
			case B_RGBA32:
			case B_RGBA32_BIG:
			case B_RGB32:
			case B_RGB32_BIG:
				// Check to make sure a transformation has already
				// been made
				if (!(m_bmp_trnsfrms & mk_bmp_trnsfrm_32_to_24)) {
					return B_ILLEGAL_DATA;
				}
			case B_RGB24:
			case B_RGB24_BIG:
				TransformRowRGB24toGRAY8(m_row_buf, &mod_row_bytes);
				break;
			default:
				return B_ILLEGAL_DATA;
		}
	} else if (m_bmp_trnsfrms & mk_bmp_trnsfrm_24_to_8c) {
		png_colorp plte;
		int num_colors;
		png_get_PLTE(m_dest_png, m_dest_info, &plte, &num_colors);
		
		switch (m_src_bmp_hdr.colors) {
			case B_RGBA32:
			case B_RGB32:
				// Check to make sure a transformation has already
				// been made
				if (!(m_bmp_trnsfrms & mk_bmp_trnsfrm_32_to_24)) {
					return B_ILLEGAL_DATA;
				}
			case B_RGB24:
				TransformRowRGB24toCMAP8(m_row_buf, &mod_row_bytes, plte, num_colors, 2, 1, 0);
				break;
			case B_RGBA32_BIG:
			case B_RGB32_BIG:
				// Check to make sure a transformation has already
				// been made
				if (!(m_bmp_trnsfrms & mk_bmp_trnsfrm_32_to_24)) {
					return B_ILLEGAL_DATA;
				}
			case B_RGB24_BIG:
				TransformRowRGB24toCMAP8(m_row_buf, &mod_row_bytes, plte, num_colors, 0, 1, 2);
				break;
			default:
				return B_ILLEGAL_DATA;
		}
	}
	
	return B_NO_ERROR;
}

status_t BitmapToPNGTranslator::ReadSourceBitmapHeader() {
	// Try to read the header
	ssize_t bmp_size(sizeof (TranslatorBitmap));
	
	if (m_src->Seek(0, SEEK_SET) != 0 || m_src->Read(&m_src_bmp_hdr, bmp_size) < bmp_size) {
		return B_IO_ERROR;
	}
	
	// Make sure the endianness is correct
	m_src_bmp_hdr.magic = B_BENDIAN_TO_HOST_INT32(m_src_bmp_hdr.magic);
	m_src_bmp_hdr.bounds.left = B_BENDIAN_TO_HOST_FLOAT(m_src_bmp_hdr.bounds.left);
	m_src_bmp_hdr.bounds.top = B_BENDIAN_TO_HOST_FLOAT(m_src_bmp_hdr.bounds.top);
	m_src_bmp_hdr.bounds.right = B_BENDIAN_TO_HOST_FLOAT(m_src_bmp_hdr.bounds.right);
	m_src_bmp_hdr.bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(m_src_bmp_hdr.bounds.bottom);
	m_src_bmp_hdr.rowBytes = B_BENDIAN_TO_HOST_INT32(m_src_bmp_hdr.rowBytes);
	m_src_bmp_hdr.colors = static_cast<color_space>(B_BENDIAN_TO_HOST_INT32(m_src_bmp_hdr.colors));
	m_src_bmp_hdr.dataSize = B_BENDIAN_TO_HOST_INT32(m_src_bmp_hdr.dataSize);
	
	// Validate the bounding rectangle
	if (!m_src_bmp_hdr.bounds.IsValid()) {
		return B_ILLEGAL_DATA;
	}
	
	// Validate the row bytes
	size_t row_pixels(static_cast<size_t>(m_src_bmp_hdr.bounds.Width() + 1.0)), row_bits(row_pixels);
	
	switch (m_src_bmp_hdr.colors) {
		// Handle the 32-bit images
		case B_RGB32:
		case B_RGB32_BIG:
		case B_RGBA32:
		case B_RGBA32_BIG:
			row_bits *= 32;
			break;
		case B_RGB24:
		case B_RGB24_BIG:
			row_bits *= 24;
			break;
		// Handle the 8-bit images
		case B_CMAP8:
		case B_GRAY8:
			row_bits *= 8;
			break;
		// Handle the 1-bit images
		case B_GRAY1:
			row_bits *= 1;
			break;
		// The following are not currently supported
		case B_RGB16:
		case B_RGB16_BIG:
		case B_RGBA15:
		case B_RGBA15_BIG:
		case B_RGB15:
		case B_RGB15_BIG:
		case B_YUV422:
		case B_YUV411:
		case B_YUV420:
		case B_YUV444:
		case B_YUV9:
		case B_YUV12:
		case B_NO_COLOR_SPACE:
		default:
			return B_ILLEGAL_DATA;
	}
	
	size_t real_row_bytes;
	
	if ((real_row_bytes = row_bits / CHAR_BIT + (row_bits % CHAR_BIT == 0) ? 0 : 1) >
		m_src_bmp_hdr.rowBytes)	{

		return B_ILLEGAL_DATA;
	}
	
	// Check the indicated data size
	size_t real_data_size;
	
	if ((real_data_size = static_cast<size_t>(m_src_bmp_hdr.rowBytes *
		m_src_bmp_hdr.bounds.Height())) > m_src_bmp_hdr.dataSize) {

		return B_ILLEGAL_DATA;
	}
	
	return B_NO_ERROR;
}

void BitmapToPNGTranslator::SetPhysicalProperties() {
	// Set up the physical properties
	png_set_pHYs(m_dest_png, m_dest_info, prefs->res_x, prefs->res_y, prefs->res_units);
	png_set_oFFs(m_dest_png, m_dest_info, prefs->offset_x, prefs->offset_y, prefs->offset_units);
}

status_t BitmapToPNGTranslator::WriteHeader() {
	// Set the basic header information
	size_t width(static_cast<size_t>(m_src_bmp_hdr.bounds.Width() + 1.0)),
		height(static_cast<size_t>(m_src_bmp_hdr.bounds.Height() + 1.0));
	
	int bit_depth(8), color_type(PNG_COLOR_TYPE_GRAY);
	png_free(m_dest_png, m_dest_info->palette);
	m_dest_info->palette = NULL;
	const color_map *map(system_colors());
	Palette plte;
	plte.m_plte = NULL;
	plte.m_size = 0;
	plte.m_used = 0;
	
	switch (m_src_bmp_hdr.colors) {
		case B_RGBA32:
		case B_RGBA32_BIG:
		case B_RGB32:
		case B_RGB32_BIG:
		case B_RGB24:
		case B_RGB24_BIG:
			bit_depth = 8;
			color_type = PNG_COLOR_TYPE_RGB_ALPHA;
			
			// Check for an unused alpha channel
			if (IsAlphaUnused()) {
				switch (m_src_bmp_hdr.colors) {
					case B_RGBA32:
					case B_RGBA32_BIG:
					case B_RGB32:
					case B_RGB32_BIG:
						m_bmp_trnsfrms |= mk_bmp_trnsfrm_32_to_24;
					default:
						break;
				}
				
				color_type = PNG_COLOR_TYPE_RGB;
				
				// Check to see if we contain any colors
				if (CheckRGBAttribute(RGBCallbackIsMono)) {
					m_bmp_trnsfrms |= mk_bmp_trnsfrm_24_to_8g;
					// Using bit depth writes 1 byte per pixel - this is bad
					//bit_depth = 1;
					bit_depth = 8;
					color_type = PNG_COLOR_TYPE_GRAY;
					png_set_packing(m_dest_png);
					break;
				}
				
				// Check to see if we contain any colors
				if (CheckRGBAttribute(RGBCallbackIsGray)) {
					m_bmp_trnsfrms |= mk_bmp_trnsfrm_24_to_8g;
					color_type = PNG_COLOR_TYPE_GRAY;
					break;
				}
				
				// Check to see if we can reduce the colors
				plte.m_size = UCHAR_MAX + 1;
				
				if ((plte.m_plte = static_cast<png_colorp>(png_malloc(m_dest_png,
					plte.m_size * sizeof (png_color)))) == NULL) {

					return B_NO_MEMORY;
				}
				
				if (CheckRGBAttribute(RGBCallbackIsPalettable, &plte)) {
					m_bmp_trnsfrms |= mk_bmp_trnsfrm_24_to_8c;
					color_type = PNG_COLOR_TYPE_PALETTE;
					break;
				}
				
				png_free(m_dest_png, plte.m_plte);
				plte.m_plte = NULL;
				plte.m_size = 0;
				plte.m_used = 0;
			}
			// Check to see if we contain any colors
			else if (CheckRGBAttribute(RGBCallbackIsGray)) {
				m_bmp_trnsfrms |= mk_bmp_trnsfrm_32_to_16g;
				color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
				break;
			}
			break;
		case B_CMAP8:
			bit_depth = 8;
			color_type = PNG_COLOR_TYPE_PALETTE;
			plte.m_size = plte.m_used = UCHAR_MAX + 1;
			
			if ((plte.m_plte = static_cast<png_colorp>(png_malloc(m_dest_png,
				plte.m_size * sizeof (png_color)))) == NULL) {

				return B_NO_MEMORY;
			}
			
			// Copy over the palette (notice the alpha channel is ignored)
			for (size_t i(0); i < plte.m_size; i++) {
				plte.m_plte[i].red = map->color_list[i].red;
				plte.m_plte[i].green = map->color_list[i].green;
				plte.m_plte[i].blue = map->color_list[i].blue;
			}
			break;
		case B_GRAY8:
			bit_depth = 8;
			color_type = PNG_COLOR_TYPE_GRAY;
			break;
		case B_GRAY1:
			bit_depth = 1;
			color_type = PNG_COLOR_TYPE_GRAY;
			break;
		default:
			return B_ILLEGAL_DATA;
	}
	
	// Set the palette if there is one
	if (plte.m_plte != NULL) {
		png_set_PLTE(m_dest_png, m_dest_info, plte.m_plte, plte.m_used);
	}
	
	png_set_IHDR(m_dest_png, m_dest_info, width, height, bit_depth, color_type,
		prefs->interlacing, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	SetPhysicalProperties();
	
	// Invert the alpha channel (see the comment in
	// translatePNGToBitmap())
	//png_set_invert_alpha(m_dest_png);
	
	// Write the info struct
	png_write_info(m_dest_png, m_dest_info);

	return B_NO_ERROR;
}