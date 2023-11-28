/*******************************************************************************
*                   Copyright (c) 1996-1997 Gemplus Development
*
* Name        : GemGCR.h
*
* Description : General definition for GEMPLUS programs using APDU API.
*
* Release     : 4.31.002
*
* Last Modif  : 07/01/98: V4.31.002 - Add PCSC Keyboard protocol type
*               13/10/97: V4.31.001 - Add COMM_PCSC.
*               23/04/97: V4.30.002 - Modify prototype function G4_ICCSet(Get)PTS.
*               04/04/97: V4.30.001 - Add function G4_ICCSet(Get)Voltage and 
*                                     G4_ICCSet(Get)PTS.
*               01/04/97: V4.10.005 - Add the MAX_IFD_CHANNEL
*               12/02/97: V4.10.004 - Modify the PERIPHERAL macro.
*               07/02/97: V4.10.003 - Add COM_TCPIP and COM_HOOK into the 
*                                     structure G4_PARAM_CAHNNEL.
*               22/04/96: V4.10.002 - Update structure for 32 bits alignement.
*               01/12/95: V4.10.001
*               27/10/95: V4.10.001
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
   _GEMGCR_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _GEMGCR_H
#define _GEMGCR_H

/*------------------------------------------------------------------------------
Include section:
   GEMCOM.H contain communication definitions.
------------------------------------------------------------------------------*/
#include "gemcom.h"

/*------------------------------------------------------------------------------
Reader section:
   SECURITY macro gives access to reader embedded security modules. It must be
      use as SECURITY(GCR500) for example.
   PERIPHERAL macro gives access to reader embedded peripherals, like security
      module.
      SECURITY(GCR500) = PERIPHERAL(1,GCR500).
------------------------------------------------------------------------------*/
#define SECURITY(x)            (0x0100 | (x))
#define PERIPHERAL(x,y)        (x == 0) ? (y):((0x0080 << (x)) | (y))

/*------------------------------------------------------------------------------
Product Interface Library section:
   MAX_RESET_LEN holds the maximal length for an answer to reset.
   MAX_APDU_LEN is the maximal length for an APDU buffer. The maximal value, 
      according to ISO standard is 65544 but today, low level drivers are 
      limited by the size of a segment.
   MAX_IFD_STRING holds the maximal length for an IFD identier.
   MAX_OFD_SPECIFIC holds the maximal size for the IFD specific field.
   MAX_DUMMY_CHANNEL and MAX_DUMMY_SESSION are used in RFU fields.
   MAX_IFD_CHANNEL defines the maximal IFD supported in a same time by the API.
------------------------------------------------------------------------------*/
#if defined (G_UNIX) || defined (WIN32) || defined (G_OS2)
#define MAX_RESET_LEN        36
#else
#define MAX_RESET_LEN        33
#endif

#define MAX_APDU_LEN      65535lu
#define MAX_DUMMY_CHANNEL   100
#define MAX_DUMMY_SESSION   100 - MAX_RESET_LEN
#define MAX_IFD_STRING      100
#define MAX_IFD_SPECIFIC    100
#define MAX_IFD_CHANNEL	    16

/*------------------------------------------------------------------------------
   COMMAND_LEN define the size of an ICC command.
   NULL_LEN is a value used with Gemplus specific ICC products.
------------------------------------------------------------------------------*/
#define COMMAND_LEN          4
#define NULL_LEN    0xFFFFFFFFlu

/*------------------------------------------------------------------------------
   Defines the available protocols between host and reader:
      TLP224_PROTOCOL or BLOCK_PROTOCOL on GCI400.
      PCCARD_PROTOCOL on GPR or GPR400
      AT_G_PROTOCOL on GCM
      TCPIP_PROTOCOL for future use
      PCSC_KB_PROTOCOL for the PCSC defined protocol for Keyboard readers.
------------------------------------------------------------------------------*/
#define TLP224_PROTOCOL      0
#define BLOCK_PROTOCOL       1
#define PCCARD_PROTOCOL      2
#define AT_G_PROTOCOL        3
#define TCPIP_PROTOCOL       4
#define PCSC_KB_PROTOCOL     5

/*------------------------------------------------------------------------------
   Defines the available protocols between reader and card (PTS Mode):
   - IFD_DEFAULT_MODE           -> same as OROS 2.x maximum speed without request,
   - IFD_WITHOUT_PTS_REQUEST    -> no PTS management (baud rate is 9600 bps),
   - IFD_NEGOTIATE_PTS_OPTIMAL  -> PTS management automatically,
   - IFD_NEGOTIATE_PTS_MANUALLY -> PTS management "manually" by parameters. 
   Defines the PTS format (PTS0) and indicates by the bits b5,b6,b7 set to 1 the 
   presence of the optional parameters PTS1,PTS2,PTS3 respectively. The least
   significant bits b1 to b4 select protocol type type T. Use the macro 
   - IFD_NEGOTIATE_PTS1,
   - IFD_NEGOTIATE_PTS2,
   - IFD_NEGOTIATE_PTS3,
   - IFD_NEGOTIATE_T0,
   - IFD_NEGOTIATE_T1 to set these bits.
   Defines the ICC power supply voltage:
   - ICC_VCC_5V is the power supply voltage 5V (by default),
   - ICC_VCC_3V is the power supply voltage 3V,
   - ICC_VCC_DEFAULT is the power supply voltage by default (5V).
------------------------------------------------------------------------------*/
#define IFD_DEFAULT_MODE					0
#define IFD_WITHOUT_PTS_REQUEST			1
#define IFD_NEGOTIATE_PTS_OPTIMAL   	2
#define IFD_NEGOTIATE_PTS_MANUALLY		3

#define IFD_NEGOTIATE_PTS1					0x10
#define IFD_NEGOTIATE_PTS2					0x20
#define IFD_NEGOTIATE_PTS3					0x40

#define IFD_NEGOTIATE_T0					0x00
#define IFD_NEGOTIATE_T1					0x01

#define ICC_VCC_5V							0
#define ICC_VCC_3V							1
#define ICC_VCC_DEFAULT						ICC_VCC_5V

/*------------------------------------------------------------------------------
   G4_CHANNEL_PARAM structure holds:
      - IFDType holds the reader connected: See reader section.
      - IFDBaudRate holds the maximal baud rate to use with the selected reader. 
        If the reader cannot change dynamically its communication baud rate, 
        this value is used as the selected baud rate.
      - IFDMode selects the connection mode for the reader.
      - Comm holds the communication parameters. A Dummy field is added for a 
        future use.
------------------------------------------------------------------------------*/
typedef struct
{
#if defined (G_UNIX) || defined (WIN32) || defined (G_OS2)
   WORD32   IFDType;
#else
   WORD16   IFDType;
#endif
   WORD32   IFDBaudRate;
   COM_TYPE IFDMode;
   union
   {
      COM_SERIAL Serial;
      COM_LPT    Lpt;
      COM_SCSI   Scsi;
      COM_PCCARD PcCard;
      COM_ASPI   Aspi;
      COM_TCPIP  TcpIp;
      COM_HOOK   Hook;
      COM_PCSC   Pcsc;
      WORD8      Dummy[MAX_DUMMY_CHANNEL];   
   }Comm;

} G4_CHANNEL_PARAM;

/*------------------------------------------------------------------------------
   G4_SESSION_PARAM structure holds:
      - CardType holds the awaited card type: See card section.
      - ApduLenMax holds the maximal APDU length supported according to ICC.
      - ResetLen holds the Answer To Reset length.
      - HistLen holds the number of available historical characters.
      - HistOffset holds offset for the first historical byte in ATR.
      - ResetChar holds the Answer To Reset characters.
      - Dummy field is added for a future use.
------------------------------------------------------------------------------*/
typedef struct
{
#if defined (G_UNIX) || defined (WIN32) || defined (G_OS2)
   WORD32 ICCType;
   WORD32 ApduLenMax;
   WORD32 ResetLen;
   WORD32 HistLen;
   WORD32 HistOffset;
#else
   WORD16 ICCType;
   WORD32 ApduLenMax;
   WORD16 ResetLen;
   WORD16 HistLen;
   WORD16 HistOffset;
#endif
   WORD8  ResetChar[MAX_RESET_LEN];
   WORD8  Dummy[MAX_DUMMY_SESSION];
   
} G4_SESSION_PARAM;

/*------------------------------------------------------------------------------
   G4_APDU_COMM structure holds:
      - Command holds the command bytes to send to ICC.
      - LengthIn holds the number of bytes to send to ICC. The allowed range is
        { 0 .. 65535 }. The NULL_LEN is used for Gemplus specific products.
      - DataIn holds the bytes to send to ICC.
        WARNING: the user must allocate this buffer.
      - LengthExpected memorises the maximal length expected in the ICC 
        response. The allowed range is { 0 .. 65536}.
------------------------------------------------------------------------------*/
typedef struct g4_apdu_comm
{
   WORD8         Command[COMMAND_LEN];
   WORD32        LengthIn;
   WORD8  G_FAR *DataIn;
   WORD32        LengthExpected;

} G4_APDU_COMM;

/*------------------------------------------------------------------------------
G4_APDU_RESP structure holds:
   - LengthOut is the real number of received bytes.
   - DataOut holds the received bytes. 
     WARNING: the user must allocate this buffer.
   - Status holds the two status bytes SW1 and SW2.
------------------------------------------------------------------------------*/
typedef struct g4_apdu_resp
{
   WORD32        LengthOut;
   WORD8  G_FAR *DataOut;
#if defined (G_UNIX) || defined (WIN32)	|| defined (G_OS2)
   WORD32        Status;
#else
   WORD16        Status;
#endif

} G4_APDU_RESP;

/*------------------------------------------------------------------------------
G4_IFD_STATUS structure holds:
   - Parameters holds the communication parameters given to G4_OpenChannel.
   - Protocol return the currently in use protocol with the reader.
     Available protocols are TLP224_PROTOCOL, BLOCK_PROTOCOL;
   - ExchangeSize holds the communication buffer size currently available in 
     the reader.
   - OSLength holds the number of bytes available in OSString.
   - OSString[MAX_IFD_STRING] holds a null terminated string which identifies
     the OS name and version.
   - SpecificLength holds the number of updated byte in Dummy field.
   - Specific[MAX_IFD_SPECIFIC] holds proprietary information for the selected 
     IFD.
------------------------------------------------------------------------------*/
typedef struct
{
   G4_CHANNEL_PARAM Parameters;
#if defined (G_UNIX) || defined (WIN32)	|| defined (G_OS2)
   WORD32           Protocol;
   WORD32           ExchangeSize;
   WORD32           OSLength;
   WORD8            OSString[MAX_IFD_STRING];
   WORD32           SpecificLength;
   WORD8            Specific[MAX_IFD_SPECIFIC];
#else
   WORD16           Protocol;
   WORD32           ExchangeSize;
   WORD16           OSLength;
   WORD8            OSString[MAX_IFD_STRING];
   WORD16           SpecificLength;
   WORD8            Specific[MAX_IFD_SPECIFIC];
#endif
} G4_IFD_STATUS;

/*------------------------------------------------------------------------------
C++ Section:
------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
Prototype section
------------------------------------------------------------------------------*/

INT16 G_DECL G4_OpenChannel       (const G4_CHANNEL_PARAM G_FAR *Channel);
INT16 G_DECL G4_CloseChannel      (const WORD16                  ChannelNb);
           
INT16 G_DECL G4_LockChannel       (const WORD16                  ChannelNb);
INT16 G_DECL G4_UnlockChannel     (const WORD16                  ChannelNb);

INT16 G_DECL G4_OpenSession       (const WORD16                  ChannelNb,
                                         G4_SESSION_PARAM G_FAR *Session
                                  );
INT16 G_DECL G4_SwitchSession     (const WORD16                  ChannelNb,
                                         G4_SESSION_PARAM G_FAR *Session
                                  );
INT16 G_DECL G4_CloseSession      (const WORD16                  ChannelNb);

INT16 G_DECL G4_ICCSetVoltage	  (const WORD16                  ChannelNb,
                                   const WORD16                  Voltage
                                  );

INT16 G_DECL G4_ICCGetVoltage	  (const WORD16                  ChannelNb,
                                         WORD16           G_FAR *Voltage
                                  );

INT16 G_DECL G4_ICCSetPTS		  (const WORD16                  ChannelNb,
                                   const WORD16                  PTSMode,
                                   const BYTE             G_FAR *PTSParameters
                                  );

INT16 G_DECL G4_ICCGetPTS		  (const WORD16                  ChannelNb,
                                         WORD16           G_FAR *PTSMode,
                                         BYTE             G_FAR *PTSParameters
                                  );

INT16 G_DECL G4_ExchangeApdu      (const WORD16                  ChannelNb,
                                         G4_APDU_COMM     G_FAR *ApduComm,
                                         G4_APDU_RESP     G_FAR *ApduResp
                                  );

INT16 G_DECL G4_CmdGetTimeout     (const WORD16                  ChannelNb, 
                                         WORD32           G_FAR *Time
                                  );
INT16 G_DECL G4_CmdSetTimeout     (const WORD16                  ChannelNb, 
                                   const WORD32                  Time
                                  );

INT16 G_DECL G4_ICCSetProtocol    (const WORD16                  ChannelNb, 
                                   const char             G_FAR *ProtocolFile
                                  );
INT16 G_DECL G4_ICCStatus         (const WORD16                  ChannelNb, 
                                         WORD16           G_FAR *Protocol,
                                         WORD32           G_FAR *BaudRate,
                                         G4_SESSION_PARAM G_FAR *Session
                                  );
INT16 G_DECL G4_ICCDetection      (const WORD16                  ChannelNb,
                                   const WORD16                  Time,
#ifndef G_UNIX
                                   const 
#endif /* G_UNIX */
                                         INT16(G_DECL *FnCheckIcc)(const INT32),
                                   const HINSTANCE               hInst
                                  );

INT16 G_DECL G4_IFDStatus         (const WORD16                  ChannelNb,
                                         G4_IFD_STATUS    G_FAR *Status
                                  );
INT16 G_DECL G4_IFDDownload       (const WORD16                  ChannelNb,
                                   const char             G_FAR *MemoryFile
                                  );
INT16 G_DECL G4_IFDExchange       (const WORD16                  ChannelNb,
                                   const WORD32                  TimeOut,
                                   const WORD32                  SendLength,
                                   const WORD8           G_HUGE *SendBuffer,
                                         WORD32          G_FAR  *ReadLength,
                                         WORD8           G_HUGE *ReadBuffer
                                  );
INT16 G_DECL G4_IFDGetPowerTimeout(const WORD16                  ChannelNb, 
                                         WORD32           G_FAR *Time
                                  );
INT16 G_DECL G4_IFDSetPowerTimeout(const WORD16                  ChannelNb, 
                                   const WORD32                  Time
                                  );

INT16 G_DECL G4_IFDDisplay        (const WORD16                  ChannelNb,
                                   const WORD16                  Value
                                  );                                  
INT16 G_DECL G4_IFDDisplayXY      (const WORD16                  ChannelNb,
                                   const WORD16                  Line,
                                   const WORD16                  Column
                                  );
INT16 G_DECL G4_IFDDisplayPrint   (const WORD16                  ChannelNb,
                                   const char             G_FAR *String
                                  );

INT16 G_DECL G4_IFDGetKey         (const WORD16                  ChannelNb,
                                   const WORD16                  Time,
                                   const INT32                   BeepAsked,
                                         WORD16           G_FAR *Line,
                                         WORD16           G_FAR *Column
                                  );

INT16 G_DECL G4_IFDBeep           (const WORD16                  ChannelNb,
                                   const WORD16                  Time,
                                   const WORD32                  Frequency
                                  );

INT16 G_DECL G4_IFDRTCRead        (const WORD16                  ChannelNb,
                                         WORD32           G_FAR *YyMmDd,
                                         WORD32           G_FAR *HhMmSs
                                  );
INT16 G_DECL G4_IFDRTCWrite       (const WORD16                  ChannelNb,
                                   const WORD32                  YyMmDd,
                                   const WORD32                  HhMmSs
                                  );

INT16 G_DECL G4_GetErrorString    (      WORD16           G_FAR *RemainingLength,
                                         char             G_FAR *Message
                                  );
void G_DECL G4_Gr40               (       INT16           G_FAR *NameLength,
                                          char            G_FAR  Name[],
                                          INT16           G_FAR *ReleaseLength,
                                          char            G_FAR  Release[]
                                  );

#ifdef __cplusplus
}
#endif

#endif
