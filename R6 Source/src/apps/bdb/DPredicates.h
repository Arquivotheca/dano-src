/*	$Id: DPredicates.h,v 1.4 1999/03/05 14:25:16 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 10:57:38
*/

#ifndef CPREDICATES_H
#define CPREDICATES_H

#include "DFunction.h"

#include <algorithm>

// predicates

class PC_Pred
{
  public:
	PC_Pred(ptr_t ipc) : pc(ipc) {};
	bool operator () (const dwarf::entry& e)
	{
		ptr_t low, high;

		return
			e.tag() == DW_TAG_subprogram &&
			e.find_attribute_value(DW_AT_low_pc, low) &&
			e.find_attribute_value(DW_AT_high_pc, high) &&
			pc >= low && pc < high;
	}
	ptr_t pc;
};

class PC_Inline_Pred
{
  public:
	PC_Inline_Pred(ptr_t ipc) : pc(ipc) {};
	bool operator () (const dwarf::entry& e)
	{
		ptr_t low, high;

		return
			e.tag() == DW_TAG_inlined_subroutine &&
			e.find_attribute_value(DW_AT_low_pc, low) &&
			e.find_attribute_value(DW_AT_high_pc, high) &&
			pc >= low && pc < high;
	}
	ptr_t pc;
};

class Tag_Pred
{
  public:
	Tag_Pred(uint32 itag) : tag (itag) {};
	bool operator () (const dwarf::entry& e)
		{ return e.tag() == tag; }
	uint32 tag;
};

class PC_UnitPred
{
  public:
	PC_UnitPred(ptr_t ipc) : pc(ipc) {};
	bool operator () (const dwarf::compile_unit& unit)
	{
		ptr_t low, high;

		dwarf::compile_unit::entry_iterator ei =
			find_if (unit.entry_begin(), unit.entry_end(), Tag_Pred(DW_TAG_compile_unit));

		return
			ei != unit.entry_end () &&
			(*ei).find_attribute_value(DW_AT_low_pc, low) &&
			(*ei).find_attribute_value(DW_AT_high_pc, high) &&
			pc >= low && pc < high;
	}
	ptr_t pc;
};

class IsFunc
{
  public:
	IsFunc (const dwarf::entry& e) : fEntry (e) {};
	bool operator () (const DFunction& f) const { return f == fEntry; }
	bool operator () (const pair<ptr_t,DFunction>& f) const { return f.second == fEntry; }
	const dwarf::entry& fEntry;
};



#endif
