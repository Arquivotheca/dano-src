#include <media2/MediaCollective.h>

#include "shared_properties.h"

#include <support2/Autolock.h>
#include <support2/Debug.h>
#include <support2/StdIO.h>

#define checkpoint \
berr << "thid " << find_thread(0) << " BMediaCollective(" << this << ") -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

using B::Support2::BAutolock;

namespace B {
namespace Media2 {

using namespace Private;

/**************************************************************************************/

class RMediaCollective : public RInterface<IMediaCollective>
{
public:
									RMediaCollective(const IBinder::ptr & binder) : RInterface<IMediaCollective>(binder) {}
	virtual	ssize_t					ListNodes(BMediaNodeVector * outNodes) const;
};

B_IMPLEMENT_META_INTERFACE(MediaCollective);

ssize_t 
RMediaCollective::ListNodes(BMediaNodeVector * outNodes) const
{
	if (!outNodes) return B_BAD_VALUE;
	BValue reply = Remote()->Get(PMETHOD_LIST_NODES);
	if (reply[PMETHOD_LIST_NODES].AsInt32() < 0)
	{
		return reply[PMETHOD_LIST_NODES].AsInt32();
	}
	else
	{
		ssize_t ret = 0;
		if (outNodes)
		{
			BValue node_v = reply[PARG_NODE];
			void * cookie = 0;
			BValue v;
			while (node_v.GetNextItem(&cookie, 0, &v) >= B_OK)
			{
				IMediaNode::ptr p = IMediaNode::AsInterface(v);
				if (p == 0)
				{
					return v.AsInteger();
				}
				outNodes->AddItem(p);
				++ret;
			}
		}
		return ret;
	}
}

/**************************************************************************************/

status_t 
LMediaCollective::Called(BValue & in, const BValue & outBindings, BValue & out)
{
	if (in[PMETHOD_LIST_NODES].IsDefined())
	{
		BMediaNodeVector nodes;
		status_t err = ListNodes(&nodes);
		BValue ret;
		ret.Overlay(PMETHOD_LIST_NODES, BValue::Int32(err));
		if (err >= B_OK)
		{
			const size_t count = nodes.CountItems();
			for (size_t n = 0; n < count; n++)
			{
				ret.Overlay(PARG_NODE, nodes[n]->AsBinder());
			}
		}
		out += outBindings * ret;
	}
	return B_OK;
}

/**************************************************************************************/

BMediaCollective::BMediaCollective()
{
}

BMediaCollective::BMediaCollective(const char *name) :
	mName(name)
{
}

BMediaCollective::~BMediaCollective()
{
}

BValue
BMediaCollective::Inspect(const BValue & v, uint32 flags)
{
	return LMediaNode::Inspect(v,flags) + LMediaCollective::Inspect(v,flags);
}

void 
BMediaCollective::SetName(const char *name)
{
	BAutolock _l(mLock.Lock());
	mName.SetTo(name);
}

status_t 
BMediaCollective::AddNode(IMediaNode::arg node)
{
	BAutolock _l(mLock.Lock());
	int32 i = _FindNode(node);
	if (i >= 0) return B_NOT_ALLOWED;
	status_t err = node->SetParent(IMediaCollective::ptr(this));
	if (err < 0) return err;
	mNodes.AddItem(node);
	// +++ notify
	return B_OK;
}

status_t 
BMediaCollective::RemoveNode(IMediaNode::arg node)
{
	BAutolock _l(mLock.Lock());
	int32 i = _FindNode(node);
	if (i < 0) return B_BAD_VALUE;
	mNodes.RemoveItemsAt(i);
	// +++ notify
	return B_OK;
}

IMediaNode::ptr 
BMediaCollective::NodeAt(int32 index) const
{
	BAutolock _l(mLock.Lock());
	return (index < 0 || index >= (int32)mNodes.CountItems()) ? IMediaNode::ptr() : mNodes[index];
}

int32 
BMediaCollective::IndexOf(IMediaNode::arg node)
{
	BAutolock _l(mLock.Lock());
	return _FindNode(node);
}

int32 
BMediaCollective::CountNodes() const
{
	BAutolock _l(mLock.Lock());
	return (int32)mNodes.CountItems();
}

status_t 
BMediaCollective::ShowEndpoint(IMediaEndpoint::arg endpoint)
{
	BAutolock _l(mLock.Lock());
	if (_FindNode(endpoint->Node()) < 0) return B_NOT_ALLOWED;
	if (_FindVisible(endpoint) < 0) mVisible.AddItem(endpoint);
	// +++ notify
	return B_OK;
}

status_t 
BMediaCollective::HideEndpoint(IMediaEndpoint::arg endpoint)
{
	BAutolock _l(mLock.Lock());
	if (_FindNode(endpoint->Node()) < 0) return B_NOT_ALLOWED;
	int32 n = _FindVisible(endpoint);
	if (n >= 0) mVisible.RemoveItemsAt(n);
	// +++ notify
	return B_OK;
}

bool 
BMediaCollective::IsVisible(IMediaEndpoint::arg endpoint)
{
	BAutolock _l(mLock.Lock());
	return (_FindVisible(endpoint) >= 0);
}

ssize_t 
BMediaCollective::ListNodes(BMediaNodeVector *outNodes) const
{
	BAutolock _l(mLock.Lock());
	ASSERT(outNodes);
	outNodes->AddVector(mNodes);
	return mNodes.CountItems();
}

BString 
BMediaCollective::Name() const
{
	BAutolock _l(mLock.Lock());
	return mName;
}

IMediaCollective::ptr 
BMediaCollective::Parent() const
{
	BAutolock _l(mLock.Lock());
	return mParent.promote();
}

status_t 
BMediaCollective::SetParent(IMediaCollective::arg parent)
{
	BAutolock _l(mLock.Lock());
	mParent = parent;
	return B_OK;
}

status_t 
BMediaCollective::ListEndpoints(BMediaEndpointVector *outEndpoints, int32 type,
								int32 state) const
{
	BAutolock _l(mLock.Lock());
	ASSERT(outEndpoints);
	const int32 c = (int32)mVisible.CountItems();
	ssize_t ret = 0;
	for (int32 n = 0; n < c; n++)
	{
		if ((mVisible[n]->EndpointType() & type)
			&&(mVisible[n]->EndpointState() & state))
		{
			outEndpoints->AddItem(mVisible[n]);
			++ret;
		}
	}
	return ret;
}

ssize_t 
BMediaCollective::ListLinkedEndpoints(
	IMediaEndpoint::arg /*fromEndpoint*/,
	BMediaEndpointVector */*outEndpoints*/,
	int32 /*state*/) const
{
#warning * IMPLEMENT BMediaCollective::ListLinkedEndpoints()
	return B_UNSUPPORTED;
}


status_t
BMediaCollective::Acquired(const void* id)
{
	LMediaCollective::Acquired(id);
	LMediaNode::Acquired(id);
	
	return B_OK;
}

status_t
BMediaCollective::Released(const void* id)
{
	mNodes.MakeEmpty();
	mVisible.MakeEmpty();
	mParent = 0;

	LMediaNode::Released(id);
	LMediaCollective::Released(id);
	return B_OK;
}

int32 
BMediaCollective::_FindNode(IMediaNode::arg node)
{
	for (int32 n = (int32)mNodes.CountItems() - 1; n >= 0; n--) if (mNodes[n] == node) return n;
	return -1;
}

int32 
BMediaCollective::_FindVisible(IMediaEndpoint::arg endpoint)
{
	for (int32 n = (int32)mVisible.CountItems() - 1; n >= 0; n--) if (mVisible[n] == endpoint) return n;
	return -1;
}

} } // B::Media2
