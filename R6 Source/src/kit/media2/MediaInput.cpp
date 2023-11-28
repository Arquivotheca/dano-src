#include <media2/MediaEndpoint.h>
#include <media2/MediaNode.h>
#include "resolve_format.h"
#include "shared_properties.h"
#include "BufferOutlet.h"
#include "BufferInlet.h"

#include <support2/Autolock.h>
#include <support2/Debug.h>

using B::Support2::BAutolock;

#include <support2/StdIO.h>

#define checkpoint \
berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

namespace B {
namespace Media2 {

using namespace Private;

/**************************************************************************************/

class RMediaInput : public RInterface<IMediaInput>
{
public:
									RMediaInput(const IBinder::ptr & binder) : RInterface<IMediaInput>(binder) {}

	virtual	status_t				HandleBuffer(
										BBuffer * buffer);

protected:
	
	virtual	status_t				AcceptReserve(
										IMediaEndpoint::arg sourceEndpoint,
										IMediaOutput::arg sourceOutput,
										const BMediaConstraint & constraint,
										IMediaEndpoint::ptr * outReservedEndpoint,
										IMediaInput::ptr * outReservedInput);
			
	virtual	status_t				AcceptConnect(
										IMediaEndpoint::arg sourceEndpoint,
										IMediaOutput::arg sourceOutput,
										IBufferOutlet::arg transport,
										const BMediaConstraint & constraint,
										const BMediaPreference & sourcePreference,
										IMediaEndpoint::ptr * outConnectedEndpoint,
										IMediaInput::ptr * outConnectedInput,
										BMediaFormat * outFormat);
			
	virtual	status_t				AcceptDependantConnect(
										IBufferOutlet::arg transport,
										const BMediaConstraint & constraint,
										const BMediaPreference & sourcePreference,
										BMediaFormat * outFormat);

	virtual	status_t				AcceptDisconnect();
};

B_IMPLEMENT_META_INTERFACE(MediaInput);

status_t 
RMediaInput::HandleBuffer(BBuffer * buffer)
{
	if (!buffer) return B_BAD_VALUE; 
	return Remote()->Invoke(buffer->AsValue(), PMETHOD_HANDLE_BUFFER).AsInt32();
}

status_t 
RMediaInput::AcceptReserve(
	IMediaEndpoint::arg sourceEndpoint, IMediaOutput::arg sourceOutput,
	const BMediaConstraint &constraint,
	IMediaEndpoint::ptr *outReservedEndpoint, IMediaInput::ptr *outReservedInput)
{
	if (!outReservedEndpoint) return B_BAD_VALUE;
	if (!outReservedInput) return B_BAD_VALUE;
	BValue in;
	in.Overlay(PARG_ENDPOINT, BValue::Binder(sourceEndpoint->AsBinder()));
	in.Overlay(PARG_OUTPUT, BValue::Binder(sourceOutput->AsBinder()));
	in.Overlay(PARG_CONSTRAINT, constraint.AsValue());
	BValue outBindings;
	outBindings.Overlay(PMETHOD_ACCEPT_RESERVE, PMETHOD_ACCEPT_RESERVE);
	outBindings.Overlay(PARG_ENDPOINT, PARG_ENDPOINT);
	outBindings.Overlay(PARG_INPUT, PARG_INPUT);
	BValue out;
	Remote()->Effect(BValue(PMETHOD_ACCEPT_RESERVE, in), BValue::null, outBindings, &out);
	status_t err = out[PMETHOD_ACCEPT_RESERVE].AsInt32();
	if (err < B_OK) return err;
	*outReservedEndpoint = IMediaEndpoint::AsInterface(out[PARG_ENDPOINT]);
	*outReservedInput = IMediaInput::AsInterface(out[PARG_INPUT]);
	return B_OK;
}

status_t 
RMediaInput::AcceptConnect(
	IMediaEndpoint::arg sourceEndpoint, IMediaOutput::arg sourceOutput,
	IBufferOutlet::arg transport,
	const BMediaConstraint &constraint, const BMediaPreference &sourcePreference,
	IMediaEndpoint::ptr *outConnectedEndpoint,
	IMediaInput::ptr *outConnectedInput,
	BMediaFormat *outFormat)
{
	if (!outConnectedEndpoint) return B_BAD_VALUE;
	if (!outConnectedInput) return B_BAD_VALUE;
	if (!outFormat) return B_BAD_VALUE;
	BValue in;
	in.Overlay(PARG_ENDPOINT, BValue::Binder(sourceEndpoint->AsBinder()));
	in.Overlay(PARG_OUTPUT, BValue::Binder(sourceOutput->AsBinder()));
	in.Overlay(PARG_TRANSPORT, BValue::Binder(transport->AsBinder()));
	in.Overlay(PARG_CONSTRAINT, constraint.AsValue());
	in.Overlay(PARG_PREFERENCE, sourcePreference.AsValue());
	BValue outBindings;
	outBindings.Overlay(PMETHOD_ACCEPT_CONNECT, PMETHOD_ACCEPT_CONNECT);
	outBindings.Overlay(PARG_ENDPOINT, PARG_ENDPOINT);
	outBindings.Overlay(PARG_INPUT, PARG_INPUT);
	outBindings.Overlay(PARG_FORMAT, PARG_FORMAT);
//berr << "RMediaInput::AcceptConnect(): requested outBindings " << outBindings << endl;
	BValue out;
	Remote()->Effect(BValue(PMETHOD_ACCEPT_CONNECT, in), BValue::null, outBindings, &out);
	status_t err = out[PMETHOD_ACCEPT_CONNECT].AsInt32();
	if (err < B_OK) return err;
	*outConnectedEndpoint = IMediaEndpoint::AsInterface(out[PARG_ENDPOINT]);
	*outConnectedInput = IMediaInput::AsInterface(out[PARG_INPUT]);
//berr << "RMediaInput::AcceptConnect(): e " << (*outConnectedEndpoint)->AsBinder() <<
//	" i " << (*outConnectedInput)->AsBinder() << endl <<
//	"out: " << out << endl;
	*outFormat = out[PARG_FORMAT];
	return B_OK;
}

status_t 
RMediaInput::AcceptDependantConnect(
	IBufferOutlet::arg transport,
	const BMediaConstraint & constraint,
	const BMediaPreference & preference,
	BMediaFormat * outFormat)
{
	if (!outFormat) return B_BAD_VALUE;
	BValue in;
	in.Overlay(PARG_TRANSPORT, BValue::Binder(transport->AsBinder()));
	in.Overlay(PARG_CONSTRAINT, constraint.AsValue());
	in.Overlay(PARG_PREFERENCE, preference.AsValue());
	BValue outBindings;
	outBindings.Overlay(PMETHOD_ACCEPT_DEPENDANT_CONNECT, PMETHOD_ACCEPT_DEPENDANT_CONNECT);
	outBindings.Overlay(PARG_FORMAT, PARG_FORMAT);
	BValue out;
	Remote()->Effect(BValue(PMETHOD_ACCEPT_DEPENDANT_CONNECT, in), BValue::null, outBindings, &out);
	status_t err = out[PMETHOD_ACCEPT_DEPENDANT_CONNECT].AsInt32();
	if (err < B_OK) return err;
	*outFormat = out[PARG_FORMAT];
	return B_OK;
}

status_t 
RMediaInput::AcceptDisconnect()
{
	return Remote()->Invoke(PMETHOD_ACCEPT_DISCONNECT).AsInt32();
}

/**************************************************************************************/

status_t 
LMediaInput::Called(BValue & in, const BValue & outBindings, BValue & out)
{
	BValue v;
	if (v = in[PMETHOD_HANDLE_BUFFER])
	{
		BBuffer b(v);
		status_t err = HandleBuffer(&b);
		out += outBindings * BValue(PMETHOD_HANDLE_BUFFER, BValue::Int32(err));
	}
	if (v = in[PMETHOD_ACCEPT_CONNECT])
	{
		IMediaEndpoint::ptr outConnectedEndpoint;
		IMediaInput::ptr outConnectedInput;
		BMediaFormat outFormat;
		status_t err = AcceptConnect(
			IMediaEndpoint::AsInterface(v[PARG_ENDPOINT]),
			IMediaOutput::AsInterface(v[PARG_OUTPUT]),
			IBufferOutlet::AsInterface(v[PARG_TRANSPORT].AsBinder()),
			BMediaConstraint(v[PARG_CONSTRAINT]),
			BMediaPreference(v[PARG_PREFERENCE]),
			&outConnectedEndpoint, &outConnectedInput, &outFormat);
		BValue ret;
		ret.Overlay(PMETHOD_ACCEPT_CONNECT, BValue::Int32(err));
		if (err >= B_OK)
		{
			ret.Overlay(PARG_ENDPOINT,
				BValue::Binder(outConnectedEndpoint->AsBinder()));
			
			ret.Overlay(PARG_INPUT,
				BValue::Binder(outConnectedInput->AsBinder()));
				
			ret.Overlay(PARG_FORMAT, outFormat);
		}
		out += outBindings * ret;
	}
	if (v = in[PMETHOD_ACCEPT_RESERVE])
	{
		IMediaEndpoint::ptr outReservedEndpoint;
		IMediaInput::ptr outReservedInput;
		status_t err = AcceptReserve(
			IMediaEndpoint::AsInterface(v[PARG_ENDPOINT]),
			IMediaOutput::AsInterface(v[PARG_OUTPUT]),
			BMediaConstraint(v[PARG_CONSTRAINT]),
			&outReservedEndpoint, &outReservedInput);
		BValue ret;
		ret.Overlay(PMETHOD_ACCEPT_RESERVE, BValue::Int32(err));
		if (err >= B_OK)
		{
			ret.Overlay(PARG_ENDPOINT, BValue::Binder(outReservedEndpoint->AsBinder()));
			ret.Overlay(PARG_INPUT, BValue::Binder(outReservedInput->AsBinder()));
		}
		out += outBindings * ret;
	}
	if (v = in[PMETHOD_ACCEPT_DEPENDANT_CONNECT])
	{
		BMediaFormat outFormat;
		status_t err = AcceptDependantConnect(
			IBufferOutlet::AsInterface(v[PARG_TRANSPORT].AsBinder()),
			BMediaConstraint(v[PARG_CONSTRAINT]),
			BMediaPreference(v[PARG_PREFERENCE]),
			&outFormat);
		BValue ret;
		ret.Overlay(PMETHOD_ACCEPT_DEPENDANT_CONNECT, BValue::Int32(err));
		if (err >= B_OK) ret.Overlay(PARG_FORMAT, outFormat);
		out += outBindings * ret;
	}
	if (v = in[PMETHOD_ACCEPT_DISCONNECT])
	{
		status_t err = AcceptDisconnect();
		out += outBindings * BValue(PMETHOD_ACCEPT_DISCONNECT, BValue::Int32(err));
	}
	return B_OK;		
}

/**************************************************************************************/

BMediaInput::BMediaInput(const char * name) :
	BMediaEndpoint(B_INPUT_ENDPOINT)
{
	if (name) SetName(name);
}


BMediaInput::~BMediaInput()
{
}

BValue 
BMediaInput::Inspect(const BValue & v, uint32 flags)
{
	return BMediaEndpoint::Inspect(v, flags) + LMediaInput::Inspect(v, flags);
}

status_t 
BMediaInput::HandleBuffer(BBuffer *buffer)
{
	atom_ptr<BMediaNode> node = LocalNode();
	if (node == 0) return B_NOT_ALLOWED;
	return node->HandleBuffer(this, buffer);
}

status_t 
BMediaInput::AcceptReserve(
	IMediaEndpoint::arg sourceEndpoint,
	IMediaOutput::arg sourceOutput,
	const BMediaConstraint & inConstraint,
	IMediaEndpoint::ptr *outReservedEndpoint,
	IMediaInput::ptr *outReservedInput)
{
	atom_ptr<BMediaNode> node = LocalNode();
	if (node == 0) return B_NOT_ALLOWED;

	BMediaConstraint constraint;
	status_t err = BeginConnect(B_RESERVED_ENDPOINT, &constraint);
	if (err < B_OK) return err;
	
//	berr << "input: src constraint " << inConstraint << endl;
//	berr << "input: internal constraint " << constraint << endl;

	constraint.And(inConstraint);
	err = constraint.Simplify();

	if (err < B_OK)
	{
		// +++ we need some specific errors
		EndConnect();
		return B_ERROR;
	}
	
	// notify the node of a pending connection
	atom_ptr<BMediaInput> target = this;
	err = node->ReserveInputConnection(sourceOutput, &target, &constraint, B_RESERVED_ENDPOINT);
	if (err < B_OK)
	{
		EndConnect();
		return err;
	}
	
	if (target != this)
	{
		// redirect to the input specified by the node, passing the current constraint
		ASSERT(target.ptr());
		status_t err = target->AcceptReserve(
			sourceEndpoint, sourceOutput, constraint,
			outReservedEndpoint, outReservedInput);
		EndConnect();
		return err;
	}

	SetReserved(sourceEndpoint, constraint);
	*outReservedEndpoint = IMediaEndpoint::ptr(this);
	*outReservedInput = IMediaInput::ptr(this);
	EndConnect(true);
	return B_OK;
}

status_t 
BMediaInput::AcceptConnect(
	IMediaEndpoint::arg sourceEndpoint,
	IMediaOutput::arg sourceOutput,
	IBufferOutlet::arg transport,
	const BMediaConstraint & inConstraint,
	const BMediaPreference & inPreference,
	IMediaEndpoint::ptr *outConnectedEndpoint,
	IMediaInput::ptr *outConnectedInput,
	BMediaFormat *outFormat)
{
	atom_ptr<BMediaNode> node = LocalNode();
	if (node == 0) return B_NOT_ALLOWED;

	BMediaConstraint constraint;
	status_t err = BeginConnect(B_CONNECTED_ENDPOINT, &constraint);
	if (err < B_OK) return err;

//	berr << "input: src constraint " << inConstraint << endl;
//	berr << "input: internal constraint " << constraint << endl;

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
	atom_ptr<BMediaInput> targetInput = this;
	berr << "AcceptConnect: this " << this << ", " << (IMediaInput*)targetInput.ptr() << endl;
	err = node->ReserveInputConnection(sourceOutput, &targetInput, &constraint, B_CONNECTED_ENDPOINT);
	if (err < B_OK)
	{
		EndConnect();
		return err;
	}
	
	if (targetInput != this)
	{
		// redirect to the input specified by the node, passing the current constraint
		ASSERT(targetInput.ptr());
		status_t err = targetInput->AcceptConnect(
			sourceEndpoint, sourceOutput, transport, constraint, inPreference,
			outConnectedEndpoint, outConnectedInput, outFormat);
		EndConnect();
		return err;
	}
	
	// distill a format
	BMediaPreference myPreference = Preference();
	myPreference.AddItem(B_FORMATKEY_BUFFER_TRANSPORT, BValue::Int32(B_MEDIA_DEFAULT_TRANSPORT));
	BMediaPreference const * prefs[2];
	prefs[0] = &inPreference;
	prefs[1] = &myPreference;

	err = resolve_format(constraint, prefs, 2, outFormat);
	if (err < B_OK)
	{
berr << "BMediaInput::AcceptConnect(): resolve_format(): " << strerror(err) << endl;
		EndConnect();
		return err;
	}
	
	// give the local node a chance to back out
	err = node->AcceptInputConnection(sourceOutput, targetInput, *outFormat);
	if (err < B_OK)
	{
		node->AbortInputConnection(sourceOutput, targetInput, B_CONNECTED_ENDPOINT);
		EndConnect();
		return err;
	}
	
	// give the source endpoint/node a chance to back out, too
	err = sourceOutput->AcceptFormat(targetInput, *outFormat);
	if (err < B_OK)
	{
		node->AbortInputConnection(sourceOutput, targetInput, B_CONNECTED_ENDPOINT);
		EndConnect();
		return err;
	}

	// set up the transport
	if (transport != 0)
	{
		buffer_transport_type t = (buffer_transport_type)
			(*outFormat)[B_FORMATKEY_BUFFER_TRANSPORT].AsInt32();
		BBufferInlet::ptr inlet = LocalNode()->Inlet();
		err = inlet->Connect(t, transport, this);
		if (err < B_OK)
		{
			node->AbortInputConnection(sourceOutput, targetInput, B_CONNECTED_ENDPOINT);
			EndConnect();
			return err;
		}
	}

	// ** connection established **

	SetConnected(sourceEndpoint, *outFormat);
	
	*outConnectedEndpoint = IMediaEndpoint::ptr(this);
	*outConnectedInput = IMediaInput::ptr(this);
	EndConnect(true);
	return B_OK;
}

status_t 
BMediaInput::AcceptDependantConnect(
	IBufferOutlet::arg transport,
	const BMediaConstraint &inConstraint,
	const BMediaPreference &inPreference,
	BMediaFormat * outFormat)
{
	status_t err;
	atom_ptr<BMediaNode> node = LocalNode();
	if (node == 0) return B_NOT_ALLOWED;

	BMediaConstraint constraint;
	err = BeginDependantConnect(&constraint);
	if (err < B_OK) return err;

	media_endpoint_state state = EndpointState();
	if (state != B_RESERVED_ENDPOINT) return B_ERROR;

	IMediaEndpoint::ptr destEndpoint = Partner();
	IMediaOutput::ptr destOutput = IMediaOutput::AsInterface(destEndpoint->AsBinder());
	if (!destOutput.ptr())
	{
		EndDependantConnect();
		return B_ERROR;
	}

	berr << "input/dep: src constraint " << inConstraint << endl;
	berr << "input/dep: internal constraint " << constraint << endl;

	constraint.And(inConstraint);
	err = constraint.Simplify();

	if (err < B_OK)
	{
		berr << "## constraint result " << err << " : " << constraint << endl;

		// +++ we need some specific errors
		EndDependantConnect();
		return B_ERROR;
	}

	// distill a format
	BMediaPreference myPreference = Preference();
	myPreference.AddItem(B_FORMATKEY_BUFFER_TRANSPORT, BValue::Int32(B_MEDIA_DEFAULT_TRANSPORT));
	BMediaPreference const * prefs[2];
	prefs[0] = &inPreference;
	prefs[1] = &myPreference;

	err = resolve_format(constraint, prefs, 2, outFormat);
	if (err < B_OK)
	{
berr << "BMediaInput::AcceptConnect(): resolve_format(): " << strerror(err) << endl;
		EndConnect();
		return err;
	}

	// give the local node a chance to back out
	err = node->AcceptInputConnection(destOutput, this, *outFormat);
	if (err < B_OK)
	{
		node->AbortInputConnection(destOutput, this, B_CONNECTED_ENDPOINT);
		EndDependantConnect();
		return err;
	}

	// give the source endpoint/node a chance to back out, too
	err = destOutput->AcceptFormat(IMediaInput::ptr(this), *outFormat);
	if (err < B_OK)
	{
		node->AbortInputConnection(destOutput, this, B_CONNECTED_ENDPOINT);
		EndDependantConnect();
		return err;
	}

	// set up the transport
	if (transport != 0)
	{
		buffer_transport_type t = (buffer_transport_type)
			(*outFormat)[B_FORMATKEY_BUFFER_TRANSPORT].AsInt32();
		BBufferInlet::ptr inlet = LocalNode()->Inlet();
		err = inlet->Connect(t, transport, this);
		if (err < B_OK)
		{
			node->AbortInputConnection(destOutput, this, B_CONNECTED_ENDPOINT);
			EndConnect();
			return err;
		}
	}

	SetConnected(destEndpoint, *outFormat, true);

	EndDependantConnect();
	return B_OK;
}

status_t 
BMediaInput::AcceptDisconnect()
{
	status_t err = BeginConnect(B_FREE_ENDPOINT);
	if (err < B_OK) return err;

	SetDisconnected();
	
	EndConnect(true);
	return B_OK;
}

status_t 
BMediaInput::AcquireBuffer(BBuffer *outBuffer)
{
	// defer to buffer group, if we have one
	BBufferGroup::ptr group = BufferGroup();
	if (group.ptr()) return group->AcquireBuffer(outBuffer);

	// pass request to node
	BMediaNode::ptr node = LocalNode();
	if (node == 0) return B_ERROR;
	return node->AcquireBuffer(this, outBuffer);
}

status_t 
BMediaInput::AllocateBuffers()
{
	return B_UNSUPPORTED;
}

status_t
BMediaInput::Acquired(const void* id)
{
	BMediaEndpoint::Acquired(id);
	LMediaInput::Acquired(id);

	return B_OK;
}

status_t
BMediaInput::Released(const void* id)
{
	LMediaInput::Released(id);
	BMediaEndpoint::Released(id);
	return B_OK;
}

status_t 
BMediaInput::PropagateMessage(const BMessage &message,
								media_endpoint_type direction,
								BMediaEndpointVector *visited)
{
	if (direction==B_OUTPUT_ENDPOINT)
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
