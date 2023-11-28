/*******************************************************************************
/
/	File:			render2/Pixmap.h
/
/   Description:    BPixmap is a raster image
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_RENDER2_PIXMAP_H
#define	_RENDER2_PIXMAP_H

#include <render2/Render.h>

namespace B {
namespace Render2 {

using namespace Support2;

/*----------------------------------------------------------------*/
/*----- BPixmap class --------------------------------------------*/

class BPixmap : public LVisual, public BPixelDescription
{
	public:
		B_STANDARD_ATOM_TYPEDEFS(BPixmap)
	
								BPixmap(const BPixelDescription &desc);
								BPixmap(uint32 width, uint32 height, pixel_format pf, uint32 flags = 0);
	
		virtual	void			Display(IRender::arg into);
		virtual	void			Invalidate(const BUpdate &update);
		virtual	void			Draw(IRender::arg into);
		virtual BRect			Bounds() const;

		virtual	void			DrawPixels(IRender::arg into) = 0;

	protected:

				int32			m_cacheId;
};

/*-------------------------------------------------------------*/

class BStaticPixmap : public BPixmap
{
	public:
		B_STANDARD_ATOM_TYPEDEFS(BStaticPixmap)
	
								BStaticPixmap(const BPixelDescription &desc, uint32 bytesPerRow);
								BStaticPixmap(uint32 width, uint32 height, pixel_format pf, uint32 bytesPerRow, uint32 flags = 0);
		virtual					~BStaticPixmap();
	
		virtual	void			DrawPixels(IRender::arg into);

				uint32			BytesPerRow() const				{ return m_bytesPerRow; };
				void *			Pixels() const					{ return m_pixels; };
				void *			PixelsForRow(uint32 row) const	{ return (uint8*)m_pixels + BytesPerRow()*row; };
				size_t			Size() const					{ return m_bytesPerRow*Height(); }

	private:

				uint32			m_bytesPerRow;
				void *			m_pixels;
				area_id			m_area;		
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} } // namespace B::Render2

#endif /* _RENDER2_PIXMAP_H */

