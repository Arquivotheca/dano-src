/*	$Id: DSymFile.cpp,v 1.10 1999/05/11 21:31:06 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/23/98 15:30:58
*/

#include "bdb.h"
#include "DSymFile.h"
#include "DStatement.h"
#include <strstream>
#include "DFunction.h"

DSymFile::DSymFile(const image_info& info)
{
	FailOSErr(get_ref_for_path(info.name, &fEntry));
	fImage = info;
	fBaseAddr = (ptr_t)info.text;
	fDataAddr = (ptr_t)info.data;
	fCodeSize = info.text_size;
	fDataSize = info.data_size;
} /* DSymFile::DSymFile */

DSymFile::~DSymFile()
{
} /* DSymFile::~DSymFile */

void DSymFile::LoadSymbols()
{
} /* DSymFile::LoadSymbols */

bool DSymFile::GetStatement(ptr_t /*pc*/, DStatement& /*statement*/, const DSymWorld& /*symWorld*/)
{
	return false;
} /* DSymFile::GetStatement */

bool DSymFile::GetAsmStatement(ptr_t /*pc*/, DStatement& /*statement*/, const DSymWorld& /*symWorld*/)
{
	return false;
} /* DSymFile::GetAsmStatement */

bool DSymFile::GetSourceStatement(ptr_t /*pc*/, DStatement& /*statement*/, const DSymWorld& /*symWorld*/)
{
	return false;
} /* DSymFile::GetSourceStatement */

bool DSymFile::GetStatement(const char* /*name*/, DStatement& /*statement*/, const DSymWorld& /*symWorld*/)
{
	return false;
} /* DSymFile::GetStatement */

void DSymFile::GetStopsForFile(DFileNr /*file*/, set<int>& /*lines*/)
{
} /* DSymFile::GetStopsForFile */

ptr_t DSymFile::GetStatementOffset(DFileNr /*file*/, int /*line*/)
{
	return 0;
} /* DSymFile::GetStatementOffset */

void DSymFile::GetFunctionOffsetAndSize(ptr_t /*pc*/, ptr_t& offset, int& size)
{
	offset = size = 0;
} /* DSymFile::GetFunctionOffsetAndSize */

void DSymFile::GetFunctionOffsetAndSize(const char* /*name*/, ptr_t& offset, int& size)
{
	offset = size = 0;
} /* DSymFile::GetFunctionOffsetAndSize */

void DSymFile::GetFunctionName(ptr_t pc, string& name)
{
	this->GetRawFunctionName(pc, name);
} /* DSymFile::GetFunctionName */

void DSymFile::GetRawFunctionName(ptr_t pc, string& name)
{
	ostrstream s;
	s << "(" << (void*)pc << ")" << ends;
	name = s.str();
	delete[] s.str();
} /* DSymFile::GetRawFunctionName */

void DSymFile::GetLocals(vector<DVariable*>& /*vars*/, ptr_t /*pc*/)
{
} /* DSymFile::GetLocals */

void DSymFile::GetGlobals(vector<pair<DVariable*, ptr_t> >& /*vars*/)
{
} // DSymFile::GetGlobals

DFunction& DSymFile::GetFunction(ptr_t pc)
{
	THROW(("Function not found for pc = 0x%08x", pc));
} // DSymFile::GetFunction

DFunction& DSymFile::GetFunction (const char *name)
{
	THROW(("Function %s not found", name));
} // DSymFile::GetFunction

ptr_t DSymFile::GetFunctionLowPC(ptr_t /*pc*/)
{
	return 0;
} // DSymFile::GetFunctionLowPC

ptr_t DSymFile::GetFunctionHighPC(ptr_t /*pc*/)
{
	return 0;
} // DSymFile::GetFunctionHighPC

bool DSymFile::IsDisabled(ptr_t /*pc*/) const
{
	return false;
} // DSymFile::IsDisabled

bool DSymFile::ContainsName(const char* /*name*/) const
{
	return false;
} // DSymFile::ContainsName

bool DSymFile::HasSourceInformation() const
{
	return false;
} // DSymFile::HasSourceInformation

DType* DSymFile::GetType(const char *type) const
{
	THROW(("Type %s not found!", type));
} // DSymFile::GetType

DType* DSymFile::GetType(TypeID id) const
{
	THROW(("Type %08x not found", id));
} // DSymFile::GetType

DVariable* DSymFile::GetVariable(const char *name, ptr_t /*pc*/) const
{
	THROW(("Variable \"%s\" not found!", name));
} // DSymFile::GetVariable
