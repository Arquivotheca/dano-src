//******************************************************************************
//
//	File:			font2/FontEngine.cpp
//
//	Description:	BFontEngine implementation
//	
//	Written by:		hippo
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <font2/FontEngine.h>

#include <storage2/File.h>
#include <storage2/Path.h>
#include <storage2/FindDirectory.h>
#include <support2/PositionIO.h>
#include <support2/StdIO.h>
#include "t2k.h"
#include <malloc.h>

using namespace B::Storage2;
using namespace B::Font2;


static inline double get_16dot16(F16Dot16 val)
{
	return ((double)val) * (1.0/65536.0);
}

static inline int32 round_16dot16(F16Dot16 val)
{
	double frac = ((double)val) * (1.0/65535.0);
	if (frac >= 0) return (int32)(frac+.5);
	return (int32)(frac-.5);
}


#define USE_FFF 0

#if USE_FFF

namespace B {
namespace Font2 {

class FontFusionFont {
	public:
							FontFusionFont(int32 id, BFile *io);
							~FontFusionFont();					

		status_t			InitFF();
		status_t			UninitFF();

		status_t			RenderGlyph(int32 glyph, IFontEngine::glyph_metrics *outMetrics, void **outBits);

		inline int32		Id() const { return fID; };
		status_t			SetIO(BFile *io);

	private:
		static int 			ReadFont(void *io, uint8 *dest, unsigned long offset, long numBytes);
		void				ResetFF();
		int32				fID;

		tsiMemObject *		fMem;
		InputStream *		fInput;
		sfntClass *			fFont;
		T2K *				fScaler;
		IStorage *			fIO;
		int32				fIOSize;
		float				fMatrix[4];
};
}}


FontFusionFont::FontFusionFont(int32 id, BFile *io) :
	fID(id),
	fMem(NULL),
	fInput(NULL),
	fFont(NULL),
	fScaler(NULL),
	fIO(io)
{
#warning FontFusionFont matrix hard-coded to 14 0 0 14
	fMatrix[0] = 14;
	fMatrix[1] = 0;
	fMatrix[2] = 0;
	fMatrix[3] = 14;
}


FontFusionFont::~FontFusionFont()
{
	UninitFF();
}

status_t 
FontFusionFont::InitFF()
{
	if (!fIO)
		return B_NO_INIT;
	
	int errCode = 0;	
	status_t status = B_OK;
	fMem = tsi_NewMemhandler(&errCode);
	if (errCode == 0) {
		fInput = New_NonRamInputStream(fMem, fIO, ReadFont, fIOSize, &errCode);
		if (errCode == 0) {
			//short fontType = FF_FontTypeFromStream(fInput, &errCode);
			short fontType = FONT_TYPE_TT_OR_T2K;
			if (errCode == 0) {
				fFont = New_sfntClass(fMem, fontType, fInput, NULL, &errCode);
				if (errCode == 0) {
					fScaler = NewT2K(fMem, fFont, &errCode);
					if (errCode == 0) {
						FF_SetBitRange255(fScaler, 1);
						Set_PlatformID(fScaler, 3);
						Set_PlatformSpecificID(fScaler, 1);
						status = B_OK;
					}
					else bout << "NewT2K failed!" << endl;
				}
				else bout << "New_sfntClass failed!" << endl;
			}
			else bout << "FF_FontTypeFromStream failed!" << endl;
		}
		else bout << "New_NonRamInputStream failed!" << endl;
	}
	else bout << "tsi_NewMemhandler failed!" << endl;
	if (errCode != 0) {
		status = B_NO_INIT;
		ResetFF();
	}
	return status;
}

status_t 
FontFusionFont::UninitFF()
{
	int errCode = 0;
	if (fScaler)
		DeleteT2K(fScaler, &errCode);
	if (fFont)
		FF_Delete_sfntClass(fFont, &errCode);
	if (fInput)
		Delete_InputStream(fInput, &errCode);
	if (fMem)
		tsi_DeleteMemhandler(fMem);
	ResetFF();
}

status_t 
FontFusionFont::RenderGlyph(int32 glyph, IFontEngine::glyph_metrics *outMetrics, void **outBits)
{
#warning need to add B2dTransform to FontFusionFont::RenderGlyph args
	// check to see if we have an io
	if (!fIO) {
		bout << "FFF:fIO == NULL!" << endl;
		return B_NO_INIT;
	}
	// try to initialize font fusion if not done already
	if (!fScaler) {
		status_t status = InitFF();
		if (status != B_OK) {
			bout << "FFF::InitFF failed!" << endl;
			return status;
		}
	
	}
	
	
	// time to render the glyph
	int errCode = 0;

	// apply our stinking default matrix
	T2K_TRANS_MATRIX matrix;
	matrix.t00 = (int32)(fMatrix[0] * ONE16Dot16);
	matrix.t01 = (int32)(fMatrix[1] * ONE16Dot16);
	matrix.t10 = (int32)(fMatrix[2] * ONE16Dot16);
	matrix.t11 = (int32)(fMatrix[3] * ONE16Dot16);

	T2K_NewTransformation(fScaler, true, 72, 72, &matrix, true, &errCode);
	if (errCode == 0) {
		uint16 cmd = T2K_SCAN_CONVERT | T2K_NAT_GRID_FIT;
		T2K_RenderGlyph(fScaler, glyph, 0, 0, GREY_SCALE_BITMAP_HIGH_QUALITY, cmd, &errCode);
		if (errCode == 0) {
			// we have a valid character!
			// copy all of the relevant info out!
			outMetrics->bounds.Set(0, 0, fScaler->width, fScaler->height);
			outMetrics->bounds.OffsetTo((fScaler->fLeft26Dot6 >> 6), -(fScaler->fTop26Dot6 >> 6));
			outMetrics->rowbytes = fScaler->rowBytes;
			outMetrics->x_escape = get_16dot16(fScaler->xAdvanceWidth16Dot16);
			outMetrics->y_escape = -get_16dot16(fScaler->yAdvanceWidth16Dot16);
			outMetrics->x_bm_escape = round_16dot16(fScaler->xLinearAdvanceWidth16Dot16);
			outMetrics->y_bm_escape = -round_16dot16(fScaler->yLinearAdvanceWidth16Dot16);
			
			// copy the bitmap
			if (fScaler->baseAddr != NULL) {
				int32 size = fScaler->rowBytes * fScaler->height;
				*outBits = (uchar *) malloc(size);
				memcpy(*outBits, fScaler->baseAddr, size);
			}
			
			T2K_PurgeMemory(fScaler, 1, &errCode);
		}
	}
	return (errCode == 0) ? B_OK : B_ERROR;
}

status_t 
FontFusionFont::SetIO(BFile *io)
{
	if (!fIO)
		return B_ERROR;
	
	fIO = io;
	return B_OK;
}

int 
FontFusionFont::ReadFont(void *io, uint8 *dest, unsigned long offset, long numBytes)
{
	IStorage *fontIO = (IStorage *)io;
	status_t result = fontIO->ReadAt(offset, dest, numBytes);
	return (result < 0) ? -1 : 0;
//	BPositionIO *font = (BPositionIO *) io;
//	font->Seek(offset, SEEK_SET);
//	int ret =font->Read(dest, numBytes);
//	return (ret < 0) ? -1 : 0;
}

void 
FontFusionFont::ResetFF()
{
	fMem = NULL;
	fFont = NULL;
	fInput = NULL;
	fScaler = NULL;
}

#endif

BFontEngine::BFontEngine()
{
#warning only using one font : Dutch801.ttf
#if USE_FFF > 0
	BPath path;
	status_t status = find_directory(B_BEOS_FONTS_DIRECTORY, &path);
	if (status == B_OK) {
		path.Append("ttfonts/Dutch801.ttf");
		BFile * file = new BFile(path.Path(), B_READ_ONLY);
		status = file->InitCheck();
		if (status == B_OK) {
			off_t size = 0;
			status = file->GetSize(&size);
			if (status == B_OK) {
				fFont = new FontFusionFont(0, file);
			}
		}
	}
#endif
}


BFontEngine::~BFontEngine()
{
#if USE_FFF
	delete fFont;
#endif
}


static int ReadFont(void *io, uint8 *dest, unsigned long offset, long numBytes) 
{
	BFile *fontIO = (BFile *)io;
	status_t result = fontIO->ReadAt(offset, dest, numBytes);
	return result < 0 ? -1 : 0;
}

ssize_t 
BFontEngine::RenderGlyph(int32 glyph, BFont *inoutFont, glyph_metrics *outMetrics, void **outBits) const
{
	if (!outMetrics || !outBits) {
		return B_BAD_VALUE;
	}
	status_t status = B_OK;
	
#if USE_FFF > 0
	// always get font at 0
	status_t b_no_init = B_NO_INIT;
	if (fFont)
		status = fFont->RenderGlyph(glyph, outMetrics, outBits);
	else
		status = B_NO_INIT;
//	bout << "FFF:RTenderGlyph() returns:  " << BHexDump(&status, sizeof(status_t)) << " (B_NO_INIT: "<<  BHexDump(&b_no_init, sizeof(status_t)) << ")" << endl;
	return status;
#else		
	// we are going to do this the really slow, really ugly, really indented way
	int errCode = 0;
	tsiMemObject *mem = NULL;
	InputStream *input = NULL;
	sfntClass *font = NULL;
	T2K *scaler = NULL;

	BPath path;
	status = find_directory(B_BEOS_FONTS_DIRECTORY, &path);
	if (status == B_OK) {
		path.Append("ttfonts/Dutch801.ttf");
		BFile *file = new BFile(path.Path(), B_READ_ONLY);
		status = file->InitCheck();
		if (status == B_OK) {
			off_t size = 0;
			status = file->GetSize(&size);
			if (status == B_OK) {
				// let's initialize Font Fusion
				mem = tsi_NewMemhandler(&errCode);
				if (errCode == 0) {
					input = New_NonRamInputStream( mem, file, ReadFont, size, &errCode);
					short fontType = FONT_TYPE_TT_OR_T2K;
					font = New_sfntClass(mem, fontType, input, NULL, &errCode);
					if (errCode == 0) {
						scaler = NewT2K(mem, font, &errCode);
						if (errCode == 0) {
							FF_SetBitRange255(scaler, 1);
							Set_PlatformID(scaler, 3);
							Set_PlatformSpecificID(scaler, 1);
							T2K_TRANS_MATRIX matrix = {(int32)(24 * ONE16Dot16), 0, 0, (int32)(24 * ONE16Dot16)};
							T2K_NewTransformation(scaler, true, 72, 72, &matrix, true, &errCode);
							if (errCode == 0) {
								// time to render the glyph
								uint16 cmd = T2K_SCAN_CONVERT | T2K_NAT_GRID_FIT;
								T2K_RenderGlyph(scaler, glyph, 0, 0, GREY_SCALE_BITMAP_HIGH_QUALITY, cmd, &errCode);
								if (errCode == 0) {
									// we have a valid character!
									// copy all of the relevant info out!
									outMetrics->bounds.Set(0, 0, scaler->width, scaler->height);
									outMetrics->bounds.OffsetTo((scaler->fLeft26Dot6 >> 6), -(scaler->fTop26Dot6 >> 6));
									outMetrics->rowbytes = scaler->rowBytes;
									outMetrics->x_escape = get_16dot16(scaler->xAdvanceWidth16Dot16);
									outMetrics->y_escape = -get_16dot16(scaler->yAdvanceWidth16Dot16);
									outMetrics->x_bm_escape = round_16dot16(scaler->xLinearAdvanceWidth16Dot16);
									outMetrics->y_bm_escape = -round_16dot16(scaler->yLinearAdvanceWidth16Dot16);
									
									// copy the bitmap
									if (scaler->baseAddr != NULL) {
										int32 size = scaler->rowBytes * scaler->height;
										*outBits = (uchar *) malloc(size);
										memcpy(*outBits, scaler->baseAddr, size);
									}
									
									T2K_PurgeMemory(scaler, 1, &errCode);
									if (errCode == 0) {
										DeleteT2K(scaler, &errCode);
										if (errCode == 0) {
											FF_Delete_sfntClass(font, &errCode);
											if (errCode == 0) {
												Delete_InputStream(input, &errCode);
												if (errCode == 0)
													tsi_DeleteMemhandler(mem);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	if (file)
		delete file;
	}

	// clean up font fusion stuff
	if (errCode != 0)
		status = B_ERROR;
	return status;
#endif
}

status_t 
BFontEngine::GetNextFamily(BFont *inoutFont) const
{
#warning implement BFontEngine::GetNextFamily
}

status_t 
BFontEngine::GetNextStyle(BFont *inoutFont) const
{
#warning implement BFontEngine::GetNextStyle
}

status_t 
BFontEngine::SetFamilyAndStyle(BFont *inoutFont, const char *family, const char *style) const
{
#warning implement BFontEngine::SetFamilyAndStyle
}

status_t 
BFontEngine::SetFamilyAndFace(BFont *inoutFont, const char *family, uint16 face) const
{
#warning implement BFontEngine::SetFamilyAndFace
}

status_t 
BFontEngine::GetFamilyAndStyle(const BFont &font, BString *outFamily, BString *outStyle) const
{
#warning implement BFontEngine::GetFamilyAndStyle
}

status_t 
BFontEngine::GetHeight(const BFont &font, font_height *outHeight)
{
#warning implement BFontEngine::GetHeight
}

status_t 
BFontEngine::GetGlyphShapes(const BFont &font, const char *charArray, size_t numChars, const IRender:: ptr &dest) const
{
#warning implement BFontEngine::GetGlyphShapes
}

status_t 
BFontEngine::GetGlyphFlags(const BFont &font, const char *charArray, size_t numChars, uint8 *outFlags) const
{
#warning implement BFontEngine::GetGlyphFlags
}

