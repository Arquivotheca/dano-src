/******************************************************************************
/
/	File:			BufferIO.h
/
/	Description:	A buffered adapter for BPositionIO
/
/	Copyright 1998, Be Incorporated
/
******************************************************************************/

#ifndef	_SUPPORT2_BUFFERIO_H
#define	_SUPPORT2_BUFFERIO_H

#include <support2/ByteStream.h>

namespace B {
namespace Support2 {

class BBufferIO : public LByteInput, public LByteOutput, public LByteSeekable
{
	enum {
		DEFAULT_BUF_SIZE = 65536L
	};

	public:
								BBufferIO(
									IByteInput::arg inStream,
									IByteOutput::arg outStream,
									IByteSeekable::arg seeker,
									size_t buf_size = DEFAULT_BUF_SIZE
								);
		virtual					~BBufferIO();
		
		virtual	ssize_t			ReadV(const struct iovec *vector, ssize_t count);
		virtual	ssize_t			WriteV(const struct iovec *vector, ssize_t count);
		virtual	status_t		End();
		virtual	status_t		Sync();
		
		virtual off_t			Seek(off_t position, uint32 seek_mode);
		virtual	off_t			Position() const;

				// XXX remove in favor of Sync()??
				status_t		Flush();

	protected:

								BBufferIO(size_t buf_size = DEFAULT_BUF_SIZE);

	private:

				IByteInput *	m_in;
				IByteOutput *	m_out;
				IByteSeekable *	m_seeker;
				off_t			m_buffer_start;
				char * 			m_buffer;
				size_t 			m_buffer_phys;
				size_t 			m_buffer_used;
				off_t 			m_seek_pos;			// _reserved_ints[0-1]
				off_t 			m_len;				// _reserved_ints[2-3]
				uint32 			_reserved_ints[2];	// was 6.
				bool 			m_buffer_dirty;
				bool 			m_owns_stream;
				bool 			_reserved_bools[6];
};

} } // namespace B::Support2

#endif /* _SUPPORT2_BUFFERIO_H */
