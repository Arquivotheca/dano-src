/*	$Id: DElfSymFile.cpp,v 1.6 1999/05/03 13:09:49 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 11/13/98 10:56:41
*/

#include "bdb.h"
#include "DElfSymFile.h"
#include "DStatement.h"
#include "DSymWorld.h"
#include "DTeam.h"
#include "DNub.h"

#include <Path.h>
#include <Autolock.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <devel/disasm.h>
#include <devel/Unmangle.h>
//#include <memory>
#include <algorithm>
#include <numeric>

// std::auto_ptr<T> does not work with arrays, so here's a cheap class
// that provides array-deletion automatically for us.
template <class T>
class auto_array_ptr
{
public:
	auto_array_ptr(T* ptr) { _p = ptr; }
	~auto_array_ptr() { delete [] _p; }
	T* get() const { return _p; }

private:
	T* _p;
};

// ---------------------------------------------------------------------------
//	class HighPCSetter
//	Used as we iterate backwards through a sorted list of SymTabEntry's
//	Remember the last lowPC and use that for the current highPC
// 	Primed with the full length of the code area
//	
//	There can be multiple alias' to the same code.  For example:
//		21080   140 FUNC    WEAK    0  18 _IO_fopen
//		21080   140 FUNC    WEAK    0  18 fopen
//	To handle these cases, I keep track of both the low and high of the
// 	last entry visited.  If we have the lowPC == fNextLowPC, then I assign
//	the latest highPC instead.  In this way, any number of these runs
//	(there can be more than two) will all have the same correct size.
// ---------------------------------------------------------------------------

class HighPCSetter {
public:
	HighPCSetter(int codeLength) 						
	{ 
		fNextLowPC = codeLength;
		fNextHighPC = codeLength;
	}

	void operator()(DElfSymFile::SymTabEntry& symbol)	
	{
		symbol.highPC = (symbol.lowPC < fNextLowPC) ? fNextLowPC : fNextHighPC;
		fNextLowPC = symbol.lowPC;
		fNextHighPC = symbol.highPC;	
	}

private:
	uint32 fNextLowPC;
	uint32 fNextHighPC;
};

// ---------------------------------------------------------------------------
//	class PCOffset
//	Given a pc (as an offset into the library)
//	This class and associated functions tests if the pc is within a SymTabEntry
// ---------------------------------------------------------------------------

class PCOffset {
public:
	PCOffset(uint32 pc) { offset = pc; }
	uint32 offset;
};

bool operator< (const DElfSymFile::SymTabEntry& symbol, const PCOffset& aPC)
{
	// A symbol is less than a pc only if the highPC of the symbol is
	// less than or equal to the pc (remember that highPC is equal to next lowPC)
	return symbol.highPC <= aPC.offset;
}

bool operator< (const PCOffset& aPC, const DElfSymFile::SymTabEntry& symbol)
{
	// A pc is less than a symbol only if the pc is less than the lowPC
	// of the symbol
	return aPC.offset < symbol.lowPC;
}

// ---------------------------------------------------------------------------
//	class NameComparator
//	Compare a string name with the name in the symbol table for the
// 	given SymTabEntry
// ---------------------------------------------------------------------------

class NameComparator {
public:
	NameComparator(const char* name, const char *stringTable)	
	{ 
		fName = name; 
		fStringTable = stringTable; 
	}

	bool operator() (const DElfSymFile::SymTabEntry& symbol)
	{ 
		return strcmp(fName, fStringTable+symbol.name) == 0; 
	}

private:
	const char* fName;
	const char* fStringTable;
};

// ---------------------------------------------------------------------------
//	class FileIndexComparator
//	Compare a file index with the file index in a DAsmFile
// ---------------------------------------------------------------------------

class FileIndexComparator {
public:
	FileIndexComparator(DFileNr file)	
	{ 
		fFileIndex = file; 
	}

	bool operator() (const DElfSymFile::DAsmFile* const & asmFile) const
	{ 
		return asmFile->fFile == fFileIndex; 
	}

private:
	DFileNr	fFileIndex;
};

// this hash function does not work, dunno why...
//static unsigned long elf_hash(const unsigned char *name)
//{
//	unsigned long h = 0, g;
//	
//	while (*name)
//	{
//		h = (h << 4) + *name++;
//		if (g = h & 0xf0000000)
//			h ^= g >> 24;
//		h &= ~g;
//	}
//	return h;
//} // elf_hash

// ---------------------------------------------------------------------------
// class DSourceAnnotator
// Class to abstract the handling of a source file to annotate the
// assembly output
//
// Can be safely used even when the source doesn't exist
//
// Call it with the pc and it will annotate the output file with the
// lines of source for that pc
// ---------------------------------------------------------------------------

class DSourceAnnotator
{
public:
						DSourceAnnotator(const DSymWorld& symWorld, ptr_t pc);
						~DSourceAnnotator();
			
	uint32				AnnotateFile(BFile& textFile, ptr_t pc);
	
private:
	bool				SeekToLine(uint32 lineNumber);
	uint32				WriteToLine(BFile& textFile, uint32 lineNumber);

	const DSymWorld&	fSymWorld;
	uint32				fCurrentLine;
	ptr_t				fNextAnnotationPC;
	char*				fText;
	char*				fTextPosition;
	bool				fHasSource;
};

// ---------------------------------------------------------------------------
// #pragma mark -

DSourceAnnotator::DSourceAnnotator(const DSymWorld& symWorld, ptr_t pc)
				 : fSymWorld(symWorld)
{
	// Make sure all our data members are in a good state
	fText = NULL;
	fHasSource = false;
	fCurrentLine = 0;
	fTextPosition = 0;
	
	// See if we have a source file associated with this symbol or not
	// If so, get the file so that we can annotate the assembly
	DStatement statement;
	if (symWorld.GetSourceStatement(pc, statement)) 
	{		
		// Now set up the file
		// The key at the end is if fHasSource gets set or not, so
		// bail out early at any error
		BFile file(&DSourceFileTable::Instance()[statement.fFile], B_READ_ONLY);
		if (file.InitCheck() != B_OK) 
		{
			return;
		}
		
		off_t fileSize = 0;
		file.GetSize(&fileSize);
		fText = new char[fileSize + 1];
		if (file.Read(fText, fileSize) != fileSize) 
		{
			return;
		}
		fText[fileSize] = 0;
		fTextPosition = fText;
		fCurrentLine = 1;
		fNextAnnotationPC = pc;
		// finally we got the file open and correctly read it
		// now seek up to fCurrentLine
		uint32 startOfFunction = statement.fLine > 0 ? statement.fLine - 1 : statement.fLine;
		fHasSource = DSourceAnnotator::SeekToLine(startOfFunction);
	}
}

// ---------------------------------------------------------------------------

DSourceAnnotator::~DSourceAnnotator()
{
	delete [] fText;
}

// ---------------------------------------------------------------------------

uint32 DSourceAnnotator::AnnotateFile(BFile& textFile, ptr_t pc)
{
	// for each assembly line, we are given a chance to annotate
	// the file with the source 
	// If we don't have source, or we aren't up to the next spot
	// for annotation, then just return
	uint32 linesWritten = 0;

	// (Keep track of the locations for annotation by keeping track
	// of the pc range covered by each statement)
	if (fHasSource && (pc >= fNextAnnotationPC))
	{
		DStatement statement;
		if (fSymWorld.GetSourceStatement(pc, statement)) 
		{
			linesWritten = this->WriteToLine(textFile, statement.fLine+1);
			fNextAnnotationPC += statement.fSize;
		}
	}
	return linesWritten;
}

// ---------------------------------------------------------------------------

bool DSourceAnnotator::SeekToLine(uint32 lineNumber)
{
	// Seek to the file position that corresponds to the
	// start of lineNumber
	// updates fCurrentLine to be that position
	// The way this is written, we can only seek forward...
	
	while (fCurrentLine < lineNumber) 
	{
		fTextPosition = strchr(fTextPosition, '\n');
		if (fTextPosition == NULL)
		{
			// Something is wrong with our file - pretend we don't have source
			fHasSource = false;
			return false;
		}
		fTextPosition += 1;
		fCurrentLine += 1;
	}
	
	return true;
}

// ---------------------------------------------------------------------------

uint32 DSourceAnnotator::WriteToLine(BFile& file, uint32 lineNumber)
{
	// Write out all the text up to the position that
	// corresponds to the start of lineNumber

	// Keep track of our current position, and then seek to
	// the new line position
	// Write out everything inbetween...

	uint32 oldLine = fCurrentLine;	
	char* currentPosition = fTextPosition;
	if (this->SeekToLine(lineNumber))
	{
		file.Write(currentPosition, fTextPosition - currentPosition);
	}

	return fCurrentLine - oldLine;
}

// ---------------------------------------------------------------------------
//	DElfSymFile member functions
// ---------------------------------------------------------------------------
// #pragma mark -

DElfSymFile::DElfSymFile(const image_info& info)
	: DSymFile(info), fElf(fEntry)
{
	fBaseAddr -= fElf.ProgramOffset();
	fStrtab = NULL;
//	fHash = NULL;
//	fElf.LoadSection(".hash", fHash, size);
}

// ---------------------------------------------------------------------------

DElfSymFile::~DElfSymFile()
{
	delete [] fStrtab;
//	delete fHash;

	// notice that we are deleting but not erasing... this would have to be
	// changed if we weren't destroying the vector right away anyway
	for (AsmFileTab::iterator i = fAsmFileTable.begin(); i != fAsmFileTable.end(); i++)
	{
		delete *i;
	}
}

// ---------------------------------------------------------------------------

void DElfSymFile::LoadSymbols()
{
	// Used to be part of constructor, but failed DDwarfSymFiles would essentially
	// do the loading twice.
	// User needs to do just a little bit more work, but it saves 1/2 loading time
	
	size_t size;
	fElf.LoadSection(".strtab", fStrtab, size);
	
	const char *symtabcp;
	fElf.LoadSection(".symtab", symtabcp, size);
	const Elf32_Sym *symtabp = (const Elf32_Sym *)symtabcp;
	size /= sizeof(Elf32_Sym);

	// Some functions are STT_NOTYPE (but are GLOBAL) so
	// keep STT_NOTYPE && GLOBAL (throw away LOCAL)
	// Notice that we are not keeping data objects (type OBJECT) 
	for (uint32 i = 0; i < size; i++)
	{
		if (symtabp->st_shndx != STN_UNDEF &&
			((ELF32_ST_TYPE(symtabp->st_info) == STT_FUNC && ELF32_ST_BIND(symtabp->st_info) <= STB_WEAK) ||
			 (ELF32_ST_TYPE(symtabp->st_info) == STT_NOTYPE && ELF32_ST_BIND(symtabp->st_info) == STB_GLOBAL)))
		{
			SymTabEntry e;
			e.lowPC = symtabp->st_value;
			e.name = symtabp->st_name;
			// initialized later...
			e.highPC = 0;
			e.asmFile = NULL;
			fSymtab.push_back(e);
		}
		symtabp++;
	}
	delete[] symtabcp;
		
	sort(fSymtab.begin(), fSymtab.end());
	
	// Now that the symbols are sorted, go through and put in the highPC for all the entries
	// Iterate backwards, for each symbol, assign the previous (but next in sorted order)
	// low to the current symbol's highPC
	for_each(fSymtab.rbegin(), fSymtab.rend(), HighPCSetter(fCodeSize));
	
//	fprintf(stderr, "ELF symbol table for %s has %d entries\n", fImage.name, fSymtab.size());
//	for (symtab::const_iterator i = fSymtab.begin(); i != fSymtab.end(); i++)
//	{
//		const char *b = fStrtab + (*i).name;
//		fprintf(stderr, "%s = %x / %x\n", b, (*i).lowPC, (*i).highPC);
//	}
}

// ---------------------------------------------------------------------------

void DElfSymFile::GetFunctionName(ptr_t pc, string& name)
{
	pc -= fBaseAddr;

	// binary search to find associated symbol	
	symtab::const_iterator i = lower_bound(fSymtab.begin(), fSymtab.end(), PCOffset(pc));

	// verify that we actually found the correct symbol
	if (i != fSymtab.end() && (*i).lowPC <= pc && (*i).highPC > pc) 
	{
		const char *raw = fStrtab + (*i).name;
		char buf[UNAME_SIZE];
		if (demangle(raw, buf, UNAME_SIZE) > 0)
			name = buf;
		else
			name = raw;
	}	
}

// ---------------------------------------------------------------------------

void DElfSymFile::GetRawFunctionName(ptr_t pc, string& name)
{
	pc -= fBaseAddr;

	// binary search to find associated symbol	
	symtab::const_iterator i = lower_bound(fSymtab.begin(), fSymtab.end(), PCOffset(pc));

	// verify that we actually found the correct symbol
	if (i != fSymtab.end() && (*i).lowPC <= pc && (*i).highPC > pc) 
	{
		name = (fStrtab + (*i).name);
	}	
}

// ---------------------------------------------------------------------------

ptr_t DElfSymFile::GetFunctionLowPC(ptr_t pc)
{
	pc -= fBaseAddr;
	ptr_t absoluteLowPC = fBaseAddr;
	
	// binary search to find associated symbol	
	symtab::const_iterator i = lower_bound(fSymtab.begin(), fSymtab.end(), PCOffset(pc));

	// verify that we actually found the correct symbol
	if (i != fSymtab.end() && (*i).lowPC <= pc && (*i).highPC > pc) 
	{
		absoluteLowPC += (*i).lowPC;
	}	
	return absoluteLowPC;
}

// ---------------------------------------------------------------------------

ptr_t DElfSymFile::GetFunctionHighPC(ptr_t pc)
{
	pc -= fBaseAddr;
	ptr_t absoluteHighPC = fBaseAddr;
	
	// binary search to find associated symbol	
	symtab::const_iterator i = lower_bound(fSymtab.begin(), fSymtab.end(), PCOffset(pc));

	// verify that we actually found the correct symbol
	if (i != fSymtab.end() && (*i).lowPC <= pc && (*i).highPC > pc) 
	{
		absoluteHighPC += (*i).highPC;
	}
	return absoluteHighPC;
}

// ---------------------------------------------------------------------------

void DElfSymFile::GetFunctionOffsetAndSize(const char *name, ptr_t& offset, int& size)
{
	symtab::const_iterator i = this->FindEntry(name);
	if (i != fSymtab.end())
	{
		offset = (*i).lowPC + fBaseAddr;
		size = (*i).highPC - (*i).lowPC;
	}
	else {
		offset = fBaseAddr;
		size = 0;
	}		
}

// ---------------------------------------------------------------------------

void DElfSymFile::GetFunctionOffsetAndSize(ptr_t pc, ptr_t& offset, int& size)
{
	pc -= fBaseAddr;
	
	// binary search to find associated symbol	
	symtab::const_iterator i = lower_bound(fSymtab.begin(), fSymtab.end(), PCOffset(pc));

	// verify that we actually found the correct symbol
	if (i != fSymtab.end() && (*i).lowPC <= pc && (*i).highPC > pc) 
	{
		offset = (*i).lowPC + fBaseAddr;
		size = (*i).highPC - (*i).lowPC;
	}
	else {
		offset = fBaseAddr;
		size = 0;
	}		
}

// ---------------------------------------------------------------------------

DElfSymFile::symtab::const_iterator DElfSymFile::FindEntry(const char *name) const
{
	symtab::const_iterator i = find_if(fSymtab.begin(), fSymtab.end(), NameComparator(name, fStrtab));
	return i;

// Code that tried to use the hashtable follows:
	
//	const long *buckets, *chain, *t = (const long *)fHash;
//	int nbucket, nchain;
//	
//	nbucket = t[0];
//	nchain = t[1];
//	
//	buckets = t + 2;
//	chain = t + 2 + nbucket;
//
//	uint32 x, y;
//
//	x = elf_hash((const unsigned char *)name);
//	y = buckets[x % nbucket];
//
//	while (y != STN_UNDEF)
//	{
//		symtab::iterator i;// = find_if(fSymtab.begin(), fSymtab.end(), FindIndex(y));
//		
//		SymTabEntry e;
//		e.index = y;
//		i = fSymtab.find(e);
//		
//		if (i != fSymtab.end())
//		{
//			const char *f = fStrtab + (*i).name;
//			if (strcmp(name, f) == 0)
//			{
////				indx = i - fSymtab.begin();
//				break;
//			}
//		}
//		
//		y = chain[y];
//	}
//	
//	return y;
} // DElfSymFile::FindEntry

// ---------------------------------------------------------------------------

bool DElfSymFile::IsDisabled(ptr_t pc) const
{
	pc -= fBaseAddr;
	
	return (pc < fElf.SectionAddress(".text"));
}

// ---------------------------------------------------------------------------

bool DElfSymFile::ContainsName(const char *name) const
{
	return this->FindEntry(name) != fSymtab.end();
}

// ---------------------------------------------------------------------------

bool DElfSymFile::GetStatement(ptr_t pc, DStatement& statement, const DSymWorld& symWorld)
{
	pc -= fBaseAddr;
	
	// binary search to find associated symbol	
	symtab::iterator functionIndx = lower_bound(fSymtab.begin(), fSymtab.end(), PCOffset(pc));

	// verify that we are not looking at .end() or the next symbol down 
	// (which lower_bound does if it doesn't find what we want)
	if (functionIndx == fSymtab.end() || (*functionIndx).lowPC > pc) 
	{
		return false;
	}

	return this->GetStatement(*functionIndx, pc, statement, symWorld);
}

// ---------------------------------------------------------------------------

bool DElfSymFile::GetAsmStatement(ptr_t pc, DStatement& statement, const DSymWorld& symWorld)
{
	// Don't call GetStatement virtually - we want our asm statement
	return DElfSymFile::GetStatement(pc, statement, symWorld);
}

// ---------------------------------------------------------------------------

bool DElfSymFile::GetStatement(const char* name, DStatement& statement, const DSymWorld& symWorld)
{
	symtab::const_iterator i = (symtab::const_iterator) this->FindEntry(name);
	if (i == fSymtab.end()) {
		return false;
	}
	
	return this->GetStatement((SymTabEntry&) *i, (*i).lowPC, statement, symWorld);
}

// ---------------------------------------------------------------------------

bool DElfSymFile::GetStatement(SymTabEntry& symbol, ptr_t pc, DStatement& statement, const DSymWorld& symWorld)
{
	// We build the assembly files on demand, if we don't have it yet
	// then build it and set up the reference from the symbol table entry
	if (symbol.asmFile == NULL) 
	{
		try 
		{
			symbol.asmFile = this->CreateAsmFile(symbol, symWorld);
			fAsmFileTable.push_back(symbol.asmFile);
		}
		catch (...)
		{
			return false;
		}
	}
	
	
	symbol.asmFile->FillStatement(pc, statement);
	return true;
}

// ---------------------------------------------------------------------------

void DElfSymFile::GetStopsForFile(DFileNr file, std::set<int>& lines)
{
	// See if we actually own file, and if so, then return the set of stopable lines in that file
	// (which for assembly is all of them)
	
	AsmFileTab::const_iterator i = find_if(fAsmFileTable.begin(), fAsmFileTable.end(), FileIndexComparator(file));
	if (i != fAsmFileTable.end())
	{
		(*i)->GetStopsForFile(lines);
	}
}

// ---------------------------------------------------------------------------

ptr_t DElfSymFile::GetStatementOffset(DFileNr file, int line)
{
	AsmFileTab::const_iterator i = find_if(fAsmFileTable.begin(), fAsmFileTable.end(), FileIndexComparator(file));
	if (i != fAsmFileTable.end())
	{
		return (*i)->GetStatementOffset(line) + fBaseAddr;
	}
	return 0;
}

// ---------------------------------------------------------------------------

DElfSymFile::DAsmFile* DElfSymFile::CreateAsmFile(const SymTabEntry& symbol, const DSymWorld& symWorld)
{
	// Create a dummy file that contains the disassembled output for the function
	// Wrap the file in a DAsmFile
	// Iterate through and disassemble the function
	// Stuff the disassembly into a file

	// Get the nub and read the memory of the function
	int32 functionSize = symbol.highPC - symbol.lowPC;
	auto_array_ptr<char> codeBuffer(new char[functionSize]);
	// we only need the lock for the read memory itself...
	{
		DNub& nub = symWorld.GetTeam()->GetNub();
		BAutolock lock(nub);
		nub.ReadData(fBaseAddr+symbol.lowPC, codeBuffer.get(), functionSize);
	}
	
	// Figure out the name we are going to use <temp directory>/<fileName>.asm
	
	BString leafName;
	char demangledName[UNAME_SIZE];
	const char *name = fStrtab + symbol.name;
	if (demangle(name, demangledName, UNAME_SIZE) > 0) {
		leafName = demangledName;
	}
	else {
		leafName = name;
	}
	leafName += ".asm";
	leafName.ReplaceAll(':', '.');
	
	// Set up the file to use
	// (if we run into problems, return a null
	BPath path;
	status_t err = find_directory(B_COMMON_TEMP_DIRECTORY, &path, true);
	if (err != B_OK) {
		THROW(("Can't find temp directory"));
	}	
	path.SetTo(path.Path(), leafName.String());
	BFile textFile(path.Path(), B_READ_WRITE+B_CREATE_FILE+B_ERASE_FILE);
	if (textFile.InitCheck() != B_OK) {
		THROW(("Can't create asm file"));
	}

	// While we are at it, go ahead and set up the type of the file
	BNodeInfo mimefile(&textFile);
	mimefile.SetType("text/x-source-code");
	
	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);
	DAsmFile* newFile = new DAsmFile(DSourceFileTable::Instance().AddFile(ref), symbol);
	newFile->Disasm(textFile, codeBuffer.get(), fBaseAddr, symWorld);
	return newFile;	
}

// ---------------------------------------------------------------------------
// DAsmFile member functions and helpers
// ---------------------------------------------------------------------------
// #pragma mark -

// ---------------------------------------------------------------------------

static status_t DisasmSymbolLookup(void *cookie, 
							uint32 eip, 
							uint32 *sym_addr, 
							char *sym_name, 
							int max_name_len, 
							int /*is_lower*/)
{
	DSymWorld* symWorld = (DSymWorld*) cookie;
	string name;
	symWorld->GetFunctionName(eip, name);
	*sym_addr = symWorld->GetFunctionLowPC (eip);
	strncpy(sym_name, name.c_str(), max_name_len);
	return B_OK;
}

DElfSymFile::DAsmFile::DAsmFile(DFileNr file, const SymTabEntry& symbol)
		 : fFile(file), fSymbol(symbol)
{
}

// ---------------------------------------------------------------------------

void DElfSymFile::DAsmFile::Disasm(BFile& textFile, const char* codeBuffer, ptr_t baseAddress, const DSymWorld& symWorld)
{
	// Iterate through the buffer and disassemble it all - writing each line to the file
	// See if we can anotate the assembly with the source	
	DSourceAnnotator annotator(symWorld, baseAddress + fSymbol.lowPC);
	uint32 offset = 0;
	uint32 curLine = 1;		// start at the first line of the file, and proceed
	ptr_t symbolOffset = baseAddress + fSymbol.lowPC;
	uint32 functionSize = fSymbol.highPC - fSymbol.lowPC;
	char textualBuffer[512];
	
	bool defaultStyle = gPrefs->GetPrefInt("at&t style disasm", 1);

	fStops.clear();
	while (offset < functionSize)
	{
		// TODO:  Specify symbol lookup function
		long instructionLength = disasm((uchar *)(codeBuffer+offset), 
										functionSize-offset, 
										textualBuffer, 
										sizeof(textualBuffer), 
										symbolOffset+offset, 
										defaultStyle ? 0 : B_DISASM_FLAG_INTEL_STYLE, 
										&DisasmSymbolLookup, 
										(DSymWorld*) &symWorld);
		if (instructionLength == -1)
		{
			// no more asm, so we're done
			break;
		}

		// valid asm, so first write the source if appropriate, keeping track of
		// how many lines of source (i.e. non-breakpointable lines) were written
		uint32 srcLines = annotator.AnnotateFile(textFile, symbolOffset+offset);
		for (uint32 m = 0; m < srcLines; m++)
		{
			// note the instruction sizes of the source lines, i.e. null [annotation only]
			fLineSizes.push_back(0);
		}
		curLine += srcLines;		// record how many lines of source we just wrote

		// Write the following to the file:
		//		+ offset (address):	instruction
		//		where...
		//			offset = 4 digits hex
		//			address = 8 digits hex
		auto_array_ptr<char> fileLine(new char[strlen(textualBuffer) + 80]);
		sprintf(fileLine.get(), "+%04lx  %08lx:        %s\n",
				offset, symbolOffset+offset, textualBuffer);

		// we're about to write an asm instruction, so the current line is a stop
		fStops.insert(curLine);
		textFile.Write(fileLine.get(), strlen(fileLine.get()));
		curLine++;		// count the asm line we just wrote

		fLineSizes.push_back(instructionLength);
		offset += instructionLength;
	}
}

// ---------------------------------------------------------------------------

void DElfSymFile::DAsmFile::FillStatement(ptr_t pc, DStatement& statement)
{
	statement.fFile = fFile;
	statement.fPC = pc;
	
	// calculate the line number that corresponds to the pc
	ptr_t offset = fSymbol.lowPC;
	int32 index = 0;
	std::vector<int32>::const_iterator i;
	for (i = fLineSizes.begin(); i != fLineSizes.end() && offset < pc; i++)
	{
		offset += (*i);
		index += 1;	
	}

	// if we're pointing to a source annotation before the actual asm
	// instruction at the given PC, skip over it to place the PC arrow on
	// the asm statement.
	while (fLineSizes[index] == 0)
	{
		index++;
	}

	// TODO: Do we tell the user about this if we fail??
	ASSERT(offset == pc);
	ASSERT(i != fLineSizes.end());
	
	// (remember that line number = index + 1)
	statement.fLine = index + 1;
	statement.fSize = fLineSizes[index] - 1;	
	statement.fSourceLevel = false;
}

// ---------------------------------------------------------------------------

void DElfSymFile::DAsmFile::GetStopsForFile(std::set<int>& lines)
{
	// Warning: If we allow viewing and stopping on mixed asm/source then we
	// have to set up a real set of stopable lines in DElfSymFile::DAsmFile::Disasm.  
	// Meanwhile, we can infer that ...
	// every line in an assembly file is stopable
	// since the first line is 1 in the source, go from [1..count]
	lines = fStops;
}

// ---------------------------------------------------------------------------

ptr_t DElfSymFile::DAsmFile::GetStatementOffset(int line)
{
	// Given the line number, decide its corresponding offset in library
	// iterate the line sizes adding up until we get to the line we want
	
	// convert line to index base
	line -= 1;	
	ASSERT(line <= fLineSizes.size());
	ptr_t offset = accumulate(fLineSizes.begin(), fLineSizes.begin()+line, fSymbol.lowPC);
	return offset;
}
