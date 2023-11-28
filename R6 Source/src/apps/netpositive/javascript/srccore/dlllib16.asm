; WinDll16.asm - ONLY FOR 16-bit Windows or DOS version running as a DLL.  This
;                is the code for dispatching to those routines and getting
;                DS correct.

; (c) COPYRIGHT 1993-98           NOMBAS, INC.
;                                 64 SALEM ST.
;                                 MEDFORD, MA 02155  USA
; 
; ALL RIGHTS RESERVED
; 
; This software is the property of Nombas, Inc. and is furnished under
; license by Nombas, Inc.; this software may be used only in accordance
; with the terms of said license.  This copyright notice may not be removed,
; modified or obliterated without the prior written permission of Nombas, Inc.
; 
; This software is a Trade Secret of Nombas, Inc.
; 
; This software may not be copied, transmitted, provided to or otherwise made
; available to any other person, company, corporation or other entity except
; as specified in the terms of said license.
; 
; No right, title, ownership or other interest in the software is hereby
; granted or transferred.
; 
; The information contained herein is subject to change without notice and
; should not be construed as a commitment by Nombas, Inc.

;   extern uword32 cdecl FAR_CALL DispatchToClient(uword16 DataSegment[bp+6],
;            ClientFunction FClient[bp+8],void _FAR_ *Parm1[bp+12],void _FAR_ *Parm2[bp+16]
;                                        ,void _FAR_ *Parm3[bp+20],void _FAR_ *Parm4[bp+24]);

;   extern uword32 cdecl FAR_CALL DispatchToClient(uword16 DataSegment[bp+4],
;            ClientFunction FClient[bp+6],void _FAR_ *Parm1[bp+8],void _FAR_ *Parm2[bp+12]
;                                        ,void _FAR_ *Parm3[bp+16],void _FAR_ *Parm4[bp+20]);

;   extern uword16 _FAR_ * cdecl NEAR_CALL Get_SS_BP(); return ss:bp

JSE_TEXT   SEGMENT BYTE PUBLIC 'CODE'

   public   _DispatchToClient
   public   _Get_SS_BP


_DispatchToClient PROC  FAR
   ; initialize stack values
   push  bp
   mov   bp, sp
; save all them registers
   push  ds
   nop
   nop
; get ds for that instance
;   mov   bx, [bp+6]
;   mov   ax, 0002H
;   int   31h            ;DPMI segment to selector
;                        ;now ax=selector
;   mov   ds, ax
;                        ;umm, looks like we don't need dpmi
   push ss
   pop  ds
   
; copy the parameters onto the stack
   
   push  [bp+26]
   push  [bp+24]
   push  [bp+22]
   push  [bp+20]
   push  [bp+18]
   push  [bp+16]
   push  [bp+14]
   push  [bp+12]
; far call to the code
   call  dword ptr [bp+8]
   add   sp, 16
; restore all the previously saved registers
   pop   ds
; restore bp
   pop   bp
; can return; AX:DX already contains return value, if any
   ret
_DispatchToClient ENDP


_Get_SS_BP  PROC  FAR
   mov   dx, ss
   mov   ax, bp
   ret
_Get_SS_BP  ENDP

JSE_TEXT   ENDS

   END
