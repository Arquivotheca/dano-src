#include <media2/MediaEndpoint.h>
#include <media2/MediaNode.h>
#include <media2/MediaProducerContext.h>

#include "shared_properties.h"
#include "BufferOutlet.h"

#include <support2/Autolock.h>
#include <support2/CallStack.h>
#include <support2/Debug.h>

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

class RMediaOutput : public RInterface<IMediaOutput>
{
public:
									RMediaOutput(const IBinder::ptr & binder) : RInterface<IMediaOutput>(binder) {}

	virtual	status_t				Reserve(
										IMediaInput::ptr * ioInput,
										const BMediaConstraint & constraint);
	virtual	status_t				Connect(
										IMediaInput::ptr * ioInput, BMediaFormat * outFormat,
										const BMediaConstraint & constraint);
	virtual	status_t				Disconnect();

private:
	virtual	status_t				AcceptFormat(const IMediaInput::ptr & input, const BMediaFormat & format);

	virtual	status_t				DependantConnect(const BMediaConstraint & constraint);
#if 0
	virtual	void					AddedToContext(const atom_ptr<IMediaProducerContext> & context);
	virtual	void					RemovedFromContext(const atom_ptr<IMediaProducerContext> & context);
#endif
};

B_IMPLEMENT_META_INTERFACE(MediaOutput);

status_t 
RMediaOutput::Reserve(IMediaInput::ptr * ioInput, const BMediaConstraint & constraint)
{
	if (!ioInput) return B_BAD_VALUE;
	BValue in;
	in.Overlay(PARG_INPUT, BValue::Binder((*ioInput)->AsBinder()));
	in.Overlay(PARG_CONSTRAINT, constraint.AsValue());
	BValue outBindings;
	outBindings.Overlay(PMETHOD_RESERVE, PMETHOD_RESERVE);
	outBindings.Overlay(PARG_INPUT, PARG_INPUT);
	BValue out;
	Remote()->Effect(BValue(PMETHOD_RESERVE, in), BValue::null, outBindings, &out);
	status_t err = out[PMETHOD_RESERVE].AsInteger();
	if (err < B_OK) return err;
	*ioInput = IMediaInput::AsInterface(out[PARG_INPUT]);
	return B_OK;
}

status_t 
RMediaOutput::Connect(IMediaInput::ptr * ioInput, BMediaFormat * outFormat, const BMediaConstraint & constraint)
{
	if (!ioInput) return B_BAD_VALUE;
	if (!outFormat) return B_BAD_VALUE;
	BValue in;
	in.Overlay(PARG_INPUT, BValue::Binder((*ioInput)->AsBinder()));
	in.Overlay(PARG_CONSTRAINT, constraint.AsValue());
	BValue outBindings;
	outBindings.Overlay(PMETHOD_CONNECT, PMETHOD_CONNECT);
	outBindings.Overlay(PARG_INPUT, PARG_INPUT);
	outBindings.Overlay(PARG_FORMAT, PARG_FORMAT);
	BValue out;
	Remote()->Effect(BValue(PMETHOD_CONNECT, in), BValue::null, outBindings, &out);
	status_t err = out[PMETHOD_CONNECT].AsInteger();
	if (err < B_OK) return err;
	*ioInput = IMediaInput::AsInterface(out[PARG_INPUT]);
	*outFormat = BMediaFormat(out[PARG_FORMAT]);
	return B_OK;
}

status_t 
RMediaOutput::Disconnect()
{
	return Remote()->Get(PMETHOD_DISCONNECT).AsInteger();
}

status_t 
RMediaOutput::AcceptFormat(IMediaInput::arg input, const BMediaFormat & format)
{
	BValue in;
	in.Overlay(PARG_INPUT, BValue::Binder(input->AsBinder()));
	in.Overlay(PARG_FORMAT, format);
	return Remote()->Invoke(in, PMETHOD_ACCEPT_FORMAT).AsInteger();
}

status_t 
RMediaOutput::DependantConnect(const BMediaConstraint & constraint)
{
	return Remote()->Invoke(constraint.AsValue(), PMETHOD_DEPENDANT_CONNECT).AsInt32();
}

#if 0
void 
RMediaOutput::AddedToContext(IMediaProducerContext::arg context)
{
	Remote()->Invoke(context->AsBinder(), PMETHOD_ADDED_TO_CONTEXT);
}

void 
RMediaOutput::RemovedFromContext(IMediaProducerContext::arg context)
{
	Remote()->Invoke(context->AsBinder(), PMETHOD_REMOVED_FROM_CONTEXT);
}
#endif
/**************************************************************************************/

status_t 
LMediaOutput::Called(BValue & in, const BValue & outBindings, BValue & out)
{
	BValue v;
	if (v = in[PMETHOD_RESERVE])
	{
		IMediaInput::ptr ioInput = IMediaInput::AsInterface(v[PARG_INPUT]);
		status_t err = Reserve(
			&ioInput,
			BMediaConstraint(v[PARG_CONSTRAINT]));
		BValue ret;
		ret.Overlay(PMETHOD_RESERVE, BValue::Int32(err));
		if (err >= B_OK)
		{
			ret.Overlay(PARG_INPUT, ioInput->AsBinder());
		}
		out += outBindings * ret;
	}
	if (v = in[PMETHOD_CONNECT])
	{
		IMediaInput::ptr ioInput = IMediaInput::AsInterface(v[PARG_INPUT]);
		BMediaFormat format;
		status_t err = Connect(
			&ioInput,
			&format, 
			BMediaConstraint(v[PARG_CONSTRAINT]));
		BValue ret;
		ret.Overlay(PMETHOD_CONNECT, BValue::Int32(err));
		if (err >= B_OK)
		{
			ret.Overlay(PARG_INPUT, ioInput->AsBinder());
			ret.Overlay(PARG_FORMAT, format);
		}
		out += outBindings * ret;
	}
	if (v = in[PMETHOD_DISCONNECT])
	{
		status_t err = Disconnect();
		out += outBindings * BValue(PMETHOD_DISCONNECT, BValue::Int32(err));
	}
	if (v = in[PMETHOD_ACCEPT_FORMAT])
	{
		status_t err = AcceptFormat(
			IMediaInput::AsInterface(v[PARG_INPUT]),
			BMediaFormat(v[PARG_FORMAT]));
		out += outBindings * BValue(PMETHOD_ACCEPT_FORMAT, BValue::Int32(err));
	}
	if (v = in[PMETHOD_DEPENDANT_CONNECT])
	{
		status_t err = DependantConnect(BMediaConstraint(v));
		out += outBindings * BValue(PMETHOD_ACCEPT_FORMAT, BValue::Int32(err));
	}
#if 0
	if (v = in[PMETHOD_ADDED_TO_CONTEXT])
	{
		AddedToContext(IMediaProducerContext::AsInterface(v));
	}
	if (v = in[PMETHOD_REMOVED_FROM_CONTEXT])
	{
		RemovedFromContext(IMediaProducerContext::AsInterface(v));
	}
#endif
	return B_OK;
}

/**************************************************************************************/

BMediaOutput::BMediaOutput(const char * name) :
	BMediaEndpoint(B_OUTPUT_ENDPOINT)
{
	if (name) SetName(name);
}

BMediaOutput::~BMediaOutput()
{
}

BValue 
BMediaOutput::Inspect(const BValue & v, uint32 flags)
{
	return BMediaEndpoint::Inspect(v, flags) + LMediaOutput::Inspect(v, flags);
}

status_t 
BMediaOutput::SendBuffer(BBuffer *buffer)
{
	IMediaInput::ptr input;
	BBufferOutlet::ptr outlet;
	mLock.Lock();
	input = mDestination;
	outlet = mOutlet;
	mLock.Unlock();
	if (outlet != 0)
	{
		return outlet->SendBuffer(buffer);
	}
	else
	{
		if (!input.ptr()) return B_NOT_ALLOWED;
		return input->HandleBuffer(buffer);
	}
}

status_t 
BMediaOutput::Reserve(IMediaInput::ptr *ioInput)
{
	BMediaConstraint constraint;
	return Reserve(ioInput, constraint);
}

status_t 
BMediaOutput::Connect(IMediaInput::ptr *ioInput, BMediaFormat *outFormat)
{
	BMediaConstraint constraint;
	return Connect(ioInput, outFormat, constraint);
}

status_t 
BMediaOutput::Reserve(IMediaInput::ptr * ioInput, const BMediaConstraint & inConstraint)
{
	BMediaNode::ptr node = LocalNode();
	if (node == 0) return B_NOT_ALLOWED;

	IMediaInput::ptr destination = *ioInput;
	ASSERT(destination.ptr());

	BMediaConstraint constraint;
	status_t err = BeginConnect(B_RESERVED_ENDPOINT, &constraint);
	if (err < B_OK) return err;

//	berr << "output: src constraint " << inConstraint << endl;
//	berr << "output: internal constraint " << constraint << endl;

	constraint.And(inConstraint);
	err = constraint.Simplify();

	if (err < B_OK)
	{
		// +++ we need some specific errors
		EndConnect();
		return B_ERROR;
	}
	
	// notify the node of a pending connection
	err = node->ReserveOutputConnection(this, destination, &constraint, B_RESERVED_ENDPOINT);
	if (err < B_OK)
	{
		EndConnect();
		return err;
	}
	
	// hand off to destination
	IMediaEndpoint::ptr reservedEndpoint;
	IMediaInput::ptr reservedInput;
	err = destination->AcceptReserve(
		IMediaEndpoint::ptr(this), IMediaOutput::ptr(this), constraint,
		&reservedEndpoint, &reservedInput);
	if (err < B_OK)
	{
		node->AbortOutputConnection(this, destination, B_RESERVED_ENDPOINT);
		EndConnect();
		return err;
	}
	SetReserved(reservedEndpoint, constraint);
	*ioInput = reservedInput;
	EndConnect(true);
	return B_OK;		
}

status_t 
BMediaOutput::Connect(IMediaInput::ptr * ioInput, BMediaFormat * outFormat, const BMediaConstraint & inConstraint)
{
	BMediaNode::ptr node = LocalNode();
	if (node == 0) return B_NOT_ALLOWED;

	IMediaInput::ptr destination = *ioInput;
	ASSERT(destination.ptr());

	BMediaConstraint constraint;
	status_t err = BeginConnect(B_CONNECTED_ENDPOINT, &constraint);
	if (err < B_OK) return err;
	
	media_endpoint_state state = EndpointState();
	IMediaEndpoint::ptr partner = Partner();

	if (state == B_RESERVED_ENDPOINT)
	{
		IMediaInput::ptr reservedInput = IMediaInput::AsInterface(partner->AsBinder());
		if (reservedInput != *ioInput)
		{
			// sorry, you're not allowed to change your mind after you've
			// reserved a connection.
			EndConnect();
			return B_BAD_VALUE;
		}
	}

	// test internal constraint
//	berr << "output: src constraint " << inConstraint << endl;
//	berr << "output: internal constraint " << constraint << endl;

	constraint.And(inConstraint);
	err = constraint.Simplify();

	if (err < B_OK)
	{
//		berr << "## constraint result " << err << " : " << constraint << endl;

		// +++ we need some specific errors
		EndConnect();
		return B_ERROR;
	}
	
	// notify the node of a pending connection
	err = node->ReserveOutputConnection(this, destination, &constraint, B_CONNECTED_ENDPOINT);
	if (err < B_OK)
	{
		EndConnect();
		return err;
	}
	
	// hand off to destination
	IMediaEndpoint::ptr connectedEndpoint;
	IMediaInput::ptr connectedInput;

	BBufferOutlet::ptr outlet = 0;
	if (dynamic_cast<RInterface<IMediaInput>*>(destination.ptr()))
	{
		outlet = new BBufferOutlet();
	}

	err = destination->AcceptConnect(
		IMediaEndpoint::ptr(this), IMediaOutput::ptr(this), outlet,
		constraint, Preference(),
		&connectedEndpoint, &connectedInput, outFormat);
	if (err < B_OK)
	{
		node->AbortOutputConnection(this, destination, B_CONNECTED_ENDPOINT);
		EndConnect();
		return err;
	}

	mLock.Lock();
	mDestination = connectedInput;
	mOutlet = outlet;
	// +++ if this output is a producer (ie. not internally linked to an input on the same node)
	// then create a BMediaProducerContext
	mLock.Unlock();

	SetConnected(connectedEndpoint, *outFormat);
	*ioInput = connectedInput;
	EndConnect(true);
	return B_OK;		
}

status_t 
BMediaOutput::Disconnect()
{
	status_t err = BeginConnect(B_FREE_ENDPOINT);
	if (err < B_OK)
	{
berr << "BMediaOutput::Disconnect() -- BeginConnect(): " << strerror(err) << endl;
		return err;
	}
	
	IMediaInput::ptr input;
	mLock.Lock();
	input = mDestination;
	mLock.Unlock();

	mLock.Lock();
	mDestination = 0;
	mOutlet = 0;
	// +++ if this output is a producer (ie. not internally linked to an input on the same node)
	// then delete the BMediaProducerContext
	// +++
	mLock.Unlock();

	SetDisconnected();

	if (input.ptr())
	{
		err = input->AcceptDisconnect();
		if (err < B_OK)
		{
berr << "BMediaOutput::Disconnect('" << Name() << "'): input->AcceptDisconnect(): "
	 << strerror(err) << endl;
		}
	}
	
	EndConnect(true);
	return B_OK;
}

status_t 
BMediaOutput::AcquireBuffer(BBuffer *outBuffer)
{
	// defer to buffer group, if we have one
	BBufferGroup::ptr group = BufferGroup();
	if (group.ptr()) return group->AcquireBuffer(outBuffer);

	// pass request downstream
	IMediaEndpoint::ptr partner = Partner();
	if (partner.ptr())
	{
		status_t err = partner->AcquireBuffer(outBuffer);
		return err;
	}
	return B_ERROR;
}

status_t 
BMediaOutput::AllocateBuffers()
{
#warning * IMPLEMENT BMediaOutput::AllocateBuffers()
	return B_UNSUPPORTED;
}

status_t
BMediaOutput::Acquired(const void* id)
{
	BMediaEndpoint::Acquired(id);
	LMediaOutput::Acquired(id);
	
	return B_OK;
}

status_t
BMediaOutput::Released(const void* id)
{
	mDestination = 0;
	mOutlet = 0;
	mContext = 0;
	LMediaOutput::Released(id);
	return BMediaEndpoint::Released(id); // +++ pay attention to both return values?
}

status_t 
BMediaOutput::AcceptFormat(const IMediaInput::ptr & input, const BMediaFormat & format)
{
	// hand off to node
	BMediaNode::ptr node = LocalNode();
	if (node == 0) return B_NOT_ALLOWED;
	return node->AcceptOutputConnection(this, input, format);
}

status_t 
BMediaOutput::DependantConnect(const BMediaConstraint &inConstraint)
{
	status_t err;
	BMediaNode::ptr node = LocalNode();
	if (node == 0) return B_NOT_ALLOWED;

	BMediaConstraint constraint;
	err = BeginDependantConnect(&constraint);
	if (err < B_OK) return err;

	media_endpoint_state state = EndpointState();
	if (state != B_RESERVED_ENDPOINT) return B_ERROR;
	
	IMediaInput::ptr input = IMediaInput::AsInterface(Partner()->AsBinder());
	if (!input.ptr())
	{
		EndDependantConnect();
		return B_ERROR;
	}
	
	// test internal constraint
	berr << "output/dep: src constraint " << inConstraint << endl;
	berr << "output/dep: internal constraint " << constraint << endl;

	constraint.And(inConstraint);
	err = constraint.Simplify();

	if (err < B_OK)
	{
		berr << "## constraint result " << err << " : " << constraint << endl;

		// +++ we need some specific errors
		EndDependantConnect();
		return B_ERROR;
	}
	
	// hand off to destination
	BBufferOutlet::ptr outlet = 0;
	if (dynamic_cast<RInterface<IMediaInput>*>(input.ptr()))
	{
		outlet = new BBufferOutlet();
	}

	BMediaFormat format;
	err = input->AcceptDependantConnect(outlet, constraint, Preference(), &format);
	if (err < B_OK)
	{
		node->AbortOutputConnection(this, input, B_CONNECTED_ENDPOINT);
		EndDependantConnect();
		return err;
	}

	mLock.Lock();
	mDestination = input;
	mOutlet = outlet;
	mLock.Unlock();

	SetConnected(IMediaEndpoint::AsInterface(input->AsBinder()), format, true);
	EndDependantConnect();
	return B_OK;
}

void 
BMediaOutput::AbortDependantConnect()
{
	mLock.Lock();
	mDestination = 0;
	mOutlet = 0;
	mLock.Unlock();
}

#if 0
void 
BMediaOutput::AddedToContext(IMediaProducerContext::arg graph)
{
	BAutolock _l(mLock.Lock());
	for (size_t n = 0; n < mUpstreamContexts.CountItems(); n++)
	{
		if (graph->AsBinder() == mUpstreamContexts[n]->AsBinder())
		{
			// already got that one, thanks
			return;
		}
	}
	mUpstreamContexts.AddItem(graph);
}

void 
BMediaOutput::RemovedFromContext(IMediaProducerContext::arg graph)
{
	BAutolock _l(mLock.Lock());
	for (size_t n = 0; n < mUpstreamContexts.CountItems(); n++)
	{
		if (graph->AsBinder() == mUpstreamContexts[n]->AsBinder())
		{
			mUpstreamContexts.RemoveItemsAt(n, 1);
			return;
		}
	}
}
#endif

status_t 
BMediaOutput::PropagateMessage(const BMessage &message,
								media_endpoint_type direction,
								BMediaEndpointVector *visited)
{
	if (direction==B_INPUT_ENDPOINT)
	{
		atom_ptr<BMediaNode> node = LocalNode();
		if (node == 0) return B_NOT_ALLOWED;
		
		if (visited)
			visited->AddItem(this);
		
		return node->PropagateMessage(message,this,direction,visited);
	}
	else
	{
		if (EndpointState()!=B_CONNECTED_ENDPOINT)
			return B_NOT_ALLOWED;
		
		return Partner()->PropagateMessage(message,direction,visited);
	}
}

} } // B::Media2
