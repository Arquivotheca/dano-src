//==============================================================================
//  Check running on MMX processor.
//
//  Checking for ability to set/clear ID flag (Bit 21) in EFLAGS
//  which indicates the presence of a processor with the ability
//  to use the CPUID instruction. Then check CPUID.
//
//  Copyright(C) 1997 CANOPUS Co.,Ltd.  <<<<< Don't change copyright!
//  1997/1/27 Tom created.				<<<<< Don't change originality!
//
//  <!!! WARNING !!!>
//  Don't change original comments. It's the morality for programmer.
//  Don't modify a lot. You can modify minimum.
//	10/21/98 Tom > add Katmai new instruction set support.
//==============================================================================
#include <windows.h>
#include "cputype.h"
#pragma	warning(disable:4704)		// disable warning message (can not do global optimization)

int	PASCAL GetCpuType( void )
{
	int	MMX_CPU;

	_asm {
		push	ebx				; EBX ﾚｼﾞｽﾀを変更する命令がｺｰﾄﾞにないのでｺﾝﾊﾟｲﾗはEBXをｾｰﾌﾞしない｡ｾｰﾌﾞが必要
		pushfd
		pop		ecx				; get original EFLAGS value.
		mov		eax,ecx
		xor		eax,200000h		; flip (XOR) ID bit in EFLAGS
		push	eax				; save new EFLAGS value on stack
		popfd					; replace current EFLAGS value
		pushfd					; get new EFLAGS
		pop		eax				; store new EFLAGS in EAX
		xor		eax,ecx			; can't toggle ID bit,
		je      end_get_cpuid	; CPU=80486
		mov		eax,1			; request for feature flags.
		_emit	0x0f			; cpuid (これはEAX,EBX,ECX,EDXを変更する)
		_emit	0xa2
		mov		eax,CPU_PRO
		test	edx,00800000h	; is IA MMX technology bit(bit 23 of EDX) in feature flags set?
		jz		end_get_cpuid
		mov		eax,CPU_MMX
		test	edx,02000000h	; is IA XMM technology bit(bit 25 of EDX) in feature flags set?
		jz		end_get_cpuid
		mov		eax,CPU_XMM
	end_get_cpuid:
		mov		MMX_CPU,eax		; 1:MMX, 5:XMM(katmai) instruction available.
		pop		ebx
	}
	return( MMX_CPU );
}

