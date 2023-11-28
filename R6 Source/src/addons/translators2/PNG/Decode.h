//****************************************************************************************
//
//	File:		Decode.h
//
//  Written by:	Matt Bagosian
//	Revised by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef DECODE_H
#define DECODE_H

#include "Translator.h"
#include "png.h"
#include <TranslatorFormats.h>

class PNGToBitmapTranslator : public Translator {
	public:
		// Public member functions
		PNGToBitmapTranslator(BPositionIO *src, BPositionIO *dest, status_t *err = NULL);
		~PNGToBitmapTranslator();
		virtual status_t Status(status_t *err = NULL) const;
	
	protected:
		// Protected member functions
		virtual status_t PerformTranslation();
	
	private:
		// Private static const data members
		static const ssize_t mk_png_check_bytes;
		
		// Private data members
		status_t m_status;
		png_structp m_src_png;
		png_infop m_src_info;
		TranslatorBitmap m_dest_bmp_hdr;
		png_bytep m_row_buf;

		// Private static member functions
		static void PNGCallbackReadInfoBeginning(png_structp const png, png_infop const info);
		static void PNGCallbackReadInfoEnd(png_structp const png, png_infop const info);
		static void PNGCallbackReadRow(png_structp const png, png_bytep const row, const png_uint_32 row_num, const int pass);
		
		// Private member functions
		void Init();
		status_t WriteHeader();
		
		// Private prohibitted member functions
		PNGToBitmapTranslator(const PNGToBitmapTranslator &obj);
		PNGToBitmapTranslator &operator=(const PNGToBitmapTranslator &obj);
};

inline PNGToBitmapTranslator::PNGToBitmapTranslator(BPositionIO *src,
	BPositionIO *dest, status_t *err) : Translator(src, dest, err) {

	Init();
}

inline void PNGToBitmapTranslator::PNGCallbackReadInfoEnd(png_structp const /*png*/,
	png_infop const /*info*/) {
	
}

#endif
