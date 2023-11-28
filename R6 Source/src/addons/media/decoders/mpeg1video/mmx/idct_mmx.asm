   GLOBAL  idct_mmx
 

	SECTION .data

shift_row 	DW 10,0,0,0
round_row	DW 512,0,512,0

shift_col 	DW 7,0,0,0
round_col	DW 64,64,64,64

tan1		DW 13036 ,13036 ,13036 ,13036 
tan2		DW 27146 ,27146 ,27146 ,27146 
tan3		DW 43790 ,43790 ,43790 ,43790 
;tan3 		DW 21895,21895,21895,21895
c4			DW 46341 ,46341 ,46341 ,46341 
;c4			DW 23171,23171,23171,23171

mul4		DW 16384,21407,16384,8867
			DW 16384,8867,-16384,-21407
			DW 16384,-8867,16384,-21407
			DW -16384,21407,16384,-8867
			DW 22725,19266,19266,-4520
			DW 12873,4520,-22725,-12873
			DW 12873,-22725,4520,-12873
			DW 4520,19266,19266,-22725
		
mul2		DW 21407,27969,21407,11585
			DW 21407,11585,-21407,-27969
			DW 21407,-11585,21407,-27969
			DW -21407,27969,21407,-11585
			DW 29692,25172,25172,-5906
			DW 16819,5906,-29692,-16819
			DW 16819,-29692,5906,-16819
			DW 5906,25172,25172,-29692
		

mul1		DW 22725,29692,22725,12299
			DW 22725,12299,-22725,-29692
			DW 22725,-12299,22725,-29692
			DW -22725,29692,22725,-12299
			DW 31521,26722,26722,-6270
			DW 17855,6270,-31521,-17855
			DW 17855,-31521,6270,-17855
			DW 6270,26722,26722,-31521

mul3		DW 19266,25175,19266,10426
			DW 19266,10426,-19266,-25175
			DW 19266,-10426,19266,-25175
			DW -19266,25175,19266,-10426
			DW 26722,22654,22654,-5315
			DW 15137,5315,-26722,-15137
			DW 15137,-26722,5315,-15137
			DW 5315,22654,22654,-26722
 
nomul 		DW 23170,30274,23170,12540
			DW 23170,12540,-23170,-30274
			DW 23170,-12540,23170,-30274
			DW -23170,30274,23170,-12540
			DW 32138,27246,27246,-6393
			DW 18205,6393,-32138,-18205
			DW 18205,-32138,6393,-18205
			DW 6393,27246,27246,-32138
  

 
    	
    SECTION .text
    
%macro idct_row 0
	
	movq		MM0,qword[esi]		;load x6 x4 x2 x0
	movq		MM1,MM0				;
	punpckldq	MM0,MM0				;x2 x0 x2 x0
	punpckhdq	MM1,MM1				;x6 x4 x6 x4
	
	
	
	movq		MM4,qword[edi]		;load g6 g4 g2 g4
	pmaddwd		MM4,MM0				;x2*g6 + x0*g4 | x2*g2 + x0*g4
	
	movq		MM5,qword[edi + 8]	;load -g2 -g4 g6 g4
	pmaddwd		MM5,MM1				;-x6*g2 - x4*g4 | x6*g6 + x4*g4
	
	movq		MM6,qword[edi + 16]	;load -g2 g4 -g6 g4
	pmaddwd		MM6,MM0				;-x2*g2 + x0*g4 | -x2*g6 + x0*g4
	
	movq		MM7,qword[edi + 24]	;load -g6 g4 g2 -g4
	pmaddwd		MM7,MM1				;-x6*g6 + x4*g4 | x6*g2 - x4*g4
	
	paddd		MM4,MM5				;-x6*g2 - x4*g4 + x2*g6 + x0*g4 = a1
									;x6*g6 + x4*g4 + x2*g2 + x0*g4 = a0
	
	paddd		MM6,MM7				;-x6*g6 + x4*g4 - x2*g2 + x0*g4 = a3
									;x6*g2 - x4*g4 - x2*g6 + x0*g4 = a2
	
	
	movq		MM0,qword[esi + 8]	;load x7 x5 x3 x1
	movq		MM1,MM0				;
	punpckldq	MM0,MM0				;x3 x1 x3 x1 
	punpckhdq	MM1,MM1				;x7 x5 x7 x5
	
	
	
	movq		MM2,qword[edi + 32]		;load -g7 g3 g3 g1
	pmaddwd		MM2,MM0					;- x3*g7 + x1*g3 | x3*g3 + x1*g1
 	
	movq		MM3,qword[edi + 40]		;load -g5 -g1 g7 g5
	pmaddwd		MM3,MM1					;- x7*g5 - x5*g1 | x7*g7 + x5*g5
	
	movq		MM5,qword[edi + 48]		;load -g5 g7 -g1 g5
	pmaddwd		MM5,MM0					;- x3*g5 + x1*g7 | - x3*g1 + x1*g5
	
	movq		MM7,qword[edi + 56]		;load -g1 g3 g3 g7
	pmaddwd		MM7,MM1					;- x7*g1 + x5*g3 | x7*g3 + x5*g7
	
	paddd		MM2,MM3					;- x7*g5 - x5*g1 - x3*g7 + x1*g3 = a5
										;x7*g7 + x5*g5 + x3*g3 + x1*g1 = a4
	
	paddd		MM5,MM7					;- x7*g1 + x5*g3 - x3*g5 + x1*g7 = a7
										; x7*g3 + x5*g7 - x3*g1 + x1*g5 = a6
	
	
	movq		MM0,MM4					;a1 | a0
	movq		MM1,MM6					;a3 | a2
	
	paddd		MM0,MM2					;a1 + a5 = y1| a0 + a4 = y0
	paddd		MM1,MM5					;a3 + a7 = y3| a2 + a6 = y2
	
	psubd		MM4,MM2					;a1 - a5 = y6| a0 - a4 = y7
	psubd		MM6,MM5					;a3 - a7 = y4| a2 - a6 = y5
	
	
	movq		MM2,qword[shift_row]
	movq		MM5,qword[round_row]
	
	
	paddd		MM0,MM5
	paddd		MM1,MM5
	paddd		MM4,MM5
	paddd		MM6,MM5
	
	
	psrad		MM0,MM2
	psrad		MM1,MM2
	psrad		MM4,MM2
	psrad		MM6,MM2

	packssdw	MM0,MM1					;y3 y2 y1 y0
	packssdw	MM6,MM4					;y6 y7 y4 y5

	movq		MM1,MM6					;y6 y7 y4 y5
	
	psrld		MM6,16					;0 y6 0 y4
	pslld		MM1,16					;y7 0 y5 0
	
	por 		MM1,MM6					;y7 y6 y5 y4
	
	movq		qword[esi],MM0
	movq		qword[esi + 8],MM1
	
	
%endmacro

%macro idct_col 0
	 
	movq		MM0,qword[esi + 16]		;x1
	movq		MM2,qword[esi + 112]	;x7
	
	movq		MM6,qword[tan1]			;tan(PI/16)
	
	movq		MM1,MM0					;dup x1
	movq		MM3,MM2					;dup x7
		
	;3dnow!
	;pmulhrwa		MM1,MM6					;tan1 * x1
	;pmulhrwa		MM3,MM6					;tan1 * x7
	
	pmulhw		MM1,MM6					;tan1 * x1
	pmulhw		MM3,MM6					;tan1 * x7
	
	psubsw		MM1,MM2					;a5 = tan1 * x1 - x7
	paddsw		MM0,MM3					;a4 = x1 + tan1 * x7 



	movq		MM2,qword[esi + 48]		;x3
	movq		MM4,qword[esi + 80]		;x5
	
	movq		MM6,qword[tan3]			;1-tan(3*PI/16)
	
	movq		MM3,MM2					;dup x3
	movq		MM5,MM4					;dup x5
		
	;3dnow!
	;pmulhrwa	MM2,MM6				;(1-tan3) * x3
	;pmulhrwa	MM4,MM6				;(1-tan3) * x5
	
	pmulhw		MM2,MM6				;(1-tan3) * x3
	pmulhw		MM4,MM6				;(1-tan3) * x5
	
	
	paddsw		MM2,MM3					;tan3 * x3
	paddsw		MM4,MM5					;tan3 * x5
	
	
	psubsw		MM2,MM5					;a7 = tan3 * x3 - x5
	paddsw		MM3,MM4					;a6 = x3 + tan3 * x5 


	;papillon a4 a5 a6 a7
	movq		MM4,MM0					;dup a4
	paddsw		MM0,MM3					;b4 = a4 + a6
	psubsw		MM4,MM3					;b5 = a4 - a6
	
	movq		MM5,MM1					;dup a5
	paddsw		MM1,MM2					;b6 = a5 + a7
	psubsw		MM5,MM2					;b7 = a5 - a7 

	;papillon b5 b6
	
	movq		MM6,MM4					;dup b5
	movq		MM7,qword[c4]			;1-cos(4*PI/16)
	paddsw		MM4,MM1					;b5bis = b5 + b6
	psubsw		MM6,MM1					;b6bis = b5 - b6
	
	movq		MM1,MM4					;b5bis
	movq		MM2,MM6					;b6bis
	
	;3dnow!
	;pmulhrwa	MM4,MM7					;c5 = (1-cos(4*PI/16)) * b5bis 
	;pmulhrwa	MM6,MM7					;c6 = (1-cos(4*PI/16)) * b6bis
	
	pmulhw		MM4,MM7					;c5 = (1-cos(4*PI/16)) * b5bis 
	pmulhw		MM6,MM7					;c6 = (1-cos(4*PI/16)) * b6bis	
 

	paddsw		MM4,MM1					;finish the good pmulhw
	paddsw		MM6,MM2					;finish the good pmulhw

	;save data in esp + 16
	movq		qword[esp + 8],MM0		;save c4 = b4
	movq		qword[esp + 16],MM4		;save c5
	movq		qword[esp + 24],MM6		;save c6
	movq		qword[esp + 32],MM5		;save c7 = b7


	;begin other part 
	movq		MM0,qword[esi]			;x0
	movq		MM2,qword[esi + 64]		;x4
	
	movq		MM1,MM0					;dup x0
	
	paddsw		MM0,MM2					;a0 = x0 + x4
	psubsw		MM1,MM2					;a1 = x0 - x4

	movq		MM2,qword[esi + 32]		;x2
	movq		MM4,qword[esi + 96]		;x6
	
	movq		MM6,qword[tan2]			;tan(2*PI/16)
	
	movq		MM3,MM2					;dup x2
	movq		MM5,MM4					;dup x6
	
	;3DNOW!
	;pmulhrwa	MM3,MM6					;tan2 * x2
	;pmulhrwa	MM4,MM6					;tan2 * x6
	pmulhw		MM3,MM6					;tan2 * x2
	pmulhw		MM4,MM6					;tan2 * x6
	
	
	psubsw		MM3,MM5					;a3 = tan2 * x2 - x6
	paddsw		MM2,MM4					;a2 = x2 + tan2 * x6 
	
	
	movq		MM5,MM0					;dup a0
	movq		MM4,MM1					;dup a1
		
	paddsw		MM0,MM2					;b0 = a0 + a2
	paddsw		MM1,MM3					;b1 = a1 + a3
	
	psubsw		MM5,MM2					;b3 = a0 - a2
	psubsw		MM4,MM3					;b2 = a1 - a3
	
	;end b0 b1 b2 b3
	
	;papillon final
	movq		MM6,qword[round_col]
	movq		MM7,qword[shift_col]	
	
	
	;Y0, Y7
	paddsw		MM0,MM6					;add round
	
	movq		MM3,qword[esp + 8]		;load c0
	movq		MM2,MM0					;dup b0
	
	paddsw		MM0,MM3					;y0 = b0 + c0
	psubsw		MM2,MM3					;y7 = b0 - c0
	
	psraw		MM0,MM7					;shift right
	psraw		MM2,MM7					;shift right
	
	movq		qword[esi + 0],MM0		;save y0
	movq		qword[esi + 112],MM2	;save y7
	
	
	;Y1,Y6
	paddsw		MM1,MM6					;add round
	
	movq		MM3,qword[esp + 16]		;load c1
	movq		MM2,MM1					;dup b1
	
	paddsw		MM1,MM3					;y1 = b1 + c1
	psubsw		MM2,MM3					;y6 = b1 - c1
	
	psraw		MM1,MM7					;shift right
	psraw		MM2,MM7					;shift right
	
	movq		qword[esi + 16],MM1		;save y1
	movq		qword[esi + 96],MM2		;save y6
	
	
	;Y2,Y5
	paddsw		MM4,MM6					;add round
	
	movq		MM3,qword[esp + 24]		;load c2
	movq		MM2,MM4					;dup b2
	
	paddsw		MM4,MM3					;y2 = b2 + c2
	psubsw		MM2,MM3					;y5 = b2 - c2
	
	psraw		MM4,MM7					;shift right
	psraw		MM2,MM7					;shift right
	
	movq		qword[esi + 32],MM4		;save y2
	movq		qword[esi + 80],MM2		;save y5
	
	
	;Y3,Y4
	paddsw		MM5,MM6					;add round
	
	movq		MM3,qword[esp + 32]		;load c3
	movq		MM2,MM5					;dup b3
	
	paddsw		MM5,MM3					;y3 = b3 + c3
	psubsw		MM2,MM3					;y4 = b3 - c3
	
	psraw		MM5,MM7					;shift right
	psraw		MM2,MM7					;shift right
	
	movq		qword[esi + 48],MM5		;save y3
	movq		qword[esi + 64],MM2		;save y4
	
%endmacro



idct_mmx:
	PUSH		ebp
	MOV			ebp,esp
	
	;alignement on 64 bit of esp
	SUB			esp,8
	AND			esp,-8
	
	;reservation of 4 data 64 bits aligned
	SUB			esp,32
	
	
	
	PUSH 		esi
	MOV			esi,dword[ebp + 8]
	
	PUSH		edi
	
	;PUSH		eax
	;PUSH		edx

	;PUSH		ebx
	;mov			ebx,dword[ebp + 12]

	;rdtsc
	
	;mov 		dword[ebx],eax
	;mov 		dword[ebx + 4],edx
	
	
	MOV			edi,mul4
	idct_row
	
	ADD			esi,64
	idct_row
	
	MOV			edi,mul2
	SUB			esi,32
	idct_row
	
	ADD			esi,64
	idct_row
	
	
	MOV			edi,mul1
	ADD			esi,16
	idct_row
	
	SUB			esi,96
	idct_row
	
	MOV			edi,mul3
	ADD			esi,32
	idct_row
	
	ADD			esi,32
	idct_row
	
	SUB			esi,80
	idct_col
	
	ADD			esi,8
	idct_col

	EMMS
	
	
	;rdtsc
	;mov 		dword[ebx + 8],eax
	;mov 		dword[ebx + 12],edx

	;POP			ebx
	;POP			edx
	;POP			eax

	POP			edi
	POP			esi
	
	MOV			esp,ebp
	POP 		ebp
	
	RET