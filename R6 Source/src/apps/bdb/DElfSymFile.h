/*	$Id: DElfSymFile.h,v 1.5 1999/05/03 13:09:49 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 11/13/98 10:50
*/

#ifndef DELFSYMFILE_H
#define DELFSYMFILE_H

#include "DSymFile.h"
#include "DElf.h"

class DSymWorld;
class BFile;
class DAsmFunction;

// ---------------------------------------------------------------------------
//	class DElfSymFile
// ---------------------------------------------------------------------------

class DElfSymFile : public DSymFile
{
  public:
	DElfSymFile(const image_info& info);
	virtual ~DElfSymFile();

	// Must be called after valid construction
	virtual void LoadSymbols();

	virtual bool GetStatement(ptr_t pc, DStatement& statement, const DSymWorld& symWorld);
	virtual bool GetStatement(const char* name, DStatement& statement, const DSymWorld& symWorld);
	virtual bool GetAsmStatement(ptr_t pc, DStatement& statement, const DSymWorld& symWorld);
	virtual void GetStopsForFile(DFileNr file, std::set<int>& lines);
	virtual ptr_t GetStatementOffset(DFileNr file, int line);

	virtual void GetFunctionOffsetAndSize(const char *name, ptr_t& offset, int& size);
	virtual void GetFunctionOffsetAndSize(ptr_t pc, ptr_t& offset, int& size);
	
	virtual void GetFunctionName(ptr_t pc, string& name);
	virtual void GetRawFunctionName(ptr_t pc, string& name);
	virtual ptr_t GetFunctionLowPC(ptr_t pc);
	virtual ptr_t GetFunctionHighPC(ptr_t pc);
	
	virtual bool IsDisabled(ptr_t pc) const;

	virtual bool ContainsName(const char *name) const;	// for types, functions and variables

	class DAsmFile;
	// An actual symbol entry in the map
	struct SymTabEntry
	{
		// lowPC is the offset of the function
		// highPC is the first location past the last machine instruction (ie: lowPC of entry + 1)
		uint32			lowPC;
		uint32			highPC;
		uint32			name;
		DAsmFile*		asmFile;
		bool operator< (const SymTabEntry& e) const
			{ return lowPC < e.lowPC; }
	};

	// Utility classes for the assembly output from functions
	class DAsmFile
	{
	public:
							DAsmFile(DFileNr file, const SymTabEntry& symbol);
							
		void 				Disasm(BFile& textFile, const char* codeBuffer, ptr_t baseAddress, const DSymWorld& symWorld);
		void				FillStatement(ptr_t pc, DStatement& statement);
		void				GetStopsForFile(std::set<int>& lines);
		ptr_t				GetStatementOffset(int line);
								
		DFileNr 			fFile;
		const SymTabEntry&	fSymbol;
		std::vector<int32>		fLineSizes;
		std::set<int> 	fStops;
	};
	
protected:
	typedef std::vector<SymTabEntry>	symtab;
	typedef std::vector<DAsmFile*> 	AsmFileTab;
	
	symtab::const_iterator 	FindEntry(const char *name) const;
	DAsmFile*						CreateAsmFile(const SymTabEntry& symbol, const DSymWorld& symWorld);
	bool 									GetStatement(SymTabEntry& symbol, ptr_t pc, DStatement& statement, const DSymWorld& symWorld);

	DElf						fElf;
	const char*					fStrtab;
//	const char*					fHash;
	symtab 							fSymtab;
	AsmFileTab					fAsmFileTable;
};

#endif
