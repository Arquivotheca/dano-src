#ifndef _FONT_ENGINE_H_
#define _FONT_ENGINE_H_


#include <String.h>
#include <Point.h>

#include "FontUtils.h"
#include "freetype.h"
#include "Object2.h"


class BPositionIO;
class FontIO;
class BFile;
class PDFFontEncoding;


class FontEngine : public PDFOpaque {
	public:
		static FontEngine *		SpecifyEngine(PDFObject *fontobj);
								~FontEngine();
	
		const char *			FontName() const;
		status_t				RenderGlyph(uint16 code, glyph_data &glyph);
		status_t				UpdateMatrix(const font_matrix &matrix);
		
		bool					IsFixedFont() const { return fFlags & 1;};
		bool					IsSerif() const { return fFlags & (1 << 2);};
		bool					IsSymbolic() const { return fFlags & (1 << 3);};
		bool					IsScript() const { return fFlags & (1 << 4);};
		bool					IsNonSymbolic() const { return fFlags & (1 << 6);};
		bool					IsItalic() const { return fFlags & (1 << 7);};
		bool					IsAllCap() const { return fFlags & (1 << 17);};
		bool					IsSmallCap() const { return fFlags & (1 << 18);}; 
		bool					IsForceBold() const { return fFlags & (1 << 19);};
		
		
	private:
								FontEngine(PDFObject *fontobj);
		status_t				LoadFont();
		status_t				LoadEmbeddedFont();
		char *					MapFontByName();
		char *					MapFontByMetrics();
		status_t				LoadFontFromDisk();
		status_t				LoadEncoding();
		
		status_t				InitFreeType();
		void					ResetFreeType();
		static void				ReadFont(void *io, uint8 *dest, unsigned long offset, long numBytes);		
		
		
		
		
		uint32					fFlags;
		uint32					fState;
								
		PDFObject *				fFontObject;
		PDFObject *				fDescriptor;
		const char *			fFontName;
		bool					fEmbeddedFont;	
		PDFFontEncoding *		fEncoding;
		uint16					fFirstChar;
		uint16					fLastChar;
		
		BMallocIO *				fFontData;
		off_t					fFontDataSize;		
		
#if 0
		tsiMemObject *			fMem;
		InputStream *			fInput;
		sfntClass *				fFont;
		T2K *					fScaler;
#endif
		FT_Face					fFace;	// font face
		uint32					fResX;	// dpi in x dimension on output device
		uint32					fResY;	// dpi in y dimension
};

#endif
