#include "mmx.yuv2rgb.h"

#include <stdio.h>
#include <stdlib.h>


void yuv_to_rgb32_mmx(unsigned char * destination,
			  const unsigned char * Y,
			  const unsigned char * Cb,
			  const unsigned char * Cr,
			  int width, int height, int stride)
{
	/*
		R = Y + 1.371 * (Cr - 128)
		G = Y - 0.698 * (Cr - 128) - 0.336 * (Cb - 128)
		B = Y + 1.732 * (Cb - 128)
	*/
	static const unsigned short delta[] = {
		0x8080, 0x8080, 0x8080, 0x8080,
		0x00ff, 0x00ff, 0x00ff, 0x0000 };

	static const short scale[] = {
		short(1.732 * 256), short(-0.336 * 256), 0, 0,
		0, short(-0.698 * 256), short(1.371 * 256), 0 };

	asm volatile(
	"2:"
		"pushl		%0\n"
		"pushl		%6\n"
		
	"1:"
		"movq		(%1),%%mm0\n"			// mm0 = Cb7 Cb6 Cb5 Cb4 Cb3 Cb2 Cb1 Cb0	0.1	-	-	-	-	-	-	-
		"movq		(%2),%%mm1\n"			// mm1 = Cr7 Cr6 Cr5 Cr4 Cr3 Cr2 Cr1 Cr0	-	1.1	-	-	-	-	-	-
		
		"psubb		%4,%%mm0\n"				// mm0 = mm0 - delta						0.2	-	-	-	-	-	-	-
		"psubb		%4,%%mm1\n"				// mm1 = mm1 - delta						-	1.2	-	-	-	-	-	-
		
		"punpcklbw	%%mm0,%%mm0\n"			// mm0 = Cb3 Cb3 Cb2 Cb2 Cb1 Cb1 Cb0 Cb0	0.3	-	-	-	-	-	-	-
		"movq		%%mm0,%%mm4\n"			// mm4 = Cb3 Cb3 Cb2 Cb2 Cb1 Cb1 Cb0 Cb0	0.4	-	-	-	4.1	-	-	-
		
		"punpcklbw	%%mm1,%%mm1\n"			// mm1 = Cr3 Cr3 Cr2 Cr2 Cr1 Cr1 Cr0 Cr0	-	1.3	-	-	-	-	-	-
		"movq		%%mm1,%%mm5\n"			// mm5 = Cr3 Cr3 Cr2 Cr2 Cr1 Cr1 Cr0 Cr0	-	1.4	-	-	-	5.1	-	-

		"punpcklwd	%%mm0,%%mm0\n"			// mm0 = Cb1 Cb1 Cb1 Cb1 Cb0 Cb0 Cb0 Cb0	0.5	-	-	-	-	-	-	-
		"punpcklwd	%%mm1,%%mm1\n"			// mm1 = Cr1 Cr1 Cr1 Cr1 Cr0 Cr0 Cr0 Cr0	-	1.5	-	-	-	-	-	-
		
		"movq		%%mm0,%%mm2\n"			// mm2 = Cb1 Cb1 Cb1 Cb1 Cb0 Cb0 Cb0 Cb0	0.6	-	2.1	-	-	-	-	-
		"punpckldq	%%mm0,%%mm0\n"			// mm0 = Cb0 Cb0 Cb0 Cb0 Cb0 Cb0 Cb0 Cb0	0.7	-	-	-	-	-	-	-

		"movq		%%mm1,%%mm3\n"			// mm3 = Cr1 Cr1 Cr1 Cr1 Cr0 Cr0 Cr0 Cr0	-	1.6	-	3.1	-	-	-	-
		"punpckhdq	%%mm2,%%mm2\n"			// mm2 = Cb1 Cb1 Cb1 Cb1 Cb1 Cb1 Cb1 Cb1	-	-	2.2	-	-	-	-	-
		
		"pmulhw		%5,%%mm0\n"				// mm0 = 00   00 CbG0 CbB0					0.8	-	-	-	-	-	-	-
		"punpckldq	%%mm1,%%mm1\n"			// mm1 = Cr0 Cr0 Cr0 Cr0 Cr0 Cr0 Cr0 Cr0	-	1.7	-	-	-	-	-	-
		
		"pmulhw		8+%5,%%mm1\n"			// mm1 = 00 CrR0 CrG0   00					-	1.8	-	-	-	-	-	-
		"punpckhdq	%%mm3,%%mm3\n"			// mm3 = Cb1 Cb1 Cb1 Cb1 Cb1 Cb1 Cb1 Cb1	-	-	-	3.2	-	-	-	-

		"pmulhw		%5,%%mm2\n"				// mm2 = 00   00 CbG1 CbB1					-	-	2.3	-	-	-	-	-
		"punpckhwd	%%mm4,%%mm4\n"			// mm4 = Cb3 Cb3 Cb3 Cb3 Cb2 Cb2 Cb2 Cb2	-	-	-	-	4.2	-	-	-
		
		"movq		%%mm4,%%mm6\n"			// mm6 = Cb3 Cb3 Cb3 Cb3 Cb2 Cb2 Cb2 Cb2	-	-	-	-	4.3	-	6.1	-
		"punpckhwd	%%mm5,%%mm5\n"			// mm5 = Cr3 Cr3 Cr3 Cr3 Cr2 Cr2 Cr2 Cr2	-	-	-	-	-	5.2	-	-
		
		"pmulhw		8+%5,%%mm3\n"			// mm3 = 00 CrR1 CrG1   00					-	-	-	3.3	-	-	-	-
		"punpckldq	%%mm4,%%mm4\n"			// mm4 = Cb2 Cb2 Cb2 Cb2 Cb2 Cb2 Cb2 Cb2	-	-	-	-	4.4	-	-	-
		
		"pmulhw		%5,%%mm4\n"				// mm4 = 00   00 CbG2 CbB2					-	-	-	-	4.5	-	-	-
		"punpckhdq	%%mm6,%%mm6\n"			// mm6 = Cb3 Cb3 Cb3 Cb3 Cb3 Cb3 Cb3 Cb3	-	-	-	-	-	-	6.2	-
		
		"pmulhw		%5,%%mm6\n"				// mm6 = 00   00 CbG3 CbB3					-	-	-	-	-	-	6.3	-
		"movq		%%mm5,%%mm7\n"			// mm7 = Cr3 Cr3 Cr3 Cr3 Cr2 Cr2 Cr2 Cr2	-	-	-	-	-	5.3	-	7.1
		
		"punpckldq	%%mm5,%%mm5\n"			// mm5 = Cr2 Cr2 Cr2 Cr2 Cr2 Cr2 Cr2 Cr2	-	-	-	-	-	5.4	-	-
		"paddsw		%%mm1,%%mm0\n"			// mm0 = 00 CrR0 CbCrG0 CbB0				0.9	1.9	-	-	-	-	-	-
		
		"pmulhw		8+%5,%%mm5\n"			// mm5 = 00 CrR2 CrG2   00					-	-	-	-	-	5.5	-	-
		"punpckhdq	%%mm7,%%mm7\n"			// mm7 = Cr3 Cr3 Cr3 Cr3 Cr3 Cr3 Cr3 Cr3	-	-	-	-	-	-	-	7.2

		"pmulhw		8+%5,%%mm7\n"			// mm7 = 00 CrR3 CrG3   00					-	-	-	-	-	-	-	7.3
		"paddsw		%%mm3,%%mm2\n"			// mm2 = 00 CrR1 CbCrG1 CbB1				-	-	2.4	3.4	-	-	-	-

		"paddsw		%%mm5,%%mm4\n"			// mm4 = 00 CrR2 CbCrG2 CbB2				-	-	-	-	4.6	5.6	-	-
		"paddsw		%%mm7,%%mm6\n"			// mm6 = 00 CrR3 CbCrG3 CbB3				-	-	-	-	-	-	6.4	7.4

		/* odd row (0-3) */
		"movq		(%8),%%mm1\n"			// mm1 = Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0			-	1.1	-	-	-	-	-	-
		"punpcklbw	%%mm1,%%mm1\n"			// mm1 = Y3 Y3 Y2 Y2 Y1 Y1 Y0 Y0			-	1.2	-	-	-	-	-	-
		"movq		%%mm1,%%mm5\n"			// mm5 = Y3 Y3 Y2 Y2 Y1 Y1 Y0 Y0			-	1.3	-	-	-	5.1	-	-
		"punpcklwd	%%mm1,%%mm1\n"			// mm1 = Y1 Y1 Y1 Y1 Y0 Y0 Y0 Y0			-	1.4	-	-	-	-	-	-
		
		"punpckhwd	%%mm5,%%mm5\n"			// mm5 = Y3 Y3 Y3 Y3 Y2 Y2 Y2 Y2			-	-	-	-	-	5.2	-	-
		"movq		%%mm1,%%mm3\n"			// mm3 = Y1 Y1 Y1 Y1 Y0 Y0 Y0 Y0			-	1.5	-	3.1	-	-	-	-
		
		"movq		%%mm5,%%mm7\n"			// mm7 = Y3 Y3 Y3 Y3 Y2 Y2 Y2 Y2			-	-	-	-	-	5.3	-	7.1
		"punpckldq	%%mm1,%%mm1\n"			// mm1 = Y0 Y0 Y0 Y0 Y0 Y0 Y0 Y0			-	1.6	-	-	-	-	-	-
		
		"punpckldq	%%mm5,%%mm5\n"			// mm5 = Y2 Y2 Y2 Y2 Y2 Y2 Y2 Y2			-	-	-	-	-	5.4	-	-
		"pand		8+%4,%%mm1\n"			// mm1 = 00 00 00 Y0 00 Y0 00 Y0			-	1.7	-	-	-	-	-	-

		"punpckhdq	%%mm3,%%mm3\n"			// mm3 = Y1 Y1 Y1 Y1 Y1 Y1 Y1 Y1			-	-	-	3.2	-	-	-	-
		"paddsw		%%mm0,%%mm1\n"			// mm1 = 00 00 00 R0 00 G0 00 B0			*	1.8	-	-	-	-	-	-
		
		"pand		8+%4,%%mm5\n"			// mm5 = 00 00 00 Y2 00 Y2 00 Y2			-	-	-	-	-	5.5	-	-
		"punpckhdq	%%mm7,%%mm7\n"			// mm7 = Y3 Y3 Y3 Y3 Y3 Y3 Y3 Y3			-	-	-	-	-	-	-	7.2

		"pand		8+%4,%%mm3\n"			// mm3 = 00 00 00 Y1 00 Y1 00 Y1			-	-	-	3.3	-	-	-	-
		"paddsw		%%mm2,%%mm5\n"			// mm5 = 00 00 00 R2 00 G2 00 B2			-	-	*	-	-	5.6	-	-
	
		"pand		8+%4,%%mm7\n"			// mm7 = 00 00 00 Y3 00 Y3 00 Y3			-	-	-	-	-	-	-	7.3
		"paddsw		%%mm0,%%mm3\n"			// mm3 = 00 00 00 R1 00 G1 00 B1			*	-	-	3.4	-	-	-	-
		
		"packuswb	%%mm3,%%mm1\n"			// mm1 = 00 R1 G1 B1 00 R0 B0 G0			-	1.9	-	3.5	-	-	-	-
		"paddsw		%%mm2,%%mm7\n"			// mm7 = 00 00 00 R3 00 G3 00 B3			-	-	*	-	-	-	-	7.4
		
		"packuswb	%%mm7,%%mm5\n"			// mm5 = 00 R3 G3 B3 00 R2 B2 G2			-	-	-	-	-	5.7	-	7.5
		
		"movq		%%mm1,0x00(%0)\n"		// destination[0] = mm1						-	1.A	-	-	-	-	-	-
		"movq		%%mm5,0x08(%0)\n"		// destination[1] = mm5						-	-	-	-	-	5.8	-	-

		/* odd row (4-7) */
		"movq		(%8),%%mm1\n"			// mm1 = Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0			-	1.1	-	-	-	-	-	-
		"punpckhbw	%%mm1,%%mm1\n"			// mm1 = Y7 Y7 Y6 Y6 Y5 Y5 Y4 Y4			-	1.2	-	-	-	-	-	-
		"movq		%%mm1,%%mm5\n"			// mm5 = Y7 Y7 Y6 Y6 Y5 Y5 Y4 Y4			-	1.3	-	-	-	5.1	-	-
		"punpcklwd	%%mm1,%%mm1\n"			// mm1 = Y5 Y5 Y5 Y5 Y4 Y4 Y4 Y4			-	1.4	-	-	-	-	-	-
		
		"punpckhwd	%%mm5,%%mm5\n"			// mm5 = Y7 Y7 Y7 Y7 Y6 Y6 Y6 Y6			-	-	-	-	-	5.2	-	-
		"movq		%%mm1,%%mm3\n"			// mm3 = Y5 Y5 Y5 Y5 Y4 Y4 Y4 Y4			-	1.5	-	3.1	-	-	-	-
		
		"movq		%%mm5,%%mm7\n"			// mm7 = Y7 Y7 Y7 Y7 Y6 Y6 Y6 Y6			-	-	-	-	-	5.3	-	7.1
		"punpckldq	%%mm1,%%mm1\n"			// mm1 = Y4 Y4 Y4 Y4 Y4 Y4 Y4 Y4			-	1.6	-	-	-	-	-	-
		
		"punpckldq	%%mm5,%%mm5\n"			// mm5 = Y6 Y6 Y6 Y6 Y6 Y6 Y6 Y6			-	-	-	-	-	5.4	-	-
		"pand		8+%4,%%mm1\n"			// mm1 = 00 00 00 Y4 00 Y4 00 Y4			-	1.7	-	-	-	-	-	-

		"punpckhdq	%%mm3,%%mm3\n"			// mm3 = Y5 Y5 Y5 Y5 Y5 Y5 Y5 Y5			-	-	-	3.2	-	-	-	-
		"paddsw		%%mm4,%%mm1\n"			// mm1 = 00 00 00 R4 00 G4 00 B4			*	1.8	-	-	*	-	-	-
		
		"pand		8+%4,%%mm5\n"			// mm5 = 00 00 00 Y6 00 Y6 00 Y6			-	-	-	-	-	5.5	-	-
		"punpckhdq	%%mm7,%%mm7\n"			// mm7 = Y7 Y7 Y7 Y7 Y7 Y7 Y7 Y7			-	-	-	-	-	-	-	7.2

		"pand		8+%4,%%mm3\n"			// mm3 = 00 00 00 Y5 00 Y5 00 Y5			-	-	-	3.3	-	-	-	-
		"paddsw		%%mm6,%%mm5\n"			// mm5 = 00 00 00 R6 00 G6 00 B6			-	-	*	-	-	5.6	*	-
	
		"pand		8+%4,%%mm7\n"			// mm7 = 00 00 00 Y7 00 Y7 00 Y7			-	-	-	-	-	-	-	7.3
		"paddsw		%%mm4,%%mm3\n"			// mm3 = 00 00 00 R5 00 G5 00 B5			*	-	-	3.4	*	-	-	-
		
		"packuswb	%%mm3,%%mm1\n"			// mm1 = 00 R5 G5 B5 00 R4 B4 G4			-	1.9	-	3.5	-	-	-	-
		"paddsw		%%mm6,%%mm7\n"			// mm7 = 00 00 00 R7 00 G7 00 B7			-	-	*	-	-	-	*	7.4
		
		"packuswb	%%mm7,%%mm5\n"			// mm5 = 00 R7 G7 B7 00 R6 B6 G6			-	-	-	-	-	5.7	-	7.5
		
		"movq		%%mm1,0x10(%0)\n"		// destination[0] = mm1						-	1.A	-	-	-	-	-	-
		"movq		%%mm5,0x18(%0)\n"		// destination[1] = mm5						-	-	-	-	-	5.8	-	-

		"addl		(%%esp),%8\n"

		/* even row (0-3) */
		"movq		(%8),%%mm1\n"			// mm1 = Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0			-	1.1	-	-	-	-	-	-
		"punpcklbw	%%mm1,%%mm1\n"			// mm1 = Y3 Y3 Y2 Y2 Y1 Y1 Y0 Y0			-	1.2	-	-	-	-	-	-
		"movq		%%mm1,%%mm5\n"			// mm5 = Y3 Y3 Y2 Y2 Y1 Y1 Y0 Y0			-	1.3	-	-	-	5.1	-	-
		"punpcklwd	%%mm1,%%mm1\n"			// mm1 = Y1 Y1 Y1 Y1 Y0 Y0 Y0 Y0			-	1.4	-	-	-	-	-	-
		
		"punpckhwd	%%mm5,%%mm5\n"			// mm5 = Y3 Y3 Y3 Y3 Y2 Y2 Y2 Y2			-	-	-	-	-	5.2	-	-
		"movq		%%mm1,%%mm3\n"			// mm3 = Y1 Y1 Y1 Y1 Y0 Y0 Y0 Y0			-	1.5	-	3.1	-	-	-	-
		
		"movq		%%mm5,%%mm7\n"			// mm7 = Y3 Y3 Y3 Y3 Y2 Y2 Y2 Y2			-	-	-	-	-	5.3	-	7.1
		"punpckldq	%%mm1,%%mm1\n"			// mm1 = Y0 Y0 Y0 Y0 Y0 Y0 Y0 Y0			-	1.6	-	-	-	-	-	-
		
		"punpckldq	%%mm5,%%mm5\n"			// mm5 = Y2 Y2 Y2 Y2 Y2 Y2 Y2 Y2			-	-	-	-	-	5.4	-	-
		"pand		8+%4,%%mm1\n"			// mm1 = 00 00 00 Y0 00 Y0 00 Y0			-	1.7	-	-	-	-	-	-

		"punpckhdq	%%mm3,%%mm3\n"			// mm3 = Y1 Y1 Y1 Y1 Y1 Y1 Y1 Y1			-	-	-	3.2	-	-	-	-
		"paddsw		%%mm0,%%mm1\n"			// mm1 = 00 00 00 R0 00 G0 00 B0			*	1.8	-	-	*	-	-	-
		
		"pand		8+%4,%%mm5\n"			// mm5 = 00 00 00 Y2 00 Y2 00 Y2			-	-	-	-	-	5.5	-	-
		"punpckhdq	%%mm7,%%mm7\n"			// mm7 = Y3 Y3 Y3 Y3 Y3 Y3 Y3 Y3			-	-	-	-	-	-	-	7.2

		"pand		8+%4,%%mm3\n"			// mm3 = 00 00 00 Y1 00 Y1 00 Y1			-	-	-	3.3	-	-	-	-
		"paddsw		%%mm2,%%mm5\n"			// mm5 = 00 00 00 R2 00 G2 00 B2			-	-	*	-	-	5.6	*	-
	
		"pand		8+%4,%%mm7\n"			// mm7 = 00 00 00 Y3 00 Y3 00 Y3			-	-	-	-	-	-	-	7.3
		"paddsw		%%mm0,%%mm3\n"			// mm3 = 00 00 00 R1 00 G1 00 B1			*	-	-	3.4	*	-	-	-
		
		"packuswb	%%mm3,%%mm1\n"			// mm1 = 00 R1 G1 B1 00 R0 B0 G0			-	1.9	-	3.5	-	-	-	-
		"paddsw		%%mm2,%%mm7\n"			// mm7 = 00 00 00 R3 00 G3 00 B3			-	-	*	-	-	-	*	7.4
		
		"packuswb	%%mm7,%%mm5\n"			// mm5 = 00 R3 G3 B3 00 R2 B2 G2			-	-	-	-	-	5.7	-	7.5
		
		"movq		%%mm1,0x00(%0,%3)\n"	// destination[0] = mm1						-	1.A	-	-	-	-	-	-
		"movq		%%mm5,0x08(%0,%3)\n"	// destination[1] = mm5						-	-	-	-	-	5.8	-	-

		/* even row (4-7) */
		"movq		(%8),%%mm1\n"			// mm1 = Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0			-	1.1	-	-	-	-	-	-
		"punpckhbw	%%mm1,%%mm1\n"			// mm1 = Y7 Y7 Y6 Y6 Y5 Y5 Y4 Y4			-	1.2	-	-	-	-	-	-
		"movq		%%mm1,%%mm5\n"			// mm5 = Y7 Y7 Y6 Y6 Y5 Y5 Y4 Y4			-	1.3	-	-	-	5.1	-	-
		"punpcklwd	%%mm1,%%mm1\n"			// mm1 = Y5 Y5 Y5 Y5 Y4 Y4 Y4 Y4			-	1.4	-	-	-	-	-	-
		
		"punpckhwd	%%mm5,%%mm5\n"			// mm5 = Y7 Y7 Y7 Y7 Y6 Y6 Y6 Y6			-	-	-	-	-	5.2	-	-
		"movq		%%mm1,%%mm3\n"			// mm3 = Y5 Y5 Y5 Y5 Y4 Y4 Y4 Y4			-	1.5	-	3.1	-	-	-	-
		
		"movq		%%mm5,%%mm7\n"			// mm7 = Y7 Y7 Y7 Y7 Y6 Y6 Y6 Y6			-	-	-	-	-	5.3	-	7.1
		"punpckldq	%%mm1,%%mm1\n"			// mm1 = Y4 Y4 Y4 Y4 Y4 Y4 Y4 Y4			-	1.6	-	-	-	-	-	-
		
		"punpckldq	%%mm5,%%mm5\n"			// mm5 = Y6 Y6 Y6 Y6 Y6 Y6 Y6 Y6			-	-	-	-	-	5.4	-	-
		"pand		8+%4,%%mm1\n"			// mm1 = 00 00 00 Y4 00 Y4 00 Y4			-	1.7	-	-	-	-	-	-

		"punpckhdq	%%mm3,%%mm3\n"			// mm3 = Y5 Y5 Y5 Y5 Y5 Y5 Y5 Y5			-	-	-	3.2	-	-	-	-
		"paddsw		%%mm4,%%mm1\n"			// mm1 = 00 00 00 R4 00 G4 00 B4			*	1.8	-	-	*	-	-	-
		
		"pand		8+%4,%%mm5\n"			// mm5 = 00 00 00 Y6 00 Y6 00 Y6			-	-	-	-	-	5.5	-	-
		"punpckhdq	%%mm7,%%mm7\n"			// mm7 = Y7 Y7 Y7 Y7 Y7 Y7 Y7 Y7			-	-	-	-	-	-	-	7.2

		"pand		8+%4,%%mm3\n"			// mm3 = 00 00 00 Y5 00 Y5 00 Y5			-	-	-	3.3	-	-	-	-
		"paddsw		%%mm6,%%mm5\n"			// mm5 = 00 00 00 R6 00 G6 00 B6			-	-	*	-	-	5.6	*	-
	
		"pand		8+%4,%%mm7\n"			// mm7 = 00 00 00 Y7 00 Y7 00 Y7			-	-	-	-	-	-	-	7.3
		"paddsw		%%mm4,%%mm3\n"			// mm3 = 00 00 00 R5 00 G5 00 B5			*	-	-	3.4	*	-	-	-
		
		"packuswb	%%mm3,%%mm1\n"			// mm1 = 00 R5 G5 B5 00 R4 B4 G4			-	1.9	-	3.5	-	-	-	-
		"paddsw		%%mm6,%%mm7\n"			// mm7 = 00 00 00 R7 00 G7 00 B7			-	-	*	-	-	-	*	7.4
		
		"packuswb	%%mm7,%%mm5\n"			// mm5 = 00 R7 G7 B7 00 R6 B6 G6			-	-	-	-	-	5.7	-	7.5
		
		"movq		%%mm1,0x10(%0,%3)\n"	// destination[0] = mm1						-	1.A	-	-	-	-	-	-
		"movq		%%mm5,0x18(%0,%3)\n"	// destination[1] = mm5						-	-	-	-	-	5.8	-	-


		"subl		(%%esp),%8\n"

		/* next */		
		"addl		$32,%0\n"
		"addl		$4,%1\n"
		"addl		$4,%2\n"
		"addl		$8,%8\n"
		
		"subl		$8,%6\n"
		"jg			1b\n"
		
		"popl		%6\n"
		"popl		%0\n"		

		"addl		%6,%8\n"
		"leal		(%0,%3,2),%0\n"
		
		"subl		$2,%7\n"
		"jg			2b\n"

		"emms\n"
		:
		: "r" (destination), "r" (Cb), "r" (Cr), "r" (stride),
		  "m" (delta), "m" (scale), "m" (width), "m" (height), "r" (Y)
	);
}
