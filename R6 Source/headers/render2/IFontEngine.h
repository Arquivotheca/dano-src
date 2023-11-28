/***************************************************************************
//
//	File:			render2/IFontEngine.h
//
//	Description:	Abstract font management and rendering class.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _RENDER2_FONT_ENGINE_INTERFACE_H
#define _RENDER2_FONT_ENGINE_INTERFACE_H

#include <support2/IInterface.h>
#include <support2/Binder.h>
#include <render2/Render.h>

namespace B {
namespace Render2 {

extern const BValue	g_keyRenderGlyph;
extern const BValue	g_keyGetNextFamily;
extern const BValue	g_keyGetNextStyle;
extern const BValue	g_keySetFamilyAndStyle;
extern const BValue	g_keySetFamilyAndFace;
extern const BValue	g_keyGetFamilyAndStyle;
extern const BValue	g_keyGetHeight;
extern const BValue	g_keyGetGlyphShapes;
extern const BValue	g_keyGetGlyphFlags;

class BFont;
struct font_height;

class IFontEngine : public IInterface
{
	public:
		B_DECLARE_META_INTERFACE(FontEngine)
		
		struct glyph_metrics {
			BRect bounds;			// Location and size of rendered bits
			int32 rowbytes;			// bytes per row
			coord x_escape;			// "Real" escapement to next character
			coord y_escape;
			int32 x_bm_escape;		// Hinted escapement to next character
			int32 y_bm_escape;
		
			void reset() {
				bounds.Set(0, 0, -1, -1);
				rowbytes = x_bm_escape = y_bm_escape = 0;
				x_escape = y_escape = 0.0;
			}
			
			glyph_metrics() { reset(); }
		
		};
		
		/*	This is the main reason for the interface.
			Given a font specification, render it and return
			the resulting bitmap.  It fills in "outMetrics" with
			the dimensions of the rendered glyph, "outBits"
			with the memory containing the bits (which the caller
			must deallocate, and whose format is dependent on
			the font specification), and returns the number of
			bytes in "outBits".
			
			In addition, RenderGlyph() may return B_REDIRECT.
			In this case, "inoutFont" is filled in with a new
			font to render, and outMetrics and outBits are invalid.
			The caller must then call RenderGlyph() again to render
			using the new font.  This protocol is intended to allow
			a font engine to perform font subsitutation, without
			duplicating rendered glyphs in the caller's cache.
		*/
		virtual	ssize_t		RenderGlyph(	int32 glyph,
											BFont* inoutFont,
											glyph_metrics* outMetrics,
											void** outBits) const = 0;
		
		/*	This function iterates through all available font families.
			Calling with an empty BFont returns the first family, each
			successive call returns the next family.
		*/
		virtual	status_t	GetNextFamily(BFont* inoutFont) const = 0;
		
		/*	This function iterates through all designed styles in a
			family, changing the given font to the next style in its
			family.
		*/
		virtual	status_t	GetNextStyle(BFont* inoutFont) const = 0;
		
		/*	Change the font to the given family and style in this engine.
		*/
		virtual	status_t	SetFamilyAndStyle(	BFont* inoutFont,
												const char* family,
												const char* style) const = 0;
		
		/*	Change the font to the given family and face in this engine.
		*/
		virtual	status_t	SetFamilyAndFace(	BFont* inoutFont,
												const char* family,
												uint16 face) const = 0;
		
		/*	Return the family and style names for this font.
		*/
		virtual	status_t	GetFamilyAndStyle(	const BFont& font,
												BString* outFamily,
												BString* outStyle) const = 0;
		
		/*	Return the height metrics for the given font.
		*/
		virtual	status_t	GetHeight(const BFont& font, font_height* outHeight) = 0;
		
		/* Draw the selected characters into the render context, as shapes.
		*/
		virtual	status_t	GetGlyphShapes(	const BFont& font,
											const char* charArray,
											size_t numChars,
											const IRender::ptr& dest) const = 0;
	
		/* Return information about character glyphs in the selected font.
		*/
		enum {
			GLYPH_EXISTS = 1<<0,			// Glyph exists in the font.
			GLYPH_LOCAL_OVERLAY = 1<<1,		// Glyph can be produced by local overlay.
			GLYPH_GLOBAL_OVERLAY = 1<<2		// Glyph can be produced by global overlay.
		};
		virtual	status_t	GetGlyphFlags(	const BFont& font,
											const char* charArray,
											size_t numChars,
											uint8* outFlags) const = 0;
};

class LFontEngine : public LInterface<IFontEngine>
{
	public:
		inline							LFontEngine() { }
		// in response to an Invoke() call
		virtual	status_t				Called(	BValue &in,
												const BValue &outBindings,
												BValue &out);
	protected:
		inline virtual					~LFontEngine() { }
};

} } // namespace B::Render2

#endif
