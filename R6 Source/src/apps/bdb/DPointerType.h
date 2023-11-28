/*	$Id: DPointerType.h,v 1.2 1999/03/13 16:12:57 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 03/08/99 16:10:35
*/

#ifndef DPOINTERTYPE_H
#define DPOINTERTYPE_H

#include "DType.h"

class DPointerType : public DType
{
  public:
	DPointerType(DType *type)
		: fTarget(type)
	{
		fSize = 4;
		fTarget->GetName(fName);
		fName += '*';
	}
	~DPointerType()
	{
		delete fTarget;
	}
	
	virtual bool IsStruct () const					{ return false; }
	virtual bool IsUnion () const					{ return false; }
	virtual bool IsEnumeration () const			{ return false; }
	virtual bool IsArray () const					{ return false; }
//	virtual bool IsTypedef () const				{ return false; }
	virtual bool IsPointer () const				{ return true; }
	virtual bool IsReference () const			{ return false; }
	virtual bool IsBase () const					{ return false; }
	virtual bool IsString () const					{ return false; }

	virtual DType* Clone() const				{ return new DPointerType(fTarget->Clone()); }
	virtual DType* Deref() const				{ return fTarget->Clone(); }

	// for base types
	virtual DTypeEncoding Encoding() const	{ return encAddress; }
	
	// for structs, unions and such
	virtual bool GetNthMemberInfo (int ix, string& name, DType*& type, DLocationString& location, bool& subclass, bool& /*isVirtual*/) const
	{
		if (ix == 0)
		{
			name = fName; name.erase(name.length() - 1, 1);
			type = Deref();
			location.clear(); location.push_back(DW_OP_deref);
			subclass = false;
			return true;
		}
		
		return false;
	} // GetNthMemberInfo
	
	// for enumerations
	virtual bool GetNthEnumInfo (int, string&, uint32&) const
														{ return false; }
	
	// for arrays
	virtual bool GetNthSubrangeInfo (int32, int32&, int32&) const
														{ return false; }
	virtual DType* GetElementType () const { return 0; }

  private:
	DType *fTarget;
};

#endif
