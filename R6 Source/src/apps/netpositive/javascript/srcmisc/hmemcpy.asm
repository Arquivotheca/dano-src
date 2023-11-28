; HugeMemCpy.asm - implement DOS and Windows versions of memcpy that
; specifically copy from the bottom up or the top down

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

UTILHUGE_TEXT  SEGMENT BYTE PUBLIC 'CODE'

   public MEMCPYBOTTOMUP
   public MEMCPYTOPDOWN

MEMCPYBOTTOMUP PROC NEAR

   ; save stack
   push  bp
   mov   bp, sp

   ; save registers being used
   pushf
   push  si
   push  di
   push  ds
   push  es

   ; get ds:si point to source, and es:di point to destination
   mov   di, [bp+10]
   mov   ax, [bp+12]
   mov   es, ax
   mov   si, [bp+6]
   mov   ax, [bp+8]
   mov   ds, ax

   ; will be moving up in memory
   cld

   ; finally do the copy, first doing words to be faster
   mov   cx, [bp+4]    ; byte count
   shr   cx, 1          ; convert bytes to words
   rep   movsw

   ; if a remaining byte, then copy that
   mov   cx, [bp+4]    ; byte count
   and   cx, 1          ; only care about the last non-word byte
   rep   movsb

   ; restore registers
   pop   es
   pop   ds
   pop   di
   pop   si
   popf

   ; all done, bye-bye
   pop   bp
   ret   10

MEMCPYBOTTOMUP ENDP


MEMCPYTOPDOWN PROC NEAR

   ; save stack
   push  bp
   mov   bp, sp

   ; save registers being used
   pushf
   push  si
   push  di
   push  ds
   push  es

   ; get ds:si point to source, and es:di point to destination
   mov   di, [bp+10]
   mov   ax, [bp+12]
   mov   es, ax
   mov   si, [bp+6]
   mov   ax, [bp+8]
   mov   ds, ax

   ; will be moving down in memory
   std

   ; set cx to byte count
   mov   cx, [bp+4]

   ; copying down in memory, so start pointers at top of their memory
   sub   cx, 2
   add   di, cx
   add   si, cx
   add   cx, 2

   ; finally do the copy, first doing words to be faster
   shr   cx, 1          ; convert bytes to words
   rep   movsw

   ; if a remaining byte, then copy that
   inc   di
   inc   si
   mov   cx, [bp+4]    ; byte count
   and   cx, 1          ; only care about the last non-word byte
   rep   movsb

   ; restore registers
   pop   es
   pop   ds
   pop   di
   pop   si
   popf

   ; all done, bye-bye
   pop   bp
   ret   10

MEMCPYTOPDOWN ENDP

UTILHUGE_TEXT  ENDS

   END

;[ ]0291  push    si
;[ ]0292  push    di
;[ ]0293  push    ds
;[ ]0294  push    es
;[ ]0295  push    bp
;[ ]0296  mov     bp,sp
;[ ]0298  mov     di,ax
;[ ]029A  mov     es,dx
;[ ]029C  mov     si,bx
;[ ]029E  mov     ds,cx
;[ ]02A0  mov     cx,+0E[bp]
;[ ]02A3  push    di
;[ ]02A4  shr     cx,1
;[ ]02A6  repe    movsw
;[ ]02A8  adc     cx,cx
;[ ]02AA  repe    movsb
;[ ]02AC  pop     di
;[ ]02AD  pop     bp
;[ ]02AE  pop     es
;[ ]02AF  pop     ds
;[ ]02B0  pop     di
;[ ]02B1  pop     si
;[ ]02B2  retf    0002

