/*	BitmapStream.cpp
 *	$Id$
 */

#include <TranslatorFormats.h>
#include <BitmapStream.h>
#include <Bitmap.h>
#include <Debug.h>
#include <string.h>
#include <byteorder.h>


BBitmapStream::BBitmapStream(
	BBitmap *		bitmap)
{
	Init(bitmap, B_NO_COLOR_SPACE);
}

BBitmapStream::BBitmapStream(
	color_space		preferred,
	BBitmap *		bitmap)
{
	Init(bitmap, preferred);
}


bool
BBitmapStream::init_xlation(
	color_space in_memory,
	color_space external)
{
	xlate_in = 0;
	xlate_out = 0;
	fExternalSize = 1;
	fMemSize = 1;
	fIsXlate = false;

	if (in_memory == B_RGB16_LITTLE) {
		switch (external) {
		case B_RGB32_LITTLE:
			fExternalSize = 4;
			fMemSize = 2;
			fIsXlate = true;
			xlate_in = &BBitmapStream::xlate_in_16_rgb32_l;
			xlate_out = &BBitmapStream::xlate_out_16_rgb32_l;
			return true;
		case B_RGBA32_LITTLE:
			fExternalSize = 4;
			fMemSize = 2;
			fIsXlate = true;
			xlate_in = &BBitmapStream::xlate_in_16_rgb32_l;
			xlate_out = &BBitmapStream::xlate_out_16_rgb32_l;
			return true;
		case B_RGB24_LITTLE:
			fExternalSize = 3;
			fMemSize = 2;
			fIsXlate = true;
			xlate_in = &BBitmapStream::xlate_in_16_rgb24_l;
			xlate_out = &BBitmapStream::xlate_out_16_rgb24_l;
			return true;
		case B_RGB32_BIG:
			fExternalSize = 4;
			fMemSize = 2;
			fIsXlate = true;
			xlate_in = &BBitmapStream::xlate_in_16_rgb32_b;
			xlate_out = &BBitmapStream::xlate_out_16_rgb32_b;
			return true;
		case B_RGBA32_BIG:
			fExternalSize = 4;
			fMemSize = 2;
			fIsXlate = true;
			xlate_in = &BBitmapStream::xlate_in_16_rgb32_b;
			xlate_out = &BBitmapStream::xlate_out_16_rgb32_b;
			return true;
		case B_RGB24_BIG:
			fExternalSize = 3;
			fMemSize = 2;
			fIsXlate = true;
			xlate_in = &BBitmapStream::xlate_in_16_rgb24_b;
			xlate_out = &BBitmapStream::xlate_out_16_rgb24_b;
			return true;
		default:
			return false;
		}
	}
	else if ((in_memory == B_RGB15_LITTLE) || (in_memory == B_RGBA15_LITTLE)) {
		switch (external) {
		case B_RGB32_LITTLE:
			fExternalSize = 4;
			fMemSize = 2;
			fIsXlate = true;
			xlate_in = &BBitmapStream::xlate_in_15_rgb32_l;
			xlate_out = &BBitmapStream::xlate_out_15_rgb32_l;
			return true;
		case B_RGBA32_LITTLE:
			fExternalSize = 4;
			fMemSize = 2;
			fIsXlate = true;
			xlate_in = &BBitmapStream::xlate_in_15_rgb32_l;
			xlate_out = &BBitmapStream::xlate_out_15_rgb32_l;
			return true;
		case B_RGB24_LITTLE:
			fExternalSize = 3;
			fMemSize = 2;
			fIsXlate = true;
			xlate_in = &BBitmapStream::xlate_in_15_rgb24_l;
			xlate_out = &BBitmapStream::xlate_out_15_rgb24_l;
			return true;
		case B_RGB32_BIG:
			fExternalSize = 4;
			fMemSize = 2;
			fIsXlate = true;
			xlate_in = &BBitmapStream::xlate_in_15_rgb32_b;
			xlate_out = &BBitmapStream::xlate_out_15_rgb32_b;
			return true;
		case B_RGBA32_BIG:
			fExternalSize = 4;
			fMemSize = 2;
			fIsXlate = true;
			xlate_in = &BBitmapStream::xlate_in_15_rgb32_b;
			xlate_out = &BBitmapStream::xlate_out_15_rgb32_b;
			return true;
		case B_RGB24_BIG:
			fExternalSize = 3;
			fMemSize = 2;
			fIsXlate = true;
			xlate_in = &BBitmapStream::xlate_in_15_rgb24_b;
			xlate_out = &BBitmapStream::xlate_out_15_rgb24_b;
			return true;
		default:
			return false;
		}
	}
	return false;
}

void 
BBitmapStream::Init(BBitmap *bitmap, color_space preferred)
{
	fSwappedHeader = new TranslatorBitmap;
	fMap = bitmap;
	fDetached = false;
	fPosition = 0;
	fSize = 0;
	fPrefSpace = preferred;
	xlate_in = 0;
	xlate_out = 0;
	fIsXlate = false;
	fExternalSize = 1;
	fMemSize = 1;
	/*	Extract header if needed */
	if (fMap) {
		fPrefSpace = fMap->ColorSpace();
		fHeader.magic = B_TRANSLATOR_BITMAP;
		fHeader.bounds = fMap->Bounds();
		fHeader.rowBytes = fMap->BytesPerRow();
		fHeader.colors = fMap->ColorSpace();
		fHeader.dataSize = ((uint32)fHeader.bounds.Height()+1)*fHeader.rowBytes;
		fSize = sizeof(TranslatorBitmap)+fHeader.dataSize;
		//	IMPORTANT: we use the reverse order of arguments here!
		init_xlation(fHeader.colors, preferred);
		if (fIsXlate) {
			fHeader.colors = preferred;
			fHeader.rowBytes = ext_rowbytes(fHeader.rowBytes);
			fHeader.dataSize = ext_position(fHeader.dataSize);
			fSize = sizeof(TranslatorBitmap)+fHeader.dataSize;
		}
	}
}


BBitmapStream::~BBitmapStream()
{
	delete fSwappedHeader;
	if (fMap && !fDetached)
		delete fMap;
}


status_t
BBitmapStream::ReadAt(
	off_t			pos,
	void *			buffer,
	size_t			size)
{
	if (fDetached)
		return B_ERROR;
	if (!fMap)
		return B_ERROR;
	if (!size)
		return B_NO_ERROR;
	if (pos >= fSize)
		return B_ERROR;

	long toRead;
	void *source;

	if (fPosition < sizeof(TranslatorBitmap)) {
		toRead = sizeof(TranslatorBitmap)-pos;
		source = ((char *)fSwappedHeader)+pos;
		SwapHeader(&fHeader, fSwappedHeader);
	} else {
		toRead = fSize-pos;
		source = ((char *)fMap->Bits())+mem_position(fPosition-sizeof(TranslatorBitmap));
	}
	if (toRead > size)
		toRead = size;
	if ((fPosition >= sizeof(TranslatorBitmap)) && fIsXlate) {
		(this->*xlate_out)(source, buffer, toRead);
	}
	else {
		memcpy(buffer, source, toRead);
	}
	return toRead;
}


status_t
BBitmapStream::WriteAt(
	off_t			pos,
	const void *	data,
	size_t			size)
{
	if (fDetached)	
		return B_ERROR;
	if (!size)
		return B_NO_ERROR;
	ssize_t written = 0;
	while (size > 0) {
		long toWrite;
		void *dest;
		/*	We depend on writing the header separately in detecting changes to it */
		if (pos < sizeof(TranslatorBitmap)) {
			toWrite = sizeof(TranslatorBitmap)-pos;
			dest = ((char *)fSwappedHeader)+pos;
			SwapHeader(&fHeader, fSwappedHeader);
		} else {
			toWrite = fHeader.dataSize-pos+sizeof(TranslatorBitmap);
			dest = ((char *)fMap->Bits())+mem_position(pos-sizeof(TranslatorBitmap));
		}
		if (toWrite > size)
			toWrite = size;
		if (!toWrite && size)	//	i e we've been told to write too much
			return B_BAD_VALUE;
		if ((pos >= sizeof(TranslatorBitmap)) && fIsXlate) {
			(this->*xlate_in)(data, toWrite, dest);
		}
		else {
			memcpy(dest, data, toWrite);
		}
		if (pos < sizeof(TranslatorBitmap)) {
			SwapHeader(fSwappedHeader, &fHeader);
			init_xlation(fPrefSpace, fHeader.colors);
		}
		pos += toWrite;
		written += toWrite;
		data = ((char *)data)+toWrite;
		size -= toWrite;
		if (pos > fSize)
			fSize = pos;
		/*	If we change the header, the rest goes */
		if (pos == sizeof(TranslatorBitmap)) {
			if (fMap && ((fMap->Bounds() != fHeader.bounds) ||
					(BitmapSpace() != fHeader.colors) ||
					(fMap->BytesPerRow() != mem_rowbytes(fHeader.rowBytes)))) {
				if (!fDetached)	//	if someone detached, we don't delete
					delete fMap;
				fMap = NULL;
			}
			if (!fMap) {
				/* fHeader.bounds.PrintToStream(); /* */
				if ((fHeader.bounds.left > 0.0) || (fHeader.bounds.top > 0.0))
					DEBUGGER("non-origin bounds!");

				if (fHeader.bounds.Width()*fHeader.bounds.Height() >= 64*64)
					fMap = new BBitmap(fHeader.bounds, B_BITMAP_IS_AREA, fHeader.colors);
				else
					fMap = new BBitmap(fHeader.bounds, 0, fHeader.colors);

				if (fMap->BytesPerRow() != mem_rowbytes(fHeader.rowBytes)) {
					fprintf(stderr, "BytesPerRow() is %d, header is %d\n", fMap->BytesPerRow(),
							mem_rowbytes(fHeader.rowBytes));
					return B_MISMATCHED_VALUES;
				}
			}
			if (fMap) {
				fSize = sizeof(TranslatorBitmap)+ext_position(fMap->BitsLength());
			}
		}
	}
	return written;
}


off_t
BBitmapStream::Seek(
	off_t			position,
	uint32			whence)
{
	if (whence == SEEK_CUR)
		position += fPosition;
	if (whence == SEEK_END)
		position += fSize;
	if (position < 0)
		return B_BAD_VALUE;
	if (position > fSize)
		return B_BAD_VALUE;
	fPosition = position;
	return fPosition;
}


off_t
BBitmapStream::Position() const
{
	return fPosition;
}


off_t
BBitmapStream::Size() const
{
	return fSize;
}


status_t
BBitmapStream::SetSize(	//	returns 0 for success
	off_t			size)
{
	if (fDetached)
		return B_ERROR;
	if (size < 0)
		return B_BAD_VALUE;
	if (fMap && (size > fHeader.dataSize+sizeof(TranslatorBitmap)))
		return B_BAD_VALUE;
	/*	Problem:
	 *	What if someone calls SetSize() before writing the header, so we don't know what 
	 *	bitmap to create?
	 *	Solution:
	 *	We assume people will write the header before any data, 
	 *	so SetSize() is really not going to do anything.
	 */
	if (fMap)	//	if we checked that the size was OK
		fSize = size;
	return B_NO_ERROR;
}


status_t
BBitmapStream::DetachBitmap(
	BBitmap * *	outBitmap)	/*	not NULL	*/
{
	if (!outBitmap)
		return B_BAD_VALUE;

	(*outBitmap) = NULL;
	if (!fMap)
		return B_ERROR;
	if (fDetached)
		return B_ERROR;
	fDetached = true;
	(*outBitmap) = fMap;
	return B_NO_ERROR;
}


color_space
BBitmapStream::PreferredSpace() const
{
	return fPrefSpace;
}


color_space
BBitmapStream::IOSpace() const
{
	return fHeader.colors;
}


color_space 
BBitmapStream::BitmapSpace() const
{
	if (fIsXlate) return fPrefSpace;
	return fHeader.colors;
}


void
BBitmapStream::SwapHeader(
	const TranslatorBitmap *	source,
	TranslatorBitmap *			destination)
{
	destination->magic = B_BENDIAN_TO_HOST_INT32(source->magic);
	destination->bounds.left = B_BENDIAN_TO_HOST_FLOAT(source->bounds.left);
	destination->bounds.top = B_BENDIAN_TO_HOST_FLOAT(source->bounds.top);
	destination->bounds.right = B_BENDIAN_TO_HOST_FLOAT(source->bounds.right);
	destination->bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(source->bounds.bottom);
	destination->rowBytes = B_BENDIAN_TO_HOST_INT32(source->rowBytes);
	destination->colors = (color_space) B_BENDIAN_TO_HOST_INT32(source->colors);
	destination->dataSize = B_BENDIAN_TO_HOST_INT32(source->dataSize);
}

//	this function does NOT do magic for the header; it deals in bitmap offsets only
off_t 
BBitmapStream::mem_position(off_t ext_pos)
{
	if (!fIsXlate) return ext_pos;
	if (fHeader.rowBytes < 1) {
		return ext_pos * fMemSize / fExternalSize;
	}
	int32 scanline = ext_pos / fHeader.rowBytes;
	int32 pixel = (ext_pos - (scanline * fHeader.rowBytes)) / fExternalSize;
	return ((off_t)scanline * mem_rowbytes(fHeader.rowBytes)) + pixel * fMemSize;
}

//	this function does NOT do magic for the header; it deals in bitmap offsets only
size_t 
BBitmapStream::ext_position(size_t mem_pos)
{
	if (!fIsXlate) return mem_pos;
	if (fHeader.rowBytes < 1) {
		return mem_pos * fExternalSize / fMemSize;
	}
	size_t mrb = mem_rowbytes(fHeader.rowBytes);
	int32 scanline = mem_pos / mrb;
	int32 pixel = (mem_pos - (scanline * mrb)) / fMemSize;
	return ((off_t)scanline * fHeader.rowBytes) + pixel * fExternalSize;
}

size_t 
BBitmapStream::mem_rowbytes(size_t ext_rb)
{
	if (!fIsXlate) return ext_rb;
	return (ext_rb * fMemSize / fExternalSize + 3) & -4;
}

size_t 
BBitmapStream::ext_rowbytes(size_t mem_rb)
{
	if (!fIsXlate) return mem_rb;
	return (mem_rb * fExternalSize / fMemSize + 3) & -4;
}


//	B G R -
void 
BBitmapStream::xlate_in_15_rgb32_l(const void *src, size_t src_size, void *d)
{
	const uchar * ptr = (const uchar *)src;
	uint16 * dst = (uint16 *)d;
	bool alpha = (fHeader.colors & 0x2000) != 0;
	while (src_size > 3) {
		uint16 pixel;
		if (!alpha || ptr[3])
			pixel = (((uint16)ptr[2]&0xf8)<<7) |
				(((uint16)ptr[1]&0xf8)<<2) |
				((uint16)ptr[0]>>3) | 0x8000;
		else
			pixel = B_TRANSPARENT_MAGIC_RGBA15;
		*dst = B_HOST_TO_LENDIAN_INT16(pixel);
		ptr += 4;
		dst += 1;
		src_size -= 4;
	}
}

//	B G R
void 
BBitmapStream::xlate_in_15_rgb24_l(const void *src, size_t src_size, void *d)
{
	const uchar * ptr = (const uchar *)src;
	uint16 * dst = (uint16 *)d;
	while (src_size > 2) {
		uint16 pixel = (((uint16)ptr[2]&0xf8)<<7) |
				(((uint16)ptr[1]&0xf8)<<2) |
				((uint16)ptr[0]>>3) | 0x8000;
		*dst = B_HOST_TO_LENDIAN_INT16(pixel);
		ptr += 3;
		dst += 1;
		src_size -= 3;
	}
}

//	- R G B
void 
BBitmapStream::xlate_in_15_rgb32_b(const void *src, size_t src_size, void *d)
{
	const uchar * ptr = (const uchar *)src;
	uint16 * dst = (uint16 *)d;
	bool alpha = (fHeader.colors & 0x2000) != 0;
	while (src_size > 3) {
		uint16 pixel;
		if (!alpha || ptr[0])
			pixel = (((uint16)ptr[1]&0xf8)<<7) |
				(((uint16)ptr[2]&0xf8)<<2) |
				((uint16)ptr[3]>>3) | 0x8000;
		else
			pixel = B_TRANSPARENT_MAGIC_RGBA15;
		*dst = B_HOST_TO_LENDIAN_INT16(pixel);
		ptr += 4;
		dst += 1;
		src_size -= 4;
	}
}

//	R G B
void 
BBitmapStream::xlate_in_15_rgb24_b(const void *src, size_t src_size, void *d)
{
	const uchar * ptr = (const uchar *)src;
	uint16 * dst = (uint16 *)d;
	while (src_size > 2) {
		uint16 pixel = (((uint16)ptr[0]&0xf8)<<7) |
				(((uint16)ptr[1]&0xf8)<<2) |
				((uint16)ptr[2]>>3) | 0x8000;
		*dst = B_HOST_TO_LENDIAN_INT16(pixel);
		ptr += 3;
		dst += 1;
		src_size -= 3;
	}
}

//	B G R -
void 
BBitmapStream::xlate_out_15_rgb32_l(const void *s, void *d, size_t dst_size)
{
	const uint16 * src = (const uint16 *)s;
	uchar * dst = (uchar *)d;
	while (dst_size > 3) {
		uint16 pixel = B_LENDIAN_TO_HOST_INT16(*src);
		src += 1;
		dst[0] = (pixel<<3) | ((pixel>>2)&0x7);
		dst[1] = ((pixel>>2)&0xf8) | ((pixel>>7)&0x7);
		dst[2] = ((pixel>>7)&0xf8) | ((pixel>>12)&0x7);
		dst[3] = (pixel & 0x8000) ? 0xff : 0x00;
		dst += 4;
		dst_size -= 4;
	}
}

//	B G R
void 
BBitmapStream::xlate_out_15_rgb24_l(const void *s, void *d, size_t dst_size)
{
	const uint16 * src = (const uint16 *)s;
	uchar * dst = (uchar *)d;
	while (dst_size > 2) {
		uint16 pixel = B_LENDIAN_TO_HOST_INT16(*src);
		src += 1;
		dst[0] = (pixel<<3) | ((pixel>>2)&0x7);
		dst[1] = ((pixel>>2)&0xf8) | ((pixel>>7)&0x7);
		dst[2] = ((pixel>>7)&0xf8) | ((pixel>>12)&0x7);
		dst += 3;
		dst_size -= 3;
	}
}

//	- R G B
void 
BBitmapStream::xlate_out_15_rgb32_b(const void *s, void *d, size_t dst_size)
{
	const uint16 * src = (const uint16 *)s;
	uchar * dst = (uchar *)d;
	while (dst_size > 3) {
		uint16 pixel = B_LENDIAN_TO_HOST_INT16(*src);
		src += 1;
		dst[0] = (pixel & 0x8000) ? 0xff : 0x00;
		dst[1] = ((pixel>>7)&0xf8) | ((pixel>>12)&0x7);
		dst[2] = ((pixel>>2)&0xf8) | ((pixel>>7)&0x7);
		dst[3] = (pixel<<3) | ((pixel>>2)&0x7);
		dst += 4;
		dst_size -= 4;
	}
}

//	R G B
void 
BBitmapStream::xlate_out_15_rgb24_b(const void *s, void *d, size_t dst_size)
{
	const uint16 * src = (const uint16 *)s;
	uchar * dst = (uchar *)d;
	while (dst_size > 2) {
		uint16 pixel = B_LENDIAN_TO_HOST_INT16(*src);
		src += 1;
		dst[0] = ((pixel>>7)&0xf8) | ((pixel>>12)&0x7);
		dst[1] = ((pixel>>2)&0xf8) | ((pixel>>7)&0x7);
		dst[2] = (pixel<<3) | ((pixel>>2)&0x7);
		dst += 3;
		dst_size -= 3;
	}
}

//	B G R -
void 
BBitmapStream::xlate_in_16_rgb32_l(const void *src, size_t src_size, void *d)
{
	const uchar * ptr = (const uchar *)src;
	uint16 * dst = (uint16 *)d;
	while (src_size > 3) {
		uint16 pixel = (((uint16)ptr[2]&0xf8)<<8) |
				(((uint16)ptr[1]&0xfc)<<3) |
				((uint16)ptr[0]>>3);
		*dst = B_HOST_TO_LENDIAN_INT16(pixel);
		ptr += 4;
		dst += 1;
		src_size -= 4;
	}
}

//	B G R
void 
BBitmapStream::xlate_in_16_rgb24_l(const void *src, size_t src_size, void *d)
{
	const uchar * ptr = (const uchar *)src;
	uint16 * dst = (uint16 *)d;
	while (src_size > 2) {
		uint16 pixel = (((uint16)ptr[2]&0xf8)<<8) |
				(((uint16)ptr[1]&0xfc)<<3) |
				((uint16)ptr[0]>>3);
		*dst = B_HOST_TO_LENDIAN_INT16(pixel);
		ptr += 3;
		dst += 1;
		src_size -= 3;
	}
}

//	- R G B
void 
BBitmapStream::xlate_in_16_rgb32_b(const void *src, size_t src_size, void *d)
{
	const uchar * ptr = (const uchar *)src;
	uint16 * dst = (uint16 *)d;
	while (src_size > 3) {
		uint16 pixel = (((uint16)ptr[1]&0xf8)<<8) |
				(((uint16)ptr[2]&0xfc)<<3) |
				((uint16)ptr[3]>>3);
		*dst = B_HOST_TO_LENDIAN_INT16(pixel);
		ptr += 4;
		dst += 1;
		src_size -= 4;
	}
}

//	R G B
void 
BBitmapStream::xlate_in_16_rgb24_b(const void *src, size_t src_size, void *d)
{
	const uchar * ptr = (const uchar *)src;
	uint16 * dst = (uint16 *)d;
	while (src_size > 2) {
		uint16 pixel = (((uint16)ptr[0]&0xf8)<<8) |
				(((uint16)ptr[1]&0xfc)<<3) |
				((uint16)ptr[2]>>3);
		*dst = B_HOST_TO_LENDIAN_INT16(pixel);
		ptr += 3;
		dst += 1;
		src_size -= 3;
	}
}

//	B G R -
void 
BBitmapStream::xlate_out_16_rgb32_l(const void *s, void *d, size_t dst_size)
{
	const uint16 * src = (const uint16 *)s;
	uchar * dst = (uchar *)d;
	while (dst_size > 3) {
		uint16 pixel = B_LENDIAN_TO_HOST_INT16(*src);
		src += 1;
		dst[0] = (pixel<<3) | ((pixel>>2)&0x7);
		dst[1] = ((pixel>>3)&0xfc) | ((pixel>>9)&0x3);
		dst[2] = ((pixel>>8)&0xf8) | (pixel>>13);
		dst[3] = 0xff;
		dst += 4;
		dst_size -= 4;
	}
}

//	B G R
void 
BBitmapStream::xlate_out_16_rgb24_l(const void *s, void *d, size_t dst_size)
{
	const uint16 * src = (const uint16 *)s;
	uchar * dst = (uchar *)d;
	while (dst_size > 2) {
		uint16 pixel = B_LENDIAN_TO_HOST_INT16(*src);
		src += 1;
		dst[0] = (pixel<<3) | ((pixel>>2)&0x7);
		dst[1] = ((pixel>>3)&0xfc) | ((pixel>>9)&0x3);
		dst[2] = ((pixel>>8)&0xf8) | (pixel>>13);
		dst += 3;
		dst_size -= 3;
	}
}

//	- R G B
void 
BBitmapStream::xlate_out_16_rgb32_b(const void *s, void *d, size_t dst_size)
{
	const uint16 * src = (const uint16 *)s;
	uchar * dst = (uchar *)d;
	while (dst_size > 3) {
		uint16 pixel = B_LENDIAN_TO_HOST_INT16(*src);
		src += 1;
		dst[0] = 0xff;
		dst[1] = ((pixel>>8)&0xf8) | (pixel>>13);
		dst[2] = ((pixel>>3)&0xfc) | ((pixel>>9)&0x3);
		dst[3] = (pixel<<3) | ((pixel>>2)&0x7);
		dst += 4;
		dst_size -= 4;
	}
}

//	R G B
void 
BBitmapStream::xlate_out_16_rgb24_b(const void *s, void *d, size_t dst_size)
{
	const uint16 * src = (const uint16 *)s;
	uchar * dst = (uchar *)d;
	while (dst_size > 2) {
		uint16 pixel = B_LENDIAN_TO_HOST_INT16(*src);
		src += 1;
		dst[0] = ((pixel>>8)&0xf8) | (pixel>>13);
		dst[1] = ((pixel>>3)&0xfc) | ((pixel>>9)&0x3);
		dst[2] = (pixel<<3) | ((pixel>>2)&0x7);
		dst += 3;
		dst_size -= 3;
	}
}


void BBitmapStream::_ReservedBitmapStream1() { }
void BBitmapStream::_ReservedBitmapStream2() { }

