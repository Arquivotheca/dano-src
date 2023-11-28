/* ++++++++++
	FILE:	asm2.c
	REVS:	$Revision: 1.2 $
	NAME:	herold
	DATE:	Fri Jan 19 16:13:34 PST 1996
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

extern long tabgauche;
extern long tabdroite;
extern long OffScreen_largeur;
extern long OffScreen_baseAddr;

//** global registers
#define RTABPTS			r3
#define RCOLOR			r4
#define RMIN			r31
#define RMAX			r30
#define HMIN			r29
#define LMIN			r28
//** First part registers
#define RX1				r27
#define RX2				r26
#define RDELTAX			r25
#define RDELTAY			r24
#define RTEMP			r23
#define RY1				r22
#define RY2				r21
#define RL1				r20
#define RINDEX			r19
#define RERREUR			r18
#define RJ				r17
#define RINDEXFIN		r16
#define RTABLEAU		r15
#define RTAB			r14
//** Second and third part registers
#define QPASH			r27
#define QPASV			r26
#define QTEMP1			r25
#define QTEMP2			r24
//** Second part registers
#define QH21			r23
#define QH31			r22
#define QV21			r21
#define QV31			r20
#define QL21			r19
#define QL31			r18
//** Third part registers
#define QPASH2			r23
#define QLUM			r22
#define QTABGAUCHE		r21
#define QTABDROITE		r20
#define QLARGEUR		r19
#define QDRAWLINE		r18
#define QX2				r17
#define QX1				r16
#define QDRAW			r15
#define RCOLOR2			r14
//*********************************************************
__asm void
BG_Goureaud (void)
{
				stwu	SP,-4*18(SP)
				stmw	r14,0(SP)
				
// copy the first point at the end of the array
				lwz		RTEMP,0(RTABPTS)
				mr		RTAB,RTABPTS
				addi	RINDEXFIN,RTAB,3*6
				stw		RTEMP,0(RINDEXFIN)

// init line range min and max
				li		RMIN,1999
				li		RMAX,0
// processing of one line
@b1				lhz		RY1,2(RTAB)
				lhz		RY2,8(RTAB)
				addi	RTAB,RTAB,6
				cmpw	cr2,RY1,RY2
// preprocessing of test end of point list
				cmpw	cr6,RTAB,RINDEXFIN
				beq		cr2,@boucle
				bgt		cr2,@tabgauche
				lhz		RX1,-6(RTAB)
				lhz		RX2,0(RTAB)
				lhz		RL1,-2(RTAB)
				lwz		RTABLEAU,tabdroite(RTOC)
				b		@tabdroite
				
@tabgauche		lhz		RX2,-6(RTAB)	//permut the points
				lhz		RX1,0(RTAB)
				lhz		RY2,-4(RTAB)
				lhz		RY1,2(RTAB)
				lhz		RL1,4(RTAB)
				lwz		RTABLEAU,tabgauche(RTOC)
@tabdroite
// processing of the delta vector
				sub		RDELTAX,RX1,RX2
				sub		RDELTAY,RY2,RY1
				mr		RJ,RDELTAY
				
// preprocessing of min and max
				cmpw	cr2,RY1,RMIN
				cmpw	cr3,RY2,RMAX
// preprocessing of deltaY >= 4
				cmpwi	cr5,RJ,4
// preprocessing of odd deltaY, and double-odd deltaY
				andi.	RTEMP,RJ,2
				cmpwi	cr4,RTEMP,2
				andi.	RTEMP,RJ,1

// get a pointer on the first case in the array
//				slwi	RINDEX,RY1,1
				rlwinm	RINDEX,RY1,1, 0, 30
				add		RINDEX,RINDEX,RTABLEAU				
// processing of the horizontal increment per line
//				slwi	RERREUR,RDELTAX,16
				rlwinm	RERREUR,RDELTAX,16, 0, 15
				divw	RERREUR,RERREUR,RDELTAY
// processing of min and max 				
				bge		cr2,@non1
				mr		RMIN,RY1
				mr		HMIN,RX1
				mr		LMIN,RL1
@non1			ble		cr3,@non2
				mr		RMAX,RY2
@non2
// processing of the horizontal increment per line (end)
//				slwi	RX1,RX1,16
				rlwinm	RX1,RX1,16, 0, 15

// do a first single step in case of odd deltaY
				beq		@tobd1
				subi	RJ,RJ,1
				srawi	RTEMP,RX1,16
				sub		RX1,RX1,RERREUR
				sth		RTEMP,0(RINDEX)
				addi	RINDEX,RINDEX,2
// processing of the next two in case of odd deltaY/2 
@tobd1			bne		cr4,@gobd1
				subi	RJ,RJ,2
				sub		RTEMP,RX1,RERREUR
				rlwimi	RX1,RTEMP,16,16,31
				stw		RX1,0(RINDEX)
				sub		RX1,RTEMP,RERREUR
				addi	RINDEX,RINDEX,4

// processing of the next four 
@gobd1			blt		cr5,@boucle	
				subic.	RJ,RJ,4
				sub		RTEMP,RX1,RERREUR
				rlwimi	RX1,RTEMP,16,16,31
				stw		RX1,0(RINDEX)
				sub		RX1,RTEMP,RERREUR
				sub		RTEMP,RX1,RERREUR
				rlwimi	RX1,RTEMP,16,16,31
				stwu	RX1,+4(RINDEX)
				sub		RX1,RTEMP,RERREUR
				beq		@boucle	
				
// processing of all the others four by four 
@bd1			subic.	RJ,RJ,4
				sub		RTEMP,RX1,RERREUR
				rlwimi	RX1,RTEMP,16,16,31
				stwu	RX1,+4(RINDEX)
				sub		RX1,RTEMP,RERREUR
				sub		RTEMP,RX1,RERREUR
				rlwimi	RX1,RTEMP,16,16,31
				stwu	RX1,+4(RINDEX)
				sub		RX1,RTEMP,RERREUR
				bne		@bd1
// next line
@boucle			blt		cr6,@b1

//****************************************************************************				
//				goureaud of the triangle
//****************************************************************************				
				cmpw	RMAX,RMIN
				ble		@notrace

// processing of the local goureaud vector
				lhz		QTEMP1,0(RTABPTS)		//H1
				lhz		QH21,6(RTABPTS)			//H2
				lhz		QH31,12(RTABPTS)		//H3
				sub		QH21,QH21,QTEMP1		//H2-H1
				sub		QH31,QH31,QTEMP1		//H3-H1
				lhz		QTEMP1,2(RTABPTS)		//V1
				lhz		QV21,8(RTABPTS)			//V2
				lhz		QV31,14(RTABPTS)		//V3
				sub		QV21,QV21,QTEMP1		//V2-V1
				sub		QV31,QV31,QTEMP1		//V3-V1
				lhz		QTEMP1,4(RTABPTS)		//L1
				lhz		QL21,10(RTABPTS)		//L2
				lhz		QL31,16(RTABPTS)		//L3
				sub		QL21,QL21,QTEMP1		//L2-L1
				sub		QL31,QL31,QTEMP1		//L3-L1
				mullw	QPASH,QL21,QV31			//L21*V31
				mullw	QTEMP1,QL31,QV21		//L31*V21
				sub		QPASH,QPASH,QTEMP1		//L21*V31-L31*V21
				mullw	QPASV,QL21,QH31			//L21*H31
				mullw	QTEMP1,QL31,QH21		//L31*H21
				sub		QPASV,QTEMP1,QPASV		//L31*H21-L21*H31
				mullw	QTEMP1,QV21,QH31		//H31*V21
				mullw	QTEMP2,QV31,QH21		//H21*V31
				sub.	QTEMP1,QTEMP2,QTEMP1	//H21*V31-H31*V21
				beq		@notrace
//				slwi	QPASH,QPASH,12
				rlwinm	QPASH,QPASH,12, 0, 19
				divw	QPASH,QPASH,QTEMP1		//PasH<<12
//				slwi	QPASV,QPASV,12
				rlwinm	QPASV,QPASV,12, 0, 19
				divw	QPASV,QPASV,QTEMP1		//PasV<<12
				mullw	HMIN,QPASH,HMIN
//				slwi	LMIN,LMIN,12
				rlwinm	LMIN,LMIN,12, 0, 19
				sub		LMIN,LMIN,HMIN			//LMIN0
//				slwi	QPASH2,QPASH,1			//PasH<<13
				rlwinm	QPASH2,QPASH,1, 0, 30	//PasH<<13

// preprocessing of first line parity selector
				andi.	QTEMP1,RMIN,2
//*************************************************************				
// goureaud the horizontal segments one by one.				
				lwz		QTABGAUCHE,tabgauche(RTOC)
				lwz		QTABDROITE,tabdroite(RTOC)
				lwz		QLARGEUR,OffScreen_largeur(RTOC)
				lwz		QDRAWLINE,OffScreen_baseAddr(RTOC)
				lwz		QLARGEUR,0(QLARGEUR)
				lwz		QDRAWLINE,0(QDRAWLINE)
// processing of the second tramage color pointer
				addi	RCOLOR2,RCOLOR,512
// get a pointeur on the left border of the RMIN line minus 4 for 'stwu +4()' use.
				mullw	QTEMP1,QLARGEUR,RMIN
				add		QDRAWLINE,QDRAWLINE,QTEMP1
				subi	QDRAWLINE,QDRAWLINE,4
// convert number of line to index in array
				subi	RMAX,RMAX,1		// minus 1 for anticipate test
//				slwi	RMAX,RMAX,1		
				rlwinm	RMAX,RMAX,1, 0, 30
//				slwi	RMIN,RMIN,1
				rlwinm	RMIN,RMIN,1, 0, 30
				bne		@Xline0			// first line parity separator
// process one line (parity 0)
@line0			
	// preprocessing of end test
				cmpw	cr5,RMIN,RMAX
				lhax	QX2,QTABGAUCHE,RMIN
				lhax	QX1,QTABDROITE,RMIN
				add		QDRAW,QDRAWLINE,QX2
				sub.	QX1,QX1,QX2
// preprocessing of the 4 by 4 loop
				cmpwi	cr4,QX1,4
// processing of the next line
				add		QDRAWLINE,QDRAWLINE,QLARGEUR
				addi	RMIN,RMIN,2
// break inverted line
				ble		@XNextLine
// preprocessing of the end drawing
				andi.	QTEMP2,QX1,2
				cmpwi	cr3,QTEMP2,2
				andi.	QTEMP2,QX1,1
				cmpwi	cr2,QTEMP2,1
// Level of the left pixel of the segment
				mullw	QTEMP2,QPASH,QX2
				add		QLUM,LMIN,QTEMP2
// draw pixel 4 by 4
				blt		cr4,@DrawEnd
@Next4			cmpwi	cr4,QX1,8
				subi	QX1,QX1,4
				rlwinm	QTEMP1,QLUM,21,23,30
				lhzx	QTEMP1,RCOLOR,QTEMP1
				add		QLUM,QLUM,QPASH2
				rlwinm	QTEMP2,QLUM,21,23,30
				lhzx	QTEMP2,RCOLOR,QTEMP2
				add		QLUM,QLUM,QPASH2
				rlwimi	QTEMP2,QTEMP1,16,0,15
				stwu	QTEMP2,4(QDRAW)
				bge		cr4,@Next4
// draw the last 2 or 3 pixel if necessary
@DrawEnd		bne		cr3,@DrawLast	
				rlwinm	QTEMP2,QLUM,21,23,30
				lhzx	QTEMP2,RCOLOR,QTEMP2
				add		QLUM,QLUM,QPASH2
				sth		QTEMP2,4(QDRAW)
				bne		cr2,@XNextLine					
				rlwinm	QTEMP1,QLUM,21,23,30
				lbzx	QTEMP1,RCOLOR,QTEMP1
				stb		QTEMP1,6(QDRAW)
				b		@XNextLine
// draw the last pixel if necessary
@DrawLast		bne		cr2,@XNextLine					
				rlwinm	QTEMP1,QLUM,21,23,30
				lbzx	QTEMP1,RCOLOR,QTEMP1
				stb		QTEMP1,4(QDRAW)
				b		@XNextLine
// next line				
@NextLine		add		LMIN,LMIN,QPASV
				bne		cr5,@line0
				b		@notrace

// process one line (parity 1)
@Xline0			
	// preprocessing of end test
				cmpw	cr5,RMIN,RMAX
				lhax	QX2,QTABGAUCHE,RMIN
				lhax	QX1,QTABDROITE,RMIN
				add		QDRAW,QDRAWLINE,QX2
				sub.	QX1,QX1,QX2
// preprocessing of the 4 by 4 loop
				cmpwi	cr4,QX1,4
// processing of the next line
				add		QDRAWLINE,QDRAWLINE,QLARGEUR
				addi	RMIN,RMIN,2
// break inverted line
				ble		@NextLine
// preprocessing of the end drawing
				andi.	QTEMP2,QX1,2
				cmpwi	cr3,QTEMP2,2
				andi.	QTEMP2,QX1,1
				cmpwi	cr2,QTEMP2,1
// Level of the left pixel of the segment
				mullw	QTEMP2,QPASH,QX2
				add		QLUM,LMIN,QTEMP2
// draw pixel 4 by 4
				blt		cr4,@XDrawEnd
@XNext4			cmpwi	cr4,QX1,8
				subi	QX1,QX1,4
				rlwinm	QTEMP1,QLUM,21,23,30
				lhzx	QTEMP1,RCOLOR2,QTEMP1
				add		QLUM,QLUM,QPASH2
				rlwinm	QTEMP2,QLUM,21,23,30
				lhzx	QTEMP2,RCOLOR2,QTEMP2
				add		QLUM,QLUM,QPASH2
				rlwimi	QTEMP2,QTEMP1,16,0,15
				stwu	QTEMP2,4(QDRAW)
				bge		cr4,@XNext4
// draw the last 2 or 3 pixel if necessary
@XDrawEnd		bne		cr3,@XDrawLast	
				rlwinm	QTEMP2,QLUM,21,23,30
				lhzx	QTEMP2,RCOLOR2,QTEMP2
				add		QLUM,QLUM,QPASH2
				sth		QTEMP2,4(QDRAW)
				bne		cr2,@NextLine					
				rlwinm	QTEMP1,QLUM,21,23,30
				lbzx	QTEMP1,RCOLOR2,QTEMP1
				stb		QTEMP1,6(QDRAW)
				b		@NextLine
// draw the last pixel if necessary
@XDrawLast		bne		cr2,@NextLine					
				rlwinm	QTEMP1,QLUM,21,23,30
				lbzx	QTEMP1,RCOLOR2,QTEMP1
				stb		QTEMP1,4(QDRAW)
				b		@NextLine
// next line				
@XNextLine		add		LMIN,LMIN,QPASV
				bne		cr5,@Xline0	
//;*********************************************
// end of goureaud process				
@notrace		lmw		r14,0(SP)
				stwu	SP,4*18(SP)
				blr
}

//*********************************************************
__asm void
BG_Goureaud24 (void)
{
				stwu	SP,-4*18(SP)
				stmw	r14,0(SP)
				
// copy the first point at the end of the array
				lwz		RTEMP,0(RTABPTS)
				mr		RTAB,RTABPTS
				addi	RINDEXFIN,RTAB,3*6
				stw		RTEMP,0(RINDEXFIN)

// init line range min and max
				li		RMIN,1999
				li		RMAX,0
// processing of one line
@b1				lhz		RY1,2(RTAB)
				lhz		RY2,8(RTAB)
				addi	RTAB,RTAB,6
				cmpw	cr2,RY1,RY2
// preprocessing of test end of point list
				cmpw	cr6,RTAB,RINDEXFIN
				beq		cr2,@boucle
				bgt		cr2,@tabgauche
				lhz		RX1,-6(RTAB)
				lhz		RX2,0(RTAB)
				lhz		RL1,-2(RTAB)
				lwz		RTABLEAU,tabdroite(RTOC)
				b		@tabdroite
				
@tabgauche		lhz		RX2,-6(RTAB)	//permut the points
				lhz		RX1,0(RTAB)
				lhz		RY2,-4(RTAB)
				lhz		RY1,2(RTAB)
				lhz		RL1,4(RTAB)
				lwz		RTABLEAU,tabgauche(RTOC)
@tabdroite
// processing of the delta vector
				sub		RDELTAX,RX1,RX2
				sub		RDELTAY,RY2,RY1
				mr		RJ,RDELTAY
				
// preprocessing of min and max
				cmpw	cr2,RY1,RMIN
				cmpw	cr3,RY2,RMAX
// preprocessing of deltaY >= 4
				cmpwi	cr5,RJ,4
// preprocessing of odd deltaY, and double-odd deltaY
				andi.	RTEMP,RJ,2
				cmpwi	cr4,RTEMP,2
				andi.	RTEMP,RJ,1

// get a pointer on the first case in the array
//				slwi	RINDEX,RY1,1
				rlwinm	RINDEX,RY1,1, 0, 30
				add		RINDEX,RINDEX,RTABLEAU				
// processing of the horizontal increment per line
//				slwi	RERREUR,RDELTAX,16
				rlwinm	RERREUR,RDELTAX,16, 0, 15
				divw	RERREUR,RERREUR,RDELTAY
// processing of min and max 				
				bge		cr2,@non1
				mr		RMIN,RY1
				mr		HMIN,RX1
				mr		LMIN,RL1
@non1			ble		cr3,@non2
				mr		RMAX,RY2
@non2
// processing of the horizontal increment per line (end)
//				slwi	RX1,RX1,16
				rlwinm	RX1,RX1,16, 0, 15

// do a first single step in case of odd deltaY
				beq		@tobd1
				subi	RJ,RJ,1
				srawi	RTEMP,RX1,16
				sub		RX1,RX1,RERREUR
				sth		RTEMP,0(RINDEX)
				addi	RINDEX,RINDEX,2
// processing of the next two in case of odd deltaY/2 
@tobd1			bne		cr4,@gobd1
				subi	RJ,RJ,2
				sub		RTEMP,RX1,RERREUR
				rlwimi	RX1,RTEMP,16,16,31
				stw		RX1,0(RINDEX)
				sub		RX1,RTEMP,RERREUR
				addi	RINDEX,RINDEX,4

// processing of the next four 
@gobd1			blt		cr5,@boucle	
				subic.	RJ,RJ,4
				sub		RTEMP,RX1,RERREUR
				rlwimi	RX1,RTEMP,16,16,31
				stw		RX1,0(RINDEX)
				sub		RX1,RTEMP,RERREUR
				sub		RTEMP,RX1,RERREUR
				rlwimi	RX1,RTEMP,16,16,31
				stwu	RX1,+4(RINDEX)
				sub		RX1,RTEMP,RERREUR
				beq		@boucle	
				
// processing of all the others four by four 
@bd1			subic.	RJ,RJ,4
				sub		RTEMP,RX1,RERREUR
				rlwimi	RX1,RTEMP,16,16,31
				stwu	RX1,+4(RINDEX)
				sub		RX1,RTEMP,RERREUR
				sub		RTEMP,RX1,RERREUR
				rlwimi	RX1,RTEMP,16,16,31
				stwu	RX1,+4(RINDEX)
				sub		RX1,RTEMP,RERREUR
				bne		@bd1
// next line
@boucle			blt		cr6,@b1

//****************************************************************************				
//				goureaud of the triangle
//****************************************************************************				
				cmpw	RMAX,RMIN
				ble		@notrace

// processing of the local goureaud vector
				lhz		QTEMP1,0(RTABPTS)		//H1
				lhz		QH21,6(RTABPTS)			//H2
				lhz		QH31,12(RTABPTS)		//H3
				sub		QH21,QH21,QTEMP1		//H2-H1
				sub		QH31,QH31,QTEMP1		//H3-H1
				lhz		QTEMP1,2(RTABPTS)		//V1
				lhz		QV21,8(RTABPTS)			//V2
				lhz		QV31,14(RTABPTS)		//V3
				sub		QV21,QV21,QTEMP1		//V2-V1
				sub		QV31,QV31,QTEMP1		//V3-V1
				lhz		QTEMP1,4(RTABPTS)		//L1
				lhz		QL21,10(RTABPTS)		//L2
				lhz		QL31,16(RTABPTS)		//L3
				sub		QL21,QL21,QTEMP1		//L2-L1
				sub		QL31,QL31,QTEMP1		//L3-L1
				mullw	QPASH,QL21,QV31			//L21*V31
				mullw	QTEMP1,QL31,QV21		//L31*V21
				sub		QPASH,QPASH,QTEMP1		//L21*V31-L31*V21
				mullw	QPASV,QL21,QH31			//L21*H31
				mullw	QTEMP1,QL31,QH21		//L31*H21
				sub		QPASV,QTEMP1,QPASV		//L31*H21-L21*H31
				mullw	QTEMP1,QV21,QH31		//H31*V21
				mullw	QTEMP2,QV31,QH21		//H21*V31
				sub.	QTEMP1,QTEMP2,QTEMP1	//H21*V31-H31*V21
				beq		@notrace
//				slwi	QPASH,QPASH,12
				rlwinm	QPASH,QPASH,12, 0, 19
				divw	QPASH,QPASH,QTEMP1		//PasH<<12
//				slwi	QPASV,QPASV,12
				rlwinm	QPASV,QPASV,12, 0, 19
				divw	QPASV,QPASV,QTEMP1		//PasV<<12
				mullw	HMIN,QPASH,HMIN
//				slwi	LMIN,LMIN,12
				rlwinm	LMIN,LMIN,12, 0, 19
				sub		LMIN,LMIN,HMIN			//LMIN0
//				slwi	QPASH2,QPASH,1			//PasH<<13
				rlwinm	QPASH2,QPASH,1, 0, 30	//PasH<<13

//*************************************************************				
// goureaud the horizontal segments one by one.				
				lwz		QTABGAUCHE,tabgauche(RTOC)
				lwz		QTABDROITE,tabdroite(RTOC)
				lwz		QLARGEUR,OffScreen_largeur(RTOC)
				lwz		QDRAWLINE,OffScreen_baseAddr(RTOC)
				lwz		QLARGEUR,0(QLARGEUR)
				lwz		QDRAWLINE,0(QDRAWLINE)
//				addi    QLARGEUR,QLARGEUR,4
// get a pointeur on the left border of the RMIN line minus 4 for 'stwu +4()' use.
				mullw	QTEMP1,QLARGEUR,RMIN
				add		QDRAWLINE,QDRAWLINE,QTEMP1
				subi	QDRAWLINE,QDRAWLINE,4
// convert number of line to index in array
				subi	RMAX,RMAX,1		// minus 1 for anticipate test
				rlwinm	RMAX,RMAX,1, 0, 30
				rlwinm	RMIN,RMIN,1, 0, 30
// process one line (parity 0)
@line0			
	// preprocessing of end test
				cmpw	cr5,RMIN,RMAX
				lhax	QX2,QTABGAUCHE,RMIN
				lhax	QX1,QTABDROITE,RMIN
				rlwinm  QTEMP1,QX2,2,0,29
				sub.	QX1,QX1,QX2
// preprocessing of big loop
				cmpwi	cr3,QX1,15
				add		QDRAW,QDRAWLINE,QTEMP1
// preprocessing of the 2 by 2 loop
				cmpwi	cr4,QX1,2
// processing of the next line
				add		QDRAWLINE,QDRAWLINE,QLARGEUR
				addi	RMIN,RMIN,2
// break inverted line
				ble		@NextLine
// preprocessing of the end drawing
				andi.	QTEMP2,QX1,1
// Level of the left pixel of the segment
				mullw	QTEMP2,QPASH,QX2
				add		QLUM,LMIN,QTEMP2
// check big line
                blt     cr3,@DrawEnd
				rlwinm  QTEMP1,QDRAW,30,29,31
				xori    QTEMP1,QTEMP1,7
				cmpwi   QTEMP1,0
				sub     QX1,QX1,QTEMP1
				beq     @DrawNext8
@NextF		    subic.	QTEMP1,QTEMP1,1
				rlwinm	QTEMP2,QLUM,22,22,29
				lwzx	QTEMP2,RCOLOR,QTEMP2
				add		QLUM,QLUM,QPASH
				stwu	QTEMP2,4(QDRAW)
				bne		@NextF
@DrawNext8      li      QTEMP2,4
// preprocessing of end loop
				andi.	QTEMP1,QX1,1                
				rlwinm	QTEMP1,QX1,0,29,31                
				cmpwi	cr4,QTEMP1,2
@DrawNext8b     //dcbz    QDRAW,QTEMP2
				subi    QX1,QX1,8
				cmpwi   cr3,QX1,8
				rlwinm	QTEMP1,QLUM,22,22,29
				lwzx	QTEMP1,RCOLOR,QTEMP1
				add		QLUM,QLUM,QPASH
				stwu	QTEMP1,4(QDRAW)
				rlwinm	QTEMP1,QLUM,22,22,29
				lwzx	QTEMP1,RCOLOR,QTEMP1
				add		QLUM,QLUM,QPASH
				stwu	QTEMP1,4(QDRAW)
				rlwinm	QTEMP1,QLUM,22,22,29
				lwzx	QTEMP1,RCOLOR,QTEMP1
				add		QLUM,QLUM,QPASH
				stwu	QTEMP1,4(QDRAW)
				rlwinm	QTEMP1,QLUM,22,22,29
				lwzx	QTEMP1,RCOLOR,QTEMP1
				add		QLUM,QLUM,QPASH
				stwu	QTEMP1,4(QDRAW)
				rlwinm	QTEMP1,QLUM,22,22,29
				lwzx	QTEMP1,RCOLOR,QTEMP1
				add		QLUM,QLUM,QPASH
				stwu	QTEMP1,4(QDRAW)
				rlwinm	QTEMP1,QLUM,22,22,29
				lwzx	QTEMP1,RCOLOR,QTEMP1
				add		QLUM,QLUM,QPASH
				stwu	QTEMP1,4(QDRAW)
				rlwinm	QTEMP1,QLUM,22,22,29
				lwzx	QTEMP1,RCOLOR,QTEMP1
				add		QLUM,QLUM,QPASH
				stwu	QTEMP1,4(QDRAW)
				rlwinm	QTEMP1,QLUM,22,22,29
				lwzx	QTEMP1,RCOLOR,QTEMP1
				add		QLUM,QLUM,QPASH
				stwu	QTEMP1,4(QDRAW)
				bge     cr3,@DrawNext8b
// draw pixel 2 by 2
@DrawEnd		blt		cr4,@DrawLast
@Next2			cmpwi	cr4,QX1,4
				rlwinm	QTEMP1,QLUM,22,22,29
				lwzx	QTEMP1,RCOLOR,QTEMP1
				add		QLUM,QLUM,QPASH
				stwu	QTEMP1,4(QDRAW)
				subi	QX1,QX1,2
				rlwinm	QTEMP2,QLUM,22,22,29
				lwzx	QTEMP2,RCOLOR,QTEMP2
				add		QLUM,QLUM,QPASH
				stwu	QTEMP2,4(QDRAW)
				bge		cr4,@Next2
// draw the last pixel if necessary
@DrawLast		beq		@NextLine					
				rlwinm	QTEMP1,QLUM,22,22,29
				lwzx	QTEMP1,RCOLOR,QTEMP1
				stwu	QTEMP1,4(QDRAW)
				b		@NextLine
// next line				
@NextLine		add		LMIN,LMIN,QPASV
				bne		cr5,@line0
				b		@notrace

//;*********************************************
// end of goureaud process				
@notrace		lmw		r14,0(SP)
				stwu	SP,4*18(SP)
				blr
}












