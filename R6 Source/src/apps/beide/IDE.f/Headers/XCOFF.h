
/*
 *  XCOFF.h - XCOFF Data Types for Metrowerks C++ (PowerPC)
 *
 *  Copyright © 1993 metrowerks inc.  All rights reserved.
 *
 */


	/*	File Header		*/

struct filehdr {
	unsigned short	f_magic;	/* magic number */
								/* Target machine on which the
								   object file is executable */
	unsigned short	f_nscns;	/* number of sections */
	long			f_timdat;	/* time & date stamp */
	long			f_symptr;	/* file pointer to symtab */
	long			f_nsyms;	/* number of symtab entries */
	unsigned short	f_opthdr;	/* sizeof(optional hdr) */
	unsigned short	f_flags;	/* flags */
};

	/*	Magic words		*/

#define	XCOFF_MAGIC	0x01DF		/*	IBM RISC System/6000	*/

	/*	Bits for f_flags	*/

#define	F_RELFLG	0x0001		/*	relocation info stripped from file	*/
#define	F_EXEC		0x0002		/*	file is executable	*/
#define	F_DYNLOAD	0x1000		/*	file is dynamically loadable and executable	*/
#define	F_SHROBJ	0x2000		/*	file is a shared library	*/


	/*	Auxiliary Header	*/

struct aouthdr {
	short	magic;		/* flags - how to execute		*/
			/* 0x0107 text & data contiguous, both writable	*/
			/* 0x0108 text is R/O, data in next section	*/
			/* 0x010B text & data aligned and may be paged	*/
	short	vstamp;		/* version stamp			*/
	long	tsize;		/* text size in bytes, padded to FW bdry*/
	long	dsize;		/* initialized data " "			*/
	long	bsize;		/* uninitialized data " "		*/
	long	entry;		/* entry pt.				*/
	unsigned long	text_start;	/* base of text used for this file	*/
	unsigned long	data_start;	/* base of data used for this file	*/
	unsigned long	o_toc;		/* address of TOC			*/
	short	o_snentry;	/* section number for entry point	*/
	short	o_sntext;	/* section number for text		*/
	short	o_sndata;	/* section number for data		*/
	short	o_sntoc;	/* section number for toc		*/
	short	o_snloader;	/* section number for loader section	*/
	short	o_snbss;	/* section number for bss		*/
	short	o_algntext;	/* max alignment for text		*/
	short	o_algndata;	/* max alignment for data		*/
	char    o_modtype[2];	/* Module type field, 1R,RE,RO          */
	char    o_resv1[2];	/* reserved field			*/
	unsigned long   o_maxstack;	/* max stack size allowed (bytes).	*/
	unsigned long   o_maxdata;	/* max data size allowed (bytes).	*/
	unsigned long   o_resv2[3];	/* reserved fields			*/
};


	/*	Section Headers		*/

struct scnhdr {
	char			s_name[8];	/* section name */
	unsigned long	s_paddr;	/* physical address */
	unsigned long	s_vaddr;	/* virtual address */
	unsigned long	s_size;		/* section size */
	long			s_scnptr;	/* file ptr to raw data for section */
	long			s_relptr;	/* file ptr to relocation */
	long			s_lnnoptr;	/* file ptr to line numbers */
	unsigned short	s_nreloc;	/* number of relocation entries */
	unsigned short	s_nlnno;	/* number of line number entries */
	long			s_flags;	/* flags */
};

	/*	Section Flags	*/

#define	STYP_MASK	0x0000FFFF
#define	STYP_TEXT	0x0020		/* section contains text only */
#define STYP_DATA	0x0040		/* section contains data only */
#define STYP_BSS	0x0080		/* section contains bss only */
#define STYP_LOADER	0x1000		/* loader section	*/
#define STYP_DEBUG	0x2000		/* debug section	*/
#define STYP_TYPCHK	0x4000		/* type check section	*/
#define STYP_OVRFLO	0x8000		/* RLD and line number overflow sec hdr */


	/*	Relocations		*/

struct reloc {
	unsigned long	r_vaddr;	/* (virtual) address of reference */
	unsigned long	r_symndx;	/* index into symbol table */
	struct {
		unsigned char	r_sign	: 1,	/* signed/unsigned	*/
							  	: 2,
						r_len	: 5;	/* bit length - 1 of field to modify */
		char			r_rtype;		/* relocation type	*/
	} r_;
};

	/*	toc style relocation types	*/

#define	R_POS		0x00		/* A(sym) Positive Relocation	*/
#define R_NEG		0x01		/* -A(sym) Negative Relocation	*/
#define R_REL		0x02		/* A(sym-*) Relative to self	*/
#define	R_TOC		0x03		/* A(sym-TOC) Relative to TOC	*/
#define R_TRL		0x12		/* A(sym-TOC) TOC Relative indirect load. modifiable inst*/
#define R_TRLA		0x13		/* A(sym-TOC) TOC Rel load address. modifiable inst */
#define R_GL		0x05		/* A(external TOC of sym) Global Linkage */
#define R_TCL		0x06		/* A(local TOC of sym) Local object TOC address */
#define R_RL		0x0C		/* A(sym) Pos indirect load. modifiable instruction */
#define R_RLA		0x0D		/* A(sym) Pos Load Address. modifiable instruction */
#define R_REF		0x0F		/* AL0(sym) Non relocating ref. No garbage collect */
#define R_BA		0x08		/* A(sym) Branch Absolute. cannot modifiable instr */
#define R_RBA		0x18		/* A(sym) Branch absolute. modifiable instr */
#define	R_RBAC		0x19		/* A(sym) Branch absolute constant. modifiable instr */
#define	R_BR		0x0A		/* A(sym-*) Branch rel to self. non modifiable */
#define R_RBR		0x1A		/* A(sym-*) Branch rel to self. modifiable instr */
#define R_RBRC		0x1B		/* A(sym-*) Branch absolute const. modifiable to R_RBR */
#define R_RTB		0x04		/* A((sym-*)/2) RT IAR Rel Branch. non modifiable */
#define R_RRTBI		0x14		/* A((sym-*)/2) RT IAR Rel Br. modifiable to R_RRTBA */
#define R_RRTBA		0x15		/* A((sym-*)/2) RT absolute br. modifiable to R_RRTBI */


	/*	Symbol Table	*/

/*		Number of characters in a symbol name */
#define	SYMNMLEN	8
/*		Number of characters in a file name */
#define	FILNMLEN	14
/*		Number of array dimensions in auxiliary entry */
#define	DIMNUM		4

struct syment
{
	union
	{
		char		_n_name[SYMNMLEN];	/* old COFF version */
		struct
		{
			long	_n_zeroes;	/* new == 0 */
			long	_n_offset;	/* offset into string table */
		} _n_n;
	} _n;
	long			n_value;	/* value of symbol */
	short			n_scnum;	/* section number */
	unsigned short	n_type;		/* type and derived type */
	char			n_sclass;	/* storage class */
	char			n_numaux;	/* number of aux. entries */
};

#define n_name		_n._n_name
#define n_zeroes	_n._n_n._n_zeroes
#define n_offset	_n._n_n._n_offset

/*
 * Relocatable symbols have a section number of the
 * section in which they are defined. Otherwise, section
 * numbers have the following meanings:
 */
	/* undefined symbol */
#define	N_UNDEF	0
	/* value of symbol is absolute */
#define	N_ABS		-1
	/* special debugging symbol -- value of symbol is meaningless */
#define	N_DEBUG	-2

union auxent
{
	union
	{
		char	x_fname[FILNMLEN];
		struct
		{
			long		x_zeroes;
			long		x_offset;
			char		x_pad[FILNMLEN-8];
			unsigned char	x_ftype;
		} _x;
	} x_file;
	struct
	{
		long		x_scnlen; /* section length */
		unsigned short	x_nreloc; /* number of relocation entries */
		unsigned short	x_nlinno; /* number of line numbers */
	} x_scn;
	struct
	{
		long			x_scnlen;	/* csect length */
		long			x_parmhash;	/* parm type hash index */
		unsigned short	x_snhash;	/* sect num with parm hash */
		unsigned char	x_smalgn : 5,	/* Log 2 of alignment */
						x_smtype : 3;	/* symbol type */
		unsigned char	x_smclas;	/* storage mapping class */
		long			x_stab;		/* dbx stab info index */
		unsigned short	x_snstab;	/* sect num with dbx stab */
	} x_csect; /* csect definition information */
};

/*	Defines for File auxiliary  definitions	*/
#define	XFT_FN	0	/* Source File Name */
#define	XFT_CT	1	/* Compile Time Stamp */
#define	XFT_CV	2	/* Compiler Version Number */
#define	XFT_CD	128	/* Compiler Defined Information */

/*	Defines for CSECT definitions	*/
/*	Symbol Type	*/
#define	XTY_ER	0	/* External Reference */
#define	XTY_SD	1	/* CSECT Section Definition */
#define XTY_LD	2	/* Entry Point - Label Definition */
#define XTY_CM	3	/* Common (BSS) */
#define XTY_EM	4	/* Error Message - Linkedit usage */
#define XTY_US	5	/* Unset */

/*	Storage Mapping Class	*/
/*		READ ONLY CLASSES */
#define	XMC_PR	0	/* Program Code */
#define	XMC_RO	1	/* Read Only Constant */
#define XMC_DB	2	/* Debug Dictionary Table */
#define XMC_GL	6	/* Global Linkage (Interfile Interface Code) */
#define XMC_XO	7	/* Extended Operation (Pseudo Machine Instruction */
#define XMC_SV	8	/* Supervisor Call */
#define XMC_TI	12	/* Traceback Index csect */
#define XMC_TB	13	/* Traceback table csect */

/*		READ WRITE CLASSES */
#define XMC_RW	5	/* Read Write Data */
#define XMC_TC0 15	/* TOC Anchor for TOC Addressability */
#define XMC_TC	3	/* General TOC Entry */
#define XMC_DS	10	/* Descriptor csect */
#define XMC_UA	4	/* Unclassified - Treated as Read Write */
#define XMC_BS	9	/* BSS class (uninitialized static internal) */
#define XMC_UC	11	/* Un-named Fortran Common */

#define	SYMENT	struct syment
#define	SYMESZ	18	/* sizeof(SYMENT) */

#define	AUXENT	union auxent
#define	AUXESZ	18	/* sizeof(AUXENT) */

/*
 *	STORAGE CLASSES
 */

#define C_EFCN		-1	/* physical end of function */
#define C_NULL		0
#define C_AUTO		1	/* automatic variable */
#define C_EXT		2	/* external symbol */
#define C_STAT		3	/* static */
#define C_REG		4	/* register variable */
#define C_EXTDEF	5	/* external definition */
#define C_LABEL		6	/* label */
#define C_ULABEL	7	/* undefined label */
#define C_MOS		8	/* member of structure */
#define C_ARG		9	/* function argument */
#define C_STRTAG	10	/* structure tag */
#define C_MOU		11	/* member of union */
#define C_UNTAG		12	/* union tag */
#define C_TPDEF		13	/* type definition */
#define C_USTATIC	14	/* undefined static */
#define C_ENTAG		15	/* enumeration tag */
#define C_MOE		16	/* member of enumeration */
#define C_REGPARM	17	/* register parameter */
#define C_FIELD		18	/* bit field */
#define C_BLOCK		100	/* ".bb" or ".eb" */
#define C_FCN		101	/* ".bf" or ".ef" */
#define C_EOS		102	/* end of structure */
#define C_FILE		103	/* file name */

	/*
	 * The following storage class is a "dummy" used only by STS
	 * for line number entries reformatted as symbol table entries
	 */

#define C_LINE		104
#define C_ALIAS		105	/* duplicate tag */
#define C_HIDDEN	106	/* special storage class for external */
				/* symbols in dmert public libraries */
#define	C_HIDEXT	107	/* Un-named external symbol */
#define	C_BINCL		108	/* Marks beginning of include file */
#define	C_EINCL		109	/* Marks ending of include file */


/*********************************************************************
 *
 *	COFF Extended Object File Format:
 *		loader.h
 *
 *	Structures used to define the dynamic loader section of the
 *		object file.
 *
 *********************************************************************/

/* Header portion	*/
struct ldhdr
{
	long	l_version;	/* Loader section version number	*/
	long	l_nsyms;	/* Qty of loader Symbol table entries	*/
	long	l_nreloc;	/* Qty of loader relocation table entries */
	unsigned long	l_istlen;	/* Length of loader import file id strings */
	long	l_nimpid;	/* Qty of loader import file ids.	*/
	unsigned long	l_impoff;	/* Offset to start of loader import	*/
				/*	file id strings			*/
	unsigned long	l_stlen;	/* Length of loader string table	*/
	unsigned long	l_stoff;	/* Offset to start of loader string table */
};

/* Symbol table portion	*/
struct ldsym
{
	union
	{
		char		_l_name[SYMNMLEN];	/* Symbol name	*/
		struct
		{
			long	_l_zeroes;	/* offset if 0		*/
			long	_l_offset;	/* offset into loader string */
		} _l_l;
		char		*_l_nptr[2];	/* allows for overlaying */
	} _l;
	unsigned long		l_value;	/* Address field	*/
	short			l_scnum;	/* Section number	*/
	char			l_smtype;	/* type and imp/exp/eps	*/
						/* 0	Unused		*/
						/* 1	Import		*/
						/* 2	Entry point	*/
						/* 3	Export		*/
						/* 4	Unused		*/
						/* 5-7	Symbol type	*/
	char			l_smclas;	/* storage class	*/
	long			l_ifile;	/* import file id	*/
						/*	string offset	*/
	long			l_parm;		/* type check offset	*/
						/*	into loader string */
};

#define	l_name		_l._l_name
#define	l_nptr		_l._l_nptr[1]
#define	l_zeroes	_l._l_l._l_zeroes
#define	l_offset	_l._l_l._l_offset

#define	L_EXPORT	0x10
#define	L_ENTRY		0x20
#define	L_IMPORT	0x40
#define	LDR_EXPORT(x)	((x).l_smtype & L_EXPORT)
#define	LDR_ENTRY(x)	((x).l_smtype & L_ENTRY)
#define	LDR_IMPORT(x)	((x).l_smtype & L_IMPORT)
#define	LDR_TYPE(x)	((x).l_smtype & 0x07)



