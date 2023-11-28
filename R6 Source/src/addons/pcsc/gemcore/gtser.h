/*******************************************************************************
*                 Copyright (c) 1991-1997 Gemplus Development
*
* Name        : GTSer.h
*
* Description : This module allows serial port management.
*
* Release     : 4.31.001
*
* Last Modif  : 10/12/97: V4.31.001 New definitinons for OS2 Warp
*               14/10/97: V4.31.001
*                         HGTSER_MAX_PORT is set to 16.
*               04/09/96: V4.20.001                         
*					 12/04/96: V4.20.000
*                         Add functions for Windows 32 bits.
*               15/05/95: V4.10.001
*                         Add functions for line state analysis.
*               22/03/95: V4.01.00
*                         Card detection implementation.
*                         G_SerPortSetDetect and G_SerPortGetDetect.
*               16/09/94: V4.00
* Date        : 15/07/94: V4.00
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/
/* static char *ID="@(#) gtser.h 4.30.1 Copyright Gemplus 97/10/22 18:23:41" */

/*------------------------------------------------------------------------------
Name definition:
   _GTSER_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _GTSER_H
#define _GTSER_H

/*------------------------------------------------------------------------------
Constant section:
 - HGTSER_MAX_PORT holds the number of managed port. Today 16.
 - HGTSER_WORD_5, HGTSER_WORD_6, HGTSER_WORD_7 and HGTSER_WORD_8 allow to 
   configure the number of bits per word.
 - HGTSER_STOP_BIT_1 and HGTSER_STOP_BIT_2 allow to configure the number of stop
   bit.
 - HGTSER_NO_PARITY, HGTSER_ODD_PARITY and HGTSER_EVEN_PARITY allow to configure
   the communication parity.
 - HGTSER_TX_QUEUE and HGTSER_RX_QUEUE are queues indentifiers.
 - HGTSER_RX_OVER is set when the reception queue is full and characters has 
   been lost.                                                                   
 
 - HGTSER_RTS_LINE is identifier of RTS line for Line status functions.
 - HGTSER_DTR_LINE is identifier of DTR line for Line status functions.
 - HGTSER_EV_RING is identifier of EV_RING line for Line status functions.
------------------------------------------------------------------------------*/
#define HGTSER_MAX_PORT         16
#define HGTSER_WORD_5         0x00
#define HGTSER_WORD_6         0x01
#define HGTSER_WORD_7         0x02
#define HGTSER_WORD_8         0x03
#define HGTSER_STOP_BIT_1     0x00
#define HGTSER_STOP_BIT_2     0x04
#define HGTSER_NO_PARITY      0x00
#define HGTSER_ODD_PARITY     0x08
#define HGTSER_EVEN_PARITY    0x18
#define HGTSER_TX_QUEUE          1
#define HGTSER_RX_QUEUE          2
#define HGTSER_RX_OVER           1

#define HGTSER_RTS_LINE       0
#define HGTSER_DTR_LINE       1
#ifndef G_UNIX
#define HGTSER_EV_RING        2
#else
#define HGTSER_CTS_LINE       2
#define HGTSER_DSR_LINE       3
#define HGTSER_RI_LINE        4
#define HGTSER_DCD_LINE       5
#endif

#ifdef G_OS2
#define EV_RECEP 1
#define EV_TIMEOUT 2
#define EV_TXFIN 4
#define EV_CTS 8
#define EV_DSR 16
#define EV_DCD 32
#define EV_BREAK 64
#define EV_ERROR 128
#ifdef EV_RING
#undef EV_RING
#endif
#define EV_RING 256
#endif /* G_OS2 */

/*------------------------------------------------------------------------------
Constant section:
 - WTX_TIMEOUT is the time out used when a WTX REQUEST is send by  the CT.
 - CHAR_TIMEOUT is the timeout at character level: today 1000 ms.
------------------------------------------------------------------------------*/
#define WTX_TIMEOUT 3000
#define CHAR_TIMEOUT 1000
/*------------------------------------------------------------------------------
Type section:
 - TGTSER_PORT gathers data used to manage a serial port:
    * Port     indicates the selected port.
               G_COM1, G_COM2, G_COM3 or G_COM4
    * BaudRate is used to set port baud rate when open routine is called.
               300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
    * ITNumber indicates the interrupt number to use. The 0XFF value indicates
               the default value. Allowed number are from 0 to 15.
    * Mode     Memorises
               WORD size       : 5, 6, 7 or 8
               stop bit number : 1 or 2
               parity          : no parity, odd or even parity
    * TimeOut  indicates the time out value, in milli-seconds, at character
               level.
    * TxSize   is the transmit buffer size, in bytes.
    * RxSize   is the reception buffer size, in mbytes.
------------------------------------------------------------------------------*/
typedef struct
{
   INT16   Port;
   WORD32  BaudRate;
   WORD16  ITNumber;
   WORD16  Mode;
   WORD16  TimeOut;
   WORD16  TxSize;
   WORD16  RxSize;

} TGTSER_PORT;
/*------------------------------------------------------------------------------
 - TGTSER_STATUS holds status bit about the serial communication.
   Today only HGTSER_RX_OVER can be set or not.
------------------------------------------------------------------------------*/
typedef WORD16 TGTSER_STATUS;

#if !(defined G_LINUX) || !(defined G_UNIX)

/*------------------------------------------------------------------------------
C++ Section:
------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
Prototypes section:
------------------------------------------------------------------------------*/
void G_DECL gtser
(
   INT16 G_FAR *NameLength,
   char  G_FAR  Name[],
   INT16 G_FAR *ReleaseLength,
   char  G_FAR  Release[]
);
INT16 G_DECL G_SerPortOpen
(
   const TGTSER_PORT G_FAR *Param
);
INT16 G_DECL G_SerPortClose
(
   const INT16 Handle
);
INT16 G_DECL G_SerPortLockAccess
(
   const INT16 Handle
);
INT16 G_DECL G_SerPortUnlockAccess
(
   const INT16 Handle
);
INT16 G_DECL G_SerPortWrite
(
   const INT16        Handle,
   const WORD16       Length,
   const BYTE   G_FAR Buffer[]
);
INT16 G_DECL G_SerPortRead
(
   const INT16         Handle,
         WORD16 G_FAR *Length,
         BYTE   G_FAR  Buffer[]
);
INT16 G_DECL G_SerPortFlush
(
   const INT16  Handle,
   const WORD16 Select
);
INT16 G_DECL G_SerPortStatus
(
   const INT16                Handle,
         WORD16        G_FAR *TxLength,
         WORD16        G_FAR *RxLength,
         TGTSER_STATUS G_FAR *Status
);
INT16 G_DECL G_SerPortSwitch
(
   const INT16  Handle,
   const WORD16 Channel
);
INT16 G_DECL G_SerPortAddUser
(
   const INT16  Port
);
INT16 G_DECL G_SerPortGetState
(
   TGTSER_PORT G_FAR *Param,
   WORD16      G_FAR *UserNb
);
INT16 G_DECL G_SerPortSetState
(
   TGTSER_PORT G_FAR *Param
);
INT16 G_DECL G_SerPortSetLineState
(
   const INT16  Handle,
   const BYTE   Line,
#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
   const INT32  Enable,
#else
   const BOOL  Enable,
#endif
   const WORD32 Time
);
INT16 G_DECL G_SerPortGetLineState
(
   const INT16        Handle,
   const BYTE         Line,
#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
	INT32 G_FAR *Enable
#else
   BOOL G_FAR *Enable
#endif
);
INT16 G_DECL G_SerPortSetEvent
(
   const INT16  Handle,
   const WORD16 Event
);
INT16 G_DECL G_SerPortGetEvent
(
   const INT16         Handle,
   const WORD16        Event,
#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
	INT32  G_FAR *Found
#else
	BOOL  G_FAR *Found
#endif
);


INT16 G_DECL G_SerPortSetTimeouts
(
   const INT16         Handle,
   DWORD         		  BWT
);

INT32 G_DECL G_SerPortAttach
(
  void
);

void G_DECL G_SerPortDetach
(
  void
);

INT32 G_DECL G_SerPortLockComm
(
    const INT16 Handle,
    const DWORD WaitRelease
);
void G_DECL G_SerPortUnlockComm
(
    const INT16 Handle
);

#ifdef __cplusplus
}
#endif

#endif

#endif

