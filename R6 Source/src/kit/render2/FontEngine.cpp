//******************************************************************************
//
//	File:			render2/FontEngine.cpp
//
//	Description:	Local and Remote implementation of IFontEngine interface.
//	
//	Written by:		hippo
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <render2/IFontEngine.h>
#include <render2/Font.h>

using namespace B::Support2;
using namespace B::Render2;

class RFontEngine : public RInterface<IFontEngine>
{
	public:
							RFontEngine(	IBinder::arg o) : RInterface<IFontEngine>(o) {};
		virtual	ssize_t		RenderGlyph(	int32 glyph,
											BFont* inoutFont,
											glyph_metrics* outMetrics,
											void** outBits) const;
		
		virtual	status_t	GetNextFamily(BFont* inoutFont) const;
		
		virtual	status_t	GetNextStyle(BFont* inoutFont) const;
		
		virtual	status_t	SetFamilyAndStyle(	BFont* inoutFont,
												const char* family,
												const char* style) const;
		
		virtual	status_t	SetFamilyAndFace(	BFont* inoutFont,
												const char* family,
												uint16 face) const;
		
		virtual	status_t	GetFamilyAndStyle(	const BFont& font,
												BString* outFamily,
												BString* outStyle) const;
		
		virtual	status_t	GetHeight(const BFont& font, font_height* outHeight);
		
		virtual	status_t	GetGlyphShapes(	const BFont& font,
											const char* charArray,
											size_t numChars,
											const IRender::ptr& dest) const;
	
		virtual	status_t	GetGlyphFlags(	const BFont& font,
											const char* charArray,
											size_t numChars,
											uint8* outFlags) const;
};

ssize_t 
RFontEngine::RenderGlyph(int32 glyph, BFont *inoutFont, glyph_metrics *outMetrics, void **outBits) const
{
#warning implement RFontEngine::RenderGlyph	
	return B_ERROR;
}

status_t 
RFontEngine::GetNextFamily(BFont *inoutFont) const
{
#warning implement RFontEngine::GetNextFamily	
	return B_ERROR;
}

status_t 
RFontEngine::GetNextStyle(BFont *inoutFont) const
{
#warning implement RFontEngine::GetNextStyle	
	return B_ERROR;
}

status_t 
RFontEngine::SetFamilyAndStyle(BFont *inoutFont, const char *family, const char *style) const
{
#warning implement RFontEngine::SetFamilyAndStyle	
	return B_ERROR;
}

status_t 
RFontEngine::SetFamilyAndFace(BFont *inoutFont, const char *family, uint16 face) const
{
#warning implement RFontEngine::SetFamilyAndFace	
	return B_ERROR;
}

status_t 
RFontEngine::GetFamilyAndStyle(const BFont &font, BString *outFamily, BString *outStyle) const
{
#warning implement RFontEngine::GetFamilyAndStyle	
	return B_ERROR;
}

status_t 
RFontEngine::GetHeight(const BFont &font, font_height *outHeight)
{
#warning implement RFontEngine::GetHeight	
	return B_ERROR;
}

status_t 
RFontEngine::GetGlyphShapes(const BFont &font, const char *charArray, size_t numChars, const IRender:: ptr &dest) const
{
#warning implement RFontEngine::GetGlyphShapes	
	return B_ERROR;
}

status_t 
RFontEngine::GetGlyphFlags(const BFont &font, const char *charArray, size_t numChars, uint8 *outFlags) const
{
#warning implement RFontEngine::GetGlyphFlags	
	return B_ERROR;
}


const BValue IFontEngine::descriptor(BValue::TypeInfo(typeid(IFontEngine)));


B_IMPLEMENT_META_INTERFACE(FontEngine)

status_t 
LFontEngine::Called(BValue &in, const BValue &outBindings, BValue &out)
{
#warning implement LFontEngine::Called
	return B_ERROR;
}


