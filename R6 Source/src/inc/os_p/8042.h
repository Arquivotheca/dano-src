/* ++++++++++
	8042.h
	Copyright (C) 1991-96 Be Inc.  All Rights Reserved.
	Defines for the keyboard controller.
+++++ */

#ifndef _8042_H
#define _8042_H

/* offset to ISA compatible keyboard port */

/* #define KB			0x60*/

/* offsets to registers in the keyboard port */

#define KB_STATUS	4
#define KB_COMMAND	4
#define KB_OBUF		0
#define KB_IBUF		0

/* Keyboard controller mode register bits */

#define	KBM_KCC		0x40	/* Keycode conversion 1=enabled */
#define KBM_DMS		0x20	/* Disable mouse 1=disabled */
#define KBM_DKB		0x10	/* Disable keyboard 1=disabled */
#define KBM_SYS		0x04	/* System flag 1=set status (2) = 1 */
#define	KBM_EMI		0x02	/* Enable mouse interrupt 1=enabled */
#define	KBM_EKI		0x01	/* Enable keyboard interrupt 1=enabled */

/* Keyboard controller status register bits */

#define KBS_PERR	0x80	/* Receive parity error 1=error */
#define KBS_GTO		0x40	/* General time out error 1=error */
#define KBS_ODS		0x20	/* Output buffer data source 1=mouse */
#define KBS_KBEN	0x10	/* Keyboard enable switch 1=enabled */
#define KBS_CD		0x08	/* Command/data 1=command/busy 0=data/idle */
#define KBS_SYS		0x04	/* System flag 1=hot reset has occurred */
#define KBS_IBF		0x02	/* Input buffer full 1=full */
#define KBS_OBF		0x01	/* Output buffer full 1=full */

/* Keyboard controller commands */

#define KBC_RDCOMMAND	0x20	/* Read command register */
#define KBC_RDRAM		0x21	/* Read keyboard controller RAM */
#define KBC_WRCOMMAND	0x60	/* Write command register */
#define KBC_WRRAM		0x61	/* Write keyboard controller RAM */
#define KBC_WRNIBBLEIN	0x90	/* Write nibble to input port */
#define KBC_TSTPSWD		0xA4	/* Test password installed */
#define KBC_SETPSWD		0xA5	/* Load password */
#define KBC_ENABLEPWD	0xA6	/* Enable password */
#define KBC_DISABLEAUX	0xA7	/* Disable pointing device */
#define KBC_ENABLEAUX	0xA8	/* Enable pointing device */
#define KBC_TSTAUX		0xA9	/* Pointing device interface test */
#define KBC_SLFTST		0xAA	/* Self test */
#define KBC_KBTST		0xAB	/* Keyboard interface test */
#define KBC_DIAG		0xAC	/* Keyboard diagnostic dump */
#define KBC_DISABLEKB	0xAD	/* Disable keyboard */
#define KBC_ENABLEKB	0xAE	/* Enable Keyboard */
#define KBC_WRNIBBLEOUT	0xB0	/* Write nibble to output port */
#define KBC_RDIP		0xC0	/* Read input port */
#define KBC_POLLIPLO	0xC1	/* Poll input port low */
#define KBC_POLLIPHI	0xC2	/* Poll input port hi */
#define KBC_RDOP		0xD0	/* Read output port */
#define KBC_WROP		0xD1	/* Write output port */
#define KBC_WRKBOB		0xD2	/* Write keyboard output buffer */
#define KBC_WRMOUSEOB	0xD3	/* Write mouse output buffer */
#define KBC_WRMOUSE		0xD4	/* Write to mouse */
#define KBC_RDTST		0xE0	/* Read test inputs */
#define KBC_POP			0xF0	/* Pulse output port */

/* Keyboard controller results */

#define KBC_ACK			0xFA	/* Acknowledge */
#define KBC_RESEND		0xFE	/* Resend */

/* Keyboard commands */

#define	KB_SETLED		0xED	/* Set/reset LEDs */
#define	KB_ECHO			0xEE	/* Echo */
#define	KB_RDID			0xF2	/* Read keyboard ID */
#define	KB_SETRATE		0xF3	/* Set typematic rate and delay */
#define	KB_ENABLE		0xF4	/* Enable keyboard */
#define	KB_DISABLE		0xF5	/* Disable keyboard */
#define	KB_DEFAULT		0xF6	/* Reset to default */
#define	KB_RESEND		0xFE	/* Resend transmission */
#define	KB_RESET		0xFF	/* Reset keyboard */

/* Mouse commands */

#define	MS_RSTSCALING	0xE6	/* Reset scaling */
#define	MS_SCALING		0xE7	/* Set scaling */
#define MS_SETRES		0xE8	/* Set resolution */
#define MS_STATUS		0xE9	/* Status request */
#define MS_STREAM		0xEA	/* Set stream mode */
#define MS_RDDATA		0xEA	/* Read mouse data */
#define MS_RSTWRAP		0xEC	/* Reset wrap mode */
#define MS_SETWRAP		0xEE	/* Set wrap mode */
#define MS_REMOTE		0xF0	/* Set remote mode */
#define MS_RDID			0xF2	/* Read mouse ID */
#define MS_SETSAMPLE	0xF3	/* set sample rate */
#define MS_ENABLE		0xF4	/* Enable mouse */
#define MS_DISABLE		0xF5	/* Disable mouse */
#define MS_DEFAULT		0xF6	/* Reset to default */
#define MS_RESEND		0xFE	/* Resend transmission */
#define MS_RESET		0xFF	/* Reset mouse */

/* Mouse interface test command result codes */

#define	MIT_OK		0x00	/* No error */
#define MIT_CSL		0x01	/* Mouse clock stuck lo */
#define MIT_CSH		0x02	/* Mouse clock stuck hi */
#define MIT_DSL		0x03	/* Mouse data stuck lo */
#define MIT_DSH		0x04	/* Mouse data stuck hi */

/* Keyboard interface test command result codes */

#define	KIT_OK		0x00	/* No error */
#define KIT_CSL		0x01	/* Keyboard clock stuck lo */
#define KIT_CSH		0x02	/* Keyboard clock stuck hi */
#define KIT_DSL		0x03	/* Keyboard data stuck lo */
#define KIT_DSH		0x04	/* Keyboard data stuck hi */

/* Read Output port command result bits */

#define KBRO_RC		0x80	/* -RC pin */
#define KBRO_A20	0x40	/* A20 pin */
#define KBRO_MDOUT	0x20	/* Mouse data pin */
#define KBRO_MCKOUT	0x10	/* Mouse clock pin */
#define KBRO_KIRQ	0x08	/* Keyboard interrupt request */
#define KBRO_MIRQ	0x04	/* Mouse interrupt request */
#define KBRO_KCKOUT	0x02	/* Keyboard data pin */
#define KBRO_KDOUT	0x01	/* Keyboard clock pin */

#endif

