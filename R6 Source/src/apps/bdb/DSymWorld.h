/*	$Id: DSymWorld.h,v 1.11 1999/05/11 21:31:08 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/04/98 19:20:37
*/

#ifndef DSYMWORLD_H
#define DSYMWORLD_H

#include "DSymFile.h"
#include "DType.h"

class DFunction;
class DSymFile;
class DTeam;

class DSymWorld {
public:
	DSymWorld(DTeam* team);
	~DSymWorld();
	
		// owner is the team window that wants status information
	void LoadSymbols(team_id, BLooper *owner);
	
	void ImageCreated(const image_info& info, BLooper *owner);
	void ImageDeleted(const image_info& info);
	void TeamDeleted();

	bool GetStatement(ptr_t pc, DStatement& statement) const;
	bool GetStatement(const char* name, DStatement& statement) const;
	bool GetAsmStatement(ptr_t pc, DStatement& statement) const;
	bool GetSourceStatement(ptr_t pc, DStatement& statement) const;
	void GetStopsForFile(DFileNr file, std::set<int>& lines) const;
	ptr_t GetStatementOffset(DFileNr file, int line) const;

	void GetFunctionName(ptr_t pc, string& name) const;
	void GetRawFunctionName(ptr_t pc, string& name) const;
	void GetFunctionOffsetAndSize(const char *name, ptr_t& pc, int& size) const;
	ptr_t GetFunctionLowPC (ptr_t pc) const;
	ptr_t GetFunctionHighPC (ptr_t pc) const;

	DTeam* GetTeam() const;
		
	void GetLocals(std::vector<DVariable*>& vars, ptr_t pc) const;
		// get globals returns a vector with pairs of DVariables and their
		// base address which you have to add to their dwarf location to get the real address
	void GetGlobals(std::vector<std::pair<DVariable*, ptr_t> >& vars) const;

	DVariable* GetVariable(const char *name, ptr_t pc) const;

	DFunction& GetFunction(ptr_t pc) const;
	
	DType* GetType(const char *typeName) const;
	
		// disabled are pc's in .plt e.g.
	bool IsDisabled(ptr_t pc) const;
	
	void GetSourceFiles(std::vector<DFileNr>&) const;
	
protected:
	
	DSymFile* LocatePC(ptr_t pc) const;
	
	void LoadSymbolFile(entry_ref& ref);
	void ReadImageSymbols(const image_info& info, BLooper* owner);
	bool SymbolsLoaded(const image_info&) const;
	
	std::vector<DSymFile*> fSymFiles;
	
	DTeam* fOwningTeam;
};

#endif
