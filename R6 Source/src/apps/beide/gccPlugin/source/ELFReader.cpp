// ---------------------------------------------------------------------------
/*
	ELFReader.cpp
	Currently there are a lot of simplications done to this reader,
	but it could serve as the base for a more generic reader.
		
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			20 October 1998

*/
// ---------------------------------------------------------------------------

#include "ELFReader.h"

#include <stdio.h>

// ---------------------------------------------------------------------------
// Static data for looking up code/data sections
// ---------------------------------------------------------------------------

static SectionSpec gCodeDataSections[] = {
	{ ".text",	SectionSpec::kCode },
	{ ".data",	SectionSpec::kData },
	{ ".bss",	SectionSpec::kData },
	{ ".fini",	SectionSpec::kCode },
	{ ".init",	SectionSpec::kCode }
};

static const long kNumberCodeDataSections = sizeof(gCodeDataSections) / sizeof(SectionSpec);

// ---------------------------------------------------------------------------
// ELFReader member functions
// ---------------------------------------------------------------------------

ELFReader::ELFReader(const char* filePath)
		  : fFile(filePath, B_READ_ONLY)
{
	fSectionHeaders = NULL;
	fStringSection = NULL;
}

// ---------------------------------------------------------------------------

ELFReader::~ELFReader()
{
	delete [] fSectionHeaders;
	delete [] fStringSection;
}

// ---------------------------------------------------------------------------

status_t
ELFReader::InitCheck()
{
	// check that the file is initialized correctly and that
	// it is readable
	// If so, read in the header and make sure it is an ELF file
	
	bool fileOK = fFile.InitCheck() == B_OK;
	if (fileOK) {
		fileOK = fFile.IsReadable();
	}
	if (fileOK) {
		// assume the worst
		fileOK = false;
		long bytesRead = fFile.ReadAt(0, &fFileHeader, sizeof(Elf32_Ehdr));
		if (bytesRead == sizeof(Elf32_Ehdr) &&
				fFileHeader.e_ident[EI_MAG0] == ELFMAG0 &&
				fFileHeader.e_ident[EI_MAG1] == ELFMAG1 &&
				fFileHeader.e_ident[EI_MAG2] == ELFMAG2 &&
				fFileHeader.e_ident[EI_MAG3] == ELFMAG3 &&
				Convert(fFileHeader.e_shentsize) == sizeof(Elf32_Shdr)) {
			// the elf identification (and section header size) checks out
			fileOK = true;
		}
	}
		
	return fileOK ? B_OK : B_ERROR;
}

// ---------------------------------------------------------------------------

status_t
ELFReader::CodeDataSize(int32& codeSize, int32& dataSize)
{
	// initialize our sizes
	codeSize = 0;
	dataSize = 0;
	
	// Create the buffer for all the section headers
	// And read them in...
	int16 headerCount = Convert(fFileHeader.e_shnum);
	if (fSectionHeaders == NULL) {
		long totalHeaderSize = headerCount * sizeof(Elf32_Shdr);
		fSectionHeaders = new Elf32_Shdr[headerCount];
		long bytesRead = fFile.ReadAt(Convert(fFileHeader.e_shoff), fSectionHeaders, totalHeaderSize);
		// if we didn't get our bytes - bail out early
		if (bytesRead != totalHeaderSize) {
			return B_ERROR;
		}
	}
	
	// Read in the string section
	if (fStringSection == NULL) {
		Elf32_Shdr* stringSectionHeader = &fSectionHeaders[Convert(fFileHeader.e_shstrndx)];
		Elf32_Off stringSectionOffset = Convert(stringSectionHeader->sh_offset);
		Elf32_Word stringSectionSize = Convert(stringSectionHeader->sh_size);
		fStringSection = new char[stringSectionSize];
		long bytesRead = fFile.ReadAt(stringSectionOffset, fStringSection, stringSectionSize);
		// if we didn't get our bytes - bail out early
		if (bytesRead != (long) stringSectionSize) {
			return B_ERROR;
		}
	}
	
	// Now iterate through the headers and look for code/data
	// Use the gCodeDataSections array to decide if each section is the kind we want
	
	for (int i = 0; i < headerCount; i++) {
		Elf32_Shdr* aHeader = &fSectionHeaders[i];
		SectionSpec::EKind kind = this->FindCodeDataSection(*aHeader);
		if (kind == SectionSpec::kCode) {
			codeSize += Convert(aHeader->sh_size);
		}
		else if (kind == SectionSpec::kData) {
			dataSize += Convert(aHeader->sh_size);
		}
	}
	
	return B_OK;
}

// ---------------------------------------------------------------------------

SectionSpec::EKind
ELFReader::FindCodeDataSection(const Elf32_Shdr& sectionHeader)
{
	// Look up the name of the section header
	// Compare against the names of sections we are looking for
	// If we find a match, return the kind of the header

	// rather than doing strcmp n times, I'll take the first 4 characters
	// of the string as a long.  (The first 4 characters of section header
	// names are unique.)
	
	SectionSpec::EKind kind = SectionSpec::kUnknown;
	for (int i = 0; i < kNumberCodeDataSections; i++) {
		long nameOffset = Convert(sectionHeader.sh_name);
		long* sectionName = (long*) &fStringSection[nameOffset];
		long* specName = (long*) gCodeDataSections[i].fSectionName;
		if (*sectionName == *specName) {
			kind = gCodeDataSections[i].fKind;
			break;
		}
	}
	
	return kind;
}
