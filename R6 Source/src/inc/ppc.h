/* ++++++++++
	ppc.h
	Copyright (C) 1994 Be Inc.  All Rights Reserved.
	Definitions for the PowerPC processor.
+++++ */

#ifndef _PPC_H
#define _PPC_H

#define FIX_603e	cror	0, 0, 0

/* msr register fields */

#define msr_le	0x00000001	/* little endian mode */
#define msr_ri	0x00000002	/* recoverable interrupt */
#define msr_dr	0x00000010	/* data relocate enable */
#define msr_ir	0x00000020	/* instruction relocate enable */
#define msr_ip	0x00000040	/* interrupt prefix */
#define msr_fe1	0x00000100	/* floating point exception mode, bit 1 */
#define msr_be	0x00000200	/* branch trace interrupt enable */
#define msr_se	0x00000400	/* single-step trace interrupt enable */
#define msr_fe0	0x00000800	/* floating point exception mode, bit 0 */
#define msr_me	0x00001000	/* machine check interrupt enable */
#define msr_fp	0x00002000	/* floating point available */
#define msr_pr	0x00004000	/* 'problem' state (more like 'privilege') */
#define msr_ee	0x00008000	/* external interrupt enable */
#define msr_ile	0x00010000	/* interrupt little-endian mode */
#define msr_pow	0x00040000	/* power management enable */

/* special purpose register numbers */

#define spr_xer		1		/* fixed point exception reg */
#define spr_lr		8		/* link reg */
#define spr_ctr		9		/* count reg */
#define spr_dsisr	18		/* data storage int status reg */
#define spr_dar		19		/* data address reg */
#define spr_dec		22		/* decrementer reg */
#define spr_sdr1	25		/* storage description reg 1 */
#define spr_srr0	26		/* machine status save/restore reg 0 */
#define spr_srr1	27		/* machine status save/restore reg 1 */
#define spr_sprg0	272		/* OS software use reg 0 */
#define spr_sprg1	273		/* OS software use reg 1 */
#define spr_sprg2	274		/* OS software use reg 2 */
#define spr_sprg3	275		/* OS software use reg 3 */
#define spr_asr		280		/* address space reg */
#define spr_ear		282		/* external access reg */
#define spr_tbl		284		/* time base lower reg */
#define spr_tbu		285		/* time base upper reg */
#define spr_pvr		287		/* processor version reg */
#define spr_ibat0u	528		/* instr block addr xlation upper reg 0 */
#define spr_ibat0l	529		/* instr block addr xlation lower reg 0 */
#define spr_ibat1u	530		/* instr block addr xlation upper reg 1 */
#define spr_ibat1l	531		/* instr block addr xlation lower reg 1 */
#define spr_ibat2u	532		/* instr block addr xlation upper reg 2 */
#define spr_ibat2l	533		/* instr block addr xlation lower reg 2 */
#define spr_ibat3u	534		/* instr block addr xlation upper reg 3 */
#define spr_ibat3l	535		/* instr block addr xlation lower reg 3 */
#define spr_dbat0u	536		/* data block addr xlation upper reg 0 */
#define spr_dbat0l	537		/* data block addr xlation lower reg 0 */
#define spr_dbat1u	538		/* data block addr xlation upper reg 1 */
#define spr_dbat1l	539		/* data block addr xlation lower reg 1 */
#define spr_dbat2u	540		/* data block addr xlation upper reg 2 */
#define spr_dbat2l	541		/* data block addr xlation lower reg 2 */
#define spr_dbat3u	542		/* data block addr xlation upper reg 3 */
#define spr_dbat3l	543		/* data block addr xlation lower reg 3 */

/* PowerPC 603-specific special purpose registers */

#define spr_dmiss	976		/* data tlb miss address */
#define spr_dcmp	977		/* data tlb miss compare */
#define spr_hash1	978		/* pteg1 address */
#define spr_hash2	979		/* pteg2 address */
#define spr_imiss	980		/* instr tlb miss address */
#define spr_icmp	981		/* instruction tlb compare */
#define spr_rpa		982		/* tlb replacement entry */
#define spr_hid0	1008		/* hw implementation reg 0 */
#define spr_iabr	1010		/* instruction address breakpoint */

/* 603 HID0 register fields */

#define hid0_noopti	0x00000001	/* NOOP touch instructions */
#define hid0_fbiob	0x00000010	/* force branch indirect on bus */
#define hid0_dci	0x00000400	/* data cache flash invalidate */
#define hid0_icfi	0x00000800	/* instruction cache flash invalidate */
#define hid0_dlock	0x00001000	/* data cache lock */
#define hid0_ilock	0x00002000	/* instruction cache lock */
#define hid0_dce	0x00004000	/* data cache enable */
#define hid0_ice	0x00008000	/* instruction cache enable */
#define hid0_nhr	0x00010000	/* not hard reset */
#define hid0_riseg	0x00080000	/* read I seg (test only) */
#define hid0_dpm	0x00100000	/* dynamic power management enable */
#define hid0_sleep	0x00200000	/* no external clock */
#define hid0_nap	0x00400000	/* PLL, time base active */
#define hid0_doze	0x00800000	/* PLL, time base, snooping active */
#define hid0_par	0x01000000	/* disable precharge of ARTRY & shared signals */
#define hid0_eclk	0x02000000	/* enable external clock test pin */
#define hid0_eice	0x04000000	/* enable ICE outputs */
#define hid0_sbclk	0x08000000	/* select bus clock for test clock pin */
#define hid0_ebd	0x10000000	/* data bus parity checking enable */
#define hid0_eba	0x20000000	/* address bus parity checking enable */
#define hid0_emcp	0x80000000	/* machine check pin enable */

/* 604 hid0 fields */

#define hid0_604_emcp	0x80000000	/* machine check pin enable */
#define hid0_604_ecp	0x40000000	/* chache parity checking enable */
#define hid0_604_eba	0x20000000	/* address bus parity checking enable */
#define hid0_604_ebd	0x10000000	/* data bus parity checking enable */
#define hid0_604_par	0x01000000	/* disable precharge of ARTRY & shared signals */
#define hid0_604_nrst	0x00010000	/* for s/w: not hard reset */
#define hid0_604_ice	0x00008000	/* instruction cache enable */
#define hid0_604_dce	0x00004000	/* data cache enable */
#define hid0_604_ilock	0x00002000	/* instruction cache lock */
#define hid0_604_dlock	0x00001000	/* data cache lock */
#define hid0_604_icfi	0x00000800	/* instruction cache flash invalidate */
#define hid0_604_dci	0x00000400	/* data cache flash invalidate */
#define hid0_604_noser	0x00000080	/* disable instruction serialization */
#define hid0_604_bht	0x00000004	/* enable branch history table */





#define PAGESIZE	0x1000		/* transaltion page size */
#define CACHE_LINE_SIZE	32		/* cache line size (603) */

#endif
