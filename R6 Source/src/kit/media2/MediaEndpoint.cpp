#include <media2/MediaEndpoint.h>
#include <media2/MediaNode.h>

#include <support2/Autolock.h>
#include <support2/CallStack.h>
#include <support2/Debug.h>

#include "shared_properties.h"

using B::Support2::BAutolock;

#include <support2/StdIO.h>

#define checkpoint \
berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

#define callstack \
//checkpoint 
//berr->BumpIndentLevel(1); 
//B::Support2::BCallStack cs; cs.Update(); cs.LongPrint(berr); berr << endl; 
//berr->BumpIndentLevel(-1);

namespace B {
namespace Media2 {

using namespace Private;

/**************************************************************************************/

status_t 
IMediaOutput::Reserve(IMediaInput::ptr *ioInput)
{
	BMediaConstraint constraint;
	return Reserve(ioInput, constraint);
}

status_t 
IMediaOutput::Connect(IMediaInput::ptr *ioInput, BMediaFormat *outFormat)
{
	BMediaConstraint constraint;
	return Connect(ioInput, outFormat, constraint);
}

/**************************************************************************************/

class RMediaEndpoint : public RInterface<IMediaEndpoint>
{
public:
									RMediaEndpoint(const IBinder::ptr & binder) : RInterface<IMediaEndpoint>(binder) {}

	virtual	BString					Name() const;
	virtual	IMediaNode::ptr			Node() const;
	virtual media_endpoint_type		EndpointType() const;

	virtual	media_endpoint_state	EndpointState() const;
	virtual	IMediaEndpoint::ptr		Partner() const;
	virtual	BMediaConstraint		Constraint() const;
	virtual	BMediaPreference		Preference() const;
	virtual	BMediaFormat			Format() const;
										
	virtual	status_t				AcquireBuffer(BBuffer * outBuffer);
	virtual	status_t				AllocateBuffers();

	// propagate a message along this arc until the whole chain
	// is visited or some endpoint along the chain returns an error
	virtual	status_t				PropagateMessage(
										const BMessage & message,
										media_endpoint_type direction,
										BMediaEndpointVector * visited);

protected:
	virtual	void					GetBufferConstraints(
										size_t * ioMinBufferCount,
										size_t * ioMinBufferCapacity);
	virtual	IBufferSource::ptr		MakeBufferSource(
										size_t minBufferCount,
										size_t minBufferCapacity);

	virtual void					CommitDependantTransaction();
	virtual void					CancelDependantTransaction();
};

B_IMPLEMENT_META_INTERFACE(MediaEndpoint)

BString 
RMediaEndpoint::Name() const
{
	return Remote()->Get(PMETHOD_NAME).AsString();
}

IMediaNode::ptr 
RMediaEndpoint::Node() const
{
	return IMediaNode::AsInterface(Remote()->Get(PMETHOD_NODE));
}

media_endpoint_type 
RMediaEndpoint::EndpointType() const
{
	return static_cast<media_endpoint_type>(Remote()->Get(PMETHOD_ENDPOINT_TYPE).AsInteger());
}

media_endpoint_state 
RMediaEndpoint::EndpointState() const
{
	return static_cast<media_endpoint_state>(Remote()->Get(PMETHOD_ENDPOINT_STATE).AsInteger());
}

IMediaEndpoint::ptr 
RMediaEndpoint::Partner() const
{
	return IMediaEndpoint::AsInterface(Remote()->Get(PMETHOD_PARTNER));
}

BMediaConstraint 
RMediaEndpoint::Constraint() const
{
	return BMediaConstraint(Remote()->Get(PMETHOD_CONSTRAINT));
}

BMediaPreference 
RMediaEndpoint::Preference() const
{
	return BMediaPreference(Remote()->Get(PMETHOD_PREFERENCE));
}

BMediaFormat 
RMediaEndpoint::Format() const
{
	return BMediaFormat(Remote()->Get(PMETHOD_FORMAT));
}

status_t 
RMediaEndpoint::AcquireBuffer(BBuffer * outBuffer)
{
	if (!outBuffer) return B_BAD_VALUE;
	BValue outMapping;
	outMapping.Overlay(PMETHOD_ACQUIRE_BUFFER, PMETHOD_ACQUIRE_BUFFER);
	outMapping.Overlay(PARG_BUFFER, PARG_BUFFER);
	BValue out;
	Remote()->Effect(BValue(PMETHOD_ACQUIRE_BUFFER, BValue::Bool(true)), BValue::null, outMapping, &out);
	status_t err = out[PMETHOD_ACQUIRE_BUFFER].AsInteger();
	if (err < B_OK) return err;
	BBuffer b(out[PARG_BUFFER]);
	err = b.InitCheck();
	if (err == B_OK) *outBuffer = b;
	return err;
}

status_t 
RMediaEndpoint::AllocateBuffers()
{
	return Remote()->Invoke(PMETHOD_ALLOCATE_BUFFERS).AsInt32();
}

status_t 
RMediaEndpoint::PropagateMessage(const BMessage &message,
									media_endpoint_type direction,
									BMediaEndpointVector *visited)
{
	BValue in;
	in.Overlay(PARG_VALUE, message);
	in.Overlay(PARG_ENDPOINT_TYPE, direction);
	if (visited)
	{
		BValue vlist;
		const size_t count = visited->CountItems();
		for (size_t n = 0; n < count; n++)
		{
			vlist.Overlay(BValue::Binder(visited->ItemAt(n)->AsBinder()));
		}
		in.Overlay(PARG_VISITED_ENDPOINTS, vlist);
	}
	BValue outMapping;
	outMapping.Overlay(PMETHOD_PROPAGATE, PMETHOD_PROPAGATE);
	outMapping.Overlay(PARG_VISITED_ENDPOINTS, PARG_VISITED_ENDPOINTS);
	BValue out;
	Remote()->Effect(BValue(PMETHOD_PROPAGATE, in), BValue::null, outMapping, &out);
	status_t err = out[PMETHOD_PROPAGATE].AsInteger();
	if (err >= B_OK && visited)
	{
		BValue vlist = out[PARG_VISITED_ENDPOINTS];
		BValue v;
		void * cookie = 0;
		while (vlist.GetNextItem(&cookie, 0, &v) >= B_OK)
		{
			visited->AddItem(IMediaEndpoint::AsInterface(v));
		}
	}
	return err;
}

void 
RMediaEndpoint::GetBufferConstraints(size_t *ioMinBufferCount, size_t *ioMinBufferCapacity)
{
	BValue in;
	in.Overlay(PARG_BUFFER_COUNT, BValue::Int32(*ioMinBufferCount));
	in.Overlay(PARG_BUFFER_CAPACITY, BValue::Int32(*ioMinBufferCapacity));
	BValue out = Remote()->Invoke(in, PMETHOD_GET_BUFFER_CONSTRAINTS);
	*ioMinBufferCount = (size_t)out[PARG_BUFFER_COUNT].AsInt32();
	*ioMinBufferCapacity = (size_t)out[PARG_BUFFER_CAPACITY].AsInt32();
}

IBufferSource::ptr 
RMediaEndpoint::MakeBufferSource(size_t minBufferCount, size_t minBufferCapacity)
{
	BValue in;
	in.Overlay(PARG_BUFFER_COUNT, BValue::Int32(minBufferCount));
	in.Overlay(PARG_BUFFER_CAPACITY, BValue::Int32(minBufferCapacity));
	return IBufferSource::AsInterface(Remote()->Invoke(in, PMETHOD_MAKE_BUFFER_SOURCE).AsBinder());
}

void 
RMediaEndpoint::CommitDependantTransaction()
{
	Remote()->Put(BValue(PMETHOD_COMMIT_DEPENDANT_TRANSACTION, BValue::Bool(true)));
}

void 
RMediaEndpoint::CancelDependantTransaction()
{
	Remote()->Put(BValue(PMETHOD_CANCEL_DEPENDANT_TRANSACTION, BValue::Bool(true)));
}

/**************************************************************************************/

status_t 
LMediaEndpoint::Called(BValue &in, const BValue &outBindings, BValue &out)
{
	BValue v;
	if (v = in[PMETHOD_NAME])
	{
		out += outBindings * BValue(PMETHOD_NAME, Name());
	}
	if (v = in[PMETHOD_NODE])
	{
		IMediaNode::ptr node = Node();
		if (node != 0) out += outBindings * BValue(PMETHOD_NODE, node->AsBinder());
	}
	if (v = in[PMETHOD_ENDPOINT_TYPE])
	{
		out += outBindings * BValue(PMETHOD_ENDPOINT_TYPE, EndpointType());
	}
	if (v = in[PMETHOD_ENDPOINT_STATE])
	{
		out += outBindings * BValue(PMETHOD_ENDPOINT_STATE, EndpointState());
	}
	if (v = in[PMETHOD_PARTNER])
	{
		IMediaEndpoint::ptr partner = Partner();
		if (partner != 0) out += outBindings * BValue(PMETHOD_PARTNER, partner->AsBinder());
	}
	if (v = in[PMETHOD_CONSTRAINT])
	{
		out += outBindings * BValue(PMETHOD_CONSTRAINT, Constraint().AsValue());
	}
	if (v = in[PMETHOD_PREFERENCE])
	{
		out += outBindings * BValue(PMETHOD_PREFERENCE, Preference().AsValue());
	}
	if (v = in[PMETHOD_FORMAT])
	{
		out += outBindings * BValue(PMETHOD_FORMAT, Format());
	}
	if (v = in[PMETHOD_ACQUIRE_BUFFER])
	{
		BBuffer b;
		status_t err = AcquireBuffer(&b);
		BValue ret;
		ret.Overlay(PMETHOD_ACQUIRE_BUFFER, BValue::Int32(err));
		if (err >= B_OK)
		{
			ret.Overlay(PARG_BUFFER, b.AsValue());
		}
		out += outBindings * ret;
	}
	if (v = in[PMETHOD_ALLOCATE_BUFFERS])
	{
		status_t err = AllocateBuffers();
		out += outBindings * BValue(PMETHOD_ALLOCATE_BUFFERS, BValue::Int32(err));
	}
	if (v = in[PMETHOD_PROPAGATE])
	{
		BMediaEndpointVector visited;
		BValue vlist = v[PARG_VISITED_ENDPOINTS];
		BValue ve;
		void * cookie;
		while (vlist.GetNextItem(&cookie, 0, &ve) >= B_OK)
		{
			visited.AddItem(IMediaEndpoint::AsInterface(ve));
		}
		size_t oldcount = visited.CountItems();
		status_t err = PropagateMessage(
			BMessage(v[PARG_VALUE]),
			static_cast<media_endpoint_type>(v[PARG_ENDPOINT_TYPE].AsInteger()),
			&visited);
		BValue ret;
		ret.Overlay(PMETHOD_PROPAGATE, BValue::Int32(err));
		if (err >= B_OK)
		{
			for (size_t n = oldcount; n < visited.CountItems(); n++)
			{
				vlist.Overlay(BValue::Binder(visited[n]->AsBinder()));
			}
			ret.Overlay(PARG_VISITED_ENDPOINTS, vlist);
		}
		out += outBindings * ret;
	}
	if (v = in[PMETHOD_GET_BUFFER_CONSTRAINTS])
	{
		size_t bufferCount = (size_t)in[PARG_BUFFER_COUNT].AsInt32();
		size_t bufferCapacity = (size_t)in[PARG_BUFFER_CAPACITY].AsInt32();
		GetBufferConstraints(&bufferCount, &bufferCapacity);
		BValue v;
		v.Overlay(PARG_BUFFER_COUNT, BValue::Int32(bufferCount));
		v.Overlay(PARG_BUFFER_CAPACITY, BValue::Int32(bufferCapacity));
		out += outBindings * BValue(PMETHOD_GET_BUFFER_CONSTRAINTS, v);
	}
	if (v = in[PMETHOD_MAKE_BUFFER_SOURCE])
	{
		const size_t bufferCount = (size_t)in[PARG_BUFFER_COUNT].AsInt32();
		const size_t bufferCapacity = (size_t)in[PARG_BUFFER_CAPACITY].AsInt32();
		IBufferSource::ptr s = MakeBufferSource(bufferCount, bufferCapacity);
		out += outBindings * BValue(PMETHOD_MAKE_BUFFER_SOURCE, s->AsBinder());
	}
	if (v = in[PMETHOD_COMMIT_DEPENDANT_TRANSACTION])
	{
		CommitDependantTransaction();
	}
	if (v = in[PMETHOD_CANCEL_DEPENDANT_TRANSACTION])
	{
		CancelDependantTransaction();
	}
	return B_OK;
}

/**************************************************************************************/

BMediaEndpoint::~BMediaEndpoint()
{
}

status_t 
BMediaEndpoint::SetName(const char * name)
{
	BAutolock _l(mLock.Lock());
	mName.SetTo(name);
	return B_OK;
}

status_t 
BMediaEndpoint::SetNode(BMediaNode::arg node)
{
	BMediaNode::ptr oldNode;
	bool was_attached = false;
	
	{
		BAutolock _l(mLock.Lock());
		if (mFlags & FLAG_CONNECTING) return B_NOT_ALLOWED;
		if (mState != B_FREE_ENDPOINT) return B_NOT_ALLOWED;
		if (mNode != 0) was_attached = true;
		oldNode = mNode.promote();
		mNode = node;
	}
	
	if (was_attached)
	{
		DetachedFromNode(oldNode);
		Push(BValue(detached_from_node, (oldNode != 0) ? oldNode->AsBinder() : BValue::null));
	}
	if (node.ptr())
	{
		AttachedToNode(node);
		Push(BValue(attached_to_node, node->AsBinder()));
	}

	return B_OK;
}

status_t 
BMediaEndpoint::SetBufferGroup(BBufferGroup::arg group)
{
	BAutolock _l(mLock.Lock());
	if (mBufferGroup.ptr())
	{
		// +++ propagate "release buffers" message via node
	}
	mBufferGroup = group;
	return B_OK;
}

BBufferGroup::ptr
BMediaEndpoint::BufferGroup() const
{
	BAutolock _l(mLock.Lock());
	return mBufferGroup;
}

status_t 
BMediaEndpoint::SetConstraint(const BMediaConstraint &constraint)
{
	BAutolock _l(mLock.Lock());
	mInternalConstraint = constraint;
	mConstraint = _MergeExternalConstraints(mInternalConstraint);
	return B_OK;
}

status_t 
BMediaEndpoint::SetPreference(const BMediaPreference &preference)
{
	BAutolock _l(mLock.Lock());
	mPreference = preference;
	return B_OK;
}

void 
BMediaEndpoint::GetBufferConstraints(size_t *, size_t *)
{
}

IBufferSource::ptr 
BMediaEndpoint::MakeBufferSource(size_t, size_t)
{
	return 0;
}

void 
BMediaEndpoint::Connected(IMediaEndpoint::arg, const BMediaFormat &)
{
}

void 
BMediaEndpoint::Disconnected(IMediaEndpoint::arg)
{
}

void 
BMediaEndpoint::AttachedToNode(BMediaNode::arg)
{
}

void 
BMediaEndpoint::DetachedFromNode(BMediaNode::arg)
{
}

void 
BMediaEndpoint::ReleaseBuffers(media_buffer_group_id)
{
}

status_t 
BMediaEndpoint::AddDependantConstraint(const atom_ptr<BMediaEndpoint> &target, const BMediaConstraint &constraint)
{
	// add to set of new-dependants-in-this-transaction
	mLock.Lock();
	for (int32 n = mDependants.CountItems()-1; n >= 0; n--)
	{
		if (mDependants[n].ptr() == target.ptr()) return B_NOT_ALLOWED;
	}
	for (int32 n = mNewDependants.CountItems()-1; n >= 0; n--)
	{
		if (mNewDependants[n].ptr() == target.ptr()) return B_NOT_ALLOWED;
	}
	mNewDependants.AddItem(target);
	mLock.Unlock();
	
	// hand off to dependant
	status_t err = target->BeginDependantTransaction(this, constraint);

	// remove from set if it declined, or if a > 0 result was returned
	// indicating a circular dependancy (not an error, but a situation
	// that needs to be handled explicitly.)
	if (err != B_OK)
	{
		mLock.Lock();
		for (int32 n = mNewDependants.CountItems()-1; n >= 0; n--)
		{
			if (mNewDependants[n].ptr() == target.ptr())
			{
				mNewDependants.RemoveItemsAt(n);
				break;
			}
		}
		mLock.Unlock();
	}
	return err;	
}

BString 
BMediaEndpoint::Name() const
{
	BAutolock _l(mLock.Lock());
	return mName;
}

IMediaNode::ptr 
BMediaEndpoint::Node() const
{
	return mNode.promote();
}

media_endpoint_type 
BMediaEndpoint::EndpointType() const
{
	return mType;
}

media_endpoint_state 
BMediaEndpoint::EndpointState() const
{
	BAutolock _l(mLock.Lock());
	return mState;
}

IMediaEndpoint::ptr 
BMediaEndpoint::Partner() const
{
	BAutolock _l(mLock.Lock());
	return mPartner;
}

BMediaConstraint 
BMediaEndpoint::Constraint() const
{
	BAutolock _l(mLock.Lock());
	return _Constraint();
}

BMediaPreference 
BMediaEndpoint::Preference() const
{
	BAutolock _l(mLock.Lock());
	return mPreference;
}

BMediaFormat 
BMediaEndpoint::Format() const
{
	BAutolock _l(mLock.Lock());
	return mFormat;
}

BMediaEndpoint::BMediaEndpoint(media_endpoint_type type) :
	LMediaEndpoint(),
#if DEBUG
	mLock("BMediaEndpoint::mLock"),
	mConnectLock("BMediaEndpoint::mConnectLock"),
#endif
	mState(B_FREE_ENDPOINT),
	mFlags(0),
	mDependantRefs(0),
	mType(type)
{
}

status_t
BMediaEndpoint::Acquired(const void* id)
{
	return LMediaEndpoint::Acquired(id);
}

status_t
BMediaEndpoint::Released(const void* id)
{
checkpoint
	ASSERT(mNode == 0);
	ASSERT(mPartner == 0);

	mBufferGroup = 0;
	mDependants.MakeEmpty();
	mNewDependants.MakeEmpty();
	for (int32 n = mExternalConstraints.CountItems()-1; n >= 0; n--)
	{
		external_constraint & e = mExternalConstraints.EditItemAt(n);
		delete e.constraint;
	}
	mExternalConstraints.MakeEmpty();
	
	return LMediaEndpoint::Released(id);
}

status_t 
BMediaEndpoint::BeginConnect(
	media_endpoint_state requestedState,
	BMediaConstraint * outConstraint)
{
	mLock.Lock();
	if (mDependantRefs)
	{
		// this endpoint is already involved in a dependant relationship
		mLock.Unlock();
		return B_NOT_ALLOWED;
	}
	mLock.Unlock();

	mConnectLock.Lock();
	mLock.Lock();

	status_t err = B_NOT_ALLOWED;

	switch (requestedState)
	{
		case B_FREE_ENDPOINT:
			if (mState == B_FREE_ENDPOINT) goto bail;
			break;
		case B_RESERVED_ENDPOINT:
			if (!(mNode.promote().ptr()))
			{
				goto bail;
			}	
			if (mState != B_FREE_ENDPOINT) goto bail;
			break;
		case B_CONNECTED_ENDPOINT:
			if (!(mNode.promote().ptr()))
			{
				goto bail;
			}	
			if (mState == B_CONNECTED_ENDPOINT) goto bail;
			break;
		
		default:
			TRESPASS();
			break;
	}
	
	if (outConstraint) *outConstraint = _Constraint();
	mFlags |= FLAG_CONNECTING;
	mLock.Unlock();	
	return B_OK;

bail:
	mLock.Unlock();
	mConnectLock.Unlock();
	return err;
}

void 
BMediaEndpoint::EndConnect(bool succeeded)
{
	mLock.Lock();

	BMediaEndpointVector newDependants = mNewDependants;
	if (succeeded) mDependants.AddVector(newDependants);
	mNewDependants.MakeEmpty();

	ASSERT(!mDependantRefs);
	mFlags &= ~FLAG_CONNECTING;
	
	mLock.Unlock();
	mConnectLock.Unlock();

	// finish dependant transactions	
	for (int32 n = newDependants.CountItems()-1; n >= 0; n--)
	{
		if (succeeded)
		{
			newDependants[n]->CommitDependantTransaction();
		}
		else
		{
			newDependants[n]->CancelDependantTransaction();
		}
	}
}

status_t 
BMediaEndpoint::BeginDependantTransaction(
	BMediaEndpoint::arg source,
	const BMediaConstraint &constraint)
{
	mLock.Lock();

	bool needConnectLock = false;	

	IMediaOutput::ptr connectOut;

	status_t err = B_OK;

	if (!(mNode.promote().ptr()))
	{
		mLock.Unlock();
		return B_NOT_ALLOWED;
	}
	
	// check for a circular dependancy between source and this endpoint.
	// it's not an error, but it's a no-op:
	for (int32 n = mNewDependants.CountItems()-1; n >= 0; n--)
	{
		if (mNewDependants[n].ptr() == source.ptr())
		{
			mLock.Unlock();
			return 1;
		}
	}
	
	switch (mState)
	{
		case B_FREE_ENDPOINT:
		{
			// attempt to merge the new constraint with the current published constraint
			BMediaConstraint merged(mConstraint);
			merged.And(constraint);
			err = merged.Simplify();
			if (err < B_OK)
			{
				mLock.Unlock();
				return err;
			}
			
			// push it on the external-constraint list
			external_constraint e = { 0, source, new BMediaConstraint(constraint) };
			mExternalConstraints.AddItem(e);
			
			// report the merged constraint immediately
			mConstraint = merged;
			
			// prevent others from messing with my connection state until this transaction
			// is complete
			needConnectLock = true;
			break;
		}
		case B_RESERVED_ENDPOINT:
		{
			// find participants in connection
			if (EndpointType() == B_OUTPUT_ENDPOINT)
			{
				connectOut = IMediaOutput::AsInterface(AsBinder());
				ASSERT(connectOut.ptr());
			}
			else
			{
				connectOut = IMediaOutput::AsInterface(mPartner->AsBinder());
				ASSERT(connectOut.ptr());
			}
			// add partner as a dependant
#if DEBUG
			for (int32 n = mDependants.CountItems()-1; n >= 0; n--)
			{
				ASSERT(mDependants[n] != mPartner);
			}
			for (int32 n = mNewDependants.CountItems()-1; n >= 0; n--)
			{
				ASSERT(mNewDependants[n] != mPartner);
			}
#endif
			mNewDependants.AddItem(mPartner);
			break;
		}
		case B_CONNECTED_ENDPOINT:
		{
			BMediaConstraint format(mFormat);
			format.And(constraint);
			err = format.Simplify();

			// prevent others from messing with my connection state until this
			// transaction is complete
			needConnectLock = true;
			break;
		}
		
		default:
			TRESPASS();
			break;
	}

	// only acquire mConnectLock for the first dependant transaction
	if (mDependantRefs++ > 0) needConnectLock = false;
	mLock.Unlock();
	
	if (needConnectLock)
	{
		mConnectLock.Lock();
	}
	else
	if (connectOut.ptr())
	{
		err = connectOut->DependantConnect(constraint);
		if (err < B_OK)
		{
			mLock.Lock();
			mDependantRefs--;
			mLock.Unlock();
		}
	}

	return err;
}

status_t 
BMediaEndpoint::BeginDependantConnect(BMediaConstraint * outConstraint)
{
	// +++ shouldn't we add the input to mNewDependants?
	mConnectLock.Lock();
	mLock.Lock();

	status_t err = B_NOT_ALLOWED;

	if (!(mNode.promote().ptr())) goto bail;
	if (mState != B_RESERVED_ENDPOINT) goto bail;

	mFlags |= FLAG_CONNECTING;
	mDependantRefs++;

	if (outConstraint) *outConstraint = _Constraint();
	mLock.Unlock();
	return B_OK;

bail:
	mLock.Unlock();
	mConnectLock.Unlock();
	return err;
}

void 
BMediaEndpoint::EndDependantConnect()
{
	mLock.Lock();

	ASSERT(mFlags & FLAG_CONNECTING);
	mFlags &= ~FLAG_CONNECTING;
	ASSERT(mDependantRefs > 0);
	
	mLock.Unlock();
	mConnectLock.Unlock();
}

void 
BMediaEndpoint::CommitDependantTransaction()
{
	BMediaNode::ptr node;
	IMediaEndpoint::ptr partner;
	BMediaFormat format;

	bool needConnectUnlock = false;
	mLock.Lock();
	switch (mState)
	{
		case B_FREE_ENDPOINT:
			// commit all the new external constraints
			for (int32 n = mExternalConstraints.CountItems()-1; n >= 0; n--)
			{
				external_constraint & e = mExternalConstraints.EditItemAt(n);
				if (e.committed) break;
				e.committed = 1;
			}
			needConnectUnlock = true;
			break;

		case B_RESERVED_ENDPOINT:
			// if the dependant connection succeeded, make it permanent
			// +++ THIS DOESN'T AFFECT BOTH SIDES.  FIXIT!
			if (mFlags & FLAG_DEPENDANT_CONNECTED)
			{
				mFlags &= ~(FLAG_DEPENDANT_CONNECTED);
				mState = B_CONNECTED_ENDPOINT;
				mPartner = mNewPartner;
				mFormat = mNewFormat;
				// snag a local copy of connection info to notify node
				node = mNode.promote();
				ASSERT(node.ptr());
				partner = mPartner;
				format = mFormat;
			}
			break;

		case B_CONNECTED_ENDPOINT:
			needConnectUnlock = true;
			break;
		
		default:
			TRESPASS();
			break;
	}
	
	// absorb new dependants
	BMediaEndpointVector newDependants = mNewDependants;
	mDependants.AddVector(newDependants);
	mNewDependants.MakeEmpty();
	
	// only release the connect lock if this was the topmost dependant transaction
	ASSERT(mDependantRefs > 0);
	if (--mDependantRefs > 0) needConnectUnlock = false;
	 
	mLock.Unlock();
	if (needConnectUnlock) mConnectLock.Unlock();
	
	// notify the node of a new connection, if applicable
	if (node.ptr())
	{
		Connected(partner, format);
		node->Connected(this, partner, format);
	}

	for (int32 n = newDependants.CountItems()-1; n >= 0; n--)
	{
//berr << "BMediaEndpoint::CommitDependantTransaction() '" << Name() << "' recursing to '" <<
//	newDependants[n]->Name() << "'" << endl;
		newDependants[n]->CommitDependantTransaction();
	}
}

void 
BMediaEndpoint::CancelDependantTransaction()
{
	bool needConnectUnlock = false;
	mLock.Lock();

	switch (mState)
	{
		case B_FREE_ENDPOINT:
		{
			// roll back new external constraints
			const int32 c = mExternalConstraints.CountItems();
			int32 n = c;
			for(; !mExternalConstraints[n-1].committed && n > 0; n--)
			{
				external_constraint & e = mExternalConstraints.EditItemAt(n-1);
				delete e.constraint;
			}
			if (n < c) mExternalConstraints.RemoveItemsAt(n, c - n);
			
			// merge remaining constraints
			mConstraint = _MergeExternalConstraints(mInternalConstraint);
			
			needConnectUnlock = true;
			break;
		}

		case B_RESERVED_ENDPOINT:
			mFlags &= ~FLAG_DEPENDANT_CONNECTED;
			break;

		case B_CONNECTED_ENDPOINT:
			needConnectUnlock = true;
			break;
		
		default:
			TRESPASS();
			break;
	}
	
	BMediaEndpointVector newDependants = mNewDependants;
	mNewDependants.MakeEmpty();

	// only release the connect lock if this was the topmost dependant transaction
	ASSERT(mDependantRefs > 0);
	if (--mDependantRefs > 0) needConnectUnlock = false;

	mLock.Unlock();
	if (needConnectUnlock) mConnectLock.Unlock();

	for (int32 n = newDependants.CountItems()-1; n >= 0; n--)
	{
		newDependants[n]->CancelDependantTransaction();
	}
}

void 
BMediaEndpoint::SetReserved(IMediaEndpoint::arg partner, const BMediaConstraint &reservation)
{
	ASSERT(partner.ptr());
	ASSERT(mFlags & FLAG_CONNECTING);

	{
		BAutolock _l(mLock.Lock());
		ASSERT(mState == B_FREE_ENDPOINT);
		mState = B_RESERVED_ENDPOINT;
		mPartner = partner;
		mReservation = reservation;
	}
}

void 
BMediaEndpoint::SetConnected(IMediaEndpoint::arg partner, const BMediaFormat &format, bool asDependant)
{
	ASSERT(partner.ptr());
	ASSERT(mFlags & FLAG_CONNECTING);

	if (asDependant)
	{
		BAutolock _l(mLock.Lock());
		ASSERT(mState == B_RESERVED_ENDPOINT);
		ASSERT(mDependantRefs > 0);
		mFlags |= FLAG_DEPENDANT_CONNECTED;
		mNewPartner = partner;
		mNewFormat = format;
	}
	else
	{
		BMediaNode::ptr node;

		mLock.Lock();
		ASSERT(mState == B_FREE_ENDPOINT || mState == B_RESERVED_ENDPOINT);
		node = mNode.promote();
		ASSERT(node.ptr());
		mState = B_CONNECTED_ENDPOINT;
		mPartner = partner;
		mFormat = format;
		mLock.Unlock();
		
		if (node.ptr())
		{
			Connected(partner, format);
			node->Connected(this, partner, format);
		}
	}
	
	status_t err = Push(BValue(connected, partner->AsBinder()));
}

void 
BMediaEndpoint::SetDisconnected()
{
	ASSERT(mFlags & FLAG_CONNECTING);

	BMediaNode::ptr node;
	IMediaEndpoint::ptr partner;
	
	{
		BAutolock _l(mLock.Lock());
		ASSERT(mState != B_FREE_ENDPOINT);
		node = mNode.promote();
		// node can be 0 to allow release-time disconnection
		mState = B_FREE_ENDPOINT;
		partner = mPartner;
		mPartner = 0;
		mFormat.Undefine();
	}

	Disconnected(partner);
	if (node != 0) node->Disconnected(this, partner);
	Push(BValue(disconnected, partner->AsBinder()));
}

BMediaNode::ptr
BMediaEndpoint::LocalNode() const
{
	BAutolock _l(mLock.Lock());
	return mNode.promote();
}

BMediaConstraint 
BMediaEndpoint::_MergeExternalConstraints(const BMediaConstraint & internal)
{
	BMediaConstraint out = internal;
	for (int32 n = mExternalConstraints.CountItems()-1; n >= 0; n--)
	{
		out.And(*mExternalConstraints[n].constraint);
	}
	return out;
}

BMediaConstraint 
BMediaEndpoint::_Constraint() const
{
	switch (mState)
	{
		case B_FREE_ENDPOINT:
			return mConstraint;
		
		case B_RESERVED_ENDPOINT:
			return mReservation;
			
		case B_CONNECTED_ENDPOINT:
			return BMediaConstraint(mFormat);
		
		default:
			debugger("BMediaEndpoint::Constraint(): invalid state");
			return BMediaConstraint(false);
	}
}


} } // B::Media2
