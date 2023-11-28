/*--------------------------------------------------------------------*\
  File:      PNGTranslator.cpp
  Creator:   Matt Bogosian <mattb@be.com>
  Copyright: (c)1998, Be, Inc. All rights reserved.
  Description: Source file containing the member functions for the PNG
      image format-specific Translator classes.
\*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-=- Included Files -=-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#include "PNGTranslator.h"
#include "AddOn.h"

#include <ByteOrder.h>
#include <DataIO.h>
#include <Debug.h>
#include <InterfaceDefs.h>
#include <Message.h>

#include <limits.h>


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-= Definitions, Enums, Typedefs, Consts =-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

// Initialize the BitmapToPNGTranslator private static const members
const ssize_t BitmapToPNGTranslator::mk_png_check_bytes(8);
const uint32 BitmapToPNGTranslator::mk_bmp_trnsfrm_none(0);
const uint32 BitmapToPNGTranslator::mk_bmp_trnsfrm_24_to_8c(1);
const uint32 BitmapToPNGTranslator::mk_bmp_trnsfrm_24_to_8g(mk_bmp_trnsfrm_24_to_8c << 1);
const uint32 BitmapToPNGTranslator::mk_bmp_trnsfrm_32_to_8c(mk_bmp_trnsfrm_24_to_8g << 1);
const uint32 BitmapToPNGTranslator::mk_bmp_trnsfrm_32_to_16g(mk_bmp_trnsfrm_32_to_8c << 1);
const uint32 BitmapToPNGTranslator::mk_bmp_trnsfrm_32_to_24(mk_bmp_trnsfrm_32_to_16g << 1);

//	PNGToBitmapTranslator statics
const ssize_t PNGToBitmapTranslator::mk_png_check_bytes(8);


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-= Function Definitions =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

//====================================================================
BitmapToPNGTranslator::~BitmapToPNGTranslator(void)
//====================================================================
{
	// Get rid of any palette we allocated
	if (m_dest_png != NULL
		&& m_dest_info != NULL)
	{
		png_colorp plte(NULL);
		int num_colors;
		png_get_PLTE(m_dest_png, m_dest_info, &plte, &num_colors);
		png_bytep trans(NULL);
		int num_trans;
		png_color_16p trans_values(NULL);
		png_get_tRNS(m_dest_png, m_dest_info, &trans, &num_trans, &trans_values);
		
		if (plte != NULL)
		{
			png_free(m_dest_png, plte);
		}
		if (trans != NULL)
		{
			PRINT(("Freeing my transparency %p\n", trans));
			png_free(m_dest_png, trans);
		}
	}
	
	if (m_dest_png != NULL)
	{
		png_destroy_write_struct(&m_dest_png, (m_dest_info != NULL) ? &m_dest_info : NULL);
	}
	
	delete [] m_row_buf;
}

//====================================================================
status_t BitmapToPNGTranslator::Status(status_t * const a_err) const
//====================================================================
{
	status_t err(m_status);
	
	if (err == B_NO_ERROR)
	{
		err = Inherited::Status();
	}
	
	if (a_err != NULL)
	{
		*a_err = err;
	}
	
	return err;
}

//====================================================================
status_t BitmapToPNGTranslator::PerformTranslation(void)
//====================================================================
{
	status_t err;
	
	if (Status(&err) != B_NO_ERROR
		|| (err = setjmp(m_dest_png->jmpbuf)) != B_NO_ERROR)
	{
		return err;
	}
	
	png_set_write_fn(m_dest_png, this, static_cast<png_rw_ptr>(PNGCallbackWrite), static_cast<png_flush_ptr>(PNGCallbackFlush));
	
	// Write out the PNG image info
	if ((err = WriteHeader()) != B_NO_ERROR)
	{
		return err;
	}
	
	// We want to make sure the channels are in the right order and in
	// the right place
	switch (m_src_bmp_hdr.colors)
	{
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
	
	for (size_t i(0);
		i < num_passes;
		i++)
	{
		// Start at the right spot
		if (SeekSource(sizeof (TranslatorBitmap)) != sizeof (TranslatorBitmap))
		{
			return B_IO_ERROR;
		}
		
		for (size_t j(0);
			j < m_src_bmp_hdr.bounds.Height() + 1.0;
			j++)
		{
			// Punch it through, baby
			if (ReadSource(m_row_buf, m_src_bmp_hdr.rowBytes) < row_bytes)
			{
				return B_IO_ERROR;
			}
			
			// Make the necessary adjustments
			if ((err = PerformTransformations()) != B_NO_ERROR)
			{
				return err;
			}
			
			// Write out the row
			png_write_rows(m_dest_png, &m_row_buf, 1);
		}
	}
	png_write_end(m_dest_png, NULL);
	
	return B_NO_ERROR;
}

//====================================================================
void BitmapToPNGTranslator::TransformRowRGB24toCMAP8(png_byte * const a_row, size_t * const a_row_bytes, const png_color * const a_plte, const size_t a_plte_size, const size_t a_red_chnl, const size_t a_green_chnl, const size_t a_blue_chnl)
//====================================================================
{
	size_t orig_row_bytes(*a_row_bytes);
	*a_row_bytes /= 3;
	
	for (size_t i(0);
		i < *a_row_bytes;
		i++)
	{
		size_t j(0);
		
		while (j < UCHAR_MAX + 1
			&& j < a_plte_size
			&& !(a_plte[j].red == a_row[i * 3 + a_red_chnl]
				&& a_plte[j].green == a_row[i * 3 + a_green_chnl]
				&& a_plte[j].blue == a_row[i * 3 + a_blue_chnl]))
		{
			j++;
		}
		
		a_row[i] = static_cast<png_byte>(j % a_plte_size);
	}
	
	memset(&a_row[*a_row_bytes], 0x00, orig_row_bytes - *a_row_bytes);
}

//====================================================================
void BitmapToPNGTranslator::TransformRowRGB32toCMAP8(png_byte * const a_row, size_t * const a_row_bytes, const png_color * const a_plte, const png_byte * const a_trans, const size_t a_plte_size, const size_t a_red_chnl, const size_t a_green_chnl, const size_t a_blue_chnl, const size_t a_alpha_chnl)
//====================================================================
{
	size_t orig_row_bytes(*a_row_bytes);
	*a_row_bytes /= 4;
	
	for (size_t i(0);
		i < *a_row_bytes;
		i++)
	{
		size_t j(0);
		
		while (j < UCHAR_MAX + 1
			&& j < a_plte_size
			&& !(a_plte[j].red == a_row[i * 4 + a_red_chnl]
				&& a_plte[j].green == a_row[i * 4 + a_green_chnl]
				&& a_plte[j].blue == a_row[i * 4 + a_blue_chnl]
				&& a_trans[j] == a_row[i * 4 + a_alpha_chnl]))
		{
			j++;
		}
		
		a_row[i] = static_cast<png_byte>(j % a_plte_size);
	}
	
	memset(&a_row[*a_row_bytes], 0x00, orig_row_bytes - *a_row_bytes);
}

//====================================================================
void BitmapToPNGTranslator::TransformRowRGB24toGRAY8(png_byte * const a_row, size_t * const a_row_bytes)
//====================================================================
{
	size_t orig_row_bytes(*a_row_bytes);
	*a_row_bytes /= 3;
	int avg;
	
	for (size_t i(0);
		i < *a_row_bytes;
		i++)
	{
		avg = 0;
		avg += a_row[i * 3 + 0];
		avg += a_row[i * 3 + 1];
		avg += a_row[i * 3 + 2];
		avg /= 3;
		a_row[i] = avg;
	}
	
	memset(&a_row[*a_row_bytes], 0x00, orig_row_bytes - *a_row_bytes);
}

//====================================================================
void BitmapToPNGTranslator::TransformRowRGB32toGRAY16(png_byte * const a_row, size_t * const a_row_bytes, const size_t a_alpha_chnl)
//====================================================================
{
	size_t orig_row_bytes(*a_row_bytes);
	*a_row_bytes /= 2;
	int avg;
	
	for (size_t i(0);
		i < *a_row_bytes;
		i += 2)
	{
		avg = 0;
		
		for (size_t j(0);
			j < 4;
			j++)
		{
			avg += (j == a_alpha_chnl) ? 0 : a_row[i * 2 + j];
		}
		
		avg /= 3;
		a_row[i] = avg;
		a_row[i + 1] = a_row[i * 2 + a_alpha_chnl];
	}
	
	memset(&a_row[*a_row_bytes], 0x00, orig_row_bytes - *a_row_bytes);
}

//====================================================================
void BitmapToPNGTranslator::TransformRowRGB32toRGB24(png_byte * const a_row, size_t * const a_row_bytes, const size_t a_dead_chnl)
//====================================================================
{
	size_t orig_row_bytes(*a_row_bytes);
	*a_row_bytes /= 4;
	
	for (size_t i(0);
		i < *a_row_bytes * 4;
		i++)
	{
		a_row[i - (i + a_dead_chnl) / 4] = a_row[i];
	}
	
	*a_row_bytes *= 3;
	memset(&a_row[*a_row_bytes], 0x00, orig_row_bytes - *a_row_bytes);
}

//====================================================================
bool BitmapToPNGTranslator::CheckRGBAttributes(Palette* a_palette)
//====================================================================
{
	// Answer for the obvious cases
	bool four_chnls(false), bgr(true);
	
	a_palette->m_is_bw = a_palette->m_is_gray = a_palette->m_is_palette = false;
	
	switch (m_src_bmp_hdr.colors)
	{
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
	
	if (SeekSource(bmp_size) != bmp_size)
	{
		return false;
	}
	
	a_palette->m_is_bw = a_palette->m_is_gray = true;
	a_palette->m_is_palette = a_palette->m_plte ? true : false;
	
	// Go through each pixel looking for colors
	const ssize_t row_bytes(m_src_bmp_hdr.rowBytes);
	const size_t base_scalar(four_chnls ? 4 : 3);
	const size_t alpha_offset(bgr ? 3 : 0);
	const size_t red_offset(bgr ? 2 : (four_chnls ? 1 : 0));
	const size_t green_offset(bgr ? 1 : (four_chnls ? 2 : 1));
	const size_t blue_offset(bgr ? 0 : (four_chnls ? 3 : 2));
	
	for (size_t i(0);
		i < m_src_bmp_hdr.bounds.Height() + 1.0;
		i++)
	{
		if (ReadSource(m_row_buf, row_bytes) != row_bytes)
		{
			return false;
		}
		
		for (size_t j(0);
			j < m_src_bmp_hdr.rowBytes / base_scalar;
			j++)
		{
			// Get the color of the current pixel
			png_color_8 col;
			col.red = m_row_buf[j * base_scalar + red_offset];
			col.green = m_row_buf[j * base_scalar + green_offset];
			col.blue = m_row_buf[j * base_scalar + blue_offset];
			col.alpha = four_chnls
					  ? (m_row_buf[j * base_scalar + alpha_offset])
					  : 255;
			
			if (a_palette->m_is_bw)
			{
				if (((col.red != 0 || col.green != 0 || col.blue != 0) &&
					(col.red != 255 || col.green != 255 || col.blue != 255)))
				{
					PRINT(("Not monochrome!\n"));
					a_palette->m_is_bw = false;
				}
			}
			if (a_palette->m_is_gray)
			{
				if ((col.red != col.green) || (col.green != col.blue))
				{
					PRINT(("Not grayscale!\n"));
					a_palette->m_is_gray = false;
				}
			}
			if (a_palette->m_is_palette)
			{
				png_colorp const plte = a_palette->m_plte;
				png_bytep trans = a_palette->m_trans;
				const size_t N = a_palette->m_used;
				size_t plte_index(0);
				
				// Try to find the color in the table
				while (plte_index < N
					&& !(plte[plte_index].red == col.red
						&& plte[plte_index].green == col.green
						&& plte[plte_index].blue == col.blue
						&& (trans ? trans[plte_index] : 255) == col.alpha))
				{
					plte_index++;
				}
				
				// If it's a new color try to add it
				if (plte_index >= N)
				{
					if (plte_index >= a_palette->m_size)
					{
						PRINT(("Not palette!\n"));
						a_palette->m_is_palette = false;
					}
					else
					{
						PRINT(("Creating new color index %ld (%d,%d,%d)\n",
								plte_index, col.red, col.green, col.blue));
						plte[N].red = col.red;
						plte[N].green = col.green;
						plte[N].blue = col.blue;
						if (col.alpha != 255 && !trans) {
							trans = a_palette->m_trans = static_cast<png_bytep>(
								png_malloc(m_dest_png, a_palette->m_size * sizeof (png_byte)));
							PRINT(("Allocated transparency %p #%ld\n",
											trans, a_palette->m_size));
							if (trans) {
								for (int32 i=0; i<N-1; i++) {
									PRINT(("Initializing #%ld\n", i));
									trans[i] = 255;
								}
							}
						}
						if (trans) {
							PRINT(("Alpha channel is %d\n", col.alpha));
							trans[N] = col.alpha;
						}
						a_palette->m_used++;
					}
				}
			}
			
			// If all the attributes have failed, bail now.
			if (!a_palette->m_is_bw &&
					!a_palette->m_is_gray &&
					!a_palette->m_is_palette)
			{
				return true;
			}
		}
	}
	
	return true;
}

//====================================================================
void BitmapToPNGTranslator::Init(BMessage * a_io_ext)
//====================================================================
{
	status_t err;
	m_status = B_NO_ERROR;
	m_dest_png = NULL;
	m_dest_info = NULL;
	m_row_buf = NULL;
	m_bmp_trnsfrms = mk_bmp_trnsfrm_none;
	
	m_io_ext = a_io_ext;
	m_color_space = B_NO_COLOR_SPACE;
	if (m_io_ext) {
		int32 space;
		if (m_io_ext->FindInt32(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE, &space) == B_OK) {
			m_color_space = (color_space)space;
		}
	}
	
	// Try to read the bitmap header
	if ((err = ReadSourceBitmapHeader()) != B_NO_ERROR)
	{
		m_status = err;
	}
	// Try to allocate space for the row (this assumes nothing will be
	// added to the end of the row)
	else if ((m_row_buf = new png_byte[m_src_bmp_hdr.rowBytes]) == NULL)
	{
		m_status = B_NO_MEMORY;
	}
	// Try to allocate the PNG image data structures
	else if ((m_dest_png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL
		|| (m_dest_info = png_create_info_struct(m_dest_png)) == NULL)
	{
		if (m_dest_png != NULL)
		{
			png_destroy_write_struct(&m_dest_png, (m_dest_info != NULL) ? &m_dest_info : NULL);
		}
		
		m_dest_png = NULL;
		m_dest_info = NULL;
		m_status = B_NO_MEMORY;
	}
}

//====================================================================
bool BitmapToPNGTranslator::IsAlphaUnused(void)
//====================================================================
{
	// Answer for the obvious cases
	size_t alpha_chnl(3);
	
	switch (m_src_bmp_hdr.colors)
	{
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
	
	if (SeekSource(bmp_size) != bmp_size)
	{
		return false;
	}
	
	// Go through each row looking for even a hint of alpha
	ssize_t row_bytes(m_src_bmp_hdr.rowBytes);
	
	for (size_t i(0);
		i < m_src_bmp_hdr.bounds.Height() + 1.0;
		i++)
	{
		if (ReadSource(m_row_buf, row_bytes) != row_bytes)
		{
			return false;
		}
		
		for (size_t j(alpha_chnl);
			j < m_src_bmp_hdr.rowBytes;
			j += 4)
		{
			if (m_row_buf[j] != 0xff)
			{
				return false;
			}
		}
	}
	
	return true;
}

//====================================================================
status_t BitmapToPNGTranslator::PerformTransformations(void)
//====================================================================
{
	size_t mod_row_bytes(m_src_bmp_hdr.rowBytes);
	
	// Perform any and all transformations necessary
	if (m_bmp_trnsfrms & mk_bmp_trnsfrm_32_to_24)
	{
		switch (m_src_bmp_hdr.colors)
		{
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
	
	if (m_bmp_trnsfrms & mk_bmp_trnsfrm_32_to_16g)
	{
		// Check to make sure no transformations have been made yet
		if (m_bmp_trnsfrms & mk_bmp_trnsfrm_32_to_24)
		{
			return B_ILLEGAL_DATA;
		}
		
		switch (m_src_bmp_hdr.colors)
		{
			case B_RGBA32:
				TransformRowRGB32toGRAY16(m_row_buf, &mod_row_bytes, 3);
				break;
			
			case B_RGBA32_BIG:
				TransformRowRGB32toGRAY16(m_row_buf, &mod_row_bytes, 0);
				break;
			
			default:
				return B_ILLEGAL_DATA;
	}
	}
	else if (m_bmp_trnsfrms & mk_bmp_trnsfrm_24_to_8g)
	{
		switch (m_src_bmp_hdr.colors)
		{
			case B_RGBA32:
			case B_RGBA32_BIG:
			case B_RGB32:
			case B_RGB32_BIG:
				// Check to make sure a transformation has already
				// been made
				if (!(m_bmp_trnsfrms & mk_bmp_trnsfrm_32_to_24))
				{
					return B_ILLEGAL_DATA;
				}
			
			case B_RGB24:
			case B_RGB24_BIG:
				TransformRowRGB24toGRAY8(m_row_buf, &mod_row_bytes);
				break;
			
			default:
				return B_ILLEGAL_DATA;
		}
	}
	else if (m_bmp_trnsfrms & mk_bmp_trnsfrm_32_to_8c)
	{
		// Check to make sure no transformations have been made yet
		if (m_bmp_trnsfrms & mk_bmp_trnsfrm_32_to_24)
		{
			return B_ILLEGAL_DATA;
		}
		
		png_colorp plte;
		int num_colors;
		png_get_PLTE(m_dest_png, m_dest_info, &plte, &num_colors);
		png_bytep trans;
		int num_trans;
		png_color_16p trans_values;
		png_get_tRNS(m_dest_png, m_dest_info, &trans, &num_trans, &trans_values);
		
		if (!trans || num_colors != num_trans)
		{
			return B_ILLEGAL_DATA;
		}
		
		switch (m_src_bmp_hdr.colors)
		{
			case B_RGBA32:
			case B_RGB32:
				TransformRowRGB32toCMAP8(m_row_buf, &mod_row_bytes, plte, trans, num_colors, 2, 1, 0, 3);
				break;
			
			case B_RGBA32_BIG:
			case B_RGB32_BIG:
				TransformRowRGB32toCMAP8(m_row_buf, &mod_row_bytes, plte, trans, num_colors, 1, 2, 3, 0);
				break;
			
			default:
				return B_ILLEGAL_DATA;
		}
	}
	else if (m_bmp_trnsfrms & mk_bmp_trnsfrm_24_to_8c)
	{
		png_colorp plte;
		int num_colors;
		png_get_PLTE(m_dest_png, m_dest_info, &plte, &num_colors);
		
		switch (m_src_bmp_hdr.colors)
		{
			case B_RGBA32:
			case B_RGB32:
				// Check to make sure a transformation has already
				// been made
				if (!(m_bmp_trnsfrms & mk_bmp_trnsfrm_32_to_24))
				{
					return B_ILLEGAL_DATA;
				}
			
			case B_RGB24:
				TransformRowRGB24toCMAP8(m_row_buf, &mod_row_bytes, plte, num_colors, 2, 1, 0);
				break;
			
			case B_RGBA32_BIG:
			case B_RGB32_BIG:
				// Check to make sure a transformation has already
				// been made
				if (!(m_bmp_trnsfrms & mk_bmp_trnsfrm_32_to_24))
				{
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

//====================================================================
status_t BitmapToPNGTranslator::ReadSourceBitmapHeader(void)
//====================================================================
{
	// Try to read the header
	ssize_t bmp_size(sizeof (TranslatorBitmap));
	
	if (SeekSource(0) != 0
		|| ReadSource(&m_src_bmp_hdr, bmp_size) < bmp_size)
	{
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
	if (!m_src_bmp_hdr.bounds.IsValid())
	{
		return B_ILLEGAL_DATA;
	}
	
	// Validate the row bytes
	size_t row_pixels(static_cast<size_t>(m_src_bmp_hdr.bounds.Width() + 1.0)), row_bits(row_pixels);
	
	switch (m_src_bmp_hdr.colors)
	{
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
		
		// The following are not supported
		case B_NO_COLOR_SPACE:
		default:
			return B_ILLEGAL_DATA;
	}
	
	size_t real_row_bytes;
	
	if ((real_row_bytes = row_bits / CHAR_BIT + (row_bits % CHAR_BIT == 0) ? 0 : 1) > m_src_bmp_hdr.rowBytes)
	{
		return B_ILLEGAL_DATA;
	}
	
	// Check the indicated data size
	size_t real_data_size;
	
	if ((real_data_size = static_cast<size_t>(m_src_bmp_hdr.rowBytes * m_src_bmp_hdr.bounds.Height())) > m_src_bmp_hdr.dataSize)
	{
		return B_ILLEGAL_DATA;
	}
	
	return B_NO_ERROR;
}

//====================================================================
void BitmapToPNGTranslator::SetPhysicalProperties(void)
//====================================================================
{
	// Set up the physical properties
	size_t res_x, res_y, offset_x, offset_y;
	int32 res_units, offset_units;
	
	if (GetConfigMessage()->FindInt32(k_config_res_x, reinterpret_cast<int32 *>(&res_x)) != B_NO_ERROR)
	{
		res_x = k_default_res_x;
	}
	
	if (GetConfigMessage()->FindInt32(k_config_res_y, reinterpret_cast<int32 *>(&res_y)) != B_NO_ERROR)
	{
		res_y = k_default_res_y;
	}
	
	if (GetConfigMessage()->FindInt32(k_config_res_units, &res_units) != B_NO_ERROR)
	{
		res_units = k_default_res_units;
	}
	
	if (GetConfigMessage()->FindInt32(k_config_offset_x, reinterpret_cast<int32 *>(&offset_x)) != B_NO_ERROR)
	{
		offset_x = k_default_offset_x;
	}
	
	if (GetConfigMessage()->FindInt32(k_config_offset_y, reinterpret_cast<int32 *>(&offset_y)) != B_NO_ERROR)
	{
		offset_y = k_default_offset_y;
	}
	
	if (GetConfigMessage()->FindInt32(k_config_offset_units, &offset_units) != B_NO_ERROR)
	{
		offset_units = k_default_offset_units;
	}
	
	png_set_pHYs(m_dest_png, m_dest_info, res_x, res_y, res_units);
	png_set_oFFs(m_dest_png, m_dest_info, offset_x, offset_y, offset_units);
}

//====================================================================
status_t BitmapToPNGTranslator::WriteHeader(void)
//====================================================================
{
	// Set the basic header information
	size_t width(static_cast<size_t>(m_src_bmp_hdr.bounds.Width() + 1.0)), height(static_cast<size_t>(m_src_bmp_hdr.bounds.Height() + 1.0));
	int32 intrlcng;
	
	if (GetConfigMessage()->FindInt32(k_config_intrlcng, &intrlcng) != B_NO_ERROR)
	{
		intrlcng = k_default_intrlcng;
	}
	
	int bit_depth(8), color_type(PNG_COLOR_TYPE_GRAY);
	png_free(m_dest_png, m_dest_info->palette);
	m_dest_info->palette = NULL;
	PRINT(("Freeing old transparency %p\n", m_dest_info->trans));
	png_free(m_dest_png, m_dest_info->trans);
	m_dest_info->trans = NULL;
	const color_map *map(system_colors());
	Palette plte;
	plte.m_size = UCHAR_MAX+1;
	plte.m_used = 0;
	plte.m_plte = static_cast<png_colorp>(png_malloc(m_dest_png, plte.m_size * sizeof (png_color)));
	if (plte.m_plte == NULL) return B_NO_MEMORY;
	plte.m_trans = NULL;
	plte.m_is_bw = plte.m_is_gray = plte.m_is_palette = false;
	
	bool have_space = false;
	bool have_alpha = false;
			
	switch (m_src_bmp_hdr.colors)
	{
		case B_RGBA32:
		case B_RGBA32_BIG:
		case B_RGB32:
		case B_RGB32_BIG:
		case B_RGB24:
		case B_RGB24_BIG:
			bit_depth = 8;
			color_type = PNG_COLOR_TYPE_RGB_ALPHA;
			
			// Check for an unused alpha channel only if the
			// caller isn't explicitly asking for a color space.
			switch (m_color_space) {
				case B_RGBA32:
				case B_RGBA32_BIG:
					have_space = have_alpha = true;
					break;
				case B_RGB32:
				case B_RGB32_BIG:
				case B_RGB24:
				case B_RGB24_BIG:
					have_space = true;
					break;
				default:
					break;
			}
			
			// Check color characteristics without alpha channel.
			if ((have_space && !have_alpha) || (!have_space && IsAlphaUnused()))
			{
				switch (m_src_bmp_hdr.colors)
				{
					case B_RGBA32:
					case B_RGBA32_BIG:
					case B_RGB32:
					case B_RGB32_BIG:
						m_bmp_trnsfrms |= mk_bmp_trnsfrm_32_to_24;
						break;
						
					default:
						break;
				}
				
				PRINT(("Image without alpha.\n"));
				color_type = PNG_COLOR_TYPE_RGB;
				
				// Check what kinds of colors are in the image.
				if (CheckRGBAttributes(&plte))
				{
					if (plte.m_is_bw)
					{
						PRINT(("Monochrome image.\n"));
						m_bmp_trnsfrms |= mk_bmp_trnsfrm_24_to_8g;
						// Using bit depth writes 1 byte per pixel - this is bad
						//bit_depth = 1;
						bit_depth = 8;
						color_type = PNG_COLOR_TYPE_GRAY;
						png_set_packing(m_dest_png);
						plte.m_is_palette = false;
					
					}
					else if (plte.m_is_gray)
					{
						PRINT(("Grayscale image.\n"));
						m_bmp_trnsfrms |= mk_bmp_trnsfrm_24_to_8g;
						color_type = PNG_COLOR_TYPE_GRAY;
						plte.m_is_palette = false;
					
					}
					else if (plte.m_is_palette)
					{
						PRINT(("Palette image.\n"));
						m_bmp_trnsfrms |= mk_bmp_trnsfrm_24_to_8c;
						color_type = PNG_COLOR_TYPE_PALETTE;
					}
				}
			}
			// Check color characteristics with alpha channel.
			else
			{
				PRINT(("Image with alpha.\n"));
				
				// For a palette image, we still -must- have a transparency
				// chunk.  The caller could have requested, say, B_RGBA_32,
				// even though there is no alpha in the image.
				plte.m_trans = static_cast<png_bytep>(
					png_malloc(m_dest_png, plte.m_size * sizeof (png_byte)));
				if (plte.m_trans == NULL) {
					png_free(m_dest_png, plte.m_plte);
					return B_NO_MEMORY;
				}
				
				// Check what kinds of colors are in the image.
				if (CheckRGBAttributes(&plte))
				{
					if (plte.m_is_gray)
					{
						PRINT(("Grayscale image.\n"));
						m_bmp_trnsfrms |= mk_bmp_trnsfrm_32_to_16g;
						color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
						plte.m_is_palette = false;
					}
					else if (plte.m_is_palette)
					{
						PRINT(("Palette image.\n"));
						m_bmp_trnsfrms |= mk_bmp_trnsfrm_32_to_8c;
						color_type = PNG_COLOR_TYPE_PALETTE;
					}
				}
			}
			
			break;
		
		case B_CMAP8:
			bit_depth = 8;
			color_type = PNG_COLOR_TYPE_PALETTE;
			plte.m_is_palette = true;
			
			if ((plte.m_trans = static_cast<png_bytep>(
					png_malloc(m_dest_png, plte.m_size * sizeof (png_byte)))) == NULL)
			{
				return B_NO_MEMORY;
			}
			
			// Copy over the palette, marking last entry as transparent
			for (size_t i(0);
				i < plte.m_size;
				i++)
			{
				plte.m_plte[i].red = map->color_list[i].red;
				plte.m_plte[i].green = map->color_list[i].green;
				plte.m_plte[i].blue = map->color_list[i].blue;
				plte.m_trans[i] = (i == plte.m_size-1) ? 0 : 255;
			}
			
			break;
		
		case B_GRAY8:
			bit_depth = 8;
			color_type = PNG_COLOR_TYPE_GRAY;
			plte.m_is_gray = true;
			break;
		
		case B_GRAY1:
			bit_depth = 1;
			color_type = PNG_COLOR_TYPE_GRAY;
			plte.m_is_bw = true;
			break;
		
		default:
			return B_ILLEGAL_DATA;
	}
	
	// Set the palette if there is one
	if (plte.m_is_palette && plte.m_plte != NULL)
	{
		PRINT(("Using palette of %ld entries at %p\n",
					plte.m_used, plte.m_plte));
		png_set_PLTE(m_dest_png, m_dest_info, plte.m_plte, plte.m_used);
	}
	else if (plte.m_plte)
	{
		if (plte.m_plte) png_free(m_dest_png, plte.m_plte);
	}
	
	if (plte.m_is_palette && plte.m_trans != NULL)
	{
		png_set_tRNS(m_dest_png, m_dest_info, plte.m_trans, plte.m_used, NULL);
		PRINT(("Set transparency to %p\n", plte.m_trans));
	}
	else if (plte.m_trans)
	{
		PRINT(("Freeing unused transparency %p\n", plte.m_trans));
		if (plte.m_trans) png_free(m_dest_png, plte.m_trans);
	}
	
	png_set_IHDR(m_dest_png, m_dest_info, width, height, bit_depth, color_type, intrlcng, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	SetPhysicalProperties();
	
	// Invert the alpha channel (see the comment in
	// translatePNGToBitmap())
	//png_set_invert_alpha(m_dest_png);
	
	// Write the info struct
	png_write_info(m_dest_png, m_dest_info);

	return B_NO_ERROR;
}

//====================================================================
PNGToBitmapTranslator::~PNGToBitmapTranslator(void)
//====================================================================
{
	if (m_src_png != NULL)
	{
		png_destroy_read_struct(&m_src_png, (m_src_info != NULL) ? &m_src_info : NULL, NULL);
	}
	
	delete [] m_row_buf;
}

//====================================================================
status_t PNGToBitmapTranslator::Status(status_t * const a_err) const
//====================================================================
{
	status_t err(m_status);
	
	if (err == B_NO_ERROR)
	{
		err = Inherited::Status();
	}
	
	if (a_err != NULL)
	{
		*a_err = err;
	}
	
	return err;
}

//====================================================================
status_t PNGToBitmapTranslator::PerformTranslation(void)
//====================================================================
{
	status_t err;
	
	if (Status(&err) != B_NO_ERROR)
	{
		return err;
	}
	
	if ((err = setjmp(m_src_png->jmpbuf)) != B_NO_ERROR)
	{
		return err;
	}
	
	// We'll set up a row callback function so that we can translage
	// from one source to another without needing the entire image
	png_set_progressive_read_fn(m_src_png, this, static_cast<png_progressive_info_ptr>(PNGCallbackReadInfoBeginning), static_cast<png_progressive_row_ptr>(PNGCallbackReadRow), static_cast<png_progressive_end_ptr>(PNGCallbackReadInfoEnd));
	
	//	BeOS now uses opacity alpha like PNG
	//png_set_invert_alpha(m_src_png);
	
	// Read from the source with our callbacks
	if (SeekSource(0) != 0)
	{
		return B_IO_ERROR;
	}
	
	ssize_t bytes_read;
	uint8 *buf;
	
	if ((buf = new uint8[mk_buf_size]) == NULL)
	{
		return B_NO_MEMORY;
	}
	
	while ((bytes_read = ReadSource(buf, sizeof (uint8) * mk_buf_size)) > 0)
	{
		png_process_data(m_src_png, m_src_info, buf, bytes_read);
	}
	
	delete [] buf;
	
	return (bytes_read < 0) ? B_NO_TRANSLATOR : B_NO_ERROR;
}

//====================================================================
void PNGToBitmapTranslator::PNGCallbackReadInfoBeginning(png_structp const a_png, png_infop const a_info)
//====================================================================
{
	status_t err;
	PNGToBitmapTranslator *trnsltr(static_cast<PNGToBitmapTranslator *>(png_get_progressive_ptr(a_png)));
	png_byte color_type(png_get_color_type(a_png, a_info));
	png_byte bit_depth(png_get_bit_depth(a_png, a_info));

	// massage the PNG's colorspace into something we can use

	// unpack and expand anything whose pixels
	// fit in one byte
	if (bit_depth <= 8) {
		png_set_expand(a_png);
		png_set_packing(a_png);
	}

	// expand any transparent images to add an alpha channel
	if (png_get_valid(a_png, a_info, PNG_INFO_tRNS)) {
		png_set_expand(a_png);
	}

	// we can only handle up to 8 bits per channel
	if (bit_depth > 8) {
		png_set_strip_16(a_png);
	}
	
	// use little-endian color order (BGR v RGB)
	png_set_bgr(a_png);

	// B_GRAY8 doesn't work
	// promote multibit grayscale to RGB(A)32
	if (!(color_type & PNG_COLOR_MASK_COLOR)) {
		png_set_gray_to_rgb(a_png);
	}

	// fill out the 32 bits for images without an alpha channel
	// so we can use B_RGB32
	if (!(color_type & PNG_COLOR_MASK_ALPHA)) {
		png_set_filler(a_png, 0xFF, PNG_FILLER_AFTER);
	}
	
	// We want the library to handle interlacing, don't we?
	png_set_interlace_handling(a_png);
	
	// Now we need to update our info struct to reflect the
	// transformation changes we've asked it to make
	png_read_update_info(a_png, a_info);
	
	// We need to allocate space to keep track of the old row and
	// write out the bitmap header
	ssize_t row_bytes(png_get_rowbytes(a_png, a_info));
	delete [] trnsltr->m_row_buf;
	
	if ((trnsltr->m_row_buf = new png_byte[row_bytes]) == NULL)
	{
		// We're imitating the library's own error handler here
		longjmp(a_png->jmpbuf, B_NO_MEMORY);
	}
	
	if ((err = trnsltr->WriteHeader()) != B_NO_ERROR)
	{
		longjmp(a_png->jmpbuf, err);
	}
	
	// Initialize the destination space
	// This lets people use BPIO's like BMallocIO, where you
	//     can't seek past EOF (which libpng seems to like doing)
	memset(trnsltr->m_row_buf, (png_get_bit_depth(a_png, a_info) > 1) ? 0xFF : 0x00, row_bytes);

// There's no reason to initialize the output stream here, and it interferes with
// streaming images for Opera, so it's commented out
	
//	if (trnsltr->SeekDestination(sizeof (TranslatorBitmap)) != sizeof (TranslatorBitmap))
//	{
//		longjmp(a_png->jmpbuf, B_IO_ERROR);
//	}
//	
////
//	for (size_t i(0);
//		i < png_get_image_height(a_png, trnsltr->m_src_info);
//		i++)
//	{
//		if (trnsltr->WriteDestination(trnsltr->m_row_buf, row_bytes) < row_bytes)
//		{
//			longjmp(a_png->jmpbuf, B_IO_ERROR);
//		}
//	}
//	
//	if (trnsltr->SeekDestination(sizeof (TranslatorBitmap)) != sizeof (TranslatorBitmap))
//	{
//		longjmp(a_png->jmpbuf, B_IO_ERROR);
//	}
////
}

//====================================================================
void PNGToBitmapTranslator::PNGCallbackReadRow(png_structp const a_png, png_bytep const a_row, const png_uint_32 a_row_num, const int /*a_pass*/)
//====================================================================
{
	PNGToBitmapTranslator *trnsltr(static_cast<PNGToBitmapTranslator *>(png_get_progressive_ptr(a_png)));
	ssize_t row_bytes(png_get_rowbytes(a_png, trnsltr->m_src_info));
	
	if (a_row != NULL)
	{
		// Get the old row
		if (trnsltr->SeekDestination(sizeof (TranslatorBitmap) + a_row_num * row_bytes) != sizeof (TranslatorBitmap) + a_row_num * row_bytes
			|| trnsltr->ReadDestination(trnsltr->m_row_buf, row_bytes) < row_bytes)
		{
			// We're imitating the library's own error handler here
			longjmp(a_png->jmpbuf, B_IO_ERROR);
		}
		
		// Combine the two (this has no effect if the image is not
		// interlaced)
		png_progressive_combine_row(a_png, trnsltr->m_row_buf, a_row);
		
		// Write the result
		if (trnsltr->SeekDestination(sizeof (TranslatorBitmap) + a_row_num * row_bytes) != sizeof (TranslatorBitmap) + a_row_num * row_bytes
			|| trnsltr->WriteDestination(trnsltr->m_row_buf, row_bytes) < row_bytes)
		{
			longjmp(a_png->jmpbuf, B_IO_ERROR);
		}
	}
}

//====================================================================
void PNGToBitmapTranslator::Init(void)
//====================================================================
{
	m_status = B_NO_ERROR;
	m_src_png = NULL;
	m_src_info = NULL;
	m_row_buf = NULL;
	
	png_byte buf[mk_png_check_bytes];
	
	// Try to read the header
	if (SeekSource(0) != 0
		|| ReadSource(buf, mk_png_check_bytes) < mk_png_check_bytes)
	{
		m_status = B_IO_ERROR;
	}
	// Check the header
	else if (png_sig_cmp(buf, 0, mk_png_check_bytes) != 0)
	{
		m_status = B_NO_TRANSLATOR;
	}
	// Try to allocate the PNG image data structures
	else if ((m_src_png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL
		|| (m_src_info = png_create_info_struct(m_src_png)) == NULL)
	{
		if (m_src_png != NULL)
		{
			png_destroy_read_struct(&m_src_png, (m_src_info != NULL) ? &m_src_info : NULL, NULL);
		}
		
		m_src_png = NULL;
		m_src_info = NULL;
		m_status = B_NO_MEMORY;
	}
}

//====================================================================
status_t PNGToBitmapTranslator::WriteHeader(void)
//====================================================================
{
	m_dest_bmp_hdr.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
	
	// Set up the image specific items
	// initialize offsets because image may not have an oFFs chunk
	png_int_32 offset_x(0), offset_y(0);
	int offset_units(PNG_OFFSET_PIXEL);

	png_get_oFFs(m_src_png, m_src_info, &offset_x, &offset_y, &offset_units);
	m_dest_bmp_hdr.bounds.left = static_cast<float>(offset_x);
	m_dest_bmp_hdr.bounds.top = static_cast<float>(offset_y);
	
	if (offset_units == PNG_OFFSET_MICROMETER)
	{
		png_get_pHYs(m_src_png, m_src_info, NULL, NULL, &offset_units);
		m_dest_bmp_hdr.bounds.left = (offset_units == PNG_RESOLUTION_METER) ? m_dest_bmp_hdr.bounds.left / 1000000.0 : 0.0;
		m_dest_bmp_hdr.bounds.top = (offset_units == PNG_RESOLUTION_METER) ? m_dest_bmp_hdr.bounds.top / 1000000.0 : 0.0;
	}
	
	m_dest_bmp_hdr.bounds.right = png_get_image_width(m_src_png, m_src_info) - 1.0 + m_dest_bmp_hdr.bounds.left;
	m_dest_bmp_hdr.bounds.bottom = png_get_image_height(m_src_png, m_src_info) - 1.0 + m_dest_bmp_hdr.bounds.top;
	
	// Try to fix corrupted images or those for which we don't have
	// the proper granularity to express (mainly to compensate for
	// Photoshop fuck-ups)
	if (!m_dest_bmp_hdr.bounds.IsValid()
		|| static_cast<size_t>(m_dest_bmp_hdr.bounds.Width() + 1) != png_get_image_width(m_src_png, m_src_info)
		|| static_cast<size_t>(m_dest_bmp_hdr.bounds.Height() + 1) != png_get_image_height(m_src_png, m_src_info))
	{
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
		m_dest_bmp_hdr.rowBytes =
			png_get_image_width(m_src_png, m_src_info) * 4;
	}
	else {	// GRAY, GRAY_ALPHA
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
	
	if (SeekDestination(0) != 0
		|| WriteDestination(&m_dest_bmp_hdr, bmp_size) < bmp_size)
	{
		return B_IO_ERROR;
	}
	
	// Use SetSizeDestination() so that BMallocIO works without initializing
	// the whole stream first, which was yanked for Opera streaming
	return SetSizeDestination(B_BENDIAN_TO_HOST_INT32(m_dest_bmp_hdr.dataSize) + 32);
}
