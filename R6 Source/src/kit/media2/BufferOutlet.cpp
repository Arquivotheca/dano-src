#include "BufferOutlet.h"
#include "shared_properties.h"

#include <support2/CallStack.h>
#include <support2/Debug.h>
#include <support2/StdIO.h>

#define checkpoint \
berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

#define callstack \
checkpoint \
berr->BumpIndentLevel(1); \
B::Support2::BCallStack cs; cs.Update(); cs.LongPrint(berr); berr << endl; \
berr->BumpIndentLevel(-1);

namespace B {
namespace Private {

using namespace Media2;

/**************************************************************************************/

class RBufferOutlet : public RInterface<IBufferOutlet>
{
public:
								RBufferOutlet(const IBinder::ptr & binder) :
									RInterface<IBufferOutlet>(binder) {}

	virtual	status_t			AcceptConnect(
									buffer_transport_type type,
									IMediaInput::arg input,
									const BValue & info);

	virtual	status_t			AcceptDisconnect();
};

B_IMPLEMENT_META_INTERFACE(BufferOutlet);

status_t 
RBufferOutlet::AcceptConnect(
	buffer_transport_type type,
	IMediaInput::arg input,
	const BValue &info)
{
	BValue in;
	in.Overlay(PARG_TRANSPORT, BValue::Int32(type));
	in.Overlay(PARG_INPUT, BValue::Binder(input->AsBinder()));
	in.Overlay(PARG_VALUE, info);
	return Remote()->Invoke(in, PMETHOD_ACCEPT_CONNECT).AsInt32();
}

status_t 
RBufferOutlet::AcceptDisconnect()
{
	return Remote()->Put(PMETHOD_ACCEPT_DISCONNECT);
}


/**************************************************************************************/

status_t 
LBufferOutlet::Called(BValue & in, const BValue & outBindings, BValue & out)
{
	BValue v;
	if (v = in[PMETHOD_ACCEPT_CONNECT])
	{
		status_t err = AcceptConnect(
			(buffer_transport_type)v[PARG_TRANSPORT].AsInt32(),
			IMediaInput::AsInterface(v[PARG_INPUT].AsBinder()),
			v[PARG_VALUE]);
		out += outBindings * BValue(PMETHOD_ACCEPT_CONNECT, BValue::Int32(err));
	}
	if (v = in[PMETHOD_ACCEPT_DISCONNECT])
	{
		status_t err = AcceptDisconnect();
		out += outBindings * BValue(PMETHOD_ACCEPT_DISCONNECT, BValue::Int32(err));
	}
	return B_OK;
}

/**************************************************************************************/

class BBufferOutlet::transport_impl
{
public:
	transport_impl() : capacity(0), outlet(-1), reply(-1), buffer(0) {}
	
	IMediaInput::ptr	input;
	size_t				capacity;
	size_t				info_size;
	int32				token;
	port_id				outlet;
	port_id				reply;
	int8 *				buffer;
};

BBufferOutlet::BBufferOutlet() :
	mImpl(new transport_impl)
{
}

BBufferOutlet::~BBufferOutlet()
{
}

status_t 
BBufferOutlet::Acquired(const void *id)
{
callstack
	return BAtom::Acquired(id);
}

status_t 
BBufferOutlet::Released(const void *id)
{
	delete mImpl;
	mImpl = 0;
	return BAtom::Released(id);
}

status_t 
BBufferOutlet::SendBuffer(BBuffer * buffer)
{
	status_t err;
	const port_id outlet = mImpl->outlet;
	const port_id reply = mImpl->reply;

	size_t write_count = sizeof(flat_buffer);
	buffer->Flatten((flat_buffer*)mImpl->buffer);
	
	const BValue & info = buffer->Info();
	if (info)
	{
		ssize_t info_size = info.FlattenedSize();
		if (info_size < 0)
		{
			return info_size;
		}
		else if (info_size > (ssize_t)mImpl->info_size)
		{
			return B_NO_MEMORY;
		}
		err = info.Flatten(mImpl->buffer + write_count, mImpl->info_size);
		if (err < B_OK)
		{
			return err;
		}
		write_count += err;
	}

	((flat_buffer*)mImpl->buffer)->token = mImpl->token;
	err = write_port(outlet, BUFFER_TRANSPORT_QUEUE, mImpl->buffer, write_count);
	if (err < B_OK) return err;
	int32 code;
	err = read_port(reply, &code, 0, 0);
	if (err < B_OK) return err;
	return code;
}

status_t 
BBufferOutlet::AcquireBuffer(BBuffer *)
{
#warning * IMPLEMENT BBufferOutlet::AcquireBuffer(BBuffer *outBuffer);
	return B_UNSUPPORTED;
}


status_t 
BBufferOutlet::AcceptConnect(buffer_transport_type type, IMediaInput::arg input, const BValue & info)
{
	if (type != B_MEDIA_QUEUEING_TRANSPORT) return B_UNSUPPORTED;

	int32 token = info["TOKN"].AsInt32();
	size_t capacity = (size_t)info["BUFS"].AsInt32();
	size_t info_size = (size_t)info["INFS"].AsInt32();
	port_id outlet = info["QUEP"].AsInt32();
	port_id reply = info["REPP"].AsInt32();
	if (!capacity || outlet < 0 || reply < 0) return B_BAD_VALUE;

	mLock.Lock();
	if (mImpl->input != 0)
	{
		mLock.Unlock();
		return B_NOT_ALLOWED;
	}
	mImpl->token = token;
	mImpl->input = input;
	mImpl->capacity = capacity;
	mImpl->info_size = info_size;
	mImpl->outlet = outlet;
	mImpl->reply = reply;
	mImpl->buffer = new int8[sizeof(flat_buffer) + info_size];
	mLock.Unlock();
	
	return B_OK;
}

status_t 
BBufferOutlet::AcceptDisconnect()
{
	mLock.Lock();
	if (mImpl->input == 0)
	{
		mLock.Unlock();
		return B_NOT_ALLOWED;
	}
	mImpl->input = 0;
	mImpl->capacity = 0;
	mImpl->info_size = 0;
	mImpl->outlet = -1;
	mImpl->reply = -1;
	mLock.Unlock();
	
	return B_OK;
}

} } // B::Private
