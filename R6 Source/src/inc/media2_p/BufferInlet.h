#ifndef _MEDIA2_BUFFERINLET_PRIVATE_
#define _MEDIA2_BUFFERINLET_PRIVATE_

#include "BufferOutlet.h"
#include <OS.h>

#include <media2/MediaEndpoint.h>

#include <support2/Atom.h>

namespace B {

namespace Media2 {
	class BBuffer;
};

namespace Private {

class BBufferInlet : virtual public BAtom
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BBufferInlet);

								BBufferInlet(
									size_t maxConnections,
									size_t maxQueued,
									size_t maxInfoSize);
	virtual						~BBufferInlet();

	virtual	status_t			Acquired(const void * id);
	virtual	status_t			Released(const void * id);

			status_t			Connect(
									buffer_transport_type type,
									IBufferOutlet::arg sender,
									BMediaInput::arg destination,
									int32 * outToken = 0);
			
			status_t			Disconnect(
									int32 token);
private:
			// mStartStopLock'd:
			status_t			_StartThread();
			void				_StopThread();

	static	status_t			ThreadEntry(void * cookie);
			void				Thread();

			BLocker				mStartStopLock;
			thread_id			mThread;
			port_id				mInPort;
	const	size_t				mMaxQueued;
	const	size_t				mMaxInfoSize;
			
			BLocker				mConnectionLock;
	const	size_t				mMaxConnections;
			size_t				mConnectionCount;
	
	enum	connection_state
	{
			CONNECTION_CLOSING	= 0x1
	};	
	struct	connection
	{
			connection() : reply(-1), state(0) {}
			BMediaInput::ptr	input;
			port_id				reply;
			volatile int32		state;
	};
	enum	internal_msg
	{
			DISCO_FLUSH	= -1,
			QUIT = -2
	};
			connection *		mConnections;
			connection **		mDisconnections;
			size_t				mDiscoCount;
			size_t				mDiscoHead;
			size_t				mDiscoTail;			
};

} } // B::Private
#endif //_MEDIA2_BUFFERINLET_PRIVATE_
