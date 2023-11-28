/******************************************************************************
//
//	File:			support2/IInterface.h
//
//	Description:	Common base class for abstract binderized interfaces.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
******************************************************************************/

#ifndef	_SUPPORT2_INTERFACE_INTERFACE_H
#define	_SUPPORT2_INTERFACE_INTERFACE_H

#include <support2/SupportDefs.h>
#include <support2/Atom.h>

namespace B {
namespace Support2 {

class IBinder;
class BValue;

/**************************************************************************************/

class IInterface : virtual public BAtom
{
	public:
		inline							IInterface() { }
		
				atom_ptr<IBinder>		AsBinder();
				atom_ptr<const IBinder>	AsBinder() const;
	
	protected:
		inline virtual					~IInterface() { }
	
		virtual	atom_ptr<IBinder>		AsBinderImpl();
		virtual	atom_ptr<const IBinder>	AsBinderImpl() const;
	
	private:
										IInterface(const IInterface&);
};

/**************************************************************************************/

// Use this macro inside of your IInterface subclass to define the
// standard IInterface meta-API.  See Atom.h for the definition of
// the B_STANDARD_ATOM_TYPEDEFS() macro.
#define B_DECLARE_META_INTERFACE(iname)												\
		B_STANDARD_ATOM_TYPEDEFS(I ## iname)										\
																					\
		static	const ::B::Support2::BValue											\
			descriptor;																\
		static	::B::Support2::atom_ptr<I ## iname>									\
			AsInterface(const ::B::Support2::atom_ptr< ::B::Support2::IBinder> &o);	\
		static	::B::Support2::atom_ptr<I ## iname>									\
			AsInterface(const ::B::Support2::BValue &v);							\

/**************************************************************************************/

// Use this macro inside of your IInterface subclass's .cpp file to
// implement the above meta-API.  Note that you must have defined your
// remote proxy class (with an "R" prefix) before using the macro.
#define B_IMPLEMENT_META_INTERFACE(iname)											\
		::B::Support2::atom_ptr<I ## iname> I ## iname ::AsInterface(				\
			const ::B::Support2::atom_ptr< ::B::Support2::IBinder> &o)				\
		{																			\
			return b_standard_as_interface<I ## iname, R ## iname>(o);				\
		}																			\
		::B::Support2::atom_ptr<I ## iname> I ## iname ::AsInterface(				\
			const ::B::Support2::BValue &v)											\
		{																			\
			return b_standard_as_interface<I ## iname, R ## iname>(v.AsBinder());	\
		}																			\

template<class INTERFACE, class REMOTE>
inline atom_ptr<INTERFACE> b_standard_as_interface(const atom_ptr<IBinder> &o)
{
	if (o == NULL) return NULL;
	atom_ptr<IBinder> b = o->Inspect(INTERFACE::descriptor).AsBinder();
	if (b == NULL) return NULL;
	
	atom_ptr<INTERFACE> p = dynamic_cast<INTERFACE*>(b.ptr());
	if (p == NULL) p = new REMOTE(b);
	return p;
}

/**************************************************************************************/

} } // namespace B::Support2

#endif /* _SUPPORT2_INTERFACE_INTERFACE_H */
