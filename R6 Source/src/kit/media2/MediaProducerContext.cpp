#include <media2/MediaProducerContext.h>
#include <media2/MediaEndpointGraph.h>
#include "shared_properties.h"

namespace B {
namespace Media2 {

//using namespace B::Private;

struct BMediaProducerContext::impl
{
	BMediaEndpointGraph		graph;
};

BMediaProducerContext::BMediaProducerContext(BMediaOutput::arg producer)
{
}

BMediaProducerContext::~BMediaProducerContext()
{
}

status_t 
BMediaProducerContext::Told(BValue &in)
{
	BValue v;
	if (v = in[IMediaEndpoint::connected])
	{
		// +++
	}
	if (v = in[IMediaEndpoint::disconnected])
	{
		// +++
	}
	if (v = in[IMediaEndpoint::allocate_buffers])
	{
		// +++
	}
	return B_OK;
}

status_t 
BMediaProducerContext::ListEndpoints(BMediaEndpointVector * outEndpoints)
{
#warning * IMPLEMENT BMediaProducerContext::ListEndpoints
	return B_UNSUPPORTED;
}

status_t 
BMediaProducerContext::AllocateBuffers()
{
#warning * IMPLEMENT BMediaProducerContext::AllocateBuffers
	return B_UNSUPPORTED;
}

IBufferSource::ptr 
BMediaProducerContext::BufferSource() const
{
#warning * IMPLEMENT BMediaProducerContext::BufferSource
	return 0;
}

status_t 
BMediaProducerContext::FreeBuffers()
{
#warning * IMPLEMENT BMediaProducerContext::FreeBuffers
	return B_UNSUPPORTED;
}

void 
BMediaProducerContext::OutputConnected(IMediaOutput::arg output)
{
#warning * IMPLEMENT BMediaProducerContext::OutputConnected
}

void 
BMediaProducerContext::OutputDisconnected(IMediaOutput::arg output)
{
#warning * IMPLEMENT BMediaProducerContext::OutputDisconnected
}

IBufferSource::ptr 
BMediaProducerContext::MakeBufferSource(size_t minBufferCount, size_t minBufferCapacity)
{
#warning * IMPLEMENT BMediaProducerContext::MakeBufferSource
	return 0;
}

} } // B::Media2
