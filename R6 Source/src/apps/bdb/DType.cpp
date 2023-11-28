/*	$Id: DType.cpp,v 1.4 1999/03/09 09:32:29 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 13:49:24
*/

#include "bdb.h"
#include <strstream>
#include "DType.h"

DType::DType()
{
	fConst = false;
	fVolatile = false;
	fSize = fBitSize = fBitOffset = 0;
} // DType::DType

DType::~DType()
{
} // DType::~DType

void DType::GetName (string& name) const
{
	name = fName;
	
	if (IsPointer())
		name += '*';
	else if (IsReference())
		name += '&';
} // DType::GetName

	// See if a type can be displayed as a string
bool DType::IsString() const
{
	bool result = false;

//// (IsPointer() || )
//	if (IsArray() &&
//		fEntry.find_attribute_value(DW_AT_type, e))
//	{
//		if (e.tag() == DW_TAG_const_type)
//			e.find_attribute_value(DW_AT_type, e);
//
//		DType target(e);
//		
//		result = target.IsBase() &&
//			(target.Encoding() == DW_ATE_signed_char ||
//			 target.Encoding() == DW_ATE_unsigned_char);
//	}
	
	return result;
} // DType::IsString

DType* DType::Deref() const
{
	THROW(("Cannot deref this type"));
} // DType::Deref
