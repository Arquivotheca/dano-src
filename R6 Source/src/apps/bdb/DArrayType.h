/*	$Id: DArrayType.h,v 1.1 1999/03/13 16:12:55 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 03/13/99 11:40:33
*/

#ifndef DARRAYTYPE_H
#define DARRAYTYPE_H

#include "DType.h"
#include <strstream>

class DArrayType : public DType
{
  public:
	DArrayType(DType *type)
		: fTarget(type)
		, fElements(gPrefs->GetPrefInt("array upper bound", 100))
	{
		fSize = fElements * type->Size();
		fTarget->GetName(fName);
		
		std::strstream s;
		s << fName << '[' << fElements << ']' << std::ends;
		fName = s.str();
		delete[] s.str();
	}
	~DArrayType()
	{
		delete fTarget;
	}
	
	virtual bool IsStruct () const					{ return false; }
	virtual bool IsUnion () const					{ return false; }
	virtual bool IsEnumeration () const			{ return false; }
	virtual bool IsArray () const					{ return true; }
//	virtual bool IsTypedef () const				{ return false; }
	virtual bool IsPointer () const				{ return false; }
	virtual bool IsReference () const			{ return false; }
	virtual bool IsBase () const					{ return false; }
	virtual bool IsString () const					{ return false; }

	virtual DType* Clone() const				{ return new DArrayType(fTarget->Clone()); }
	virtual DType* Deref() const				{ return fTarget->Clone(); }

	// for base types
	virtual DTypeEncoding Encoding() const	{ return encAddress; }
	
	// for structs, unions and such
	virtual bool GetNthMemberInfo (int, string&, DType*&, DLocationString&, bool&, bool&) const
														{ return false; }
	
	// for enumerations
	virtual bool GetNthEnumInfo (int, string&, uint32&) const
														{ return false; }
	
	// for arrays
	virtual bool GetNthSubrangeInfo (int32 ix, int32& lower_bound, int32& upper_bound) const
	{
		if (ix == 0)
		{
			lower_bound = 0;
			upper_bound = fElements;
			return true;
		}
		
		return false;
	} // GetNthSubrangeInfo
	
	virtual DType* GetElementType () const
	{
		return fTarget->Clone();
	}

  private:
	DType *fTarget;
	int fElements;
};

#endif
