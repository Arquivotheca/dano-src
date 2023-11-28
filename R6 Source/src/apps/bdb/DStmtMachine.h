/*	$Id: DStmtMachine.h,v 1.5 1999/05/11 21:31:06 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 10:57:45
*/

#ifndef DSTMTMACHINE_H
#define DSTMTMACHINE_H

using namespace std;

#include "DSourceFileTable.h"

#include <set>

struct DStatement;

class DStmtMachine
{
  public:
  	DStmtMachine ();
	DStmtMachine (const char *data, string& comp_dir);
	
	typedef map<int,DFileNr>::iterator file_iterator;
	
	file_iterator files_begin ()
		{ return files.begin (); }
	file_iterator files_end ()
		{ return files.end (); }
	
	void GetStopsForFile (DFileNr file, set<int>& lines);
	bool GetStatement (ptr_t pc, DStatement& stmt);
	ptr_t GetStatementOffset (DFileNr file, int line);
	
	void GetFiles(set<DFileNr>& files);
	
  protected:

	void build(int fileNr, set<int>& lines);
	bool FindPCInSequence(ptr_t pc, const char*& p);
	
	void InitMachine();
		// process the next opcode in the statement program
	void ProcessOpcode(const char*& opc);
		// advance through the statement program until we have a new row
	void NextRow(const char*& opc);

	uint32 						address, file, line, column;
	bool 							is_stmt, basic_block, end_sequence;

	uint32							total_length;
	uint16							version;
	uint32							prologue_length;
	uchar							minimum_instruction_length;
	uchar							default_is_stmt;
	char							line_base;
	uchar							line_range;
	uchar							opcode_base;
	vector<uchar>				standard_opcode_lengths;
	vector<const char*>		include_directories;
	map<int,DFileNr>			files;
	const char					*program, *program_end;
};


#endif
