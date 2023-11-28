/******************************************************************************
//
//	File:			support2/IBinder.h
//
//	Description:	Abstract interface for a binder node.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
******************************************************************************/

#ifndef _SUPPORT2_BINDER_INTERFACE_H_
#define _SUPPORT2_BINDER_INTERFACE_H_

#include <support2/Atom.h>

namespace B { namespace Private { class RBinder; } }

namespace B {
namespace Support2 {

class BBinder;
class BParcel;
class BValue;

/*-------------------------------------------------------------*/
/*------- IBinder Interface -----------------------------------*/

// Standard Effect() transaction code.
enum {
	B_EFFECT_TRANSACTION	=	'EFCT'
};

class IBinder : virtual public BAtom
{
	public:
		B_STANDARD_ATOM_TYPEDEFS(IBinder);

				//!	Probe binder for interface information.
				/*!	Return interfaces implemented by this binder object
					that are requested by \a which.  This is a composition
					of all interfaces expressed as { descriptor -> binder }
					mappings, selected through \a which.
				*/
		virtual	BValue					Inspect(const BValue &which,
												uint32 flags = 0) = 0;
				
				//!	Link registers the IBinder "to" for notification of events.
				/*!	The \a bindings is a mapping of keys that will get pushed on this
					IBinder, to keys that will get pushed on the "to" IBinder.
					e.g.
						- If you call link thus:
							binder1->Link(binder2, BValue("A", "B"));
						- When "A" is pushed on binder1:
							binder1->Push(BValue("A", "C"));
						- The following will get called on binder2:
							binder2->Effect(BValue("B", "C"), BValue::null, BValue::undefined, NULL);
				*/
		virtual	status_t				Link(	IBinder::arg to,
												const BValue &bindings) = 0;
				//! Remove a mapping previously added by Link().
		virtual	status_t				Unlink(	IBinder::arg from,
												const BValue &bindings) = 0;
		
				//!	Perform an action on the binder -- either a get, put,
				//!	or invocation, depending on the supplied and requested
				//!	bindings.
		virtual	status_t				Effect(	const BValue &in,
												const BValue &inBindings,
												const BValue &outBindings,
												BValue *out) = 0;
				//!	Synonym for Effect(in, BValue::null, BValue::undefined, NULL).
				status_t				Put(const BValue &in);
				//!	Synonym for Effect(BValue::undefined, BValue::undefined, bindings, [result]).
				BValue					Get(const BValue &bindings) const;
				//!	Synonym for Effect(BValue::undefined, BValue::undefined, BValue::null, [result]).
				BValue					Get() const;
				//!	Synonym for Effect(in, BValue::null, BValue::null, [result]).
				BValue					Invoke(const BValue &in);
				//!	Synonym for Effect(BValue(binding,in), BValue::null, binding, [result]).
				BValue					Invoke(const BValue &in, const BValue &binding);
		
				//!	Low-level data transfer.
		virtual	status_t				Transact(	uint32 code,
													BParcel& data,
													BParcel* reply = NULL,
													uint32 flags = 0) = 0;
		
				//!	Use these functions instead of a dynamic_cast<> for
				//!	the requested type.
				/*!	Since multiple BBinder instances can appear in a
					single object, a regular dynamic_cast<> is ambiguous.
				*/
		virtual	BBinder*				LocalBinder() = 0;
		virtual	B::Private::RBinder*	RemoteBinder() = 0;
		
		// deprecated
		typedef BValue 					value;
	
	protected:
		inline							IBinder() { }
		inline virtual					~IBinder() { }
	
	private:
										IBinder(const IBinder& o);
};

/*-------------------------------------------------------------*/

} } // namespace B::Support2

#endif	// _SUPPORT2_BINDER_INTERFACE_H_
