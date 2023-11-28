/*	$Id: DDwarf2.h,v 1.7 1999/05/03 13:09:48 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 10:57:14
*/

#ifndef DDWARF2_H
#define DDWARF2_H

#include "dwarf2.h"

#include <string>
#include <vector>

class DElf;
class DDwarfSymFile;

using namespace std;

namespace dwarf {

// utility routines for reading a LEB128 or unsigned LEB128 from
// a character pointer, thereby increasing the pointer
template <class InputIterator>
inline void ReadLEB128(InputIterator& s, int32& x)
{
	register int32 shift = 0, X = 0;
	int size = sizeof(x) * 8;
	register char b;
	
	do
	{
		b = *s++;
		X |= (b & 0x7f) << shift;
		shift += 7;
	}
	while ((b & 0x80) != 0);
	
	if ((shift < size) && (b & 0x40))
		X |= -(1 << shift);
	
	x = X;
} // ReadLEB128

template <class InputIterator>
inline void ReadULEB128(InputIterator& s, uint32& x)
{
	register uint32 shift = 0, X = 0;
	register char b;
	
	do
	{
		b = *s++;
		X |= (b & 0x7f) << shift;
		shift += 7;
	}
	while ((b & 0x80) != 0);
	
	x = X;
} // ReadLEB128

template<class InputIterator, class T>
inline void Read (InputIterator& s, T& t)
{
	int cnt = sizeof(T);
	char *tc = (char *)&t;
	
	while (cnt--)
		*tc++ = *s++;
} // Read 

// forward declarations
class dwarf2;
class entry;
class compile_unit;
struct attribute;
struct abbreviation;

// attribute is a utility struct that represents an attribute from a
// debug information entry.
struct attribute
{
	uint32			name;
	uint32			form;
	const char	*data;
	
	uint32			data_size ();
};

// abbreviation is a class that 
struct abbreviation
{
	abbreviation (const compile_unit& unit, uint32 offset);
	abbreviation ();
	
 	uint32			code;
  	uint32			tag;
  	bool			children;

	const char	*data;		// pointer into abbreviation table, points to first name/form pair
};

class entry
{
	static char * const end_abbrev_data;
	static const uint32 end_offset;

  public:
  	entry ()
  		: unit (NULL), offset (0) 										{};
  	entry (const entry& e)
  		: unit (e.unit), offset (e.offset), abbrev (e.abbrev)	{}
	entry (const compile_unit *unit, uint32 offset = 0);
	
	uint32 dump (int& level) const;

	uint32 size () const;		// total size of entry in it's info table,
								// including all attribute data

	uint32 code () const		{ return abbrev.code; }
	uint32 tag () const			{ return abbrev.tag; }
	bool has_children () const	{ return abbrev.children; }
	uint32 get_offset() const	{ return offset; }
	
	bool find_attribute_value (uint32 at, uint32& value) const;
	bool find_attribute_value (uint32 at, string& value) const;
	bool find_attribute_value (uint32 at, entry& e) const;
	bool find_attribute_value (uint32 at, vector<char>& data) const;

  private:

	struct attribute_iterator : public iterator<input_iterator_tag, attribute>
	{
	  public:
 	  	attribute_iterator ()
 	  		: abbreviated_data (NULL), unit(NULL) {}
		attribute_iterator (const entry& e, const char* a_data);
		attribute_iterator (const attribute_iterator& at);
		~attribute_iterator();
		attribute_iterator& operator= (const attribute_iterator& at);
		attribute_iterator& operator++ ()
		{
			if (abbreviated_data != end_abbrev_data)
			{
				attr.data += attr.data_size();
				ReadULEB128 (abbreviated_data, attr.name);
				ReadULEB128 (abbreviated_data, attr.form);

				if (attr.name == 0)
					abbreviated_data = end_abbrev_data;	// signals end
			}

			return *this;
		}
		attribute_iterator operator++ (int)
		{
			attribute_iterator t(*this);
			++(*this);
			return t;
		}
		bool operator == (const attribute_iterator& at)
			{ return (abbreviated_data == at.abbreviated_data); }
		bool operator != (const attribute_iterator& at)
			{ return (abbreviated_data != at.abbreviated_data); }
		attribute& operator* ()
			{ return attr; }
		
		void dump() const;
	
	  private:
		const char			*abbreviated_data;
		const compile_unit	*unit;
		attribute				attr;
	};
	
	attribute_iterator attribute_begin () const
		{ return attribute_iterator (*this, unit ? abbrev.data : end_abbrev_data); }
	attribute_iterator attribute_end () const
		{ return attribute_iterator (*this, end_abbrev_data); }

  public:

	class sibling_iterator : public iterator<input_iterator_tag, entry>
	{
	  public:
		sibling_iterator ()
			: unit (NULL), offset (0) {}
		sibling_iterator (const compile_unit *u, uint32 o)
			: unit (u), offset (o) {}
		sibling_iterator (const sibling_iterator& i)
			: unit (i.unit), offset (i.offset) {}
		
		bool operator == (const sibling_iterator& i) const
			{ return unit == i.unit && offset == i.offset; }
		bool operator != (const sibling_iterator& i) const
			{ return unit != i.unit || offset != i.offset; }
		
		entry operator* ()
			{ return entry (unit, offset); }
		
		sibling_iterator& operator++ ()
		{
			if (offset != end_offset)
			{
				entry e(unit, offset);
				uint32 o;
				
				if (e.find_attribute_value(DW_AT_sibling, o))
					offset = o;
				else if (e.has_children())
				{
					// there are children but no explicit sibling pointer, so we have to
					// manually walk the debug info to skip over the children.
					offset += e.size();		// point to the next entry, which is the first child
					int level = 1;				// for our children's children's children...
					do
					{
						// read one entry
						entry e2(unit, offset);
						offset += e2.size();		// point to next entry, i.e. maybe no longer a child
						if (e2.code() == 0)			// was this the end of this sibling chain?
						{
							-- level;						// note that we've backed up one level
						}
						else			// not a null entry, so there are more entries at this level or below
						{
							if (e2.has_children())
							{
								++ level;					// keep track of the child depth
							}
						}
					} while (level);

					// at this point, "offset" points to the original entry's next sibling
				}
				else
					offset += e.size();

				if (entry(unit, offset).tag() == 0)
					offset = end_offset;
			}
			return *this;
		}
		
		sibling_iterator operator++ (int)
		{
			sibling_iterator t = *this;
			++(*this);
			return t;
		}
		
	  private:
		const compile_unit	*unit;
		uint32				offset;
	};

  public:
  	
  	sibling_iterator sibling_begin () const
  		{ sibling_iterator i (unit, unit ? offset : end_offset); return ++i; }
  	sibling_iterator sibling_end () const
 		{ return sibling_iterator (unit, end_offset); }
 	
	sibling_iterator child_begin () const
	{
		if (has_children () && unit)
			return sibling_iterator (unit, offset + size());
		else
			return sibling_iterator (unit, end_offset);
	}
	sibling_iterator child_end () const
		{ return sibling_iterator (unit, end_offset); }

	const compile_unit& get_compile_unit () const
		{ return *unit; }
	
	bool operator == (const entry& e) const
		{ return unit == e.unit && offset == e.offset; }
	
	entry& operator= (const entry& e);
	
  protected:
  	const compile_unit	*unit;		// compilation unit to which this entry belongs
  	uint32					offset;		// starting offset into the .debug_info table
  	abbreviation			abbrev;		// abbreviation describing entries content

	friend class attribute_iterator;
	friend class sibling_iterator;
};

class compile_unit
{
	friend class entry;
	friend struct abbreviation;
	
	static compile_unit no_unit;		// useful
	static const uint32 end_offset;

  public:
  	compile_unit ();
	compile_unit (dwarf2 *inDwarf, uint32 inOffset);
	
	static void init();
	
	void dump() const;

	bool find_entry (uint32 tag, entry& e);
	bool find_entry (uint32 tag, const char *name, entry& e);
	bool find_subprogram (uint32 offset, entry& e);
	
public:
	class entry_iterator : public std::iterator<input_iterator_tag, entry>
	{
	  public:
		entry_iterator ()
			: unit (NULL), offset (0) {}
		entry_iterator (const compile_unit *u, uint32 o)
			: unit (u), offset (o) {}
		entry_iterator (const entry_iterator& i)
			: unit (i.unit), offset (i.offset) {}
		
		bool operator == (const entry_iterator& i)
			{ return unit == i.unit && offset == i.offset; }
		bool operator != (const entry_iterator& i)
			{ return unit != i.unit || offset != i.offset; }
		
		entry operator* ()
			{ return entry (unit, offset); }
		
		entry_iterator& operator++ ()
		{
			if (offset != end_offset)
			{
				offset += entry (unit, offset).size();
				if (offset >= unit->data_size)
					offset = end_offset;
			}
   			return *this;
		}
		
		entry_iterator operator++ (int)
		{
			entry_iterator t = *this;
			++(*this);
			return t;
		}
		
	  private:
		const compile_unit	*unit;
		uint32				offset;
	};

	friend class compile_unit::entry_iterator;
	
  public:
	entry_iterator entry_begin () const
		{ return entry_iterator (this, 11); }
	entry_iterator entry_end () const
		{ return entry_iterator (this, end_offset); }
	
	uint32 size() const						{ return data_size; }
	uint32 info_size() const;
	const char* debug_info() const;
	void use_debug_info() const;
	void release_debug_info() const;
	uint32 info_offset() const			{ return offset; }

	const dwarf2& get_dwarf() const { return *dwarf; }

  protected:
	dwarf2		*dwarf;
	uint32			offset;
	
	uint32			data_size;
	uint16			version;
	uint32			abbrev_offset;
	uchar			address_size;
	
	abbreviation get_abbreviation (uint32 code) const;

	// offsets into the abbreviation table for fast access to abbreviation objects
	vector<uint32> ab_offset;
	
	// What follows is for managing a disk cache
	
	const char* debug_info_from_cache() const;
	static void lock_cache();
	static void unlock_cache();
	
	struct CacheBlock
	{
		dwarf2 *dwarf;
		uint32 offset;
		size_t size;
		const char *block;
		int32 useCount;
	};

	static int compare_use(const void *a, const void *b);
	
	static CacheBlock *sCacheBlocks;
	static uint32 sCacheBlockCount;
	static bool sUseCache, sInited;
	static sem_id sCacheLock;
	static int32 sCacheLockCount;
};

// class dwarf2 encapsulates the DWARF2 data format
class dwarf2
{
	friend class entry;
	friend class compile_unit;
	friend struct abbreviation;

  public:
	dwarf2 (const DElf&, const DDwarfSymFile*);
	~dwarf2 ();

  	void dump () const;
  	
  	typedef vector<compile_unit>::const_iterator unit_iterator;
  	
  	unit_iterator unit_begin () const
  		{ return compile_units.begin(); }
  	
  	unit_iterator unit_end () const
  		{ return compile_units.end(); }
  	
  	unit_iterator unit_for_pc(ptr_t pc) const;
  	
  	const char *get_debug_line () const		{ return debug_line; }
  	
  	compile_unit::entry_iterator get_entry (const char *name);
//  	unit_iterator get_compile_unit_containing (const char *name);

	const DDwarfSymFile& get_symfile () const { return *fSymFile; }

	bool has_info() const { return (info_size > 0); }

  protected:

	void get_debug_info(uint32 offset, const char *& info, size_t& size);

	const DElf& fElf;
	
	// raw dwarf2 data copied from executable 
	const char *debug_aranges, *debug_info, *debug_abbrev, *debug_line, *debug_pubnames; 
	size_t abbrev_size, aranges_size, pubnames_size, info_size;

	// array with the compilation units from this dwarf file
	vector<compile_unit> compile_units;

	// DDwarfSymFile that corresponds to this object
	const DDwarfSymFile* fSymFile;
};

inline const char* compile_unit::debug_info() const
{
	return (sUseCache) ? debug_info_from_cache() : dwarf->debug_info + offset;
} // compile_unit::debug_info

} // namespace

#endif
