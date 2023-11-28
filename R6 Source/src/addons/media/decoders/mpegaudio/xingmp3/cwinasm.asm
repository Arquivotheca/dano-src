;/*
; * Window functions ASM version for the XingMPG Decoder
; * 16bits output version
; *
; * Copyright (c) 1999, Jean-Michel HERVE
; *
; * Code : TuO / StG
; * Date : 08/04/99
; *
; * NOTE : All functions tested succesfully
; * NOTE : Macros can only accept 2 or 4 as argument
; *
; * This program is free software; you can redistribute it and/or modify
; * it under the terms of the GNU General Public License as published by
; * the Free Software Foundation; either version 2 of the License, or
; * (at your option) any later version.
; * 
; * This program is distributed in the hope that it will be useful,
; * but WITHOUT ANY WARRANTY; without even the implied warranty of
; * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; * GNU General Public License for more details.
; * 
; * You should have received a copy of the GNU General Public License
; * along with this program; if not, write to the Free Software
; * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
; * 
; * $Id: cwinasm.asm,v 1.3 1999/04/22 08:24:01 mhw Exp $
; */

  BITS 32

SECTION .data USE32

GLOBAL window_mpg_asm
GLOBAL window_dual_asm
GLOBAL window16_asm
GLOBAL window16_dual_asm
GLOBAL window8_asm
GLOBAL window8_dual_asm

EXTERN wincoef

SI_MASK  equ 111111111b
SI_MASK2 equ 1111111b

Temp dd 0

;/* Small table for all ((Value+64)&511) progression */
ADD64AND511:
%assign Value 0
%rep 512
        dd ((Value+64) & SI_MASK)
%assign Value Value+1
%endrep

;/* Small table for all ((Value+16)&127) progression */
ADD16AND127:
%assign Value 0
%rep 128
        dd ((Value+16) & SI_MASK2)
%assign Value Value+1
%endrep

SECTION .text USE32

;/*
; * This macro is general for mono and stereo output (full rate)
; * It allow to optimize only this piece of code for both versions
; */
%macro window32 1 ;// PROC vbuf, vb_ptr, pcm :  DWORD
                  ;//      ebp   ebp+4   ebp+8
        push   ebp
        lea    ebp,[esp+8]
        pushad

        mov    esi,[ebp+8]             ;/* pcm is esi */
        lea    esi,[esi+16*%1]         ;/* Points to the end of half part */
        mov    edx,[ebp+4]
        add    edx,16                  ;/* SI is edx */
        mov    ebp,[ebp]               ;/* vbuf is ebp */
        mov    ebx,edx
        add    ebx,32
        and    ebx,SI_MASK             ;/* BX is ebx */
        mov    edi,wincoef            ;/* coef is edi */

;/*-- first 16 --*/
        mov    ecx,-16                 ;/* Use negative to 0 counter */
      .FirstLoop:
          fldz
;// Unrolled loop with avoided incrementation of buffer
%assign j 0
%rep 8
            fld     dword [edi+j*4]
            fmul    dword [ebp+edx*4]
            faddp   st1,st0
            mov     edx,[ADD64AND511+edx*4]
            fld     dword [edi+j*4+4]
            fmul    dword [ebp+ebx*4]
            fsubp   st1,st0
            mov     ebx,[ADD64AND511+ebx*4]
%assign j j+2
%endrep
          add    edi,64                ;/* Update buffer position */

          inc    edx
          dec    ebx

          fistp  dword [Temp]          ;/* Check for 16b validity */
          mov    eax,[Temp]
          cmp    eax,32767
          jle    .GoodSup
          mov    eax,32767
          jmp    .GoodDown
        .GoodSup:
          cmp    eax,-32768
          jge    .GoodDown
          mov    eax,-32768
        .GoodDown:

          mov    word [esi+ecx*%1],ax  ;/* Store value using counter as index */

        inc    ecx                     ;/* Increment counter to next value */
        jnz    near .FirstLoop

;/*--  special case --*/
        fldz
;// Unrolled loop
%assign j 0
%rep 8
          fld     dword [edi+j*4]
          fmul    dword [ebp+ebx*4]
          faddp   st1,st0
          mov     ebx,[ADD64AND511+ebx*4]
%assign j j+1
%endrep

        fistp  dword [Temp]            ;/* Check for 16b validity */
        mov    eax,[Temp]
        cmp    eax,32767
        jle    .GoodSup2
        mov    eax,32767
        jmp    .GoodDown2
      .GoodSup2:
        cmp    eax,-32768
        jge    .GoodDown2
        mov    eax,-32768
      .GoodDown2:

        mov    word [esi],ax           ;/* Store value */

;/*-- last 15 --*/
        lea    edi,[wincoef+255*4]    ;/* coef[255] is edi */
        lea    esi,[esi+16*%1]         ;/* Put to the end of second half */

        mov    ecx,-15                 ;/* Use negative to 0 counter */
      .SecondLoop:

          dec     edx
          inc     ebx

          fldz
;// Unrolled loop with avoided incrementation of buffer
%assign j 0
%rep 8
            fld     dword [edi-j*4]
            fmul    dword [ebp+edx*4]
            faddp   st1,st0
            mov     edx,[ADD64AND511+edx*4]
            fld     dword [edi-j*4-4]
            fmul    dword [ebp+ebx*4]
            faddp   st1,st0
            mov     ebx,[ADD64AND511+ebx*4]
%assign j j+2
%endrep
          sub    edi,64                ;/* Update buffer position */

          fistp  dword [Temp]          ;/* Check for 16b validity */
          mov    eax,[Temp]
          cmp    eax,32767
          jle    .GoodSup3
          mov    eax,32767
          jmp    .GoodDown3
        .GoodSup3:
          cmp    eax,-32768
          jge    .GoodDown3
          mov    eax,-32768
        .GoodDown3:

          mov    word [esi+ecx*%1],ax  ;/* Store value using counter has index */

        inc    ecx                     ;/* Increment counter */
        jnz    near .SecondLoop

        popad
        pop    ebp
        ret
%endmacro

;/*
; * This macro is general for mono and stereo output (half rate)
; * It allow to optimize only this piece of code for both versions
; */
%macro window16 1 ;// PROC vbuf, vb_ptr, pcm :  DWORD
                  ;//      ebp   ebp+4   ebp+8
        push   ebp
        lea    ebp,[esp+8]
        pushad

        mov    esi,[ebp+8]             ;/* pcm is esi */
        lea    esi,[esi+8*%1]          ;/* Points to the end of half part */
        mov    edx,[ebp+4]
        add    dl,8                    ;/* SI is edx */
        mov    ebp,[ebp]               ;/* vbuf is ebp */
        mov    ebx,edx
        add    bl,16                   ;/* BX is ebx */
        mov    edi,wincoef            ;/* coef is edi */

;/*-- first 8 --*/
        mov    ecx,-8                  ;/* Use negative to 0 counter */
      .FirstLoop:
          fldz
;// Unrolled loop with avoided incrementation of buffer
%assign j 0
%rep 8
            fld     dword [edi+j*4]
            fmul    dword [ebp+edx*4]
            faddp   st1,st0
            add     dl,32
            fld     dword [edi+j*4+4]
            fmul    dword [ebp+ebx*4]
            fsubp   st1,st0
            add     bl,32
%assign j j+2
%endrep
          add    edi,64+64             ;/* Update buffer position */

          inc    dl
          dec    bl

          fistp  dword [Temp]          ;/* Check for 16b validity */
          mov    eax,[Temp]
          cmp    eax,32767
          jle    .GoodSup
          mov    eax,32767
          jmp    .GoodDown
        .GoodSup:
          cmp    eax,-32768
          jge    .GoodDown
          mov    eax,-32768
        .GoodDown:

          mov    word [esi+ecx*%1],ax  ;/* Store value using counter as index */

        inc    ecx                     ;/* Increment counter to next value */
        jnz    near .FirstLoop

;/*--  special case --*/
        fldz
;// Unrolled loop
%assign j 0
%rep 8
          fld     dword [edi+j*4]
          fmul    dword [ebp+ebx*4]
          faddp   st1,st0
          add     bl,32
%assign j j+1
%endrep

        fistp  dword [Temp]            ;/* Check for 16b validity */
        mov    eax,[Temp]
        cmp    eax,32767
        jle    .GoodSup2
        mov    eax,32767
        jmp    .GoodDown2
      .GoodSup2:
        cmp    eax,-32768
        jge    .GoodDown2
        mov    eax,-32768
      .GoodDown2:

        mov    word [esi],ax           ;/* Store value */

;/*-- last 7 --*/
        lea    edi,[wincoef+(255-16)*4] ;/* coef[255] is edi */
        lea    esi,[esi+8*%1]          ;/* Put to the end of second half */

        mov    ecx,-7                  ;/* Use negative to 0 counter */
      .SecondLoop:

          dec     dl
          inc     bl

          fldz
;// Unrolled loop with avoided incrementation of buffer
%assign j 0
%rep 8
            fld     dword [edi-j*4]
            fmul    dword [ebp+edx*4]
            faddp   st1,st0
            add     dl,32
            fld     dword [edi-j*4-4]
            fmul    dword [ebp+ebx*4]
            faddp   st1,st0
            add     bl,32
%assign j j+2
%endrep
          sub    edi,64+64             ;/* Update buffer position */

          fistp  dword [Temp]          ;/* Check for 16b validity */
          mov    eax,[Temp]
          cmp    eax,32767
          jle    .GoodSup3
          mov    eax,32767
          jmp    .GoodDown3
        .GoodSup3:
          cmp    eax,-32768
          jge    .GoodDown3
          mov    eax,-32768
        .GoodDown3:

          mov    word [esi+ecx*%1],ax  ;/* Store value using counter has index */

        inc    ecx                     ;/* Increment counter */
        jnz    near .SecondLoop

        popad
        pop    ebp
        ret
%endmacro

;/*
; * This macro is general for mono and stereo output (quarter rate)
; * It allow to optimize only this piece of code for both versions
; */
%macro window8 1  ;// PROC vbuf, vb_ptr, pcm :  DWORD
                  ;//      ebp   ebp+4   ebp+8
        push   ebp
        lea    ebp,[esp+8]
        pushad

        mov    esi,[ebp+8]             ;/* pcm is esi */
        lea    esi,[esi+4*%1]          ;/* Points to the end of half part */
        mov    edx,[ebp+4]
        add    edx,4                   ;/* SI is edx */
        mov    ebp,[ebp]               ;/* vbuf is ebp */
        mov    ebx,edx
        add    ebx,8
        and    ebx,SI_MASK2            ;/* BX is ebx */
        mov    edi,wincoef            ;/* coef is edi */

;/*-- first 16 --*/
        mov    ecx,-4                  ;/* Use negative to 0 counter */
      .FirstLoop:
          fldz
;// Unrolled loop with avoided incrementation of buffer
%assign j 0
%rep 8
            fld     dword [edi+j*4]
            fmul    dword [ebp+edx*4]
            faddp   st1,st0
            mov     edx,[ADD16AND127+edx*4]
            fld     dword [edi+j*4+4]
            fmul    dword [ebp+ebx*4]
            fsubp   st1,st0
            mov     ebx,[ADD16AND127+ebx*4]
%assign j j+2
%endrep
          add    edi,64+48*4           ;/* Update buffer position */

          inc    edx
          dec    ebx

          fistp  dword [Temp]          ;/* Check for 16b validity */
          mov    eax,[Temp]
          cmp    eax,32767
          jle    .GoodSup
          mov    eax,32767
          jmp    .GoodDown
        .GoodSup:
          cmp    eax,-32768
          jge    .GoodDown
          mov    eax,-32768
        .GoodDown:

          mov    word [esi+ecx*%1],ax  ;/* Store value using counter as index */

        inc    ecx                     ;/* Increment counter to next value */
        jnz    near .FirstLoop

;/*--  special case --*/
        fldz
;// Unrolled loop
%assign j 0
%rep 8
          fld     dword [edi+j*4]
          fmul    dword [ebp+ebx*4]
          faddp   st1,st0
          mov     ebx,[ADD16AND127+ebx*4]
%assign j j+1
%endrep

        fistp  dword [Temp]            ;/* Check for 16b validity */
        mov    eax,[Temp]
        cmp    eax,32767
        jle    .GoodSup2
        mov    eax,32767
        jmp    .GoodDown2
      .GoodSup2:
        cmp    eax,-32768
        jge    .GoodDown2
        mov    eax,-32768
      .GoodDown2:

        mov    word [esi],ax           ;/* Store value */

;/*-- last 15 --*/
        lea    edi,[wincoef+(255-48)*4] ;/* coef[255] is edi */
        lea    esi,[esi+4*%1]          ;/* Put to the end of second half */

        mov    ecx,-3                  ;/* Use negative to 0 counter */
      .SecondLoop:

          dec     edx
          inc     ebx

          fldz
;// Unrolled loop with avoided incrementation of buffer
%assign j 0
%rep 8
            fld     dword [edi-j*4]
            fmul    dword [ebp+edx*4]
            faddp   st1,st0
            mov     edx,[ADD16AND127+edx*4]
            fld     dword [edi-j*4-4]
            fmul    dword [ebp+ebx*4]
            faddp   st1,st0
            mov     ebx,[ADD16AND127+ebx*4]
%assign j j+2
%endrep
          sub    edi,64+48*4           ;/* Update buffer position */

          fistp  dword [Temp]          ;/* Check for 16b validity */
          mov    eax,[Temp]
          cmp    eax,32767
          jle    .GoodSup3
          mov    eax,32767
          jmp    .GoodDown3
        .GoodSup3:
          cmp    eax,-32768
          jge    .GoodDown3
          mov    eax,-32768
        .GoodDown3:

          mov    word [esi+ecx*%1],ax  ;/* Store value using counter has index */

        inc    ecx                     ;/* Increment counter */
        jnz    near .SecondLoop

        popad
        pop    ebp
        ret
%endmacro

;/*
; * Window mpg ASM Version
; */
window_mpg_asm:
        window32 2                     ;/* 16b mono */

;/*
; * Window dual ASM Version
; */
window_dual_asm:
        window32 4                     ;/* 16b stereo */

;/*
; * Window16 ASM Version
; */
window16_asm:
        window16 2                     ;/* 16b mono */

;/*
; * Window16 Dual ASM Version
; */
window16_dual_asm:
        window16 4                     ;/* 16b stereo */

;/*
; * Window8 ASM Version
; */
window8_asm:
        window8 2                      ;/* 16b mono */

;/*
; * Window8 Dual ASM Version
; */
window8_dual_asm:
        window8 2                      ;/* 16b stereo */

end
