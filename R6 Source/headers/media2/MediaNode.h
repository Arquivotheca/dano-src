/***************************************************************************
//
//	File:			media2/MediaNode.h
//
//	Description:	A BMediaNode aggregates one or more endpoints into
//					a functional group, with which one can implement a
//					buffer producer, filter, consumer, or any combination
//					thereof.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _MEDIA2_MEDIANODE_H_
#define _MEDIA2_MEDIANODE_H_

#include <support2/Binder.h>
#include <support2/String.h>

#include <media2/MediaDefs.h>
#include <media2/IMediaNode.h>
#include <media2/IMediaCollective.h>
#include <media2/MediaEndpoint.h>

namespace B {
namespace Media2 {

using namespace Support2;

class LMediaNode : public LInterface<IMediaNode>
{
public:
	virtual	status_t				Called(BValue & in, const BValue & outBindings, BValue & out);
};

class BMediaNode : public LMediaNode
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BMediaNode);

									BMediaNode(
										const char * name,
										size_t maxRemoteInputs		= 16,
										size_t maxRemoteBuffers 	= 128,
										size_t maxRemoteBufferInfo 	= 1024);
	
	virtual	status_t				AddEndpoint(BMediaEndpoint::arg endpoint);
	virtual	status_t				RemoveEndpoint(BMediaEndpoint::arg endpoint);
			BMediaEndpoint::ptr		EndpointAt(size_t index) const;
			size_t					CountEndpoints() const;
			
			// call LinkEndpoints() to indicate that 'input' may operate in place
			// and retransmit its buffers via 'output'.
	virtual	status_t				LinkEndpoints(
										BMediaInput::arg input,
										BMediaOutput::arg output);

			// [re-]allocate buffers via every BMediaProducerContext that
			// intersects this node.
	virtual	status_t				AllocateBuffers();

	// ** IMediaNode

	virtual	BString					Name() const;
	virtual	IMediaCollective::ptr	Parent() const;
	virtual	ssize_t					ListEndpoints(
										BMediaEndpointVector * outEndpoints,
										int32 type = B_ANY_ENDPOINT_TYPE,
										int32 state = B_ANY_ENDPOINT_STATE) const;
	virtual	ssize_t					ListLinkedEndpoints(
										IMediaEndpoint::arg fromEndpoint,
										BMediaEndpointVector * outEndpoints,
										int32 state = B_ANY_ENDPOINT_STATE) const;
													
protected:
	virtual							~BMediaNode();
	
	// ** BUFFER-PROCESSING HOOKS

	// base impl: return 0
	virtual	BMediaOutput::ptr		FilterOutputFor(
										BMediaInput::const_arg input) const;

	// base impl: return 0
	virtual	BMediaInput::ptr		FilterInputFor(
										BMediaOutput::const_arg output) const;

	// base impl: no-op, return B_OK
	virtual	status_t	AcceptBufferCount(	size_t * ioMinimum,
											uint32 streamingFlags = 0);

	// base impl: relay request to each output, return B_ERROR when exhausted
	virtual	status_t				AcquireBuffer(
										BMediaInput::arg forInput,
										BBuffer * outBuffer);
	
	// base impl: error
	virtual	status_t				HandleBuffer(
										BMediaInput::arg receiver,
										BBuffer * buffer);

	// base impl: relay request to each endpoint
	virtual	void					ReleaseBuffers(media_buffer_group_id owner);

	// ** NOTIFICATION

	// a connection between the given endpoints has been completed.
	virtual	void					Connected(
										BMediaEndpoint::arg localEndpoint,
										IMediaEndpoint::arg remoteEndpoint,
										const BMediaFormat & format);

	// a reservation or connection between the given endpoints has been broken.
	virtual	void					Disconnected(
										BMediaEndpoint::arg localEndpoint,
										IMediaEndpoint::arg remoteEndpoint);

	// ** EXTENDED CONNECTION HOOKS
	//    You can safely ignore these.
	
	// base impl: no-op, return B_OK
	// at this point, constraint has been validated against localOutput's default format (if any.)
	// return B_OK to continue the connection process.
	virtual	status_t				ReserveOutputConnection(
										BMediaOutput::arg output,
										IMediaInput::arg remoteInput,
										BMediaConstraint * constraint,
										media_endpoint_state requestedState);

	// base impl: no-op, return B_OK
	// the given remote output wishes to form a connection; the given constraints are valid from
	// its point of view and have been resolved against the input's default format (if any.)
	// you may point ioInput at a new or pre-existing input if you want the original input to
	// act as an always-free placeholder.  if you do that, ReserveInputConnection() will be called
	// again with the input you specify.
	// return B_OK to continue the connection process.
	virtual	status_t				ReserveInputConnection(
										IMediaOutput::arg remoteOutput,
										BMediaInput::ptr * ioInput,
										BMediaConstraint * constraint,
										media_endpoint_state requestedState);

	// base impl: no-op, return B_OK
	// a format has been negotiated for the given connection.  if you need to allocate
	// shared system or hardware resources, now is a good time to do so; if you return
	// an error the connection will be cancelled.
	// return B_OK to continue the connection process.
	virtual	status_t				AcceptInputConnection(
										IMediaOutput::arg remoteOutput,
										BMediaInput::arg input,
										const BMediaFormat & format);

	// base impl: no-op
	// at this point the remote input and its node have accepted the given format.
	// if you need to allocate shared system or hardware resources, now is a good time to do so;
	// if you return an error the connection will be cancelled.
	// return B_OK to continue the connection process.
	virtual	status_t				AcceptOutputConnection(
										BMediaOutput::arg output,
										IMediaInput::arg remoteInput,
										const BMediaFormat & format);

	// base impl: no-op
	// the given reservation or connection failed.
	virtual	void					AbortInputConnection(
										IMediaOutput::arg remoteOutput,
										BMediaInput::arg input,
										media_endpoint_state deniedState);

	// base impl: no-op
	// the given reservation or connection failed.
	virtual	void					AbortOutputConnection(
										BMediaOutput::arg output,
										IMediaInput::arg remoteInput,
										media_endpoint_state deniedState);

	// message hooks
	
	// base impl: propagate message along all connected endpoints in
	// the direction specified
	virtual status_t	PropagateMessage (const BMessage &message,
											IMediaEndpoint::arg from,
											media_endpoint_type direction,
											BMediaEndpointVector *visited);

	// ** BAtom

	virtual	status_t				Acquired(const void* id);
	virtual	status_t				Released(const void* id);

private:
	friend	class BMediaEndpoint;
	friend	class BMediaOutput;
	friend	class BMediaInput;

	virtual	status_t				SetParent(IMediaCollective::arg parent);

			int32					_FindEndpoint(BMediaEndpoint::arg endpoint);
			atom_ptr<B::Private::BBufferInlet> Inlet() const;

	mutable	BLocker					mLock;
			BString					mName;

			BMediaLocalEndpointVector	mEndpoints;
			IMediaCollective::ref		mParent;
			
			atom_ptr<B::Private::BBufferInlet>	mInlet;
			size_t								mMaxRemoteInputs;
			size_t								mMaxRemoteBuffers;
			size_t								mMaxRemoteBufferInfo;
};

}; }; // namespace B::Media2
#endif //_MEDIA2_MEDIANODE_H_
