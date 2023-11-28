/* ++++++++++
	$Source: /net/bally/be/rcs/src/kernel/isa/serial.h,v $
	$Revision: 1.4 $
	$Author: erich $
	$Date: 1995/02/24 17:58:18 $
	Copyright (c) 1994 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _SERIAL_H
#define _SERIAL_H

/* offsets to ISA-standard serial ports */

#define COM1		0x3f8
#define COM2		0x2f8
#define COM3		0x3e8
#define COM4		0x2e8

/* offsets to serial ports, as Be chooses to call them */

#define SERIAL1		0x3f8
#define SERIAL2		0x2f8
#define SERIAL3		0x380
#define SERIAL4		0x388
#define MIDI1		0x3a0
#define MIDI2		0x3a8

/* offsets within a generic ISA-compatible 450/550 UART */

#define COMBASE		0

#define COM_RBR		(COMBASE)
#define COM_THR		(COMBASE)
#define COM_IER		(COMBASE+1)
#define COM_FCR		(COMBASE+2)
#define COM_IIR		(COMBASE+2)
#define COM_LCR		(COMBASE+3)
#define COM_MCR		(COMBASE+4)
#define COM_LSR		(COMBASE+5)
#define COM_MSR		(COMBASE+6)
#define COM_SCR		(COMBASE+7)
#define COM_DLL		(COMBASE)	/* bit 7 of LCR must be set to access this register */
#define COM_DLM		(COMBASE+1)	/* bit 7 of LCR must be set to access this register */

/* bit definitions for IER Interrupt Enable Register */

#define	IER_rcv_data_int	(0x01)	/* enable rcvd data available/time_out int */
#define IER_thre_int		(0x02)	/* enable transmitter holding empty */
#define IER_linestat_int	(0x04)
#define IER_modemstat_int	(0x08)

/* bit definitions for FCR FIFO Control */

#define FCR_fifo_enable		(0x01)
#define FCR_FCR1			(0x02)
#define FCR_FCR2			(0x04)
#define FCR_FCR3			(0x08)
#define FCR_fifo_level_1	(0x00)
#define FCR_fifo_level_4	(0x40)
#define FCR_fifo_level_8	(0x80)
#define FCR_fifo_level_14	(0xC0)

/* bit definitions for IIR */

#define IIR_no_interrupt	(0x01)
#define IIR_thre_int		(0x02)
#define IIR_rcv_data_int	(0x04)
#define	IIR_timeout_int		(0x0C)
#define IIR_linestat_int	(0x06)
#define IIR_modemstat_int	(0x00)
#define IIR_bits			(0x0F)

/* bit definitions for LCR */

#define LCR_5bit			(0x00)
#define LCR_6bit			(0x01)
#define LCR_7bit			(0x02)
#define LCR_8bit			(0x03)
#define LCR_2stop			(0x04)
#define LCR_parity_enable	(0x08)
#define LCR_even_parity		(0x10)
#define LCR_stick_parity	(0x20)
#define LCR_break_enable	(0x40)
#define LCR_divisor_access	(0x80)

/* bit definitions for LSR */

#define LSR_data_ready		(0x01)
#define LSR_overrun			(0x02)
#define LSR_parity			(0x04)
#define LSR_framing			(0x08)
#define LSR_break			(0x10)
#define LSR_thre			(0x20)
#define LSR_temt			(0x40)
#define LSR_rcvr_fifo		(0x80)

/* bit definitions for MCR */

#define MCR_DTR				(0x01)
#define MCR_RTS				(0x02)
#define MCR_OUT1			(0x04)
#define MCR_IRQ_ENABLE		(0x08)
#define MCR_LOOP			(0x10)

/* bit definitions for MSR */

#define MSR_DCTS			(0x01)
#define MSR_DDSR			(0x02)
#define MSR_TERI			(0x04)
#define MSR_DDCD			(0x08)
#define MSR_CTS				(0x10)
#define MSR_DSR				(0x20)
#define MSR_RI				(0x40)
#define MSR_DCD				(0x80)

#endif
