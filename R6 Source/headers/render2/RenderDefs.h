/*******************************************************************************
/
/	File:			GraphicsDefs.h
/
/   Description:    Color space definitions.
/
/	Copyright 1992-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _RENDER2_RENDERDEFS_H
#define _RENDER2_RENDERDEFS_H

#include <OS.h>
#include <support2/SupportDefs.h>

namespace B {
namespace Render2 {

using namespace Support2;

class IRender;
class IVisual;
class BRegion;
class BUpdate;
class BFont;
class BRenderInputPipe;
class BRenderOutputPipe;
class B2dTransform;

// -----------------------
// needed for compilation only (will go away)
class BShape;		// see Font.h:343
// -----------------------


struct BColorTransform {
	float matrix[4][5];
};
struct BGradient {
	float matrix[3][3];
};

/*----------------------------------------------------------------*/

enum winding_rule {
	B_WINDING_EVENODD = 1,
	B_WINDING_NONZERO,
	B_WINDING_POSITIVE,
	B_WINDING_NEGATIVE,
	B_WINDING_ABS_GEQ_TWO
};

enum texttopath_flags {
	B_JUSTIFY_LEFT		= 0x00000001,
	B_JUSTIFY_RIGHT		= 0x00000002,
	B_JUSTIFY_FULL		= 0x00000003,
	B_JUSTIFY_CENTER	= 0x00000004,

	B_VALIGN_BASELINE	= 0x00000010,
	B_VALIGN_TOP		= 0x00000020,
	B_VALIGN_BOTTOM		= 0x00000030,
	B_VALIGN_CENTER		= 0x00000040
};

enum join_mode {
	B_ROUND_JOIN = 0,
	B_MITER_JOIN,
	B_BEVEL_JOIN,
	B_BUTT_JOIN,
	B_SQUARE_JOIN
};

enum cap_mode {
	B_ROUND_CAP		= B_ROUND_JOIN,
	B_BUTT_CAP		= B_BUTT_JOIN,
	B_SQUARE_CAP	= B_SQUARE_JOIN

};

/*----------------------------------------------------------------*/

#define 	B_CSTR_LEN		(-1)

struct escapements {
	escapements() { }
	escapements(uint32 n, uint32 s) : nonspace(n), space(s) { }
	bool operator == (const escapements& e) const { return ((nonspace==e.nonspace) && (space==e.space)); }
	coord nonspace;
	coord space;
};

extern const escapements B_NO_ESCAPEMENT;

/*----------------------------------------------------------------*/

struct pixel_format {
	pixel_format() { }
	pixel_format(uint32 l, uint32 c) : layout(l), components(c) { }
	uint32	layout;
	uint32	components;
};


class BPixelData;

class BPixelDescription
{
public:
	enum {
		B_EDGE_PIXELS = 0x00000001
	};

	BPixelDescription()  {}
	BPixelDescription(uint32 width, uint32 height, pixel_format pf, uint32 flags = 0)
		: 	m_pixelFormat(pf), m_width(width), m_height(height), m_flags(flags)
	{
	}

	uint32				Width() const			{ return m_width; }
	uint32				Height() const			{ return m_height; }
	const pixel_format&	ColorSpace() const		{ return m_pixelFormat; }
	uint32				Flags() const			{ return m_flags; }
private:
	friend class BPixelData;
	pixel_format	m_pixelFormat;
	uint32			m_width;
	uint32			m_height;
	uint32			m_flags;
};


class BPixelData : public BPixelDescription
{
public:
	BPixelData()  {}
	BPixelData(	uint32 width,
				uint32 height,
				uint32 bytesPerRow,
				const pixel_format& pf,
				const void* data,
				area_id area=-1)
		:	BPixelDescription(width, height, pf, 0),
			m_bytesPerRow(bytesPerRow), m_data(data), m_area(area)
	{
	}
	
	uint32			BytesPerRow() const		{ return m_bytesPerRow; }
	size_t			Size() const			{ return m_bytesPerRow*m_height; }
	bool			IsValid() const			{ return (m_data!=NULL); }
	const void*		Data() const			{ return m_data; }

private:
	bool			IsArea() const			{ return (m_area >= 0); }
	area_id			Area() const			{ return m_area; }

private:
	uint32			m_bytesPerRow;
	const void* 	m_data;
	area_id			m_area;		
};



/*----------------------------------------------------------------*/

} } // namespace B::Render2

#endif	/* _RENDER2_RENDERDEFS_H */
