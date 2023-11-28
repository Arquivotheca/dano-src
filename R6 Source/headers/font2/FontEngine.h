/***************************************************************************
//
//	File:			font2/FontEngine.h
//
//	Description:	BFontEngine is a concrete implementation of IFontEngine
//					based on the FontFusion font engine
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _FONT2_FONT_ENGINE_H
#define _FONT2_FONT_ENGINE_H

#include <render2/IFontEngine.h>


namespace B {

namespace Font2 {

using namespace B::Render2;

class FontFusionFont;

class BFontEngine : public LFontEngine
{
	public:
							BFontEngine();
		virtual				~BFontEngine();
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
	private:
		FontFusionFont *	fFont;
};

} } // namespace B::Font2

#endif
