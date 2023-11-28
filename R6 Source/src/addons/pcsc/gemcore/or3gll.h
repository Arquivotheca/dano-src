/*******************************************************************************
*                 Copyright (c) 1991-1997 Gemplus Development
*
* Name        : or3GLL.H
*
* Description : Public interface for low level GemCore IFD module.
*
* Release     : 4.31.001
*
* Last Modif  : 13/10/97: V4.31.001  (GP)
*               18/03/97: V4.30.001  (TF)
*                 - Start of development.
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
   _OR3GLL_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _OR3GLL_H
#define _OR3GLL_H

/*------------------------------------------------------------------------------
Constant section:
  - HOR3GLL_DEFAULT_TIME is the default timeout for a command to be proceeded by
    an OROS3.X IFD. Today 5s (5000ms).
  - HOR3GLL_LOW_TIME is the timeout used for IFD management commands. Today 1s.
  - HOR3GLL_DEFAULT_VPP is the default VPP value. Today 0.
  - HOR3GLL_DEFAULT_PRESENCE is the default presence byte. Today 3 for no
    presence detection.                  
  - HOR3GLL_BUFFER_SIZE holds the size for the exchange buffers. We used the
    maximal size for TLP protocol to allow OROS3.X upgrades.
  - HOR3GLL_OS_STRING_SIZE is the size for a GemCore OS string. 13 characters are
    used for <Reader Status>"GemCore-R1.00".
------------------------------------------------------------------------------*/
#define HOR3GLL_DEFAULT_TIME      5000
#define HOR3GLL_LOW_TIME           500
#define HOR3GLL_DEFAULT_VPP          0
#define HOR3GLL_DEFAULT_PRESENCE     3
#define HOR3GLL_BUFFER_SIZE        261
#define HOR3GLL_OS_STRING_SIZE     HOR3GLL_IFD_LEN_VERSION+1

/*------------------------------------------------------------------------------
   Reader list of commands:
------------------------------------------------------------------------------*/
#define HOR3GLL_IFD_CMD_MODE_SET    		"\x01\x00"
#define HOR3GLL_IFD_CMD_SIO_SET	    		0x0A
#define HOR3GLL_IFD_CMD_DIR		    		"\x17\x00"
#define HOR3GLL_IFD_CMD_ICC_DEFINE_TYPE	0x17
#define HOR3GLL_IFD_CMD_ICC_POWER_DOWN 	0x11
#define HOR3GLL_IFD_CMD_ICC_POWER_UP 		0x12
#define HOR3GLL_IFD_CMD_ICC_ISO_OUT    	0x13
#define HOR3GLL_IFD_CMD_ICC_ISO_IN     	0x14
#define HOR3GLL_IFD_CMD_ICC_APDU    		0x15
#define HOR3GLL_IFD_CMD_ICC_SYNCHRONE 	   0x16
#define HOR3GLL_IFD_CMD_ICC_DISPATCHER 	"\x17\x01"
#define HOR3GLL_IFD_CMD_ICC_STATUS    	   0x17
#define HOR3GLL_IFD_CMD_MOD_DEFINE_TYPE	0x1F
#define HOR3GLL_IFD_CMD_MOD_POWER_DOWN 	0x19
#define HOR3GLL_IFD_CMD_MOD_POWER_UP 		0x1A
#define HOR3GLL_IFD_CMD_MOD_ISO_OUT    	0x1B
#define HOR3GLL_IFD_CMD_MOD_ISO_IN     	0x1C
#define HOR3GLL_IFD_CMD_MOD_APDU    		0x1D
#define HOR3GLL_IFD_CMD_MOD_SYNCHRONE 	   0x1E
#define HOR3GLL_IFD_CMD_MOD_DISPATCHER 	"\x1F\x01"
#define HOR3GLL_IFD_CMD_MOD_STATUS    	   0x1F
#define HOR3GLL_IFD_CMD_MEM_RD	    		0x22
#define HOR3GLL_IFD_CMD_MEM_WR	    		0x23
#define HOR3GLL_IFD_CMD_CPU_RD	    		0x24
#define HOR3GLL_IFD_CMD_CPU_WR	    		0x25
#define HOR3GLL_IFD_CMD_MEM_ERASE    		0x26
#define HOR3GLL_IFD_CMD_MEM_SELECT    	   0x27
#define HOR3GLL_IFD_CMD_IO_RD		   	   0x42
#define HOR3GLL_IFD_CMD_IO_WR		   	   0x43
#define HOR3GLL_IFD_CMD_IO_BIT_WR	   	0x44
#define HOR3GLL_IFD_CMD_LCD_ON	    		0x2A
#define HOR3GLL_IFD_CMD_LCD_OFF	    		0x29
#define HOR3GLL_IFD_CMD_LCD_CHAR    		0x2C
#define HOR3GLL_IFD_CMD_LCD_STRING    	   0x2B
#define HOR3GLL_IFD_CMD_LCD_CMD	    		0x2D
#define HOR3GLL_IFD_CMD_KEY_TIMEOUT    	0x32
#define HOR3GLL_IFD_CMD_SOUND_BUZZER   	0x33
#define HOR3GLL_IFD_CMD_RTC_RD	    		0x3A
#define HOR3GLL_IFD_CMD_RTC_WR	    		0x3B

/*------------------------------------------------------------------------------
   Reader special address:
------------------------------------------------------------------------------*/
#define HOR3GLL_IFD_TYP_VERSION	    		0x05
#define HOR3GLL_IFD_ADD_VERSION	    		0x3FE0
#define HOR3GLL_IFD_LEN_VERSION	    		0x10

/*------------------------------------------------------------------------------
C++ Section:
------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
Prototype section:
------------------------------------------------------------------------------*/
INT16 G_DECL G_Oros3OpenComm
(
   const INT16 port_no,
   WORD32 BaudRate
);

INT16 G_DECL G_ChangeIFDBaudRate
(
const INT16 port_no,
DWORD baud_rate
); 

INT16 G_DECL G_Oros3Exchange
(
   const WORD32        Timeout,
   const WORD16        CmdLen,
   const BYTE   G_FAR  Cmd[],
         WORD16 G_FAR *RspLen,
         BYTE   G_FAR  Rsp[]
);
INT16 G_DECL G_Oros3CloseComm
(
//   const WORD16 Handle
);
INT16 G_DECL G_Oros3SIOConfigure
(
   const WORD32        Timeout,
   const INT16         Parity,
   const INT16         ByteSize,
   const WORD32        BaudRate,
         WORD16 G_FAR *RspLen,
         WORD8  G_FAR  Rsp[]
);

INT16 G_DECL G_Oros3IccPowerDown
(
   const WORD32        Timeout,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3IccPowerUp
(
   const WORD32        Timeout,
   const BYTE          ICCVcc,
   const BYTE          PTSMode,
   const BYTE          PTS0,
   const BYTE          PTS1,
   const BYTE          PTS2,
   const BYTE          PTS3,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3IsoOutput
(
   const WORD32        Timeout,
   const WORD8         OrosCmd,
   const WORD8  G_FAR  Command[5],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3IsoInput
(
   const WORD32        Timeout,
   const WORD8         OrosCmd,
   const WORD8  G_FAR  Command[5],
   const WORD8  G_FAR  Data[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3IsoT1
(
   const WORD32        Timeout,
   const WORD8         OrosCmd,
   const WORD16        ApduLen,
   const WORD8  G_FAR  ApduCommand[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3IccIsoOutput
(
   const WORD32        Timeout,
   const WORD8  G_FAR  Command[5],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3IccIsoInput
(
   const WORD32        Timeout,
   const WORD8  G_FAR  Command[5],
   const WORD8  G_FAR  Data[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3IccIsoT1
(
   const WORD32        Timeout,
   const WORD16        ApduLen,
   const WORD8  G_FAR  ApduCommand[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3IccDefineType
(
   const WORD32        Timeout,
   const WORD16        Type,
   const WORD16        Vpp,
   const WORD16        Presence,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3ModReset
(
   const WORD32        Timeout,
   const BYTE          ICCVcc,
   const BYTE          PTSMode,
   const BYTE          PTS0,
   const BYTE          PTS1,
   const BYTE          PTS2,
   const BYTE          PTS3,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3ModDisactivate
(
   const WORD32        Timeout,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3ModIsoOutput
(
   const WORD32        Timeout,
   const WORD8  G_FAR  Command[5],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3ModIsoInput
(
   const WORD32        Timeout,
   const WORD8  G_FAR  Command[5],
   const WORD8  G_FAR  Data[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3ModIsoT1
(
   const WORD32        Timeout,
   const WORD16        ApduLen,
   const WORD8  G_FAR  ApduCommand[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3ModDefineType
(
   const WORD32        Timeout,
   const WORD16        Type,
   const WORD16        Vpp,
   const WORD16        Presence,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3ModSelect
(
   const WORD32        Timeout,
   const WORD16        Type,
   const WORD16        ModuleId,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3Dir
(
   const WORD32        Timeout,
   const WORD16        Type,
         WORD16 G_FAR *Version,
         WORD16 G_FAR *Protocol
);
INT16 G_DECL G_Oros3ActivateDispatcher
(
   const WORD32        Timeout,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3SynchPowerUp
(
   const WORD32        Timeout,
   const WORD32        CodeLen,
   const BYTE   G_FAR *Code,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3SynchPowerDown
(
   const WORD32        Timeout,
   const WORD32        CodeLen,
   const BYTE   G_FAR *Code,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3SynchIsoCmd
(
   const WORD32        Timeout,
   const WORD8  G_FAR  Command[4],
   const WORD32        LIn,
   const WORD8  G_FAR  DataIn[],
   const WORD32        LOut,
   const WORD32        CodeLen,
   const BYTE   G_FAR *Code,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3ModActivateDispatcher
(
   const WORD32        Timeout,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3ModSynchPowerUp
(
   const WORD32        Timeout,
   const WORD32        CodeLen,
   const BYTE   G_FAR *Code,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3ModSynchPowerDown
(
   const WORD32        Timeout,
   const WORD32        CodeLen,
   const BYTE   G_FAR *Code,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3ModSynchIsoCmd
(
   const WORD32        Timeout,
   const WORD8  G_FAR  Command[4],
   const WORD32        LIn,
   const WORD8  G_FAR  DataIn[],
   const WORD32        LOut,
   const WORD32        CodeLen,
   const BYTE   G_FAR *Code,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3SetMode
(
   const WORD32        Timeout,
   const WORD16        Option,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);

INT16 G_DECL G_Oros3ReadMemory
(
   const WORD32        Timeout,
   const WORD16        MemoryType,
   const WORD16        Address,
   const WORD16        Length,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3WriteMemory
(
   const WORD32        Timeout,
   const WORD16        MemoryType,
   const WORD16        Address,
   const WORD16        Length,
   const BYTE   G_FAR  Data[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3ReadCPUPort
(
   const WORD32        Timeout,
   const WORD16        Port,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3WriteCPUPort
(
   const WORD32        Timeout,
   const WORD16        Port,
   const WORD16        Value,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3ClearMemory
(
   const WORD32        Timeout,
   const WORD16        MemoryType,
   const WORD16        Address,
   const WORD16        Data,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3SelectPage
(
   const WORD32        Timeout,
   const WORD16        Page,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
  
INT16 G_DECL G_Oros3DisplayOff
(
   const WORD32        Timeout,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3DisplayOn
(
   const WORD32        Timeout,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3DisplayPrint
(
   const WORD32        Timeout,
   const INT16         Pos,
   const char   G_FAR *String,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3DisplayChar
(
   const WORD32        Timeout,
   const WORD16        Character,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3DisplayCommand
(
   const WORD32        Timeout,
   const WORD16        Command,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);

INT16 G_DECL G_Oros3WaitKeyPressed
(
   const WORD32        Timeout,
   const WORD16        Time,
   const INT32         Beep,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3Beep
(
   const WORD32        Timeout,
   const WORD16        Frequency,
   const WORD16        Time,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);

INT16 G_DECL G_Oros3ReadRTC
(
   const WORD32        Timeout,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3WriteRTC
(
   const WORD32        Timeout,
   const WORD16        Year,
   const WORD16        Month,
   const WORD16        Day,
   const WORD16        Hour,
   const WORD16        Minute,
   const WORD16        Second,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);

INT16 G_DECL G_Oros3ReadIO
(
   const WORD32        Timeout,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3WriteIO
(
   const WORD32        Timeout,
   const WORD16        Value,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3WriteIOBit
(
   const WORD32        Timeout,
   const WORD16        BitNb,
   const INT32         Value,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);

INT16 G_DECL G_Oros3BufferSize
(
	WORD16 G_FAR *Length,
	WORD8  G_FAR *Buffer
);

// Handy functions to decode ATR
INT16 G_DECL FindTA1(BYTE *Atr, BYTE *TA1);
  
#ifdef __cplusplus
}
#endif

#endif
