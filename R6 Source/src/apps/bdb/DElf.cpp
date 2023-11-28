/*	$Id: DElf.cpp,v 1.7 1999/02/11 15:51:52 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 13:49:08
*/

#include "bdb.h"
#include "DElf.h"

DElf::DElf(entry_ref& image)
{
	sect_strs = NULL;
	fileName = strdup(image.name);

	FailOSErr(file.SetTo(&image, B_READ_ONLY));

	file.Read(&hdr, sizeof(hdr));

	// read the sections
	ASSERT_OR_THROW (sizeof(Elf32_Shdr) == hdr.e_shentsize);		// we need this for reading section headers
	sections = new Elf32_Shdr[hdr.e_shnum];

	file.ReadAt(hdr.e_shoff, sections, sizeof(Elf32_Shdr) * hdr.e_shnum);
	
	Elf32_Shdr *shstr = sections + hdr.e_shstrndx;
	sect_strs = new char[shstr->sh_size];

	file.ReadAt(shstr->sh_offset, sect_strs, shstr->sh_size);

	// read the program headers
	ASSERT_OR_THROW(sizeof(Elf32_Phdr) == hdr.e_phentsize);		// we need this for reading program headers
	programs = new Elf32_Phdr[hdr.e_phnum];
	file.ReadAt(hdr.e_phoff, programs, sizeof(Elf32_Phdr) * hdr.e_phnum);

	// the first program header has the executable vaddr offsets etc.
	pcOffset = programs[0].p_vaddr;
} /* DElf::DElf */

DElf::~DElf ()
{
	delete[] sect_strs;
	delete[] sections;
	delete [] programs;
	free(fileName);
} // DElf::~DElf

void DElf::LoadSection (const char *section, const char*& data, std::size_t& size) const
{
	int i;

	for (i = 0; i < hdr.e_shnum; i++)
	{
		if (strcmp(section, sect_strs + sections[i].sh_name) == 0)
		{
			size = sections[i].sh_size;
			char *d = new char[size];
			data = d;
			
			off_t off = sections[i].sh_offset;
			
			while (size > 0)
			{
				size_t s = std::min(size, 64UL * 1024UL);		// 64K chunks to bypass the cache

				file.ReadAt(off, d, s);
				off += s;
				d += s;
				size -= s;
			}
			
			size = sections[i].sh_size;
			return;
		}
	}

	THROW(("Section \"%s\" not found in %s!", section, fileName));
} // DElf::LoadSection

void DElf::LoadDebugInfoPart(uint32 offset, const char *& data, uint32& size) const
{
	int i;

	for (i = 0; i < hdr.e_shnum; i++)
	{
		if (strcmp(".debug_info", sect_strs + sections[i].sh_name) == 0)
		{
			off_t off = sections[i].sh_offset + offset;
			
			FailOSErr2(file.ReadAt(off, &size, sizeof(size)));
			size += sizeof(size);

			char *d = new char[size];
			data = d;
			
			while (size > 0)
			{
				size_t s = std::min(size, 32768UL);

				FailOSErr2(file.ReadAt(off, d, s));
				off += s;
				d += s;
				size -= s;
			}
			
			size = *(size_t*)data;
			return;
		}
	}

	THROW(("Section \"%s\" not found in %s!", ".debug_info", fileName));
} // DElf::LoadDebugInfoPart

ptr_t DElf::SectionAddress(const char *section) const
{
	int i;

	for (i = 0; i < hdr.e_shnum; i++)
	{
		if (strcmp(section, sect_strs + sections[i].sh_name) == 0)
			return sections[i].sh_addr;
	}
	
	THROW(("Section %s not found!", section));
} // DElf::SectionAddress

size_t DElf::SectionSize(const char *section) const
{
	int i;

	for (i = 0; i < hdr.e_shnum; i++)
	{
		if (strcmp(section, sect_strs + sections[i].sh_name) == 0)
			return sections[i].sh_size;
	}
	
	THROW(("Section %s not found!", section));
} // DElf::SectionSize
