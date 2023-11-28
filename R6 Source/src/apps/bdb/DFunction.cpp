/*	$Id: DFunction.cpp,v 1.9 1999/05/03 13:09:50 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 13:49:19
*/

#include "bdb.h"
#include <devel/Unmangle.h>
#include "DFunction.h"
#include <strstream>
#include "DDwarfType.h"
#include <stack>

using namespace dwarf;

DFunction::DFunction(const dwarf::entry& e)
	: fInfoEntry(e)
{
	fLowPC = fHighPC = 0;
	fInfoEntry.find_attribute_value(DW_AT_low_pc, fLowPC);
	fInfoEntry.find_attribute_value(DW_AT_high_pc, fHighPC);
} // DFunction::DFunction

void DFunction::GetName (string& name) const
{
	entry e;
	bool mangled = false;
	
	if (fInfoEntry.find_attribute_value(DW_AT_MIPS_linkage_name, name))
		mangled = true;
	else if (! fInfoEntry.find_attribute_value (DW_AT_name, name) &&
			(fInfoEntry.find_attribute_value(DW_AT_abstract_origin, e) ||
			fInfoEntry.find_attribute_value(DW_AT_specification, e)))
	{
		if (e.find_attribute_value(DW_AT_MIPS_linkage_name, name))
			mangled = true;
		else
			e.find_attribute_value(DW_AT_name, name);
	}
	
	if (mangled)
	{
		char buf[UNAME_SIZE];
		if (demangle(name.c_str(), buf, UNAME_SIZE) > 0)
			name = buf;
	}
} // DFunction::GetName

void DFunction::GetRawName (string& name) const
{
	entry e;
	
	if (fInfoEntry.find_attribute_value(DW_AT_MIPS_linkage_name, name))
		;
	else if (fInfoEntry.find_attribute_value(DW_AT_specification, e) ||
		fInfoEntry.find_attribute_value(DW_AT_abstract_origin, e))
	{
		if (! e.find_attribute_value(DW_AT_MIPS_linkage_name, name))
			e.find_attribute_value(DW_AT_name, name);
	}
	else
		fInfoEntry.find_attribute_value(DW_AT_name, name);
} // DFunction::GetRawName

// DType DFunction::GetReturnType ()
// {
// 	
// } // DFunction::GetReturnType

void DFunction::BuildVariableList(varlist& list)
{
	BuildVariableListForScope(list, fInfoEntry);
	
	int i;
	varlist::iterator vi;
	
	for (vi = list.begin(), i = 0; vi != list.end(); vi++, i++)
	{
		if (strlen((*vi)->Text()) == 0)
		{
			char s[32];
			sprintf(s, "param%d", i);
			(*vi)->SetText(s);
		}
	}
} // DFunction::BuildVariableList

void DFunction::BuildVariableListForScope(varlist& list, const entry& scope)
{
	entry::sibling_iterator child;
	ptr_t low, high;
	
	if (!scope.find_attribute_value(DW_AT_low_pc, low)) low = 0;
	if (!scope.find_attribute_value(DW_AT_high_pc, high)) high = 0;
		
	for (child = scope.child_begin (); child != scope.child_end(); child++)
	{
		entry childEntry = (*child);

		if (childEntry.tag () == DW_TAG_formal_parameter ||
			childEntry.tag () == DW_TAG_variable)
		{
			string name;
			entry typeEntry;
			DLocationString location;

			// (*child) may be an abstract reference to some other concrete entry, so we
			// dereference it before extracting the name & type attributes.  note that
			// at most one of these will succeed dereferences will succeed, so it's safe to
			// use this approach (and cleaner than testing for the presence of the attributes
			// *then* repeating the find_attribute_value() call).
			entry concreteEntry = childEntry;
			concreteEntry.find_attribute_value(DW_AT_abstract_origin, concreteEntry);
			concreteEntry.find_attribute_value(DW_AT_specification, concreteEntry);

			if (concreteEntry.find_attribute_value(DW_AT_name, name) &&
				childEntry.find_attribute_value(DW_AT_location, location) &&
				concreteEntry.find_attribute_value(DW_AT_type, typeEntry))
			{
				list.push_back (DVariableItem::CreateItem (name, new DDwarfType(typeEntry), location));
				list.back()->SetScope(low, high);
			}
		}
		else if (childEntry.tag() == DW_TAG_lexical_block)
			BuildVariableListForScope(list, childEntry);
	}
} // DFunction::BuildVariableListForScope

DVariable* DFunction::GetVariable(const char *name, ptr_t pc) const
{
	entry::sibling_iterator child;
	stack<dwarf::entry> s;
	DVariable *var = NULL;

	s.push(fInfoEntry);
	
	while (s.size())
	{
		dwarf::entry scope = s.top();
		
		s.pop();
		
		ptr_t low, high;
		if (scope.find_attribute_value(DW_AT_low_pc, low) && pc < low)
			continue;
		if (scope.find_attribute_value(DW_AT_high_pc, high) && pc >= high)
			continue;
		
		for (child = scope.child_begin (); child != scope.child_end(); child++)
		{
			dwarf::entry childEntry = *child;
			if (childEntry.tag () == DW_TAG_formal_parameter ||
				childEntry.tag () == DW_TAG_variable)
			{
				string vn;
				entry e;
				DLocationString location;
				
				if (childEntry.find_attribute_value(DW_AT_name, vn) &&
					vn == name &&
					childEntry.find_attribute_value(DW_AT_location, location) &&
					childEntry.find_attribute_value(DW_AT_type, e))
				{
					if (var) delete var;	// nested scopes, can happen
					var = new DVariable(name, new DDwarfType(e), location);
				}
			}
			else if (childEntry.tag() == DW_TAG_lexical_block)
				s.push(childEntry);
		}
	}
	
	if (var == NULL)
		THROW(("No variable \"%s\" for this scope", name));
	
	return var;
} // DFunction::GetVariable
