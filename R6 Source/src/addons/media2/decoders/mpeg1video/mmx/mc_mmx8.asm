    GLOBAL  mc_hf_vf_mmx8
    GLOBAL  mc_hf_vh_mmx8
    GLOBAL  mc_hh_vf_mmx8
    GLOBAL  mc_hh_vh_mmx8
    
    SECTION .data
un			DW 1,0,0,0
zero 		DW 0,0,0,0    
deux		DW 2,0,0,0
mask1 		DW 0x01010101,0x01010101,0x01010101,0x01010101
nonmask1 	DW 0xfefefefe,0xfefefefe,0xfefefefe,0xfefefefe
mask2		DW 0x02020202,0x02020202,0x02020202,0x02020202
mask3		DW 0x03030303,0x03030303,0x03030303,0x03030303
nonmask3	DW 0xfcfcfcfc,0xfcfcfcfc,0xfcfcfcfc,0xfcfcfcfc
    
    
    
    SECTION .text

	
mc_hf_vf_mmx8:
	PUSH		ebp
	MOV			ebp,esp
	
	SUB			esp,8
	AND			esp,-8
	
	SUB			esp,32
	
	;reference frame
	PUSH 		esi
	MOV			esi,dword[ebp + 8]
	
	;current frame
	PUSH 		edi
	MOV			edi,dword[ebp + 12]
	
	; stride
	PUSH 		eax
	MOV			eax,dword[ebp + 16]
	
	; alignment stride 64 bits for MM
	PUSH 		ebx
	MOV			ebx,dword[ebp + 20]
	
	PUSH		ecx
	
	
	MOV 		dword[esp + 24],ebx
	MOV			dword[esp + 28],0
	
	MOV 		dword[esp + 32],64
	MOV			dword[esp + 36],0
	SUB			[esp + 32],ebx
	
	

	MOVQ		MM5,qword[esp + 24]
	MOVQ		MM6,qword[esp + 32]
	
	MOV			dword[esp + 36],7
	
.loop	
	MOVQ        MM0,qword[esi]
	MOV 		ecx,ebx
	JECXZ 		.alignment
	MOVQ        MM1,qword[esi + 8]
	PSRLQ		MM0,MM5
	PSLLQ		MM1,MM6
	POR			MM0,MM1
	
.alignment:
	MOVQ		qword[edi],MM0
	
	MOV 		ecx,dword[esp + 36]
	JECXZ		.fin
	DEC			ecx
	MOV			dword[esp + 36],ecx
	ADD			esi,eax
	ADD			edi,eax
	JMP			.loop
	
	
.fin	
	POP			ecx
	POP			ebx
	POP			eax
	POP			edi
	POP			esi
	
	MOV			esp,ebp
	POP 		ebp 		
	
	RET
	
	
mc_hh_vf_mmx8:
	PUSH		ebp
	MOV			ebp,esp
	
	SUB			esp,8
	AND			esp,-8
	
	SUB			esp,64
	
	;reference frame
	PUSH 		esi
	MOV			esi,dword[ebp + 8]
	
	;current frame
	PUSH 		edi
	MOV			edi,dword[ebp + 12]
	
	; stride
	PUSH 		eax
	MOV			eax,dword[ebp + 16]
	
	; alignment stride 64 bits for MM
	PUSH 		ebx
	MOV			ebx,dword[ebp + 20]
	
	PUSH		ecx
	
	
	MOV 		dword[esp + 24],ebx
	MOV			dword[esp + 28],0
	
	MOV 		dword[esp + 32],64
	MOV			dword[esp + 36],0
	SUB			[esp + 32],ebx
	
	MOV 		dword[esp + 40],8
	ADD			dword[esp + 40],ebx
	MOV			dword[esp + 44],0
	
	MOV 		dword[esp + 48],56
	MOV			dword[esp + 52],0
	SUB			[esp + 48],ebx
	
	

	MOVQ		MM4,qword[esp + 24]
	MOVQ		MM5,qword[esp + 32]
	MOVQ		MM6,qword[esp + 40]
	MOVQ		MM7,qword[esp + 48]
	
	MOV			ebx,7
	
.loop	
	MOVQ        MM0,qword[esi]
	MOVQ		MM2,qword[esi + 8]
	MOVQ		MM3,MM0
	
	MOV 		ecx,dword[esp + 24]
	JECXZ 		.alignment1
	MOVQ        MM1,MM2
	PSRLQ		MM0,MM4
	PSLLQ		MM1,MM5
	POR			MM0,MM1
	
.alignment1:
	MOV 		ecx,dword[esp + 48]
	JECXZ 		.alignment2
	PSRLQ		MM3,MM6
	PSLLQ		MM2,MM7
	POR			MM2,MM3

.alignment2
	
	MOVQ		MM3,MM0
	POR			MM3,MM2
	PAND		MM3,qword[mask1]
	PAND		MM0,qword[nonmask1]
	PAND		MM2,qword[nonmask1]
	PSRLQ		MM0,qword[un]
	PSRLQ		MM2,qword[un]
	PADDUSB		MM0,MM2
	PADDUSB		MM0,MM3
	

	MOVQ		qword[edi],MM0
	
	MOV 		ecx,ebx
	JECXZ		.fin
	DEC			ebx
	ADD			esi,eax
	ADD			edi,eax
	JMP			.loop
	
	
.fin	
	POP			ecx
	POP			ebx
	POP			eax
	POP			edi
	POP			esi
	
	MOV			esp,ebp
	POP 		ebp 		
	
	RET
	
mc_hf_vh_mmx8:
	PUSH		ebp
	MOV			ebp,esp
	
	SUB			esp,8
	AND			esp,-8
	
	SUB			esp,64
	
	;reference frame
	PUSH 		esi
	MOV			esi,dword[ebp + 8]
	
	;current frame
	PUSH 		edi
	MOV			edi,dword[ebp + 12]
	
	; stride
	PUSH 		eax
	MOV			eax,dword[ebp + 16]
	
	; alignment stride 64 bits for MM
	PUSH 		ebx
	MOV			ebx,dword[ebp + 20]
	
	PUSH		ecx
	
	
	MOV 		dword[esp + 24],ebx
	MOV			dword[esp + 28],0
	
	MOV 		dword[esp + 32],64
	MOV			dword[esp + 36],0
	SUB			[esp + 32],ebx
	

	
	

	MOVQ		MM4,qword[esp + 24]
	MOVQ		MM5,qword[esp + 32]
	MOVQ		MM6,qword[nonmask1]
	MOVQ		MM7,qword[un]
	
	MOV			ebx,7
	
	MOVQ        MM2,qword[esi]
	MOV 		ecx,dword[esp + 24]
	JECXZ 		.alignment0
	MOVQ		MM3,qword[esi + 8]
	PSRLQ		MM2,MM4
	PSLLQ		MM3,MM5
	POR			MM2,MM3
	
.alignment0:
	
	
	ADD			esi,eax
	
	
.loop	
	MOVQ        MM0,qword[esi]
	MOV 		ecx,dword[esp + 24]
	JECXZ 		.alignment
	MOVQ		MM1,qword[esi + 8]
	PSRLQ		MM0,MM4
	PSLLQ		MM1,MM5
	POR			MM0,MM1
	
.alignment:
	MOVQ		MM1,MM0
	
	MOVQ		MM3,MM1
	POR			MM3,MM2
	PAND		MM3,qword[mask1]
	PAND		MM1,MM6
	PAND		MM2,MM6
	PSRLQ		MM1,MM7
	PSRLQ		MM2,MM7
	PADDUSB		MM1,MM2
	PADDUSB		MM1,MM3
	
	MOVQ		qword[edi],MM1
	
	MOV 		ecx,ebx
	JECXZ		.fin
	DEC			ebx
	MOVQ		MM2,MM0
	ADD			esi,eax
	ADD			edi,eax
	JMP			.loop
	
	
.fin	
	POP			ecx
	POP			ebx
	POP			eax
	POP			edi
	POP			esi
	
	MOV			esp,ebp
	POP 		ebp 		
	
	RET
	
	
mc_hh_vh_mmx8:
	PUSH		ebp
	MOV			ebp,esp
	
	SUB			esp,8
	AND			esp,-8
	
	SUB			esp,64
	
	;reference frame
	PUSH 		esi
	MOV			esi,dword[ebp + 8]
	
	;current frame
	PUSH 		edi
	MOV			edi,dword[ebp + 12]
	
	; stride
	PUSH 		eax
	MOV			eax,dword[ebp + 16]
	
	; alignment stride 64 bits for MMX
	PUSH 		ebx
	MOV			ebx,dword[ebp + 20]
	
	PUSH		ecx
	
	
	MOV 		dword[esp + 24],ebx
	MOV			dword[esp + 28],0
	
	MOV 		dword[esp + 32],64
	MOV			dword[esp + 36],0
	SUB			[esp + 32],ebx
	
	MOV 		dword[esp + 40],8
	ADD			dword[esp + 40],ebx
	MOV			dword[esp + 44],0
	
	MOV 		dword[esp + 48],56
	MOV			dword[esp + 52],0
	SUB			[esp + 48],ebx
	
	


	MOVQ		MM6,qword[deux]
	MOVQ		MM7,qword[nonmask3]
	
	
	MOVQ        MM4,qword[esi]
	MOVQ		MM5,qword[esi + 8]
	MOVQ		MM3,MM4
	
	MOV 		ecx,dword[esp + 24]
	JECXZ 		.alignment01
	MOVQ        MM0,MM5
	PSRLQ		MM4,qword[esp + 24]
	PSLLQ		MM0,qword[esp + 32]
	POR			MM4,MM0
	
.alignment01:
	MOV 		ecx,dword[esp + 48]
	JECXZ 		.alignment02
	PSRLQ		MM3,qword[esp + 40]
	PSLLQ		MM5,qword[esp + 48]
	POR			MM5,MM3

.alignment02
	
	
	MOVQ		MM0,MM4
	MOVQ		MM1,MM5
	
	PAND		MM4,MM7
	PSRLQ		MM4,MM6
	PAND		MM0,qword[mask3]
	
	PAND		MM1,MM7
	PSRLQ		MM1,MM6
	PAND		MM5,qword[mask3]
	
	PADDUSB		MM4,MM1
	PADDUSB		MM5,MM0
		
	
	
	
	
	MOV			ebx,7
	ADD			esi,eax
	
	
.loop	
	MOVQ        MM0,qword[esi]
	MOVQ		MM2,qword[esi + 8]
	MOVQ		MM3,MM0
	
	MOV 		ecx,dword[esp + 24]
	JECXZ 		.alignment1
	MOVQ        MM1,MM2
	PSRLQ		MM0,qword[esp + 24]
	PSLLQ		MM1,qword[esp + 32]
	POR			MM0,MM1
	
.alignment1:
	MOV 		ecx,dword[esp + 48]
	JECXZ 		.alignment2
	PSRLQ		MM3,qword[esp + 40]
	PSLLQ		MM2,qword[esp + 48]
	POR			MM2,MM3

.alignment2

	MOVQ		MM1,MM0
	MOVQ		MM3,MM2
	
	PAND		MM0,MM7
	PSRLQ		MM0,MM6
	PAND		MM1,qword[mask3]
	
	PAND		MM2,MM7
	PSRLQ		MM2,MM6
	PAND		MM3,qword[mask3]
	
	PADDUSB		MM0,MM2
	PADDUSB		MM1,MM3
	
	PADDUSB		MM4,MM0
	PADDUSB		MM5,MM1
	PADDUSB		MM5,qword[mask2]
	
	PAND		MM5,MM7
	PSRLQ		MM5,MM6
	PADDUSB		MM4,MM5
	
	
	MOVQ		qword[edi],MM4
	
	MOV 		ecx,ebx
	JECXZ		.fin
	DEC			ebx
	MOVQ		MM4,MM0
	MOVQ		MM5,MM1
	ADD			esi,eax
	ADD			edi,eax
	JMP			.loop
	
	
.fin	
	POP			ecx
	POP			ebx
	POP			eax
	POP			edi
	POP			esi
	
	MOV			esp,ebp
	POP 		ebp 		
	
	RET	