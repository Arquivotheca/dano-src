    GLOBAL  mc2_hf_vf_mmx16
    GLOBAL  mc2_hf_vh_mmx16
    GLOBAL  mc2_hh_vf_mmx16
    GLOBAL  mc2_hh_vh_mmx16
    
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
	
mc2_hf_vf_mmx16:
	PUSH		ebp
		
	MOV			ebp,esp
	
	;alignement on 64 bit of esp
	SUB			esp,8
	AND			esp,-8
	
	SUB			esp,96
	
	
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
	MOV 		dword[esp + 28],0

	MOV 		dword[esp + 32],64
	SUB			dword[esp + 32],ebx
	MOV 		dword[esp + 36],0

	MOVQ		MM6,qword[esp + 24]
	MOVQ		MM7,qword[esp + 32]
	
	MOV			ebx,15
	
.loop	
	MOVQ        MM0,qword[esi]
	MOVQ        MM2,qword[esi + 8]
	
	MOV			ecx,dword[esp + 24]
	JECXZ		.alignment
	MOVQ		MM1,MM2
	
	PSRLQ		MM0,MM6
	PSLLQ		MM1,MM7
	POR			MM0,MM1
	
	MOVQ        MM3,qword[esi + 16]
	PSRLQ		MM2,MM6
	PSLLQ		MM3,MM7
	POR			MM2,MM3
	
.alignment
	MOVQ		MM1,MM0
	MOVQ		MM4,qword[edi]
	
	POR			MM1,MM4
	PAND		MM1,qword[mask1]
	
	PAND		MM0,qword[nonmask1]
	PSRLQ		MM0,qword[un]
	
	PAND		MM4,qword[nonmask1]
	PSRLQ		MM4,qword[un]
	
	PADDUSB		MM0,MM4
	PADDUSB		MM0,MM1
	
	MOVQ		qword[edi],MM0
	
	
	MOVQ		MM1,MM2
	MOVQ		MM4,qword[edi + 8]
	
	POR			MM1,MM4
	PAND		MM1,qword[mask1]
	
	PAND		MM2,qword[nonmask1]
	PSRLQ		MM2,qword[un]
	
	PAND		MM4,qword[nonmask1]
	PSRLQ		MM4,qword[un]
	
	PADDUSB		MM2,MM4
	PADDUSB		MM2,MM1
	
	MOVQ		qword[edi + 8],MM2
	
	MOV			ecx,ebx
	JECXZ		.fin
	DEC			ebx
	ADD			esi,eax
	ADD			edi,eax
	JMP			.loop
	
	
	
.fin

	EMMS
	POP			ecx
	POP			ebx
	POP			eax
	POP			edi
	POP			esi
	
	MOV			esp,ebp
	POP 		ebp 		
	
	RET
		
		
mc2_hh_vf_mmx16:
	PUSH		ebp
		
	MOV			ebp,esp
	
	;alignement on 64 bit of esp
	SUB			esp,8
	AND			esp,-8
	
	SUB			esp,96
	
	
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
	MOV 		dword[esp + 28],0

	MOV 		dword[esp + 32],64
	SUB			dword[esp + 32],ebx
	MOV 		dword[esp + 36],0
	
	MOV 		dword[esp + 40],8
	ADD			dword[esp + 40],ebx
	MOV 		dword[esp + 44],0
	
	MOV 		dword[esp + 48],56
	SUB			dword[esp + 48],ebx
	MOV 		dword[esp + 52],0


	MOV			ebx,15
	
.loop	
	MOVQ        MM0,qword[esi]
	MOVQ        MM2,qword[esi + 8]
	MOVQ        MM3,qword[esi + 16]
			
	MOVQ		MM4,MM0
	MOVQ		MM5,MM2
	MOVQ		MM7,MM3	
	
	MOV			ecx,dword[esp + 24]
	JECXZ		.alignment1
	MOVQ		MM1,MM2
	
	PSRLQ		MM0,qword[esp + 24]
	PSLLQ		MM1,qword[esp + 32]
	POR			MM0,MM1
	
	PSRLQ		MM2,qword[esp + 24]
	PSLLQ		MM3,qword[esp + 32]
	POR			MM2,MM3
	
.alignment1

	MOV			ecx,dword[esp + 48]
	JECXZ		.alignment2
	MOVQ		MM6,MM5
	
	PSRLQ		MM4,qword[esp + 40]
	PSLLQ		MM5,qword[esp + 48]
	POR			MM5,MM4
	
	PSRLQ		MM6,qword[esp + 40]
	PSLLQ		MM7,qword[esp + 48]
	POR			MM7,MM6
	
.alignment2
	MOVQ		MM1,MM0
	POR			MM1,MM5
	PAND		MM1,qword[mask1]
	
	PAND		MM0,qword[nonmask1]
	PSRLQ		MM0,qword[un]
	
	PAND		MM5,qword[nonmask1]
	PSRLQ		MM5,qword[un]
	
	PADDUSB		MM0,MM5
	PADDUSB		MM0,MM1
	
	
	MOVQ		MM1,MM2
	POR			MM1,MM7
	PAND		MM1,qword[mask1]
	
	PAND		MM2,qword[nonmask1]
	PSRLQ		MM2,qword[un]
	
	PAND		MM7,qword[nonmask1]
	PSRLQ		MM7,qword[un]
	
	PADDUSB		MM2,MM7
	PADDUSB		MM2,MM1
	
	
	
	MOVQ		MM1,MM0
	MOVQ		MM4,qword[edi]
	
	POR			MM1,MM4
	PAND		MM1,qword[mask1]
	
	PAND		MM0,qword[nonmask1]
	PSRLQ		MM0,qword[un]
	
	PAND		MM4,qword[nonmask1]
	PSRLQ		MM4,qword[un]
	
	PADDUSB		MM0,MM4
	PADDUSB		MM0,MM1
	
	MOVQ		qword[edi],MM0
	
	
	MOVQ		MM1,MM2
	MOVQ		MM4,qword[edi + 8]
	
	POR			MM1,MM4
	PAND		MM1,qword[mask1]
	
	PAND		MM2,qword[nonmask1]
	PSRLQ		MM2,qword[un]
	
	PAND		MM4,qword[nonmask1]
	PSRLQ		MM4,qword[un]
	
	PADDUSB		MM2,MM4
	PADDUSB		MM2,MM1
	
	MOVQ		qword[edi + 8],MM2
	
	MOV			ecx,ebx
	JECXZ		.fin
	DEC			ebx
	ADD			esi,eax
	ADD			edi,eax
	JMP			.loop
	
	
	
.fin

	EMMS
	POP			ecx
	POP			ebx
	POP			eax
	POP			edi
	POP			esi
	
	MOV			esp,ebp
	POP 		ebp 		
	
	RET
	
mc2_hf_vh_mmx16:
	PUSH		ebp
		
	MOV			ebp,esp
	
	;alignement on 64 bit of esp
	SUB			esp,8
	AND			esp,-8
	
	SUB			esp,96
	
	
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
	MOV 		dword[esp + 28],0

	MOV 		dword[esp + 32],64
	SUB			dword[esp + 32],ebx
	MOV 		dword[esp + 36],0

	
	
	
	MOVQ        MM4,qword[esi]
	MOVQ        MM6,qword[esi + 8]
	
	MOV			ecx,dword[esp + 24]
	JECXZ		.alignment0
	MOVQ		MM5,MM6
	
	PSRLQ		MM4,qword[esp + 24]
	PSLLQ		MM5,qword[esp + 32]
	POR			MM4,MM5
	
	MOVQ        MM7,qword[esi + 16]
	PSRLQ		MM6,qword[esp + 24]
	PSLLQ		MM7,qword[esp + 32]
	POR			MM6,MM7
.alignment0

	MOV			ebx,15
	ADD			esi,eax
	
.loop	
	MOVQ        MM0,qword[esi]
	MOVQ        MM2,qword[esi + 8]
	
	MOV			ecx,dword[esp + 24]
	JECXZ		.alignment
	MOVQ		MM1,MM2
	
	PSRLQ		MM0,qword[esp + 24]
	PSLLQ		MM1,qword[esp + 32]
	POR			MM0,MM1
	
	MOVQ        MM3,qword[esi + 16]
	PSRLQ		MM2,qword[esp + 24]
	PSLLQ		MM3,qword[esp + 32]
	POR			MM2,MM3
.alignment
	MOVQ		MM1,MM0

	
	MOVQ		MM5,MM0
	POR			MM5,MM4
	PAND		MM5,qword[mask1]
	
	
	PAND		MM1,qword[nonmask1]
	PSRLQ		MM1,qword[un]
	
	PAND		MM4,qword[nonmask1]
	PSRLQ		MM4,qword[un]
	
	PADDUSB		MM1,MM4
	PADDUSB		MM1,MM5
	
	
	MOVQ		MM3,qword[edi]
	MOVQ		MM4,MM1
	
	POR			MM4,MM3
	PAND		MM4,qword[mask1]
	
	PAND		MM1,qword[nonmask1]
	PSRLQ		MM1,qword[un]
	
	PAND		MM3,qword[nonmask1]
	PSRLQ		MM3,qword[un]
	
	PADDUSB		MM1,MM3
	PADDUSB		MM1,MM4

	MOVQ		qword[edi],MM1
	

	MOVQ		MM1,MM2
	
	MOVQ		MM5,MM2
	POR			MM5,MM6
	PAND		MM5,qword[mask1]
	
	
	PAND		MM1,qword[nonmask1]
	PSRLQ		MM1,qword[un]
	
	PAND		MM6,qword[nonmask1]
	PSRLQ		MM6,qword[un]
	
	PADDUSB		MM1,MM6
	PADDUSB		MM1,MM5
	
	
	MOVQ		MM3,qword[edi + 8]
	MOVQ		MM4,MM1
	
	POR			MM4,MM3
	PAND		MM4,qword[mask1]
	
	PAND		MM1,qword[nonmask1]
	PSRLQ		MM1,qword[un]
	
	PAND		MM3,qword[nonmask1]
	PSRLQ		MM3,qword[un]
	
	PADDUSB		MM1,MM3
	PADDUSB		MM1,MM4
	
	MOVQ		qword[edi + 8],MM1
	
	MOV			ecx,ebx
	JECXZ		.fin
	DEC			ebx
	MOVQ		MM4,MM0
	MOVQ		MM6,MM2
	ADD			esi,eax
	ADD			edi,eax
	JMP			.loop
	
	
	
.fin

	EMMS
	POP			ecx
	POP			ebx
	POP			eax
	POP			edi
	POP			esi
	
	MOV			esp,ebp
	POP 		ebp 		
	
	RET
	
mc2_hh_vh_mmx16:
	PUSH		ebp
		
	MOV			ebp,esp
	
	;alignement on 64 bit of esp
	SUB			esp,8
	AND			esp,-8
	
	SUB			esp,96
	
	
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
	MOV 		dword[esp + 28],0

	MOV 		dword[esp + 32],64
	SUB			dword[esp + 32],ebx
	MOV 		dword[esp + 36],0
	
	MOV 		dword[esp + 40],8
	ADD			dword[esp + 40],ebx
	MOV 		dword[esp + 44],0
	
	MOV 		dword[esp + 48],56
	SUB			dword[esp + 48],ebx
	MOV 		dword[esp + 52],0


	MOVQ        MM0,qword[esi]
	MOVQ        MM2,qword[esi + 8]
	MOVQ        MM3,qword[esi + 16]
			
	MOVQ		MM4,MM0
	MOVQ		MM5,MM2
	MOVQ		MM7,MM3	
	
	MOV			ecx,dword[esp + 24]
	JECXZ		.alignment01
	MOVQ		MM1,MM2
	
	PSRLQ		MM0,qword[esp + 24]
	PSLLQ		MM1,qword[esp + 32]
	POR			MM0,MM1
	
	PSRLQ		MM2,qword[esp + 24]
	PSLLQ		MM3,qword[esp + 32]
	POR			MM2,MM3
	
.alignment01

	MOV			ecx,dword[esp + 48]
	JECXZ		.alignment02
	MOVQ		MM6,MM5
	
	PSRLQ		MM4,qword[esp + 40]
	PSLLQ		MM5,qword[esp + 48]
	POR			MM5,MM4
	
	PSRLQ		MM6,qword[esp + 40]
	PSLLQ		MM7,qword[esp + 48]
	POR			MM7,MM6
	
.alignment02

	MOVQ		MM1,MM0
	MOVQ		MM3,MM2
	MOVQ		MM4,MM5
	MOVQ		MM6,MM7
	
		
	PUNPCKLBW	MM0,qword[zero]
	PUNPCKHBW	MM1,qword[zero]
	
	PUNPCKLBW	MM2,qword[zero]
	PUNPCKHBW	MM3,qword[zero]
	
	
	PUNPCKLBW	MM4,qword[zero]
	PUNPCKHBW	MM5,qword[zero]
	
	PUNPCKLBW	MM6,qword[zero]
	PUNPCKHBW	MM7,qword[zero]
	
	PADDUSW		MM0,MM4
	PADDUSW		MM1,MM5
	PADDUSW		MM2,MM6
	PADDUSW		MM3,MM7


	MOVQ		qword[esp + 56],MM0
	MOVQ		qword[esp + 64],MM1
	MOVQ		qword[esp + 72],MM2
	MOVQ		qword[esp + 80],MM3


	MOV			ebx,15
	ADD			esi,eax
	
.loop	
	MOVQ        MM0,qword[esi]
	MOVQ        MM2,qword[esi + 8]
	MOVQ        MM3,qword[esi + 16]
			
	MOVQ		MM4,MM0
	MOVQ		MM5,MM2
	MOVQ		MM7,MM3	
	
	MOV			ecx,dword[esp + 24]
	JECXZ		.alignment1
	MOVQ		MM1,MM2
	
	PSRLQ		MM0,qword[esp + 24]
	PSLLQ		MM1,qword[esp + 32]
	POR			MM0,MM1
	
	PSRLQ		MM2,qword[esp + 24]
	PSLLQ		MM3,qword[esp + 32]
	POR			MM2,MM3
	
.alignment1

	MOV			ecx,dword[esp + 48]
	JECXZ		.alignment2
	MOVQ		MM6,MM5
	
	PSRLQ		MM4,qword[esp + 40]
	PSLLQ		MM5,qword[esp + 48]
	POR			MM5,MM4
	
	PSRLQ		MM6,qword[esp + 40]
	PSLLQ		MM7,qword[esp + 48]
	POR			MM7,MM6
	
.alignment2

	MOVQ		MM1,MM0
	MOVQ		MM3,MM2
	MOVQ		MM4,MM5
	MOVQ		MM6,MM7
	
		
	PUNPCKLBW	MM0,qword[zero]
	PUNPCKHBW	MM1,qword[zero]
	
	PUNPCKLBW	MM2,qword[zero]
	PUNPCKHBW	MM3,qword[zero]
	
	
	PUNPCKLBW	MM4,qword[zero]
	PUNPCKHBW	MM5,qword[zero]
	
	PUNPCKLBW	MM6,qword[zero]
	PUNPCKHBW	MM7,qword[zero]
	
	PADDUSW		MM0,MM4
	PADDUSW		MM1,MM5
	PADDUSW		MM2,MM6
	PADDUSW		MM3,MM7
	
	MOVQ		MM4,qword[esp + 56]
	MOVQ		MM5,qword[esp + 64]
	MOVQ		MM6,qword[esp + 72]
	MOVQ		MM7,qword[esp + 80]
	
	PADDUSW		MM4,MM0
	PADDUSW		MM5,MM1
	PADDUSW		MM6,MM2
	PADDUSW		MM7,MM3
	
	PSRAW		MM4,qword[deux]
	PSRAW		MM5,qword[deux]
	PSRAW		MM6,qword[deux]
	PSRAW		MM7,qword[deux]
	
	MOVQ		qword[esp + 56],MM0
	MOVQ		qword[esp + 64],MM1
	MOVQ		qword[esp + 72],MM2
	MOVQ		qword[esp + 80],MM3
	
	MOVQ		MM0,qword[edi]
	MOVQ		MM2,qword[edi + 8]
	
	MOVQ		MM1,MM0
	MOVQ		MM3,MM2
	
	PUNPCKLBW	MM0,qword[zero]
	PUNPCKHBW	MM1,qword[zero]
	
	PUNPCKLBW	MM2,qword[zero]
	PUNPCKHBW	MM3,qword[zero]
	
		
	PADDUSW		MM4,MM0
	PADDUSW		MM5,MM1
	PADDUSW		MM6,MM2
	PADDUSW		MM7,MM3
	
	PSRAW		MM4,qword[un]
	PSRAW		MM5,qword[un]
	PSRAW		MM6,qword[un]
	PSRAW		MM7,qword[un]
	
	PACKUSWB	MM4,MM5
	
	PACKUSWB	MM6,MM7

	MOVQ		qword[edi],MM4
	MOVQ		qword[edi + 8],MM6
	
	MOV			ecx,ebx
	JECXZ		.fin
	DEC			ebx
	ADD			esi,eax
	ADD			edi,eax
	JMP			.loop
	
	
	
.fin

	EMMS
	POP			ecx
	POP			ebx
	POP			eax
	POP			edi
	POP			esi
	
	MOV			esp,ebp
	POP 		ebp 		
	
	RET
	