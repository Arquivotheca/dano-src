// ---------------------------------------------------------------------------
/*
	ELFReader.h
	Currently there are a lot of simplications done to this reader,
	but it could serve as the base for a more generic reader.
		
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			20 October 1998

*/
// ---------------------------------------------------------------------------

#ifndef _ELFREADER_H
#define _ELFREADER_H

#include <ByteOrder.h>
#include <SupportDefs.h>
#include <File.h>
#include <elf.h>

// ---------------------------------------------------------------------------
//	class SectionSpec
//	A name and kind for a section
// ---------------------------------------------------------------------------

class SectionSpec
{
public:
	enum EKind { kUnknown = 0, kCode = 1, kData = 2 };

	const char* 			fSectionName;
	SectionSpec::EKind		fKind;
};


// ---------------------------------------------------------------------------
//	Class ELFReader
// ---------------------------------------------------------------------------

class ELFReader
{
public:
						ELFReader(const char* filePath);
	virtual				~ELFReader();
		
	virtual status_t 	CodeDataSize(int32& codeSize, int32& dataSize);

	status_t			InitCheck();

private:
	SectionSpec::EKind	FindCodeDataSection(const Elf32_Shdr& sectionHeader);
	
	Elf32_Word			Convert(Elf32_Word arg);
	Elf32_Half			Convert(Elf32_Half arg);

private:
	BFile 				fFile;
	Elf32_Ehdr			fFileHeader;
	Elf32_Shdr*			fSectionHeaders;
	char*				fStringSection;
};

// ---------------------------------------------------------------------------

inline Elf32_Word
ELFReader::Convert(Elf32_Word arg)
{
	if (fFileHeader.e_ident[EI_DATA] == ELFDATA2LSB) {
		return B_LENDIAN_TO_HOST_INT32(arg);
	}
	else {
		return B_BENDIAN_TO_HOST_INT32(arg);
	}

}

// ---------------------------------------------------------------------------

inline Elf32_Half
ELFReader::Convert(Elf32_Half arg)
{
	if (fFileHeader.e_ident[EI_DATA] == ELFDATA2LSB) {
		return B_LENDIAN_TO_HOST_INT16(arg);
	}
	else {
		return B_BENDIAN_TO_HOST_INT16(arg);
	}

}

#endif
