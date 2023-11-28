/***************************************************************************
//
//	File:			media2/MediaEndpoint.h
//
//	Description:	Endpoints manage connections between nodes, buffer
//					acquisition, and buffer passing.
//
//					BMediaEndpoint: common endpoint functionality
//					BMediaOutput: buffer-producing endpoint
//					BMediaInput: buffer-consuming endpoint
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _MEDIA2_MEDIAENDPOINT_H_
#define _MEDIA2_MEDIAENDPOINT_H_

#include <support2/Binder.h>
#include <support2/Locker.h>
#include <support2/String.h>

#include <media2/BufferGroup.h>
#include <media2/IMediaEndpoint.h>
#include <media2/IMediaNode.h>
#include <media2/MediaConstraint.h>
#include <media2/MediaPreference.h>

namespace B {
namespace Private {
	class IBufferOutlet;
	class BBufferOutlet;
	class BBufferInlet;
};
namespace Media2 {

using namespace Support2;

class BMediaNode;
class BMediaEndpoint;

class BMediaProducerContext;

class LMediaEndpoint : public LInterface<IMediaEndpoint>
{
public:
	virtual	status_t			Called(BValue &in, const BValue &outBindings, BValue &out);
};

class BMediaEndpoint : public LMediaEndpoint
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BMediaEndpoint)

	// ** local API

	virtual	status_t				SetName(const char * name);
	virtual	status_t				SetNode(const atom_ptr<BMediaNode> & node);
	virtual	status_t				SetBufferGroup(BBufferGroup::arg group);
	virtual	BBufferGroup::ptr		BufferGroup() const;
	
	// you can call SetConstraint() at any time, but it will only have a visible
	// effect if/once the endpoint is disconnected.  when an endpoint is reserved
	// or connected,  IMediaEndpoint::Constraint() will reflect that state.
	virtual	status_t				SetConstraint(const BMediaConstraint & constraint);
	
	virtual	status_t				SetPreference(const BMediaPreference & preference);
	
	// call AddDependantConstraint() from BMediaNode::AcceptXXXConnection()
	// or BMediaNode::ReserveXXXConnection() to register a dependancy
	// between this endpoint (which must currently be being connected) and 'target',
	// with the given constraint.  the call WILL FAIL if this node is not currently
	// being connected, if a dependant constraint on 'target' has already been
	// added for this endpoint, or if 'target' does not belong to the same node as
	// this endpoint.  the effects of this call vary depending on the current state
	// of 'target':
	
	// B_FREE_ENDPOINT: the constraint is And()'d with the target's current constraint
	// (the one reported by Constraint()).  when this endpoint is disconnected, the
	// constraint is removed.  yes, you can stack multiple dependant constraints
	// on a given target and disconnect the endpoints in any order.  if a Simplify() of the
	// combined constraint fails, an appropriate error is returned and the endpoint
	// is left unmolested.
	
	// B_RESERVED_ENDPOINT: triggers a connection to the reserved partner endpoint,
	// using the given constraint. returns B_OK if the connection succeeded, or an
	// appropriate error otherwise.
	
	// B_CONNECTED_ENDPOINT: returns the result of a Simplify() of the given constraint
	// And()'d against the connection format.  doesn't result in any state change.
	//
	// any changes in endpoint state resulting from this call will take effect if
	// and only if the original connection to/from this endpoint is successful.

	virtual	status_t				AddDependantConstraint(
										const atom_ptr<BMediaEndpoint> & target,
										const BMediaConstraint & constraint);

	// ** IMediaEndpoint
	
	virtual	BString					Name() const;
	virtual	IMediaNode::ptr			Node() const;
	virtual media_endpoint_type		EndpointType() const;
	virtual	media_endpoint_state	EndpointState() const;
	virtual	IMediaEndpoint::ptr		Partner() const;
	virtual	BMediaConstraint		Constraint() const;
	virtual	BMediaPreference		Preference() const;
	virtual	BMediaFormat			Format() const;

protected:

	// ** IMediaEndpoint
	
	// default impl: no-op
	virtual	void					GetBufferConstraints(
										size_t * ioMinBufferCount,
										size_t * ioMinBufferCapacity);

	// default impl: no-op, return 0
	virtual	IBufferSource::ptr		MakeBufferSource(
										size_t minBufferCount,
										size_t minBufferCapacity);

	// ** hooks
	
	// default impl: no-op
	virtual	void					Connected(
										IMediaEndpoint::arg remoteEndpoint,
										const BMediaFormat & format);

	// default impl: no-op
	virtual	void					Disconnected(
										IMediaEndpoint::arg remoteEndpoint);


	// implementation

	virtual							~BMediaEndpoint();
	
	virtual	void					AttachedToNode(const atom_ptr<BMediaNode> & node);
	virtual	void					DetachedFromNode(const atom_ptr<BMediaNode> & node);
	virtual	void					ReleaseBuffers(media_buffer_group_id owner);

	virtual	status_t				Acquired(const void* id);
	virtual	status_t				Released(const void* id);

	virtual void					CommitDependantTransaction();
	virtual void					CancelDependantTransaction();

private:
	friend	class BMediaNode;
	friend	class BMediaInput;
	friend	class BMediaOutput;

									BMediaEndpoint(media_endpoint_type type);

			status_t				BeginConnect(
										media_endpoint_state requestedState,
										BMediaConstraint * outConstraint = 0);
			void					SetReserved(
										IMediaEndpoint::arg partner,
										const BMediaConstraint & reservation);
			void					SetConnected(
										IMediaEndpoint::arg partner,
										const BMediaFormat & format,
										bool asDependant = false);
			void					SetDisconnected();
			void					EndConnect(
										bool succeeded = false);
			
			status_t				BeginDependantTransaction(
										const atom_ptr<BMediaEndpoint> & source,
										const BMediaConstraint & constraint);
			status_t				BeginDependantConnect(
										BMediaConstraint * outConstraint = 0);
			void					EndDependantConnect();
			atom_ptr<BMediaNode> 	LocalNode() const;
						
			BMediaConstraint		_MergeExternalConstraints(const BMediaConstraint & internal);
			BMediaConstraint		_Constraint() const;

	enum flag_t
	{
		FLAG_CONNECTING 			= 1,
		FLAG_DEPENDANT_CONNECTED	= 2
	};

	struct external_constraint
	{
		int32				committed;
		IMediaEndpoint::ptr	source;
		BMediaConstraint *	constraint;
	};	
	typedef	BVector<external_constraint> ext_constraint_vector;
	
	mutable	BLocker					mLock;
			BLocker					mConnectLock;
			media_endpoint_state	mState;
			int32					mFlags;
			// count of transactions in which this endpoint is the dependant
			int32					mDependantRefs;
			BString					mName;
			atom_ref<BMediaNode> 	mNode;
	const	media_endpoint_type		mType;
			IMediaEndpoint::ptr		mPartner;
			BBufferGroup::ptr		mBufferGroup;
			BMediaFormat			mFormat;
			BMediaPreference		mPreference;
			BMediaConstraint		mInternalConstraint;
			ext_constraint_vector	mExternalConstraints;	// +++ BDumbVector! (hide external_constraint)
			BMediaConstraint		mConstraint;
			BMediaConstraint		mReservation;
			BMediaEndpointVector	mDependants;
			// dependant-connection state:
			IMediaEndpoint::ptr		mNewPartner;
			BMediaFormat			mNewFormat;
			BMediaEndpointVector	mNewDependants;
};

/**************************************************************************************/

class LMediaOutput : public LInterface<IMediaOutput>
{
public:
	virtual	status_t		Called(BValue &in, const BValue &outBindings, BValue &out);
};

class BMediaOutput : public BMediaEndpoint, public LMediaOutput
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BMediaOutput)

							BMediaOutput(const char * name = 0);

	virtual	status_t		SendBuffer(BBuffer * buffer);

	// ** IMediaOutput

	virtual	status_t		Reserve(IMediaInput::ptr * ioInput, const BMediaConstraint & constraint);
			status_t		Reserve(IMediaInput::ptr * ioInput);
	virtual	status_t		Connect(IMediaInput::ptr * ioInput, BMediaFormat * outFormat,
									const BMediaConstraint & constraint);
			status_t		Connect(IMediaInput::ptr * ioInput, BMediaFormat * outFormat);
	virtual	status_t		Disconnect();
	
	// ** IMediaEndpoint

	virtual	status_t		AcquireBuffer(BBuffer * outBuffer);
	virtual	status_t		AllocateBuffers();

	// propagate a message along this arc until the whole chain
	// is visited or some endpoint along the chain returns an error
	virtual	status_t		PropagateMessage(
								const BMessage & message,
								media_endpoint_type direction,
								BMediaEndpointVector * visited);

	virtual	BValue			Inspect(const BValue &which, uint32 flags = 0);

protected:
	virtual					~BMediaOutput();
	
	virtual	status_t		Acquired(const void* id);
	virtual	status_t		Released(const void* id);

	virtual	status_t		AcceptFormat(IMediaInput::arg input, const BMediaFormat & format);

	virtual	status_t		DependantConnect(const BMediaConstraint & constraint);

	virtual void			AbortDependantConnect();

#if 0
	virtual	void			AddedToContext(const atom_ptr<IMediaProducerContext> & context);
	virtual	void			RemovedFromContext(const atom_ptr<IMediaProducerContext> & context);
#endif
private:
			BLocker				mLock;
			IMediaInput::ptr	mDestination;
			atom_ptr<B::Private::BBufferOutlet>	mOutlet;
			atom_ptr<BMediaProducerContext>		mContext;
#if 0
			typedef BVector<atom_ptr<IMediaProducerContext> > context_vector;
			context_vector						mUpstreamContexts;
#endif
};

/**************************************************************************************/

class LMediaInput : public LInterface<IMediaInput>
{
public:
	virtual	status_t			Called(BValue &in, const BValue &outBindings, BValue &out);
};

class BMediaInput : public BMediaEndpoint, public LMediaInput
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BMediaInput)

								BMediaInput(const char * name = 0);
	
	// ** IMediaInput

	// default implementation:
	// - hand off to BMediaNode::HandleBuffer().
	virtual	status_t			HandleBuffer(BBuffer * buffer);

	// ** IMediaEndpoint

	virtual	status_t			AcquireBuffer(BBuffer * outBuffer);
	virtual	status_t			AllocateBuffers();

	// propagate a message along this arc until the whole chain
	// is visited or some endpoint along the chain returns an error
	virtual	status_t			PropagateMessage(
									const BMessage & message,
									media_endpoint_type direction,
									BMediaEndpointVector * visited);

	virtual	BValue				Inspect(const BValue &which, uint32 flags = 0);

protected:
	virtual						~BMediaInput();

	virtual	status_t			Acquired(const void* id);
	virtual	status_t			Released(const void* id);

	virtual	status_t			AcceptReserve(
									IMediaEndpoint::arg sourceEndpoint,
									IMediaOutput::arg sourceOutput,
									const BMediaConstraint & constraint,
									IMediaEndpoint::ptr * outReservedEndpoint,
									IMediaInput::ptr * outReservedInput);

	virtual	status_t			AcceptConnect(
									IMediaEndpoint::arg sourceEndpoint,
									IMediaOutput::arg sourceOutput,
									const atom_ptr<B::Private::IBufferOutlet> & transport,
									const BMediaConstraint & constraint,
									const BMediaPreference & sourcePreference,
									IMediaEndpoint::ptr * outConnectedEndpoint,
									IMediaInput::ptr * outConnectedInput,
									BMediaFormat * outFormat);

	virtual	status_t			AcceptDependantConnect(
									const atom_ptr<B::Private::IBufferOutlet> & transport,
									const BMediaConstraint & constraint,
									const BMediaPreference & sourcePreference,
									BMediaFormat * outFormat);

	virtual	status_t			AcceptDisconnect();
	
private:
};

}; }; // namespace B::Media2
#endif //_MEDIA2_MEDIAENDPOINT_H_
