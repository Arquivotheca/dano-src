%if _PROCESSOR_MMX_==1

				global add_block
				global copy_block

				SECTION .data
				
; void add_block (uint8 *ptr, const int16 *block, size_t stride);

add_block:
				push ebp
				mov ebp,esp
				
				push esi
				push edi
				push eax
				push ecx
				
				mov edi,[ebp+8]
				mov eax,[ebp+16]
				mov esi,[ebp+12]

				mov ecx,8

add_block_mmx_loop:
				pxor mm0,mm0
				movq mm1,[edi]
				movq mm2,mm1
				punpcklbw mm1,mm0
				punpckhbw mm2,mm0
				paddw mm1,[esi]
				paddw mm2,[esi+8]
				packuswb mm1,mm2
				movq [edi],mm1
				
				add esi,16
				add edi,eax
				
				loop add_block_mmx_loop
				
				pop ecx
				pop eax
				pop edi
				pop esi
				
				mov esp,ebp
				pop ebp
				
				ret

; void copy_block (uint8 *ptr, const int16 *block, size_t stride);

copy_block:
				push ebp
				mov ebp,esp
				
				push esi
				push edi
				push eax
				push ecx
				
				mov edi,[ebp+8]
				mov eax,[ebp+16]
				mov esi,[ebp+12]

				mov ecx,8

copy_block_mmx_loop:
				movq mm0,[esi]
				packuswb mm0,[esi+8]
				movq [edi],mm0
				
				add esi,16
				add edi,eax
				
				loop copy_block_mmx_loop
				
				pop ecx
				pop eax
				pop edi
				pop esi
				
				mov esp,ebp
				pop ebp
				
				ret

%endif
