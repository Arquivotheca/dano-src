
#ifndef	_SUPPORT2_STREAMPIPE_H_
#define	_SUPPORT2_STREAMPIPE_H_

#include <support2/IByteStream.h>
#include <support2/SupportDefs.h>
#include <support2/BufferedPipe.h>
#include <kernel/OS.h>

namespace B {
namespace Support2 {

class BStreamPipe : virtual public BBufferedPipe
{
	public:
							BStreamPipe(size_t bufferSize);
		virtual				~BStreamPipe();

	protected:
	
				uint8 *		Buffer(int32 assertSize=0);
				int32		BufferSize();

	private:
	
				uint8 *		m_buffer;
				int32		m_bufferSize;
};

class BStreamInputPipe : public BStreamPipe
{
	public:
							BStreamInputPipe(IByteInput::arg stream, size_t bufferSize);
		virtual				~BStreamInputPipe();

		virtual	void		RenewBuffer(uint8 *&buffer, int32 &size, int32 &filled);

	private:

		IByteInput::ptr		m_stream;
};

class BStreamOutputPipe : public BStreamPipe
{
	public:
							BStreamOutputPipe(IByteOutput::arg stream, size_t bufferSize);
		virtual				~BStreamOutputPipe();

		virtual	void		RenewBuffer(uint8 *&buffer, int32 &size, int32 &filled);

	protected:

							BStreamOutputPipe(IByteOutput *stream, size_t bufferSize);

	private:
	
		IByteOutput *		m_stream;
};

} }	// namespace B::Support2

#endif	/* _SUPPORT2_STREAMPIPE_H_ */
