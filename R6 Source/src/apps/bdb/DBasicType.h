/*	$Id: DBasicType.h,v 1.1 1999/05/03 13:10:44 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 04/22/99 16:42:07
*/

#ifndef DBASICTYPE_H
#define DBASICTYPE_H

#include "DType.h"

class DIntType : public DType
{
	bool fSigned;
	
  public:
	DIntType(int size, bool sign)
	{
		fSize = size;
		fSigned = sign;
	}
	
	virtual bool IsStruct () const					{ return false; }
	virtual bool IsUnion () const					{ return false; }
	virtual bool IsEnumeration () const			{ return false; }
	virtual bool IsArray () const					{ return false; }
//	virtual bool IsTypedef () const				{ return false; }
	virtual bool IsPointer () const				{ return false; }
	virtual bool IsReference () const			{ return false; }
	virtual bool IsBase () const					{ return true; }
	virtual bool IsString () const					{ return false; }

	virtual DType* Clone() const				{ return new DIntType(fSize, fSigned); }
	virtual DType* Deref() const				{ THROW(("Cannot deref a base type")); }

	// for base types
	virtual DTypeEncoding Encoding() const	{ return fSigned ? encSigned : encUnsigned; }
	
	// for structs, unions and such
	virtual bool GetNthMemberInfo (int, string&, DType*&, DLocationString&, bool&, bool&) const
														{ return false; }
	
	// for enumerations
	virtual bool GetNthEnumInfo (int, string&, uint32&) const
														{ return false; }
	
	// for arrays
	virtual bool GetNthSubrangeInfo (int32, int32&, int32&) const
														{ return false; }
	virtual DType* GetElementType () const
														{ THROW(("Base type has no elements")); }
};

class DFloatType : public DType
{
  public:
	DFloatType(int size)
	{
		fSize = size;
	}
	
	virtual bool IsStruct () const					{ return false; }
	virtual bool IsUnion () const					{ return false; }
	virtual bool IsEnumeration () const			{ return false; }
	virtual bool IsArray () const					{ return false; }
//	virtual bool IsTypedef () const				{ return false; }
	virtual bool IsPointer () const				{ return false; }
	virtual bool IsReference () const			{ return false; }
	virtual bool IsBase () const					{ return true; }
	virtual bool IsString () const					{ return false; }

	virtual DType* Clone() const				{ return new DFloatType(fSize); }
	virtual DType* Deref() const				{ THROW(("Cannot deref a base type")); }

	// for base types
	virtual DTypeEncoding Encoding() const	{ return encFloat; }
	
	// for structs, unions and such
	virtual bool GetNthMemberInfo (int, string&, DType*&, DLocationString&, bool&, bool&) const
														{ return false; }
	
	// for enumerations
	virtual bool GetNthEnumInfo (int, string&, uint32&) const
														{ return false; }
	
	// for arrays
	virtual bool GetNthSubrangeInfo (int32, int32&, int32&) const
														{ return false; }
	virtual DType* GetElementType () const
														{ THROW(("Base type has no elements")); }
};

#endif
