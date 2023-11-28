
#ifndef	_SUPPORT2_PORTPIPE_H_
#define	_SUPPORT2_PORTPIPE_H_

#include <support2/SupportDefs.h>
#include <support2/BufferedPipe.h>
#include <kernel/OS.h>

namespace B {
namespace Support2 {

class BPortPipe : virtual public BBufferedPipe
{
	public:
							BPortPipe(port_id port, int32 id);
		virtual				~BPortPipe();

	protected:
	
				port_id		Port();
				int32		ID();
				uint8 *		Buffer(int32 assertSize = 0);

	private:
	
				port_id		m_port;
				int32		m_id;
				uint8 *		m_portBuffer;
				int32		m_portBufferSize;
};

class BPortInputPipe : public BPortPipe
{
	public:
							BPortInputPipe(port_id inputPort, int32 id);
		virtual				~BPortInputPipe();

		virtual	void		RenewBuffer(uint8 *&buffer, int32 &size, int32 &filled);
};

class BPortOutputPipe : public BPortPipe
{
	public:
							BPortOutputPipe(port_id outputPort, int32 id, int32 bufferSize);
		virtual				~BPortOutputPipe();

		virtual	void		RenewBuffer(uint8 *&buffer, int32 &size, int32 &filled);

	private:
	
				int32		m_bufferSize;
};

} }	// namespace B::Support2

#endif	/* _SUPPORT2_PORTPIPE_H_ */
