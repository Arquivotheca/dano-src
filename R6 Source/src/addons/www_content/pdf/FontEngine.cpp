
#include "FontEngine.h"


#include <FindDirectory.h>
#include <Path.h>
#include <Directory.h>
#include <DataIO.h>
#include <Entry.h>
#include <File.h>
#include <string.h>
#include <String.h>
#include <Binder.h>

#include <functional>
#include <algorithm>
#include <hash_map>

#include "cachemgr.h"
#include "FontUtils.h"

#define DUMP_FONTS 0

#define FONT_CACHE_SIZE (64 * 1024)
static FF_CM_Class 	*theCache = 0;
static int32 theCacheUseCount = 0;

extern PDFAtoms BPrivate::PDFAtom;


static StandardWidths		gStdWidths;
static CodeToPSNameMap		gCtNMap;
static PSNameToUnicodeMap	gNtUMap;

glyph_data::glyph_data() :
	bits(0),
	rowbytes(0),
	lefttop(B_ORIGIN),
	width(0.0),
	height(0.0)
{
}


glyph_data::~glyph_data()
{
}

void 
glyph_data::reset()
{
	bits = 0;
	rowbytes = 0;
	width = height = 0.0;
	lefttop = B_ORIGIN;
}

//#pragma mark -

static T2K_TRANS_MATRIX gVectorMatrix = {(int32)(1000 * ONE16Dot16), 0, 0, -(int32)(1000 * ONE16Dot16)};

status_t 
FontEngine::Strike::RenderGlyph(uint16 code, glyph_data &glyph)
{
	int errCode = 0;
	uint16 cmd = T2K_SCAN_CONVERT;

	if (fEngine.fCurrentStrike != this)
	{
		fEngine.fCurrentStrike = this;
		T2K_TRANS_MATRIX dummy = fMatrix;
		T2K_NewTransformation(fEngine.fScaler, true, 72, 72, &dummy, true, &errCode );
		assert(errCode == 0);
	}

#if 1
	assert(theCache != 0);
	FF_CM_RenderGlyph(theCache, (uint32)this, &(fEngine.fScaler), code, 0, 0, GREY_SCALE_BITMAP_HIGH_QUALITY, cmd, &errCode);
#else
	T2K_RenderGlyph(fEngine.fScaler, code, 0, 0, GREY_SCALE_BITMAP_HIGH_QUALITY, cmd, &errCode);
#endif
	if (errCode != 0) {
		//ResetFontFusion();
		debugger("RenderGlyph() failed");
		return B_ERROR;
	}
	
	// copy the data into the gylph
	glyph.rowbytes = fEngine.fScaler->rowBytes;
	glyph.lefttop.Set((fEngine.fScaler->fLeft26Dot6 >> 6), (fEngine.fScaler->fTop26Dot6 >> 6));
	glyph.width = fEngine.fScaler->width;
	glyph.height = fEngine.fScaler->height;
	glyph.bits = fEngine.fScaler->baseAddr;
	return B_OK;
}

void 
FontEngine::Strike::ReleaseGlyph()
{
	int errCode = 0;
	T2K_PurgeMemory(fEngine.fScaler, 1, &errCode);
	assert(errCode == 0);
}

FontEngine::Strike::Strike(FontEngine &engine, T2K_TRANS_MATRIX &matrix, bool unused)
	: 	fEngine(engine), fMatrix(matrix)
{
	(void)unused;
}

FontEngine::Strike::Strike(FontEngine &engine, T2K_TRANS_MATRIX &matrix)
	: fEngine(engine), fMatrix(matrix)
{
	int errCode;
	T2K_TRANS_MATRIX dummy = fMatrix;
	T2K_NewTransformation(fEngine.fScaler, true, 72, 72, &dummy, true, &errCode );
	assert(errCode == 0);
	fEngine.fCurrentStrike = this;
}


FontEngine::Strike::~Strike()
{
}

//#pragma mark -

FontEngine::Encoding::Encoding() :
	fBaseEncoding(UNKNOWN_ENCODING)
{
}

FontEngine::Encoding::~Encoding()
{
}

void 
FontEngine::Encoding::SetTo(int8 baseEncoding, PDFObject *diffs)
{
	fBaseEncoding = baseEncoding;
	if (diffs && diffs->IsArray()) {
		uint16 pdfCode = 0;
		
		for (object_array::iterator i = diffs->begin(); i != diffs->end(); i++) {
			PDFObject *obj = *i;
			if (obj->IsNumber()) {
				pdfCode = (int8) obj->GetInt32();
			}
			else if (obj->IsName()) {
				fDifferences[pdfCode] = obj->GetCharPtr();
				pdfCode++;
			}
		}
	}
}

const char *
FontEngine::Encoding::NameForCode(uint16 code)
{
	if (fBaseEncoding == UNKNOWN_ENCODING && fDifferences.size() == 0)
		return NULL;
		
	// look for the name in the differences map
	difference_map::iterator i = fDifferences.find(code);
	if (i != fDifferences.end()) {
		return i->second;
	// if not there take a look in the base encoding
	}
	else 
		return gCtNMap.NameForCode(code, fBaseEncoding);
}


//#pragma mark -
FontEngine *
FontEngine::SpecifyEngine(PDFObject *fontobj, status_t &status)
{
	FontEngine *eng = NULL;
	// look for the font engine embedded in the object
	if (fontobj) {
		PDFObject * fe = fontobj->Find(PDFAtom.__font_engine__);
		if (!fe) {
			// pass ownership of fontobj to engine
			eng = new FontEngine(fontobj);
			status = eng->LoadFont();
			if (status != B_OK && status != B_UNSUPPORTED) {
				// we still want to keep Type3 font engines around as we need the width info
//				printf("LoadFont returned: %s (%lx)\n", strerror(status), status);
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
			//fontobj->Release();
		}
		// don't keep the font object around on our behalf
		fontobj->Release();
	}
	return eng;
}

bool 
FontEngine::Strike::strike_less(Strike const *a, Strike const *b)
{
#if 0
	printf("strike_less: A [%f %f %f %f] vs B [%f %f %f %f]\n",
				(float)a->fMatrix.t00 / (ONE16Dot16),
				(float)a->fMatrix.t01 / (ONE16Dot16),
				(float)a->fMatrix.t10 / (ONE16Dot16),
				(float)a->fMatrix.t11 / (ONE16Dot16),
				(float)b->fMatrix.t00 / (ONE16Dot16),
				(float)b->fMatrix.t01 / (ONE16Dot16),
				(float)b->fMatrix.t10 / (ONE16Dot16),
				(float)b->fMatrix.t11 / (ONE16Dot16));
#endif
	if (a->fMatrix.t00 < b->fMatrix.t00) return true;
	if (a->fMatrix.t00 > b->fMatrix.t00) return false;
	if (a->fMatrix.t01 < b->fMatrix.t01) return true;
	if (a->fMatrix.t01 > b->fMatrix.t01) return false;
	if (a->fMatrix.t01 < b->fMatrix.t01) return true;
	if (a->fMatrix.t01 > b->fMatrix.t01) return false;
	if (a->fMatrix.t11 < b->fMatrix.t11) return true;
	return false;
#if 0
	
	if (a->fMatrix.t11 < b->fMatrix.t11) return
	return (
		(a->fMatrix.t00 < b->fMatrix.t00) && 
		(a->fMatrix.t01 < b->fMatrix.t01) && 
		(a->fMatrix.t10 < b->fMatrix.t10) && 
		(a->fMatrix.t11 < b->fMatrix.t11)
	);
	//return (memcmp(&(a->fMatrix), &(b->fMatrix), sizeof(a->fMatrix)) < 0);
#endif
}

FontEngine::Strike *
FontEngine::StrikeForFont(Transform2D &fm, status_t &status)
{
	
	if (fSubtype == PDFAtom.Type3 || fSubtype == PDFAtom.Type0)
		return 0;

	Strike *strike = 0;
	T2K_TRANS_MATRIX matrix;
	matrix.t00 = (int32) (ONE16Dot16 * fm.A());
	// yes, the next two items ARE swapped on purpose
	matrix.t01 = (int32) (ONE16Dot16 * fm.C());
	matrix.t10 = (int32) -(ONE16Dot16 * fm.B());
	//
	matrix.t11 = (int32) -(ONE16Dot16 * fm.D());
	// hunt for a pre-existing strike
	Strike target(*this, matrix, false);
	strike_vector::iterator svi = lower_bound(fStrikes.begin(), fStrikes.end(), &target, Strike::strike_less);
	// svi points to either the correct strike, or the one after it (possibly end()).
#if 0
	printf("svi == fStrikes.end()?   %s\n", svi == fStrikes.end() ? "true" : "false");
	printf("svi == fStrikes.begin()? %s\n", svi == fStrikes.begin() ? "true" : "false");
	if (svi != fStrikes.end()) printf("lower_bound() found %p = [ %f %f %f %f ] looking for [%f %f %f %f]\n", (*svi),
				(float)(*svi)->fMatrix.t00 / (ONE16Dot16),
				(float)(*svi)->fMatrix.t01 / (ONE16Dot16),
				(float)(*svi)->fMatrix.t10 / (ONE16Dot16),
				(float)(*svi)->fMatrix.t11 / (ONE16Dot16),
				(float)target.fMatrix.t00 / (ONE16Dot16),
				(float)target.fMatrix.t01 / (ONE16Dot16),
				(float)target.fMatrix.t10 / (ONE16Dot16),
				(float)target.fMatrix.t11 / (ONE16Dot16)
				);
#endif
	if ((svi == fStrikes.end()) || (memcmp(&((*svi)->fMatrix), &matrix, sizeof(matrix)) != 0))
	{
#ifndef NDEBUG
		printf("New strike for %s ", FontName()); fm.PrintToStream(); printf("\n");
#endif
		// make the new strike
		strike = new Strike(*this, matrix);
#if 0
		printf("Strikes for %s.\n", FontName());
		strike_vector::iterator walker = fStrikes.begin();
		while (walker != fStrikes.end())
		{
			Strike *s = *walker++;
			printf("%p = [ %f %f %f %f ]\n", s,
				(float)s->fMatrix.t00 / (ONE16Dot16),
				(float)s->fMatrix.t01 / (ONE16Dot16),
				(float)s->fMatrix.t10 / (ONE16Dot16),
				(float)s->fMatrix.t11 / (ONE16Dot16));
		}
#endif
		// add it to our list of existing strikes
		fStrikes.insert(svi, strike);
#if 0
		//
		printf("New strike %p\n", strike);
		walker = fStrikes.begin();
		while (walker != fStrikes.end())
		{
			Strike *s = *walker++;
			printf("%p = [ %f %f %f %f ]\n", s,
				(float)s->fMatrix.t00 / (ONE16Dot16),
				(float)s->fMatrix.t01 / (ONE16Dot16),
				(float)s->fMatrix.t10 / (ONE16Dot16),
				(float)s->fMatrix.t11 / (ONE16Dot16));
		}
#endif
	}
	else strike = *svi;
#if 0
	printf("Found strike %p = [ %f %f %f %f ]\n", strike,
		(float)strike->fMatrix.t00 / (ONE16Dot16),
		(float)strike->fMatrix.t01 / (ONE16Dot16),
		(float)strike->fMatrix.t10 / (ONE16Dot16),
		(float)strike->fMatrix.t11 / (ONE16Dot16));
	if (
		(matrix.t00 != strike->fMatrix.t00) ||
		(matrix.t01 != strike->fMatrix.t01) ||
		(matrix.t10 != strike->fMatrix.t10) ||
		(matrix.t11 != strike->fMatrix.t11)
	) {
		printf("Failed to locate correct strike!\n");
		printf("Wanted [ %ld %ld %ld %ld ], got [ %ld %ld %ld %ld ]\n",
		matrix.t00,
		matrix.t01,
		matrix.t10,
		matrix.t11,
		strike->fMatrix.t00,
		strike->fMatrix.t01,
		strike->fMatrix.t10,
		strike->fMatrix.t11
		);
		debugger("matrix mismatch");
	}
//	else printf("Matched a strike correctly\n");
#endif
	return strike;
}


FontEngine::FontEngine(PDFObject *fontobj) :
	PDFOpaque(),
	fFontObject(fontobj),
	fSubtype(NULL),
	fFontName(NULL),
	fStandardFont(0),
	fDescriptor(NULL),
	fFlags(0),
	fBoundingBox(0, 0, 0, 0),
	fDefaultWidth(0.0),
	fFirstChar(0),
	fLastChar(0),
	fWidths(NULL),
	fEmbeddedFont(NULL),
	fFontDataSize(0),
	fFontData(NULL),
	fUsingT1Font(false),
	fT1FontData(NULL),
	fMem(NULL),
	fInput(NULL),
	fFont(NULL),
	fScaler(NULL),
	fCurrentStrike(0)
{
	int errCode;
	if (atomic_add(&theCacheUseCount, 1) == 0)
	{
		/* Create a new Cache Manager to play around with. */
		theCache = FF_CM_New(FONT_CACHE_SIZE, &errCode);
		assert( errCode == 0 );
		/* configure Cache filterTag for all the characters we will make */
		FF_CM_SetFilter(theCache, 
						0,
						NULL, 
						NULL);
	}
}

FontEngine::~FontEngine()
{
	int errCode;
	// delete all of the strikes
	{
		strike_vector::iterator i = fStrikes.begin();
		strike_vector::iterator end = fStrikes.end();
		while (i != end) delete *i++;
	}
	if (fScaler) DeleteT2K(fScaler, &errCode);
	if (fFont) FF_Delete_sfntClass(fFont, &errCode);
	if (fInput) Delete_InputStream(fInput, &errCode);
	if (fMem) tsi_DeleteMemhandler(fMem);
	ResetFontFusion();
	
	if (!fEmbeddedFont && fFontData) {
		if (fUsingT1Font) {
			free(fT1FontData);
			fT1FontData = NULL;
		}
		else
			free(fFontData);
			
		fFontData = NULL;
	}
	if (fEmbeddedFont && fWidths)
		delete [] fWidths;
	fWidths = NULL;
	
	if (fEmbeddedFont)
		fEmbeddedFont->Release();
	if (fDescriptor)
		fDescriptor->Release();
	if (atomic_add(&theCacheUseCount, -1) == 1)
	{
		FF_CM_Delete(theCache, &errCode);
#if DEBUG
		printf("\n** Deleted FF font cache! **\n");
#endif
	}
}

const char *
FontEngine::FontName() const
{
	return fFontName;
}

const char *
FontEngine::Subtype() const
{
	return fSubtype;
}

BRect 
FontEngine::BoundingBox() const
{
	return fBoundingBox;
}

void
FontEngine::GetGlyphInfo(uint16 pdfCode, glyph_info &info)
{
	// check to see if we have the glyph info stashed somewhere
	ginfo_map::iterator i = fGlyphInfo.find(pdfCode);
	if (i != fGlyphInfo.end()) {
		info = i->second;
	}
	else {
		const char *name = fEncoding.NameForCode(pdfCode);
		info.ffcode = (name) ? FFCodeForName(name) : pdfCode;
		if (name == PDFAtom.space)
			info.isSpace = true;

		if (fEmbeddedFont || fWidths != NULL) {
			if (IsFixedFont() || pdfCode < fFirstChar || pdfCode > fLastChar)
				info.width = fDefaultWidth;
			else
				info.width = fWidths[pdfCode - fFirstChar];
		}
		else {
			// we are using a standard font
			info.width = gStdWidths.WidthForName(name, fStandardFont);
		}

		fGlyphInfo[pdfCode] = info;
//		printf("pdfCode: %d ffcode: %d width: %0.3f name: %s\n", pdfCode, info.ffcode, info.width, name);
	}
}

status_t
FontEngine::LoadFont()
{
//	printf("LoadFont()\n");
	// resolve the dictionary
	status_t status = B_OK;
	fFontObject->ResolveArrayOrDictionary();
#if !defined(NDEBUG)
	printf("FontEngine::LoadFont() "); fFontObject->PrintToStream(3); printf("\n");
#endif
	PDFObject *obj = NULL;
	
	// get the required subtype
	obj = fFontObject->Find(PDFAtom.Subtype);
	if (obj == NULL)
		return B_FILE_ERROR;
	else {
		fSubtype = obj->GetCharPtr();
		if (fSubtype == PDFAtom.Type3 || fSubtype == PDFAtom.Type0) {
//			printf("%s fonts are unsupported!\n", fSubtype);
			status = B_UNSUPPORTED;
		}
	}

	if (fSubtype == PDFAtom.Type3) {
		// look for a name
//		obj = fFontObject->Find(PDFAtom.Name);
//		if (obj)
//			fFontName = obj->GetCharPtr();
	
		// look for the font bbox
		PDFObject * obj = fFontObject->Find(PDFAtom.FontBBox);
		if (obj && obj->IsArray()) {
			object_array *bbox = obj->Array();
			float llx, lly, urx, ury;
			llx = ((*bbox)[0])->GetFloat();
			lly = ((*bbox)[1])->GetFloat();
			urx = ((*bbox)[2])->GetFloat();
			ury = ((*bbox)[3])->GetFloat();
	
			fBoundingBox.left = llx * .001;
			fBoundingBox.bottom = lly * .001;
			fBoundingBox.right = urx * .001;
			fBoundingBox.top = ury * .001;
		}
	}
	else if (fSubtype != PDFAtom.Type0) {
		// we have a type1, truetype or other similar font
		// get the required base font
		obj = fFontObject->Find(PDFAtom.BaseFont);
		if (obj == NULL)
			return B_FILE_ERROR;
		else {
			fFontName = obj->GetCharPtr();
	//		printf("fFontName: %s\n", fFontName);
		}

		// see if there is a descriptor
		obj = fFontObject->Find(PDFAtom.FontDescriptor);
		if (obj) {
			fDescriptor = obj->Resolve();
			
			// get the flags
			obj = fDescriptor->Find(PDFAtom.Flags);
			if (obj) {
				fFlags = obj->GetInt32();
			}
			// get the bounding box
			PDFObject * obj = fDescriptor->Find(PDFAtom.FontBBox);
			if (obj && obj->IsArray()) {
				object_array *bbox = obj->Array();
				float llx, lly, urx, ury;
				llx = ((*bbox)[0])->GetFloat();
				lly = ((*bbox)[1])->GetFloat();
				urx = ((*bbox)[2])->GetFloat();
				ury = ((*bbox)[3])->GetFloat();
		
				fBoundingBox.left = llx * .001;
				fBoundingBox.bottom = lly * .001;
				fBoundingBox.right = urx * .001;
				fBoundingBox.top = ury * .001;
			}
			
			// get the missing width info
			obj = fDescriptor->Find(PDFAtom.MissingWidth);
			fDefaultWidth = (obj) ? (obj->GetFloat() * .001) : 0.0;
		}
		else
			LoadStandardDescriptor();

	}
			
	// look for the width dictionary
	PDFObject *widths = fFontObject->Find(PDFAtom.Widths);
	if (widths && widths->IsArray()) {
		
		obj = fFontObject->Find(PDFAtom.FirstChar);
		if (obj)
			fFirstChar = (uint16) obj->GetInt32();
		
		obj = fFontObject->Find(PDFAtom.LastChar);
		if(obj)
			fLastChar = (uint16)obj->GetInt32();
	
	
		object_array *w = widths->Array();
		// check to see if we are fixed width!
		if (!IsFixedFont()) {
//			printf("Not a fixed font!:\n   widths: ");
			int arraySize = fLastChar - fFirstChar +1;
			fWidths = new float[arraySize];
			for (int ix = 0; ix < arraySize; ix++) {
				fWidths[ix] = (((*w)[ix])->GetFloat() * .001);
//				printf("%d:%.3f ", fFirstChar + ix, fWidths[ix]);
			}
//			printf("\n");
		}
		else {
			fDefaultWidth = (((*w)[0])->GetFloat() * .001);
		}
	}


	if (fSubtype != PDFAtom.Type3 && fSubtype != PDFAtom.Type0) {
		// try and load the font and associated info
		status = B_ERROR;	
		if (fDescriptor)
			status = LoadEmbeddedFont();
			
		if (status != B_OK)
			// we need to use a replacement font from disk!
			status = LoadStandardFont();

		if (status == B_OK) {
			status = LoadEncoding();
			if (status == B_OK) {
				status = InitFontFusion();
			}
		}
	}

	return status;	
}

status_t 
FontEngine::LoadEmbeddedFont()
{
//	return B_NAME_NOT_FOUND;
	ASSERT(fEmbeddedFont == 0);

#if DEBUG
	printf("LoadEmbeddedFont()\n");
#endif

	if (!fDescriptor) {
#if DEBUG
		printf("no descriptor!\n");
#endif
		return B_NAME_NOT_FOUND;
	}
	PDFObject *fontFileRef = NULL;
#if 0
#if !NDEBUG
	printf("Descriptor: \n====================\n"); fDescriptor->PrintToStream(); printf("\n====================\n");
#endif
#endif
	fontFileRef = fDescriptor->Find(PDFAtom.FontFile);
	if (fontFileRef)
		;//printf("using FontFile\n");
	else {
		fontFileRef = fDescriptor->Find(PDFAtom.FontFile2);
		if (fontFileRef)
			;//printf("using FontFile2\n");
		else {
			fontFileRef = fDescriptor->Find(PDFAtom.FontFile3);
			if (fontFileRef)
				;//printf("using FontFile3\n");
			else {
				//printf("no embedded font file in descriptor\n");
				return B_NAME_NOT_FOUND;	
			}
		}
	}
	PDFObject *fontFile = fontFileRef->Resolve();
	
	#ifndef NDEBUG
//	printf("fontFile: "); fontFile->PrintToStream(); printf("\n");	
	#endif
	
	BMallocIO *fontIO = dynamic_cast<BMallocIO *>(fontFile->RealizeStream());
	if (fontIO) {
		fEmbeddedFont = fontFile;
		fFontDataSize = fontIO->BufferLength();
		fFontData = (uchar *)fontIO->Buffer();
		
#if DUMP_FONTS > 0
		// dump the file out to disk
		BString filename(fFontName);
		BDirectory dir("/source/private/pdf/fonts/");
		// check the type of font
		PDFObject *subtype = fFontObject->Find(PDFAtom.Subtype);
		if (subtype) {
			filename+= '.';
			filename.Append(subtype->GetCharPtr());
		}
		BFile file;
		if (dir.CreateFile(filename.String(), &file, true) == B_OK) {
			file.Write(fFontData, fFontDataSize);
		}
#endif		
		
		return B_OK;
	}
	else {
		fontFile->Release();
		return B_NAME_NOT_FOUND;
	}
}

int8
FontEngine::FindStandardFont(const char *name) {
	
	int8 font = 0;
	
	if (name == PDFAtom.Courier) {
		font = COURIER;
	}
	else if (name == PDFAtom.Courier_Bold) {
		font = COURIER_BOLD;
	}
	else if (name == PDFAtom.Courier_Oblique) {
		font = COURIER_OBLIQUE;
	}
	else if (name == PDFAtom.Courier_BoldOblique) {
		font = COURIER_BOLDOBLIQUE;
	}
	else if (name == PDFAtom.Helvetica) {
		font = HELVETICA;
	}
	else if (name == PDFAtom.Helvetica_Bold) {
		font = HELVETICA_BOLD;
	}
	else if (name == PDFAtom.Helvetica_Oblique) {
		font = HELVETICA_OBLIQUE;
	}
	else if (name == PDFAtom.Helvetica_BoldOblique) {
		font = HELVETICA_BOLDOBLIQUE;
	}
	else if (name == PDFAtom.Times_Roman) {
		font = TIMES_ROMAN;
	}
	else if (name == PDFAtom.Times_Bold) {
		font = TIMES_ROMAN_BOLD;
	}
	else if (name == PDFAtom.Times_Italic) {
		font = TIMES_ROMAN_ITALIC;
	}
	else if (name == PDFAtom.Times_BoldItalic) {
		font = TIMES_ROMAN_BOLDITALIC;
	}
	else if (name == PDFAtom.Symbol) {
		font = SYMBOL;
	}
	else if (name == PDFAtom.ZapfDingbats) {
		font = ZAPF_DINGBATS;
	}
	else {
		// we need to try and do something else fancy
		BString fname(name);
		bool bold = false;
		bool italic = false;
		
		// check for bold status
		if ((fname.IFindFirst("Bold") != B_ERROR) ||
			(fname.IFindFirst("Black") != B_ERROR) ||
			(fname.IFindFirst("Heavy") != B_ERROR) ||
			(fname.IFindFirst("Super") != B_ERROR) ||
			(fname.FindFirst("Bd") != B_ERROR) ||
			(fname.FindFirst("Blk") != B_ERROR) ||
			(fname.FindFirst("Hv") != B_ERROR) ||
			(fname.FindFirst("Su") != B_ERROR)) {
			
			bold = true;	
		}
		
		// check for oblique status
		if ((fname.IFindFirst("Oblique") != B_ERROR) ||
			(fname.IFindFirst("Italic") != B_ERROR) ||
			(fname.IFindFirst("Inclined") != B_ERROR) ||
			(fname.IFindFirst("Sloped") != B_ERROR) ||
			(fname.FindFirst("Obl") != B_ERROR) ||
			(fname.FindFirst("It") != B_ERROR) ||
			(fname.FindFirst("Ic") != B_ERROR) ||
			(fname.FindFirst("Sl") != B_ERROR)) {
			
			italic = true;	
		}
		
		// try and find a default base name
		if ((fname.IFindFirst(PDFAtom.Symbol) != B_ERROR)) {
			font = SYMBOL;
		}
//		else if ((fname.IFindFirst(PDFAtom.ZapfDingbats) != B_ERROR)) {
//			font = ZAPF_DINGBATS;
//		}
		else if ((fFlags & IS_FIXED)||
				(fname.IFindFirst(PDFAtom.Courier) != B_ERROR)) {
			if (bold && italic)
				font = COURIER_BOLDOBLIQUE;
			else if (italic)
				font = COURIER_OBLIQUE;
			else if (bold)
				font = COURIER_BOLD;
			else
				font = COURIER;
		}
		else if ((fFlags & IS_SERIF) ||
				(fname.IFindFirst("Times") != B_ERROR) ||
				(fname.IFindFirst("Dutch") != B_ERROR)) {
			if (bold && italic)
				font = TIMES_ROMAN_BOLDITALIC;
			else if (italic)
				font = TIMES_ROMAN_ITALIC;
			else if (bold)
				font = TIMES_ROMAN_BOLD;
			else
				font = TIMES_ROMAN;
		}

		// use Helvetica as the default if nothing else matches
		// note that we would try to match to Helvetica, Arial and Swiss if we
		// needed to keep searching
		else {
			if (bold && italic)
				font = HELVETICA_BOLDOBLIQUE;
			else if (italic)
				font = HELVETICA_OBLIQUE;
			else if (bold)
				font = HELVETICA_BOLD;
			else
				font = HELVETICA;
		}
	}
//	printf("standard font: %d\n", font);
	return font;
}


void 
FontEngine::LoadStandardDescriptor()
{
	// get the bounding box, flags and default width
	if (fStandardFont == 0)
		fStandardFont = FindStandardFont(fFontName);

	switch(fStandardFont) {
		case COURIER:
			fBoundingBox.Set(-.023, .805, .715, -.250);
			fFlags = IS_NONSYMBOLIC || IS_FIXED || IS_SERIF;
			break;
		case COURIER_BOLD:
			fBoundingBox.Set(-.113, .801, .749, -.250);
			fFlags = IS_NONSYMBOLIC || IS_FIXED || IS_SERIF;
			break;
		case COURIER_OBLIQUE:
			fBoundingBox.Set(-.027, .805, .849, -.250);
			fFlags = IS_NONSYMBOLIC || IS_FIXED || IS_SERIF || IS_ITALIC;
			break;
		case COURIER_BOLDOBLIQUE:
			fBoundingBox.Set(-.057, .801, .869, -.250);
			fFlags = IS_NONSYMBOLIC || IS_FIXED || IS_SERIF || IS_ITALIC;
			break;
		case HELVETICA:
			fBoundingBox.Set(-.166, .931, 1.000, -.225);
			fFlags = IS_NONSYMBOLIC;
			break;
		case HELVETICA_BOLD:
			fBoundingBox.Set(-.170, .962, 1.003, -.228);
			fFlags = IS_NONSYMBOLIC;
			break;
		case HELVETICA_OBLIQUE:
			fBoundingBox.Set(-.174, .962, 1.114, -.228);
			fFlags = IS_NONSYMBOLIC || IS_ITALIC;
			break;
		case HELVETICA_BOLDOBLIQUE:
			fBoundingBox.Set(-.174, .962, 1.114, -.228);
			fFlags = IS_NONSYMBOLIC || IS_ITALIC;
			break;
		case TIMES_ROMAN:
			fBoundingBox.Set(-.168, .898, 1.000, -.218);
			fFlags = IS_NONSYMBOLIC || IS_SERIF;
			break;
		case TIMES_ROMAN_BOLD:
			fBoundingBox.Set(-.168, .935, 1.000, -.218);
			fFlags = IS_NONSYMBOLIC || IS_SERIF;
			break;
		case TIMES_ROMAN_ITALIC:
			fBoundingBox.Set(-.169, .883, 1.010, -.217);
			fFlags = IS_NONSYMBOLIC || IS_SERIF || IS_ITALIC;
			break;
		case TIMES_ROMAN_BOLDITALIC:
			fBoundingBox.Set(-.200, .921, .996, -.218);
			fFlags = IS_NONSYMBOLIC || IS_SERIF || IS_ITALIC;
			break;
		case SYMBOL:
			fBoundingBox.Set(-.180, 1.010, 1.090, -.293);
			fFlags = IS_SYMBOLIC || IS_SERIF;
			break;
		case ZAPF_DINGBATS:
			fBoundingBox.Set(-.001, .820, .981, -.143);
			fFlags = IS_SYMBOLIC;
			break;
	}
}


status_t 
FontEngine::LoadStandardFont()
{
	ASSERT(fFontData == 0);
//	printf("LoadStandardFont()\n");
	if (fStandardFont == 0)
		fStandardFont = FindStandardFont(fFontName);

	const char *truetype = NULL;
	const char *type1 = NULL;

	switch(fStandardFont) {
		case COURIER:
			type1 = "Courier10PitchBT-Roman.pfb";
			truetype = "Courier10Pitch.ttf";
			break;
		case COURIER_BOLD:
			type1 = "Courier10PitchBT-Bold.pfb";
			truetype = "Courier10Pitch_Bold.ttf";
			break;
		case COURIER_OBLIQUE:
			type1 = "Courier10PitchBT-Italic.pfb";
			truetype = "Courier10Pitch_Italic.ttf";
			break;
		case COURIER_BOLDOBLIQUE:
			type1 = "Courier10PitchBT-BoldItalic.pfb";
			truetype = "Courier10Pitch_BoldItalic.ttf";
			break;
		case HELVETICA:
			type1 = "Swiss721BT-Roman.pfb";
			truetype = "Swiss721.ttf";
			break;
		case HELVETICA_BOLD:
			type1 = "Swiss721BT-Bold.pfb";
			truetype = "Swiss721_Bold.ttf";
			break;
		case HELVETICA_OBLIQUE:
			type1 = "Swiss721BT-Italic.pfb";
			truetype = "Swiss721_Italic.ttf";
			break;
		case HELVETICA_BOLDOBLIQUE:
			type1 = "Swiss721BT-BoldItalic.pfb";
			truetype = "Swiss721_BoldItalic.ttf";
			break;
		case TIMES_ROMAN:
			type1 = "Dutch801BT-Roman.pfb";
			truetype = "Dutch801.ttf";
			break;
		case TIMES_ROMAN_BOLD:
			type1 = "Dutch801BT-Bold.pfb";
			truetype = "Dutch801_Bold.ttf";
			break;
		case TIMES_ROMAN_ITALIC:
			type1 = "Dutch801BT-Italic.pfb";
			truetype = "Dutch801_Italic.ttf";
			break;
		case TIMES_ROMAN_BOLDITALIC:
			type1 = "Dutch801BT-BoldItalic.pfb";
			truetype = "Dutch801_BoldItalic.ttf";
			break;
		case SYMBOL:
			type1 = "SymbolMonospacedBT-Regular.pfb";
			truetype = "Symbol_Proportional.ttf";
			break;
		case ZAPF_DINGBATS:
			type1 = "ZapfDingbats.pfb";
			truetype = NULL;
			break;
	}

//	printf("fStandardFont: %d type1: %s truetype: %s\n", fStandardFont, type1, truetype);

	BPath fontdir;
	find_directory(B_BEOS_FONTS_DIRECTORY, &fontdir);
	

	BPath t1Path(fontdir.Path(), "PS-Type1/");
	BPath ttPath(fontdir.Path(), "ttfonts/");
	BPath fontPath;
	BEntry entry;
	// try type1
	if (type1) {
		fontPath.SetTo(t1Path.Path(), type1);
		entry.SetTo(fontPath.Path());
	}
	if (entry.Exists()) {
//		printf("found %s\n", fontPath.Path());
		fUsingT1Font = true;
	}
	// try truetype instead
	else if (truetype) {
//		printf("failed to find %s\n", fontPath.Path());
		fontPath.SetTo(ttPath.Path(), truetype);
		entry.SetTo(fontPath.Path());
	}
	// if we still don't have a good entry use the default
	if (!entry.Exists()) {
//		printf("failed to find %s\n", fontPath.Path());
		fontPath.SetTo(ttPath.Path(), "Swiss721.ttf");
		entry.SetTo(fontPath.Path()); 
	}
	
	if (entry.Exists()) {
		BPath p;
		entry.GetPath(&p);
//		printf("using: %s\n", p.Path());
		BFile file(&entry, B_READ_ONLY);
		status_t status = file.InitCheck();
		if (status != B_OK)
			return status;
	
		file.GetSize(&fFontDataSize);
		fFontData = (uchar *)malloc(fFontDataSize);;
		file.Read(fFontData, fFontDataSize);	

		if (fUsingT1Font) {
			int errCode;
			ulong len = fFontDataSize;
			uchar * fdata = ExtractPureT1FromPCType1( fFontData, &len, &errCode);
			if (errCode == 0) {
				fT1FontData = fFontData;
				fFontData = fdata;
				fFontDataSize = len;
			}
		}
		return B_OK;	
	}
	else
		return B_ENTRY_NOT_FOUND;
}


status_t 
FontEngine::LoadEncoding()
{
	int32 encode = UNKNOWN_ENCODING;
	if (IsNonSymbolic()) {
		encode = STD_ADOBE_ENCODING;
	} else if (fFontName == PDFAtom.Symbol) {
		encode = SYMBOL_ENCODING;
	} else if (fFontName == PDFAtom.ZapfDingbats) {
		encode = ZAPF_DINGBATS_ENCODING;
	}

	PDFObject *diffs = NULL;
	PDFObject *encoding = fFontObject->Find(PDFAtom.Encoding);
	
#ifndef NDEBUG
	printf("FontEngine::LoadEncoding() "); encoding->PrintToStream(3); printf("\n");
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
//		else
//			printf("unknown encoding name: %s\n", baseEncodingName);
	}
	else if (encode == UNKNOWN_ENCODING)
		encode = STD_ADOBE_ENCODING;
	
	fEncoding.SetTo(encode, diffs);

	return B_OK;
}

status_t 
FontEngine::InitFontFusion()
{
	int errCode;

	fMem = tsi_NewMemhandler(&errCode);
	if (errCode == 0) {
//		fInput = New_NonRamInputStream( fMem, fFontData, ReadFont, fFontDataSize, &errCode);
//		printf("InitFontFusion:: fMem: %p fFontData: %p fFontDataSize: %Ld\n", fMem, fFontData, fFontDataSize);
		fInput = New_InputStream3( fMem, fFontData, fFontDataSize, &errCode );
		if (errCode == 0) {
			short fontType = ff_FontTypeFromStream(fInput);
			if (fontType != -1) {
				fFont = New_sfntClass(fMem, fontType, fInput, NULL, &errCode);
				if (errCode == 0) {
					fScaler = NewT2K(fMem, fFont, &errCode);
					if (errCode == 0) {
						FF_SetBitRange255(fScaler, 1);
						Set_PlatformID( fScaler, 3 );
						Set_PlatformSpecificID( fScaler, 1 );
						fCurrentStrike = 0;
						T2K_TRANS_MATRIX matrix = gVectorMatrix;
						T2K_NewTransformation(fScaler, true, 72, 72, &matrix, true, &errCode );
						if (errCode == 0) {
							return B_OK;
						}
//						else printf("failed to get a new transformation\n");			
					}
//					else printf("failed to get a new scaler!\n");
				}
//				else printf("failed to build a new sfntclass\n");
			}
//			else printf("bogus font type!\n");
		}
//		else printf("failed to get new input stream!\n");
	}
//	else printf("failed to get a memhandler\n");

//	printf("have to reset fontfusion and return an error!\n");

	ResetFontFusion();
	return B_ERROR;
}

void 
FontEngine::ResetFontFusion()
{
	fMem = NULL;
	fFont = NULL;
	fScaler = NULL;
}


uint16 
FontEngine::FFCodeForName(const char *name)
{
	return gNtUMap.UnicodeFor(name);
}



//int 
//FontEngine::ReadFont(void *io, uint8 *dest, unsigned long offset, long numBytes)
//{
//	BPositionIO *fontIO = (BPositionIO *)io;
//	//if (offset >= (unsigned long)fontIO->GetSize()) return -1;
//	status_t result = fontIO->ReadAt(offset, dest, numBytes);
//	return result < 0 ? -1 : 0;
//}







