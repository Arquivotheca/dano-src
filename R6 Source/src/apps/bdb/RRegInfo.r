/*	$Id: RRegInfo.r,v 1.3 1999/02/11 15:51:51 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
*/

#include "RTypes.r"
#include "DMessages.h"

Type 'RegI' {
	longint = $$CountOf(regs);
	array regs {
		cstring[12];	/* name */
		longint;			/* offset in cpu_state structure */
		longint;			/* size in bytes */
		longint;			/* kInteger when integer, kFloat when FP */
	};
};

#define kInteger 0
#define kFloat 1

/* Regl 0-2 are for x86 */
/* Regl 0  is integer-only, RegI 1 is old-style, Regl 2 is new-style */

Resource 'RegI' (0)
{
	{
		"eax", 152+408, 4, kInteger,
		"ebx", 140+408, 4, kInteger,
		"ecx", 148+408, 4, kInteger,
		"edx", 144+408, 4, kInteger,
		"ebp", 132+408, 4, kInteger,
		"edi", 124+408, 4, kInteger,
		"esi", 128+408, 4, kInteger,
		"eip", 164+408, 4, kInteger,
		"uesp", 176+408, 4, kInteger,
		"eflags", 172+408, 4, kInteger,
		"error_code", 160+408, 4, kInteger,
		"cs", 168+408, 2, kInteger,
		"ds", 120+408, 2, kInteger,
		"es", 116+408, 2, kInteger,
		"fs", 112+408, 2, kInteger,
		"gs", 108+408, 2, kInteger,
		"ss", 180+408, 2, kInteger,
		"esp_res", 136+408, 4, kInteger,
		"trap_no", 156+408, 4, kInteger,
	}
};

Resource 'RegI' (1)
{
	{
		"eax", 152+408, 4, kInteger,
		"ebx", 140+408, 4, kInteger,
		"ecx", 148+408, 4, kInteger,
		"edx", 144+408, 4, kInteger,
		"ebp", 132+408, 4, kInteger,
		"edi", 124+408, 4, kInteger,
		"esi", 128+408, 4, kInteger,
		"eip", 164+408, 4, kInteger,
		"uesp", 176+408, 4, kInteger,
		"eflags", 172+408, 4, kInteger,
		"error_code", 160+408, 4, kInteger,
		"cs", 168+408, 2, kInteger,
		"ds", 120+408, 2, kInteger,
		"es", 116+408, 2, kInteger,
		"fs", 112+408, 2, kInteger,
		"gs", 108+408, 2, kInteger,
		"ss", 180+408, 2, kInteger,
		"esp_res", 136+408, 4, kInteger,
		"trap_no", 156+408, 4, kInteger,

		/* FP old-style regs */
		"fp_control", 0, 2, kInteger,
		"fp_status", 4, 2, kInteger,
		"fp_tag", 8, 2, kInteger,
		"fp_eip", 12, 4, kInteger,
		"fp_cs", 16, 2, kInteger,
		"fp_opcode", 18, 2, kInteger,
		"fp_datap", 20, 4, kInteger,
		"fp_ds", 22, 2, kInteger,
	}
};

Resource 'RegI' (2)
{
	{
		"eax", 152+408, 4, kInteger,
		"ebx", 140+408, 4, kInteger,
		"ecx", 148+408, 4, kInteger,
		"edx", 144+408, 4, kInteger,
		"ebp", 132+408, 4, kInteger,
		"edi", 124+408, 4, kInteger,
		"esi", 128+408, 4, kInteger,
		"eip", 164+408, 4, kInteger,
		"uesp", 176+408, 4, kInteger,
		"eflags", 172+408, 4, kInteger,
		"error_code", 160+408, 4, kInteger,
		"cs", 168+408, 2, kInteger,
		"ds", 120+408, 2, kInteger,
		"es", 116+408, 2, kInteger,
		"fs", 112+408, 2, kInteger,
		"gs", 108+408, 2, kInteger,
		"ss", 180+408, 2, kInteger,
		"esp_res", 136+408, 4, kInteger,
		"trap_no", 156+408, 4, kInteger,

		/* FP new-style */
		"fp_control", 0, 2, kInteger,
		"fp_status", 2, 2, kInteger,
		"fp_tag", 4, 2, kInteger,
		"fp_opcode", 6, 2, kInteger,
		"fp_eip", 8, 4, kInteger,
		"fp_cs", 12, 2, kInteger,
		"fp_datap", 16, 4, kInteger,
		"fp_ds", 20, 2, kInteger,
		"mxcsr", 24, 4, kInteger,

		/* FP stack and MMX, which share storage */
		"st0/mm0", 32, 10, kInteger,
		"st1/mm1", 48, 10, kInteger,
		"st2/mm2", 64, 10, kInteger,
		"st3/mm3", 80, 10, kInteger,
		"st4/mm4", 96, 10, kInteger,
		"st5/mm5", 112, 10, kInteger,
		"st6/mm6", 128, 10, kInteger,
		"st7/mm7", 144, 10, kInteger,

		/* XMMX / SSE registers */
		"xmm0", 160, 16, kInteger,
		"xmm1", 176, 16, kInteger,
		"xmm2", 192, 16, kInteger,
		"xmm3", 208, 16, kInteger,
		"xmm4", 224, 16, kInteger,
		"xmm5", 240, 16, kInteger,
		"xmm6", 256, 16, kInteger,
		"xmm7", 272, 16, kInteger,
	}
};

/* Regl 10 is for ARM */
Resource 'RegI' (0)
{
	{
		"r0", 0, 4, kInteger,
		"r1", 4, 4, kInteger,
		"r2", 8, 4, kInteger,
		"r3", 12, 4, kInteger,
		"r4", 16, 4, kInteger,
		"r5", 20, 4, kInteger,
		"r6", 24, 4, kInteger,
		"r7", 28, 4, kInteger,
		"r8", 32, 4, kInteger,
		"r9", 36, 4, kInteger,
		"r10", 40, 4, kInteger,
		"r11", 44, 4, kInteger,
		"r12", 48, 4, kInteger,
		"r13", 52, 4, kInteger,
		"r14", 56, 4, kInteger,
		"r15", 60, 4, kInteger,
	}
};

Resource 'MENU' (200, "Data")
{
	"Data",
	{
		Item		{ "View Memory", kMsgDumpMemory, none, noKey },
		Item		{ "Add Watchpoint", kMsgAddWatchpoint, none, noKey },
	}
};

Resource 'MBAR' (200, "RegisterWindowMenuBar" )
{
	{
		200
	}
};
