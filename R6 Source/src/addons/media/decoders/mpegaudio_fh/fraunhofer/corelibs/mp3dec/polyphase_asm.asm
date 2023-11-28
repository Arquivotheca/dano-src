; Fraunhofer MP3 decoder
;
; CPolyphase::window_band_s(int bufOffset,short *out_samples,int short_window)
; CPolyphase::window_band_m(int bufOffset,short *out_samples,int short_window)

SECTION .text

HAN_SIZE   equ 512

; window_band_s:  bufOffset  short_window   syn_buf    qual     resl    out_samples
;                 ebp        ebp+4          ebp+8      ebp+12   ebp+16  ebp+20
;
; locals relative to esp
_ISAM       equ -276
_WIN_INC    equ -16
_BUF_INC    equ -12
_WIN_START  equ -8
_TMP_EDI    equ -4

extern debugger

GLOBAL CPolyphaseASM___window_band_s
ALIGN 16
CPolyphaseASM___window_band_s:

	push   ebp
	lea    ebp,[esp+8]
	add    esp,_ISAM
	pushad

	mov   edi, [ebp+8] ; load syn_buf ptr
	
	mov   ecx,[ebp+12] ; fetch qual
	mov   edx,16*4
	shr   edx,cl  
	
	mov   eax,1
	shl   eax,cl
	mov   [esp+_BUF_INC],eax
	
	cmp   dword [ebp+4],0 ; test short_window
	jne   .ba
	
	mov   eax,32*4
	shl   eax,cl
	mov   [esp+_WIN_INC],eax
	mov   dword [esp+_WIN_START],-32
	lea   ecx,[SYN_F_WINDOW+32*4]
	jmp   .baa
.ba:
	mov   eax,24*4
	shl   eax,cl
	mov   [esp+_WIN_INC],eax
	mov   dword [esp+_WIN_START],-24
	add   dword [ebp],64 ;bufOffset
	and   dword [ebp],(HAN_SIZE-1)
	lea   ecx,[SYN_F_WINDOW_SHORT+24*4]

.baa:
%if _USE_3DNOW
	pxor mm7, mm7	; sum 1l
	pxor mm6, mm6	; sum 1r
	pxor mm5, mm5	; sum 2l
	pxor mm4, mm4	; sum 2r
%else
    fldz
    fldz
    fldz
    fldz
%endif
    
	mov   esi,[esp+_WIN_START]
	mov   ebx,[ebp] ;bufOffset

.l_a:
%if _USE_3DNOW
	movd mm0, [ecx+esi*4+0*4]
    movd mm1, [edi+ebx*4+16*4]
    movd mm2, [edi+ebx*4+16*4+512*4]
    pfmul mm1, mm0
    pfmul mm2, mm0
 	pfadd mm7, mm1
 	pfadd mm6, mm2

    add   ebx,32
    and   ebx,(HAN_SIZE-1)
 	
	movd mm0, [ecx+esi*4+3*4]
    movd mm1, [edi+ebx*4]
    movd mm2, [edi+ebx*4+512*4]
    pfmul mm1, mm0
    pfmul mm2, mm0
 	pfadd mm5, mm1
 	pfadd mm4, mm2

	movd mm0, [ecx+esi*4+2*4]
    movd mm1, [edi+ebx*4+16*4]
    movd mm2, [edi+ebx*4+16*4+512*4]
    pfmul mm1, mm0
    pfmul mm2, mm0
 	pfadd mm7, mm1
 	pfadd mm6, mm2
%else
    fld   dword [edi+ebx*4+16*4]
    fmul  dword [ecx+esi*4+0*4]
    fld   dword [edi+ebx*4+16*4+512*4]
    fmul  dword [ecx+esi*4+0*4]
    fxch  st1
    faddp st2,st0
    faddp st2,st0

    add   ebx,32
    and   ebx,(HAN_SIZE-1)

    fld   dword [edi+ebx*4]
    fmul  dword [ecx+esi*4+3*4]
    fld   dword [edi+ebx*4+512*4]
    fmul  dword [ecx+esi*4+3*4]

    fld   dword [edi+ebx*4+16*4]
    fmul  dword [ecx+esi*4+2*4]
    fld   dword [edi+ebx*4+16*4+512*4]
    fmul  dword [ecx+esi*4+2*4]

    fxch  st3
    faddp st6,st0
    faddp st3,st0
    faddp st5,st0
    faddp st2,st0
%endif

    add   ebx,32
    and   ebx,(HAN_SIZE-1)
    add   esi,4
    js    near .l_a

    lea   edi, [esp+_ISAM]

%if _USE_3DNOW
	psllq mm6, 32
	psllq mm4, 32
	por mm7, mm6
	por mm5, mm4
	pf2id mm7, mm7
	pf2id mm5, mm5
	movq [edi+0], mm7
	movq [edi+edx*2], mm5
%else
    fistp dword [edi+0]
    fistp dword [edi+4]
    fistp dword [edi+edx*2]
    fistp dword [edi+edx*2+4]
%endif

    sub   edx,4

.l_b:
%if _USE_3DNOW
	pxor mm7, mm7	; sum 1l
	pxor mm6, mm6	; sum 1r
	pxor mm5, mm5	; sum 2l
	pxor mm4, mm4	; sum 2r
%else
    fldz
    fldz
    fldz
    fldz
%endif

    mov   esi,[esp+_WIN_START]
    mov   ebx,[ebp] ;bufOffset   
    add   ebx,[esp+_BUF_INC]
    mov   [ebp],ebx
    add   ecx,[esp+_WIN_INC]

    mov   [esp+_TMP_EDI], edi
    mov   edi, [ebp+8] ;syn_buf

.l_c:
%if _USE_3DNOW
	movd mm0, [ecx+esi*4+0*4]
    movd mm1, [edi+ebx*4+16*4]
    movd mm2, [edi+ebx*4+16*4+512*4]
    pfmul mm1, mm0
    pfmul mm2, mm0
 	pfadd mm7, mm1
 	pfadd mm6, mm2

	movd mm0, [ecx+esi*4+1*4]
    movd mm1, [edi+ebx*4+16*4]
    movd mm2, [edi+ebx*4+16*4+512*4]
    pfmul mm1, mm0
    pfmul mm2, mm0
 	pfadd mm5, mm1
 	pfadd mm4, mm2

    add   ebx,32
    and   ebx,(HAN_SIZE-1)

	movd mm0, [ecx+esi*4+2*4]
    movd mm1, [edi+ebx*4]
    movd mm2, [edi+ebx*4+512*4]
    pfmul mm1, mm0
    pfmul mm2, mm0
 	pfadd mm7, mm1
 	pfadd mm6, mm2

	movd mm0, [ecx+esi*4+3*4]
    movd mm1, [edi+ebx*4]
    movd mm2, [edi+ebx*4+512*4]
    pfmul mm1, mm0
    pfmul mm2, mm0
 	pfadd mm5, mm1
 	pfadd mm4, mm2
%else
    fld   dword [edi+ebx*4+16*4]
    fmul  dword [ecx+esi*4+0*4]
    
    fld   dword [edi+ebx*4+16*4+512*4]
    fmul  dword [ecx+esi*4+0*4]
    
    fld   dword [edi+ebx*4+16*4]
    fmul  dword [ecx+esi*4+1*4]
    fld   dword [edi+ebx*4+16*4+512*4]
    fmul  dword [ecx+esi*4+1*4]

    fxch  st3

    faddp st4,st0      
    faddp st5,st0      
    faddp st3,st0      
    faddp st4,st0      

    add   ebx,32
    and   ebx,(HAN_SIZE-1)

    fld   dword [edi+ebx*4]
    fmul  dword [ecx+esi*4+2*4]
    fld   dword [edi+ebx*4+512*4]
    fmul  dword [ecx+esi*4+2*4]
    
    fld   dword [edi+ebx*4]
    fmul  dword [ecx+esi*4+3*4]
    fld   dword [edi+ebx*4+512*4]
    fmul  dword [ecx+esi*4+3*4]

    fxch  st3
    faddp st4,st0
    faddp st5,st0
    faddp st3,st0
    faddp st4,st0
%endif

    add   ebx,32
    and   ebx,(HAN_SIZE-1)
    add   esi,4
    js    near .l_c

    mov   edi, [esp+_TMP_EDI]

    lea   edi,[edi+8]

%if _USE_3DNOW
	psllq mm6, 32
	psllq mm4, 32
	por mm7, mm6
	por mm5, mm4
	pf2id mm7, mm7
	pf2id mm5, mm5
	movq [edi+0], mm7
	movq [edi+edx*4], mm5
%else
    fistp dword [edi+0]
    fistp dword [edi+4]
    fistp dword [edi+edx*4]
    fistp dword [edi+edx*4+4]
%endif

    sub   edx,4
    jnz   near .l_b

    mov   edx,64
    mov   ecx,[ebp+12];qual
    shr   edx,cl
    lea   esi,[esp+_ISAM+edx*4]
    mov   edi,[ebp+20] ;out_samples

    cmp   dword [ebp+16],0 ;resl
    je    .c_16bit

    lea   edi,[edi+edx]
    neg   edx

.l_d_byte:  
%if _USE_3DNOW
    movd   mm0, [esi+edx*4]
    packssdw mm0, mm0
    movd eax, mm0
    add   ax,0x8000
%else
    mov   eax,[esi+edx*4]
    cmp   eax,0x00007fff
    jle   .l_e_byte
    mov   eax,0x00007fff
.l_e_byte:  
    cmp   eax,0xffff8000
    jge   .l_f_byte
    mov   eax,0xffff8000
.l_f_byte:  
    add   ax,0x8000
%endif
    mov   [edi+edx],ah
    inc   edx
    js    .l_d_byte
    jmp   .done

.c_16bit:
    lea   edi,[edi+edx*2]
    neg   edx

.l_d_word:  
%if _USE_3DNOW
    movd   mm0, [esi+edx*4]
    packssdw mm0, mm0
    movd eax, mm0
%else
    mov   eax,[esi+edx*4]
    cmp   eax,0x00007fff
    jle   .l_e_word
    mov   eax,0x00007fff
.l_e_word: 
    cmp   eax,0xffff8000
    jge   .l_f_word
    mov   eax,0xffff8000
.l_f_word:
%endif
    mov   [edi+edx*2],ax
    inc   edx
    js    .l_d_word
      
.done:
	popad
	sub    esp,_ISAM
	pop    ebp
%if _USE_3DNOW
	femms
%endif
	ret

; ----------------------------------------------------------------------------

GLOBAL CPolyphaseASM___window_band_m
ALIGN 16
CPolyphaseASM___window_band_m:

	push   ebp
	lea    ebp,[esp+8]
	add    esp,_ISAM
	pushad

  mov   edi, [ebp+8] ; load syn_buf ptr
	
  mov   ecx,[ebp+12] ; fetch qual
  mov   edx,16*4
  shr   edx,cl  
    
  mov   eax,1
  shl   eax,cl
  mov   [esp+_BUF_INC],eax

  cmp   dword [ebp+4],0 ; test short_window
  jne   .ba
    
  mov   eax,32*4
  shl   eax,cl
  mov   [esp+_WIN_INC],eax
  mov   dword [esp+_WIN_START],-32
  lea   ecx,[SYN_F_WINDOW+32*4]
  jmp   .baa
.ba:
  mov   eax,24*4
  shl   eax,cl
  mov   [esp+_WIN_INC],eax
  mov   dword [esp+_WIN_START],-24
  add   dword [ebp],64 ;bufOffset
  and   dword [ebp],(HAN_SIZE-1)
  lea   ecx,[SYN_F_WINDOW_SHORT+24*4]

.baa:
  fldz
  fldz
    
  mov   esi,[esp+_WIN_START]
  mov   ebx,[ebp] ;bufOffset

.l_a:
    fld   dword [edi+ebx*4+16*4]
    fmul  dword [ecx+esi*4+0*4]

    add   ebx,32
    and   ebx,(HAN_SIZE-1)

    fld   dword [edi+ebx*4]
    fmul  dword [ecx+esi*4+3*4]

    fld   dword [edi+ebx*4+16*4]
    fmul  dword [ecx+esi*4+2*4]
    
    fxch  st2
    faddp st3,st0
    faddp st3,st0
    faddp st1,st0

    add   ebx,32
    and   ebx,(HAN_SIZE-1)
    add   esi,4
    js    .l_a

    lea   edi, [esp+_ISAM]

    fistp dword [edi+0]
    fistp dword [edi+edx]

    sub   edx,4

.l_b:
    fldz
    fldz

    mov   esi,[esp+_WIN_START]
    mov   ebx,[ebp] ;bufOffset   
    add   ebx,[esp+_BUF_INC]
    mov   [ebp],ebx
    add   ecx,[esp+_WIN_INC]

    mov   [esp+_TMP_EDI], edi
    mov   edi, [ebp+8] ;syn_buf

.l_c:
    fld   dword [edi+ebx*4+16*4]
    fmul  dword [ecx+esi*4+0*4]
    
    fld   dword [edi+ebx*4+16*4]
    fmul  dword [ecx+esi*4+1*4]

    fxch  st1

    faddp st2,st0      
    faddp st2,st0      

    add   ebx,32
    and   ebx,(HAN_SIZE-1)

    fld   dword [edi+ebx*4]
    fmul  dword [ecx+esi*4+2*4]
    
    fld   dword [edi+ebx*4]
    fmul  dword [ecx+esi*4+3*4]

    fxch  st1
    faddp st2,st0
    faddp st2,st0

    add   ebx,32
    and   ebx,(HAN_SIZE-1)
    add   esi,4
    js    .l_c

    mov   edi, [esp+_TMP_EDI]

    lea   edi,[edi+4]

    fistp dword [edi]
    fistp dword [edi+edx*2]

    sub   edx,4
    jnz   near .l_b

    mov   edx,32
    mov   ecx,[ebp+12];qual
    shr   edx,cl
    lea   esi,[esp+_ISAM+edx*4]
    mov   edi,[ebp+20] ;out_samples

    cmp   dword [ebp+16],0 ;resl
    je    .c_16bit

    lea   edi,[edi+edx]
    neg   edx

.l_d_byte:  
    mov   eax,[esi+edx*4]
    cmp   eax,0x00007fff
    jle   .l_e_byte
    mov   eax,0x00007fff
.l_e_byte:  
    cmp   eax,0xffff8000
    jge   .l_f_byte
    mov   eax,0xffff8000
.l_f_byte:  
    add   ax,0x8000
    mov   [edi+edx],ah
    inc   edx
    js    .l_d_byte
    jmp   .done

.c_16bit:
    lea   edi,[edi+edx*2]
    neg   edx

.l_d_word:  
    mov   eax,[esi+edx*4]
    cmp   eax,0x00007fff
    jle   .l_e_word
    mov   eax,0x00007fff
.l_e_word: 
    cmp   eax,0xffff8000
    jge   .l_f_word
    mov   eax,0xffff8000
.l_f_word:
    mov   [edi+edx*2],ax
    inc   edx
    js    .l_d_word
      
.done:

	popad
	sub    esp,_ISAM
	pop    ebp
	ret

.data

ALIGN 32
SYN_F_WINDOW:
dd   0.00000000, 0.00007629, 0.00044250, 0.00158691
dd   0.00325012,-0.00222778, 0.00700378, 0.02391052
dd   0.03108215, 0.00068665, 0.07862854, 0.14842224
dd   0.10031128, 0.15220642, 0.57203674, 0.97685242
dd   1.14498901,-0.97685242,-0.57203674,-0.15220642
dd   0.10031128,-0.14842224,-0.07862854,-0.00068665
dd   0.03108215,-0.02391052,-0.00700378, 0.00222778
dd   0.00325012,-0.00158691,-0.00044250,-0.00007629

dd  -0.00001526, 0.00039673, 0.00047302, 0.00317383
dd   0.00332642, 0.00611877, 0.00791931, 0.03147888
dd   0.03051758, 0.07305908, 0.08418274, 0.10885620
dd   0.09092712, 0.54382324, 0.60021973, 1.14428711
dd   1.14428711,-0.60021973,-0.54382324, 0.09092712
dd   0.10885620,-0.08418274,-0.07305908, 0.03051758
dd   0.03147888,-0.00791931,-0.00611877, 0.00332642
dd   0.00317383,-0.00047302,-0.00039673,-0.00001526

dd  -0.00001526, 0.00036621, 0.00053406, 0.00308227
dd   0.00338745, 0.00529480, 0.00886536, 0.03173828
dd   0.02978516, 0.06752014, 0.08970642, 0.11657715
dd   0.08068848, 0.51560974, 0.62829590, 1.14221191
dd   1.14221191,-0.62829590,-0.51560974, 0.08068848
dd   0.11657715,-0.08970642,-0.06752014, 0.02978516
dd   0.03173828,-0.00886536,-0.00529480, 0.00338745
dd   0.00308227,-0.00053406,-0.00036621,-0.00001526

dd  -0.00001526, 0.00032044, 0.00057983, 0.00299072
dd   0.00343323, 0.00448608, 0.00984192, 0.03184509
dd   0.02888489, 0.06199646, 0.09516907, 0.12347412
dd   0.06959534, 0.48747253, 0.65621948, 1.13876343
dd   1.13876343,-0.65621948,-0.48747253, 0.06959534
dd   0.12347412,-0.09516907,-0.06199646, 0.02888489
dd   0.03184509,-0.00984192,-0.00448608, 0.00343323
dd   0.00299072,-0.00057983,-0.00032044,-0.00001526

dd  -0.00001526, 0.00028992, 0.00062561, 0.00289917
dd   0.00346374, 0.00372314, 0.01084900, 0.03181458
dd   0.02780151, 0.05653381, 0.10054016, 0.12957764
dd   0.05761719, 0.45947266, 0.68391418, 1.13392639
dd   1.13392639,-0.68391418,-0.45947266, 0.05761719
dd   0.12957764,-0.10054016,-0.05653381, 0.02780151
dd   0.03181458,-0.01084900,-0.00372314, 0.00346374
dd   0.00289917,-0.00062561,-0.00028992,-0.00001526

dd  -0.00001526, 0.00025940, 0.00068665, 0.00279236
dd   0.00347900, 0.00300598, 0.01188660, 0.03166199
dd   0.02653503, 0.05113220, 0.10581970, 0.13488770
dd   0.04478455, 0.43165588, 0.71131897, 1.12774658
dd   1.12774658,-0.71131897,-0.43165588, 0.04478455
dd   0.13488770,-0.10581970,-0.05113220, 0.02653503
dd   0.03166199,-0.01188660,-0.00300598, 0.00347900
dd   0.00279236,-0.00068665,-0.00025940,-0.00001526

dd  -0.00001526, 0.00024414, 0.00074768, 0.00268555
dd   0.00347900, 0.00233459, 0.01293945, 0.03138733
dd   0.02508545, 0.04583740, 0.11094666, 0.13945007
dd   0.03108215, 0.40408325, 0.73837280, 1.12022400
dd   1.12022400,-0.73837280,-0.40408325, 0.03108215
dd   0.13945007,-0.11094666,-0.04583740, 0.02508545
dd   0.03138733,-0.01293945,-0.00233459, 0.00347900
dd   0.00268555,-0.00074768,-0.00024414,-0.00001526

dd  -0.00003052, 0.00021362, 0.00080872, 0.00257873
dd   0.00346374, 0.00169373, 0.01402283, 0.03100586
dd   0.02342224, 0.04063416, 0.11592102, 0.14326477
dd   0.01651001, 0.37680054, 0.76502991, 1.11137390
dd   1.11137390,-0.76502991,-0.37680054, 0.01651001
dd   0.14326477,-0.11592102,-0.04063416, 0.02342224
dd   0.03100586,-0.01402283,-0.00169373, 0.00346374
dd   0.00257873,-0.00080872,-0.00021362,-0.00003052

dd  -0.00003052, 0.00019836, 0.00088501, 0.00245667
dd   0.00341797, 0.00109863, 0.01512146, 0.03053284
dd   0.02157593, 0.03555298, 0.12069702, 0.14636230
dd   0.00106812, 0.34986877, 0.79121399, 1.10121155
dd   1.10121155,-0.79121399,-0.34986877, 0.00106812
dd   0.14636230,-0.12069702,-0.03555298, 0.02157593
dd   0.03053284,-0.01512146,-0.00109863, 0.00341797
dd   0.00245667,-0.00088501,-0.00019836,-0.00003052

dd  -0.00003052, 0.00016785, 0.00096130, 0.00234985
dd   0.00337219, 0.00054932, 0.01623535, 0.02993774
dd   0.01953125, 0.03060913, 0.12525940, 0.14877319
dd  -0.01522827, 0.32331848, 0.81686401, 1.08978271
dd   1.08978271,-0.81686401,-0.32331848,-0.01522827
dd   0.14877319,-0.12525940,-0.03060913, 0.01953125
dd   0.02993774,-0.01623535,-0.00054932, 0.00337219
dd   0.00234985,-0.00096130,-0.00016785,-0.00003052

dd  -0.00003052, 0.00015259, 0.00103760, 0.00224304
dd   0.00328064, 0.00003052, 0.01734924, 0.02928162
dd   0.01725769, 0.02581787, 0.12956238, 0.15049744
dd  -0.03237915, 0.29721069, 0.84194946, 1.07711792
dd   1.07711792,-0.84194946,-0.29721069,-0.03237915
dd   0.15049744,-0.12956238,-0.02581787, 0.01725769
dd   0.02928162,-0.01734924,-0.00003052, 0.00328064
dd   0.00224304,-0.00103760,-0.00015259,-0.00003052

dd  -0.00004578, 0.00013733, 0.00111389, 0.00212097
dd   0.00317383,-0.00044250, 0.01846313, 0.02853394
dd   0.01480103, 0.02117920, 0.13359070, 0.15159607
dd  -0.05035400, 0.27159119, 0.86636353, 1.06321716
dd   1.06321716,-0.86636353,-0.27159119,-0.05035400
dd   0.15159607,-0.13359070,-0.02117920, 0.01480103
dd   0.02853394,-0.01846313, 0.00044250, 0.00317383
dd   0.00212097,-0.00111389,-0.00013733,-0.00004578

dd  -0.00004578, 0.00012207, 0.00120544, 0.00201416
dd   0.00305176,-0.00086975, 0.01957703, 0.02772522
dd   0.01211548, 0.01670837, 0.13729858, 0.15206909
dd  -0.06916809, 0.24650574, 0.89009094, 1.04815674
dd   1.04815674,-0.89009094,-0.24650574,-0.06916809
dd   0.15206909,-0.13729858,-0.01670837, 0.01211548
dd   0.02772522,-0.01957703, 0.00086975, 0.00305176
dd   0.00201416,-0.00120544,-0.00012207,-0.00004578

dd  -0.00006103, 0.00010681, 0.00129700, 0.00190735
dd   0.00288391,-0.00126648, 0.02069092, 0.02684021
dd   0.00923157, 0.01242065, 0.14067078, 0.15196228
dd  -0.08877563, 0.22198486, 0.91305542, 1.03193665
dd   1.03193665,-0.91305542,-0.22198486,-0.08877563
dd   0.15196228,-0.14067078,-0.01242065, 0.00923157
dd   0.02684021,-0.02069092, 0.00126648, 0.00288391
dd   0.00190735,-0.00129700,-0.00010681,-0.00006103

dd  -0.00006103, 0.00010681, 0.00138855, 0.00178528
dd   0.00270081,-0.00161743, 0.02178955, 0.02590942
dd   0.00613403, 0.00831604, 0.14367676, 0.15130615
dd  -0.10916138, 0.19805908, 0.93519592, 1.01461792
dd   1.01461792,-0.93519592,-0.19805908,-0.10916138
dd   0.15130615,-0.14367676,-0.00831604, 0.00613403
dd   0.02590942,-0.02178955, 0.00161743, 0.00270081
dd   0.00178528,-0.00138855,-0.00010681,-0.00006103

dd  -0.00007629, 0.00009155, 0.00148010, 0.00169373
dd   0.00248718,-0.00193787, 0.02285767, 0.02493286
dd   0.00282288, 0.00439453, 0.14625549, 0.15011597
dd  -0.13031006, 0.17478943, 0.95648193, 0.99624634
dd   0.99624634,-0.95648193,-0.17478943,-0.13031006
dd   0.15011597,-0.14625549,-0.00439453, 0.00282288
dd   0.02493286,-0.02285767, 0.00193787, 0.00248718
dd   0.00169373,-0.00148010,-0.00009155,-0.00007629

SYN_F_WINDOW_SHORT:
dd -0.0000000000, -0.0022294761,  0.0070019178,  0.0239159092,
dd  0.0310762096,  0.0006843590,  0.0786295608,  0.1484193951,
dd  0.1003170982,  0.1522008926,  0.5720440745,  0.9768452048,
dd  1.1449840069, -0.9768452048, -0.5720440745, -0.1522008926,
dd  0.1003170982, -0.1484193951, -0.0786295608, -0.0006843590,
dd  0.0310762096, -0.0239159092, -0.0070019178,  0.0022294761,

dd  0.0033227950,  0.0061257849,  0.0079142749,  0.0314808004,
dd  0.0305130407,  0.0730657727,  0.0841832235,  0.1088624969,
dd  0.0909292325,  0.5438203216,  0.6002249122,  1.1442910433,
dd  1.1442910433, -0.6002249122, -0.5438203216,  0.0909292325,
dd  0.1088624969, -0.0841832235, -0.0730657727,  0.0305130407,
dd  0.0314808004, -0.0079142749, -0.0061257849,  0.0033227950,

dd  0.0033810670,  0.0052873888,  0.0088609839,  0.0317342617,
dd  0.0297841094,  0.0675140470,  0.0897035599,  0.1165779009,
dd  0.0806888789,  0.5156124234,  0.6283028126,  1.1422129869,
dd  1.1422129869, -0.6283028126, -0.5156124234,  0.0806888789,
dd  0.1165779009, -0.0897035599, -0.0675140470,  0.0297841094,
dd  0.0317342617, -0.0088609839, -0.0052873888,  0.0033810670,

dd  0.0034272340,  0.0044880132,  0.0098399213,  0.0318443291,
dd  0.0288826302,  0.0619953983,  0.0951662809,  0.1234773025, 
dd  0.0695880502,  0.4874784052,  0.6562176943,  1.1387569904,
dd  1.1387569904, -0.6562176943, -0.4874784052,  0.0695880502,
dd  0.1234773025, -0.0951662809, -0.0619953983,  0.0288826302,
dd  0.0318443291, -0.0098399213, -0.0044880132,  0.0034272340,

dd  0.0034598880,  0.0037286030,  0.0108485902,  0.0318188891,
dd  0.0278021507,  0.0565297604,  0.1005462036,  0.1295771003,
dd  0.0576211400,  0.4594751894,  0.6839085817,  1.1339299679,
dd  1.1339299679, -0.6839085817, -0.4594751894,  0.0576211400,
dd  0.1295771003, -0.1005462036, -0.0565297604,  0.0278021507,
dd  0.0318188891, -0.0108485902, -0.0037286030,  0.0034598880,

dd  0.0034775150,  0.0030099021,  0.0118842097,  0.0316660590,
dd  0.0265367609,  0.0511358418,  0.1058171019,  0.1348952055,
dd  0.0447848216,  0.4316585064,  0.7113146782,  1.1277459860,
dd  1.1277459860, -0.7113146782, -0.4316585064,  0.0447848216,
dd  0.1348952055, -0.1058171019, -0.0511358418,  0.0265367609,
dd  0.0316660590, -0.0118842097, -0.0030099021,  0.0034775150,

dd  0.0034786200,  0.0023323391,  0.0129436301,  0.0313939899,
dd  0.0250809509,  0.0458312109,  0.1109521016,  0.1394512951,
dd  0.0310782604,  0.4040825069,  0.7383748293,  1.1202180386,
dd  1.1202180386, -0.7383748293, -0.4040825069,  0.0310782604,
dd  0.1394512951, -0.1109521016, -0.0458312109,  0.0250809509,
dd  0.0313939899, -0.0129436301, -0.0023323391,  0.0034786200,

dd  0.0034616119,  0.0016961680,  0.0140234102,  0.0310109705,
dd  0.0234298706,  0.0406321697,  0.1159232035,  0.1432666034,
dd  0.0165030695,  0.3767997921,  0.7650279999,  1.1113669872,
dd  1.1113669872, -0.7650279999, -0.3767997921,  0.0165030695,
dd  0.1432666034, -0.1159232035, -0.0406321697,  0.0234298706,
dd  0.0310109705, -0.0140234102, -0.0016961680,  0.0034616119,

dd  0.0034249341,  0.0011013590,  0.0151197100,  0.0305252392,
dd  0.0215791892,  0.0355538614,  0.1207021028,  0.1463640034,
dd  0.0010634609,  0.3498612940,  0.7912135720,  1.1012150049,
dd  1.1012150049, -0.7912135720, -0.3498612940,  0.0010634609,
dd  0.1463640034, -0.1207021028, -0.0355538614,  0.0215791892,
dd  0.0305252392, -0.0151197100, -0.0011013590,  0.0034249341,

dd  0.0033669460,  0.0005477320,  0.0162284095,  0.0299451109,
dd  0.0195253193,  0.0306101404,  0.1252595931,  0.1487675011,
dd -0.0152338203,  0.3233161867,  0.8168715835,  1.0897879601,
dd  1.0897879601, -0.8168715835, -0.3233161867, -0.0152338203,
dd  0.1487675011, -0.1252595931, -0.0306101404,  0.0195253193,
dd  0.0299451109, -0.0162284095, -0.0005477320,  0.0033669460,

dd  0.0032860560,  0.0000348550,  0.0173449796,  0.0292787701,
dd  0.0172653105,  0.0258137006,  0.1295658946,  0.1505026072,
dd -0.0323793218,  0.2972115874,  0.8419424891,  1.0771130323,
dd  1.0771130323, -0.8419424891, -0.2972115874, -0.0323793218,
dd  0.1505026072, -0.1295658946, -0.0258137006,  0.0172653105,
dd  0.0292787701, -0.0173449796, -0.0000348550,  0.0032860560,

dd  0.0031806100, -0.0004378230,  0.0184646100,  0.0285343695,
dd  0.0147970403,  0.0211759601,  0.1335908026,  0.1515956074,
dd -0.0503610000,  0.2715924978,  0.8663678169,  1.0632220507,
dd  1.0632220507, -0.8663678169, -0.2715924978, -0.0503610000,
dd  0.1515956074, -0.1335908026, -0.0211759601,  0.0147970403,
dd  0.0285343695, -0.0184646100,  0.0004378230,  0.0031806100,

dd  0.0030490190, -0.0008710770,  0.0195821002,  0.0277198907,
dd  0.0121191498,  0.0167071596,  0.1373039037,  0.1520740986,
dd -0.0691640675,  0.2465019971,  0.8900899291,  1.0481510162,
dd  1.0481510162, -0.8900899291, -0.2465019971, -0.0691640675,
dd  0.1520740986, -0.1373039037, -0.0167071596,  0.0121191498,
dd  0.0277198907, -0.0195821002,  0.0008710770,  0.0030490190,

dd  0.0028896530, -0.0012657720,  0.0206919704,  0.0268432405,
dd  0.0092311660,  0.0124163199,  0.1406743973,  0.1519663036,
dd -0.0887710974,  0.2219804972,  0.9130526781,  1.0319360495,
dd  1.0319360495, -0.9130526781, -0.2219804972, -0.0887710974,
dd  0.1519663036, -0.1406743973, -0.0124163199,  0.0092311660,
dd  0.0268432405, -0.0206919704,  0.0012657720,  0.0028896530,

dd  0.0027009640, -0.0016229640,  0.0217883606,  0.0259120706,
dd  0.0061334972,  0.0083113024,  0.1436710954,  0.1513013989,
dd -0.1091618985,  0.1980662942,  0.9352014065,  1.0146180391,
dd  1.0146180391, -0.9352014065, -0.1980662942, -0.1091618985,
dd  0.1513013989, -0.1436710954, -0.0083113024,  0.0061334972,
dd  0.0259120706, -0.0217883606,  0.0016229640,  0.0027009640,

dd  0.0024813900, -0.0019437710,  0.0228651706,  0.0249338895,
dd  0.0028275431,  0.0043987860,  0.1462630928,  0.1501089931,
dd -0.1303136945,  0.1747952998,  0.9564827085,  0.9962394834,
dd  0.9962394834, -0.9564827085, -0.1747952998, -0.1303136945,
dd  0.1501089931, -0.1462630928, -0.0043987860,  0.0028275431,
dd  0.0249338895, -0.0228651706,  0.0019437710,  0.0024813900





struc cost32_stack
.t0_0	resd 1
.t0_1	resd 1
.t0_2	resd 1
.t0_3	resd 1
.t0_4	resd 1
.t0_5	resd 1
.t0_6	resd 1
.t0_7	resd 1
.t0_8	resd 1
.t0_9	resd 1
.t0_10	resd 1
.t0_11	resd 1
.t0_12	resd 1
.t0_13	resd 1
.t0_14	resd 1
.t0_15	resd 1
.t1_0	resd 1
.t1_1	resd 1
.t1_2	resd 1
.t1_3	resd 1
.t1_4	resd 1
.t1_5	resd 1
.t1_6	resd 1
.t1_7	resd 1

.r0_0	resd 1
.r0_1	resd 1
.r0_2	resd 1
.r0_3	resd 1
.r0_4	resd 1
.r0_5	resd 1
.r0_6	resd 1
.r0_7	resd 1
.r0_8	resd 1
.r0_9	resd 1
.r0_10	resd 1
.r0_11	resd 1
.r0_12	resd 1
.r0_13	resd 1
.r0_14	resd 1
.r0_15	resd 1
.r1_0	resd 1
.r1_1	resd 1
.r1_2	resd 1
.r1_3	resd 1
.r1_4	resd 1
.r1_5	resd 1
.r1_6	resd 1
.r1_7	resd 1
.end
endstruc

%if _USE_3DNOW


SECTION .data progbits alloc noexec write align=16

ALIGN 8
cst_c4:	dd 0.70710678118655, 0

cst_c3:	dd 0.54119610014620, 0
		dd 1.30656296487638, 0

cst_c2:	dd 0.50979557910416, 0
		dd 0.60134488693505, 0
		dd 0.89997622313642, 0
		dd 2.56291544774151, 0

cst_c1: dd 0.50241928618816, 0
		dd 0.52249861493969, 0
		dd 0.56694403481636, 0
		dd 0.64682178335999, 0
		dd 0.78815462345125, 0
		dd 1.06067768599035, 0
		dd 1.72244709823833, 0
		dd 5.10114861868916, 0

cst_c0:	dd 0.50060299823520, 0
		dd 0.50547095989754, 0
		dd 0.51544730992262, 0
		dd 0.53104259108978, 0
		dd 0.55310389603444, 0
		dd 0.58293496820613, 0
		dd 0.62250412303566, 0
		dd 0.67480834145501, 0
		dd 0.74453627100230, 0
		dd 0.83934964541553, 0
		dd 0.97256823786196, 0
		dd 1.16943993343288, 0
		dd 1.48416461631417, 0
		dd 2.05778100995341, 0
		dd 3.40760841846872, 0
		dd 10.19000812354803, 0



SECTION .test

ALIGN 16
GLOBAL CPolyphaseASM___cost32
CPolyphaseASM___cost32:
	mov ecx, [esp+4]		; vec
	mov edx, [esp+8]		; f_vec
	sub esp, cost32_stack.end

%if 0
pusha
push dword 0
call debugger
pop eax
popa
%endif

		movd mm0, [ecx + 4 * 0]
		movd mm7, [ecx + 4 * 31]
		pfadd mm0, mm7				; t0_0
		movd mm1, [ecx + 4 * 15]
		movd mm7, [ecx + 4 * 16]
		pfadd mm1, mm7				; t0_15
		movd [esp + cost32_stack.t0_0], mm0
		movd mm2, [ecx + 4 * 7]
		movd mm7, [ecx + 4 * 24]
		pfadd mm2, mm7				; t0_7
		movd [esp + cost32_stack.t0_15], mm1
		pfadd mm0, mm1
		movd mm3, [ecx + 4 * 8]
		movd mm7, [ecx + 4 * 23]
		pfadd mm3, mm7				; t0_8
		movd [esp + cost32_stack.t0_7], mm2
		movd [esp + cost32_stack.t0_8], mm3
		pfadd mm2, mm3
		movd [esp + cost32_stack.t1_0], mm0
		movd [esp + cost32_stack.t1_7], mm2
		pfadd mm0, mm2							; mm0 = t2_0
	
		movd mm1, [ecx + 4 * 1]
		movd mm7, [ecx + 4 * 30]
		pfadd mm1, mm7				; t0_1
		movd mm2, [ecx + 4 * 14]
		movd mm7, [ecx + 4 * 17]
		pfadd mm2, mm7				; t0_14
		movd [esp + cost32_stack.t0_1], mm1
		movd mm3, [ecx + 4 * 6]
		movd mm7, [ecx + 4 * 25]
		pfadd mm3, mm7				; t0_6
		movd [esp + cost32_stack.t0_14], mm2
		pfadd mm1, mm2
		movd mm4, [ecx + 4 * 9]
		movd mm7, [ecx + 4 * 22]
		pfadd mm4, mm7				; t0_9
		movd [esp + cost32_stack.t0_6], mm3
		movd [esp + cost32_stack.t0_9], mm4
		pfadd mm3, mm4
		movd [esp + cost32_stack.t1_1], mm1
		movd [esp + cost32_stack.t1_6], mm3
		pfadd mm1, mm3							; mm1 = t2_1
	
		movd mm2, [ecx + 4 * 2]
		movd mm7, [ecx + 4 * 29]
		pfadd mm2, mm7				; t0_2
		movd mm3, [ecx + 4 * 13]
		movd mm7, [ecx + 4 * 18]
		pfadd mm3, mm7				; t0_13
		movd [esp + cost32_stack.t0_2], mm2
		movd mm4, [ecx + 4 * 5]
		movd mm7, [ecx + 4 * 26]
		pfadd mm4, mm7				; t0_5
		movd [esp + cost32_stack.t0_13], mm3
		pfadd mm2, mm3
		movd mm5, [ecx + 4 * 10]
		movd mm7, [ecx + 4 * 21]
		pfadd mm5, mm7				; t0_10
		movd [esp + cost32_stack.t0_5], mm4
		movd [esp + cost32_stack.t0_10], mm5
		pfadd mm4, mm5
		movd [esp + cost32_stack.t1_2], mm2
		movd [esp + cost32_stack.t1_5], mm4
		pfadd mm2, mm4							; mm2 = t2_2
	
		movd mm3, [ecx + 4 * 3]
		movd mm7, [ecx + 4 * 28]
		pfadd mm3, mm7				; t0_3
		movd mm4, [ecx + 4 * 12]
		movd mm7, [ecx + 4 * 19]
		pfadd mm4, mm7				; t0_12
		movd [esp + cost32_stack.t0_3], mm3
		movd mm5, [ecx + 4 * 4]
		movd mm7, [ecx + 4 * 27]
		pfadd mm5, mm7				; t0_4
		movd [esp + cost32_stack.t0_12], mm4
		pfadd mm3, mm4
		movd mm6, [ecx + 4 * 11]
		movd mm7, [ecx + 4 * 20]
		pfadd mm6, mm7				; t0_11
		movd [esp + cost32_stack.t0_4], mm5
		movd [esp + cost32_stack.t0_11], mm6
		pfadd mm5, mm6
		movd [esp + cost32_stack.t1_3], mm3
		movd [esp + cost32_stack.t1_4], mm5
		pfadd mm3, mm5							; mm3 = t3_3
		
		
		movq mm4, mm0
		pfadd mm4, mm3	; t3_0
		movq mm5, mm1
		pfadd mm5, mm2	; t3_1
		
		movq mm6, mm4
		pfadd mm6, mm5
		movd [edx + 4 * 0], mm6					; f_vec[0]
		pfsub mm4, mm5
		pfmul mm4, [cst_c4]
		movd [edx + 4 * 16], mm4				; f_vec[16]
		
		movq mm4, mm0
		pfsub mm4, mm3
		movq mm5, mm1
		pfsub mm5, mm2
		pfmul mm4, [cst_c3]
		pfmul mm5, [cst_c3 +8]
		
		movq mm6, mm4
		pfsub mm4, mm5
		pfadd mm6, mm5					; res3_0
		pfmul mm4, [cst_c4]			; res3_1
		pfadd mm6, mm4
		movd [edx + 4 * 24], mm4				; f_vec[24]
		movd [edx + 4 * 8], mm6					; f_vec[8]
		
		
		movd mm0, [esp + cost32_stack.t1_0]
		movd mm7, [esp + cost32_stack.t1_7]
		pfsub mm0, mm7
		pfmul mm0, [cst_c2]
		movd mm1, [esp + cost32_stack.t1_1]
		movd mm7, [esp + cost32_stack.t1_6]
		pfsub mm1, mm7
		pfmul mm1, [cst_c2 +8]
		movd mm2, [esp + cost32_stack.t1_2]
		movd mm7, [esp + cost32_stack.t1_5]
		pfsub mm2, mm7
		pfmul mm2, [cst_c2 +16]
		movd mm3, [esp + cost32_stack.t1_3]
		movd mm7, [esp + cost32_stack.t1_4]
		pfsub mm3, mm7
		pfmul mm3, [cst_c2 +24]
		
		movq mm4, mm0
		pfadd mm4, mm3
		movq mm5, mm1
		pfadd mm5, mm2
		
		movq mm6, mm4
		pfsub mm4, mm5
		pfadd mm6, mm5			; res2_0
		pfmul mm4, [cst_c4]	; res2_2
		
		pfsub mm0, mm3
		pfsub mm1, mm2
		pfmul mm0, [cst_c3]	; t3_0
		pfmul mm1, [cst_c3 +8]	; t3_1
		
		movq mm2, mm0
		pfsub mm0, mm1
		pfadd mm2, mm1			; mm2 = r3_0
		pfmul mm0, [cst_c4]	; mm0 = r3_1
		pfadd mm2, mm0			; mm2 = r2_1
		
		movd [edx + 4 * 28], mm0					; f_vec[28]
		pfadd mm0, mm4
		movd [edx + 4 * 20], mm0					; f_vec[20]
		pfadd mm4, mm2
		movd [edx + 4 * 4], mm4						; f_vec[4]
		pfadd mm6, mm2
		movd [edx + 4 * 12], mm6					; f_vec[12]
		

		movd mm0, [esp + cost32_stack.t0_0]
		movd mm7, [esp + cost32_stack.t0_15]
		pfsub mm0, mm7
		pfmul mm0, [cst_c1 +8 * 0]
		movd mm1, [esp + cost32_stack.t0_7]
		movd mm7, [esp + cost32_stack.t0_8]
		pfsub mm1, mm7
		pfmul mm1, [cst_c1 +8 * 7]
		movd [esp + cost32_stack.t1_0], mm0
		pfadd mm0, mm1
		movd [esp + cost32_stack.t1_7], mm1
	
		movd mm1, [esp + cost32_stack.t0_1]
		movd mm7, [esp + cost32_stack.t0_14]
		pfsub mm1, mm7
		pfmul mm1, [cst_c1 +8 * 1]
		movd mm2, [esp + cost32_stack.t0_6]
		movd mm7, [esp + cost32_stack.t0_9]
		pfsub mm2, mm7
		pfmul mm2, [cst_c1 +8 * 6]
		movd [esp + cost32_stack.t1_1], mm1
		pfadd mm1, mm2
		movd [esp + cost32_stack.t1_6], mm2
	
		movd mm2, [esp + cost32_stack.t0_2]
		movd mm7, [esp + cost32_stack.t0_13]
		pfsub mm2, mm7
		pfmul mm2, [cst_c1 +8 * 2]
		movd mm3, [esp + cost32_stack.t0_5]
		movd mm7, [esp + cost32_stack.t0_10]
		pfsub mm3, mm7
		pfmul mm3, [cst_c1 +8 * 5]
		movd [esp + cost32_stack.t1_2], mm2
		pfadd mm2, mm3
		movd [esp + cost32_stack.t1_5], mm3
	
		movd mm3, [esp + cost32_stack.t0_3]
		movd mm7, [esp + cost32_stack.t0_12]
		pfsub mm3, mm7
		pfmul mm3, [cst_c1 +8 * 3]
		movd mm4, [esp + cost32_stack.t0_4]
		movd mm7, [esp + cost32_stack.t0_11]
		pfsub mm4, mm7
		pfmul mm4, [cst_c1 +8 * 4]
		movd [esp + cost32_stack.t1_3], mm3
		pfadd mm3, mm4
		movd [esp + cost32_stack.t1_4], mm4

		movq mm4, mm0
		movq mm5, mm1
		pfadd mm4, mm3			; t3_0
		pfadd mm5, mm2			; t3_1
		movq mm6, mm4
		pfsub mm4, mm5
		pfadd mm6, mm5
		pfmul mm4, [cst_c4]
		movd [esp + cost32_stack.r1_0], mm6	
		movd [esp + cost32_stack.r1_4], mm4	
		
		movq mm4, mm0
		movq mm5, mm1
		pfsub mm4, mm3
		pfsub mm5, mm2
		pfmul mm4, [cst_c3]
		pfmul mm5, [cst_c3 +8]
		movq mm6, mm4
		pfsub mm4, mm5
		pfadd mm6, mm5
		pfmul mm4, [cst_c4]
		pfadd mm6, mm4
		movd [esp + cost32_stack.r1_6], mm4
		movd [esp + cost32_stack.r1_2], mm6

		movd mm0, [esp + cost32_stack.t1_0]	
		movd mm7, [esp + cost32_stack.t1_7]
		pfsub mm0, mm7
		pfmul mm0, [cst_c2]
		movd mm1, [esp + cost32_stack.t1_1]	
		movd mm7, [esp + cost32_stack.t1_6]
		pfsub mm1, mm7
		pfmul mm1, [cst_c2 +8]
		movd mm2, [esp + cost32_stack.t1_2]	
		movd mm7, [esp + cost32_stack.t1_5]
		pfsub mm2, mm7
		pfmul mm2, [cst_c2 +16]
		movd mm3, [esp + cost32_stack.t1_3]	
		movd mm7, [esp + cost32_stack.t1_4]
		pfsub mm3, mm7
		pfmul mm3, [cst_c2 +24]
		movq mm4, mm0
		movq mm5, mm1
		pfadd mm4, mm3		; t3_0
		pfadd mm5, mm2		; t3_1

		movq mm6, mm4
		pfsub mm4, mm5
		pfadd mm6, mm5		; r2_0
		pfmul mm4, [cst_c4]		; r2_2
		
		pfsub mm0, mm3
		pfsub mm1, mm2
		pfmul mm0, [cst_c3]
		pfmul mm1, [cst_c3 +8]
		
		movq mm2, mm0
		pfsub mm0, mm1
		pfadd mm2, mm1
		pfmul mm0, [cst_c4]	; r2_3, r1_7
		pfadd mm2, mm0		; r2_1
	
		pfadd mm6, mm2		; r1_1
		pfadd mm2, mm4		; r1_3
		pfadd mm4, mm0		; r1_5
	
		movd mm7, [esp + cost32_stack.r1_6]
		movd mm5, [esp + cost32_stack.r1_4]
		movd mm3, [esp + cost32_stack.r1_2]
		movd mm1, [esp + cost32_stack.r1_0]

		movd [edx + 4 * 30], mm0					; f_vec[30]
		pfadd mm0, mm7
		movd [edx + 4 * 26], mm0					; f_vec[26]
		pfadd mm7, mm4
		movd [edx + 4 * 22], mm7					; f_vec[22]
		pfadd mm4, mm5
		movd [edx + 4 * 18], mm4					; f_vec[18]
		pfadd mm5, mm2
		movd [edx + 4 * 2], mm5						; f_vec[2]
		pfadd mm2, mm3
		movd [edx + 4 * 6], mm2						; f_vec[6]
		pfadd mm3, mm6
		movd [edx + 4 * 10], mm3					; f_vec[10]
		pfadd mm6, mm1
		movd [edx + 4 * 14], mm6					; f_vec[14]
	
	
	;---------------------------------------------------------------------------
	
	
		movd mm0, [ecx + 4 * 0]
		movd mm7, [ecx + 4 * 31]
		pfsub mm0, mm7
		pfmul mm0, [cst_c0 +8* 0]				; t0_0
		movd mm1, [ecx + 4 * 15]
		movd mm7, [ecx + 4 * 16]
		movd [esp + cost32_stack.t0_0], mm0
		pfsub mm1, mm7
		pfmul mm1, [cst_c0 +8* 15]				; t0_15
		movd mm2, [ecx + 4 * 7]
		movd mm7, [ecx + 4 * 24]
		movd [esp + cost32_stack.t0_15], mm1
		pfsub mm2, mm7
		pfmul mm2, [cst_c0 +8* 7]				; t0_7
		movd mm3, [ecx + 4 * 8]
		movd mm7, [ecx + 4 * 23]
		movd [esp + cost32_stack.t0_7], mm2
		pfsub mm3, mm7
		pfmul mm3, [cst_c0 +8* 8]				; t0_8
		pfadd mm0, mm1
		pfadd mm2, mm3
		movd [esp + cost32_stack.t0_8], mm3
		movd [esp + cost32_stack.t1_0], mm0
		movd [esp + cost32_stack.t1_7], mm2
		pfadd mm0, mm2							; mm0 = t2_0

	
		movd mm1, [ecx + 4 * 1]
		movd mm7, [ecx + 4 * 30]
		pfsub mm1, mm7
		pfmul mm1, [cst_c0 +8* 1]				; t0_1
		movd mm2, [ecx + 4 * 14]
		movd mm7, [ecx + 4 * 17]
		movd [esp + cost32_stack.t0_1], mm1
		pfsub mm2, mm7
		pfmul mm2, [cst_c0 +8* 14]				; t0_14
		movd mm3, [ecx + 4 * 6]
		movd mm7, [ecx + 4 * 25]
		movd [esp + cost32_stack.t0_14], mm2
		pfsub mm3, mm7
		pfmul mm3, [cst_c0 +8* 6]				; t0_6
		movd mm4, [ecx + 4 * 9]
		movd mm7, [ecx + 4 * 22]
		movd [esp + cost32_stack.t0_6], mm3
		pfsub mm4, mm7
		pfmul mm4, [cst_c0 +8* 9]				; t0_9
		pfadd mm1, mm2
		pfadd mm3, mm4
		movd [esp + cost32_stack.t0_9], mm4
		movd [esp + cost32_stack.t1_1], mm1
		movd [esp + cost32_stack.t1_6], mm3
		pfadd mm1, mm3							; mm0 = t2_0

		movd mm2, [ecx + 4 * 2]
		movd mm7, [ecx + 4 * 29]
		pfsub mm2, mm7
		pfmul mm2, [cst_c0 +8* 2]				; t0_2
		movd mm3, [ecx + 4 * 13]
		movd mm7, [ecx + 4 * 18]
		movd [esp + cost32_stack.t0_2], mm2
		pfsub mm3, mm7
		pfmul mm3, [cst_c0 +8* 13]				; t0_13
		movd mm4, [ecx + 4 * 5]
		movd mm7, [ecx + 4 * 26]
		movd [esp + cost32_stack.t0_13], mm3
		pfsub mm4, mm7
		pfmul mm4, [cst_c0 +8* 5]				; t0_5
		movd mm5, [ecx + 4 * 10]
		movd mm7, [ecx + 4 * 21]
		movd [esp + cost32_stack.t0_5], mm4
		pfsub mm5, mm7
		pfmul mm5, [cst_c0 +8* 10]				; t0_10
		pfadd mm2, mm3
		pfadd mm4, mm5
		movd [esp + cost32_stack.t0_10], mm5
		movd [esp + cost32_stack.t1_2], mm2
		movd [esp + cost32_stack.t1_5], mm4
		pfadd mm2, mm4							; mm0 = t2_2
	
		movd mm3, [ecx + 4 * 3]
		movd mm7, [ecx + 4 * 28]
		pfsub mm3, mm7
		pfmul mm3, [cst_c0 +8* 3]				; t0_3
		movd mm4, [ecx + 4 * 12]
		movd mm7, [ecx + 4 * 19]
		movd [esp + cost32_stack.t0_3], mm3
		pfsub mm4, mm7
		pfmul mm4, [cst_c0 +8* 12]				; t0_12
		movd mm5, [ecx + 4 * 4]
		movd mm7, [ecx + 4 * 27]
		movd [esp + cost32_stack.t0_12], mm4
		pfsub mm5, mm7
		pfmul mm5, [cst_c0 +8* 4]				; t0_6
		movd mm6, [ecx + 4 * 11]
		movd mm7, [ecx + 4 * 20]
		movd [esp + cost32_stack.t0_4], mm5
		pfsub mm6, mm7
		pfmul mm6, [cst_c0 +8* 11]				; t0_11
		pfadd mm3, mm4
		pfadd mm5, mm6
		movd [esp + cost32_stack.t0_11], mm6
		movd [esp + cost32_stack.t1_3], mm3
		movd [esp + cost32_stack.t1_4], mm5
		pfadd mm3, mm5							; mm0 = t2_3
	
		movq mm4, mm0
		movq mm5, mm1
		pfadd mm4, mm3		; t3_0
		pfadd mm5, mm2		; t3_1
	
	;;;;
	
		movq mm6, mm4
		pfsub mm4, mm5
		pfadd mm6, mm5
		pfmul mm4, [cst_c4]
		movd [esp + cost32_stack.r0_0], mm6
		movd [esp + cost32_stack.r0_8], mm4
	
		pfsub mm0, mm3
		pfsub mm1, mm2
		pfmul mm0, [cst_c3]
		pfmul mm1, [cst_c3 +8]
		movq mm2, mm0
		pfsub mm0, mm1
		pfadd mm2, mm1
		pfmul mm0, [cst_c4]
		pfadd mm2, mm0
		movd [esp + cost32_stack.r0_12], mm0
		movd [esp + cost32_stack.r0_4], mm2
		
		movd mm0, [esp + cost32_stack.t1_0]
		movd mm7, [esp + cost32_stack.t1_7]
		pfsub mm0, mm7
		pfmul mm0, [cst_c2]
		movd mm1, [esp + cost32_stack.t1_1]
		movd mm7, [esp + cost32_stack.t1_6]
		pfsub mm1, mm7
		pfmul mm1, [cst_c2 +8]
		movd mm2, [esp + cost32_stack.t1_2]
		movd mm7, [esp + cost32_stack.t1_5]
		pfsub mm2, mm7
		pfmul mm2, [cst_c2 +16]
		movd mm3, [esp + cost32_stack.t1_3]
		movd mm7, [esp + cost32_stack.t1_4]
		pfsub mm3, mm7
		pfmul mm3, [cst_c2 +24]
		
		movq mm4, mm0
		movq mm5, mm1
		pfadd mm4, mm3
		pfadd mm5, mm2
		movq mm6, mm4
		pfsub mm4, mm5
		pfadd mm6, mm5			; r2_0
		pfmul mm4, [cst_c4]	; r2_2
		
		pfsub mm0, mm3
		pfsub mm1, mm2
		pfmul mm0, [cst_c3]		; t3_0
		pfmul mm1, [cst_c3+8]	; t3_1
		movq mm2, mm0
		pfsub mm0, mm1
		pfadd mm2, mm1			; r3_0
		pfmul mm0, [cst_c4]	; r3_1, r2_3
		pfadd mm2, mm0			; r2_1		
		
		movd [esp + cost32_stack.r0_14], mm0
		pfadd mm0, mm4
		movd [esp + cost32_stack.r0_10], mm0
		pfadd mm4, mm2
		movd [esp + cost32_stack.r0_6], mm4
		pfadd mm2, mm6
		movd [esp + cost32_stack.r0_2], mm2
		
		;;;;;;
		
		movd mm0, [esp + cost32_stack.t0_0]
		movd mm7, [esp + cost32_stack.t0_15]
		pfsub mm0, mm7
		pfmul mm0, [cst_c1 +8 * 0]
		movd mm6, [esp + cost32_stack.t0_7]
		movd mm7, [esp + cost32_stack.t0_8]
		movd [esp + cost32_stack.t1_0], mm0
		pfsub mm6, mm7
		pfmul mm6, [cst_c1 +8 * 7]

		movd mm1, [esp + cost32_stack.t0_1]
		movd mm7, [esp + cost32_stack.t0_14]
		movd [esp + cost32_stack.t1_7], mm6
		pfadd mm0, mm6		; t2_0
		pfsub mm1, mm7
		pfmul mm1, [cst_c1 +8 * 1]
		movd mm6, [esp + cost32_stack.t0_6]
		movd mm7, [esp + cost32_stack.t0_9]
		movd [esp + cost32_stack.t1_1], mm1
		pfsub mm6, mm7
		pfmul mm6, [cst_c1 +8 * 6]


		pfadd mm1, mm6
		movd [esp + cost32_stack.t1_6], mm6

		movd mm2, [esp + cost32_stack.t0_2]
		movd mm7, [esp + cost32_stack.t0_13]
		pfsub mm2, mm7
		pfmul mm2, [cst_c1 +8 * 2]
		movd mm6, [esp + cost32_stack.t0_5]
		movd mm7, [esp + cost32_stack.t0_10]
		movd [esp + cost32_stack.t1_2], mm2
		pfsub mm6, mm7
		pfmul mm6, [cst_c1 +8 * 5]
		pfadd mm2, mm6
		movd [esp + cost32_stack.t1_5], mm6
		
		movd mm3, [esp + cost32_stack.t0_3]
		movd mm7, [esp + cost32_stack.t0_12]
		pfsub mm3, mm7
		pfmul mm3, [cst_c1 +8 * 3]
		movd mm6, [esp + cost32_stack.t0_4]
		movd mm7, [esp + cost32_stack.t0_11]
		movd [esp + cost32_stack.t1_3], mm3
		pfsub mm6, mm7
		pfmul mm6, [cst_c1 +8 * 4]
		pfadd mm3, mm6
		movd [esp + cost32_stack.t1_4], mm6
		
		movq mm4, mm0
		movq mm5, mm1
		pfadd mm4, mm3
		pfadd mm5, mm2
	

		movq mm6, mm4
		pfsub mm4, mm5
		pfadd mm6, mm5
		pfmul mm4, [cst_c4]
		movd [esp + cost32_stack.r1_0], mm6
		movd [esp + cost32_stack.r1_4], mm4
	
	
		pfsub mm0, mm3
		pfsub mm1, mm2
		pfmul mm0, [cst_c3]
		pfmul mm1, [cst_c3 +8]
		movq mm2, mm0
		pfsub mm0, mm1
		pfadd mm2, mm1
		pfmul mm0, [cst_c4]
		pfadd mm2, mm0
		movd [esp + cost32_stack.r1_6], mm0
		movd [esp + cost32_stack.r1_2], mm2
		
		
		movd mm0, [esp + cost32_stack.t1_0]
		movd mm7, [esp + cost32_stack.t1_7]
		pfsub mm0, mm7
		pfmul mm0, [cst_c2]
		
		movd mm1, [esp + cost32_stack.t1_1]
		movd mm7, [esp + cost32_stack.t1_6]
		pfsub mm1, mm7
		pfmul mm1, [cst_c2 +8]
		
		movd mm2, [esp + cost32_stack.t1_2]
		movd mm7, [esp + cost32_stack.t1_5]
		pfsub mm2, mm7
		pfmul mm2, [cst_c2 +16]
		
		movd mm3, [esp + cost32_stack.t1_3]
		movd mm7, [esp + cost32_stack.t1_4]
		pfsub mm3, mm7
		pfmul mm3, [cst_c2 +24]
		
		movq mm4, mm0
		movq mm5, mm1
		pfadd mm4, mm3
		pfadd mm5, mm2
		movq mm6, mm4
		pfsub mm4, mm5
		pfadd mm6, mm5			; r2_0 
		pfmul mm4, [cst_c4]		; r2_2
		

		pfsub mm0, mm3
		pfsub mm1, mm2
		pfmul mm0, [cst_c3]
		pfmul mm1, [cst_c3 +8]
		movq mm2, mm0
		pfsub mm0, mm1
		pfadd mm2, mm1			; r3_0
		pfmul mm0, [cst_c4]		; r3_1, r2_3, r1_7
		pfadd mm2, mm0			; r2_1
		
		pfadd mm6, mm2			; r1_1
		pfadd mm2, mm4			; r1_3
		pfadd mm4, mm0			; r1_5
		
		
		movd mm1, [esp + cost32_stack.r1_0]
		movd mm3, [esp + cost32_stack.r1_2]
		movd mm5, [esp + cost32_stack.r1_4]
		movd mm7, [esp + cost32_stack.r1_6]

		pfadd mm1, mm6
		movd [esp + cost32_stack.r0_1], mm1
		pfadd mm6, mm3
		pfadd mm3, mm2
		pfadd mm2, mm5
		pfadd mm5, mm4
		pfadd mm4, mm7
		pfadd mm7, mm0
		
		movd mm1, [esp + cost32_stack.r0_14]
		movd [edx + 4 * 31], mm0					; f_vec[31]
		pfadd mm0, mm1
		movd [edx + 4 * 29], mm0					; f_vec[29]
		movd mm0, [esp + cost32_stack.r0_12]
		pfadd mm1, mm7
		movd [edx + 4 * 27], mm1					; f_vec[27]
		pfadd mm7, mm0
		movd [edx + 4 * 25], mm7					; f_vec[25]
		movd mm1, [esp + cost32_stack.r0_10]
		pfadd mm0, mm4
		movd [edx + 4 * 23], mm0					; f_vec[23]
		pfadd mm4, mm1
		movd [edx + 4 * 21], mm4					; f_vec[21]
		movd mm0, [esp + cost32_stack.r0_8]
		pfadd mm1, mm5
		movd [edx + 4 * 19], mm1					; f_vec[19]
		pfadd mm5, mm0
		movd [edx + 4 * 17], mm5					; f_vec[17]
		movd mm1, [esp + cost32_stack.r0_6]
		pfadd mm0, mm2
		movd [edx + 4 * 1], mm0						; f_vec[1]
		pfadd mm2, mm1
		movd [edx + 4 * 3], mm2						; f_vec[3]
		movd mm0, [esp + cost32_stack.r0_4]
		pfadd mm1, mm3
		movd [edx + 4 * 5], mm1						; f_vec[5]
		pfadd mm3, mm0
		movd [edx + 4 * 7], mm3						; f_vec[7]
		movd mm1, [esp + cost32_stack.r0_2]
		pfadd mm0, mm6
		movd [edx + 4 * 9], mm0						; f_vec[9]
		
		pfadd mm6, mm1
		movd [edx + 4 * 11], mm6					; f_vec[11]
		movd mm2, [esp + cost32_stack.r0_1]
		movd mm0, [esp + cost32_stack.r0_0]
		pfadd mm1, mm2
		movd [edx + 4 * 13], mm1					; f_vec[13]
		pfadd mm2, mm0
		movd [edx + 4 * 15], mm2					; f_vec[15]

		
	add esp, cost32_stack.end
	femms
	ret
	
	


%endif

	
	