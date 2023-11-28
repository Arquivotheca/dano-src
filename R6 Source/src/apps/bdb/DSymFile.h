/*	$Id: DSymFile.h,v 1.9 1999/05/11 21:31:07 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/23/98 14:51:43
*/

#ifndef DSYMFILE_H
#define DSYMFILE_H

#include "DSourceFileTable.h"
#include "DType.h"

#include <set>
#include <algorithm>

struct DStatement;
class DVariable;
class DFunction;
class DSymWorld;

class DSymFile
{
  public:
	DSymFile(const image_info& info);
	virtual ~DSymFile();

	// Must be called after valid construction
	virtual void LoadSymbols();

	virtual bool GetStatement(ptr_t pc, DStatement& statement, const DSymWorld& symWorld);
	virtual bool GetStatement(const char* name, DStatement& statement, const DSymWorld& symWorld);
	virtual bool GetAsmStatement(ptr_t pc, DStatement& statement, const DSymWorld& symWorld);
	virtual bool GetSourceStatement(ptr_t pc, DStatement& statement, const DSymWorld& symWorld);
	virtual void GetStopsForFile(DFileNr file, std::set<int>& lines);
	virtual ptr_t GetStatementOffset(DFileNr file, int line);
	
	virtual void GetFunctionOffsetAndSize(ptr_t pc, ptr_t& offset, int& size);
	virtual void GetFunctionOffsetAndSize(const char* name, ptr_t& offset, int& size);
	
	virtual void GetFunctionName(ptr_t pc, string& name);
	virtual void GetRawFunctionName(ptr_t pc, string& name);
	virtual ptr_t GetFunctionLowPC(ptr_t pc);
	virtual ptr_t GetFunctionHighPC (ptr_t pc);
	
	virtual DFunction& GetFunction (ptr_t);
	virtual DFunction& GetFunction (const char *name);

	virtual DVariable* GetVariable(const char *name, ptr_t pc) const;
	
	virtual DType* GetType(const char *name) const;
	virtual DType* GetType(TypeID id) const;
	
	virtual void GetLocals(std::vector<DVariable*>& vars, ptr_t pc);
	virtual void GetGlobals(std::vector<std::pair<DVariable*, ptr_t> >& vars);
	
	virtual bool IsDisabled(ptr_t pc) const;
	
	virtual bool ContainsName(const char *name) const;	// for types, functions and variables
	
	virtual bool HasSourceInformation() const;
	
	DFileNr SourceFile(int indx) const			{ return fFiles[indx]; }
	int SourceFileIndx(DFileNr file)				{ return find(fFiles.begin(), fFiles.end(), file) - fFiles.begin(); }
	int SourceFileCount() const					{ return fFiles.size(); }
	
	bool Contains(ptr_t pc) const				{ return (pc >= fBaseAddr && pc < fBaseAddr + fCodeSize) ||
														 (pc >= fDataAddr && pc < fDataAddr + fDataSize); }
	
	image_id ImageID() const					{ return fImage.id; }
	
	ptr_t BaseAddr() const						{ return fBaseAddr; }
	
protected:

	entry_ref fEntry;
	image_info fImage;
	ptr_t fBaseAddr, fDataAddr;
	int32 fCodeSize, fDataSize;
	std::vector<DFileNr> fFiles;
};

#endif
