/*******************************************************************************
*                  Copyright (c) 1996 - 1997 Gemplus Development
*
* Name        : GemCom.h
*
* Description : General definition for GEMPLUS programs using Communication.
*
* Release     : 4.31.002
*
* Last Modif  : 07/01/97: V4.31.002 - Add the G_KEYBOARD connection mode.
*               13/10/97: V4.31.001 - Add the G_PCSC connection mode.
*               11/04/97: V4.10.006 - Modify conditionnal compilation for 
*                                     Borland compatibility.
*               02/01/97: V4.10.003 - Add the G_HOOK and G_TCPIP connection mode
*               22/04/96: V4.10.002 - Update structure for 32 bits alignement.
*               01/12/95: V4.10.001 - Update to new Gemplus 4.10 Version.
*               27/10/95: V4.10.001 - First version.
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/


/*------------------------------------------------------------------------------
Name definition:
   _GEMCOM_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _GEMCOM_H
#define _GEMCOM_H


/*------------------------------------------------------------------------------
Connexion section:
   COM_TYPE defines the today supported Gemplus peripheral connection type.
------------------------------------------------------------------------------*/
typedef enum
{
   G_SERIAL,
   G_LPT,
   G_SCSI,
   G_PCCARD,
   G_ASPI,
   G_HOOK,
   G_TCPIP,
   G_PCSC,
   G_KEYBOARD
} COM_TYPE;
/*------------------------------------------------------------------------------
   Serial port definitions.
      COM_SERIAL structure gathers the serial port parameters:
         - Port holds the port key word or directly the port address.
           Under Windows, it is not possible to give directly the address.
         - BaudRate holds the baud rate for the selected communication port.
         - ITNumber is used to select the interrupt which will be used to drive
           the selected port.
           Under Windows, it is not possible to give directly the ITNumber.
      G_COMx must be used for automatic address selection.
      GCRx_y must be used with Gemplus GRI200 card. The first digit declares 
         serial port in use and the second digit declares card port in use.
      DEFAULT_IT is used for automatic interrupt selection.
------------------------------------------------------------------------------*/
typedef struct
{
#if defined (G_UNIX) || defined (WIN32) || defined (G_OS2)
   WORD32 Port;
   WORD32 BaudRate;
   WORD32 ITNumber;
#else
   WORD16 Port;
   WORD32 BaudRate;
   WORD16 ITNumber;
#endif   
} COM_SERIAL;

#define G_COM1        1
#define G_COM2        2
#define G_COM3        3
#define G_COM4        4
#define GRI1_1     0x11
#define GRI1_2     0x21
#define GRI2_1     0x12
#define GRI2_2     0x22
#define GRI3_1     0x13
#define GRI3_2     0x23
#define GRI4_1     0x14
#define GRI4_2     0x24

#define DEFAULT_IT 0xFF
/*------------------------------------------------------------------------------
   LPT port definitions.
      COM_LPT structure gathers the LPT port parameters:
         - Port holds the port key word or directly the port address.
      G_LPTx must be used for automatic address selection.
------------------------------------------------------------------------------*/
typedef struct
{
#if defined (G_UNIX) || defined (WIN32) || defined (G_OS2)
   INT32 Port;
#else
   INT16 Port;
#endif   

} COM_LPT;

#define G_LPT1 (-1)
#define G_LPT2 (-2)
#define G_LPT3 (-3)
#define G_LPT4 (-4)
/*------------------------------------------------------------------------------
   SCSI port definitions.
      COM_SCSI structure gathers the SCSI port parameters:
         - Address holds the SCSI port base address.
         - HostId holds the host SCSI identifier and PeriphId holds the target
           SCSI identifier in {0,1,2,3,4,5,6,7}
------------------------------------------------------------------------------*/
typedef struct
{
#if defined (G_UNIX) || defined (WIN32) || defined (G_OS2)
   WORD32 Address;
   WORD32 HostId;
   WORD32 PeriphId;
#else
   WORD16 Address;
   WORD16 HostId;
   WORD16 PeriphId;
#endif   
} COM_SCSI;
/*------------------------------------------------------------------------------
   PCCARD port definitions.
      COM_PCCARD structure gathers the PCMCIA port parameters:
         - Socket holds a logical socket number from 0 to 5.
         - ITNumber is used to select the interrupt which will be used to drive
           the selected port.
      DEFAULT_IT is used for automatic interrupt selection.
      NO_IT is used when the PCCARD is not interrupt driven.
------------------------------------------------------------------------------*/
typedef struct
{
#if defined (G_UNIX) || defined (WIN32) || defined (G_OS2)
   WORD32 Socket;
   WORD32 ITNumber;
#else
   WORD16 Socket;
   WORD16 ITNumber;
#endif
} COM_PCCARD;

#define NO_IT 0xFE
/*------------------------------------------------------------------------------
   ASPI SCSI  port definitions.
      COM_ASPI structure gathers the SCSI port parameters (ASPI Driven):
         - HostId holds the host SCSI identifier and PeriphId holds the target
           SCSI identifier in {0,1,2,3,4,5,6,7}
         - BoardNumber holds the SCSI Board Number if more than one is installed.
           The first board is associated to 0.
------------------------------------------------------------------------------*/
typedef struct
{
#if defined (G_UNIX) || defined (WIN32) || defined (G_OS2)
   WORD32 HostId;
   WORD32 PeriphId;
   WORD32 BoardNumber;
#else
   WORD16 HostId;
   WORD16 PeriphId;
   WORD16 BoardNumber;
#endif   
} COM_ASPI;

/*------------------------------------------------------------------------------
   Hook port definitions.
      COM_HOOK structure gathers the Hook port parameters:
         - Port holds the port key word.
------------------------------------------------------------------------------*/
typedef struct
{
   WORD32 Port;
   
} COM_HOOK;

/*------------------------------------------------------------------------------
   TCP IP port definitions.
      COM_TCPIP structure gathers the Hook port parameters:
         - Port holds the port key word.
         - Address holds the TCP IP address.
------------------------------------------------------------------------------*/
typedef struct
{
   WORD32 Port;
   BYTE   Address[4];
} COM_TCPIP;

/*------------------------------------------------------------------------------
   PC/SC port definitions.
      COM_PCSC structure gathers the PC/SC port parameters:
         - ReaderIdentification holds the reader's name.
------------------------------------------------------------------------------*/
typedef struct
{
   BYTE   ReaderIdentification[100];
} COM_PCSC;

/*------------------------------------------------------------------------------
   Generic port definitions.
   COM_GENERIC defines the today supported Gemplus peripheral connection type.
------------------------------------------------------------------------------*/
typedef union
{
   COM_SERIAL Serial;
   COM_LPT    Lpt;
   COM_SCSI   Scsi;
   COM_PCCARD PcCard;
   COM_ASPI   Aspi;
   COM_HOOK   Hook;
   COM_TCPIP  TcpIp;
   COM_PCSC   Pcsc;
} COM_GENERIC;

#endif
