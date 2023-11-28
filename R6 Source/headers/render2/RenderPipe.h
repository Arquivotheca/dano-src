
#ifndef	_RENDER2_RENDERPIPE_H_
#define	_RENDER2_RENDERPIPE_H_

#include <support2/SupportDefs.h>
#include <support2/BufferedPipe.h>
#include <support2/String.h>

#include <render2/RenderDefs.h>
#include <raster2/RasterRect.h>
#include <raster2/RasterRegion.h>
#include <render2/Rect.h>
#include <render2/Point.h>

namespace B {
namespace Render2 {

using namespace Support2;
using namespace Raster2;

enum {
	B_RENDER_TRANSACTION = 'REND'
};

class BRenderOutputPipe : public BBufferedOutputPipe
{
	public:
							BRenderOutputPipe() {};
		virtual				~BRenderOutputPipe() {};

				void		writeRect(const BRect &p);
				void		writePoint(const BPoint &p);
				void		writeRasterRect(const BRasterRect &r);
				void		writeRasterPoint(const BRasterPoint &p);
				void		writeRasterRegion(const BRasterRegion &r);
				void		writeFont(const BFont &f);
				void		writeCoord(coord value);
};

class BRenderInputPipe : public BBufferedInputPipe
{
	public:
							BRenderInputPipe() {};
		virtual				~BRenderInputPipe() {};

				void		readRect(BRect &p);
				void		readPoint(BPoint &p);
				void		readRasterRect(BRasterRect &r);
				void		readRasterPoint(BRasterPoint &p);
				void		readRasterRegion(BRasterRegion &r);
				void		readFont(BFont &f);
				coord		readCoord();
};

class BRenderSocket
{
	public:
									BRenderSocket(BRenderInputPipe *in, BRenderOutputPipe *out)
										: m_in(in), m_out(out) {};
									~BRenderSocket() {};

				BRenderInputPipe *	Input() { return m_in; };
				BRenderOutputPipe *	Output() { return m_out; };

				void				write(const void *buffer, int32 size) { m_out->write(buffer,size); };
				void				write32(int32 value) { m_out->write32(value); };
				void				write16(int16 value) { m_out->write16(value); };
				void				write8(int8 value) { m_out->write8(value); };
				void				writef64(double value) { m_out->writef64(value); };
				void				writef32(float value) { m_out->writef32(value); };
				void				writestr(const char *str) { m_out->writestr(str); };
				void				writeRasterRect(const BRasterRect &r) { m_out->writeRasterRect(r); };
				void				writeRasterPoint(const BRasterPoint &p) { m_out->writeRasterPoint(p); };
				void				writeRasterRegion(const BRasterRegion &r) { m_out->writeRasterRegion(r); };
				void				writeRect(const BRect &p) { m_out->writeRect(p); };
				void				writePoint(const BPoint &p) { m_out->writePoint(p); };
				void				writeFont(const BFont &f) { m_out->writeFont(f); };
				void				writeCoord(coord value) { m_out->writeCoord(value); };
				void				flush() { m_out->flush(); };

				void				read(void *buffer, int32 size) { m_in->read(buffer,size); };
				int32				read32() { return m_in->read32(); };
				int16				read16() { return m_in->read16(); };
				int8				read8() { return m_in->read8(); };
				double				readf64() { return m_in->readf64(); };
				float				readf32() { return m_in->readf32(); };
				void				readstr(char *str) { m_in->readstr(str); };
				BString				readstr() { return m_in->readstr(); };
				void				readRect(BRect &p) { m_in->readRect(p); };
				void				readPoint(BPoint &p) { m_in->readPoint(p); };
				void				readRasterRect(BRasterRect &r) { m_in->readRasterRect(r); };
				void				readRasterPoint(BRasterPoint &p) { m_in->readRasterPoint(p); };
				void				readRasterRegion(BRasterRegion &r) { m_in->readRasterRegion(r); };
				void				readFont(BFont &f) { m_in->readFont(f); };
				coord				readCoord() { return m_in->readCoord(); };
				void				drain(int32 size) { m_in->drain(size); };

	private:

				BRenderInputPipe *	m_in;
				BRenderOutputPipe *	m_out;
};

inline void 
BRenderOutputPipe::writePoint(const BPoint &p)
{
	write(&p,sizeof(BPoint));
}

inline void 
BRenderOutputPipe::writeRasterRect(const BRasterRect &r)
{
	write((void*)&r,(int32)sizeof(BRasterRect));
}

inline void 
BRenderOutputPipe::writeRasterPoint(const BRasterPoint &p)
{
	write((void*)&p,(int32)sizeof(BRasterPoint));
}

inline void 
BRenderOutputPipe::writeRect(const BRect &p)
{
	write((void*)&p,(int32)sizeof(BRect));
}

inline void 
BRenderOutputPipe::writeRasterRegion(const BRasterRegion &r)
{
	const int32 count = r.CountRects();
	const BRasterRect *rects = r.Rects();
		
	write32(count);
	writeRasterRect(r.Bounds());
	for (int32 i = 0; i < count; i++)
		writeRasterRect(rects[i]);
}

inline void 
BRenderOutputPipe::writeCoord(coord value)
{
	write(&value,sizeof(coord));
}

inline void 
BRenderInputPipe::readRect(BRect &p)
{
	read(&p,sizeof(BRect));
}

inline void 
BRenderInputPipe::readPoint(BPoint &p)
{
	read(&p,sizeof(BPoint));
}

inline void 
BRenderInputPipe::readRasterRect(BRasterRect &r)
{
	read(&r,sizeof(BRasterRect));
}

inline void 
BRenderInputPipe::readRasterPoint(BRasterPoint &p)
{
	read(&p,sizeof(BRasterPoint));
}

inline coord 
BRenderInputPipe::readCoord()
{
	coord value;
	read(&value,sizeof(coord));
	return value;
}

} }	// namespace B::Render2

#endif	/* _RENDER2_RENDERPIPE_H_ */
