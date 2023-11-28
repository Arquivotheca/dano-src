/*	$Id: DFunction.h,v 1.8 1999/05/03 13:09:50 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 11:20:03
*/

#ifndef DFUNCTION_H
#define DFUNCTION_H

#include "DDwarf2.h"
#include "DVariableItem.h"

class DLocals;
class DVariable;

class DDwarfSymFile;

class DFunction
{
  public:
	DFunction () : fLowPC(0), fHighPC(0) {}
	DFunction (const dwarf::entry& e);
	DFunction (const DFunction& f)
		: fInfoEntry (f.fInfoEntry), fLowPC(f.fLowPC), fHighPC(f.fHighPC) {}
	
	void GetName (string& name) const;
	void GetRawName (string& name) const;

	ptr_t LowPC ()											{ return fLowPC; }
	ptr_t HighPC ()											{ return fHighPC; }
	
//	DType GetReturnType ();

	bool operator == (const dwarf::entry& e) const
		{ return fInfoEntry == e; }
	
	void BuildVariableList(varlist& list);
	DVariable* GetVariable(const char *name, ptr_t pc) const;
	
  protected:
  	
	void BuildVariableListForScope(varlist& list, const dwarf::entry& scope);

	dwarf::entry fInfoEntry;
	ptr_t fLowPC, fHighPC;
};

#endif
