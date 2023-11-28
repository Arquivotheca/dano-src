
#ifndef	_SUPPORT2_BUFFEREDPIPE_H_
#define	_SUPPORT2_BUFFEREDPIPE_H_

#include <support2/SupportDefs.h>
#include <support2/IBinder.h>

namespace B {
namespace Support2 {

class BParcel;

class BBufferedPipe
{
	public:
							BBufferedPipe();
		virtual				~BBufferedPipe();

		inline	void *		Reserve(int32 size, int32 &outSize);
		inline	void *		Assert(int32 size);
		inline	void		Renew();
		
				status_t	Status();

	protected:

				void		SetStatus(status_t err);
				size_t		Position() { return m_pos; }

	private:

		virtual	void		RenewBuffer(uint8 *&buffer, int32 &size, int32 &filled) = 0;
		virtual	void		FreeBuffer(uint8 *buffer, int32 size);

				status_t	m_error;
				uint8 *		m_buffer;
				int32		m_size;
				int32		m_pos;
};

class BBufferedInputPipe : virtual public BBufferedPipe
{
	public:
							BBufferedInputPipe();
		virtual				~BBufferedInputPipe();

				void		read(void *buffer, int32 size);
				int64		read64();
				int32		read32();
				int16		read16();
				int8		read8();
				double		readf64();
				float		readf32();
				void		readstr(char *);
				BString		readstr();
				void		drain(int32 size);
};

class BBufferedOutputPipe : virtual public BBufferedPipe
{
	public:
							BBufferedOutputPipe();
		virtual				~BBufferedOutputPipe();

				void		write(const void *buffer, int32 size);
				void		write64(int64 value);
				void		write32(int32 value);
				void		write16(int16 value);
				void		write8(int8 value);
				void		writef64(double value);
				void		writef32(float value);
				void		writestr(const char *);
				void		writebinder(const IBinder::ptr& binder, BParcel *offset);
				void		flush();
};

inline void 
BBufferedPipe::Renew()
{
	if (!m_buffer || m_pos) {
		if (Status() != B_OK) m_pos = 0;
		else RenewBuffer(m_buffer,m_size,m_pos);
	}
}

inline void *
BBufferedPipe::Reserve(int32 size, int32 &outSize)
{
	if ((m_pos + size) > m_size) size = m_size - m_pos;
	outSize = size;
	m_pos += size;
	return m_buffer+m_pos-size;
}

inline void *
BBufferedPipe::Assert(int32 size)
{
	if ((m_pos + size) > m_size) {
		Renew();
		if ((m_pos + size) > m_size) return NULL;
	}
	m_pos += size;
	return m_buffer+m_pos-size;
}

} }	// namespace B::Support2

#endif	/* _SUPPORT2_BUFFEREDPIPE_H_ */
