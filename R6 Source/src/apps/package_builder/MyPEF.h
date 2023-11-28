/*
	File:       PEF.h

	Contains:   PowerPC Executable Format
				Apple Public

	Copyright: (c) 1992-1994 by Apple Computer, Inc., all rights reserved.
*/




#ifndef __PEF__
#define __PEF__


/*
	Definitions for PEF executable file format used in PowerPC systems.
	
	Everything is aligned for PowerPC and to force that alignment
	even on the Mac.
*/


/* #include <CodeFragmentsPriv.h> */

/* since we don't have CodeFragmentsPriv.h we do the following typedefs */
typedef short			TSignedl6;
typedef unsigned short	TUnsignedl6;
typedef unsigned long	TUWord32;
typedef unsigned long	TVersion;
typedef short			TSectNum;
typedef long			lAddress;
typedef long			TOffset;
typedef long			TLength;
typedef unsigned char	TPLSectionKind;
typedef unsigned char	TCFLShareKind;
typedef unsigned char	TUnsigned8;
typedef long			TIndex;
typedef long			TCount;
typedef unsigned		TUBitF;		//????


/* #include <CFLoader.h> */

typedef TSignedl6	TSectNum;
typedef TOffset		TLdrStrOff;
typedef TUBitF		TLdrStrOff_BitFld;
#define				TLdrStrOff_BitCnt 24



/*============= File Header =============*/

typedef struct {
TUnsignedl6 magicl;		/* magic flag describing execution machine and
						environment */
TUnsignedl6 magic2;		/* magic flag describing execution machine and
						environment */

TUWord32	fileTypeID;			/* OSType identifier = 'peff' */
TUWord32	architectureID;		/* OSType identifier = 'pwpc' */
TVersion	versionNumber;		/* version number of this file format */
TUWord32	dateTimeStamp;		/* Macintosh date/time stamp */
TVersion	oldDefVersion;		/* old definition version number */
TVersion	oldImpVersion;		/* old implementation version number */
TVersion	currentVersion;			/* current version number */
TSectNum	numberSections;			/* number of sections */
TSectNum	loadableSections;		/* number of sections that are loadable
						            for execution, also the section # of
						            first non-loadable section */
lAddress	memoryAddress;			/* the location this container was last
						            loaded */
} FileHeader, *FileHeaderPtr;

#define kPEFVersion 1				/* current version number */
#define peffMagicl		0x4A6F		/* value of magicl for PEFF */
#define peffMagic2		0x7921		/* value of magic2 for PEFF */
#define peffTypeID		0x70656666	/* value of fileTypeID for 'peff'*/
#define powerPCID		0x70777063	/* value of architecture ID 'pwpc' */

/*========== Section Header ==========*/

typedef TUnsigned8 TPLSectionKind;

typedef struct {
	TOffset		sectionName;		/* offset into global string table
										for section name */
	TUWord32	sectionAddress;		/*preferred base address for the
										section*/
	TLength		execSize;			/* section size in bytes during execution
									in memory including zero initialization */
	TLength		initSize;			/* section size in bytes during execution
									in memory before zero initialization */
	TLength		rawSize;			/* section size in bytes in container
									before loading */
	TOffset		containerOffset;	/* container offest to section's raw data
                              		*/
	TPLSectionKind	regionKind;			/* section/region classification */
	TCFLShareKind	shareKind;			/* sharing classification */
	TUnsigned8		alignment;			/* execution alignment requirement
    	                          		(0=byte,1=half,2=word,3=doubleword,
										4=quadword..) */
	TUnsigned8		reserved;
} SectionHeader, *SectionHeaderPtr;

/* TCFLSectionKind */
#define kCodeSection 0
#define kDataSection 1
#define kPIDataSection 2
#define kConstantSection 3
#define kLoaderSection 4
#define kDebugSection 5
#define kExecDataSection 6
#define kExceptionSection 7
#define kTracebackSection 8

/* TCFLShareKind */
#define kNeverShare 0
#define kContextShare 1
#define kTeamShare 2
#define kTaskShare 3
#define kGlobalShare 4

/* Defines for PIDataSections */
#define kZero 0
#define kBlock 1
#define kRepeat 2
#define kRepeatBlock 3
#define kRepeatZero 4
#define kNoOpcode Ox0fff
#define kOpcodeShift 5
#define kFirstOperandMask 31

/*========== Loader Header ==========*/

typedef struct {
	TIndex		entryPointSection;		/* section number containing
										entry point descriptor */
	TOffset		entryPointOffset;		/* offset to entry point
										descriptor within section */
	TIndex		initPointSection;		/* section number containing entry
										point descriptor */
	TOffset		initPointOffset;		/* offset to entry point descriptor
										within section */
	TIndex		termPointSection;		/* section number containing entry
										point descriptor */
	TOffset		termPointOffset;		/* offset to entry point descriptor
										within section */
	TCount		numImportFiles;		/*number of import file id entries */
	TCount		numImportSyms;		/* number of import symbol table
									entries */
	TCount		numSections;		/* number of sections with load-time
									relocations */
	TOffset			relocationsOffset;		/* offset to relocation
											descriptions table */
	TOffset			stringsOffset;		/* offset to loader string table */
	TOffset			hashSlotTable;		/* offset to hash slot table */
	TCount			hashSlotTabSize;	/* number of hash slot entries */
	TCount			numExportSyms;		/* number of export symbol table
										entries */
} LoaderHeader, *LoaderHeaderPtr;



/*========== Loader Section Header ==========*/

typedef struct {
	TSectNum	sectionNumber;		/* reference to primary section number */
	TSectNum	dummyl6;			/* if TSectNum were 16 bits, which it
									isn't */
	TCount		numRelocations;		/* number of loader relocations for this
									section */
	TOffset		relocationsOffset; /* offset to relocation descriptions for
									this section */
} LoaderRelExpHeader, *LoaderRelExpHeaderPtr;

/*========== Loader Import File ID's Entry ==========*/

typedef struct {
	TLdrStrOff	fileNameOffset;		/* offset into loader string table for
									file name */
	TVersion	oldImpVersion;		/* oldest compatible implementation
									library */
	TVersion	linkedVersion;		/* current version at link time */
	TCount		numImports;			/* number of imports from this file */
	TIndex		impFirst;			/* number of the first imports from this
									file (relative to all imports) */
	TUnsigned8	initBefore;			/* call this libraries initialization
									routine before mine */
	TUnsigned8		reservedB;
	TUnsignedl6		reservedH;
} LoaderImportFileID, *LoaderImportFileIDPtr;

#if 0

/*========== Loader Import Symbol Table Entry ==========*/

typedef struct {
    TCFLSymbolClass		symClass;		/* import's symbol class */
	TLdrStrOff_BitFld	nameOffset : TLdrStrOff_BitCnt;	/* offset to name in
														loader string table */
} LoaderImport, *LoaderImportPtr;

/*========== Loader Export Hash Slot Table Entry ==========*/

typedef struct {
	unsigned chainCount : 14;
	unsigned chainIndex : 18;
} HashSlotEntry, *HashSlotEntryPtr;

/*========== Loader Export Hash Chain Table Entry ==========*/

typedef struct {
	TUWord32	hashword;	/* (hashword >> 16) == nameLength !! */
} HashChainEntry, *HashChainEntryPtr;

/*========== Loader Export Symbol Table Entry =========*/

/*	Section number controls how 'address' is interpreted.
	>=0: section number exporting the symbol; 'address' is offset from
	start of the section to the symbol being exported (ie address of a
	routine or data item)
	-1: value is absolute (non-relocatable)
	-2: value is a physical address (non-relocatable)
	-3: re-export imported symbol whose number is in 'address'
*/

/* this struct is stored in the file, non-aligned: size = 10 */
#if defined(powerc) || defined (__powerc)
	#pragma options align=mac68k
#endif

typedef struct {
	TCFLSymbolClass		symClass; /* export's symbol class */
	TLdrStrOff_BitFld	nameOffset : TLdrStrOff_BitCnt; /* offset name in
													loader string table */
	TOffset				address; /* offset into section to exported symbol */

    TSectNum			sectionNumber;
} LoaderExport, *LoaderExportPtr;

#if defined(powerc) || defined (__powerc)
      #pragma options align=reset
#endif
#define SIZEOF_LoaderExport (sizeof (TUnsigned32)*2 + sizeof (TSectNum))

#define kAbsoluteExport -1
#define kPhysicalExport -2
#define kReExportImport -3

/*========== Loader Relocation Entry ==========*/

typedef TUnsignedl6 RelocInstr;

typedef union {
	struct { unsigned op:7, rest:9;					} opcode;
	struct { unsigned op:2, delta_d4:8, cnt:6;		} deltadata;
	struct { unsigned op:7, cnt ml:9;				} run;
	struct { unsigned op:7, idx:9;					} glp;
	struct { unsigned op:4, delta ml:12;			} delta;
	struct { unsigned op:4, icnt ml:4, rcnt ml:8;	} rpt;
	struct { unsigned op:6, idx_top:l0;				} largel;
	struct { unsigned op:6, cnt ml:4, idx_top:6;	} large2;
	RelocInstr instr;
	TUnsignedl6 bot;
} Relocation;

// opcode definitions which can be used with
// Relocation.opcode.op:7, if masked properly
// by the up coming table
// (NOTE: a half word of 0 is garunteed to be an unused relocation
// instruction)

#define krDDAT	0x00	// type deltadata

#define krCODE	0x20	// type run
#define krDATA	0x21	// type run
#define krDESC	0x22	// type run
#define krDSC2	0x23	// type run
#define krVTBL	0x24	// type run
#define krSYMR	0x25	// type run
//				0x26
//				Ox2F

#define krSYMB	0x30 // type glp
#define krCDIS	0x31 // type glp
#define krDTIS	0x32 // type glp
#define krSECN	0x33 // type glp
//				0x34
//				0x3F

#define krDELT	0x40 // type delta
#define krRPT	0x48 // type rpt

#define krLABS 0x50 // type largel
#define krLSYM 0x52 // type largel
//             0x54
//             0x56

#define krLRPT 0x58 // type large2
#define krLSEC 0x5A // type large2
//             0x5C
//             0x5E

		// LSEC	usage:
		// LSEC	0, n		-- Long SECN
		// LSEC	1, n		-- Long CDIS
		// LSEC	2, n		-- Long DTIS
		// LSEC  3, n		-- free
			// LSEC 15, n		-- free

// constants that indicate the maximum sizes of fields
// (before packing, ie: subtracting one, in some cases)

#define ksDELTA		4096	// delta max for DELTA from

#define ksDDDMAX	1023	// delta max for DELTA-DAT (DDAT) form
#define ksDDRMAX	63		// run max for DELTA-DAT (DDAT) form
#define ksCODE		512		// count max for CODE form
#define ksDATA		512		// count max for DATA form
#define ksDEMAX		512		// count max for DESC form
#define ksVTMAX		512		// count max for VTBL form
#define ksISMAX		512		// count max for IMPS form
#define ksRPTMAX	256		// count max for RPT form

#define IsLARG(op)	(((op) & 0x70) == 0x50)

#define RELOPSHFT	9

#define ksDVDMAX	0         // (63) delta max for DELTA-VTBL (DVBL) form
#define ksDVRMAX	0         // (256) run max for DELTA-VTBL (DVBL) form

#define krXXXX		0xff

#endif		/* reloc stuff */
#endif		/* __PEF__ */
