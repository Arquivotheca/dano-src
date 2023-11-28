/*--------------------------------------------------------------------*\
  File:      PNGTranslator.h
  Creator:   Matt Bogosian <mattb@be.com>
  Copyright: (c)1998, Be, Inc. All rights reserved.
  Description: Header file describing the PNG image format-specific
      Translator classes.
\*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-=- Included Files -=-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#include "Translator.h"

#include "png.h"

#include <TranslatorFormats.h>


#ifndef PNG_TRANSLATORS_H
#define PNG_TRANSLATORS_H


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-= Structs, Classes =-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*\
  Class name:       BitmapToPNGTranslator
  Inherits from:    public ConfigurableTranslator
  New data members: private static const ssize_t mk_png_check_bytes -
                        the number of bytes to pass to the PNG library
                        to check to see if a file is actually a PNG
                        file.
                    private static const uint32 mk_bmp_trnsfrm_none -
                        a transformation constant indicating the
                        desire to transform nothing.
                    private static const uint32
                        mk_bmp_trnsfrm_24_to_8c - a transformation
                        constant indicating the desire to transform a
                        24-bit RGB image into an 8-bit paletted image.
                    private static const uint32
                        mk_bmp_trnsfrm_24_to_8g - a transformation
                        constant indicating the desire to transform a
                        24-bit RGB image into an 8-bit grayscale
                        image.
                    private static const uint32
                        mk_bmp_trnsfrm_32_to_16g - a transformation
                        constant indicating the desire to transform a
                        32-bit RBG image into an 8-bit grayscale image
                        with an alpha channel.
                    private static const uint32
                        mk_bmp_trnsfrm_32_to_24 - a transformation
                        constant indicating the desire to transform a
                        32-bit RBG image into a 24-bit RGB image by
                        eliminating one of the channels (usually the
                        alpha channel).
                    private status_t m_status - the status of the
                        object.
                    private png_structp m_dest_png - the destination
                        PNG image.
                    private png_infop m_dest_info - the destination
                        PNG image's info.
                    private TranslatorBitmap m_src_bmp_hdr - the
                        source image's header.
                    private png_bytep m_row_buf - a buffer for image
                        row manipulation.
                    private uint32 m_bmp_trnsfrms - the desired
                        transformations to occur during the
                        translation.
  Description: Class to represent a translator which translates (some)
      bitmaps into PNG images when invoked with the Translate()
      function. There are many limitations with this particular
      translator which include but are not necessarily limited to the
      following:
      
      1. There is no support for several source bitmap formats (e.g.,
         15- or 16-bit bitmap formats).
      2. Aggressive compaction of color spaces when writing occurs
         only if the source is a 24- or 32-bit bitmap format.
      3. There is currently no way to view existing or create new PNG
         text items.
      4. There is nothing in place to give the user the option of
         compressing text items (i.e., they are always uncompressed
         when the image is written out).
      5. PNG images are either written with 8- or 1-bit channels.
      6. The destination PNG image format is picked entirely by the
         translator, with no influence from the user.
      7. IO extension messages are ignored.
\*--------------------------------------------------------------------*/

class BitmapToPNGTranslator : public ConfigurableTranslator
{
	public:
	
		// Public member functions
		
/*--------------------------------------------------------------------*\
  Function name: BitmapToPNGTranslator
  Member of:     public BitmapToPNGTranslator
  Defined in:    PNGTranslator.h
  Arguments:     BPositionIO * const a_src - the source of the data to
                     be translated.
                 BPositionIO * const a_dest - the destination for the
                     translated data.
                 status_t * const a_err - a placeholder for the error
                     encountered during construction (see below).
                     Default: NULL.
  Throws:        none
  Description: Class constructor. You must check the value of a_err or
      the return value of Status() after invoking the constructor to
      see if the object is usable. See the Status() function (below)
      for possible error values.
\*--------------------------------------------------------------------*/
		
		BitmapToPNGTranslator(BPositionIO * const a_src, BPositionIO * const a_dest, BMessage * a_io_ext, status_t * const a_err = NULL);
		
/*--------------------------------------------------------------------*\
  Function name: virtual ~BitmapToPNGTranslator
  Member of:     public BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     none
  Throws:        none
  Description: Class destructor.
\*--------------------------------------------------------------------*/
		
		virtual ~BitmapToPNGTranslator(void);
		
/*--------------------------------------------------------------------*\
  Function name: virtual Status const
  Member of:     public BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     status_t * const a_err - a placeholder for the error
                     code (this will be the same value that is
                     returned). Default: NULL.
  Returns:       status_t - B_NO_ERROR if the object is valid, another
                     error code if not (see below).
  Throws:        none
  Description: Function to check if the object is valid. This function
      must be called after creating the object to insure that its
      instantiation was successful. Classes who wish to override this
      function, should always call this function first to insure that
      the underlying structure is valid. Possible return values in
      addition to those defined in the parent class are as follows:
      
      B_NO_MEMORY - on construction, there was not enough memory
          available to create the object.
      B_IO_ERROR - there was a problem reading from the source data
          area.
      B_NO_TRANSLATOR - the source data is not translatable to the
          destination format.
\*--------------------------------------------------------------------*/
		
		virtual status_t Status(status_t * const a_err = NULL) const;
	
	protected:
	
		// Protected member functions
		
/*--------------------------------------------------------------------*\
  Function name: virtual PerformTranslation
  Member of:     protected BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     none
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual status_t PerformTranslation(void);
	
	private:
	
		typedef ConfigurableTranslator Inherited;
		typedef bool (*RGBTestFunc)(void * const, const png_color_8 * const);
		
		struct Palette
		{
			png_colorp m_plte;
			png_bytep m_trans;
			size_t m_size;
			size_t m_used;
			bool m_is_bw;
			bool m_is_gray;
			bool m_is_palette;
		};
		
		// Private static const data members
		static const ssize_t mk_png_check_bytes;
		static const uint32 mk_bmp_trnsfrm_none;
		static const uint32 mk_bmp_trnsfrm_24_to_8c;
		static const uint32 mk_bmp_trnsfrm_24_to_8g;
		static const uint32 mk_bmp_trnsfrm_32_to_8c;
		static const uint32 mk_bmp_trnsfrm_32_to_16g;
		static const uint32 mk_bmp_trnsfrm_32_to_24;
		
		// Private data members
		status_t m_status;
		png_structp m_dest_png;
		png_infop m_dest_info;
		TranslatorBitmap m_src_bmp_hdr;
		png_bytep m_row_buf;
		uint32 m_bmp_trnsfrms;
		
		BMessage* m_io_ext;
		color_space m_color_space;
		
		// Private static member functions
		
/*--------------------------------------------------------------------*\
  Function name: static PNGCallbackFlush
  Member of:     private BitmapToPNGTranslator
  Defined in:    PNGTranslator.h
  Arguments:     png_structp const a_png - the PNG image.
  Returns:       none
  Throws:        none
  Description: Hook function called when the library is told to use
      an alternate IO scheme.
\*--------------------------------------------------------------------*/
		
		static void PNGCallbackFlush(png_structp const a_png);
		
/*--------------------------------------------------------------------*\
  Function name: static PNGCallbackWrite
  Member of:     private BitmapToPNGTranslator
  Defined in:    PNGTranslator.h
  Arguments:     png_structp const a_png - the PNG image.
                 png_bytep const a_buf - the buffer.
                 const png_uint_32 a_buf_len - the buffer length.
  Returns:       none
  Throws:        none
  Description: Hook function called when the library is told to use
      an alternate IO scheme.
\*--------------------------------------------------------------------*/
		
		static void PNGCallbackWrite(png_structp const a_png, png_bytep const a_buf, const png_uint_32 a_buf_len);
		
/*--------------------------------------------------------------------*\
  Function name: static TransformRowRGB24toCMAP8
  Member of:     private BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     png_byte * const a_row - the image row to transform.
                 size_t * const a_row_bytes - the byte-width of the
                     row (and a placeholder for the new byte-width).
                 const png_color * const a_plte - the palette of
                     of colors.
                 const size_t a_plte_size - the number of colors in
                     the palette (with a maximum of UCHAR_MAX + 1).
                 const size_t a_red_chnl - the color byte representing
                     the red channel. Default: 0.
                 const size_t a_green_chnl - the color byte
                     representing the green channel. Default: 1.
                 const size_t a_blue_chnl - the color byte
                     representing the blue channel. Default: 2.
  Returns:       none
  Throws:        none
  Description: Function to change a 24-bit image into a paletted 8-bit
      image. Note: this function assumes that every color in the
      24-bit image is represented in the give palette. In other words,
      this function doesn't do partial-matching or dithering or any of
      that fancy crap (yet).
\*--------------------------------------------------------------------*/
		
		static void TransformRowRGB24toCMAP8(png_byte * const a_row, size_t * const a_row_bytes, const png_color * const a_plte, const size_t a_plte_size, const size_t a_red_chnl = 0, const size_t a_green_chnl = 1, const size_t a_blue_chnl = 2);
		
/*--------------------------------------------------------------------*\
  Function name: static TransformRowRGB32toCMAP8
  Member of:     private BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     png_byte * const a_row - the image row to transform.
                 size_t * const a_row_bytes - the byte-width of the
                     row (and a placeholder for the new byte-width).
                 const png_color * const a_plte - the palette of
                     of colors.
                 const png_byte * const a_trans - the transparency
                     channel
                 const size_t a_plte_size - the number of colors in
                     the palette (with a maximum of UCHAR_MAX + 1).
                 const size_t a_red_chnl - the color byte representing
                     the red channel. Default: 0.
                 const size_t a_green_chnl - the color byte
                     representing the green channel. Default: 1.
                 const size_t a_blue_chnl - the color byte
                     representing the blue channel. Default: 2.
                 const size_t a_alpha_chnl - the color byte
                     representing the alpha channel. Default: 3.
  Returns:       none
  Throws:        none
  Description: Function to change a 32-bit image into a paletted 8-bit
      image with transparency channel. Note: this function assumes
      that every color in the
      24-bit image is represented in the given palette. In other words,
      this function doesn't do partial-matching or dithering or any of
      that fancy crap (yet).
\*--------------------------------------------------------------------*/
		
		static void TransformRowRGB32toCMAP8(png_byte * const a_row, size_t * const a_row_bytes, const png_color * const a_plte, const png_byte * const a_trans, const size_t a_plte_size, const size_t a_red_chnl = 0, const size_t a_green_chnl = 1, const size_t a_blue_chnl = 2, const size_t a_alpha_chnl = 3);
		
/*--------------------------------------------------------------------*\
  Function name: static TransformRowRGB24toGRAY8
  Member of:     private BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     png_byte * const a_row - the image row to transform.
                 size_t * const a_row_bytes - the byte-width of the
                     row (and a placeholder for the new byte-width).
  Returns:       none
  Throws:        none
  Description: Function to average the colors in a 24-bit image to
      create an 8-bit grayscale image.
\*--------------------------------------------------------------------*/
		
		static void TransformRowRGB24toGRAY8(png_byte * const a_row, size_t * const a_row_bytes);
		
/*--------------------------------------------------------------------*\
  Function name: static TransformRowRGB32toGRAY16
  Member of:     private BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     png_byte * const a_row - the image row to transform.
                 size_t * const a_row_bytes - the byte-width of the
                     row (and a placeholder for the new byte-width).
                 const size_t a_alpha_chnl - the alpha channel.
                     Default: 0.
  Returns:       none
  Throws:        none
  Description: Function to average the colors in a 32-bit image to
      create an 8-bit grayscale image with the alpha channel of the
      original 32-bit image.
\*--------------------------------------------------------------------*/
		
		void TransformRowRGB32toGRAY16(png_byte * const a_row, size_t * const a_row_bytes, const size_t a_alpha_chnl = 0);
		
/*--------------------------------------------------------------------*\
  Function name: static TransformRowRGB32toRGB24
  Member of:     private BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     png_byte * const a_row - the image row to transform.
                 size_t * const a_row_bytes - the byte-width of the
                     row (and a placeholder for the new byte-width).
                 const size_t a_dead_chnl - the channel to extract.
                     Default: 0.
  Returns:       none
  Throws:        none
  Description: Function to extract a channel from a 32-bit image,
      thereby making it a 24-bit image.
\*--------------------------------------------------------------------*/
		
		static void TransformRowRGB32toRGB24(png_byte * const a_row, size_t * const a_row_bytes, const size_t a_dead_chnl = 0);
		
		// Private member functions
		
/*--------------------------------------------------------------------*\
  Function name: CheckRGBAttribute
  Member of:     private BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     RGBTestFunc a_func - the test function to call
                     for each pixel.
                 void * const a_func_arg - the optional data given
                     back to the test function. Default: NULL
  Returns:       bool - true if the image passed the given test for
                     every pixel, false otherwise.
  Throws:        none
  Description: Function to call a test function on each pixel of the
      source image.
\*--------------------------------------------------------------------*/
		
		bool CheckRGBAttributes(Palette* a_palette);
		
/*--------------------------------------------------------------------*\
  Function name: Init
  Member of:     private BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Function to initialize a new object.
\*--------------------------------------------------------------------*/
		
		void Init(BMessage * a_io_ext);
		
/*--------------------------------------------------------------------*\
  Function name: IsAlphaUnused
  Member of:     private BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     none
  Returns:       bool - true if the source image does not have an, or
                     use its alpha channel, false otherwise.
  Throws:        none
  Description: Function to test whether or not the source image
      contains any alpha information.
\*--------------------------------------------------------------------*/
		
		bool IsAlphaUnused(void);
		
/*--------------------------------------------------------------------*\
  Function name: PerformTransformations
  Member of:     private BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     none
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Function to perform the desired transformations (as
      indicated by m_trnsfrms) to the current row (in m_row_buf).
\*--------------------------------------------------------------------*/
		
		status_t PerformTransformations(void);
		
/*--------------------------------------------------------------------*\
  Function name: ReadSourceBitmapHeader
  Member of:     private BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     none
  Returns:       status_t - B_NO_ERROR if the header was read and was
                     valid, another error code otherwise.
  Throws:        none
  Description: Function to read the bitmap header from the source data
      area and check if that bitmap header is valid.
\*--------------------------------------------------------------------*/
		
		status_t ReadSourceBitmapHeader(void);
		
/*--------------------------------------------------------------------*\
  Function name: SetPhysicalProperties
  Member of:     private BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Function to set up the physical properties according to
      the user settings before the header is written.
\*--------------------------------------------------------------------*/
		
		void SetPhysicalProperties(void);
		
/*--------------------------------------------------------------------*\
  Function name: WriteHeader
  Member of:     private BitmapToPNGTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     none
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Function to write out the PNG image information to the
      destination data area.
\*--------------------------------------------------------------------*/
		
		status_t WriteHeader(void);
		
		// Private prohibitted member functions
		BitmapToPNGTranslator(const BitmapToPNGTranslator &a_obj);
		BitmapToPNGTranslator &operator=(const BitmapToPNGTranslator &a_obj);
};

/*--------------------------------------------------------------------*\
  Class name:       PNGToBitmapTranslator
  Inherits from:    public ConfigurableTranslator
  New data members: private static const ssize_t mk_png_check_bytes -
                        the number of bytes to pass to the PNG library
                        to check to see if a file is actually a PNG
                        file.
                    private status_t m_status - the status of the
                        object.
                    private png_structp m_src_png - the source PNG
                        image.
                    private png_infop m_src_info - the source PNG
                        image's info.
                    private TranslatorBitmap m_dest_bmp_hdr - the
                        destination image's header.
                    private png_bytep m_row_buf - a buffer for image
                        row manipulation.
  Description: Class to represent a translator which translates PNG
      images into bitmaps when invoked with the Translate() function.
      There are many limitations with this particular translator which
      include but are not necessarily limited to the following:
      
      1. Translations are made to only a select fiew destination
         bitmap formats.
      2. The destination bitmap format is picked entirely by the
         translator, with no influence from the user.
      3. IO extension messages are ignored.
\*--------------------------------------------------------------------*/

class PNGToBitmapTranslator : public ConfigurableTranslator
{
	public:
	
		// Public member functions
		
/*--------------------------------------------------------------------*\
  Function name: PNGToBitmapTranslator
  Member of:     public PNGToBitmapTranslator
  Defined in:    PNGTranslator.h
  Arguments:     BPositionIO * const a_src - the source of the data to
                     be translated.
                 BPositionIO * const a_dest - the destination for the
                     translated data.
                 status_t * const a_err - a placeholder for the error
                     encountered during construction (see below).
                     Default: NULL.
  Throws:        none
  Description: Class constructor. You must check the value of a_err or
      the return value of Status() after invoking the constructor to
      see if the object is usable. See the Status() function (below)
      for possible error values.
\*--------------------------------------------------------------------*/
		
		PNGToBitmapTranslator(BPositionIO * const a_src, BPositionIO * const a_dest, status_t * const a_err = NULL);
		
/*--------------------------------------------------------------------*\
  Function name: virtual ~PNGToBitmapTranslator
  Member of:     public PNGToBitmapTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     none
  Throws:        none
  Description: Class destructor.
\*--------------------------------------------------------------------*/
		
		virtual ~PNGToBitmapTranslator(void);
		
/*--------------------------------------------------------------------*\
  Function name: virtual Status const
  Member of:     public PNGToBitmapTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     status_t * const a_err - a placeholder for the error
                     code (this will be the same value that is
                     returned). Default: NULL.
  Returns:       status_t - B_NO_ERROR if the object is valid, another
                     error code if not (see below).
  Throws:        none
  Description: Function to check if the object is valid. This function
      must be called after creating the object to insure that its
      instantiation was successful. Classes who wish to override this
      function, should always call this function first to insure that
      the underlying structure is valid. Possible return values in
      addition to those defined in the parent class are as follows:
      
      B_NO_MEMORY - on construction, there was not enough memory
          available to create the object.
      B_IO_ERROR - there was a problem reading from the source data
          area.
      B_NO_TRANSLATOR - the source data is not translatable to the
          destination format.
\*--------------------------------------------------------------------*/
		
		virtual status_t Status(status_t * const a_err = NULL) const;
	
	protected:
	
		// Protected member functions
		
/*--------------------------------------------------------------------*\
  Function name: virtual PerformTranslation
  Member of:     protected PNGToBitmapTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     none
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual status_t PerformTranslation(void);
	
	private:
	
		typedef ConfigurableTranslator Inherited;
		
		// Private static const data members
		static const ssize_t mk_png_check_bytes;
		
		// Private data members
		status_t m_status;
		png_structp m_src_png;
		png_infop m_src_info;
		TranslatorBitmap m_dest_bmp_hdr;
		png_bytep m_row_buf;

		// Private static member functions
		
/*--------------------------------------------------------------------*\
  Function name: static PNGCallbackReadInfoBeginning
  Member of:     private PNGToBitmapTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     png_structp const a_png - the PNG image.
                 png_infop const a_info - the PNG image's info.
  Returns:       none
  Throws:        none
  Description: Hook function called when the beginning info is read
      from a PNG file. We want to keep the pixel information
      consistent with the internal storage format of the BeOS, so
      we've got to tell the library about a couple of changes. We do
      that (mostly) in this function.
\*--------------------------------------------------------------------*/
		
		static void PNGCallbackReadInfoBeginning(png_structp const a_png, png_infop const a_info);
		
/*--------------------------------------------------------------------*\
  Function name: static PNGCallbackReadInfoEnd
  Member of:     private PNGToBitmapTranslator
  Defined in:    PNGTranslator.h
  Arguments:     png_structp const a_png - the PNG image.
                 png_infop const a_info - the PNG image's info.
  Returns:       none
  Throws:        none
  Description: Hook function called when the end info is read. This
      function doesn't do much.
\*--------------------------------------------------------------------*/
		
		static void PNGCallbackReadInfoEnd(png_structp const a_png, png_infop const a_info);
		
/*--------------------------------------------------------------------*\
  Function name: static PNGCallbackReadRow
  Member of:     private PNGToBitmapTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     png_structp const a_png - the PNG image.
                 png_bytep const a_row - the raw row data.
                 const png_uint_32 a_row_num - the row number.
                 const int a_pass - the pass number.
  Returns:       none
  Throws:        none
  Description: Hook function to do the real meat of the PNG to bitmap
      translation. Actually all this does is copy one row of bytes to
      another, but it combines the old and the new if the image was
      interlaced. Doing it this way means that we only need to
      allocate enough memory (in the translator) to hold one row of
      the image as opposed to the entire image.
\*--------------------------------------------------------------------*/
		
		static void PNGCallbackReadRow(png_structp const a_png, png_bytep const a_row, const png_uint_32 a_row_num, const int a_pass);
		
		// Private member functions
		
/*--------------------------------------------------------------------*\
  Function name: Init
  Member of:     private PNGToBitmapTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Function to initialize a new object.
\*--------------------------------------------------------------------*/
		
		void Init(void);
		
/*--------------------------------------------------------------------*\
  Function name: WriteHeader
  Member of:     private PNGToBitmapTranslator
  Defined in:    PNGTranslator.cpp
  Arguments:     none
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Function to write out the bitmap header to the
      destination data area.
\*--------------------------------------------------------------------*/
		
		status_t WriteHeader(void);
		
		// Private prohibitted member functions
		PNGToBitmapTranslator(const PNGToBitmapTranslator &a_obj);
		PNGToBitmapTranslator &operator=(const PNGToBitmapTranslator &a_obj);
};


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-= Function Definitions =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

//====================================================================
inline BitmapToPNGTranslator::BitmapToPNGTranslator(BPositionIO * const a_src, BPositionIO * const a_dest, BMessage * a_io_ext, status_t * const a_err) :
//====================================================================
	Inherited(a_src, a_dest, a_err)
{
	Init(a_io_ext);
}

//====================================================================
inline void BitmapToPNGTranslator::PNGCallbackFlush(png_structp const /*a_png*/)
//====================================================================
{
	// To my knowledge, there is no concept of "flushing" associated
	// with BPositionIOs
	;
}

//====================================================================
inline void BitmapToPNGTranslator::PNGCallbackWrite(png_structp const a_png, png_bytep const a_buf, const png_uint_32 a_buf_len)
//====================================================================
{
	BitmapToPNGTranslator *trnsltr(static_cast<BitmapToPNGTranslator *>(png_get_io_ptr(a_png)));
	ssize_t buf_len(a_buf_len);
	
	if (trnsltr->WriteDestination(a_buf, buf_len) != buf_len)
	{
		// We're imitating the library's own error handler here
		longjmp(a_png->jmpbuf, B_IO_ERROR);
	}
}

//====================================================================
inline PNGToBitmapTranslator::PNGToBitmapTranslator(BPositionIO * const a_src, BPositionIO * const a_dest, status_t * const a_err) :
//====================================================================
	Inherited(a_src, a_dest, a_err)
{
	Init();
}

//====================================================================
inline void PNGToBitmapTranslator::PNGCallbackReadInfoEnd(png_structp const /*a_png*/, png_infop const /*a_info*/)
//====================================================================
{
	;
}


#endif    // PNG_TRANSLATORS_H
