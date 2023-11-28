			global mpeg2bits_get
			global mpeg2bits_peek
			global mpeg2bits_skip

			global mpeg2bits_save_state
			global mpeg2bits_restore_state
						
			extern mpeg2bits_refill_from_buffer
						
			section .data

_64_minus_index
			dd 64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,
			dd 44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,
			dd 24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1

			section .text

; uint32 CSmartBitStream::GetBits(uint32 count)

mpeg2bits_get:

			push ebp
			
			mov ebp,[esp+8]
			mov eax,[ebp+16]						; eax=mBitsLeft
			mov ecx,[esp+12]						; ecx=count
			sub eax,ecx
			jl new64bit
			movd mm3,[_64_minus_index+ecx*4]
			movq mm2,mm0
			movd mm1,ecx
			psrlq mm2,mm3
			mov [ebp+16],al
			movd eax,mm2
			psllq mm0,mm1
			
			pop ebp			
			ret

new64bit:
			movd mm3,[_64_minus_index+ecx*4]  			                                        ;(for shifting) 
			movq mm2,mm0
			mov edx,[ebp]		         
			mov ecx,[ebp+4]                     

			cmp edx,ecx                          
			jge do_refill
			
refill: 
			add edx,8                            
			add eax,64                           
			                                        
			mov [ebp],edx      			 
			mov ecx,[edx-8]                      
			mov edx,[edx-4]                     

			bswap edx                             
			bswap ecx                             
			movd mm4,ecx                
			psrlq mm2,mm3                       
			                                       
			                                     
			movd mm1,edx               
			psllq mm4,32                         
			                                       
			movd mm3,eax                     
			por mm4,mm1                        
			movq mm0,mm4                      
			psrlq mm4,mm3                       
			                                         
			mov [ebp+16],eax               
			por mm2,mm4                         
			                                        
			movd mm1,[_64_minus_index+4*eax]      
			movd eax,mm2                        
			psllq mm0,mm1                          
			
			pop ebp			
			ret 

do_refill: 
			push eax 
			push ebp 
			call mpeg2bits_refill_from_buffer
			pop	ebp
			mov edx,[ebp]
			pop eax 
			jmp refill 

; uint32 CSmartBitStream::PeekBits(uint32 count)

mpeg2bits_peek:

			push ebp
			
			mov ebp,[esp+8]
			mov eax,[ebp+16]						; eax=mBitsLeft
			mov ecx,[esp+12]						; ecx=count
			sub eax,ecx
			jl peek_new64bit
			movd mm3,[_64_minus_index+ecx*4]
			movq mm2,mm0
			movd mm1,ecx
			psrlq mm2,mm3
			movd eax,mm2
			
			pop ebp
			ret

peek_new64bit:
			movd mm3,[_64_minus_index+ecx*4]		; mm3 = 64-count			                                        ;(for shifting) 
			mov edx,[ebp]		         
			mov ecx,[ebp+4]                     

			cmp edx,ecx                          
			jge peek_do_refill

peek_refill: 
			add eax,32 								; eax = 32+mBitsLeft-count                          
			                                        
			mov ecx,[edx]                      
			bswap ecx
			
			movq mm2,mm0
			                      
			movd mm4,ecx                
			psrlq mm2,mm3							; mm0 = mCurrentWord>>(64-count)                       			                                       

			movd mm3,eax			
			psrlq mm4,mm3							                 

			por mm2,mm4								; mm0 = new mCurrentWord
			movd eax,mm2                        
			                                       
			pop ebp
			ret 

peek_do_refill: 
			push eax 
			push ebp 
			call mpeg2bits_refill_from_buffer
			pop	ebp
			mov edx,[ebp]
			pop eax 
			jmp peek_refill 


; uint32 CSmartBitStream::SkipBits(uint32 count)

mpeg2bits_skip:

			push ebp
			
			mov ebp,[esp+8]
			mov eax,[ebp+16]						; eax=mBitsLeft
			mov ecx,[esp+12]						; ecx=count
			sub eax,ecx
			jl skip_new64bit
			movd mm3,[_64_minus_index+ecx*4]
			movq mm2,mm0
			movd mm1,ecx
			psrlq mm2,mm3
			mov [ebp+16],al
			movd eax,mm2
			psllq mm0,mm1
			
			pop ebp
			ret

skip_new64bit:
			movd mm3,[_64_minus_index+ecx*4]  			                                        ;(for shifting) 
			movq mm2,mm0
			mov edx,[ebp]		         
			mov ecx,[ebp+4]                     

			cmp edx,ecx                          
			jge skip_do_refill
			
skip_refill: 
			add edx,8                            
			add eax,64                           
			                                        
			mov [ebp],edx      			 
			mov ecx,[edx-8]                      
			mov edx,[edx-4]                     

			bswap edx                             
			bswap ecx                             
			movd mm4,ecx                
			psrlq mm2,mm3                       
			                                       
			                                     
			movd mm1,edx               
			psllq mm4,32                         
			                                       
			movd mm3,eax                     
			por mm4,mm1                        
			movq mm0,mm4                      
			psrlq mm4,mm3                       
			                                         
			mov [ebp+16],eax               
			por mm2,mm4                         
			                                        
			movd mm1,[_64_minus_index+4*eax]      
			movd eax,mm2                        
			psllq mm0,mm1                          
			
			pop ebp
			ret 

skip_do_refill: 
			push eax 
			push ebp 
			call mpeg2bits_refill_from_buffer
			pop	ebp
			mov edx,[ebp]
			pop eax 
			jmp skip_refill 

; void CSmartBitStream::SaveInternalState()

mpeg2bits_save_state:
			mov edx,[esp+4]
			movq [edx+8],mm0
			emms
			ret
			
; void CSmartBitStream::RestoreInternalState()

mpeg2bits_restore_state:
			mov edx,[esp+4]
			movq mm0,[edx+8]
			ret
			