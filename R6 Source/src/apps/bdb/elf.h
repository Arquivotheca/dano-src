/* Copyright (C) 2000, Be Inc.

   Just enough ELF structs, defines, and macros to allow BDB to grab
   what it needs out of an ELF format file.  This information comes
   from "Executable and Linkable Format (ELF)", published by Intel in
   their Tool Interface Standards (TIS) documentation.	*/

/* Typedefs to implement the 32 bit data types used in the ELF
   standard. */

typedef uint32	Elf32_Addr;	/* 4 byte unsigned program address */
typedef uint16	Elf32_Half;	/* 2 byte unsigned medium integer */
typedef uint32	Elf32_Off;	/* 4 byte unsigned file offset */
typedef int32	Elf32_Sword;	/* 4 byte signed large integer */
typedef uint32	Elf32_Word;	/* 4 byte unsigned large integer */

/* Layout of the ELF header. */
#define EI_NIDENT 16

typedef struct
{
  unsigned char e_ident[EI_NIDENT];
  Elf32_Half	e_type;
  Elf32_Half	e_machine;
  Elf32_Word	e_version;
  Elf32_Addr	e_entry;
  Elf32_Off	e_phoff;
  Elf32_Off	e_shoff;
  Elf32_Word	e_flags;
  Elf32_Half	e_ehsize;
  Elf32_Half	e_phentsize;
  Elf32_Half	e_phnum;
  Elf32_Half	e_shentsize;
  Elf32_Half	e_shnum;
  Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

/* Layout of the Program Header */
typedef struct
{
	Elf32_Word	p_type;
	Elf32_Off		p_offset;
	Elf32_Addr	p_vaddr;
	Elf32_Addr	p_paddr;
	Elf32_Word	p_filesz;
	Elf32_Word	p_memsz;
	Elf32_Word	p_flags;
	Elf32_Word	p_align;
} Elf32_Phdr;

/* Layout of section headers. */
typedef struct
{
  Elf32_Word	sh_name;
  Elf32_Word	sh_type;
  Elf32_Word	sh_flags;
  Elf32_Addr	sh_addr;
  Elf32_Off	sh_offset;
  Elf32_Word	sh_size;
  Elf32_Word	sh_link;
  Elf32_Word	sh_info;
  Elf32_Word	sh_addralign;
  Elf32_Word	sh_entsize;
} Elf32_Shdr;

/* Layout of symbol table entries. */
typedef struct
{
  Elf32_Word	st_name;
  Elf32_Addr	st_value;
  Elf32_Word	st_size;
  unsigned char st_info;
  unsigned char st_other;
  Elf32_Half	st_shndx;
} Elf32_Sym;

/* Access st_info parts. */
#define ELF32_ST_BIND(x) ((x) >> 4)
#define ELF32_ST_TYPE(x) ((x) & 0xf)

#define STN_UNDEF	0

#define STT_NOTYPE	0
#define STT_FUNC	2

#define STB_GLOBAL	1
#define STB_WEAK	2
