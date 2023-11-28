/*	$Id: DDwarfSymFile.h,v 1.9 1999/05/11 21:31:04 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 10:57:26
*/

#ifndef DDWARFSYMFILE_H
#define DDWARFSYMFILE_H

#include "DElfSymFile.h"
#include "DDwarf2.h"
#include "DFunction.h"
#include "DStmtMachine.h"

class DSymWorld;
class DDwarfType;
class DTeam;

class DDwarfSymFile : public DElfSymFile
{
  public:
	DDwarfSymFile (const image_info& info, DTeam* team);
	virtual ~DDwarfSymFile();
	
	// Must be called after valid construction
	virtual void LoadSymbols();

	virtual bool GetStatement(ptr_t pc, DStatement& statement, const DSymWorld& symWorld);
	virtual bool GetStatement(const char* name, DStatement& statement, const DSymWorld& symWorld);
	virtual bool GetAsmStatement(ptr_t pc, DStatement& statement, const DSymWorld& symWorld);
	virtual bool GetSourceStatement(ptr_t pc, DStatement& statement, const DSymWorld& symWorld);
	virtual void GetStopsForFile(DFileNr file, set<int>& lines);
	virtual void GetFunctionName(ptr_t pc, string& name);
	virtual void GetRawFunctionName(ptr_t pc, string& name);
	virtual ptr_t GetStatementOffset(DFileNr file, int line);
	
	virtual DFunction& GetFunction (ptr_t);
	virtual DFunction& GetFunction (const char *name);
	virtual void GetGlobals(std::vector<std::pair<DVariable*, ptr_t> >& vars);
	
	virtual DVariable* GetVariable(const char *name, ptr_t pc) const;
	
	virtual DType* GetType(const char *name) const;
	virtual DType* GetType(TypeID id) const;
	
	virtual void GetFunctionOffsetAndSize(const char *name, ptr_t& offset, int& size);
	virtual void GetFunctionOffsetAndSize(ptr_t pc, ptr_t& offset, int& size);
	
	virtual ptr_t GetFunctionLowPC(ptr_t pc);
	virtual ptr_t GetFunctionHighPC (ptr_t pc);
	
	virtual bool ContainsName(const char *name) const;	// for types, functions and variables
	virtual bool HasSourceInformation() const;

	const dwarf::entry& GetEntryAt(uint32 global_offset) const;

  private:
  	
  	DStmtMachine& GetStmtMachine(int compile_unit_nr);

	virtual DType* GetType(const dwarf::entry& e);
  	
  	dwarf::dwarf2				*fDwarf;

	typedef std::map<ptr_t, DFunction> function_map;
	typedef std::map<uint32, dwarf::entry> type_offset_map;	// offset to DIE mapping

	function_map fFunctions;					// key == highPC
	function_map fInlines;						// key == highPC
	mutable type_offset_map fTypeOffsets;		// where is the concrete entry for the type at the given DIE location?

  	std::map<int,DStmtMachine>	fStmtMachines;
  	std::vector<DVariable*>				fGlobals;

	team_id fTeam;

	void WaitForSymThread(const char*, ...);
	friend int32 load_symbol_thread(void*);
};



#endif
