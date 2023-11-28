/*	$Id: DSymWorld.cpp,v 1.15 1999/05/11 21:31:07 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/13/98 14:04:28
*/

#include "bdb.h"
#include "DSymWorld.h"
#include "DSymFile.h"
#include "DDwarfSymFile.h"
#include "DElfSymFile.h"
#include "DStatement.h"
#include "DNub.h"
#include "DTeam.h"
#include <strstream>
#include "DMessages.h"
#include <Message.h>
#include <Messenger.h>
#include <algorithm>

// binary predicate for alphabetically sorting the vector of global variables,
// which is a vector of pair<DVariable*, ptr_t> objects
struct GlobalSorter
{
	bool operator()(const pair<DVariable*, ptr_t>& left, const pair<DVariable*, ptr_t>& right)
	{
		string leftName, rightName;
		left.first->GetName(leftName);
		right.first->GetName(rightName);
		return (leftName < rightName);
	}
};

DSymWorld::DSymWorld(DTeam* team)
{
	fOwningTeam = team;
} /* DSymWorld::DSymWorld */

DSymWorld::~DSymWorld()
{

} /* DSymWorld::DSymWorld */

void DSymWorld::LoadSymbols(team_id team, BLooper *owner)
{
	image_info info;
	long cookie = 0;
	
	while (get_next_image_info(team, &cookie, &info) == B_OK)
		ImageCreated(info, owner);
} /* DSymWorld::LoadSymbols */

void 
DSymWorld::ReadImageSymbols(const image_info& info, BLooper* owner)
{
	BMessage status(kMsgStatus);
	string statusMsg;
	
	statusMsg = "Loading image: ";
	statusMsg += info.name;
	status.AddString("status", statusMsg.c_str());

	(void)BMessenger(owner).SendMessage(&status, (BHandler *)0, 1000);
	
	DSymFile *symFile = NULL;
	
#if defined(__ELF__)
	try
	{ 
		symFile = new DDwarfSymFile(info, fOwningTeam);
		symFile->LoadSymbols();
	}
	catch (HErr& e)
	{ /*...*/ }

	if (symFile == NULL)
	{
		try
		{ 
			symFile = new DElfSymFile(info);
			symFile->LoadSymbols();
		}
		catch (HErr& e)
		{ /*...*/ }
	}

#else
#	error "No idea what to do next..."
#endif

	if (symFile == NULL)
	{
		symFile = new DSymFile(info);
		symFile->LoadSymbols();
	}
	
	fSymFiles.push_back(symFile);
	
	status.ReplaceString("status", "");
	(void)BMessenger(owner).SendMessage(&status, (BHandler *)0, 1000);
}

// returns true if we already have a DSymFile for the given image
bool DSymWorld::SymbolsLoaded(const image_info& info) const
{
	for (vector<DSymFile*>::const_iterator i = fSymFiles.begin(); i != fSymFiles.end(); i++)
	{
		if (info.id == (*i)->ImageID()) return true;
	}
	return false;
}

void DSymWorld::ImageCreated(const image_info& info, BLooper *owner)
{
	// parse in the new image, which is an add-on
	if (!SymbolsLoaded(info))
	{
		ReadImageSymbols(info, owner);
	}

	// The kernel does *not* currently issue IMAGE_CREATED nub messages when
	// shared libraries are loaded.  So, in order to detect possible newly-loaded shared
	// libraries, we iterate over the new image list and process anything we haven't seen
	// before.
	image_list images;
	fOwningTeam->GetNub().GetImageList(fOwningTeam->GetID(), images);
	for (image_list::iterator i = images.begin(); i != images.end(); i++)
	{
		if (!SymbolsLoaded(*i))
		{
			ReadImageSymbols(*i, owner);
		}
	}
} /* DSymWorld::ImageCreated */

void DSymWorld::ImageDeleted(const image_info& info)
{
	vector<DSymFile*>::iterator sfi;
	for (sfi = fSymFiles.begin(); sfi != fSymFiles.end(); sfi++)
	{
		if ((*sfi)->ImageID() == info.id)
		{
			delete *sfi;
			fSymFiles.erase(sfi);
			break;
		}
	}
} /* DSymWorld::ImageDeleted */

void DSymWorld::TeamDeleted()
{
	vector<DSymFile*>::iterator sfi;

	for (sfi = fSymFiles.begin(); sfi != fSymFiles.end(); sfi++)
		delete *sfi;
	
	fSymFiles.clear();
} // DSymWorld::TeamDeleted

bool DSymWorld::GetStatement(ptr_t pc, DStatement& statement) const
{
	DSymFile *file = LocatePC(pc);
//	ASSERT(file);
	
	if (file)
		return file->GetStatement(pc, statement, *this);
	else
		return false;
} /* DSymWorld::GetStatement */

bool DSymWorld::GetAsmStatement(ptr_t pc, DStatement& statement) const
{
	DSymFile *file = LocatePC(pc);
//	ASSERT(file);
	
	if (file)
		return file->GetAsmStatement(pc, statement, *this);
	else
		return false;
} /* DSymWorld::GetAsmStatement */

bool DSymWorld::GetSourceStatement(ptr_t pc, DStatement& statement) const
{
	DSymFile *file = LocatePC(pc);
//	ASSERT(file);
	
	if (file)
		return file->GetSourceStatement(pc, statement, *this);
	else
		return false;
} /* DSymWorld::GetSourceStatement */

bool DSymWorld::GetStatement(const char *name, DStatement& statement) const
{
	// Notice that a mangled name will get the asm file while a C++ name
	// will get the source (if it exists)	
	vector<DSymFile*>::const_iterator sfi;
	for (sfi = fSymFiles.begin(); sfi != fSymFiles.end(); sfi++)
	{
		if ((*sfi)->HasSourceInformation() && (*sfi)->ContainsName(name))
		{
			return (*sfi)->GetStatement(name, statement, *this);
		}
	}
	return false;	
} /* DSymWorld::GetStatement */

void DSymWorld::GetStopsForFile(DFileNr file, set<int>& lines) const
{
	lines.clear();

	vector<DSymFile*>::const_iterator sfi;
	for (sfi = fSymFiles.begin(); sfi != fSymFiles.end(); sfi++)
		(*sfi)->GetStopsForFile(file, lines);
} /* DSymWorld::GetStopsForFile */

ptr_t DSymWorld::GetStatementOffset(DFileNr file, int line) const
{
	ptr_t result = 0;
	
	vector<DSymFile*>::const_iterator sfi;
	for (sfi = fSymFiles.begin(); sfi != fSymFiles.end() && result == 0; sfi++)
		result = (*sfi)->GetStatementOffset(file, line);
	
	return result;
} /* DSymWorld::GetStatementOffset */

void DSymWorld::GetFunctionName(ptr_t pc, string& name) const
{
	DSymFile *symFile = LocatePC(pc);
//	ASSERT(symFile);
	
	if (symFile)
		symFile->GetFunctionName(pc, name);
	else
	{
		ostrstream o;
		o << "(" << (void*)pc << ')' << ends;
		name = o.str();
		delete[] o.str();
	}
} /* DSymWorld::GetFunctionName */

void DSymWorld::GetRawFunctionName(ptr_t pc, string& name) const
{
	DSymFile *symFile = LocatePC(pc);
//	ASSERT(symFile);
	
	if (symFile)
		symFile->GetRawFunctionName(pc, name);
	else
	{
		ostrstream o;
		o << "(" << (void*)pc << ')' << ends;
		name = o.str();
		delete[] o.str();
	}
} /* DSymWorld::GetRawFunctionName */

DSymFile *DSymWorld::LocatePC(ptr_t pc) const
{
	vector<DSymFile*>::const_iterator sfi;
	for (sfi = fSymFiles.begin(); sfi != fSymFiles.end(); sfi++)
	{
		if ((*sfi)->Contains(pc))
			return *sfi;
	}

	return NULL;
} /* DSymWorld::LocatePC */

void DSymWorld::GetFunctionOffsetAndSize(const char *name, ptr_t& pc, int& size) const
{
	pc = 0;

	vector<DSymFile*>::const_iterator sfi;
	for (sfi = fSymFiles.begin(); sfi != fSymFiles.end() && pc == 0; sfi++)
	{
		if ((*sfi)->ContainsName(name))
		{
			try
				{ (*sfi)->GetFunctionOffsetAndSize(name, pc, size); }
			catch (...)
				{ pc = 0; }
		}
	}
	
	if (pc == 0)
		THROW(("Function %s not found", name));
} /* DSymWorld::GetFunctionOffsetAndSize */

void DSymWorld::GetSourceFiles(vector<DFileNr>& sourceFiles) const
{
	vector<DSymFile*>::const_iterator sfi;
	for (sfi = fSymFiles.begin(); sfi != fSymFiles.end(); sfi++)
	{
		for (int i = 0; i < (*sfi)->SourceFileCount(); i++)
		{
			if (find(sourceFiles.begin(), sourceFiles.end(), (*sfi)->SourceFile(i)) == sourceFiles.end())
				sourceFiles.push_back((*sfi)->SourceFile(i));
		}
	}
} /* DSymWorld::GetSourceFiles */

void DSymWorld::GetLocals(vector<DVariable*>& vars, ptr_t pc) const
{
	DSymFile *symFile = LocatePC(pc);
	if (symFile)
		symFile->GetLocals(vars, pc);
} // DSymWorld::GetLocals

void DSymWorld::GetGlobals(vector<pair<DVariable*, ptr_t> >& vars) const
{
	vector<DSymFile*>::const_iterator sfi;
	for (sfi = fSymFiles.begin(); sfi != fSymFiles.end(); sfi++)
	{
		(*sfi)->GetGlobals(vars);
	}

	// sort alphabetically after we have all the globals from all images
	sort(vars.begin(), vars.end(), GlobalSorter());
} // DSymWorld::GetGlobals

DFunction& DSymWorld::GetFunction(ptr_t pc) const
{
	DSymFile *symFile = LocatePC(pc);
	if (symFile == NULL)
		THROW(("Function not found for pc = 0x%08x", pc));
	return symFile->GetFunction(pc);
} // DSymWorld::GetFunction

ptr_t DSymWorld::GetFunctionLowPC (ptr_t pc) const
{
	DSymFile *symFile = LocatePC(pc);
	if (symFile)
		return symFile->GetFunctionLowPC(pc);
	else
		return 0;
} // DSymWorld::GetFunctionLowPC

ptr_t DSymWorld::GetFunctionHighPC (ptr_t pc) const
{
	DSymFile *symFile = LocatePC(pc);
	if (symFile)
		return symFile->GetFunctionHighPC(pc);
	else
		return 0;
} // DSymWorld::GetFunctionHighPC

bool DSymWorld::IsDisabled(ptr_t pc) const
{
	DSymFile *symFile = LocatePC(pc);
	if (symFile)
		return symFile->IsDisabled(pc);
	else
		return false;
} // DSymWorld::IsDisabled

DType* DSymWorld::GetType(const char *name) const
{
	vector<DSymFile*>::const_iterator sfi;

	for (sfi = fSymFiles.begin(); sfi != fSymFiles.end(); sfi++)
	{
		try
		{
			return (*sfi)->GetType(name);
		}
		catch (HErr& e)
		{
		}
	}
	
	THROW(("Type %s not found!", name));
} // DSymWorld::GetType

DVariable* DSymWorld::GetVariable (const char *name, ptr_t pc) const
{
	DSymFile *symFile = LocatePC(pc);
	if (symFile)
		return symFile->GetVariable(name, pc);
	else
		return 0;
} // DSymWorld::GetVariable

DTeam* DSymWorld::GetTeam() const
{
	return fOwningTeam;
} // DSymWorld::GetTeam
