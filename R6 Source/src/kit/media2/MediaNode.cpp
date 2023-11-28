#include <media2/MediaNode.h>
#include <media2/MediaEndpoint.h>

#include <media2/IMediaCollective.h>

#include "shared_properties.h"
#include "BufferInlet.h"

#include <support2/Autolock.h>
#include <support2/Debug.h>
#include <support2/OrderedVector.h>

using B::Support2::BAutolock;

#include <support2/StdIO.h>

#define checkpoint \
berr << "thid " << find_thread(0) << ": (" << this << ") -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

namespace B {
namespace Media2 {

using namespace Private;

/**************************************************************************************/

class RMediaNode : public RInterface<IMediaNode>
{
public:
									RMediaNode(const IBinder::ptr & binder) : RInterface<IMediaNode>(binder) {}

	virtual	BString					Name() const;
	virtual	IMediaCollective::ptr	Parent() const;
	virtual	ssize_t					ListEndpoints(
										BMediaEndpointVector * outEndpoints,
										int32 type,
										int32 state) const;
	virtual	ssize_t					ListLinkedEndpoints(
										IMediaEndpoint::arg fromEndpoint,
										BMediaEndpointVector * outEndpoints,
										int32 state = B_ANY_ENDPOINT_STATE) const;

private:
	virtual	status_t				SetParent(IMediaCollective::arg parent);
};

B_IMPLEMENT_META_INTERFACE(MediaNode);

BString 
RMediaNode::Name() const
{
	return Remote()->Get(PMETHOD_NAME).AsString();
}

IMediaCollective::ptr 
RMediaNode::Parent() const
{
	return IMediaCollective::AsInterface(Remote()->Get(PMETHOD_PARENT));
}

status_t 
RMediaNode::SetParent(IMediaCollective::arg parent)
{
	return Remote()->Put(BValue(PMETHOD_PARENT, parent->AsBinder()));
}

ssize_t 
RMediaNode::ListEndpoints(BMediaEndpointVector * endpoints, int32 type, int32 state) const
{
	if (!type) return B_BAD_VALUE;
	if (!state) return B_BAD_VALUE;

	BValue request;
	request.Overlay(PMETHOD_LIST_ENDPOINTS);
	request.Overlay(PARG_ENDPOINT_TYPE, BValue::Int32(type));
	request.Overlay(PARG_ENDPOINT_STATE, BValue::Int32(state));

	BValue reply = Remote()->Get(request);
	if (reply[PMETHOD_LIST_ENDPOINTS].AsInt32() < 0)
	{
		return reply[PMETHOD_LIST_ENDPOINTS].AsInt32();
	}
	else
	{
		ssize_t ret = 0;
		if (endpoints)
		{
			BValue endpoint_v = reply[PARG_ENDPOINT];
			void * cookie = 0;
			BValue v;
			while (endpoint_v.GetNextItem(&cookie, 0, &v) == B_OK)
			{
				IMediaEndpoint::ptr e = IMediaEndpoint::AsInterface(v);
				if (e != 0)
				{
					endpoints->AddItem(e);
					++ret;
				}
			}
		}
		return ret;
	}
}

ssize_t 
RMediaNode::ListLinkedEndpoints(IMediaEndpoint::arg fromEndpoint, BMediaEndpointVector * endpoints, int32 state) const
{
	if (!state) return B_BAD_VALUE;
	if (fromEndpoint == 0) return B_BAD_VALUE;

	BValue request;
	request.Overlay(PMETHOD_LIST_LINKED_ENDPOINTS);
	request.Overlay(PARG_ENDPOINT, fromEndpoint->AsBinder());
	request.Overlay(PARG_ENDPOINT_STATE, BValue::Int32(state));

	BValue reply = Remote()->Get(request);
	if (reply[PMETHOD_LIST_LINKED_ENDPOINTS].AsInt32() < 0)
	{
		return reply[PMETHOD_LIST_LINKED_ENDPOINTS].AsInt32();
	}
	else
	{
		ssize_t ret = 0;
		if (endpoints)
		{
			BValue endpoint_v = reply[PARG_ENDPOINT];
			void * cookie = 0;
			BValue v;
			while (endpoint_v.GetNextItem(&cookie, 0, &v) == B_OK)
			{
				IMediaEndpoint::ptr e = IMediaEndpoint::AsInterface(v);
				if (e != 0)
				{
					endpoints->AddItem(e);
					++ret;
				}
			}
		}
		return ret;
	}
}

/**************************************************************************************/

status_t 
LMediaNode::Called(value & in, const value & outBindings, value & out)
{
	BValue v;
	if (in[PMETHOD_PARENT].IsDefined())
	{
		status_t err = SetParent(IMediaCollective::AsInterface(v));
		out += outBindings * BValue(PMETHOD_PARENT, BValue::Int32(err));
	}
	if (in[PMETHOD_NAME].IsDefined())
	{
		out += outBindings * BValue(PMETHOD_NAME, Name());
	}
	if (in[PMETHOD_PARENT].IsDefined())
	{
		IMediaCollective::ptr parent = Parent();
		if (parent != 0) out += outBindings * BValue(PMETHOD_PARENT, parent->AsBinder());
	}
	if (v = in[PMETHOD_LIST_ENDPOINTS])
	{
		BMediaEndpointVector endpoints;
		status_t err = ListEndpoints(
			&endpoints,
			v[PARG_ENDPOINT_TYPE].AsInteger(),
			v[PARG_ENDPOINT_STATE].AsInteger());
		BValue ret;
		if (err < B_OK)
		{
			ret.Overlay(PMETHOD_LIST_ENDPOINTS, BValue::Int32(err));
		}
		else
		{
			size_t count = endpoints.CountItems();
			for (size_t n = 0; n < count; n++)
			{
				ret.Overlay(PARG_ENDPOINT, endpoints[n]->AsBinder());
			}
		}
		out += outBindings * ret;
	}
	if (v = in[PMETHOD_LIST_LINKED_ENDPOINTS])
	{
		BMediaEndpointVector endpoints;
		status_t err = ListLinkedEndpoints(
			IMediaEndpoint::AsInterface(outBindings[PARG_ENDPOINT]),
			&endpoints,
			v[PARG_ENDPOINT_STATE].AsInteger());
		BValue ret;
		if (err < B_OK)
		{
			ret.Overlay(PMETHOD_LIST_LINKED_ENDPOINTS, BValue::Int32(err));
		}
		else
		{
			size_t count = endpoints.CountItems();
			for (size_t n = 0; n < count; n++)
			{
				ret.Overlay(PARG_ENDPOINT, endpoints[n]->AsBinder());
			}
		}
		out += outBindings * ret;
	}
	return B_OK;
}

/**************************************************************************************/


BMediaNode::BMediaNode(
	const char *name,
	size_t maxRemoteInputs,
	size_t maxRemoteBuffers,
	size_t maxRemoteBufferInfo)
	:	mName(name),
		mMaxRemoteInputs(maxRemoteInputs),
		mMaxRemoteBuffers(maxRemoteBuffers),
		mMaxRemoteBufferInfo(maxRemoteBufferInfo)
{
}

BMediaNode::~BMediaNode()
{
}

status_t
BMediaNode::Acquired(const void* id)
{
	LMediaNode::Acquired(id);
	
	mInlet = new BBufferInlet(
		mMaxRemoteInputs, mMaxRemoteBuffers, mMaxRemoteBufferInfo);
	
	return B_OK;
}

status_t
BMediaNode::Released(const void* id)
{
	for (size_t n = 0; n < mEndpoints.CountItems(); n++)
	{
		IMediaEndpoint::ptr partner = mEndpoints[n]->Partner();
		if (partner != 0)
		{
			IMediaOutput::ptr out;
			if (mEndpoints[n]->EndpointType() == B_OUTPUT_ENDPOINT)
			{
				out = IMediaOutput::AsInterface(mEndpoints[n]->AsBinder());
			}
			else
			{
				out = IMediaOutput::AsInterface(partner->AsBinder());
			}
	
			if (out != 0)
			{
//berr << "BMediaNode(" << mName << ")::Released(): disconnect from " << partner->Name() << endl;
				out->Disconnect();
			}
		}

		mEndpoints[n]->SetNode(0);
	}	
	mEndpoints.MakeEmpty();
	mParent = 0;
	mInlet = 0;
	
//berr << "releasing BMediaNode " << mName << endl;
	return LMediaNode::Released(id);
}

status_t 
BMediaNode::AddEndpoint(BMediaEndpoint::arg endpoint)
{
	if (!endpoint.ptr()) return B_BAD_VALUE;
	status_t err = endpoint->SetNode(this);
	if (err < B_OK) return err;

	BAutolock _l(mLock.Lock());
	int32 i = _FindEndpoint(endpoint);
	if (i >= 0) return B_NOT_ALLOWED;
	mEndpoints.AddItem(endpoint);
	return B_OK;
}

status_t 
BMediaNode::RemoveEndpoint(BMediaEndpoint::arg endpoint)
{
	if (!endpoint.ptr()) return B_BAD_VALUE;

	IMediaEndpoint::ptr partner = endpoint->Partner();
	if (partner != 0)
	{
		IMediaOutput::ptr out;
		if (endpoint->EndpointType() == B_OUTPUT_ENDPOINT)
		{
			out = IMediaOutput::AsInterface(endpoint->AsBinder());
		}
		else
		{
			out = IMediaOutput::AsInterface(partner->AsBinder());
		}

		if (out != 0)
		{
berr << "BMediaNode(" << mName << ")::RemoveEndpoint(): disconnect from " << partner->Name() << endl;
			out->Disconnect();
		}
	}

	status_t err = endpoint->SetNode(0);
	if (err < B_OK) return err;

	BAutolock _l(mLock.Lock());
	int32 i = _FindEndpoint(endpoint);
	if (i < 0) return B_BAD_VALUE;
	mEndpoints.RemoveItemsAt(i);
	return B_OK;
}

BMediaEndpoint::ptr
BMediaNode::EndpointAt(size_t index) const
{
	BAutolock _l(mLock.Lock());
	return (index >= mEndpoints.CountItems()) ? 0 : mEndpoints[index];
}

size_t 
BMediaNode::CountEndpoints() const
{
	BAutolock _l(mLock.Lock());
	return mEndpoints.CountItems();
}

status_t 
BMediaNode::LinkEndpoints(BMediaInput::arg input, BMediaOutput::arg output)
{
#warning * IMPLEMENT BMediaNode::LinkEndpoints(BMediaInput::arg input, BMediaOutput::arg output)
	return B_UNSUPPORTED;
}

status_t 
BMediaNode::AllocateBuffers()
{
#warning * IMPLEMENT BMediaNode::AllocateBuffers()
	return B_UNSUPPORTED;
}

BString 
BMediaNode::Name() const
{
	BAutolock _l(mLock.Lock());
	return mName;
}

IMediaCollective::ptr 
BMediaNode::Parent() const
{
	BAutolock _l(mLock.Lock());
	return mParent.promote();
}

status_t 
BMediaNode::SetParent(IMediaCollective::arg parent)
{
	BAutolock _l(mLock.Lock());
	mParent = parent;
	return B_OK;
}

ssize_t 
BMediaNode::ListEndpoints(BMediaEndpointVector *outEndpoints, int32 type, int32 state) const
{
	BAutolock _l(mLock.Lock());
	ASSERT(outEndpoints);
	const int32 c = (int32)mEndpoints.CountItems();
	ssize_t ret = 0;
	for (int32 n = 0; n < c; n++)
	{
		if ((mEndpoints[n]->EndpointType() & type) &&
			(mEndpoints[n]->EndpointState() & state))
		{
			outEndpoints->AddItem(mEndpoints[n]);
			++ret;
		}
	}
	return ret;
}

ssize_t 
BMediaNode::ListLinkedEndpoints(IMediaEndpoint::arg fromEndpoint, BMediaEndpointVector *outEndpoints, int32 state) const
{
#warning * IMPLEMENT BMediaNode::ListLinkedEndpoints(IMediaEndpoint::arg fromEndpoint, BMediaEndpointVector *outEndpoints, int32 state) const
	return B_UNSUPPORTED;
}



status_t 
BMediaNode::PropagateMessage(const BMessage &message,
								IMediaEndpoint::arg,
								media_endpoint_type direction,
								BMediaEndpointVector *visited)
{
	status_t result=B_OK;
	
	BMediaEndpointVector e;
	ssize_t n=ListEndpoints(&e,direction,B_CONNECTED_ENDPOINT);
	
	if (n<B_OK)
		return n;
		
	for (ssize_t i=0;(result>=B_OK) && i<n;++i)
	{
		IMediaEndpoint *ep=dynamic_cast<IMediaEndpoint *>(e[i].ptr());
		
		result=ep->PropagateMessage(message,direction,visited);
	}
	
	return result;
}

BMediaOutput::ptr 
BMediaNode::FilterOutputFor(BMediaInput::const_arg) const
{
	return 0;
}

BMediaInput::ptr 
BMediaNode::FilterInputFor(BMediaOutput::const_arg) const
{
	return 0;
}

status_t 
BMediaNode::AcceptBufferCount(size_t *, uint32)
{
	return B_OK;
}

status_t 
BMediaNode::AcquireBuffer(BMediaInput::arg, BBuffer *outBuffer)
{
	BMediaEndpointVector e;
	const ssize_t c=ListEndpoints(&e, B_OUTPUT_ENDPOINT);
	
	if (c<B_OK)
		return c;
		
	for (ssize_t n = 0; n < c; n++)
	{
		status_t err = e[n]->AcquireBuffer(outBuffer);
		if (err == B_OK) return err;
	}
	return B_ERROR;
}

status_t 
BMediaNode::HandleBuffer(BMediaInput::arg, BBuffer *)
{
	return B_ERROR;
}

void 
BMediaNode::ReleaseBuffers(media_buffer_group_id owner)
{
	BMediaEndpointVector e;
	const ssize_t c=ListEndpoints(&e);
	
	for (ssize_t n = 0; n < c; n++)
	{
		BMediaEndpoint * impl = dynamic_cast<BMediaEndpoint*>(e[n].ptr());
		ASSERT(impl);
		impl->ReleaseBuffers(owner);
	}
}

void 
BMediaNode::Connected(BMediaEndpoint::arg, IMediaEndpoint::arg, const BMediaFormat &)
{
}

void 
BMediaNode::Disconnected(BMediaEndpoint::arg, IMediaEndpoint::arg)
{
}

status_t 
BMediaNode::ReserveOutputConnection(BMediaOutput::arg, IMediaInput::arg, BMediaConstraint *, media_endpoint_state)
{
	return B_OK;
}

status_t 
BMediaNode::ReserveInputConnection(IMediaOutput::arg, BMediaInput::ptr *, BMediaConstraint *, media_endpoint_state)
{
	return B_OK;
}

status_t 
BMediaNode::AcceptInputConnection(IMediaOutput::arg, BMediaInput::arg, const BMediaFormat &)
{
	return B_OK;
}

status_t 
BMediaNode::AcceptOutputConnection(BMediaOutput::arg, IMediaInput::arg, const BMediaFormat &)
{
	return B_OK;
}

void 
BMediaNode::AbortInputConnection(IMediaOutput::arg, BMediaInput::arg, media_endpoint_state)
{
}

void 
BMediaNode::AbortOutputConnection(BMediaOutput::arg, IMediaInput::arg, media_endpoint_state)
{
}

int32 
BMediaNode::_FindEndpoint(BMediaEndpoint::arg endpoint)
{
	for (int32 n = (int32)mEndpoints.CountItems() - 1; n >= 0; n--) if (mEndpoints[n] == endpoint) return n;
	return -1;
}

BBufferInlet::ptr
BMediaNode::Inlet() const
{
	return mInlet;
}

} } // B::Media2
