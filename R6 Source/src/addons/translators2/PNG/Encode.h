//****************************************************************************************
//
//	File:		Encode.h
//
//  Written by:	Matt Bagosian
//	Revised by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef ENCODE_H
#define ENCODE_H

#include "Translator.h"
#include "Prefs.h"
#include "png.h"
#include <TranslatorFormats.h>

class BitmapToPNGTranslator : public Translator {
	public:
		// Public member functions
		BitmapToPNGTranslator(BPositionIO *src, BPositionIO *dest, status_t *err = NULL);
		~BitmapToPNGTranslator();
		virtual status_t Status(status_t *err = NULL) const;
	
	protected:
		// Protected member functions
		virtual status_t PerformTranslation();
	
	private:
		typedef bool (*RGBTestFunc)(void *, const png_color *);
		
		struct Palette {
			png_colorp m_plte;
			size_t m_size;
			size_t m_used;
		};
		
		// Private static const data members
		static const ssize_t mk_png_check_bytes;
		static const uint32 mk_bmp_trnsfrm_none;
		static const uint32 mk_bmp_trnsfrm_24_to_8c;
		static const uint32 mk_bmp_trnsfrm_24_to_8g;
		static const uint32 mk_bmp_trnsfrm_32_to_16g;
		static const uint32 mk_bmp_trnsfrm_32_to_24;
		
		// Private data members
		Prefs *prefs;

		status_t m_status;
		png_structp m_dest_png;
		png_infop m_dest_info;
		TranslatorBitmap m_src_bmp_hdr;
		png_bytep m_row_buf;
		uint32 m_bmp_trnsfrms;
		
		static void PNGCallbackFlush(png_structp const png);
		static void PNGCallbackWrite(png_structp const png, png_bytep const buf, const png_uint_32 buf_len);
		static bool RGBCallbackIsGray(void *data, const png_color *color);
		static bool RGBCallbackIsMono(void *data, const png_color *color);
		static bool RGBCallbackIsPalettable(void *data, const png_color *color);
		static void TransformRowRGB24toCMAP8(png_byte *row, size_t *row_bytes, const png_color *plte,
			const size_t plte_size, const size_t red_chnl = 0, const size_t green_chnl = 1, const size_t blue_chnl = 2);
		static void TransformRowRGB24toGRAY8(png_byte *row, size_t *row_bytes);
		void TransformRowRGB32toGRAY16(png_byte *row, size_t *row_bytes, const size_t alpha_chnl = 0);
		static void TransformRowRGB32toRGB24(png_byte *row, size_t *row_bytes, const size_t dead_chnl = 0);
		bool CheckRGBAttribute(RGBTestFunc func, void *func_arg = NULL);
		void Init();
		bool IsAlphaUnused();
		status_t PerformTransformations();
		status_t ReadSourceBitmapHeader();
		void SetPhysicalProperties();
		status_t WriteHeader();
		
		// Private prohibitted member functions
		BitmapToPNGTranslator(const BitmapToPNGTranslator &obj);
		BitmapToPNGTranslator &operator=(const BitmapToPNGTranslator &obj);
};

inline BitmapToPNGTranslator::BitmapToPNGTranslator(BPositionIO *src,
	BPositionIO *dest, status_t *err) : Translator(src, dest, err) {

	Init();
}


inline void BitmapToPNGTranslator::PNGCallbackFlush(png_structp const /*png*/) {
	// To my knowledge, there is no concept of "flushing" associated
	// with BPositionIOs
}

inline void BitmapToPNGTranslator::PNGCallbackWrite(png_structp const png,
	png_bytep const buf, const png_uint_32 buf_len) {

	BitmapToPNGTranslator *trnsltr(static_cast<BitmapToPNGTranslator *>(png_get_io_ptr(png)));
	ssize_t len(buf_len);
	
	if (trnsltr->m_dest->Write(buf, len) != len) {
		// We're imitating the library's own error handler here
		longjmp(png->jmpbuf, B_IO_ERROR);
	}
}

inline bool BitmapToPNGTranslator::RGBCallbackIsGray(void */*data*/,
	const png_color *color) {
	
	return (color->red == color->green && color->green == color->blue);
}


inline bool BitmapToPNGTranslator::RGBCallbackIsMono(void */*data*/,
	const png_color *color) {

	return ((color->red == 0 && color->green == 0 && color->blue == 0) ||
		(color->red == 255 && color->green == 255 && color->blue == 255));
}

#endif
