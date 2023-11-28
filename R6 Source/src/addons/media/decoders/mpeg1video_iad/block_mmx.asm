    GLOBAL  add_block_mmx
    GLOBAL  copy_block_mmx
    
	SECTION .data

const		DW 128, 128,128, 128  
un			DW 1,0,0,0
mask1 		DW 0x01010101,0x01010101,0x01010101,0x01010101
nonmask1 	DW 0xfefefefe,0xfefefefe,0xfefefefe,0xfefefefe    
    
    SECTION .text
    
add_block_mmx:
	PUSH		ebp
	MOV			ebp,esp
	
	
	PUSH 		esi
	MOV			esi,dword[ebp + 8]
	
	PUSH 		edi
	MOV			edi,dword[ebp + 12]
	
	PUSH 		ebx
	MOV			ebx,[ebp + 16]
	
	PUSH		ecx
	MOV			ecx,7
	
	PXOR		MM7,MM7

.loop
	MOVQ		MM0,qword[esi]
	MOVQ		MM2,qword[edi]
	
	MOVQ		MM3,qword[edi + 8]
	MOVQ		MM1,MM0
	
	
	PUNPCKLBW	MM0,MM7
	PUNPCKHBW	MM1,MM7
	
	PADDSW		MM0,MM2
	PADDSW		MM1,MM3
	
	PACKUSWB	MM0,MM1
	
	MOVQ		qword[esi],MM0
		
	JECXZ		.fin
	DEC			ecx
	ADD			edi,16
	ADD			esi,ebx
	JMP			.loop


.fin
	EMMS
	
	pop			ecx
	
	pop 		ebx
	
	pop			edi
	
	POP			esi
	
	MOV			esp,ebp
	POP 		ebp 		
	
	RET
    
copy_block_mmx:
	PUSH		ebp
	MOV			ebp,esp
	
	
	PUSH 		esi
	MOV			esi,dword[ebp + 8]
	
	PUSH 		edi
	MOV			edi,dword[ebp + 12]
	
	PUSH 		ecx
	MOV			ecx,[ebp + 16]
	
	
	
	;line 0
		
	PXOR		MM7,MM7
	MOVQ		MM2,qword[const]
	
	
	MOVQ		MM0,qword[edi]
	MOVQ		MM1,qword[edi + 8]
	
	PADDSW		MM0,MM2
	PADDSW		MM1,MM2
	
	PACKUSWB	MM0,MM1
	
	MOVQ		qword[esi],MM0
	
	;line 1
	ADD			esi,ecx
	ADD			edi,16
	
	
	
	MOVQ		MM0,qword[edi]
	MOVQ		MM1,qword[edi + 8]
	
	PADDSW		MM0,MM2
	PADDSW		MM1,MM2
	
	PACKUSWB	MM0,MM1
	
	MOVQ		qword[esi],MM0
	
	;line 2
	ADD			esi,ecx
	ADD			edi,16
	
	
	MOVQ		MM0,qword[edi]
	MOVQ		MM1,qword[edi + 8]
	
	PADDSW		MM0,MM2
	PADDSW		MM1,MM2
	
	PACKUSWB	MM0,MM1
	
	MOVQ		qword[esi],MM0
	
	;line 3
	ADD			esi,ecx
	ADD			edi,16
	
	
	MOVQ		MM0,qword[edi]
	MOVQ		MM1,qword[edi + 8]
	
	PADDSW		MM0,MM2
	PADDSW		MM1,MM2
	
	PACKUSWB	MM0,MM1
	
	MOVQ		qword[esi],MM0
	
	;line 4
	ADD			esi,ecx
	ADD			edi,16

	MOVQ		MM0,qword[edi]
	MOVQ		MM1,qword[edi + 8]
	
	PADDSW		MM0,MM2
	PADDSW		MM1,MM2
	
	PACKUSWB	MM0,MM1
	
	MOVQ		qword[esi],MM0
	
	;line 5
	ADD			esi,ecx
	ADD			edi,16

	MOVQ		MM0,qword[edi]
	MOVQ		MM1,qword[edi + 8]
	
	PADDSW		MM0,MM2
	PADDSW		MM1,MM2
	
	PACKUSWB	MM0,MM1
	
	MOVQ		qword[esi],MM0
	
	;line 6
	ADD			esi,ecx
	ADD			edi,16

	MOVQ		MM0,qword[edi]
	MOVQ		MM1,qword[edi + 8]
	
	PADDSW		MM0,MM2
	PADDSW		MM1,MM2
	
	PACKUSWB	MM0,MM1
	
	MOVQ		qword[esi],MM0
	
	;line 7
	ADD			esi,ecx
	ADD			edi,16

	MOVQ		MM0,qword[edi]
	MOVQ		MM1,qword[edi + 8]
	
	PADDSW		MM0,MM2
	PADDSW		MM1,MM2
	
	PACKUSWB	MM0,MM1
	
	MOVQ		qword[esi],MM0
	
	
	EMMS
	
	pop			ecx
	
	pop			edi
	
	POP			esi
	
	MOV			esp,ebp
	POP 		ebp 		
	
	RET   
    