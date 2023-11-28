/*	$Id: DDwarf2.cpp,v 1.12 1999/05/05 19:48:36 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 13:48:55
*/

#include "bdb.h"
#include "DElf.h"
#include "DDwarf2.h"

#include <iostream>
#include <fstream>
#include <algorithm>

using namespace dwarf;

static void dump_hex (const char *s, size_t size, char *d)
{
	const char h[] = "0123456789abcdef";
	
	unsigned char b;

	while (b = *(unsigned char *)s++, size--)
		*d++ = h[b >> 4], *d++ = h[b & 0x0f];
	
	*d = 0;
}

// dump a location atom into a string
static void dump_location(const char* atom, size_t size, char* outstring)
{
	uint32 len;

	unsigned char opcode = *((const unsigned char*) atom);
	switch (opcode)
	{
	// 1-byte constants
	case DW_OP_const1u:
	case DW_OP_const1s:
	case DW_OP_pick:
	case DW_OP_deref_size:
	case DW_OP_xderef_size:
		dump_hex(atom + 1, 1, outstring); break;

	// 2-byte constants
	case DW_OP_const2u:
	case DW_OP_const2s:
	case DW_OP_skip:
	case DW_OP_bra:
		dump_hex(atom + 1, 2, outstring); break;

	// ULEB128/SLEB128 numbers
	case DW_OP_constu:
	case DW_OP_consts:	// we don't care about signedness
	case DW_OP_plus_uconst:
	case DW_OP_breg0:
	case DW_OP_regx:
	case DW_OP_fbreg:
	case DW_OP_piece:
		atom++;
		ReadULEB128(atom, len); sprintf(outstring, "0x%lx", len); break;

	// 2 {U,S}LEB128 args
	case DW_OP_bregx:
		ReadULEB128(atom, len); sprintf(outstring, "0x%lx, ", len); atom+= len;
		ReadULEB128(atom, len); sprintf(outstring + strlen(outstring), "0x%lx", len); break;

	default:
		dump_hex(atom, size, outstring);
		break;
	}
}

//#pragma mark -

dwarf2::dwarf2 (const DElf& elf, const DDwarfSymFile* symfile)
	: fElf(elf), fSymFile(symfile)
{
	size_t size;
	
		// initialize cache
	compile_unit::init();

	elf.LoadSection(".debug_aranges", debug_aranges, aranges_size);
	elf.LoadSection(".debug_line", debug_line, size);
	elf.LoadSection(".debug_pubnames", debug_pubnames, pubnames_size);

#if 0
	if (gPrefs->GetPrefInt("use cache"))
	{
		info_size = fElf.SectionSize(".debug_info");
		debug_info = NULL;
	}
	else
#endif
	{
		info_size = 0;
		elf.LoadSection(".debug_info", debug_info, info_size);
	}

// load .debug_abbrev info and make a table for fast access
	elf.LoadSection(".debug_abbrev", debug_abbrev, abbrev_size);
	
	uint32 x = 0;

	do
	{
		compile_units.push_back(compile_unit(this, x));
		x += 4 + compile_units.back().size();
	}
	while (x < info_size);
} // dwarf2::dwarf2

dwarf2::~dwarf2 ()
{
	delete[] debug_aranges;
	delete[] debug_info;
	delete[] debug_line;
	delete[] debug_pubnames;
	delete[] debug_abbrev;
} // dwarf2::~dwarf2

void dwarf2::dump () const
{
	for (unit_iterator i = unit_begin(); i != unit_end(); i++)
		(*i).dump();
} // dwarf2::dump

compile_unit::entry_iterator dwarf2::get_entry (const char *name)
{
	const char *t = debug_pubnames;

	do
	{
		uint32 cu_data_size;		// size of .debug_pubnames data for this compilation unit (CU)
//		uint16 dwarf_version;	// dwarf version number (not used here)
		uint32 cu_start;				// offset of of CU start in .debug_info
//		uint32 cu_size;				// size of CU data in .debug_info (not used here)
		const char* current_t = t;		// remember start of this CU's pubname data

		Read(t, cu_data_size);
//		Read(t, dwarf_version);
		t += 2;		// don't bother with the dwarf version number
		Read(t, cu_start);
//		Read(t, cu_size);
		t += 4;		// don't bother with the compilation-unit size field

		// find the compile_unit() object at this point in the data
		for (unit_iterator ui = unit_begin(); ui != unit_end(); ui++)
		{
			if ((*ui).info_offset() == cu_start)		// this is it!
			{
				uint32 routine_offset;
				do		// check all names now within this CU
				{
					Read(t, routine_offset);
					if (routine_offset)
					{
						if (!strcmp(name, t))	// is this it?
						{
							return compile_unit::entry_iterator (&(*ui), routine_offset);
						}

						// this isn't it; advance to the next offset/name pair
						t += strlen(t) + 1;
					}
				} while (routine_offset);
				break;
			}
		}
		t = current_t + 4 + cu_data_size;
	} while (t < debug_pubnames + pubnames_size);

	// didn't find it
	return (*unit_begin()).entry_end();
}

dwarf2::unit_iterator dwarf2::unit_for_pc(ptr_t pc) const
{
	struct aranges
	{
		uint32		length;
//		uint16		version;				// why didn't those %^#$%$ make these fields aligned???
//		uint32		info_offset;
//		uint8		address_size;
//		uint8		segment_desc_size;
		uint8		pad[12];
		struct
		{
			uint32 offset;
			uint32 length;
		} table[1];
	};
	
	uint32 offset = 0, nr = 0;
	while (offset < aranges_size)
	{
		aranges *a = (aranges *)(debug_aranges + offset);
		
		int cnt = (a->length - 16) / 8;
		
		for (int i = 0; i < cnt; i++)
		{
			if (pc >= a->table[i].offset && pc < a->table[i].offset + a->table[i].length)
			{
				uint32 info_offset = *(uint32*)&a->pad[2];

				unit_iterator ui = unit_begin() + nr;
				
					// sometimes the index found is the same as the compile_unit index...
				if ((*ui).info_offset() == info_offset)
					return ui;

					// ...if not we'll have to look for it
				for (ui = unit_begin(); ui != unit_end(); ui++)
				{
					if ((*ui).info_offset() == info_offset)
						return ui;
				}
				
					// could not find the compile unit, weird....
				return unit_end();
			}
		}
		
		offset += a->length + sizeof(uint32);
		nr++;
	}
	
	return unit_end();
} // dwarf2::unit_for_pc

void dwarf2::get_debug_info(uint32 offset, const char *& info, size_t& size)
{
	fElf.LoadDebugInfoPart(offset, info, size);
} // dwarf2::get_debug_info

//#pragma mark -

const uint32 compile_unit::end_offset = 0xFFFFFFFFUL;
uint32 compile_unit::sCacheBlockCount = 0;
compile_unit::CacheBlock *compile_unit::sCacheBlocks = NULL;
bool compile_unit::sUseCache, compile_unit::sInited;
sem_id compile_unit::sCacheLock;
int32 compile_unit::sCacheLockCount;

void compile_unit::init()
{
	if (!sInited)
	{
#if 0
		sUseCache = gPrefs->GetPrefInt("use cache", 1);

		if (sUseCache)
		{
			sCacheBlockCount = gPrefs->GetPrefInt("cache block count", 20);
			sCacheBlocks = new CacheBlock[sCacheBlockCount];
			memset(sCacheBlocks, 0, sCacheBlockCount * sizeof(CacheBlock));
		}
#else
		sUseCache = 0;
#endif
		
		FailOSErr2(sCacheLock = create_sem(0, "CacheLock"));
		sCacheLockCount = 0;
		
		sInited = true;
	}
} // compile_unit::init

compile_unit::compile_unit ()
	: dwarf (NULL), offset (0)
{
} // compile_unit::compile_unit

	// since we don't use the values in the constructor of compile_unit, we
	// might as well use our intimate knowledge of ULEB128's by skipping
	// over them instead of reading them. That results in a dramatic performance
	// boost
#define SkipULEB128(s)	while ((*s++) & 0x80) {}

compile_unit::compile_unit (dwarf2 *inDwarf, uint32 inOffset)
	: dwarf (inDwarf), offset (inOffset)
{
	const char *data = debug_info();
	
	Read(data, data_size);
	Read(data, version);
	Read(data, abbrev_offset);
	Read(data, address_size);
	
	ab_offset.push_back(0);
	
	const char *a = dwarf->debug_abbrev + abbrev_offset, *t = a;

	while (true)
	{
		uint32 off = t - a;

		// sanity check; bail if we're off the end of the .debug_abbrev data
		if (off >= dwarf->abbrev_size - abbrev_offset)
			break;
			
			// an abbreviation code of 0 signifies the end of the list
		if (a[off] == 0)
			break;
		
		ab_offset.push_back(off);
		
		SkipULEB128(t);
		SkipULEB128(t);
		++t;
		
		char b;
		do
		{
			b = *t;
			SkipULEB128(t);
			SkipULEB128(t);
		}
		while (b != 0);
	}
	
	release_debug_info();
} // compile_unit::compile_unit

abbreviation compile_unit::get_abbreviation (uint32 code) const
{
	if (code < 1 || code >= ab_offset.size() || ab_offset[code] == 1)
		THROW(("code index out of bounds (%04x)", code));

	if (code != abbreviation (*this, ab_offset[code]).code)
		THROW(("Debug information is inconsistent."));

	return abbreviation (*this, ab_offset[code]);
} // compile_unit::get_abbreviation

void compile_unit::dump () const
{
	cout << "\nCompilation unit" << endl;
	
	int level = 0;
	
	for (entry_iterator i = entry_begin(); i != entry_end(); i++)
		(*i).dump(level);
} // compile_unit::dump

int compile_unit::compare_use(const void *va, const void *vb)
{
	const CacheBlock *a = (const CacheBlock*)va, *b = (const CacheBlock*)vb;
	
	return b->useCount - a->useCount;
} // compile_unit::compare_use

uint32 
compile_unit::info_size() const
{
	return dwarf->info_size;
}

const char* compile_unit::debug_info_from_cache () const
{
	lock_cache();

	const char *result = NULL;
	CacheBlock t;
	uint32 i;
	
	for (i = 0; i < sCacheBlockCount; i++)
	{
		if (sCacheBlocks[i].dwarf == dwarf &&
			sCacheBlocks[i].offset == offset)
		{
			if (i > 0)
			{
				t = sCacheBlocks[i];
				
				memmove(sCacheBlocks + 1, sCacheBlocks, i * sizeof(CacheBlock));
				
				sCacheBlocks[0] = t;
			}
			
			sCacheBlocks[0].useCount++;
			result = sCacheBlocks[0].block;
			break;
		}
		else if (sCacheBlocks[i].dwarf == NULL)
			break;
	}
	
	if (result == NULL)
	{
		try
		{
			dwarf->get_debug_info(offset, t.block, t.size);

			while (sCacheBlocks[sCacheBlockCount - 1].block &&
				sCacheBlocks[sCacheBlockCount - 1].useCount > 0)
			{
					// sort the cache block to their relative use
				qsort(sCacheBlocks, sCacheBlockCount, sizeof(CacheBlock), compare_use);

				unlock_cache();
				snooze(10000);
				lock_cache();
			}
			
			if (sCacheBlocks[sCacheBlockCount - 1].block)
				delete[] sCacheBlocks[sCacheBlockCount - 1].block;
			
			memmove(sCacheBlocks + 1, sCacheBlocks, sizeof(CacheBlock) * (sCacheBlockCount - 1));
			
			sCacheBlocks[0] = t;
			sCacheBlocks[0].dwarf = dwarf;
			sCacheBlocks[0].offset = offset;
			sCacheBlocks[0].useCount = 1;
		
			result = sCacheBlocks[0].block;
		}
		catch (...)
		{
			unlock_cache();
			throw;
		}
	}
	
	unlock_cache();
	
	return result;
} // compile_unit::debug_info_from_cache

void compile_unit::use_debug_info() const
{
	if (sUseCache)
	{
		lock_cache();
		
		for (uint32 i = 0; i < sCacheBlockCount; i++)
		{
			if (sCacheBlocks[i].dwarf == dwarf &&
				sCacheBlocks[i].offset == offset)
			{
				sCacheBlocks[i].useCount++;
				break;
			}
			else if (sCacheBlocks[i].block == NULL)
				break;
		}
		
		unlock_cache();
	}
} // compile_unit::use_debug_info

void compile_unit::release_debug_info() const
{
	if (sUseCache)
	{
		lock_cache();
		
		for (uint32 i = 0; i < sCacheBlockCount; i++)
		{
			if (sCacheBlocks[i].dwarf == dwarf &&
				sCacheBlocks[i].offset == offset)
			{
				sCacheBlocks[i].useCount--;

				if (sCacheBlocks[i].useCount < 0)	// should never happen...
					sCacheBlocks[i].useCount = 0;
				break;
			}
			else if (sCacheBlocks[i].block == NULL)
				break;
		}
		
		unlock_cache();
	}
} // compile_unit::release_debug_info

void compile_unit::lock_cache()
{
	if (atomic_add(&sCacheLockCount, 1) > 0)
		acquire_sem(sCacheLock);
} // compile_unit::lock_cache

void compile_unit::unlock_cache()
{
	if (atomic_add(&sCacheLockCount, -1) > 1)
		release_sem(sCacheLock);
} // compile_unit::unlock_cache

//#pragma mark -

uint32 attribute::data_size()
{
	const char *t = data;
	uint32 f = form;
			
	if (f == DW_FORM_indirect)
		ReadULEB128(t, f);
	
	uint32 udata;
	int32 sdata;
	uint16 data2;
	
	switch (f)
	{
		case DW_FORM_block1:	udata = *t++; t += udata; break;
		case DW_FORM_block2:	Read(t, data2); t += udata; break;
		case DW_FORM_block4:	Read(t, udata); t += udata; break;
		case DW_FORM_block:		ReadULEB128(t, udata); t += udata; break;
		
		case DW_FORM_ref1:
		case DW_FORM_data1:
		case DW_FORM_flag:		t++; break;

		case DW_FORM_data2:
		case DW_FORM_ref2:		t += 2; break;

		case DW_FORM_addr:		
		case DW_FORM_ref4:
		case DW_FORM_ref_addr:
		case DW_FORM_strp:
		case DW_FORM_data4:		t += 4; break;

		case DW_FORM_ref8:
		case DW_FORM_data8:		t += 8; break;

		case DW_FORM_string:	t += strlen(t) + 1; break;

		case DW_FORM_sdata:		ReadLEB128(t, sdata); break;

		case DW_FORM_udata:
		case DW_FORM_ref_udata:	ReadULEB128(t, udata); break;
	}
	
	return t - data;
} // attribute::data_size

//#pragma mark -

char * const entry::end_abbrev_data = reinterpret_cast<char*>(0xffffffffUL);
const uint32 entry::end_offset = 0xFFFFFFFFUL;

entry::attribute_iterator::attribute_iterator (const dwarf::entry& e, const char *data)
	: unit(e.unit)
{
	if (unit) unit->use_debug_info();
	
	if (data == NULL)
	{
		abbreviated_data = end_abbrev_data;
	}
	else if ((abbreviated_data = data) != end_abbrev_data)
	{
		ReadULEB128(abbreviated_data, attr.name);
		ReadULEB128(abbreviated_data, attr.form);
	
// watch out! The next lines assume that data points to the first
// entry in the abbrev table for the current entry.
		
		ASSERT_OR_THROW (e.abbrev.data == data);

		if (attr.name)
		{
			attr.data = e.unit->debug_info() + e.offset;
			unit->release_debug_info();	// since we already referenced it at the beginning
		
			uint32 dummy;
			ReadULEB128 (attr.data, dummy);
		}
		else
		{
			attr.data = NULL;
			abbreviated_data = end_abbrev_data;
		}
	}
} // attribute_iterator::attribute_iterator

entry::attribute_iterator::attribute_iterator (const entry::attribute_iterator& at)
	: abbreviated_data (at.abbreviated_data), unit(at.unit), attr (at.attr)
{
	if (unit)
		unit->use_debug_info();
} // attribute_iterator::attribute_iterator

entry::attribute_iterator::~attribute_iterator()
{
	if (unit)
		unit->release_debug_info();
} // entry::attribute_iterator::~attribute_iterator

entry::attribute_iterator& entry::attribute_iterator::operator= (const entry::attribute_iterator& at)
{
	if (unit != at.unit)
	{
		if (unit) unit->release_debug_info();
		unit = at.unit;
		if (unit) unit->use_debug_info();
	}
	else
		unit = at.unit;

	abbreviated_data = at.abbreviated_data;
	attr = at.attr;

	return *this;
} // entry::attribute_iterator::operator=

//void dwarf::entry::attribute_iterator::dump() const
//{
//	cout << "data: " << hex << (unsigned long)abbreviated_data << "attr: " << hex << attr.name << endl;
//} // dwarf::entry::attribute_iterator::dump

//#pragma mark -

entry::entry (const compile_unit *u, uint32 info)
	: unit(u), offset(info)
{
	const char *t = unit->debug_info() + offset;

	uint32 code;
	ReadULEB128(t, code);
	
	if (code)
		abbrev = unit->get_abbreviation(code);
	else
		abbrev.code = abbrev.tag = 0;
	
	unit->release_debug_info();
} // entry::entry

#include "dwarfnames.h"

static void indent(int level)
{
	for (int i = 0; i < level; i++)
		cout << '\t';
}

uint32 entry::dump (int& level) const
{
	if (code () == 0)
	{
		cout << '\n';
		indent(level);
		cout << "NULL entry : offset " << offset - 1 << endl;
		if (level > 0)
			level--;
		return 0;
	}

	const char *tag = abbrev.tag <= 0x35 ? kTagName[abbrev.tag] : "unknown tag";

	cout << '\n';
	indent(level);
	cout << "info entry " << tag << " : offset " << offset - 1 << endl;

	for (attribute_iterator i = attribute_begin(); i != attribute_end(); i++)
	{
		uint32 form = (*i).form;
		const char *t = (*i).data;
		
		if (form == DW_FORM_indirect)
			ReadULEB128(t, form);
		
		char s[256];
		uint32 data;
		int32 sdata;
		uint16 data2;
		
		switch (form)
		{
			case DW_FORM_addr:		Read(t, data); sprintf(s, "0x%08lx", data); break;
			case DW_FORM_block:		ReadULEB128(t, data); dump_hex(t, min(255, (int)data), s); t += data; break;
			case DW_FORM_block1:	data = *t++; dump_location(t, data, s); t += data; break;
			case DW_FORM_block2:	Read(t, data2); dump_hex(t, min(255, (int)data), s); t += data; break;
			case DW_FORM_block4:	Read(t, data); dump_hex(t, min(255, (int)data), s); t += data; break;
			case DW_FORM_data1:		sprintf(s, "%d", *t++); break;
			case DW_FORM_data2:		Read(t, data2); sprintf(s, "%d", data2); break;
			case DW_FORM_data4:		Read(t, data); sprintf(s, "%ld", data); break;
			case DW_FORM_data8:		{ long long l; Read(t, l); sprintf(s, "%Ld", l); break; }
			case DW_FORM_string:	strcpy(s, t); t += strlen(s) + 1; break;
			case DW_FORM_flag:		sprintf(s, "%s", *t++ ? "on" : "off"); break;
			case DW_FORM_sdata:		ReadLEB128(t, sdata); sprintf(s, "%ld", sdata); break;
			case DW_FORM_strp:		Read(t, data); sprintf(s, "string %ld", data); break;
			case DW_FORM_udata:		ReadULEB128(t, data); sprintf(s, "%ld", data); break;
			case DW_FORM_ref_addr:	Read(t, data); sprintf(s, "ref 0x%08lx", data); break;
			case DW_FORM_ref1:		sprintf(s, "ref %d", *t++); break;
			case DW_FORM_ref2:		Read(t, data2); sprintf(s, "ref %d", data2); break;
			case DW_FORM_ref4:		Read(t, data); sprintf(s, "ref %ld", data); break;
			case DW_FORM_ref8:		{ long long l; Read(t, l); sprintf(s, "ref %Ld", l); break; }
			case DW_FORM_ref_udata:	ReadULEB128(t, data); sprintf(s, "%ld", data); break;
		}

		const char* name;
		if ((*i).name == 0x2007) name = "DW_AT_MIPS_linkage_name";
		else name = (*i).name <= 0x4d ? kDWName[(*i).name] : "undefined name...";

		const char* formName = (*i).form <= 0x16 ? kFormName[(*i).form] : "unknown form...";

		indent(level);
		cout << "\tattribute: " << name << ", form: " << formName << ", data: \"" << s << "\"" << endl;

	}

	if (abbrev.children)
		level++;
	
	return size();
} // entry::dump

uint32 entry::size () const
{
	// don't crash when asking for the size of a default-constructed entry
	if (!unit) return 0;

	const char *dbi = unit->debug_info(), *t = dbi + offset;

	uint32 code; // we start with a unsigned leb128 with the code
	ReadULEB128 (t, code);
	
	uint32 result = t - dbi - offset;
	
	if (code != 0)
	{
		for (attribute_iterator i = attribute_begin(); i != attribute_end(); i++)
			result += (*i).data_size();
	}

	unit->release_debug_info();

	return result;
} // entry::size

entry& entry::operator= (const entry& e)
{
	unit = e.unit;
	offset = e.offset;
	abbrev = e.abbrev;
	
	return *this;
} // entry::operator=

class AttributePred
{
  public:
	AttributePred (uint32 attribute) : at (attribute) {};
	bool operator () (const attribute& attr) { return attr.name == at; }
	uint32 at;
};

bool entry::find_attribute_value (uint32 at, uint32& value) const
{
	bool result = true;
	attribute_iterator i = find_if (attribute_begin (), attribute_end (), AttributePred (at));

	if (i != attribute_end())
	{
//		uint32 size;
		uint16 x;
		int32 sdata;
		unsigned const char *t = (unsigned const char *)(*i).data;

		switch ((*i).form)
		{
// 			case DW_FORM_block1:	data = *t++; t += data; break;
// 			case DW_FORM_block2:	Read(t, data2); t += data; break;
// 			case DW_FORM_block4:	Read(t, data); t += data; break;
// 			case DW_FORM_block:		ReadULEB128(t, data); t += data; break;
			case DW_FORM_block1:
			case DW_FORM_block2:
			case DW_FORM_block4:
			case DW_FORM_block:		result = false; break;
			
			case DW_FORM_ref1:
			case DW_FORM_data1:
			case DW_FORM_flag:		value = *t; break;

			case DW_FORM_data2:
			case DW_FORM_ref2:		Read(t, x); value = x; break;

			case DW_FORM_addr:		
			case DW_FORM_ref4:
			case DW_FORM_ref_addr:
			case DW_FORM_strp:
			case DW_FORM_data4:		Read(t, value); break;

			case DW_FORM_ref8:
			case DW_FORM_data8:		{ long long l; Read(t, l); value = l; break; }

			case DW_FORM_string:	result = false; break;

			case DW_FORM_sdata:		ReadLEB128(t, sdata); value = sdata; break;

			case DW_FORM_udata:
			case DW_FORM_ref_udata:	ReadULEB128(t, value); break;
		}
	}
	else
		result = false;
	
	return result;
} // entry::find_attribute

bool entry::find_attribute_value (uint32 at, string& value) const
{
	bool result = false;
	attribute_iterator i = find_if (attribute_begin (), attribute_end (), AttributePred (at));

	if (i != attribute_end())
	{
		if ((*i).form == DW_FORM_string)
		{
			value = (*i).data;
			result = true;
		}
	}
	
	return result;
} // entry::find_attribute

bool entry::find_attribute_value (uint32 at, entry& value) const
{
	bool result = true;
	attribute_iterator i = find_if (attribute_begin (), attribute_end (), AttributePred (at));

	uint32 o;

	if (i != attribute_end())
	{
		uint16 x;
		const char *t = (*i).data;

		switch ((*i).form)
		{
			case DW_FORM_ref1:
				o = *t;
				break;

			case DW_FORM_ref2:
				Read(t, x);
				o = x;
				break;

			case DW_FORM_ref4:
				Read(t, o);
				break;

			case DW_FORM_ref8:
			{
				long long l;			// truncate the value and hope it fits...
				Read(t, l);
				o = l;
				break;
			}
			case DW_FORM_ref_udata:
				ReadULEB128(t, o);
				break;

			default:
				result = false;
				break;
		}
	}
	else
		result = false;

	if (result)
		value = entry(unit, o);
	
	return result;
} // entry::find_attribute

bool entry::find_attribute_value (uint32 at, vector<char>& value) const
{
	bool result = false;
	attribute_iterator i = find_if (attribute_begin (), attribute_end (), AttributePred (at));
	
	value.clear();

	if (i != attribute_end())
	{
		const char *data = (*i).data;
		uint32 size;
		
		switch ((*i).form)
		{
 			case DW_FORM_block1:
 				size = *data++;
				result = true;
 				break;

 			case DW_FORM_block2:
 				size = *(ushort*)data;
 				data += 2;
				result = true;
 				break;

 			case DW_FORM_block4:
 				size = *(uint32*)data;
 				data += 4;
				result = true;
 				break;

 			case DW_FORM_block:
 				ReadULEB128(data, size);
				result = true;
 				break;
		}
		
		if (result)
			value.insert(value.begin(), data, data + size);
	}
	
	return result;
} // entry::find_attribute

//#pragma mark -

abbreviation::abbreviation (const compile_unit& unit, uint32 offset)
{
	data = unit.dwarf->debug_abbrev + unit.abbrev_offset + offset;
	
	ReadULEB128(data, code);
	ReadULEB128(data, tag);
	children = *data++;
} // abbreviation

abbreviation::abbreviation ()
{
	data = NULL;
} // abbreviation::abbreviation

