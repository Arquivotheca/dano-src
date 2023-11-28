/*	$Id: DType.h,v 1.9 1999/03/09 09:32:30 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 13:38:23
*/

#ifndef DTYPE_H
#define DTYPE_H

#include "DLocation.h"

#include <string>
using std::string;

enum DTypeEncoding
{
	encVoid,
	encAddress,
	encBoolean,
	encFloat,
	encUnsignedChar,	
	encSignedChar,
	encSigned,
	encUnsigned,
	encComplexFloat
};

	// TypeID is an opaque type used by the implementations of symfiles to
	// identify uniquely the types available in this symfile.
	// Types can also have a TypeID of 0 in which case the DType is a syntetic
	// type that does not correspond to any entry in the symfile (pointer types e.g.)
typedef uint32 TypeID;

class DSymFile;

class DType
{
  protected:
	DType ();

  public:
	virtual ~DType ();
	
	virtual void GetName (string& name) const;
//	TypeID ID () const
//		{ return fIndirection ? 0 : fID; }
	
	virtual bool IsStruct () const = 0;
	virtual bool IsUnion () const = 0;
	virtual bool IsEnumeration () const = 0;
	virtual bool IsArray () const = 0;
//	virtual bool IsTypedef () const = 0;
	virtual bool IsPointer () const = 0;
	virtual bool IsReference () const = 0;
	virtual bool IsBase () const = 0;
	virtual bool IsString () const;
	
	bool IsConst () const
		{ return fConst; }
	bool IsVolatile () const
		{ return fVolatile; }
	
	virtual DType* Clone() const = 0;
	virtual DType* Deref() const;		// these return a new instance to the requested type...
//	virtual DType* Pointer() const = 0;
	
	uint32 Size () const						// the size of one instance of this type
		{ return fSize; }

	uint32 BitSize() const { return fBitSize; }	// for handling bitfields
	uint32 BitOffset() const { return fBitOffset; }

	// for base types
	virtual DTypeEncoding Encoding() const = 0;
	
	// for structs, unions and such
	virtual bool GetNthMemberInfo (int ix, string& name, DType*& type, DLocationString& location, bool& subclass, bool& isVirtual) const = 0;
	
	// for enumerations
	virtual bool GetNthEnumInfo (int ix, string& name, uint32& value) const = 0;
	
	// for arrays
	virtual bool GetNthSubrangeInfo (int32 ix, int32& lower_bound, int32& upper_bound) const = 0;
	virtual DType* GetElementType () const = 0;

  protected:

	bool				fConst, fVolatile;
//	TypeID			fID;
//	DSymFile			*fSymFile;
	uint32				fSize, fBitSize, fBitOffset;
	string				fName;
};

#endif
