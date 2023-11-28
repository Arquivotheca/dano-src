/* ++++++++++
	elf.h
	Copyright (C) 1991 Be Labs, Inc.  All Rights Reserved.
	Definitions for AT&T's ELF object file format.
	
	Modification History (most recent first):

	24 may 91	rwh		added section stuff for debugger symbol tables.
	?? apr 91	elr		created definitions needed for loader.

+++++ */

#ifndef _ELF_H
#define _ELF_H

typedef char			*Elf32_Addr;
typedef unsigned short	Elf32_Half;
typedef unsigned long	Elf32_Off;
typedef long			Elf32_Sword;
typedef unsigned long	Elf32_Word;


/* indices for the e_ident array */

#define EI_MAG0		0		/* file identification */
#define EI_MAG1		1		/* file identification */
#define EI_MAG2		2		/* file identification */
#define EI_MAG3		3		/* file identification */
#define EI_CLASS	4		/* file class (32,64... bit) */
#define EI_DATA		5		/* data encoding */
#define EI_VERSION	6		/* elf header version # */
#define EI_PAD		7		/* index of 1st pad byte */
#define EI_NIDENT	16		/* size of e_ident[] */

#define	ELFMAG0		0x7f
#define	ELFMAG1		'E'
#define	ELFMAG2		'L'
#define	ELFMAG3		'F'

#define	ELFMAG		"\177ELF"

typedef struct {
	unsigned char	e_ident[EI_NIDENT];	/* file identifiers */
	Elf32_Half		e_type;				/* object file type */
	Elf32_Half		e_machine;			/* CPU architecture */
	Elf32_Word		e_version;			/* object file version */
	Elf32_Addr		e_entry;			/* Va of entry point */
	Elf32_Off		e_phoff;			/* pheader file offset */
	Elf32_Off		e_shoff;			/* section header file offset */
	Elf32_Word		e_flags;			/* machine specific flags */
	Elf32_Half		e_ehsize;			/* elf header size */
	Elf32_Half		e_phentsize;		/* size of a pheader entry */
	Elf32_Half		e_phnum;			/* # of program header entries */
	Elf32_Half		e_shentsize;		/* size of section hdr entry */
	Elf32_Half		e_shnum;			/* # of section header entries */
	Elf32_Half		e_shstrndx;			/* section header table index */
} Elf32_Ehdr;


/* e_ident [EI_CLASS] values */

#define ELFCLASSNONE	0	/* invalid class */
#define ELFCLASS32	1		/* 32 bit objects */
#define ELFCLASS64	2		/* 64 bit objects */

/* e_ident [EI_DATA] values */

#define ELFDATANONE	0		/* invalid data encoding */
#define ELFDATA2LSB	1		/* little endian, 2's complement */
#define ELFDATA2MSB	2		/* big endian, 2's complement */

/* e_ident [EI_VERSION] values */

#define EV_NONE		0		/* invalid version */
#define EV_CURRENT	1		/* current ELF version (if new one, this changes) */

/* types for the e_type field */

#define ET_NONE		0
#define ET_REL		1		/* relocatable type */
#define ET_EXEC		2		/* executable file */
#define ET_DYN		3		/* shared object file */
#define ET_CORE		4		/* core file */

/* values for the e_machine field */

#define EM_NONE		0		/* no machine */
#define EM_M32		1		/* AT&T WE 32100 */
#define EM_SPARC	2		/* SPARC */
#define EM_386		3		/* Intel 386 */
#define EM_68k		4		/* Motorola 68000 */
#define EM_88k		5		/* Motorola 88000 */
#define EM_860		6		/* Intel 860 */
#define EM_PPCTOC	0xb
#define EM_PPC		0x11	/* IBM/Motorola PowerPC */
#define EM_HOBBIT	925		/* AT&T Hobbit */


/* the format for a program header entry */

typedef struct {
	Elf32_Word	p_type;		/* segment type */
	Elf32_Off	p_offset;	/* file offset of segment */
	Elf32_Addr	p_vaddr;	/* virtual address of segment */
	Elf32_Addr	p_paddr;	/* physical address of segment */
	Elf32_Word	p_filesz;	/* # file bytes of segment */
	Elf32_Word	p_memsz;	/* # memory bytes of segment */
	Elf32_Word	p_flags;	/* segment flags */
	Elf32_Word	p_align;	/* alignment value */
} Elf32_Phdr;

/* types for the p_type field */

#define PT_NULL		0
#define PT_LOAD		1		/* loadable segment */
#define PT_DYNAMIC	2		/* dynamic linking information */
#define PT_INTERP	3		/* segment interpreter */
#define PT_NOTE		4		/* a 'note' segment */
#define PT_SHLIB	5		/* reserved, but unspecified */
#define PT_PHDR		6		/* program header info, for INTERP */

/* types for the p_flags field */

#define PF_X		1		/* Execute */
#define PF_W		2		/* Write */
#define PF_R		4		/* Read */




/* reserved section header table indices */

#define SHN_UNDEF	0			/* undef/missing/irrelevant index */
#define SHN_ABS		0xfff1		/* reference is an absolute value */
#define SHN_COMMON	0xfff2		/* Fortran COMMON/unallocated C external */


/* the format for a section header entry */

typedef struct {
	Elf32_Word	sh_name;		/* section name */
	Elf32_Word	sh_type;		/* section type */
 	Elf32_Word	sh_flags;		/* misc attribute flags */
	Elf32_Addr	sh_addr;		/* address in memory image */
	Elf32_Off	sh_offset;		/* file offset of section */
	Elf32_Word	sh_size;		/* size of section */
	Elf32_Word	sh_link;		/* type-dependent link to other section */
	Elf32_Word	sh_info;		/* type-dependent extra info */
	Elf32_Word	sh_addralign;	/* address alignment modulus */
	Elf32_Word	sh_entsize;		/* entry size, for tables */
} Elf32_Shdr;

/* types for the sh_type field */

#define SHT_NULL		0		/* inactive section */
#define SHT_PROGBITS	1		/* program-defined info */
#define SHT_SYMTAB		2		/* link edit/dyn link symbol table */
#define SHT_STRTAB		3		/* string table */
#define SHT_RELA		4		/* relocation entries w/addends */
#define SHT_HASH		5		/* a symbol hash table */
#define	SHT_DYNAMIC		6		/* dynamic linking information */
#define SHT_NOTE		7		/* note information */
#define SHT_NOBITS		8		/* program-defined, no file space info */
#define SHT_REL			9		/* relocation entries w/o addends */
#define SHT_SHLIB		10		/* reserved, meaning unspecified */
#define SHT_DYNSYM		11		/* minimal dynamic link symbol table */

/* types for the sh_flags field */

#define SHF_WRITE		0x1		/* section writeable */
#define SHF_ALLOC		0x2		/* section occupies memory */
#define SHF_EXECINSTR	0x4		/* section contains executable code */

/* the format of a symbol table entry */

typedef struct {
	Elf32_Word		st_name;	/* index to name in string table */
	Elf32_Addr		st_value;	/* symbol value */
	Elf32_Word		st_size;	/* symbol size */
	unsigned char	st_info;	/* symbol type & binding attributes */
	unsigned char	st_other;	/* no meaning currently */
	Elf32_Half		st_shndx;	/* section index sym defined relative to */
} Elf32_Sym;

/* the format of the relocation entries */

typedef struct {
	Elf32_Addr		r_offset;
	Elf32_Word		r_info;
} Elf32_Rel;

typedef struct {
	Elf32_Addr		r_offset;
	Elf32_Word		r_info;
	Elf32_Sword		r_addend;
} Elf32_Rela;

#define	ELF32_R_SYM(i)		((i)>>8)
#define	ELF32_R_TYPE(i)		((unsigned char)(i))
#define	ELF32_R_INFO(s,t)	(((s)<<8)+(unsigned char)(t))

#define	R_386_NONE		0
#define	R_386_32		1
#define	R_386_PC32		2
#define	R_386_GOT32		3
#define	R_386_PLT32		4
#define	R_386_COPY		5
#define	R_386_GLOB_DAT	6
#define	R_386_JMP_SLOT	7
#define	R_386_RELATIVE	8
#define	R_386_GOTOFF	9
#define R_386_GOTPC		10

/* the format of the dynamic section */

typedef struct {
	Elf32_Sword		d_tag;
	union {
		Elf32_Word	d_val;
		Elf32_Addr	d_ptr;
	} d_un;
} Elf32_Dyn;

#define	DT_NULL		0
#define	DT_NEEDED	1
#define	DT_PLTRELSZ	2
#define	DT_PLTGOT	3
#define	DT_HASH		4
#define	DT_STRTAB	5
#define	DT_SYMTAB	6
#define	DT_RELA		7
#define	DT_RELASZ	8
#define	DT_RELAENT	9
#define	DT_STRSZ	10
#define	DT_SYMENT	11
#define	DT_INIT		12
#define	DT_FINI		13
#define	DT_SONAME	14
#define	DT_RPATH	15
#define	DT_SYMBOLIC	16
#define	DT_REL		17
#define	DT_RELSZ	18
#define	DT_RELENT	19
#define	DT_PLTREL	20
#define	DT_DEBUG	21
#define	DT_TEXTREL	22
#define	DT_JMPREL	23
#define DT_VERSYM		0x6ffffff0
#define	DT_VERDEF		0x6ffffffc
#define	DT_DEFNUM		0x6ffffffd
#define DT_VERNEED		0x6ffffffe
#define DT_VERNEEDNUM	0x6fffffff

/* macros to get/set symbol type & binding info */

#define ELF32_ST_BIND(i)	((i)>>4)
#define ELF32_ST_TYPE(i)	((i)&0xf)
#define ELF32_ST_INFO(b,t)	(((b<<4)+((t)&0xf)))

#define	STN_UNDEF	0

/* symbol bindings */

#define STB_LOCAL	0		/* not visible outside defining file */
#define STB_GLOBAL	1		/* visible to all object files */
#define STB_WEAK	2		/* lower precedence global */

/* symbol types */

#define STT_NOTYPE	0		/* type not specified */
#define STT_OBJECT	1		/* a data object (variable, array.. */
#define STT_FUNC	2		/* function or executable code */
#define STT_SECTION	3		/* associated w/section */ 
#define STT_FILE	4		/* name of source file */


#endif
