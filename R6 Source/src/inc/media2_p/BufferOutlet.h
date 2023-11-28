#ifndef _MEDIA2_BUFFERTRANSPORT_PRIVATE_
#define _MEDIA2_BUFFERTRANSPORT_PRIVATE_

#include <support2/IInterface.h>
#include <support2/IBinder.h>
#include <support2/Locker.h>

#include <media2/MediaDefs.h>
#include <media2/IMediaEndpoint.h>
#include <media2/IBufferSource.h>

namespace B {
namespace Private {

using namespace Support2;
using namespace Media2;

struct flat_buffer
{
	int32			token;
	area_id			area;
	size_t			offset;
	size_t			size;
	int32			id;
	union
	{
		area_id		list_area;
		sem_id		ring_sem;
	}				owner;
	int32			_reserved[2];
};

enum buffer_transport_msg
{
	// send buffer.  data: flat_buffer [info: flattened BValue]
	BUFFER_TRANSPORT_QUEUE,

	// acquire buffer.  sends flat_buffer, flattened BValue to reply port
	BUFFER_TRANSPORT_ACQUIRE
};

class IBufferOutlet : public IInterface
{
public:
	B_DECLARE_META_INTERFACE(BufferOutlet);

	virtual	status_t			AcceptConnect(
									buffer_transport_type type,
									IMediaInput::arg input,
									const BValue & info) = 0;

	virtual	status_t			AcceptDisconnect() = 0;	
};

class LBufferOutlet : public LInterface<IBufferOutlet>
{
public:
	virtual	status_t			Called(BValue & in, const BValue & outBindings, BValue & out);
};

class BBufferOutlet : public LBufferOutlet
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BBufferOutlet);

								BBufferOutlet();
	virtual						~BBufferOutlet();

	virtual	status_t			Acquired(const void * id);
	virtual	status_t			Released(const void * id);

	virtual	status_t			SendBuffer(BBuffer * buffer);
	
	virtual	status_t			AcquireBuffer(BBuffer * outBuffer);

	// ** IBufferOutlet

	virtual	status_t			AcceptConnect(
									buffer_transport_type type,
									IMediaInput::arg input,
									const BValue & info);

	virtual	status_t			AcceptDisconnect();
	
private:
			BLocker				mLock;			
			class transport_impl;
			transport_impl *	mImpl;
};

} } // namespace B::Private
#endif // _MEDIA2_BUFFERTRANSPORT_PRIVATE_
