/* 
 *          pe.h
 *          Appropiate copyright message to be included later!
 *
 * This file contains structure definitions of Portable Executable (PE) or
 * Common Object File Format (COFF) files.  Refer to Microsoft(R) Portable
 * Executable and Common Object File Format Specification 4.1 for any
 * explanations.  Actual loader module would probably be using only few
 * of these structures.
 */

#ifndef    _PE_H
#define    _PE_H

/*
 * File Header Format.
 */

#define PE_SIGNATURE_OFFSET  0x3c
#define SIZE_OF_PE_SIGNATURE  4

typedef struct coff_hdr {
	unsigned short 	mchn_typ;  /*Number identifying target machine*/
	short	num_sectns;   /*Number of sections*/	
	ulong	tim_date;     /*Time and day stamp*/
	char	*symtab_ptr;  /*Pointer to symbol table */
	ulong	nsyms;	      /*Number of symbol table enteries*/
	ushort	opthdr_sz;    /*Size of optional header*/
	ushort	flags;        /*Flags indicating characteristics of file*/
} coff_header;

/*
 * In coff_header mchn_typ has one of the following values.....normally for
 * iA it should be 0x14c
 */
#define IMAGE_FILE_MACHINE_UNKNOWN    0x0
#define IMAGE_FILE_MACHINE_I386       0x14c

/*
 * In coff_header flags has one of the following attribute values.
 */

#define IMAGE_FILE_RELOCS_STRPD       0x0001
#define IMAGE_FILE_EXE_IMAGE          0x0002
#define IMAGE_FILE_LINENUM_STRPD      0x0004
#define IMAGE_FILE_LOCLSYM_STRPD      0x0008
#define IMAGE_FILE_MIN_OBJ            0x0010
#define IMAGE_FILE_UPDAT_OBJ          0x0020
#define IMAGE_FILE_16BIT_MCHN         0x0040
#define IMAGE_FILE_BYTES_RSRVD_LO     0x0080
#define IMAGE_FILE_32BIT_MCHN         0x0100
#define IMAGE_FILE_DBG_STRPD          0x0200
#define IMAGE_FILE_PATCH              0x0400
#define IMAGE_FILE_SYSTEM             0x1000
#define IMAGE_FILE_DLL                0x2000
#define IMAGE_FILE_BYTES_RSRVD_HI     0x8000

typedef struct standard_flds{
	ushort	magic;   /*Identifying the state of image file.*/
	char	lmajor;	 /*Major revision number of linker*/
	char	lminor;  /*Minor revision number of linker*/
	ulong	csize;   /*Code size or sum of all text sections*/
	ulong	dsize;   /*Intlzd data size or sum of all data sections*/
	ulong	bsize;   /*Unintlzd data size or sum of all BSS sections*/
	/*Following three pointers are relative to image base.*/
	char	*entry;  /*Address of entry point */
	char	*code_base;/*Address of begining of code section*/
	char	*data_base;/*Address of begining of data section*/
} standard_flds;
/*
 * Valid values for magic field specifying the state of executable.
 */

 #define EXEC_FILE     0x10b
 #define ROM_FILE      0x107

typedef struct nt_specific {
	ulong	prefer_base;
	int	sctn_algnmnt;
	int	file_algnmnt;
	ushort	os_major;
	ushort	os_minor;
	ushort	img_major;
	ushort	img_minor;
	ushort	subsys_major;
	ushort	subsys_minor;
	char	resrvd[4];
	ulong	img_size;
	ulong	hdr_size;
	ulong	file_chksum;
	ushort	subsys;
	ushort	dll;
	ulong	stk_rsrv_size;
	ulong	stk_commit_size;
	ulong	heap_rsrv_size;
	ulong	heap_commit_size;
	ulong	loader_flgs;
	int	num_data_sctns;
} nt_specific;

typedef struct data_directories {
	ulong	rva;
	ulong	size;
} data_directories;

typedef struct optional_hdr {
	standard_flds   std;
	nt_specific	nt;
	data_directories data[16];
}optnl_header;
/*
 * Following are the indices of various tables in above 'data' arary.  The
 * last five entries in this array are reserved for future use.
 */

#define EXPORT_TAB        0
#define IMPORT_TAB        1
#define RESRC_TAB         2
#define EXCPTN_TAB        3
#define SECURITY_TAB      4
#define BASE_RELOC_TAB    5
#define DEBUG_DATA        6
#define COPYRIGHT         7
#define GLOBAL_PTR        8
#define TLS_TAB           9
#define LOAD_CONFIG_TAB   10

typedef struct  section_hdr {
	char	name[8];      /*section name 8 bytes NULL terminated*/
	ulong	virtualsz;    /*Total size when loaded in memory*/
	char	*rva;	      /*Address of first byte of the section*/
	ulong	raw_datasz;   /*Size of section */
	ulong	raw_datap;   /*file pointer to raw data for section*/
	char	*relocp;      /*file pointer to relocation*/
	char	*line_nump;   /*file pointer to line numbers*/
	ushort	num_reloc;    /*Number of relocation enteries*/
	ushort	num_line_nums;/*Number of line number enteries*/
	ulong	flags;        /*Flags determining sectoin characteristics*/
} sectn_header;
/*
 * Values for flags field in section_hdr.
 */
#define  IMAGE_SCN_TYPE_REGULAR   	0x00000000
#define  IMAGE_SCN_TYPE_DUMMY     	0x00000001
#define  IMAGE_SCN_TYPE_NO_LOAD   	0x00000002
#define  IMAGE_SCN_TYPE_GROUPED   	0x00000004
#define  IMAGE_SCN_TYPE_NO_PAD		0x00000008
#define  IMAGE_SCN_TYPE_COPY		0x00000010
#define  IMAGE_SCN_CNT_CODE		0x00000020
#define  IMAGE_SCN_CNT_INTLZD_DATA	0x00000040
#define  IMAGE_SCN_CNT_UNINTLZD_DATA	0x00000080
#define  IMAGE_SCN_LNK_INFO		0x00000200
#define  IMAGE_SCN_LNK_OVERLAY		0x00000400
#define  IMAGE_SCN_LNK_REMOVE		0x00000800
#define  IMAGE_SCN_LNK_COMDAT		0x00001000
#define  IMAGE_SCN_MEM_DISCARADABLE	0x02000000
#define  IMAGE_SCN_MEM_NOT_CACHED	0x04000000
#define  IMAGE_SCN_MEM_NOT_PAGED	0x08000000
#define  IMAGE_SCN_MEM_SHARED		0x10000000
#define  IMAGE_SCN_MEM_EXECUTE		0x20000000
#define  IMAGE_SCN_MEM_READ		0x40000000
#define  IMAGE_SCN_MEM_WRITE		0x80000000

typedef struct coff_reloc {
	ulong 	offset;
	int	symtab_index;
	ushort	type;
} coff_reloc;
/*
 * Type field of relocation record indicates what kind of relocation 
 * should be performed.  For iA we have the following types available.
 */
#define IMAGE_REL_I386_ABSOLUTE		0
#define IMAGE_REL_I386_DIR32		6
#define IMAGE_REL_I386_DIR32NB		7
#define IMAGE_REL_I386_SECTION		0xA
#define IMAGE_REL_I386_SECREL		0xB
#define IMAGE_REL_I386_REL32		0x14

/*
 * COFF LINE NUMBERS indicate the relationship between code and line numbers
 * in source files.  It consists of array of fixed length records.  The
 * location and size of the array are specified in the section header.  The 
 * "type" field is union of two four byte fields, Symnol Table Index and RVA.
 */
typedef struct coff_linenum {
	union {
		int	symtbl_ndx;
		ulong	rva;
	} type;
	ushort	linenum;
} coff_linenum;

/*
 * COFF SYMBOL TABLE - Its location is indicated in the COFF header.  Its an
 * array of records, each 18 bytes long.  Each record is either a standard
 * or an auxiliary symbol_table record.  Standard record has the following
 * format.
 */
typedef struct coff_symtbl {
	union {
		char short_nm[8];/*padded with nulls if name is shorter than 8*/
		struct {
			char zeroes[4];/*Setto all 0s if name is longer than 8*/
			ulong offset;  /*offset into the string table*/
		} long_nm;
	} name;
	ulong	value;          /*Value (address) of the symbol */
	short	scnnum;         /*Signed integer identifying the section*/
	ushort	type;           /*Type of section*/
	char	storage_class;  /*Storage class*/
	char	numaux_syms;    /*Num of auxialiary enteries that follow*/
}coff_symtbl;

/*
 * Section number Values "scnnum" in coff_symtbl structure is a signed 
 * integer and can also take following special negative values.
 */
#define   IMAGE_SYM_UNDEFINED   0
#define   IMAGE_SYM_ABSOLUTE    -1
#define   IMAGE_SYM_DEBUG       -2

/* 
 * Type Representation-- Type field of symbol table entry contains two bytes,
 * each byte representing type information.  LSB(yte) represents simple (base)
 * data types, and MSB(yte) represents complex type(if any)
 * ********* *********IMPORTANT********************
 * If we are going to support long long type then it should be defined here.
 */
#define IMAGESYM_TYPE_LSB_NULL         0
#define IMAGESYM_TYPE_LSB_VOID         1
#define IMAGESYM_TYPE_LSB_CHAR         2
#define IMAGESYM_TYPE_LSB_SHORT        3
#define IMAGESYM_TYPE_LSB_INT          4
#define IMAGESYM_TYPE_LSB_LONG         5
#define IMAGESYM_TYPE_LSB_FLOAT        6
#define IMAGESYM_TYPE_LSB_DOUBLE       7
#define IMAGESYM_TYPE_LSB_STRUCT       8
#define IMAGESYM_TYPE_LSB_UNION        9
#define IMAGESYM_TYPE_LSB_ENUM        10
#define IMAGESYM_TYPE_LSB_MOE         11
#define IMAGESYM_TYPE_LSB_BYTE        12
#define IMAGESYM_TYPE_LSB_WORD        13
#define IMAGESYM_TYPE_LSB_UINT        14
#define IMAGESYM_TYPE_LSB_DWORD       15


#define IMAGESYM_TYPE_MSB_NULL         0
#define IMAGESYM_TYPE_MSB_POINTER      1
#define IMAGESYM_TYPE_MSB_FUNCTION     2
#define IMAGESYM_TYPE_MSB_ARRAY        3

/*
 * STORAGE CLASS-- Unsigned one byte integer indicating what kind of 
 * definition  a symbol represents.  The special value -1 shoud therefore be
 * taken to mean its unsigned equivalent, 0xFF.
 */

#define  IMAGESYM_CLASS_EOF            -1
#define  IMAGESYM_CLASS_NULL            0
#define  IMAGESYM_CLASS_AUTOMATIC       1
#define  IMAGESYM_CLASS_EXTERNAL        2
#define  IMAGESYM_CLASS_STATIC          3
#define  IMAGESYM_CLASS_REGISTER        4
#define  IMAGESYM_CLASS_EXTERNAL_DEF    5
#define  IMAGESYM_CLASS_LABEL           6
#define  IMAGESYM_CLASS_UNDEFINED_LABEL 7
#define  IMAGESYM_CLASS_MEMBR_STRUCT    8
#define  IMAGESYM_CLASS_ARGUMENT        9
#define  IMAGESYM_CLASS_STRUCT_TAG     10
#define  IMAGESYM_CLASS_MEMBR_UNION    11
#define  IMAGESYM_CLASS_UNION_TAG      12
#define  IMAGESYM_CLASS_TYPE_DEFINTN   13
#define  IMAGESYM_CLASS_UNDEFND_STATIC 14
#define  IMAGESYM_CLASS_ENUM_TAG       15
#define  IMAGESYM_CLASS_MEMBR_ENUM     16
#define  IMAGESYM_CLASS_REGSTR_PARAM   17
#define  IMAGESYM_CLASS_BIT_FIELD      18
#define  IMAGESYM_CLASS_BLOCK         100
#define  IMAGESYM_CLASS_FUNCTION      101
#define  IMAGESYM_CLASS_END_OF_STRUCT 102
#define  IMAGESYM_CLASS_FILE          103
#define  IMAGESYM_CLASS_SECTION       104
#define  IMAGESYM_CLASS_WEAK_EXTRN    105

/*
 * Auxiliary Symbol Records
 */

typedef union aux_symtab {
	struct {
		int      tag_ndx;
		int      size;
		ulong    offst_linno;
		ulong    ndx_nxtfn;
		char	 unused[2];
	} fn_def;
	struct {
		char	 unused1[4];
		ushort	 linno;
		char	 unused2[4];
		ulong	 ndx_nxtfn;
		char	 unused3[2];
	} bf_ef;
	struct {
		ulong	 tag_ndx;
		int	 srch_libs;
		char	 unused[10];
	} weak_ext;
	struct {
		char	file_name[18];
	} files;
}  aux_symtbl ; /* of aux_symtab */

/*
 * Exprot data section.  We only maintain a pointer to export directory
 * table, which contains information for rest of section.
 */

 typedef struct export_directory_table {
 	unsigned long	flags;
	unsigned long	time_stamp;
	unsigned short	mjr_ver;
	unsigned short	mnr_ver;
	unsigned long	name;
	int		base;
	unsigned long	num_entries;
	unsigned long	num_names;
/* At present assuming that adrs_tbl only has EXPORT RVAs, if compiler
 * generates FORWARDER RVAs then peloader.c needs to be changed.
 */
	char 		*eat;
	char		*npt;
	/*Each ordinal is 16 bit index, but this is just an rva*/
	char		*ot;
 }  export_drctry_tbl;

/* Each  entry in export address table is a union of two formats.  If the 
 * address specified is not with in the range of export section then this
 * entry corresponds to export RVA.
 */
typedef struct export_adrs_tbl {
	char	*rva;
} export_adrs_tbl;

 typedef struct	import_directory_table {
	unsigned long	*ilt;
	unsigned long	time_stamp;
	unsigned long	frwd_chn;
	char		*name;
	char		*iat;
 } import_drctry_tbl;

/* Following is definition of each entry of Hint/Name Table. exec_info has a 
 * pointer to Hint/Name table.  At present though I think that this structure
 * will not be needed since import Lookup table will directly contain the RVA
 * of Hint/Name Table entry it is refering.
 */
typedef struct hint_name_table {
	unsigned short 	hint;
	char		*name;	
} hint_nm_tbl;

/* Type of relocations.
 */
#define IMAGE_REL_BASED_ABSOLUTE   0
#define IMAGE_REL_BASED_HIGHLOW	   3

#endif
