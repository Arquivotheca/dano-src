
/*
 *  Obj.h	- object file definitions for Metrowerks C++ (PowerPC)
 *
 *  Copyright © 1993 metrowerks inc.  All rights reserved.
 *
 */


	/*	public data structures	*/


#define	LIB_MAGIC_WORD	'MWOB'
#define LIB_VERSION		1

typedef struct LibFile {
	unsigned long	moddate;			/*	modification date of object file	*/
	long			filename;			/*	offset to object file name (C string)	*/
	long			fullpathname;		/*	offset to full path name (C string)		*/
	long			objectstart;		/*	start of object file data	*/
	long			objectsize;			/*	size of object file data	*/
} LibFile;

typedef struct LibHeader {
	long			magicword;			/*	always set to LIB_MAGIC_WORD	*/
	long			magicproc;			/*	'PPC '	*/
	long			magicflags;			/*	(reserved for future use)	*/
	long			version;			/*	always set to LIB_VERSION	*/
	long			code_size;			/*	total size of code in library	*/
	long			data_size;			/*	total size of data in library	*/
	long			nobjectfiles;		/*	# of contained object files	*/
	LibFile			objectfiles[1];		/*	index to contained object files	*/
	// The above item modified so it would compile under the C++ compiler  BDS
} LibHeader;

#define	OBJ_MAGIC_WORD	'POWR'
#define	OBJ_VERSION		0

typedef struct ObjHeader {		//	object file header
	long	magic_word;			//	always set to OBJ_MAGIC_WORD
	short	version;			//	always set to OBJ_VERSION
	short	flags;				//	flags (see below)
	long	obj_size;			//	size of object data ( follows this header)
	long	nametable_offset;	//	offset to name table
	long	nametable_names;	//	number of names in table ( max id )
	long	symtable_offset;	//	offset to symbol file info table
	long	symtable_size;		//	size of symtable
	long	code_size;			//	size of code in object
	long	udata_size;			//	size of initialized data in object
	long	idata_size;			//	size of uninitialized data in object
	long	toc_size;			//	size of TOC data in object
	long	old_def_version;	//	old definition version# (for shared libraries)
	long	old_imp_version;	//	old implementation version# (for shared libraries)
	long	current_version;	//	current version# (for shared libraries)
	long	reserved[13];		//	reserved for future extensions
} ObjHeader;

	/*	flags	*/

enum {
	fObjIsSharedLib	= 0x0001,	//	object file is a shared library
	fObjIsLibrary	= 0x0002	//	object file is a library
};


#define isobjectlibrary(o)		(((LibHeader *) (o))->magicword == LIB_MAGIC_WORD)
#define isobjectfile(o)			(((ObjHeader *) (o))->magic_word == OBJ_MAGIC_WORD)


/*	READ ONLY CLASSES	*/
#define	XMC_PR	0	/* Program Code */
#define	XMC_RO	1	/* Read Only Constant */
#define XMC_DB	2	/* Debug Dictionary Table (NOT USED) */
#define XMC_GL	6	/* Global Linkage (Linker Generated) */
#define XMC_XO	7	/* Extended Operation (NOT USED) */
#define XMC_SV	8	/* Supervisor Call (NOT USED) */
#define XMC_TI	12	/* Traceback Index csect (NOT USED) */
#define XMC_TB	13	/* Traceback table csect (NOT USED) */

/*	READ WRITE CLASSES	*/
#define XMC_RW	5	/* Read Write Data */
#define XMC_TC0 15	/* TOC Anchor for TOC Addressability */
#define XMC_TC	3	/* General TOC Entry */
#define XMC_TD	16	/* Scalar TOC Data */
#define XMC_DS	10	/* Routine Descriptor */
#define XMC_UA	4	/* Unclassified (NOT USED) */
#define XMC_BS	9	/* BSS class (NOT USED) */
#define XMC_UC	11	/* Unnamed Fortran Common (NOT USED) */


/*	Object File Records  */

enum {
	HUNK_START=0x4567,
	HUNK_END,
	HUNK_SEGMENT,				//	segment for subsequent CODE hunks
	HUNK_LOCAL_CODE,
	HUNK_GLOBAL_CODE,
	HUNK_LOCAL_UDATA,
	HUNK_GLOBAL_UDATA,
	HUNK_LOCAL_IDATA,
	HUNK_GLOBAL_IDATA,
	HUNK_GLOBAL_ENTRY,
	HUNK_LOCAL_ENTRY,
	HUNK_IMPORT,				//	import symbol (for shared libraries)
	HUNK_XREF_16BIT,			//	16-bit TOC-relative reference
	HUNK_XREF_16BIT_IL,			//	16-bit TOC-relative indirect load
	HUNK_XREF_24BIT,			//	24-bit PC-relative reference
	HUNK_XREF_32BIT,			//	32-bit Absolute reference to symbol
	HUNK_INIT_CODE,				//	(reserved)
	HUNK_DEINIT_CODE,			//	(reserved)
	HUNK_LIBRARY_BREAK,			//	(obsolescent)
	HUNK_IMPORT_CONTAINER,		//	import container name (shared library)
	HUNK_SOURCE_BREAK			//	source file for subsequent source->object info
};
#define HUNKISXREF(h)	((h) >= HUNK_XREF_16BIT && (h) <= HUNK_XREF_32BIT)
#define HUNKISENTRY(h)	((h) >= HUNK_GLOBAL_ENTRY && (h) <= HUNK_LOCAL_ENTRY)

typedef struct ObjSegHunk {
	short	hunk_type;			//	HUNK_SEGMENT
	short	unused;				//	pad
	long	name_id;			//	name of segment
} ObjSegHunk;

typedef struct ObjCodeHunk {
	short	hunk_type;			//	HUNK_LOCAL_CODE, HUNK_GLOBAL_CODE
	char	sm_class;			//	XMC_PR or XMC_GL
	unsigned char
			multi_def : 1,		//	module may have multiple identical definitions
			over_load : 1,		//	module may be overloaded by another definition
			exported : 1,		//	module is to be exported
			reserved : 1,		//	(reserved for future use)
			alignment : 4;		//	always 4=word
	long	name_id;			//	name of code module
	long	size;				//	size of code module
	long	sym_offset;			//	offset to symbol data
	long	sym_decl_offset;	//	source declaration offset
} ObjCodeHunk;					//	followed by 'size' bytes of code

typedef struct ObjDataHunk {
	short	hunk_type;			//	HUNK_LOCAL_IDATA, HUNK_GLOBAL_IDATA, HUNK_LOCAL_UDATA, HUNK_GLOBAL_UDATA
	char	sm_class;			//	XMC_RW, XMC_DS, XMC_TC, XMC_TC0
	unsigned char
			multi_def : 1,		//	module may have multiple identical definitions
			over_load : 1,		//	module may be overloaded by another definition
			exported : 1,		//	module is to be exported
			reserved : 1,		//	(reserved for future use)
			alignment : 4;		//	1=byte, 2=halfword, 4=word, 8=doubleword
	long	name_id;			//	name of data module
	long	size;				//	size of data module
	long	sym_type_id;		//	local data type id
	long	sym_decl_offset;	//	source declaration offset
} ObjDataHunk;					//	followed by 'size' bytes of data if IDATA hunk

/* Sequence after CODE/DATA hunks:	[ENTRIES]opt -> [XREFS]opt	*/

typedef struct ObjEntryHunk {
	short	hunk_type;			//	HUNK_GLOBAL_ENTRY, HUNK_LOCAL_ENTRY
	short	unused;				//	pad
	long	name_id;			//	name of entry
	long	offset;				//	offset of entry in module
	long	sym_type_id;		//	local data type id
	long	sym_decl_offset;	//	source declaration offset
} ObjEntryHunk;

typedef struct ObjXRefHunk {
	short	hunk_type;			//	HUNK_XREF_16BIT, HUNK_XREF_16BIT_IL, HUNK_XREF_24BIT, HUNK_XREF_32BIT
	char	sm_class;			//	storage-mapping class of referenced symbol
	char	unused;				//	pad
	long	name_id;			//	name of referenced symbol
	long	offset;				//	offset to instruction/data being relocated
} ObjXRefHunk;

typedef struct ObjImportHunk {
	short	hunk_type;			//	HUNK_IMPORT
	char	sm_class;			//	XMC_RW or XMC_DS
	char	unused;				//	pad
	long	name_id;			//	name of import symbol
} ObjImportHunk;

typedef struct ObjContainerHunk {
	short	hunk_type;			//	HUNK_IMPORT_CONTAINER
	short	unused;				//	pad
	long	name_id;			//	runtime name of import container
	long	old_def_version;	//	old definition version#
	long	old_imp_version;	//	old implementation version#
	long	current_version;	//	current version#
} ObjContainerHunk;

typedef struct ObjSourceHunk {
	short	hunk_type;			//	HUNK_SOURCE_BREAK
	short	unused;				//	pad
	long	name_id;			//	name of source file (full path)
	long	moddate;			//	modification date
} ObjSourceHunk;

typedef struct ObjMiscHunk {
	short	hunk_type;			//	HUNK_START, HUNK_END, HUNK_LIBRARY_BREAK
	short	unused;				//	pad
} ObjMiscHunk;


#ifndef FOR_LINKER


	/*	private interface  */


extern void		ObjGen_Setup(void);
extern void		ObjGen_Finish(void);
extern void		ObjGen_Cleanup(void);
extern void		ObjGen_CodeSetup(bool initcode);
extern void		ObjGen_CodeCleanup(void);
extern void		ObjGen_SetupSym(void);
extern void		ObjGen_DeclareData(Object *,void *,OLink *);
extern void		ObjGen_SegmentName(HashNameNode *name);
extern void		ObjGen_SymFunc(Object *function);
extern void		ObjGen_SrcBreakName(HashNameNode *filepath, long moddate, bool addfile);

extern void		declareidata(Object *, void *, OLink *);
extern void		declareudata(Object *);
extern void		declaretoc(Object *);
extern void		declaretoc0(Object *);
extern void		declaredescriptor(Object *);
extern long		declarecode(Object *, long);
extern void		declarexref(short, long, Object *);
extern void		declarefloatconst(Object *);
extern void		declareswitchtable(Object *, Object *);
extern void		declarestatement(long sourceoffset, long codeoffset);
extern void		declaresyminfo(void);
extern void		declarecodelabel(Object *, long, Object *);


#endif	//	FOR_LINKER
