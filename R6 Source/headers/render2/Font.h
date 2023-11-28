/*******************************************************************************
/
/	File:			Font.h
/
/   Description:    BFont objects represent individual font styles.
/                   Global font cache and font info functions defined below.
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _RENDER2_FONT_H
#define _RENDER2_FONT_H


#include <support2/Value.h>
#include <render2/2dTransform.h>
#include <render2/IFontEngine.h>

namespace B {
namespace Render2 {

class IRender;

using namespace Support2;

struct font_height {
	float ascent;
	float descent;
	float leading;
	
	static font_height	undefined;
	
						font_height() { *this = undefined; }
						font_height(float a, float d, float l) : ascent(a), descent(d), leading(l) {}
						font_height(const font_height& fh) : ascent(fh.ascent), descent(fh.descent), leading(fh.leading) {}

	void				set_to(float _ascent, float _descent, float _leading) {
							ascent = _ascent; descent = _descent; leading = _leading;
						}
						
	font_height &		operator=(const font_height& fh) {
							ascent = fh.ascent; descent = fh.descent; leading = fh.leading;						
							return *this;
						}
	bool				operator==(const font_height& fh) const {
							return ((ascent == fh.ascent) && (descent == fh.descent) && (leading == fh.leading));
						}
	bool				operator!=(const font_height& fh) const {
							return ((ascent != fh.ascent) || (descent != fh.descent) || (leading != fh.leading));
						}

};

class BFont
{
	public:
								BFont(const IFontEngine::ptr& engine = IFontEngine::ptr());
								BFont(const BFont &font);
								~BFont();
	
		
		status_t				SetFamilyAndStyle(const char* family, 
												  const char* style);
		status_t				SetFamilyAndFace(const char* family, uint16 face);
		
		status_t				SetStyle(const char* style);
		status_t				SetFace(uint16 face);
		
		status_t				Status() const;
		
		void					SetSize(float size);
		void					SetTransform(const B2dTransform& transform);
		void					SetSpacing(uint8 spacing);
		void					SetFlags(uint32 flags);
	
		void					GetFamilyAndStyle(BString* outFamily,
												  BString* outStyle) const;
		float					Size() const;
		B2dTransform			Transform() const;
		uint8					Spacing() const;
		uint16					Face() const;
		uint32					Flags() const;
		
		status_t				GetHeight(font_height* outHeight);
			
		status_t				GetGlyphShapes(	const char* charArray,
												size_t numChars,
												const atom_ptr<IRender>& dest) const;
		status_t				GetGlyphFlags(	const char* charArray,
												size_t numChars,
												uint8* outFlags) const;
		BValue					AsValue();
		
		/* API for internal IFontEngine use Only ------ */
		static int32			invalid_token;
		
		void					SetEngine(const IFontEngine::ptr& engine);
		void					SetToken(int32 token);
		
		IFontEngine::ptr		Engine() const;
		int32					Token() const;
		
		BValue					AsParcel(uint8 type);
		/* -------------------------------------------- */
	private:
		
		IFontEngine::ptr		m_engine;
		int32					m_token;
		uint8					m_spacing;
		uint8					_m_pad;
		coord					m_size;
		uint32					m_flags;
		coord					m_transform[4];
		font_height				m_cachedHeight;
};

} } // end namespace B::Render2

#endif
