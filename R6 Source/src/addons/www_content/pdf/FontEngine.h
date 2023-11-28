#ifndef _FONT_ENGINE_H_
#define _FONT_ENGINE_H_


#include <String.h>
#include <Point.h>
#include <Rect.h>

#include "t2k.h"
#include "Object2.h"
#include "Transform2D.h"

class BPositionIO;
class FontIO;
class BFile;
class PSNameMap;

#include <map>
#include <vector>

const float	invalidWidth = -256.256;

struct glyph_info {
	uint16	ffcode;
	float	width;
	bool	isSpace;
			glyph_info() : ffcode(0xffff), width(invalidWidth), isSpace(false) {}
};

struct glyph_data{
	uint8 *		bits;		//must be freed
	int32		rowbytes;
	BPoint		lefttop;
	float		width;
	float		height;
				glyph_data();
				~glyph_data();
	void		reset();
};


enum {
	IS_FIXED = 			(1 << 0),
	IS_SERIF =			(1 << 1),
	IS_SYMBOLIC =		(1 << 2),
	IS_SCRIPT =			(1 << 3),
	IS_NONSYMBOLIC =	(1 << 5),
	IS_ITALIC =			(1 << 6),
	IS_ALL_CAP =		(1 << 16),
	IS_SMALL_CAP =		(1 << 17),
	IS_FORCE_BOLD = 	(1 << 18)
};

class FontEngine : public PDFOpaque {
	public:
		class Strike {
			public:
			status_t			RenderGlyph(uint16 code, glyph_data &glyph);
			void				ReleaseGlyph();
			private:
								Strike(FontEngine &engine, T2K_TRANS_MATRIX &matrix);
								Strike(FontEngine &engine, T2K_TRANS_MATRIX &matrix, bool unused);
								~Strike();
			static bool			strike_less(Strike const *a, Strike const *b);
			FontEngine			&fEngine;
			T2K_TRANS_MATRIX 	fMatrix;
			friend class		FontEngine;
		};
		static FontEngine *		SpecifyEngine(PDFObject *fontobj, status_t &status);
		Strike *				StrikeForFont(Transform2D &matrix, status_t &error);
								~FontEngine();
	
		const char *			FontName() const;
		const char *			Subtype() const;
		BRect					BoundingBox() const;
		void					GetGlyphInfo(uint16 pdfCode, glyph_info &info);
		
		bool					IsFixedFont() const { return fFlags & IS_FIXED;};
		bool					IsSerif() const { return fFlags & IS_SERIF;};
		bool					IsSymbolic() const { return fFlags & IS_SYMBOLIC;};
		bool					IsScript() const { return fFlags & IS_SCRIPT;};
		bool					IsNonSymbolic() const { return fFlags & IS_NONSYMBOLIC;};
		bool					IsItalic() const { return fFlags & IS_ITALIC;};
		bool					IsAllCap() const { return fFlags & IS_ALL_CAP;};
		bool					IsSmallCap() const { return fFlags & IS_SMALL_CAP;}; 
		bool					IsForceBold() const { return fFlags & IS_FORCE_BOLD;};
		
		
		
	private:


		class	Encoding {
			public:
				typedef map<uint16, const char *> difference_map;
									Encoding();
									~Encoding();
				void				SetTo(int8 baseEncoding, PDFObject * differences = NULL);
				const char *		NameForCode(uint16 code);
				int8				fBaseEncoding;
			private:
				difference_map		fDifferences;
		};
		
		

								FontEngine(PDFObject *fontobj);
		status_t				LoadFont();
		status_t				LoadEmbeddedFont();
		int8					FindStandardFont(const char *name);
		void					LoadStandardDescriptor();
		status_t				LoadStandardFont();
		status_t				LoadEncoding();
		
		status_t				InitFontFusion();
		void					ResetFontFusion();

		uint16					FFCodeForName(const char *name);
		
		// data
		PDFObject *				fFontObject;
		const char *			fSubtype;
		const char *			fFontName;
		int8					fStandardFont;
		PDFObject *				fDescriptor;
		uint32					fFlags;
		BRect					fBoundingBox;
		float					fDefaultWidth;
		uint16					fFirstChar;
		uint16					fLastChar;
		float *					fWidths;		
		PDFObject *				fEmbeddedFont;
		off_t					fFontDataSize;		
		uchar *					fFontData;
		bool					fUsingT1Font;
		uchar *					fT1FontData;
		Encoding				fEncoding;
		typedef map<uint16, glyph_info> ginfo_map;
		ginfo_map				fGlyphInfo;
		
		tsiMemObject *			fMem;
		InputStream *			fInput;
		sfntClass *				fFont;
		T2K *					fScaler;
		typedef vector<Strike *> strike_vector;
		strike_vector			fStrikes;
		Strike *				fCurrentStrike;
		friend class			Strike;
};

#endif
