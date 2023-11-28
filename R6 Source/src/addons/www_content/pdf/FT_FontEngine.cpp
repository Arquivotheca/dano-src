
#include "FT_FontEngine.h"
#include "freetype/internal/ftcalc.h"

#include <FindDirectory.h>
#include <Path.h>
#include <Directory.h>
#include <DataIO.h>
#include <File.h>
#include <string.h>

#include "PDFFontEncoding.h"

#if SUPPORT_CMAP > 0
#include "CMapEngine.h"
#include "PushContents.h"
#include "ObjectParser.h"

#if DEBUG_ENCODING > 0
#include "TeePusher.h"
#endif
#endif

static FT_Fixed
EuclidianDistance( register FT_Fixed A, register FT_Fixed B )
{
	FT_Fixed root;
		
	if ( A < 0 ) A = -A;
	if ( B < 0 ) B = -B;
	
	if ( A == 0 ) {
		return B; /*****/
	} else if ( B == 0 ) {
		return A; /*****/
	} else {
		root	= A > B ? A + (B>>1) : B + (A>>1); /* Do an initial approximation, in root */

		/* Ok, now enter the Newton Raphson iteration sequence */
		root = (root + FT_MulFix( A, FT_DivFix( A, root) ) + FT_MulFix( B, FT_DivFix( B, root) ) + 1) >> 1; 
		root = (root + FT_MulFix( A, FT_DivFix( A, root) ) + FT_MulFix( B, FT_DivFix( B, root) ) + 1) >> 1; 
		root = (root + FT_MulFix( A, FT_DivFix( A, root) ) + FT_MulFix( B, FT_DivFix( B, root) ) + 1) >> 1; 
		/* Now the root should be correct, so get out of here! */
		return root; /*****/
	}
}

struct FreeTypeLibrary {
	FreeTypeLibrary(void);
	~FreeTypeLibrary();
	FT_Library	fLibrary;
	FT_Error	fError;
};

FreeTypeLibrary::FreeTypeLibrary(void)
{
	fError = FT_Init_FreeType(&fLibrary);
}


FreeTypeLibrary::~FreeTypeLibrary()
{
	fError = FT_Done_FreeType(fLibrary);
}

static FreeTypeLibrary gLibrary;

typedef uint16 single_encoding[256];

FontEngine *
FontEngine::SpecifyEngine(PDFObject *fontobj)
{
	FontEngine *eng = NULL;
	// look for the font engine embedded in the object
	PDFObject * fe = fontobj->Find(PDFAtom.__font_engine__);
	if (!fe) {
		// pass ownership of fontobj to engine
		eng = new FontEngine(fontobj);
		status_t status = eng->LoadFont();
		if (status != B_OK) {
			delete eng;
			eng = NULL;
		}
		else {
			fontobj->Assign(PDFObject::makeName(PDFAtom.__font_engine__), PDFObject::makeOpaque(eng));
		}
	}
	else if (fe->IsOpaque()) {
		// extract the font engine
		eng = (FontEngine *)fe->GetOpaquePtr();
		// don't need this fontobj, as we had one already
		fontobj->Release();
	}
	return eng;
}

FontEngine::FontEngine(PDFObject *fontobj) :
	PDFOpaque(),
	fFontObject(fontobj),
	fDescriptor(NULL),
	fFontName(NULL),
	fEmbeddedFont(false),
	fEncoding(NULL),
	fFirstChar(0),
	fLastChar(255),
	fFontData(NULL),
#if 0
	fMem(NULL),
	fInput(NULL),
	fFont(NULL),
	fScaler(NULL)
#endif
	fFace(0),
	fResX(72),
	fResY(72)
{
	fontobj->ResolveArrayOrDictionary();
	PDFObject *base_font = fontobj->Find(PDFAtom.BaseFont);
	if (base_font) fFontName = base_font->GetCharPtr();

	PDFObject *encoding = fFontObject->Find(PDFAtom.Encoding);
	if (encoding) {
#ifndef NDEBUG
//		printf("Encoding:");encoding->PrintToStream(); printf("\n");
#endif
	}

#if 1	
	PDFObject *firstChar = fFontObject->Find(PDFAtom.FirstChar);
	if (firstChar) {
		fFirstChar = (uint16) firstChar->GetInt32();
	}
	
	PDFObject *lastChar = fFontObject->Find(PDFAtom.LastChar);
	if (lastChar) {
		fLastChar = (uint16) lastChar->GetInt32();
	}

#endif
	fDescriptor = fFontObject->Find(PDFAtom.FontDescriptor);
	if (fDescriptor) {
		#ifndef NDEBUG
//		printf("Descriptor:"); descriptor->PrintToStream(); printf("\n");
		#endif	
		PDFObject *flags = fDescriptor->Find(PDFAtom.Flags);
		if (flags) fFlags = flags->GetInt32();
	}

#if 0
	printf("FontEngine::FontEngine:");
	fontobj->PrintToStream(5);
	printf("\n");
#endif
}

FontEngine::~FontEngine()
{
	int errCode;
#if 0
	if (fScaler) DeleteT2K(fScaler, &errCode);
	if (fFont) FF_Delete_sfntClass(fFont, &errCode);
	if (fInput) Delete_InputStream(fInput, &errCode);
	if (fMem) tsi_DeleteMemhandler(fMem);
#endif
	ResetFreeType();
	delete fFontData; fFontData = NULL;
	delete fEncoding; fEncoding = NULL;
	fFontObject->Release();
}

const char *
FontEngine::FontName() const
{
	return fFontName;
}

status_t 
FontEngine::RenderGlyph(uint16 code, glyph_data &glyph)
{
	int errCode = 0;
	uint16 unicode = (fEmbeddedFont) ? code : fEncoding->UnicodeFor(code);
#if 1
	printf("render glyph: 0x%04x->0x%04x\n", code, unicode);
	uint8 buffer[64];
	FT_Get_Glyph_Name(fFace, code, buffer, sizeof(buffer));
	printf("Name for glyph ->%s<-\n", buffer);
#endif
	// load the glyph and render it
	errCode = FT_Load_Char(fFace, unicode, FT_LOAD_RENDER);
	if (errCode == 0)
	{
		// copy the data into the gylph
		FT_GlyphSlot gs = fFace->glyph;
		glyph.height = gs->bitmap.rows;;
		glyph.width = gs->bitmap.width;
		glyph.rowbytes = gs->bitmap.pitch;
		int32 toCopy = glyph.rowbytes * glyph.height;
		printf("code: %d\n", code);
		for (int i = 0; i < toCopy; i++)
		{
			printf("%02x", gs->bitmap.buffer[i]);
			if ((i % glyph.rowbytes) == (glyph.rowbytes-1)) printf("\n");
		}
		glyph.bits = (uint8 *)malloc(toCopy);
		memcpy(glyph.bits, gs->bitmap.buffer, toCopy); 
		glyph.lefttop.Set(gs->bitmap_left, gs->bitmap_top);
		glyph.vector.Set((double)gs->advance.x / (1 << 6), (double)gs->advance.y / (1 << 6));
	}
	return errCode;
}

#define FIXED_AS_FLOAT(x) ((x) / 65536.0)
status_t 
FontEngine::UpdateMatrix(const font_matrix &fm)
{
	int errCode;
	FT_Matrix matrix;
	matrix.xx = FLOAT_TO_FIXED(fm.f00);
	// yes, the next two items ARE NOT swapped on purpose
	matrix.xy = FLOAT_TO_FIXED(fm.f01);
	matrix.yx = FLOAT_TO_FIXED(fm.f10);
	//
	matrix.yy = -FLOAT_TO_FIXED(fm.f11);

	// extract point size from matrix
	FT_Fixed xPointSize = EuclidianDistance(matrix.xx, matrix.xy);
	FT_Fixed yPointSize = EuclidianDistance(matrix.yy, matrix.yx);
	printf("point size: %f,%f\n", FIXED_AS_FLOAT(xPointSize), FIXED_AS_FLOAT(yPointSize));
	matrix.xx = FT_DivFix(matrix.xx, xPointSize);
	matrix.xy = FT_DivFix(matrix.xy, xPointSize);
	matrix.yy = FT_DivFix(matrix.yy, yPointSize);
	matrix.yx = FT_DivFix(matrix.yx, yPointSize);
	// set char size in points
	FT_Set_Char_Size(fFace, xPointSize >> 10, yPointSize >> 10, fResX, fResY);
	// set matrix for transforming glyphs
	FT_Set_Transform(fFace, &matrix, NULL);
	return B_OK;
}

status_t 
FontEngine::LoadFont()
{
	status_t status = B_OK;
	status = LoadEmbeddedFont();
	if (status != B_OK) {
		status = LoadFontFromDisk();
	}
	if (status != B_OK)
		return status;
	status = FT_Select_Charmap(fFace, ft_encoding_unicode);
	if (status != B_OK)
		return status;
	status = LoadEncoding();
	if (status != B_OK)
		return status;
		
	status = InitFreeType();

	return status;
}

status_t 
FontEngine::LoadEmbeddedFont()
{
	if (!fDescriptor) {
		return B_NAME_NOT_FOUND;
	}
	PDFObject *fontFileRef = NULL;
	
	fontFileRef = fDescriptor->Find(PDFAtom.FontFile1);
	if (fontFileRef)
		;//printf("using FontFile1\n");
	else {
		fontFileRef = fDescriptor->Find(PDFAtom.FontFile2);
		if (fontFileRef)
			;//printf("using FontFile2\n");
		else {
			fontFileRef = fDescriptor->Find(PDFAtom.FontFile3);
			if (fontFileRef)
				;//printf("using FontFile3\n");
			else {
				return B_NAME_NOT_FOUND;	
			}
		}
	}
	PDFObject *fontFile = fontFileRef->Resolve();
	
	#ifndef NDEBUG
	printf("fontFile: "); fontFile->PrintToStream(); printf("\n");	
	#endif
	
	BMallocIO *mio = dynamic_cast<BMallocIO *>(fontFile->RealizeStream());
	if (mio) {
		printf("buffer: %p, length: %Ld\n", mio->Buffer(), mio->Seek(0, SEEK_END));
		fEmbeddedFont = true;
		return FT_New_Memory_Face(gLibrary.fLibrary, (uint8*)mio->Buffer(), mio->Seek(0, SEEK_END), 0, &fFace);
	}
	else {
		return B_NAME_NOT_FOUND;
	}
}

status_t 
FontEngine::LoadFontFromDisk()
{
	char * fontName = MapFontByName();
	if (!fontName)
		fontName = MapFontByMetrics();
	
	BString fileName;
	
	// look for the fonts in the font directories

	BPath dirpath;
	find_directory(B_BEOS_FONTS_DIRECTORY, &dirpath);
	// try true-type
	BDirectory dir(dirpath.Path());
	fileName << "ttfonts/" << fontName << ".ttf";
	BEntry entry;
	if (dir.FindEntry(fileName.String(), &entry) != B_OK) {
		// try postscript binary
		fileName = "PS-Type1/";
		fileName << fontName << ".pfb";
		if (dir.FindEntry(fileName.String(), &entry) != B_OK) {
			// try postscript ascii
			fileName = "PS-Type1/";
			fileName << fontName << ".pfa";
			dir.FindEntry(fileName.String(), &entry);
		}
	}
	if (!entry.Exists()) {
		// get a default
		fileName = "ttfonts/Swiss721.ttf";
		dir.FindEntry(fileName.String(), &entry);
	}
	BString fullPath(dirpath.Path());
	fullPath << fileName;
	// actually load the font
	return FT_New_Face(gLibrary.fLibrary, fullPath.String(), 0, &fFace);
}

char *
FontEngine::MapFontByName()
{
	char *font = NULL;
	if (!fFontName) return font;

	if (fFontName == PDFAtom.Helvetica) {
		font = "Swiss721";
	}
	else if (fFontName == PDFAtom.Helvetica_Bold) {
		font = "Swiss721_Bold";
	}
	else if (fFontName == PDFAtom.Helvetica_BoldOblique) {
		font = "Swiss721_BoldItalic";
	}
	else if (fFontName == PDFAtom.Helvetica_Oblique) {
		font = "Swiss721_Italic";
	}
	else if (fFontName == PDFAtom.Courier) {
		font = "Courier10Pitch";
	}
	else if (fFontName == PDFAtom.Courier_Bold) {
		font = "Courier10Pitch_Bold";
	}
	else if (fFontName == PDFAtom.Courier_BoldOblique) {
		font = "Courier10Pitch_BoldItalic";
	}
	else if (fFontName == PDFAtom.Courier_Oblique) {
		font = "Courier10Pitch_Italic";
	}
	else if (fFontName == PDFAtom.Times_Roman) {
		font = "Dutch801";
	}
	else if (fFontName == PDFAtom.Times_Bold) {
		font = "Dutch801_Bold";
	}
	else if (fFontName == PDFAtom.Times_BoldItalic) {
		font = "Dutch801_BoldItalic";
	}
	else if (fFontName == PDFAtom.Times_Italic) {
		font = "Dutch801_Italic";
	}
	else if (fFontName == PDFAtom.Symbol) {
		font = "Symbol_Proportional";
	}
	// try and map by name
	else {
		const char *fname = fFontName;
		bool bold = (strstr(fname, "Bold") != NULL);
		bool italic = ((strstr(fname, "Italic") != NULL) || (strstr(fname, "Oblique") != NULL));
		const char *basefont = NULL;
	
		if	(	(strstr(fname, PDFAtom.Helvetica) != NULL)	||
				(strstr(fname, "Arial") != NULL) ||
				(strstr(fname, "Swiss") != NULL)
		  	) {
			basefont = PDFAtom.Helvetica;	
		}
		else if (strstr(fname, PDFAtom.Courier) != NULL) {
			basefont = PDFAtom.Courier;
		}
		else if ((strstr(fname, "Times") != NULL) ||
				 (strstr(fname, "Dutch") != NULL)) {
			basefont = PDFAtom.Times_Roman;	
		}
		else if (strstr(fname, PDFAtom.Symbol) != NULL) {
			basefont = PDFAtom.Symbol;
		}
	
		if (basefont == PDFAtom.Helvetica) {
			if (bold && italic) font = "Swiss721_BoldItalic";
			else if (bold) font = "Swiss721_Bold";
			else if (italic) font = "Swiss721_Italic";
			else font = "Swiss721";
		}
		else if (basefont == PDFAtom.Courier) {
			if (bold && italic) font = "Courier10Pitch_BoldItalic";
			else if (bold) font = "Courier10Pitch_Bold";
			else if (italic) font = "Courier10Pitch_Italic";
			else font = "Courier10Pitch";
		}
		else if (basefont == PDFAtom.Times_Roman) {
			if (bold && italic) font = "Dutch801_BoldItalic";
			else if (bold) font = "Dutch801_Bold";
			else if (italic) font = "Dutch801_Italic";
			else font = "Dutch801";
		}
		else if (basefont == PDFAtom.Symbol) font = "Symbol_Proportional";
	}
	return font;
}

char *
FontEngine::MapFontByMetrics()
{
	return NULL;
}

status_t 
FontEngine::LoadEncoding()
{
	uint32 encode = UNKNOWN_ENCODING;
	if (IsNonSymbolic()) {
		encode = STD_ADOBE_ENCODING;
	} else if (fFontName == PDFAtom.Symbol) {
		encode = SYMBOL_ENCODING;
	} else if (fFontName == PDFAtom.ZapfDingbats) {
		encode = ZAPF_DIGBATS_ENCODING;
	}

	PDFObject *diffs = NULL;
	PDFObject *encoding = fFontObject->Find(PDFAtom.Encoding);
	
#ifndef NDEBUG
	//encoding->PrintToStream();
#endif	
	
	if (encoding) {
		const char *baseEncodingName = NULL;
		if (encoding->IsName()) {
			baseEncodingName = encoding->GetCharPtr();
		}
		else if (encoding->IsDictionary()) {
			
			PDFObject *baseEncoding = encoding->Find(PDFAtom.BaseEncoding);
			if (baseEncoding)
					baseEncodingName = baseEncoding->GetCharPtr();
	
			diffs = encoding->Find(PDFAtom.Differences);
		}
		
		// adjust base encoding
		if (baseEncodingName == PDFAtom.WinAnsiEncoding)
			encode = WIN_ANSI_ENCODING;
		else if (baseEncodingName == PDFAtom.MacRomanEncoding)
			encode = MAC_ROMAN_ENCODING;
		else if (baseEncodingName == PDFAtom.MacExpertEncoding)
			encode = MAC_EXPERT_ENCODING;
	}	
	fEncoding = new PDFFontEncoding(encode);

	if (diffs && diffs->IsArray()) {
		uint8 charcode = 0;
		int32 array_size = diffs->Array()->size();
		int32 count = 0;
		for (object_array::iterator i = diffs->begin(); i != diffs->end(); i++) {
			PDFObject *obj = *i;
			if (obj->IsNumber()) {
				charcode = (uint8) obj->GetInt32();
			}
			else if (obj->IsName()) {
				fEncoding->SetUnicodeFor(charcode, obj->GetCharPtr());
				charcode++;
			}
		}
	}	
	
#if SUPPORT_CMAP > 0
	// check to see if there is a ToUnicode array specified
	if (fEmbeddedFont) {
		printf("encoding before cmap: ");fEncoding->PrintToStream();
		PDFObject *toUnicode = fFontObject->Find(PDFAtom.ToUnicode);
		if (toUnicode) {
			ObjectParser *op = new ObjectParser(new CMapEngine(fEncoding));
#if DEBUG_ENCODING > 0
			PushStream(toUnicode, new TeePusher(op, stdout));
#else
			PushStream(toUnicode, op);
#endif
		}
		printf("encoding after cmap: ");fEncoding->PrintToStream();
	}
#endif
	return B_OK;
}

status_t 
FontEngine::InitFreeType()
{
#if 0
	int errCode;
	fMem = tsi_NewMemhandler(&errCode);
	if (errCode == 0) {
		fInput = New_NonRamInputStream( fMem, fFontData, ReadFont, fFontDataSize, &errCode);
		if (errCode == 0) {
			short fontType = FF_FontTypeFromStream(fInput);
			fFont = New_sfntClass(fMem, fontType, fInput, NULL, &errCode);
			if (errCode == 0) {
				fScaler = NewT2K(fMem, fFont, &errCode);
				if (errCode == 0) {
					Set_PlatformID( fScaler, 3 );
					Set_PlatformSpecificID( fScaler, 1 );
					return B_OK;
					
				}
			}
		}
	}

	if (errCode != 0) {
		ResetFreeType();
		return B_ERROR;
	}
	else return B_OK;
#endif
	return B_OK;
}

void 
FontEngine::ResetFreeType()
{
#if 0
	fMem = NULL;
	fFont = NULL;
	fScaler = NULL;
#endif
}

void 
FontEngine::ReadFont(void *io, uint8 *dest, unsigned long offset, long numBytes)
{
	BPositionIO *fontIO = (BPositionIO *)io;
	fontIO->ReadAt(offset, dest, numBytes);
}

