/* ++++++++++
	FILE:	interrupts.h
	REVS:	$Revision$
	NAME:	herold
	DATE:	Tue Mar 19 17:30:40 PST 1996
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

/* ---
	hardware-independent ids for the various interrupt sources
--- */
/* ---
	hardware-independent interrupt numbers
--- */

#define B_INT_IRQ1		1
#define B_INT_IRQ3		3
#define B_INT_IRQ4		4
#define B_INT_IRQ5		5
#define B_INT_IRQ6		6
#define B_INT_IRQ7		7
#define B_INT_IRQ8		8
#define B_INT_IRQ9		9
#define B_INT_IRQ10		10
#define B_INT_IRQ11		11
#define B_INT_IRQ12		12
#define B_INT_IRQ14		14
#define B_INT_IRQ15		15

#define B_INT_SERIAL3	16
#define B_INT_SERIAL4	17
#define B_INT_MIDI1		18
#define B_INT_MIDI2		19
#define B_INT_SCSI		20
#define B_INT_PCI1		21
#define B_INT_PCI2		22
#define B_INT_PCI3		23
#define B_INT_SND		24
#define B_INT_8259		25
#define B_INT_IR		26
#define B_INT_A2D		27
#define B_INT_GEEK		28

#define	B_INT_ICI		29

#define B_INT_INVALID	255

#define B_INT_COM1		(B_INT_IRQ4)
#define	B_INT_COM2		(B_INT_IRQ3)
#define B_INT_KBD		(B_INT_IRQ1)
#define B_INT_RTC		(B_INT_IRQ8)
#define B_INT_FDC		(B_INT_IRQ6)
#define B_INT_LPT		(B_INT_IRQ7)
#define B_INT_MOUSE		(B_INT_IRQ12)
#define B_INT_IDE		(B_INT_IRQ14)

#endif

