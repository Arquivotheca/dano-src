/*	$Id: DDwarfType.cpp,v 1.3 1999/03/13 16:12:56 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 03/03/99 13:54:41
*/

#include "bdb.h"
#include "DDwarfType.h"
#include "DDwarfSymFile.h"
#include <strstream>

using namespace dwarf;

void 
DDwarfType::Init()
{
	do
	{
		if (fConst = (fEntry.tag() == DW_TAG_const_type))
		{
			fEntry.find_attribute_value(DW_AT_type, fEntry);
			continue;
		}
		if (fVolatile = (fEntry.tag() == DW_TAG_volatile_type))
		{
			fEntry.find_attribute_value(DW_AT_type, fEntry);
			continue;
		}
		if (fEntry.tag() == DW_TAG_typedef)
		{
			fEntry.find_attribute_value(DW_AT_type, fEntry);
			continue;
		}
	}
	while (false);

	switch (fEntry.tag())
	{
		case DW_TAG_pointer_type:
		{
			entry e;

			fEntry.find_attribute_value (DW_AT_type, e);
			DDwarfType(e).GetName(fName);
			fName = '*' + fName;
			break;
		}
		
		case DW_TAG_reference_type:
		{
			entry e;

			fEntry.find_attribute_value (DW_AT_type, e);
			DDwarfType(e).GetName(fName);
			fName += '&';
			break;
		}
		
		default:
			fEntry.find_attribute_value (DW_AT_name, fName);
			break;
	}

	fString = false;

	// char*, unsigned char*, char[], and unsigned char[] are displayed as C strings by default
	entry e;
	if ((IsPointer() || IsArray()) && fEntry.find_attribute_value(DW_AT_type, e))
	{
		if (e.tag() == DW_TAG_const_type)
			e.find_attribute_value(DW_AT_type, e);

		DDwarfType target(e);
		
		fString = target.IsBase() && (target.Encoding() == encSignedChar);
	}

	uint32 encoding = 0;
	fEntry.find_attribute_value(DW_AT_encoding, encoding);
	
	switch (encoding)
	{
		default:
		case DW_ATE_void:				fEncoding = encVoid;					break;
		case DW_ATE_address:			fEncoding = encAddress;				break;
		case DW_ATE_boolean:			fEncoding = encBoolean;				break;
		case DW_ATE_float:				fEncoding = encFloat;					break;
		case DW_ATE_unsigned_char:	fEncoding = encUnsignedChar;		break;
		case DW_ATE_signed_char:		fEncoding = encSignedChar;			break;
		case DW_ATE_signed:			fEncoding = encSigned;				break;
		case DW_ATE_unsigned:			fEncoding = encUnsigned;			break;
		case DW_ATE_complex_float:	fEncoding = encComplexFloat;		break;
	}

	if (fEntry.tag() == DW_TAG_structure_type)
	{
		uint32 isDeclaration = 0;
		fEntry.find_attribute_value (DW_AT_declaration, isDeclaration);
		if (isDeclaration)
		{
			// if it's a declaration, look up the concrete entry for this type in the symfile's cache
			try
			{
				const DDwarfSymFile& symfile = fEntry.get_compile_unit().get_dwarf().get_symfile();
				uint32 full_offset = fEntry.get_offset() + fEntry.get_compile_unit().info_offset();
				fEntry = symfile.GetEntryAt(full_offset);
			}
			catch (...)
			{
				// no such entry; we just bail at this point
			}
		}
	}

	fEntry.find_attribute_value (DW_AT_byte_size, fSize);
}

DDwarfType::DDwarfType()
{
}

DDwarfType::DDwarfType(const dwarf:: entry &entry)
	: fEntry(entry)
{
	Init();
	fFullName = fName;
}

DDwarfType::DDwarfType (const string& fullname, const dwarf::entry& e)
	: fEntry(e), fFullName(fullname)
{
	Init();
} // DDwarfType::DDwarfType


DDwarfType::DDwarfType(const DDwarfType& other)
	: DType(), fEntry(other.fEntry), fFullName(other.fFullName), fString(other.fString), fEncoding(other.fEncoding)
{
}

DDwarfType &
DDwarfType::DDwarfType::operator=(const DDwarfType& rhs)
{
	fEntry = rhs.fEntry;
	fFullName = rhs.fFullName;
	fString = rhs.fString;
	fEncoding = rhs.fEncoding;
	return *this;
}

void 
DDwarfType::GetName(string &outName) const
{
	outName = fFullName;
}

bool DDwarfType::IsStruct () const
{
	return fEntry.tag() == DW_TAG_structure_type;
} // DDwarfType::IsStruct

bool DDwarfType::IsUnion () const
{
	return fEntry.tag() == DW_TAG_union_type;
} // DDwarfType::IsUnion

bool DDwarfType::IsEnumeration () const
{
	return fEntry.tag() == DW_TAG_enumeration_type;
} // DDwarfType::IsEnumeration

bool DDwarfType::IsArray () const
{
	return fEntry.tag() == DW_TAG_array_type;
} // DDwarfType::IsArray

//bool DDwarfType::IsTypedef () const
//{
//} // DDwarfType::IsTypedef

bool DDwarfType::IsPointer () const
{
	return fEntry.tag() == DW_TAG_pointer_type;
} // DDwarfType::IsPointer

bool DDwarfType::IsReference () const
{
	return fEntry.tag() == DW_TAG_reference_type;
} // DDwarfType::IsReference

bool DDwarfType::IsBase () const
{
	return fEntry.tag() == DW_TAG_base_type;
} // DDwarfType::IsBase

bool DDwarfType::IsString () const
{
	return fString;
} // DDwarfType::IsString

DType* DDwarfType::Clone() const
{
	return new DDwarfType(fEntry);
} // DDwarfType::Clone()

DType* DDwarfType::Deref() const
{
	entry e;

	if (!fEntry.find_attribute_value(DW_AT_type, e))
		THROW(("No type"));
	
	return new DDwarfType(e);
} // DDwarfType::Deref()

DTypeEncoding DDwarfType::Encoding() const
{
	return fEncoding;
} // DDwarfType::Encoding()

bool DDwarfType::GetNthMemberInfo (int ix, string& name, DType*& type, DLocationString& location, bool& subclass, bool& isVirtual) const
{
	bool result = false;
	member_iterator i = member_begin();
	
	while (ix--)
		i++;
	
	if (i != member_end())
	{
		entry e = *i;
		entry ref_entry;
		uint32 virtuality = 0;
		
		e.find_attribute_value (DW_AT_data_member_location, location);
		bool hasName = e.find_attribute_value (DW_AT_name, name);
		subclass = (e.tag() == DW_TAG_inheritance);
		e.find_attribute_value(DW_AT_virtuality, virtuality);
		isVirtual = (virtuality == DW_VIRTUALITY_virtual);

		// if this is a bitfield member, then *this* entry will have the bitfield's size
		// and offset information, not the referenced type, so we pass them along
		// to the output DType object.
		e.find_attribute_value (DW_AT_type, ref_entry);
		DDwarfType* ddType = new DDwarfType (ref_entry);		// this class needed for access to fBitSize & fBitOffset members
		e.find_attribute_value(DW_AT_bit_size, ddType->fBitSize);
		e.find_attribute_value(DW_AT_bit_offset, ddType->fBitOffset);

		if (!hasName)
		{
			name = (ddType->fEntry.tag() == DW_TAG_union_type) ? "<anonymous union>" : "<anonymous>";
		}

		type = ddType;
		result = true;
	}
	
	return result;
} // DDwarfType::GetNthMemberInfo

bool DDwarfType::GetNthEnumInfo (int ix, string& name, uint32& value) const
{
	bool result = false;
	enum_iterator i = enum_begin();

	while (ix--)
		i++;

	if (i != enum_end())
	{
		(*i).find_attribute_value (DW_AT_name, name);
		(*i).find_attribute_value (DW_AT_const_value, value);
		result = true;
	}
	
	return result;
} // DDwarfType::GetNthEnumInfo

bool DDwarfType::GetNthSubrangeInfo (int32 ix, int32& lower_bound, int32& upper_bound) const
{
	bool result = false;
	subrange_iterator i = subrange_begin();

	while (ix--)
		i++;

	uint32 u = 0, l = 0;

	if (i != subrange_end())
	{
		if (!(*i).find_attribute_value(DW_AT_upper_bound, u))
			u = 100;
		if (!(*i).find_attribute_value(DW_AT_lower_bound, l))
			l = 0;
		
		upper_bound = u;
		lower_bound = l;
		
		result = true;
	}
	
	return result;
} // DDwarfType::GetNthSubrangeInfo

DType* DDwarfType::GetElementType () const
{
	entry e;
	fEntry.find_attribute_value (DW_AT_type, e);
	return new DDwarfType(e);
} // DDwarfType::GetElementType

