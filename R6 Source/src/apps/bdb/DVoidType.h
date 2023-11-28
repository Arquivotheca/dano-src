/*	$Id: DVoidType.h,v 1.1 1999/03/05 14:25:20 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 03/04/99 13:33:56
*/

#ifndef DVOIDTYPE_H
#define DVOIDTYPE_H

#include "DType.h"

class DVoidType : public DType
{
  public:
	DVoidType()
	{
		fSize = 1;
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

	virtual DType* Clone() const				{ return new DVoidType; }
	virtual DType* Deref() const				{ return new DVoidType; }

	// for base types
	virtual DTypeEncoding Encoding() const	{ return encVoid; }
	
	// for structs, unions and such
	virtual bool GetNthMemberInfo (int, string&, DType*&, DLocationString&, bool&, bool&) const
														{ return false; }
	
	// for enumerations
	virtual bool GetNthEnumInfo (int, string&, uint32&) const
														{ return false; }
	
	// for arrays
	virtual bool GetNthSubrangeInfo (int32, int32&, int32&) const
														{ return false; }
	virtual DType* GetElementType () const { return 0; }
};

#endif
