/*	$Id: DDwarfSymFile.cpp,v 1.13 1999/05/11 21:31:03 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 13:49:03
*/

#include "bdb.h"
#include "DElf.h"
#include "DDwarfSymFile.h"
#include "DStmtMachine.h"
#include "DPredicates.h"
#include "DStatement.h"
#include "DDwarfType.h"
#include "DTeam.h"
#include "DTeamWindow.h"
#include "DMessages.h"
#include "DWaitWindow.h"
#include <Messenger.h>
#include <MessageQueue.h>
#include <vector>
#include <map>

// work crew pattern - # of concurrent dwarf scans == # of cpus
// gScanList == list of scans to perform
// gScanSem == signals the condition "there's a new item in the queue"
// gDwarfScanThreads == thread_ids of the scan threads, so we can shut them down etc.
// gScansPending == count of pending scans, so we can tell when we're done
static vector<thread_id> gDwarfScanThreads;
static BMessageQueue* gScanList = NULL;
static sem_id gScanSem;
static volatile int32 gScansPending = 0;

using namespace dwarf;

typedef vector<string> string_list;
string scope_list_to_full_name(const string_list& scopeList)
{
	string_list::const_iterator sli = scopeList.begin();
	string fullname = *sli;
	sli++;
	while (sli != scopeList.end())
	{
		if ((*sli).length())
		{
			fullname += "::";
			fullname += *sli;
		}
		sli++;
	}
	return fullname;
}

DDwarfSymFile::DDwarfSymFile (const image_info& info, DTeam* team)
	: DElfSymFile(info), fTeam(team->GetID())
{
	// first time, create our dwarf-scanning work crew
	if (gScanList == NULL)
	{
		gScanList = new BMessageQueue("dwarf scan queue");
		gScanSem = create_sem(0, "dwarf scan count");

		system_info info;
		get_system_info(&info);
		for (int32 i = 0; i < info.cpu_count; i++)
		{
			thread_id tid = spawn_thread(load_symbol_thread, "dwarf walk", B_NORMAL_PRIORITY, NULL);
			gDwarfScanThreads.push_back(tid);
			resume_thread(tid);
		}
	}

	fDwarf = new dwarf2(fElf, this);
}

DDwarfSymFile::~DDwarfSymFile()
{
  	// The members of fGlobals are NOT deleted here. (The DVariableItem takes over ownership
  	// of the DVariable.)  This isn't the best situation, but it is better than 
  	// duplicating all the global variables.
	// If we deleted them here, the DVariableItem would point to a deleted object until the
	// DVariableItem itself was deleted ... not a good idea.

	delete fDwarf;
} // DDwarfSymFile::~DDwarfSymFile

// we'll need these types for LoadSymbols()
typedef map<uint32, string> offset_name_map;	// offset to full name mapping
typedef map<string, dwarf::entry> type_name_map;		// name to DIE location
typedef vector<entry> entry_list;

void DDwarfSymFile::LoadSymbols()
{
	// We need the ELF symbols too...
	DElfSymFile::LoadSymbols();
	
	// read the files list
	set<DFileNr> files;
	for (dwarf2::unit_iterator ui = fDwarf->unit_begin(); ui != fDwarf->unit_end(); ui++)
	{
		// find the descriptor for this compilation unit (CU)
		compile_unit::entry_iterator ei =
			find_if ((*ui).entry_begin(), (*ui).entry_end(), Tag_Pred (DW_TAG_compile_unit));

		// add the files from this CU to the fileset
		try
		{
			if (ei != (*ui).entry_end ())
				GetStmtMachine(ui - fDwarf->unit_begin()).GetFiles (files);
		}
		catch (...) { /* do nothing */}
	}
	fFiles.insert(fFiles.begin(), files.begin(), files.end());

	// queue this image's dwarf info to be scanned
	if (fDwarf->has_info())
	{
		BMessage* msg = new BMessage(5038);
		msg->AddPointer("symfile", this);
		gScanList->AddMessage(msg);
		atomic_add(&gScansPending, 1);
		release_sem(gScanSem);
	}
} // DDwarfSymFile::DDwarfSymFile

// background thread for processing the DWARF information for the symfile
int32 load_symbol_thread(void*)
{
	// block until there's an item in the work queue
	while (acquire_sem(gScanSem) == B_OK)
	{
		DDwarfSymFile* symfile;
		BMessage* msg = gScanList->NextMessage();
		msg->FindPointer("symfile", (void**) &symfile);
		delete msg;

		// name ourselves by the image we're scanning, for debug purposes
		char buf[NAME_MAX + 10];	// max filename length + "walking: " + NUL
		sprintf(buf, "walking: ");
		char* s = strrchr(symfile->fImage.name, '/');
		if (s) strcat(buf, s+1);
		rename_thread(find_thread(NULL), buf);

		// Now load the dwarf symbols
		entry_list globalList;
		type_name_map concreteTypes;
		offset_name_map unknownTypes;

		for (dwarf2::unit_iterator ui = symfile->fDwarf->unit_begin(); ui != symfile->fDwarf->unit_end(); ui++)
		{
			// walk the debug info, building the fast data structures for later use		
			int depth = 0;
			string_list scopeName;

			for (compile_unit::entry_iterator ei = (*ui).entry_begin(); ei != (*ui).entry_end(); ei++)
			{
				entry e = *ei;

				// skip the compilation-unit descriptor for now
				if (e.tag() == DW_TAG_compile_unit) continue;

				ptr_t highPC;
				string name;
				bool hasName = e.find_attribute_value(DW_AT_name, name);

				switch (e.tag())
				{
				case 0:
					--depth;
					scopeName.pop_back();
					break;

				case DW_TAG_inlined_subroutine:
					if (e.find_attribute_value(DW_AT_high_pc, highPC))
					{
						symfile->fInlines[highPC] = DFunction(e);
					}
					break;

				case DW_TAG_subprogram:
				{
					string fullname;
					if (hasName)
					{
						if (scopeName.size())
						{
							fullname = scope_list_to_full_name(scopeName);
							fullname += "::";
						}
						fullname += name;
					}
					if (e.find_attribute_value(DW_AT_high_pc, highPC))
					{
						symfile->fFunctions[highPC] = DFunction(e);
					}
					break;
				}

				case DW_TAG_union_type:
				case DW_TAG_structure_type:
				case DW_TAG_enumeration_type:
				case DW_TAG_typedef:
					{
						string fullname;
						if (hasName)
						{
							if (scopeName.size())
							{
								fullname = scope_list_to_full_name(scopeName);
								fullname += "::";
							}
							fullname += name;
						}
						// is it a concrete struct/union entry?
						uint32 byteSize;
						if (e.find_attribute_value(DW_AT_byte_size, byteSize))
						{
							// only one ref per concrete type is necessary, so we don't
							// worry about stomping on a previously-added entry here
							// in the case where multiple CUs have the same type info.
							// What is important is that we get *some* reference for each
							// concrete type known to the app.
							concreteTypes[fullname] = e;
							symfile->fTypeOffsets[e.get_offset() + (*ui).info_offset()] = e;
						}
						else
						{
							// look to see whether this type name is already concretely known;
							// if it is, we just point directly to that entry 
							type_name_map::iterator ni = concreteTypes.find(fullname);
							if (ni == concreteTypes.end())
							{
								unknownTypes[e.get_offset() + (*ui).info_offset()] = fullname;
							}
							else symfile->fTypeOffsets[e.get_offset() + (*ui).info_offset()] = (*ni).second;
						}
					}
					break;

				case DW_TAG_base_type:
				{
					uint32 byteSize;
					if (e.find_attribute_value(DW_AT_byte_size, byteSize))
					{
						concreteTypes[name] = e;
						symfile->fTypeOffsets[e.get_offset() + (*ui).info_offset()] = e;
					}
					else
					{
						type_name_map::iterator ni = concreteTypes.find(name);
						if (ni == concreteTypes.end())
						{
							unknownTypes[e.get_offset() + (*ui).info_offset()] = name;
						}
						else symfile->fTypeOffsets[e.get_offset() + (*ui).info_offset()] = (*ni).second;
					}
					break;
				}

				case DW_TAG_variable:
					// we only care about *named* variables at global scope, since all other
					// variables will be dealt with by their enclosing subprogram &c. objects,
					// or are anonymous and hidden from the app anyway
					if ((depth == 0) && name.size())
					{
						// globals without a location attribute are unused by the application,
						// so we don't need to try to display them.  We don't know where to
						// find them anyway.  Note that a *lot* of these will correspond to
						// constants, and so aren't strictly "global variables."  We'll cull those
						// when we really process this list; right now we're just building an
						// efficient list of references so we can do the "real" work after we
						// reconcile all the types.
						DLocationString location;
						if (e.find_attribute_value(DW_AT_location, location))
						{
							globalList.push_back(e);
						}
					}
					break;

				default:
					break;
				}

				// the 991026 toolchain emits null DIEs with a TRUE has-children abbrev flag.
				// this is blatantly wrong, and we test for (and ignore) that situation here.
				if (e.tag() && e.has_children())
				{
					++depth;
					scopeName.push_back(name);
				}
			}
		}

		// now we walk unknownTypes and resolve all that we can
		while (unknownTypes.size())
		{
			offset_name_map::iterator oi = unknownTypes.begin();
			type_name_map::iterator ni = concreteTypes.find((*oi).second);		// does this name exist concretely yet?
			if (ni != concreteTypes.end())
			{
				symfile->fTypeOffsets[(*oi).first] = (*ni).second;
			}
			unknownTypes.erase(oi);
		}

		// now we really set up the concrete globals -- stripping out the "const"
		// ones requires having the rest of the type mechanism set up, which is
		// why we deferred this step until after the full DWARF scan
		for (entry_list::iterator ei = globalList.begin(); ei != globalList.end(); ei++)
		{
			entry globalEntry(*ei);
			entry e;
			DLocationString location;
			string name;
			if (globalEntry.find_attribute_value(DW_AT_type, e) &&
				globalEntry.find_attribute_value(DW_AT_location, location) &&
				globalEntry.find_attribute_value(DW_AT_name, name) &&
				strncmp(name.c_str(), "__ti", 4))
			{
				DType* type = symfile->GetType(e);
				if (! type->IsConst())
				{
					symfile->fGlobals.push_back(new DVariable(name, type, location));
				}
			}
		}

		// tell the team window to rebuild its globals list
		DTeamWindow* teamWin = DTeamWindow::GetTeamWindow(symfile->fTeam);
		if (teamWin)
		{
			BMessenger(teamWin).SendMessage(kMsgGlobalsChanged);
		}
		atomic_add(&gScansPending, -1);
		rename_thread(find_thread(NULL), "dwarf walk");
	}
	return 0;
}

void 
DDwarfSymFile::WaitForSymThread(const char* fmt, ...)
{
	char msg[512];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(msg, fmt, ap);

	DWaitWindow* win = new DWaitWindow(BRect(250, 150, 550, 275), msg);
	win->Show();

	// wait until the dwarf scans are all finished
	while (gScansPending > 0)
	{
		snooze(1000000);
	}

	// close the wait window
	if (win->Lock())
	{
		win->Quit();
	}
}

void DDwarfSymFile::GetStopsForFile(DFileNr file, set<int>& lines)
{
	map<int,DStmtMachine>::iterator smi;
	
	for (smi = fStmtMachines.begin(); smi != fStmtMachines.end(); smi++)
		(*smi).second.GetStopsForFile (file, lines);

	// If we didn't get any lines from the dwarf information, try elf information
	if (lines.empty()) {
		DElfSymFile::GetStopsForFile(file, lines);
	}
} // DDwarfSymFile::GetStopsForFile

const dwarf::entry& DDwarfSymFile::GetEntryAt(uint32 global_offset) const
{
	return fTypeOffsets[global_offset];
}

void DDwarfSymFile::GetFunctionName(ptr_t pc, string& name)
{
	try
	{
		DFunction& f = GetFunction(pc);
		f.GetName(name);
	}
	catch (HErr& e)	
 	{
 		DElfSymFile::GetFunctionName(pc, name);
	}
	
} // DDwarfSymFile::GetFunctionName

void DDwarfSymFile::GetRawFunctionName(ptr_t pc, string& name)
{
	try
	{
		DFunction& f = GetFunction(pc);
		f.GetRawName(name);
	}
	catch (HErr& e)	
 	{
 		DElfSymFile::GetRawFunctionName(pc, name);
	}
	
} // DDwarfSymFile::GetRawFunctionName

DFunction& DDwarfSymFile::GetFunction (ptr_t pc)
{
	// We can't do function name lookups until we've parsed the function
	// blocks out of the DWARF info, so wait for the background parsing
	// thread to finish before proceeding.
	if (gScansPending > 0)
	{
		WaitForSymThread("Please wait while bdb scans %s for debug information...", fImage.name);
	}
	pc -= fBaseAddr;	// make it relative to the start of the current block

	// first check inlines
	function_map::iterator i = fInlines.lower_bound(pc);

	// if it's not in an inline, check the enclosing function blocks
	if (i == fInlines.end() || ((*i).second.LowPC() > pc) || ((*i).second.HighPC() < pc))
	{
		i = fFunctions.lower_bound(pc);
		if (i == fFunctions.end() || ((*i).second.LowPC() > pc) || ((*i).second.HighPC() < pc))
		{
			THROW(("Function not found for PC=%p", pc));
		}
	}

	return (*i).second;
} // DDwarfSymFile::GetFunction

DFunction& DDwarfSymFile::GetFunction (const char *name)
{
	for (function_map::iterator fi = fFunctions.begin(); fi != fFunctions.end(); fi++)
	{
		string fName;
		DFunction& f = (*fi).second;
		f.GetName(fName);
		if (! strcmp(name, fName.c_str()))
		{
			return f;
		}
	}
	THROW(("Function %s not found!", name));
} // DDwarfSymFile::GetFunction


bool DDwarfSymFile::GetSourceStatement(ptr_t pc, DStatement& statement, const DSymWorld& /*symWorld*/)
{
	// Only look in dwarf information, don't fall back on elf information
	pc -= fBaseAddr;
	if (pc < (ptr_t) fCodeSize)
	{
		map<int,DStmtMachine>::iterator smi;
		
		for (smi = fStmtMachines.begin(); smi != fStmtMachines.end(); smi++)
		{
			if ((*smi).second.GetStatement(pc, statement))
			{
				statement.fPC += fBaseAddr;
				return true;
			}
		}
	}

	return false;
}

bool DDwarfSymFile::GetStatement(ptr_t pc, DStatement& statement, const DSymWorld& symWorld)
{
	// rely only on the DWARF iff not interleaving src and asm
	if (!gPrefs->GetPrefInt("interleave src and asm", false))
	{
		if (this->GetSourceStatement(pc, statement, symWorld))
		{
			return true;
		}
	}
	
	// if we didn't find the statement, try for assembly level	
	return DElfSymFile::GetStatement(pc, statement, symWorld);
} // DDwarfSymFile::GetStatement

bool DDwarfSymFile::GetAsmStatement(ptr_t pc, DStatement& statement, const DSymWorld& symWorld)
{
	// Force getting an asm file for this pc
	return DElfSymFile::GetAsmStatement(pc, statement, symWorld);
}

bool DDwarfSymFile::GetStatement(const char* name, DStatement& statement, const DSymWorld& symWorld)
{
	try
	{
		DFunction& f = GetFunction(name);
		return this->GetStatement(f.LowPC()+fBaseAddr, statement, symWorld);
	}
	catch (...)
	{
		return DElfSymFile::GetStatement(name, statement, symWorld);
	}
} // DDwarfSymFile::GetStatement

void DDwarfSymFile::GetFunctionOffsetAndSize(const char *name, ptr_t& offset, int& size)
{
	try
	{
		DFunction& f = GetFunction(name);
		offset = f.LowPC();
		size = f.HighPC() - offset;
		offset += fBaseAddr;
	}
	catch (...)
	{
		DElfSymFile::GetFunctionOffsetAndSize(name, offset, size);
	}
} // DDwarfSymFile::GetFunctionOffsetAndSize

void DDwarfSymFile::GetFunctionOffsetAndSize(ptr_t pc, ptr_t& offset, int& size)
{
	try
	{
		DFunction& f = GetFunction(pc - fBaseAddr);
		offset = f.LowPC();
		size = f.HighPC() - offset;
		offset += fBaseAddr;
	}
	catch (...)
	{
		DElfSymFile::GetFunctionOffsetAndSize(pc, offset, size);
	}
	
} // DDwarfSymFile::GetFunctionOffsetAndSize

ptr_t DDwarfSymFile::GetStatementOffset(DFileNr file, int line)
{
	ptr_t result = 0;
	
	map<int,DStmtMachine>::iterator smi;
	
	for (smi = fStmtMachines.begin(); result == 0 && smi != fStmtMachines.end(); smi++)
		result = (*smi).second.GetStatementOffset (file, line);
	
	if (result > 0) {
		return result + fBaseAddr;
	}
	else {
		return DElfSymFile::GetStatementOffset(file, line);
	}
} // DDwarfSymFile::GetStatementOffset

DStmtMachine& DDwarfSymFile::GetStmtMachine(int compile_unit_nr)
{
	if (! fStmtMachines.count(compile_unit_nr))
	{
		dwarf2::unit_iterator ui = fDwarf->unit_begin() + compile_unit_nr;
		compile_unit::entry_iterator ei =
			find_if ((*ui).entry_begin(), (*ui).entry_end(), Tag_Pred (DW_TAG_compile_unit));

		uint32 offset;
		string compdir;
		
		if (! (*ei).find_attribute_value (DW_AT_stmt_list, offset))
			THROW(("compile unit has no statement program"));

		if (! (*ei).find_attribute_value (DW_AT_comp_dir, compdir))
			compdir = "";

		fStmtMachines[compile_unit_nr] = DStmtMachine(fDwarf->get_debug_line() + offset, compdir);
	}
	
	return fStmtMachines[compile_unit_nr];
} // DDwarfSymFile::GetStmtMachine

ptr_t DDwarfSymFile::GetFunctionLowPC(ptr_t pc)
{
	ptr_t result = 0;

	try
	{
		result = GetFunction(pc).LowPC() + fBaseAddr;
	}
	catch (...)
	{
		result = DElfSymFile::GetFunctionLowPC(pc);
	}
	
	return result;
} // DDwarfSymFile::GetFunctionLowPC

ptr_t DDwarfSymFile::GetFunctionHighPC (ptr_t pc)
{
	ptr_t result = 0;

	try
	{
		result = GetFunction(pc).HighPC() + fBaseAddr;
	}
	catch (...)
	{
		result = DElfSymFile::GetFunctionHighPC(pc);
	}
	
	return result;
} // DDwarfSymFile::GetFunctionHighPC

void DDwarfSymFile::GetGlobals(vector<pair<DVariable*, ptr_t> >& vars)
{
	for (vector<DVariable*>::iterator gi = fGlobals.begin(); gi != fGlobals.end(); gi++)
	{
		vars.push_back(pair<DVariable*, ptr_t>(*gi, fBaseAddr));
	}
} // DDwarfSymFile::GetGlobals

bool DDwarfSymFile::ContainsName(const char *name) const
{
	return (fDwarf->get_entry(name) != (*fDwarf->unit_begin()).entry_end() || DElfSymFile::ContainsName(name));
} // DDwarfSymFile::ContainsName

DType* DDwarfSymFile::GetType(const char *name) const
{
	dwarf2::unit_iterator ui;
	compile_unit::entry_iterator ei;
	
	for (ui = fDwarf->unit_begin(); ui != fDwarf->unit_end(); ui++)
	{
		for (ei = (*ui).entry_begin(); ei != (*ui).entry_end(); ei++)
		{
			string n;
			if ((*ei).find_attribute_value(DW_AT_name, n) && name == n)
				return new DDwarfType(*ei);
		}
	}

	THROW(("Type %s not found!", name));
} // DDwarfSymFile::GetType

bool DDwarfSymFile::HasSourceInformation() const
{
	return true;
} // DDwarfSymFile::HasSourceInformation

DType* DDwarfSymFile::GetType(TypeID type) const
{
	dwarf2::unit_iterator ui;
	uint32 offset = type;
	
	ui = fDwarf->unit_begin();
	
	while (offset >= (*ui).size() && ui != fDwarf->unit_end())
		offset -= (*ui++).size();
	
	if (ui != fDwarf->unit_end())
		return new DDwarfType(entry(&(*ui), offset));

	THROW(("Type not found!"));
} // DDwarfSymFile::GetType

DType* DDwarfSymFile::GetType(const entry& e)
{
	return new DDwarfType(e);
} // DDwarfSymFile::GetType

DVariable* DDwarfSymFile::GetVariable(const char *name, ptr_t pc) const
{
	return (const_cast<DDwarfSymFile*>(this))->GetFunction(pc).GetVariable(name, pc - fBaseAddr);
} // DDwarfSymFile::GetVariable
