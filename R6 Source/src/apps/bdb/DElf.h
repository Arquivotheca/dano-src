/*	$Id: DElf.h,v 1.6 1999/02/11 15:51:52 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 10:57:32
*/

#ifndef DELF_H
#define DELF_H

#include "elf.h"

#include <Entry.h>
#include <File.h>

class DElf
{
  public:
	DElf(entry_ref& image);
	~DElf();

	void LoadSection(const char *section, const char*& data, std::size_t& size) const;
	void LoadDebugInfoPart(uint32 offset, const char *& data, uint32& size) const;
	
	ptr_t SectionAddress(const char *section) const;
	size_t SectionSize(const char *section) const;

	ptr_t ProgramOffset() const { return pcOffset; }

  protected:
  	
  	DElf& operator=(const DElf&);
  
	mutable BFile	file;
  	char				*fileName;
	char 				*sect_strs;
	Elf32_Ehdr		hdr;
	Elf32_Shdr*	sections;
	Elf32_Phdr*	programs;
	ptr_t				pcOffset;
};

#endif
