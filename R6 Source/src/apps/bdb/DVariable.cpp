/*	$Id: DVariable.cpp,v 1.19 1999/05/03 13:09:58 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 13:49:27
*/

#include "bdb.h"
#include "DVariable.h"
#include "DStackFrame.h"
#include "DNub.h"
#include "DPredicates.h"
#include <strstream>
#include <cctype>
#include "DVoidType.h"
#include <ByteOrder.h>

DVariable::DVariable(const string& name, DType *type, const DLocationString& location)
{
	fType = type;
	fName = name;
	fLocation = location;
	
	if (type->IsStruct() || type->IsUnion())
		Expand();
	else if (type->IsPointer() || type->IsReference())
		fMembers.push_back(new DPointerVariable(*this));
	else if (type->IsArray())
	{
//		if (type->SubrangeCount() != 1)
//			THROW(("Unimplemented multidimensional array type"));
		
		int32 lower_bound, upper_bound;

		if (type->GetNthSubrangeInfo(0, lower_bound, upper_bound))
		{
			int32 max_upper_bound = gPrefs->GetPrefInt("array upper bound", 100);

			if (max_upper_bound && upper_bound - lower_bound > max_upper_bound)
				upper_bound = lower_bound + max_upper_bound;
	
			// DType::GetElementType() return a *new* object, so we have to call it for
			// each DArrayVariable that we create
			for (int indx = lower_bound; indx <= upper_bound; indx++)
			{
				fMembers.push_back(new DArrayVariable(*this, indx, type->GetElementType()));
			}
		}
		else
			THROW(("Invalid array"));
	}
} // DVariable::DVariable

DVariable::~DVariable ()
{
	for (member_iterator i = member_begin(); i != member_end(); i++)
		delete *i;
	delete fType;
} // DVariable::~DVariable

void DVariable::GetName (string& name) const
{
	name = fName;
} // DVariable::GetName

void DVariable::SetName(const string& name)
{
	fName = name;
} // DVariable::SetName

DType* DVariable::Type () const
{
	return fType;
} // DVariable::Type

bool DVariable::IsExpandable () const
{
	return fType->IsStruct() || fType->IsUnion() || fType->IsPointer() || fType->IsReference() || fType->IsArray();
} // DVariable::IsExpandable

void DVariable::Expand ()
{
	if (fMembers.size() == 0)
	{
		int i = 0;
		bool isSubclass, isVirtual;
		string name;
		DType *type;
		DLocationString location;
		
		while (fType->GetNthMemberInfo(i++, name, type, location, isSubclass, isVirtual))
		{
			if (isSubclass)
			{
				// virtual inheritance is handled very differently -- there is no actual base class member
				// data for virtual base classes - so we ignore them here, and handle them in the member
				// variable code [since virtual inheritance produces a pointer member to the virtual base
				// class data]
				if (!isVirtual)
					fMembers.push_back(new DInheritedVariable(*this, name, type, location));
			}
			else
			{
				fMembers.push_back(new DMemberVariable(*this, name, type, location));
			}
		}
	}
} // DVariable::Expand

DVariable* DVariable::GetMemberByName(const char *name)
{
	if (fMembers.size() == 0)
		Expand();
	
	for (member_iterator i = member_begin(); i != member_end(); i++)
	{
		string m;
		(*i)->GetName(m);
		if (m == name)
			return *i;
	}
	
	THROW(("No \"%s\" member found", name));
} // DVariable::GetMemberByName

void DVariable::GetValue(const DStackFrame& frame, void *data, uint32& size) const
{
	ptr_t addr;
	locType lType = LocationMachine(fLocation, frame, addr);
	
	switch (lType)
	{
		case ltRegister:
		{
			uint32 x;

			frame.GetRegister(addr, x);

			switch (size)
			{
				case 1:	*(uchar *)data = x; break;
				case 2:	*(ushort *)data = x; break;
				default:	*(ulong *)data = x; break;
			}
			size = min(size, Type()->Size());
			break;
		}
		
		case ltMemory:
		{
			DNub& nub = frame.GetNub();
			BAutolock lock(nub);
			
			size = min(size, Type()->Size());
			nub.ReadData(addr, data, size);
			break;
		}
		
		default:
			THROW(("Error in location type"));
	}
} // DVariable::GetValue

void DVariable::SetValue(DStackFrame& frame, const void *data, uint32 size) const
{
	ptr_t addr;
	locType lType;
	
	try
	{
		lType = ltMemory;
		addr = GetLocation(frame);
	}
	catch (HErr& e)
	{
		lType = LocationMachine(fLocation, frame, addr);
	}
	
	switch (lType)
	{
		case ltRegister:
		{
//			uint32 x;
//
//			frame.GetRegister(addr, x);
//
//			switch (size)
//			{
//				case 1:	*(uchar *)data = x; break;
//				case 2:	*(ushort *)data = x; break;
//				case 4:	*(ulong *)data = x; size = sizeof(uint32); break;
//				default:	THROW(("Unsupported size"));
//			}
			THROW(("Unsupported location"));
			break;
		}
		
		case ltMemory:
		{
			DNub& nub = frame.GetNub();
			BAutolock lock(nub);
			
			if (size > Type()->Size()) THROW(("Does not fit"));
			nub.WriteData(addr, data, size);
			break;
		}
		
		default:
			THROW(("Error in location type"));
	}
} // DVariable::SetValue

void DVariable::GetLocationString(DLocationString& loc) const
{
	loc = fLocation;
} // DVariable::GetLocationString

ptr_t DVariable::GetLocation(const DStackFrame& frame) const
{
	ptr_t addr;
	
	if (LocationMachine(fLocation, frame, addr) != ltMemory)
		THROW(("•Location is not in memory•"));
	
	return addr;
} // DVariable::GetLocation

ptr_t DVariable::GetLocation(DNub& nub, ptr_t base) const
{
	return base + LocationMachine(fLocation, nub);
} // DVariable::GetLocation

	// GetValueAsText tries to return a sensible value
void DVariable::GetValueAsText(const DStackFrame& frame, string& value, EVarFormat format) const
{
	try
	{
		bool done = false;
	
		if (fType->IsEnumeration() && (format == fmtEnum || format == fmtDefault))
		{
			unsigned long long l = 0;
			char *b = (char*)&l;
			uint32 size = 16, eValue;
			int32 i = 0;
			
			GetValue(frame, b, size);
#if !__LITTLE_ENDIAN
#	error
#endif
			while (!done && fType->GetNthEnumInfo(i++, value, eValue))
			{
				if (eValue == l)
					done = true;
			}
			
			if (! done)
				format = fmtUnsigned;
		}
		
		if (!done)
		{
			if (format == fmtString || (fType->IsString() && format == fmtDefault))
			{
				ptr_t addr;
				// GetLocation() gives us the array's location, but doesn't work for register pointers;
				// GetValue() gives us pointer values, but dereferences arrays, giving the wrong addr.
				// So, we do a little special-casing....
				if (fType->IsArray())
					addr = GetLocation(frame);
				else
					GetValue(frame, addr);
				FormatString(addr, frame.GetNub(), value);
				done = true;
			}
			else if (fType->IsBase() || (fType->IsEnumeration() && format != fmtEnum))
			{
				char buf[16];
				uint32 size = 16;
				
				GetValue(frame, buf, size);
				FormatBaseType(buf, size, value, format);
				done = true;
			}
			else if (fType->IsPointer() || fType->IsReference())
			{
				ptr_t addr;
				GetValue(frame, addr);
				
				strstream s;
				s << (void*)addr << ends;
				value = s.str();
				delete[] s.str();
				done = true;
			}
		}

		if (!done)
		{
			strstream s;
			s << '(' << (void*)GetLocation(frame) << ')' << ends;
			value = s.str();
			delete[] s.str();
		}
	}
	catch (HErr& e)
	{
		strstream s;
		s << "•" << (char *)e << "•" << ends;
		value = s.str();
		delete[] s.str();
	}
} // DVariable::GetValueAsText

	// GetValueAsText tries to return a sensible value
void DVariable::GetValueAsText(DNub& nub, ptr_t base, string& value, EVarFormat format) const
{
	BAutolock lock(nub);
	
	ptr_t addr = GetLocation(nub, base);
	
	try
	{
		bool done = false;
	
		if (fType->IsEnumeration() && (format == fmtEnum || format == fmtDefault))
		{
			unsigned long long l = 0;
			char *b = (char*)&l;
			uint32 size = fType->Size(), eValue;
			int32 i = 0;
			
			nub.ReadData(addr, b, size);
#if !__LITTLE_ENDIAN
#	error
#endif
			while (!done && fType->GetNthEnumInfo(i++, value, eValue))
			{
				if (eValue == l)
					done = true;
			}
			
			if (! done)
				format = fmtUnsigned;
		}
		
		if (!done)
		{
			if (fType->IsBase() || (fType->IsEnumeration() && format != fmtEnum))
			{
				char buf[16];
				uint32 size = fType->Size();
				
				nub.ReadData(addr, buf, size);
				FormatBaseType(buf, size, value, format);
				done = true;
			}
			else if (fType->IsString() ||
				(format == fmtString && (fType->IsPointer() || fType->IsReference())))
			{
				FormatString(addr, nub, value);
				done = true;
			}
			else if (fType->IsPointer() || fType->IsReference())
			{
				nub.Read(addr, addr);

				strstream s;
				s << (void*)addr << ends;
				value = s.str();
				delete[] s.str();

				done = true;
			}
		}

		if (!done)
		{
			strstream s;
			s << '(' << (void*)addr << ')' << ends;
			value = s.str();
			delete[] s.str();
		}
	}
	catch (HErr& e)
	{
		strstream s;
		s << "•" << (char *)e << "•" << ends;
		value = s.str();
		delete[] s.str();
	}
} // DVariable::GetValueAsText

void DVariable::SetValue (DStackFrame *frame, DNub *nub, ptr_t addr, const void *data, uint32 size) const
{
	if (frame)
		SetValue(*frame, data, size);
	else
	{
		BAutolock lock(nub);
		nub->WriteData(addr, data, size);
	}
} // DVariable::SetValue

void DVariable::SetValueAsText(DStackFrame *frame, DNub *nub, ptr_t base, const string& value, EVarFormat format) const
{
	ptr_t addr = 0;
	
	if (nub)
		addr = GetLocation(*nub, base);
	
	try
	{
		bool done = false;
	
		if (fType->IsEnumeration() && (format == fmtEnum || format == fmtDefault))
		{
#if !__LITTLE_ENDIAN
#	error
#endif
 			unsigned long long l = 0;
			int32 i = 0;
			string eName;
			uint32 eValue;

			while (!done && fType->GetNthEnumInfo(i++, eName, eValue))
			{
				if (eName == value)
				{
					l = eValue;
					SetValue(frame, nub, addr, &l, fType->Size());
					done = true;
				}
			}

			if (!done)
				format = fmtUnsigned;
		}
		
		if (!done)
		{
			if (fType->IsString() ||
				(format == fmtString && (fType->IsPointer() || fType->IsReference())))
			{
				string nuval;
				
				for (string::const_iterator i = value.begin(); i != value.end(); i++)
				{
					if (*i == '\\')
					{
						switch (*++i)
						{
							case 'r':	nuval += '\r'; break;
							case 'n':	nuval += '\n'; break;
							case 't':	nuval += '\t'; break;
							default:	nuval += *i; break;
						}
					}
					else
						nuval += *i;
				}
				
				if (nuval[0] == '"') nuval.erase(0, 1);
				if (nuval[nuval.length() - 1] == '"') nuval.erase(nuval.length() - 1, 1);
				
				if (fType->IsString())
					SetValue(frame, nub, addr, nuval.c_str(), nuval.size() + 1);
				else
				{
					ptr_t addr = GetLocation(*frame);
					
					DNub& nub = frame->GetNub();
					BAutolock lock(nub);
					
					nub.Read(addr, addr);
					nub.WriteData(addr, nuval.c_str(), nuval.size() + 1);
				}
			}
			else if (fType->IsBase() || (fType->IsEnumeration() && format != fmtEnum))
			{
				if (format == fmtDefault)
					format = DefaultFormat();

				switch (format)
				{
					case fmtSigned:
					{
						long long l = strtoll(value.c_str(), NULL, 10);
						SetValue(frame, nub, addr, &l, min(fType->Size(), sizeof(long long)));
						break;
					}
					
					case fmtUnsigned:
					{
						unsigned long long l = strtoull(value.c_str(), NULL, 10);
						SetValue(frame, nub, addr, &l, min(fType->Size(), sizeof(unsigned long long)));
						break;
					}
					
					case fmtHex:
					{
						const char *s = value.c_str();
						if (strncmp(s, "0x", 2) == 0) s += 2;
						
						unsigned long long l = strtoull(s, NULL, 16);
						SetValue(frame, nub, addr, &l, min(fType->Size(), sizeof(unsigned long long)));
						break;
					}

					case fmtOctal:
					{
						const char *s = value.c_str();
						unsigned long long l = strtoull(s, NULL, 8);
						SetValue(frame, nub, addr, &l, min(fType->Size(), sizeof(unsigned long long)));
						break;
					}

					case fmtChar:
					{
						const char *s = value.c_str();
						unsigned long long l = 0;
						
						if (*s == '\'') s++;
						while (*s)
						{
							if (*s == '\'')
								break;

							l <<= 8;

							if (*s == '\\')
							{
								s++;
								switch (*s)
								{
									case 'r':	l |= '\r'; break;
									case 'n':	l |= '\n'; break;
									case 't':	l |= '\t'; break;
									default:
										if (isdigit(*s))
										{
											THROW(("Unimplemented escapes in character constant"));
										}
										else
											THROW(("Incorrectly formatted character constant"));
								}
							}
							else
								l |= *s;
							
							s++;
						}

						SetValue(frame, nub, addr, &l, min(fType->Size(), sizeof(unsigned long long)));
						break;
					}

					case fmtString:
					{
						THROW(("Impossible (?)"));
						break;
					}

					case fmtFloat:
					{
						if (fType->Size() == 4)
						{
							float f;
							f = strtod(value.c_str(), NULL);
							SetValue(frame, nub, addr, &f, sizeof(float));
						}
						else
						{
							double d;
							d = strtod(value.c_str(), NULL);
							SetValue(frame, nub, addr, &d, sizeof(double));
						}
						break;
					}
					
					default:
						THROW(("Unimplemented"));
				}
			}
			else if (fType->IsPointer() || fType->IsReference())
			{
				THROW(("Not implemented yet"));
//				ptr_t addr;
//				GetValue(frame, addr);
//				
//				strstream s;
//				s << (void*)addr << ends;
//				value = s.str();
//				delete[] s.str();
			}

			done = true;
		}

//		if (!done)
//		{
//			strstream s;
//			s << "(" << (void*)GetLocation(*frame) << ')' << ends;
//			value = s.str();
//			delete[] s.str();
//		}
	}
	catch (HErr& e)
	{
		e.DoError();
	}
} // DVariable::SetValueAsText

void DVariable::SetValueAsText(DStackFrame& frame, const string& value, EVarFormat format) const
{
	SetValueAsText(&frame, NULL, 0, value, format);
} // DVariable::SetValueAsText

void DVariable::SetValueAsText(DNub& nub, ptr_t base, const string& value, EVarFormat format) const
{
	SetValueAsText(NULL, &nub, base, value, format);
} // DVariable::SetValueAsText

void DVariable::FormatBaseType(void *buf, uint32 size, string& value, EVarFormat format) const
{
	char s[64];
	const char kHexChar[] = "0123456789ABCDEF";

	if (format == fmtDefault)
		format = DefaultFormat();
	
	unsigned long long V;

	if (size <= 8)
	{
#if __LITTLE_ENDIAN
		char *p = (char *)&V;

		for (uint32 i = 0; i < 8; i++)
			p[i] = i < size ? ((char *)buf)[i] : 0;
#else
#	error
#endif
	}
	
	switch (format)
	{
		case fmtHex:
		{
			strcpy(s, "0x");
			char *p = s + size * 2 + 2;
			*p-- = 0;
			
			while (size--)
			{
				*p-- = kHexChar[V & 0x000f];
				*p-- = kHexChar[(V & 0x00f0) >> 4];
				V >>= 8;
			}
			break;
		}

		case fmtOctal:
		{
			s[0] = '0';
			uint32 digits = (size * 8 + 2) / 3;
			char* p = s + digits + 1;		// point beyond the initial '0' to the eos position
			*p-- = 0;									// nul terminate

			while (digits--)
			{
				*p-- = kHexChar[V & 0x07];			// write one octal digit
				V >>= 3;
			}
			break;
		}

		case fmtBool:
			sprintf(s, "%s", *(bool *)buf ? "True" : "False");	
			break;

		case fmtFloat:
		{
			double d;
			switch (size)
			{
				case 4:	d = *(float *)buf; break;
				case 8:	d = *(double *)buf; break;
				default:	THROW(("Unsupported float size"));
			}
			sprintf(s, "%g", d);
			break;
		}
		case fmtChar:
		{
			char *t = s;

			while (size--)
			{
				char c = (V >> (size * 8)) & 0x00ff;

				if (iscntrl(c))
				{
					*t++ = '\\';
					switch (c)
					{
						case '\t':	*t++ = 't';	break;
						case 'r':	*t++ = 'r';	break;
						case 'n':	*t++ = 'n';	break;
						default:	sprintf(t, "%o", c); t += strlen(t); break;
					}
				}
				else
					*t++ = c;
			}

			*t = 0;
			break;
		}

		case fmtSigned:
		{
			if (size < 8)		// sign extend a smaller type's value to a long long
			{
				int bSize = size * 8;
				if (V & (1 << (bSize - 1)))
				{
					long long m = ~0;
					m <<= bSize;
					V |= m;
				}
			}

			sprintf(s, "%Ld", V);
			break;
		}

		case fmtUnsigned:
			sprintf(s, "%Lu", V);
			break;
		
		case fmtString:
			strcpy(s, "Not a string");
			break;

		default:
			break;
	}
	
	value = s;
} // DVariable::FormatBaseType

void DVariable::FormatString(ptr_t addr, DNub& nub, string& value) const
{
	BAutolock lock(nub);
	
	if (fType->IsReference())
		nub.Read(addr, addr);

	nub.ReadString(addr, value);
} // DVariable::FormatString

EVarFormat DVariable::DefaultFormat() const
{
	EVarFormat format;

	switch (Type()->Encoding())
	{
		case encVoid:
		case encUnsigned:
		case encUnsignedChar:	
		case encAddress:			format = fmtHex;			break;
		case encBoolean:			format = fmtBool;		break;
		case encFloat:				format = fmtFloat;		break;
		case encSignedChar:		format = fmtChar;		break;
		case encSigned:				format = fmtSigned;		break;
		case encComplexFloat:
			THROW(("How do you format a complex float ???"));
			break;
		default:
			THROW(("Unsupported encoding"));
	}
	
	return format;
} // DVariable::DefaultFormat

bool DVariable::operator== (const DVariable& var)
{
	return
		fLocation.size() == var.fLocation.size() &&
		fName == var.fName &&
		fLocation == var.fLocation;
} // DVariable::operator==

//#pragma mark -

void DMemberVariable::GetValue (const DStackFrame& frame, void *value, uint32& size) const
{
	ptr_t addr = GetLocation(frame);
	
	DNub& nub = frame.GetNub();
	BAutolock lock(nub);

	// don't read more data than requested, EVER
	size = min(size, Type()->Size());

	// if this is a bitfield member, we need to shift it around in 
	uint32 bitSize = Type()->BitSize();
	if (bitSize)
	{
		uint32 bitOffset = Type()->BitOffset();

		// bitfields are at most 8 bytes in current gcc.  this will need to be fixed up
		// if we move to some other compiler or architecture.
		ASSERT(Type()->Size() <= 8);

		// now extract the bitfield member.  bit offsets are 0 == most significant bit, and
		// are expressed relative to the number of bits in the underlying type's size [hence
		// the (8 * Type()->Size()) term below].
		uint64 val = 0;
		uint64* v64 = (uint64*) &val;
		nub.ReadData(addr, v64, size);				// read in the relevant data
		*v64 >>= (8 * Type()->Size()) - bitOffset - bitSize;		// shift it into LSB position
		if (bitSize < 64)
			*v64 &= (1ULL << bitSize) - 1;			// if necessary, mask it away from its neighbors
		memcpy(value, &val, size);					// copy it into the output
	}
	else
	{
		nub.ReadData(addr, value, size);
	}
} // DMemberVariable::GetValue

ptr_t DMemberVariable::GetLocation(const DStackFrame& frame) const
{
	ptr_t addr = fParent.GetLocation(frame);

	if (fLocation.size())
	{
		if (LocationMachine(fLocation, frame, addr, addr) != ltMemory)
			THROW(("•Unsupported location for member•"));
	}
	
	return addr;
} // DMemberVariable::GetLocation

ptr_t DMemberVariable::GetLocation(DNub& nub, ptr_t base) const
{
	ptr_t addr = fParent.GetLocation(nub, base);

	if (fLocation.size())
		addr = LocationMachine(fLocation, nub, addr);
	
	return addr;
} // DMemberVariable::GetLocation

void DMemberVariable::GetLocationString (DLocationString& loc) const
{
	fParent.GetLocationString(loc);
	loc.insert(loc.end(), fLocation.begin(), fLocation.end());
} // DMemberVariable::GetLocationString

//#pragma mark -

ptr_t DInheritedVariable::GetLocation(const DStackFrame& frame) const
{
	ptr_t addr = fParent.GetLocation(frame);

	if (LocationMachine(fLocation, frame, addr, addr) != ltMemory)
		THROW(("•Unsupported location for inherited member•"));
	
	return addr;
} // DInheritedVariable::GetLocation

ptr_t DInheritedVariable::GetLocation(DNub& nub, ptr_t base) const
{
	ptr_t addr = fParent.GetLocation(nub, base);

	addr = LocationMachine(fLocation, nub, addr);
	
	return addr;
} // DInheritedVariable::GetLocation

void DInheritedVariable::GetLocationString (DLocationString& loc) const
{
	fParent.GetLocationString(loc);
	loc.insert(loc.end(), fLocation.begin(), fLocation.end());
} // DInheritedVariable::GetLocationString

//#pragma mark -

DPointerVariable::DPointerVariable(DVariable& pointer)
	: fPointer(pointer)
{
	try
	{
		fType = pointer.Type()->Deref();
	}
	catch (HErr& e)
	{
		fType = new DVoidType;
	}
} // DPointerVariable::DPointerVariable

void DPointerVariable::GetName(string& name) const
{
	fPointer.GetName(name);
	if (fPointer.Type()->IsPointer())
		name = '*' + name;
	else
		name += '&';
} // DPointerVariable::GetName

ptr_t DPointerVariable::GetLocation(const DStackFrame& frame) const
{
	ptr_t addr;

	fPointer.GetValue(frame, addr);

	return addr;
} // DPointerVariable::GetLocation

ptr_t DPointerVariable::GetLocation(DNub& nub, ptr_t base) const
{
	ptr_t addr;

	addr = fPointer.GetLocation(nub, base);
	
	BAutolock lock(nub);
	nub.Read(addr, addr);

	return addr;
} // DPointerVariable::GetLocation

void DPointerVariable::GetLocationString (DLocationString& loc) const
{
	fPointer.GetLocationString(loc);
	loc.push_back(DW_OP_deref);
} // DPointerVariable::GetLocationString

void DPointerVariable::GetValue(const DStackFrame& frame, void *data, uint32& size) const
{
	ptr_t addr;

	fPointer.GetValue(frame, addr);

	DNub& nub(frame.GetNub());
	BAutolock lock(nub);
	
	size = min(size, Type()->Size());
	nub.ReadData(addr, data, size);
} // DPointerVariable::GetValue

//#pragma mark -

void DArrayVariable::GetName (string& name) const
{
	strstream s;
	s << '[' << fIndex << ']' << ends;
	name = s.str();
	delete[] s.str();
} // DArrayVariable::GetName

void DArrayVariable::GetValue (const DStackFrame& frame, void *value, uint32& size) const
{
	ptr_t addr = GetLocation(frame);
	
	DNub& nub = frame.GetNub();
	BAutolock lock(nub);
	
	size = min(size, Type()->Size());
	nub.ReadData(addr, value, size);
} // DArrayVariable::GetValue

ptr_t DArrayVariable::GetLocation (const DStackFrame& frame) const
{
	ptr_t addr = fParent.GetLocation(frame);
	return addr + (Type()->Size() * fIndex);
} // DArrayVariable::GetLocation

ptr_t DArrayVariable::GetLocation (DNub& nub, ptr_t base) const
{
	ptr_t addr = fParent.GetLocation(nub, base);
	return addr + (Type()->Size() * fIndex);
} // DArrayVariable::GetLocation

void DArrayVariable::GetLocationString (DLocationString& loc) const
{
	fParent.GetLocationString(loc);

	uint32 l = Type()->Size() * fIndex;
	char *t = (char *)&l;

	loc.push_back(DW_OP_const4u);
	loc.insert(loc.end(), t, t + 4);
	loc.push_back(DW_OP_plus);
} // DArrayVariable::GetLocationString

//#pragma mark -

DConstVariable::DConstVariable(const string& name, DType *type, const void *data)
	: DVariable(name, type, DLocationString())
{
	fData = new char[type->Size()];
	FailNil(fData);
	memcpy(fData, data, type->Size());
} // DConstVariable::DConstVariable

DConstVariable::~DConstVariable()
{
	delete[] fData;
} // DConstVariable::~DConstVariable

void DConstVariable::GetValue(const DStackFrame& /*frame*/, void *data, uint32& size) const
{
	size = min(size, fType->Size());
	memcpy(data, fData, size);
} // DConstVariable::GetValue

ptr_t DConstVariable::GetLocation(const DStackFrame&) const
{
	THROW(("not an lvalue"));
} // DConstVariable::GetLocation

ptr_t DConstVariable::GetLocation(DNub&, ptr_t) const
{
	THROW(("not an lvalue"));
} // DConstVariable::GetLocation

void DConstVariable::GetLocationString (DLocationString&) const
{
	THROW(("not an lvalue"));
} // DConstVariable::GetLocationString

