
#include <malloc.h>
#include <raster2/RasterPoint.h>
#include <render2/Pixmap.h>
#include <support2/StdIO.h>

namespace B {
namespace Render2 {

BPixmap::BPixmap(const BPixelDescription &desc)
	:	BPixelDescription(desc.Width(), desc.Height(), desc.ColorSpace(), desc.Flags()),
		m_cacheId(0)
{
}

BPixmap::BPixmap(uint32 width, uint32 height, pixel_format pf, uint32 flags)
	:	BPixelDescription(width, height, pf, flags),
		m_cacheId(0)
{
}

void 
BPixmap::Display(const IRender::ptr& into)
{
	into->DisplayCached(this, Flags(), m_cacheId);
}

void 
BPixmap::Invalidate(const BUpdate &update)
{
	#warning revisit Invalidate()
	m_cacheId++;
}

BRect 
BPixmap::Bounds() const
{
	return BRect(0,0,Width(),Height());
}

void 
BPixmap::Draw(const IRender::ptr& into)
{
	into->BeginPixelBlock(*this);
	DrawPixels(into);
	into->EndPixelBlock();
}

/*-------------------------------------------------------------*/

BStaticPixmap::BStaticPixmap(const BPixelDescription &desc, uint32 bytesPerRow)
	: BPixmap(desc),
	  m_bytesPerRow(bytesPerRow),
	  m_pixels(NULL),
	  m_area(-1)
{
	m_pixels = malloc(Size());
}

BStaticPixmap::BStaticPixmap(uint32 width, uint32 height, pixel_format pf, uint32 bytesPerRow, uint32 flags)
	: BPixmap(width, height, pf, flags),
	  m_bytesPerRow(bytesPerRow),
	  m_pixels(NULL),
	  m_area(-1)
{
	m_pixels = malloc(Size());
}

BStaticPixmap::~BStaticPixmap()
{
	free(m_pixels);
}

void 
BStaticPixmap::DrawPixels(const IRender::ptr& into)
{
	BPixelData pd(Width(),Height(),BytesPerRow(),ColorSpace(),Pixels());
	into->PlacePixels(BRasterPoint(0,0),pd);
}

} }
