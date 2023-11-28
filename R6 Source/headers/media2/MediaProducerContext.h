/***************************************************************************
//
//	File:			media2/MediaProducerContext.h
//
//	Description:	Represents a set of endpoints that operate on buffers
//					from a given source.  The root of the graph is the
//					producer output.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _MEDIA2_MEDIAPRODUCERCONTEXT_H_
#define _MEDIA2_MEDIAPRODUCERCONTEXT_H_

#include <support2/Binder.h>
#include <media2/MediaEndpoint.h>

namespace B {
namespace Media2 {

using namespace ::B::Support2;

class BMediaProducerContext : public BBinder
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BMediaProducerContext);

									BMediaProducerContext(BMediaOutput::arg producer);
	virtual							~BMediaProducerContext();

	virtual	status_t				Told(BValue & in);

			status_t				ListEndpoints(BMediaEndpointVector * outEndpoints);

			// create a buffer source: first delegate to endpoints (starting as far
			// downstream as possible) then call MakeBufferSource() if none of
			// the endpoints returned a buffer source.
	virtual	status_t				AllocateBuffers();
			IBufferSource::ptr		BufferSource() const;
			status_t				FreeBuffers();

			void					OutputConnected(IMediaOutput::arg output);
			void					OutputDisconnected(IMediaOutput::arg output);

protected:
	virtual	IBufferSource::ptr		MakeBufferSource(size_t minBufferCount, size_t minBufferCapacity);

private:
	class	impl;
			impl *	mImpl;
};

} } // namespace B::Media2
#endif //_MEDIA2_MEDIAPRODUCERCONTEXT_H_
