
#ifndef	_RENDER2_RENDERINPUTPIPE_H_
#define	_RENDER2_RENDERINPUTPIPE_H_

#include <support2_p/BinderKeys.h>
#include <render2_p/RenderProtocol.h>

#include <support2/SupportDefs.h>
#include <support2/Binder.h>
#include <support2/Parcel.h>
#include <support2/StdIO.h>
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


class BRenderInputPipe
{
public:
							BRenderInputPipe();
	virtual					~BRenderInputPipe();

	virtual		status_t	Receive(	uint32 code,
										BParcel& data,
										BParcel* reply = NULL,
										uint32 flags = 0);

	inline		void		PreloadIfAvaillable();
	inline		status_t	readOp(uint8 *opcode, uint32 *count);
	inline		status_t 	lookOp(uint8 *opcode);
	inline		void		rewindOp();
				
	template<typename T>	inline	ssize_t read(T* const value);
									ssize_t readptr(uint8 const ** const pvalue, size_t size);

private:
	inline		status_t	assert_dat(size_t s);
	inline		status_t	assert_cmd();
	inline		size_t		size_dat();
	inline		size_t		size_cmd();
	inline		size_t		size_cmd_ahead();
				status_t	next_dat();
				status_t	next_cmd();

	BVector<BParcel*>		m_opcodes;
	BVector<BParcel*>		m_data;
	const uint8				*m_cmdBuffer;
	const uint8				*m_cmdPtr;
	const uint8				*m_cmdLookAheadPtr;
	const uint8				*m_dataBuffer;
	const uint8				*m_dataPtr;
	size_t					m_datSize;
	size_t					m_cmdSize;
	int32					m_lookAheadIndex;
};

// *****************************************************************

inline size_t BRenderInputPipe::size_dat() {
	return (m_dataBuffer + m_datSize - m_dataPtr);	
}
inline size_t BRenderInputPipe::size_cmd() {
	return (m_cmdBuffer + m_cmdSize - m_cmdPtr);	
}
inline size_t BRenderInputPipe::size_cmd_ahead() {
	return (static_cast<const uint8 *>(m_opcodes.ItemAt(m_lookAheadIndex)->Data())
			+ m_opcodes.ItemAt(m_lookAheadIndex)->Length()
			- m_cmdLookAheadPtr);	
}

inline status_t	BRenderInputPipe::assert_dat(size_t size)
{
	const size_t s = size_dat();
	if (s >= size)
		return B_OK;
	if (s != 0) {	// Sanity check
		debugger("BRenderInputPipe::read() protocol error");
	}	
	return next_dat();
}

inline status_t	BRenderInputPipe::assert_cmd()
{
	if (size_cmd())
		return B_OK;
	return next_cmd();
}

inline void BRenderInputPipe::PreloadIfAvaillable() {
	assert_cmd();
	assert_dat(1);
}

// *****************************************************************

inline status_t BRenderInputPipe::readOp(uint8 *opcode, uint32 *count)
{
	if (assert_cmd() != B_OK) {
		*opcode = GRP_NOP;
		*count = 0;
		return B_ERROR;
	}
	int c = 0;
	uint8 op = *m_cmdPtr++;
	if (op & GRP_RLE) {
		op &= ~GRP_RLE;
		c = *m_cmdPtr++;
	}
	*opcode = op;
	*count = c+1;
	return B_OK;
}

inline status_t BRenderInputPipe::lookOp(uint8 *opcode)
{
	if (size_cmd_ahead() == 0) {
		if (m_opcodes.CountItems() >= (uint32)m_lookAheadIndex+2) {
			m_lookAheadIndex++;
			m_cmdLookAheadPtr = static_cast<const uint8 *>(m_opcodes.ItemAt(m_lookAheadIndex)->Data());
		} else {
			*opcode = GRP_NOP;
			return B_ERROR;
		}
	}
	uint8 op = *m_cmdLookAheadPtr++;
	if (op & GRP_RLE) {
		op &= ~GRP_RLE;
		m_cmdLookAheadPtr++;
	}
	*opcode = op;
	return B_OK;
}

inline void BRenderInputPipe::rewindOp()
{
	m_lookAheadIndex = 0;
	m_cmdLookAheadPtr = m_cmdPtr;
}

// *****************************************************************

template<typename T>
inline ssize_t BRenderInputPipe::read(T* const value)
{
	// Fixed size types always fit in the buffer
	if (assert_dat(sizeof(T)) != B_OK)
		return B_ERROR;	
	*value = *reinterpret_cast<const T *>(m_dataPtr);
	m_dataPtr += sizeof(T);
	return B_OK;
}

template<>
inline ssize_t BRenderInputPipe::read<IBinder::ptr>(IBinder::ptr* const value)
{
	// Fixed size types always fit in the buffer
	if (assert_dat(4) != B_OK)
		return B_ERROR;	
	const size_t size = *reinterpret_cast<const int32*>(m_dataPtr);
	if (assert_dat(size) != B_OK)
		return B_ERROR;	
	BValue v;
	v.Unflatten(m_dataPtr+4, size);
	m_dataPtr += (size+4);
	*value = v.AsBinder();
	return B_OK;
}

template<>
inline ssize_t BRenderInputPipe::read<B2dTransform>(B2dTransform* const value)
{
	// Fixed size types always fit in the buffer
	B2dTransform& t = *value;
	if (assert_dat(t.FlattenedSize()) != B_OK)
		return B_ERROR;
	t.Unflatten(t.TypeCode(), m_dataPtr, t.FlattenedSize());
	m_dataPtr += t.FlattenedSize();
	return B_OK;
}

template <>
inline ssize_t BRenderInputPipe::read<BFont>(BFont* const value)
{
	#warning implement read(BFont)
	return B_UNSUPPORTED;
}

template <>
inline ssize_t BRenderInputPipe::read<BColorTransform>(BColorTransform* const value)
{
	#warning implement read(BColorTransform)
	return B_UNSUPPORTED;
}

} }	// namespace B::Render2

#endif	/* _RENDER2_RENDERINPUTPIPE_H_ */
