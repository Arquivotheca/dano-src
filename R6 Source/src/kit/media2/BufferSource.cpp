
#include <media2/IBufferSource.h>
#include "shared_properties.h"

namespace B {
namespace Media2 {

using namespace Private;

class RBufferSource : public RInterface<IBufferSource>
{
public:
						RBufferSource(const IBinder::ptr & binder) :
							RInterface<IBufferSource>(binder) {}

	virtual	status_t	AcquireBuffer(
							BBuffer * outBuffer,
							int32 id = BBuffer::ANY_BUFFER,
							bigtime_t timeout = B_INFINITE_TIMEOUT);

	virtual ssize_t		ListBuffers(
							BVector<BBuffer> * outBuffers);
};

B_IMPLEMENT_META_INTERFACE(BufferSource);

status_t 
RBufferSource::AcquireBuffer(BBuffer *outBuffer, int32 id, bigtime_t timeout)
{
	BValue request;
	request.Overlay(PMETHOD_ACQUIRE_BUFFER);
	request.Overlay(PARG_BUFFER_ID, BValue::Int32(id));
	request.Overlay(PARG_TIMEOUT, BValue::Int32(timeout));	
	BValue ret = Remote()->Invoke(request);
	status_t err = ret[PMETHOD_ACQUIRE_BUFFER].AsInt32();
	if (err >= B_OK) *outBuffer = BBuffer(ret[PARG_BUFFER]);
	return err;
}

ssize_t 
RBufferSource::ListBuffers(BVector<BBuffer> *outBuffers)
{
	if (!outBuffers) return B_BAD_VALUE;
	BValue ret = Remote()->Invoke(PMETHOD_LIST_BUFFERS);
	ssize_t count = ret[PMETHOD_LIST_BUFFERS].AsInt32();
	if (count > 0)
	{
		BValue buffers = ret[PARG_BUFFER];
		for (size_t n = 0; n < (size_t)count; n++)
		{
			outBuffers->AddItem(BBuffer(ret[BValue::Int32(n)]));
		}
	}
	return count;
}

status_t 
LBufferSource::Called(BValue &in, const BValue &outBindings, BValue &out)
{
	BValue v;
	if (v = in[PMETHOD_ACQUIRE_BUFFER])
	{
		BBuffer buffer;
		status_t err = AcquireBuffer(
			&buffer,
			v[PARG_BUFFER_ID].AsInt32(),
			v[PARG_TIMEOUT].AsTime());
		BValue ret;
		ret.Overlay(PMETHOD_ACQUIRE_BUFFER, BValue::Int32(err));
		if (err >= B_OK)
		{
			ret.Overlay(PARG_BUFFER, buffer.AsValue());
		}
		out += outBindings * ret;
	}
	if (v = in[PMETHOD_LIST_BUFFERS])
	{
		BVector<BBuffer> buffers;
		ssize_t count = ListBuffers(&buffers);
		BValue ret;
		ret.Overlay(BValue(PMETHOD_LIST_BUFFERS, BValue::Int32(count)));
		if (count > 0)
		{
			BValue v_buffers;
			for (size_t n = 0; n < (size_t)count; n++)
			{
				v_buffers.Overlay(BValue::Int32(n), buffers[n].AsValue());
			}
			ret.Overlay(BValue(PARG_BUFFER, v_buffers));
		}
		out += outBindings * ret;
	}
	return B_OK;
}

} } // B::Media2
