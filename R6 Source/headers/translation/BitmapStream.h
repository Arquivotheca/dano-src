/********************************************************************************
/
/      File:           BitmapStream.h
/
/      Description:    BPositionIO subclass to read/write bitmap format to/from 
/                      BBitmap instance.
/
/      Copyright 1998, Be Incorporated, All Rights Reserved.
/      Copyright 1995-1997, Jon Watte
/
********************************************************************************/

#if !defined(_BITMAP_STREAM_H)
#define _BITMAP_STREAM_H

#include <BeBuild.h>
#include <TranslationDefs.h>
#include <DataIO.h>
#include <TranslatorFormats.h>


class BBitmap;


class BBitmapStream :
	public BPositionIO
{
public:

		//	This constructor serves two purposes.
		//
		//	If you don't pass a bitmap, "preferred" means the preferred color_space to
		//	use for BBitmaps created by this bitmap stream (if the built-in conversion
		//	can handle the I/O format specified in WriteAt() and your preferred format).
		//	If you DO pass a bitmap, the color space of the bitmap is assumed to be the
		//	preferred internal format and the "preferred" argument means the external
		//	format to convert to in ReadAt()/WriteAt().
		//
		//	As of 4.6, the only conversion supported is for B_RGB16 internal data format
		//	and RGB(A) 24/32 BIG/LITTLE external (WriteAt()/ReadAt()) formats.
		//	If there is no supported conversion, this constructor and the object so
		//	constructed will behave just like the bitmap-only constructor.
		//
		//	To import a RGB16 bitmap from a translator that only emits RGB32:
		//
		//	BBitmapStream strm(B_RGB16);
		//	roster->Translate(&file, 0, 0, &strm, B_TRANSLATOR_BITMAP);
		//	BBitmap * bm16bit;
		//	strm.DetachBitmap(&bm16bit);
		//	ASSERT(bm16bit->ColorSpace() == B_RGB16);
		//
		//	To export a RGB16 bitmap using a translator which only accepts RGB32:
		//
		//	ASSERT(bm16bit->ColorSpace() == B_RGB16);
		//	BBitmapStream strm(B_RGB32, bm16bit);
		//	roster->Translate(&strm, 0, 0, &file, 'JPEG');
		//	strm.DetachBitmap(&bm16bit);	//	else it will be deleted when the stream is
		//
		BBitmapStream(
				color_space		preferred_space,
				BBitmap *		map = NULL);

		BBitmapStream(
				BBitmap *		map = NULL);
		~BBitmapStream();

	/* Overrides from BPositionIO */

		ssize_t ReadAt(
				off_t			pos,
				void *			buffer,
				size_t			size);
		ssize_t WriteAt(
				off_t			pos,
				const void *	data,
				size_t			size);
		off_t Seek(
				off_t			position,
				uint32			whence);
		off_t Position() const;
		off_t Size() const;
		status_t SetSize(
				off_t			size);

		status_t DetachBitmap(
				BBitmap * *		outMap);	/*	not NULL	*/

		color_space PreferredSpace() const;
		color_space IOSpace() const;
		color_space BitmapSpace() const;

protected:
		TranslatorBitmap fHeader;
		BBitmap * fMap;
		size_t fPosition;		//	logical (external) units
		size_t fSize;			//	logical (external) units
		bool fDetached;
		bool fIsXlate;			//	is the mapping logical (external) <-> physical (memory) not 1:1?
		char fExternalSize;
		char fMemSize;
		color_space fPrefSpace;	//	preferred internal color space
		void (BBitmapStream::*xlate_in)(const void * src, size_t src_size, void * dst);
		void (BBitmapStream::*xlate_out)(const void * src, void * dst, size_t dst_size);

		void SwapHeader(
				const TranslatorBitmap *	source,
				TranslatorBitmap *			destination);
private:

virtual	void _ReservedBitmapStream1();
virtual void _ReservedBitmapStream2();

		TranslatorBitmap * fSwappedHeader; /* always in big-endian format */
		long _reservedBitmapStream[2];

		void Init(
				BBitmap *		bitmap,
				color_space		preferred);
		bool init_xlation(
				color_space		in_memory,
				color_space		external);

		off_t mem_position(
				off_t			ext_pos);
		size_t ext_position(
				size_t			mem_pos);
		size_t mem_rowbytes(
				size_t			ext_rowbytes);
		size_t ext_rowbytes(
				size_t			mem_rowbytes);

		void xlate_in_15_rgb32_l(
				const void *	src,
				size_t			src_size,
				void *			dst);
		void xlate_in_15_rgb24_l(
				const void *	src,
				size_t			src_size,
				void *			dst);
		void xlate_in_15_rgb32_b(
				const void *	src,
				size_t			src_size,
				void *			dst);
		void xlate_in_15_rgb24_b(
				const void *	src,
				size_t			src_size,
				void *			dst);

		void xlate_out_15_rgb32_l(
				const void *	src,
				void *			dst,
				size_t			dst_size);
		void xlate_out_15_rgb24_l(
				const void *	src,
				void *			dst,
				size_t			dst_size);
		void xlate_out_15_rgb32_b(
				const void *	src,
				void *			dst,
				size_t			dst_size);
		void xlate_out_15_rgb24_b(
				const void *	src,
				void *			dst,
				size_t			dst_size);

		void xlate_in_16_rgb32_l(
				const void *	src,
				size_t			src_size,
				void *			dst);
		void xlate_in_16_rgb24_l(
				const void *	src,
				size_t			src_size,
				void *			dst);
		void xlate_in_16_rgb32_b(
				const void *	src,
				size_t			src_size,
				void *			dst);
		void xlate_in_16_rgb24_b(
				const void *	src,
				size_t			src_size,
				void *			dst);

		void xlate_out_16_rgb32_l(
				const void *	src,
				void *			dst,
				size_t			dst_size);
		void xlate_out_16_rgb24_l(
				const void *	src,
				void *			dst,
				size_t			dst_size);
		void xlate_out_16_rgb32_b(
				const void *	src,
				void *			dst,
				size_t			dst_size);
		void xlate_out_16_rgb24_b(
				const void *	src,
				void *			dst,
				size_t			dst_size);
};

#endif /* _BITMAP_STREAM_H */

