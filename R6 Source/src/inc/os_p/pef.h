/* ++++++++++
	pef.h
	Copyright (C) 1995 Be Labs, Inc.  All Rights Reserved
+++++ */

#ifndef	_PEF_H
#define	_PEF_H

#define	PEF_MAGIC1	0x4a6f
#define	PEF_MAGIC2	0x7921
#define	PEF_CONTID	0x70656666		/* 'peff' */
#define	PEF_ARCHID	0x70777063

#define	PEF_CODE		0
#define	PEF_DATA		1
#define	PEF_PIDATA		2
#define	PEF_CONSTANT	3
#define	PEF_LOADER		4
#define	PEF_DEBUG		5
#define	PEF_EXECDATA	6
#define	PEF_EXCEPTION	7
#define	PEF_TRACEBACK	8

#define	PEF_ZERO		0
#define	PEF_BLOCK		1
#define	PEF_REPEAT		2
#define	PEF_REPEATBLOCK	3
#define	PEF_REPEATZERO	4

#define	PEF_INST_TO_OPCODE(c)	((unsigned char)(c) >> 5)
#define	PEF_INST_TO_COUNT(c)	((c) & ((1<<5)-1))

#define	PEF_op_deltadata(a)		(((a) >> 14) & ((1 << 2) - 1))
#define	PEF_op_run(a)			(((a) >> 9) & ((1 << 7) - 1))
#define	PEF_op_delta(a)			(((a) >> 12) & ((1 << 4) - 1))
#define	PEF_op_rpt(a)			PEF_op_delta(a)
#define	PEF_op_large1(a)		(((a) >> 10) & ((1 << 6) - 1))
#define	PEF_op_large2(a)		PEF_op_large1(a)
#define	PEF_op_opcode(a)		PEF_op_run(a)
#define	PEF_op_glp(a)			PEF_op_opcode(a)
#define	PEF_delta_d4(a)			(((a) >> 6) & ((1 << 8) - 1))
#define	PEF_cnt(a)				(((a) >> 0) & ((1 << 6) - 1))
#define	PEF_cnt_m1(a)			(((a) >> 0) & ((1 << 9) - 1))
#define	PEF_delta_m1(a)			(((a) >> 0) & ((1 << 12) - 1))
#define	PEF_idx_top(a)			(((a) >> 0) & ((1 << 10) - 1))
#define	PEF_rest(a)				(((a) >> 0) & ((1 << 9) - 1))
#define	PEF_idx9(a)				PEF_rest(a)

typedef	unsigned short	pef_rel;

typedef struct pef_cheader {
	short	magic1;
	short	magic2;
	int		cont_id;
	int		arch_id;
	int		version;
	int		mac_time_stamp;
	int		od_version;
	int		oi_version;
	int		c_version;
	short	snum;
	short	slnum;
	char	*addr;
} pef_cheader;

typedef struct pef_sheader {
	int		name;
	char	*addr;
	int		exec_size;
	int		init_size;
	int		raw_size;
	int		offset;
	char	kind;
	char	alignment;
	char	sharing;
	char	reserved;
} pef_sheader;

typedef struct pef_lheader {
	int		entry_section;
	int		entry_offset;
	int		init_section;
	int		init_offset;
	int		term_section;
	int		term_offset;
	int		cont_id_num;
	int		imp_sym_num;
	int		rel_section_num;
	int		rel_offset;
	int		str_offset;
	int		hash_offset;
	int		hash_size;
	int		exp_sym_num;
} pef_lheader;

typedef struct pef_rheader {
	short	section;
	short	reserved;
	int		num;
	int		offset;
} pef_rheader;

typedef unsigned long	pef_chain;
#define	PEF_chain_namelen(c)	(((c) >> 16) & 0xffff)

typedef	unsigned long	pef_isym;
#define	PEF_isym_flags(s)	(((s) >> 28) & 0xf)
#define	PEF_isym_class(s)	(((s) >> 24) & 0xf)
#define	PEF_isym_name(s)	((s) & 0x00ffffff)

#define	PEF_FLAGS_WEAK		0x8

#define	PEF_CLASS_CODE		0x0
#define	PEF_CLASS_DATA		0x1
#define	PEF_CLASS_TVECT		0x2
#define	PEF_CLASS_TOC		0x3
#define	PEF_CLASS_GLUE		0x4

typedef struct pef_esym {
	long	class_and_name;
	long	offset;
	short	section;
} pef_esym;

#define	PEF_esym_class(s)		(((s).class_and_name >> 24) & 0xf)
#define	PEF_esym_name(s)		((s).class_and_name & 0x00ffffff)
#define	PEF_esym_offset(s)		((s).offset)
#define	PEF_esym_section(s)		((s).section)
#define	PEF_inc_esym(sp)		((sp) = (pef_esym *)(((char *)(sp)) + 10))
#define	PEF_index_esym(sp,i)	((pef_esym *)((char *)(sp) + (i) * 10))

typedef struct pef_contid {
	long	name;
	long	old_version;
	long	cur_version;
	long	import_num;
	long	first_import;
	long	flag;
} pef_contid;

typedef unsigned long	pef_hash;
#define	PEF_hash_count(h)		(((h) >> 18) & ((1 << 14) - 1))
#define	PEF_hash_offset(h)		(((h) >> 0) & ((1 << 18) - 1))

#endif
