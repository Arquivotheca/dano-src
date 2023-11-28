    GLOBAL  ycbcr_mmx_1
    GLOBAL  ycbcr_mmx_2
   
	SECTION .data
    
    SECTION .text
    
ycbcr_mmx_1:
	PUSH		ebp
	MOV			ebp,esp
	
	SUB			esp,16
	
	PUSH 		esi
	MOV			esi,dword[ebp + 8]
	
	PUSH 		edi
	MOV			edi,dword[ebp + 12]
	
	PUSH 		eax
	MOV			eax,dword[ebp + 16]
	
	PUSH 		ebx
	MOV			ebx,dword[ebp + 20]
	
	PUSH		ecx
	MOV			ecx,dword[ebp + 24]
	
	PUSH		edx
	MOV			edx,dword[ebp + 28]
	
	SHR			ecx,1
	
	MOV			dword[ebp - 4],ecx
	
	MOV			ecx,edx
	
	SHR			ecx,4
	
	MOV			dword[ebp - 8],ecx
	
	MOV			dword[ebp - 12],ecx
	
	MOV			ecx,dword[ebp - 4]
.loop1
	;begin loop on lines
	;we trait 2 lines a time
	
	MOV			dword[ebp - 4],ecx
	MOV			ecx,dword[ebp - 12]
	
	
	
.loop2	
	;begin loop for 2 lines
	;we trait 2 lines a time
	
	
	MOVQ		MM1,qword[eax]
	PXOR		MM0,MM0
	PXOR		MM2,MM2
	
	PUNPCKLBW	MM0,MM1
	PUNPCKHBW	MM2,MM1
	
	PXOR        MM7,MM7
	
	MOVQ		MM1,MM0
	MOVQ		MM3,MM2
	
	
	PUNPCKLWD	MM0,MM7
	PUNPCKHWD	MM1,MM7
	
	PUNPCKLWD	MM2,MM7
	PUNPCKHWD	MM3,MM7
	
	
	
	MOVQ		MM6,qword[ebx]
	
	PUNPCKLBW	MM7,MM6
	
	PXOR		MM4,MM4
	PXOR		MM5,MM5
	
	PUNPCKLWD	MM4,MM7
	PUNPCKHWD	MM5,MM7
	
	POR			MM0,MM4
	POR			MM1,MM5
	
	
	PXOR		MM7,MM7
	PUNPCKHBW	MM7,MM6
	
	PXOR		MM4,MM4
	PXOR		MM5,MM5
	
	PUNPCKLWD	MM4,MM7
	PUNPCKHWD	MM5,MM7
	
	POR			MM2,MM4
	POR			MM3,MM5
	

	
	
	
	MOVQ		MM4,qword[edi]
	PXOR		MM7,MM7
	MOVQ		MM5,MM4
	PUNPCKLBW	MM4,MM7
	PUNPCKHBW	MM5,MM7
	
	POR			MM4,MM0
	POR			MM5,MM1
	
	MOVQ		qword[esi],MM4
	MOVQ		qword[esi + 8],MM5
	
	
	MOVQ		MM4,qword[edi + edx]
	MOVQ		MM5,MM4
	PUNPCKLBW	MM4,MM7
	PUNPCKHBW	MM5,MM7
	
	POR			MM4,MM0
	POR			MM5,MM1
	
	MOVQ		qword[esi + 2*edx],MM4
	MOVQ		qword[esi + 2*edx + 8],MM5
	
	
	MOVQ		MM4,qword[edi + 8]
	MOVQ		MM5,MM4
	PUNPCKLBW	MM4,MM7
	PUNPCKHBW	MM5,MM7
	
	POR			MM4,MM2
	POR			MM5,MM3
	
	MOVQ		qword[esi + 16],MM4
	MOVQ		qword[esi + 24],MM5
	
	
	MOVQ		MM4,qword[edi + 8 + edx]
	MOVQ		MM5,MM4
	PUNPCKLBW	MM4,MM7
	PUNPCKHBW	MM5,MM7
	
	POR			MM4,MM2
	POR			MM5,MM3
	
	MOVQ		qword[esi + 16 + 2*edx],MM4
	MOVQ		qword[esi + 24 + 2*edx],MM5
	
	
	
	ADD 		esi,32
	ADD 		edi,16
	ADD 		eax,8
	ADD 		ebx,8
	
	DEC			ecx
	JNZ		NEAR	.loop2

	
	ADD			esi,edx
	ADD			esi,edx

	ADD			edi,edx
	
	
	MOV			ecx,dword[ebp - 4]
	DEC			ecx
	JNZ		NEAR	.loop1


	
.fin
	EMMS
	
	pop			edx
	
	pop			ecx
	
	pop 		ebx
	
	pop 		eax
	
	pop			edi
	
	POP			esi
	
	MOV			esp,ebp
	POP 		ebp 		
	
	RET
	
	
ycbcr_mmx_2:
	PUSH		ebp
	MOV			ebp,esp
	
	SUB			esp,16
	
	PUSH 		esi
	MOV			esi,dword[ebp + 8]
	
	PUSH 		edi
	MOV			edi,dword[ebp + 12]
	
	PUSH 		eax
	MOV			eax,dword[ebp + 16]
	
	PUSH 		ebx
	MOV			ebx,dword[ebp + 20]
	
	PUSH		ecx
	MOV			ecx,dword[ebp + 24]
	
	PUSH		edx
	MOV			edx,dword[ebp + 28]
	
	SHR			ecx,1
	
	MOV			dword[ebp - 4],ecx
	
	MOV			ecx,edx
	
	SHR			ecx,1
	
	MOV			dword[ebp - 8],ecx
	
	
	SHR			ecx,3
	MOV			dword[ebp - 12],ecx
	
	MOV			ecx,dword[ebp - 4]
.loop1
	;begin loop on lines
	;we trait 2 lines a time
	
	MOV			dword[ebp - 4],ecx
	MOV			ecx,dword[ebp - 12]
	
	
	
.loop2	
	;begin loop for 2 lines
	;we trait 2 lines a time
	
	
	MOVQ		MM1,qword[eax]
	PXOR		MM0,MM0
	PXOR		MM2,MM2
	
	PUNPCKLBW	MM0,MM1
	PUNPCKHBW	MM2,MM1
	
	PXOR        MM7,MM7
	
	MOVQ		MM1,MM0
	MOVQ		MM3,MM2
	
	
	PUNPCKLWD	MM0,MM7
	PUNPCKHWD	MM1,MM7
	
	
	
	
	
	MOVQ		MM6,qword[ebx]
	
	PUNPCKLWD	MM2,MM7
	PUNPCKHWD	MM3,MM7
	
	PUNPCKLBW	MM7,MM6
	
	PXOR		MM4,MM4
	PXOR		MM5,MM5
	
	PUNPCKLWD	MM4,MM7
	PUNPCKHWD	MM5,MM7
	
	PXOR		MM7,MM7
	
	POR			MM0,MM4
	POR			MM1,MM5
	
	
	PUNPCKHBW	MM7,MM6
	
	PXOR		MM4,MM4
	PXOR		MM5,MM5
	
	PUNPCKLWD	MM4,MM7
	PUNPCKHWD	MM5,MM7
	
	POR			MM2,MM4
	POR			MM3,MM5
	

	
	
	
	MOVQ		MM4,qword[edi]
	PXOR		MM7,MM7
	MOVQ		MM5,MM4
	PUNPCKLBW	MM4,MM7
	PUNPCKHBW	MM5,MM7
	
	POR			MM0,MM4
	POR			MM1,MM5
	
	
	
	
	
	MOVQ		qword[esi],MM0
	
	MOVQ		MM4,qword[edi + 8]
	
	
	MOVQ		qword[esi + 8],MM1
	
	MOVQ		MM5,MM4
	PUNPCKLBW	MM4,MM7
	PUNPCKHBW	MM5,MM7
	
	POR			MM2,MM4
	POR			MM3,MM5
	
	

	MOVQ		qword[esi + 16],MM2
	MOVQ		qword[esi + 24],MM3
	
	
	ADD 		esi,32
	ADD 		edi,16
	ADD 		eax,8
	ADD 		ebx,8
	
	DEC			ecx
	JNZ		NEAR	.loop2

	
	
	MOV			ecx,dword[ebp - 12]
	SUB			eax,dword[ebp - 8]
	SUB			ebx,dword[ebp - 8]
	
.loop3	
	;begin loop for 2 lines
	;we trait 2 lines a time
	
	
	MOVQ		MM1,qword[eax]
	PXOR		MM0,MM0
	PXOR		MM2,MM2
	
	PUNPCKLBW	MM0,MM1
	PUNPCKHBW	MM2,MM1
	
	PXOR        MM7,MM7
	
	MOVQ		MM1,MM0
	MOVQ		MM3,MM2
	
	
	PUNPCKLWD	MM0,MM7
	PUNPCKHWD	MM1,MM7
	
	
	
	
	
	MOVQ		MM6,qword[ebx]
	
	PUNPCKLWD	MM2,MM7
	PUNPCKHWD	MM3,MM7
	
	PUNPCKLBW	MM7,MM6
	
	PXOR		MM4,MM4
	PXOR		MM5,MM5
	
	PUNPCKLWD	MM4,MM7
	PUNPCKHWD	MM5,MM7
	
	POR			MM0,MM4
	POR			MM1,MM5
	
	
	PXOR		MM7,MM7
	PUNPCKHBW	MM7,MM6
	
	PXOR		MM4,MM4
	PXOR		MM5,MM5
	
	PUNPCKLWD	MM4,MM7
	PUNPCKHWD	MM5,MM7
	
	POR			MM2,MM4
	POR			MM3,MM5
	

	
	
	
	MOVQ		MM4,qword[edi]
	PXOR		MM7,MM7
	MOVQ		MM5,MM4
	PUNPCKLBW	MM4,MM7
	PUNPCKHBW	MM5,MM7
	
	POR			MM0,MM4
	POR			MM1,MM5
	
	
	
	
	
	
	MOVQ		qword[esi],MM0
	MOVQ		MM4,qword[edi + 8]
	
	
	MOVQ		qword[esi + 8],MM1
	
	MOVQ		MM5,MM4
	PUNPCKLBW	MM4,MM7
	PUNPCKHBW	MM5,MM7
	
	POR			MM2,MM4
	POR			MM3,MM5
	
	
	MOVQ		qword[esi + 16],MM2
	MOVQ		qword[esi + 24],MM3
	
	
	ADD 		esi,32
	ADD 		edi,16
	ADD 		eax,8
	ADD 		ebx,8
	
	DEC			ecx
	JNZ		NEAR	.loop3	
	
	

	MOV			ecx,dword[ebp - 4]
	DEC			ecx
	JNZ		NEAR	.loop1


	
.fin
	EMMS
	
	pop			edx
	
	pop			ecx
	
	pop 		ebx
	
	pop 		eax
	
	pop			edi
	
	POP			esi
	
	MOV			esp,ebp
	POP 		ebp 		
	
	RET
    