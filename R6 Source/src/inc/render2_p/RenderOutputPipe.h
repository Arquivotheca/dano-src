
#ifndef	_RENDER2_RENDEROUTPUTPIPE_H_
#define	_RENDER2_RENDEROUTPUTPIPE_H_

#include <support2_p/BinderKeys.h>
#include <support2/SupportDefs.h>

#include <support2/Binder.h>
#include <support2/Parcel.h>
#include <render2/RenderDefs.h>
#include <raster2/RasterRect.h>
#include <raster2/RasterRegion.h>

#include <render2/Color.h>
#include <render2/Point.h>
#include <render2/Rect.h>
#include <render2/2dTransform.h>

namespace B {
namespace Render2 {

using namespace Support2;
using namespace Raster2;


class BRenderOutputPipe
{
public:
							BRenderOutputPipe(const IBinder::ptr& remote, size_t data_size, size_t cmd_size);
	virtual					~BRenderOutputPipe();
	inline		IBinder*	Remote() const	{ return m_remote.ptr(); }

				status_t	InitCheck() const;
				status_t	Close();

				void		writeOp(uint8 op);
				void		flush_cmd();
				void		flush_dat();
				void		assert_size(size_t s) { assert_dat(s); }

		template<typename T>
		inline	void write(const T& value);
				void write(const uint8 *buffer, size_t length);
				void write_align(const uint8 *buffer, size_t length);
	
private:
				size_t		size_cmd() { return (m_cmdBuffer + m_cmdSize - m_cmdPtr); }
				size_t		size_dat() { return (m_dataBuffer + m_datSize - m_dataPtr); }
	inline		void		assert_cmd(size_t size);
	inline		void		assert_dat(size_t size);

	BParcel				m_offsets;
	size_t				m_cmdSize;
	size_t				m_datSize;
	uint8				*m_cmdBuffer;
	uint8				*m_dataBuffer;
	uint8				*m_cmdPtr;
	uint8				*m_dataPtr;
	uint32				m_count;
	uint8				m_opcode;
	IBinder::ptr		m_remote;
	bool				m_isOpen;
};

inline void BRenderOutputPipe::assert_cmd(size_t size) {
	if (size_cmd() < size)
		flush_cmd();
}
inline void BRenderOutputPipe::assert_dat(size_t size) {
	if (size_dat() < size)
		flush_dat();
}

template<typename T>
inline void BRenderOutputPipe::write(const T& value)
{
	assert_dat(sizeof(T));
	*reinterpret_cast<T *>(m_dataPtr) = value;
	m_dataPtr += sizeof(T);
}

template<>
inline void BRenderOutputPipe::write<IBinder::ptr>(const IBinder::ptr& binder)
{
	BValue v(binder);
	const size_t size = v.FlattenedSize();
	assert_dat(size + 4);
	m_offsets.SetBase((m_dataPtr+4) - m_dataBuffer);
	*reinterpret_cast<int32*>(m_dataPtr) = size;
	v.Flatten(m_dataPtr+4, size, &m_offsets);
	m_offsets.AddBinder(binder);
	m_dataPtr += (size+4);
}

template <>
inline void BRenderOutputPipe::write<B2dTransform>(const B2dTransform& t)
{
	assert_dat(t.FlattenedSize());
	t.Flatten(m_dataPtr, t.FlattenedSize());
	m_dataPtr += t.FlattenedSize();
}

template <>
inline void BRenderOutputPipe::write<BFont>(const BFont&)
{
#warning implement write(BFont)
}

template <>
inline void BRenderOutputPipe::write<BColorTransform>(const BColorTransform&)
{
#warning implement write(BColorTransform)
}

template <>
inline void BRenderOutputPipe::write<BGradient>(const BGradient&)
{
#warning implement write(BGradient)
}


} }	// namespace B::Render2

#endif	/* _RENDER2_RENDEROUTPUTPIPE_H_ */
