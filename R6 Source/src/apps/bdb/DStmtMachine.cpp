/*	$Id: DStmtMachine.cpp,v 1.12 1999/05/11 21:31:06 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 13:48:39
*/

#include "bdb.h"
#include "DStmtMachine.h"
#include "DDwarf2.h"
#include "DStatement.h"

#include <algorithm>

using namespace dwarf;

DStmtMachine::DStmtMachine ()
{
	program = NULL;
} // DStmtMachine::DStmtMachine

DStmtMachine::DStmtMachine (const char *data, string& comp_dir)
{
	Read(data, total_length);
	program_end = data + total_length;
	Read(data, version);
	Read(data, prologue_length);
	const char *t = data;
	Read(data, minimum_instruction_length);
	Read(data, default_is_stmt);
	Read(data, line_base);
	Read(data, line_range);
	Read(data, opcode_base);
	
	standard_opcode_lengths.push_back(0);

	for (int i = 1; i < opcode_base; i++)
		standard_opcode_lengths.push_back(*(uchar *)data++);
	
	if (*data)
	{
	 	while (*data)
	 	{
	 		include_directories.push_back (data);
	 		data += strlen (data) + 1;
	 	}
	 }
 	
	++data;
	
	vector<DFileNr> fls;
	
	while (*data)
	{
		uint32 directory, moddate, size;
		
		fls.push_back (DSourceFileTable::Instance().AddFile(comp_dir, data));

		data += strlen(data) + 1;
		ReadULEB128(data, directory);
		ReadULEB128(data, moddate);
		ReadULEB128(data, size);
	}
	
	program = t + prologue_length;
	
	set<int> fileNrs;
	
	InitMachine();
	const char *p = program;

	while (p < program_end)
	{
		NextRow(p);
		fileNrs.insert(file);
	}
	
	for (set<int>::iterator i = fileNrs.begin(); i != fileNrs.end(); i++)
		files[*i] = fls[*i - 1];
} // DStmtMachine::DStmtMachine

void DStmtMachine::GetFiles(set<DFileNr>& fls)
{
	for (file_iterator i = files.begin(); i != files.end(); i++)
		fls.insert((*i).second);
} // DStmtMachine::GetFiles

class CFileFinder
{
  public:
	CFileFinder (DFileNr _ref) : ref(_ref) {}
	bool operator () (const pair<int,DFileNr> & fi)
		{ return fi.second == ref; }
	DFileNr ref;
};

void DStmtMachine::GetStopsForFile (DFileNr file, set<int>& lines)
{
	FailNilMsg(program, "Not a valid statement machine");
	
	file_iterator i = find_if (files_begin(), files_end(), CFileFinder(file));
	
	if (i == files_end())
		return;
	
	build((*i).first, lines);
} // DStmtMachine::GetStopsForFile

bool DStmtMachine::GetStatement (ptr_t pc, DStatement& stmt)
{
	FailNilMsg(program, "Not a valid statement machine");
	
	bool result = false;

	InitMachine();
	const char *p = program;
	
	while (p < program_end)
	{
		if (FindPCInSequence(pc, p))
		{
			stmt.fFile = files[file];
			
			int found_line = line;
			
			while (address <= pc && !end_sequence)
			{
				found_line = line;
				NextRow(p);
			}
			
			stmt.fLine = found_line;
			stmt.fPC = pc;
			stmt.fSize = address - pc - 1;
			stmt.fSourceLevel = true;

			result = true;
			break;
		}
	}
	
	return result;
} // DStmtMachine::GetStatement

	// find a pc in the code sequence of this compilation unit pointed to by p
bool DStmtMachine::FindPCInSequence(ptr_t pc, const char *& p)
{
	InitMachine();
	
	while (address == 0)
		NextRow(p);
		
	if (address > pc)	// Not in this sequence since the first address is already larger than pc
		return false;
	
	while (!end_sequence && address < pc)
	{
			// store registers since we might go too far
		uint32
			b_address = address,
			b_file = file,
			b_line = line,
			b_column = column;
		bool
			b_is_stmt = is_stmt,
			b_basic_block = basic_block;
		const char *b_p = p;

		NextRow(p);
		
		if (address > pc)
		{
			address = b_address;
			file = b_file;
			line = b_line;
			column = b_column;
			is_stmt = b_is_stmt;
			basic_block = b_basic_block;
			end_sequence = false;
			p = b_p;
			
			break;
		}
	}
	
	return ! end_sequence;
} // DStmtMachine::FindPCInSequence

void DStmtMachine::build (int fileNr, set<int>& lines)
{
	InitMachine();
	const char *p = program;
	ptr_t last_address = ~0UL;
	int last_line = -1;
	
	while (p < program_end)
	{
		NextRow(p);

		if (file == (uint32) fileNr)
		{
			if (last_address == address && last_line > 0)
				lines.erase(last_line);

			lines.insert(line);
			last_line = line;
			last_address = address;
		}
	}
} // DStmtMachine::build

ptr_t DStmtMachine::GetStatementOffset (DFileNr ref, int pc_line)
{
	FailNilMsg(program, "Not a valid statement machine");
	
	file_iterator i = find_if (files_begin(), files_end(), CFileFinder(ref));
	
	if (i == files_end())
		return 0;
	
	int fileNr = (*i).first;
	
	InitMachine();	
	const char *p = program;
	
	while (p < program_end)
	{
		NextRow(p);

		if (file == (uint32) fileNr && line == (uint32) pc_line)
			return address;
	}

	return 0;
} // DStmtMachine::GetStatementOffset

void DStmtMachine::InitMachine()
{
	address = 0;
	file = 1;
	line = 1;
	column = 0;
	is_stmt = default_is_stmt;
	basic_block = false;
	end_sequence = false;
} // DStmtMachine::InitMachine

void DStmtMachine::NextRow(const char*& p)
{
	if (end_sequence)
		InitMachine();

	uint32 l = line;
	while (l == line && !end_sequence)
		ProcessOpcode(p);
} // DStmtMachine::NextRow

void DStmtMachine::ProcessOpcode(const char *& p)
{
	uchar opcode = (uchar)*p++;
	uint32 x;
	int32 y;

	switch (opcode)
	{
		case DW_LNS_copy:
			basic_block = false;
			break;

		case DW_LNS_advance_pc:
			ReadULEB128(p, x);
			address += minimum_instruction_length * x;
			break;

		case DW_LNS_advance_line:
			ReadLEB128(p, y);
			line += y;
			break;
			
		case DW_LNS_set_file:
			ReadULEB128(p, x);
			file = x;
			break;
		
		case DW_LNS_set_column:
			ReadULEB128(p, x);
			break;
			
		case DW_LNS_negate_stmt:
			is_stmt = !is_stmt;
			break;

		case DW_LNS_set_basic_block:
			basic_block = true;
			break;
			
		case DW_LNS_const_add_pc:
			// ????
			break;
			
		case DW_LNS_fixed_advance_pc:
			uint16 y;
			Read(p, y);
			address += y;
			break;
		
		case DW_LNS_extended_op:
		{
			uint32 length = *(uchar *)p++;
			const char *op = p;

			switch (*(uchar *)p++)
			{
				case DW_LNE_end_sequence:
					end_sequence = true;
					break;
				
				case DW_LNE_set_address:
					Read(p, address);
					break;
				
				case DW_LNE_define_file:
				{
//					file_info finfo;
//				
////					finfo.name = p;
//					BEntry(p).GetRef(&finfo.file);
//					
//					p += strlen(p) + 1;
//					ReadULEB128(p, finfo.directory);
//					ReadULEB128(p, finfo.moddate);
//					ReadULEB128(p, finfo.size);
//					
//					file_iterator i = find_if(files_begin(), files_end(), CFileFinder(finfo.file));
//					
//					if (i != files_end())
//						file = i - files_begin() + 1;
					THROW(("Unsupported DWARF2 Statement program opcode"));
					break;
				}
			}

			p = op + length;
			break;
		}
		
		default:
			opcode -= opcode_base;
			address += opcode / line_range;
			line += line_base + opcode % line_range;
			break;
	}
} // DStmtMachine::ProcessOpcode
