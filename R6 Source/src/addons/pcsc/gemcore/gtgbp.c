/*******************************************************************************
*                    Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : GTGBP.C
*
* Description : This module holds the functions needed for communication on a
*               serial line according to Gemplus Block protocol.
*
* Author      : 


* Compiler    : 
*
* Host        : 

* Release     : 4.31.002
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/

/*------------------------------------------------------------------------------
Information section:
 - G_NAME is set to "GtGbp"
 - G_RELEASE is set to "4.31.002"
------------------------------------------------------------------------------*/
#define G_NAME     "GtGbp"
#define G_RELEASE  "4.31.002"
/*------------------------------------------------------------------------------
Pragma section
 - comment is called if _MSC_VER is defined.
------------------------------------------------------------------------------*/
#ifdef _MSC_VER
#pragma comment(exestr,"Gemplus(c) "G_NAME" Ver "G_RELEASE" "__DATE__)
#endif
/*------------------------------------------------------------------------------
Compatibility section
 - __MSC is define for compatibily with MSC under Borland environment.
------------------------------------------------------------------------------*/
#define __MSC

// Added by atul
#define G_UNIX

#include "gemplus.h"
#include "gemgcr.h"
#include "gtser.h"
#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
#include "gemansi.h"
#endif

#include "config.h"		// for debug macros

/*------------------------------------------------------------------------------
   Module public interface.
    - gtgbp.h
------------------------------------------------------------------------------*/
#include "gtgbp.h"
/*------------------------------------------------------------------------------
Constant section:
 - IBLOCK_PCB (0x00) and IBLOCK_MASK (0xA0) are used to detect the I-Block PCB
   signature (0x0xxxxxb).
 - IBLOCK_SEQ_POS indicates the number of left shift to apply to 0000000xb for
   x to be the sequence bit for a I-Block.
 - RBLOCK_PCB (0x80) and RBLOCK_MASK (0xEC) are used to detect the R-Block PCB
   signature (100x00xx). 
 - RBLOCK_SEQ_POS indicates the number of left shift to apply to 0000000xb for
   x to be the sequence bit for a R-Block.
 - ERR_OTHER and ERR_EDC are the error bits in a R-Block PCB.
 - RESYNCH_REQUEST (0xC0) and RESYNCH_RESPONSE (0xE0) are the PCB used in 
   S-Blocks.  
------------------------------------------------------------------------------*/
#define IBLOCK_PCB       0x00
#define IBLOCK_MASK      0xA0
#define IBLOCK_SEQ_POS   0x06
#define RBLOCK_PCB       0x80
#define RBLOCK_MASK      0xEC
#define RBLOCK_SEQ_POS   0x04
#define ERR_OTHER        0x02
#define ERR_EDC          0x01
#define RESYNCH_REQUEST  0xC0
#define RESYNCH_RESPONSE 0xE0           
/*------------------------------------------------------------------------------
Macro section:
 - HOST2IFD (Handle) returns the NAD byte for message from Host to IFD.
 - IFD2HOST (Handle) returns the NAD byte awaited in an IFD message.
 - MK_IBLOCK_PCB  (x) builds an I-Block PCB where x is the channel handle.
 - MK_RBLOCK_PCB  (x) builds an R-Block PCB where x is the channel handle.
 - ISA_IBLOCK_PCB (x) return TRUE if the given parameter is a I-Block PCB.
 - ISA_RBLOCK_PCB (x) return TRUE if the given parameter is a R-Block PCB.
 - SEQ_IBLOCK     (x) return the state of the sequence bit: 0 or 1.
 - INC_SEQUENCE   (x) increment a sequence bit: 0 -> 1 and 1 -> 0.
------------------------------------------------------------------------------*/
#define HOST2IFD			((WORD8)                                           \
                             (                                               \
                                (g_IFDAdd << 4)        \
                               + g_HostAdd             \
                             )                                               \
                          )
#define IFD2HOST			((WORD8)                                           \
                             (                                               \
                                (g_HostAdd << 4)       \
                               + g_IFDAdd              \
                             )                                               \
                          )
#define MK_IBLOCK_PCB    ((WORD8)                                           \
                             (                                               \
                                IBLOCK_PCB                                   \
                              + (g_SSeq << IBLOCK_SEQ_POS)  \
                             )                                               \
                          )
#define MK_RBLOCK_PCB    ((WORD8)                                           \
                             (                                               \
                                RBLOCK_PCB                                   \
                              + (g_RSeq << RBLOCK_SEQ_POS)  \
                              + g_Error                     \
                             )                                               \
                          )
#define ISA_IBLOCK_PCB(x) (((x) & IBLOCK_MASK) == IBLOCK_PCB)
#define ISA_RBLOCK_PCB(x) (((x) & RBLOCK_MASK) == RBLOCK_PCB)
#define SEQ_IBLOCK(x)     (((x) & (0x01 << IBLOCK_SEQ_POS)) >> IBLOCK_SEQ_POS)
#define INC_SEQUENCE(x)   (x) = (WORD8)(((x) + 1) % 2)
/*------------------------------------------------------------------------------
    * HostAdd holds the address identifier for the host in 0..15.
    * IFDAdd holds the address identifier for the associated IFD in 0..15.
    * PortCom holds the serial port number
    * SSeq holds the sequence bit for the next I-Block to send: 0 or 1.
    * RSeq holds the awaited sequence bit: 0 or 1.
    * Error gathers the encountered error conditions.
------------------------------------------------------------------------------*/
   WORD8 g_UserNb; 	
   WORD8 g_HostAdd;
   WORD8 g_IFDAdd;
   INT16 g_PortCom;
   WORD8 g_SSeq;
   WORD8 g_RSeq;
   WORD8 g_Error;
/*------------------------------------------------------------------------------
Global variable section (Shared):
 - gtgbp_channel[MAX_IFD_CHANNEL] is an array of TGTGBP_CHANNEL which memorises
   the communication parameters for each opened channel.
 - handle_GBP is a conversion for the logical channel to the GBP channel.
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------
Force the DLL to share this data section (only with WIN32)
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------
Function definition section:
------------------------------------------------------------------------------*/
/*******************************************************************************
* INT16 G_DECL G_GBPOpen
* (
*    const WORD16 HostAdd,
*    const WORD16 IFDAdd,
*    const INT16  PortCom
* )
*
* Description :
* -------------
* This function initialises internal variables for communicating in Gemplus
* Block protocol through a serial port.
* This function must be called before any other in this module.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the communication handle to associate to the channel.
*  - HostAdd is the host address. A valid value must be in 0.. 15. The memorised
*    value is (HostAdd mod 16).
*  - AFDAdd is the IFD address. A valid value must be in 0.. 15. The memorised
*    value is (IFDAdd mod 16).
*    This value should be different from the HostAdd.
*  - PortCom holds the serial port number
*   
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*    G_OK               (   0).
* If an error condition is raised:
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the valid range.
*  
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  gtgbp_channel is read and the selected channel is eventually updated.

*******************************************************************************/
INT16 G_DECL G_GBPOpen
(
   const WORD16 rHostAdd,
   const WORD16 rIFDAdd,
   const INT16  rPortCom
)
{
/*------------------------------------------------------------------------------
Local Variable:
   -  cptHandleLog holds the counter to scan structure
   -  present indicate if the trio (HostAdd, IFDAdd, PortCom) exist
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------
The given parameters are controlled:
<= Test Handle parameter (0 <= HostAdd < MAX_IFD_CHANNEL): GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((rHostAdd <= 0) || (rHostAdd >= MAX_IFD_CHANNEL))
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
The given parameters are controlled:
<= Test Handle parameter (0 <= IFDAdd < MAX_IFD_CHANNEL): GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((rIFDAdd <= 0) || (rIFDAdd >= MAX_IFD_CHANNEL))
   {
      return (GE_HOST_PARAMETERS);
   }

/*------------------------------------------------------------------------------
The given parameters are controlled:
<= Test Handle parameter (rHostAdd!=IFDAdd): GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if (rHostAdd == rIFDAdd)
   {
      return (GE_HOST_PARAMETERS);
   }

   g_UserNb   = 1;
   g_HostAdd  = (WORD8)(rHostAdd & 0x000F);
   g_IFDAdd   = (WORD8)(rIFDAdd  & 0x000F);
   g_PortCom   = rPortCom;
   g_SSeq     = 0;
   g_RSeq     = 0;
   g_Error    = 0;

/*------------------------------------------------------------------------------
<= G_OK
------------------------------------------------------------------------------*/
   return (G_OK);
}

/*******************************************************************************
* INT16 G_DECL G_GBPClose
* (
* )
*
* Description :
* -------------
* This function resets internal variables for communicating in Gemplus Block 
* protocol through a serial port.
* This function must be called for each opened channel (G_GBPOpen).
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the communication handle to associate to the channel.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*    G_OK               (   0).
* If an error condition is raised:
*  - GE_HOST_PORT_CLOSE (-412) if the selected channel has not been opened.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the valid range.
*  
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  gtgbp_channel is read and the selected channel is eventually updated.

*******************************************************************************/
INT16 G_DECL G_GBPClose
(
)
{
   if ( g_UserNb == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }

   g_UserNb = 0;

   return (G_OK);
}

/*******************************************************************************
* INT16 G_DECL G_GBPBuildIBlock
* (
*    const WORD16        CmdLen,
*    const WORD8  G_FAR  Cmd[], 
*          WORD16 G_FAR *MsgLen,
*          WORD8  G_FAR  Msg[]
* )
*
* Description :
* -------------
* This function takes a command and builds an Information Gemplus Block 
* Protocol.
*
* Remarks     :
* -------------
* When this command is successful, the send sequence bit is updated for the next
* exchange.
*
* In          :
* -------------
*  - Handle holds the communication handle to associate to the channel.
*  - CmdLen indicates the number of bytes in the Cmd buffer. 
*    This value must be lower than HGTGBP_MAX_DATA (Today 254).
*  - Cmd holds the command bytes.
*  - MsgLen indicates the number of available bytes in Msg.
*    This value must be at least 4 + CmdLen to allow to build the message.
*
* Out         :
* -------------
*  - MsgLen and Msg are updated with the message length and the message bytes.
*
* Responses   :
* -------------
* If everything is Ok:
*    G_OK               (   0).
* If an error condition is raised:
*  - GE_HI_CMD_LEN      (-313) if the CmdLen is greater than HGTGBP_MAX_DATA or
*    if MsgLen is too small to receive the built message.
*  - GE_HOST_PORT_CLOSE (-412) if the selected channel has not been opened.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the valid range.
*  
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  gtgbp_channel is read.

*******************************************************************************/
INT16 G_DECL G_GBPBuildIBlock
(
   const WORD16        CmdLen, 
   const WORD8  G_FAR  Cmd[], 
         WORD16 G_FAR *MsgLen,
         WORD8  G_FAR  Msg[]
)
{
/*------------------------------------------------------------------------------
Local variable:
 - edc receives the exclusive or between all the character from <NAD> to the
   last data byte.
 - i is the index which allows to read each Cmd byte.
 - j is the index which allows to write each Msg byte.
   It indicates the next free position in this buffer and is initialized to 0.
------------------------------------------------------------------------------*/
   WORD8
      edc;
   WORD16
      i,
      j = 0;

/*------------------------------------------------------------------------------
<= Test channel state (UserNb different from 0): GE_HOST_PORT_CLOSE
------------------------------------------------------------------------------*/
   if ( g_UserNb == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }
/*------------------------------------------------------------------------------
<= Test CmdLen (<= HGTGBP_MAX_DATA) and MsgLen (>= 4 + CmdLen): GE_HI_CMD_LEN.
   Msg must be able to receive the following GBP block
      <NAD> <PCB> <Len> [ Data ...] <EDC>
------------------------------------------------------------------------------*/
   if ((CmdLen > HGTGBP_MAX_DATA) || (*MsgLen < CmdLen + 4))
   {
      return (GE_HI_CMD_LEN);
   }
/*------------------------------------------------------------------------------
The message is built:
   NAD holds Target address in high part and Source address in low part.
   PCB holds I-Block mark: 0 SSeq 0 x x x x x
   Len is given by CmdLen
   [.. Data ..] are stored in Cmd.
   EDC is an exclusive or of all the previous bytes.
      It is updated when Msg buffer is updated.
------------------------------------------------------------------------------*/
   edc  = Msg[j++] = HOST2IFD;
   edc ^= Msg[j++] = MK_IBLOCK_PCB;
   edc ^= Msg[j++] = (WORD8) CmdLen;
   for (i = 0; i < CmdLen; i++)
   {
      edc ^= Msg[j++] = Cmd[i];
   }
   Msg[j++] = edc;
/*------------------------------------------------------------------------------
MsgLen is updated with the number of bytes written in Msg buffer.
------------------------------------------------------------------------------*/
   *MsgLen = (WORD16)j;
/*------------------------------------------------------------------------------
The sequence number is updated for the next exchange.
------------------------------------------------------------------------------*/
   INC_SEQUENCE(g_SSeq);
/*------------------------------------------------------------------------------
<= G_OK
------------------------------------------------------------------------------*/
   return (G_OK);
}

/*******************************************************************************
* INT16 G_DECL G_GBPBuildRBlock
* (
*          WORD16 G_FAR *MsgLen,
*          WORD8  G_FAR  Msg[]
* )
*
* Description :
* -------------
* This function builds a Repeat Gemplus Block Protocol.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the communication handle to associate to the channel.
*  - MsgLen indicates the number of available bytes in Msg.
*    This value must be at least 4 to allow to build the message.
*
* Out         :
* -------------
*  - MsgLen and Msg are updated with the message length and the message bytes.
*
* Responses   :
* -------------
* If everything is Ok:
*    G_OK               (   0).
* If an error condition is raised:
*  - GE_HI_CMD_LEN      (-313) if MsgLen is too small to receive the built 
*    message.
*  - GE_HOST_PORT_CLOSE (-412) if the selected channel has not been opened.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the valid range.
*  
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  gtgbp_channel is read.

*******************************************************************************/
INT16 G_DECL G_GBPBuildRBlock
(
         WORD16 G_FAR *MsgLen,
         WORD8  G_FAR  Msg[]
)
{
   WORD8
      edc;
   WORD16
      j = 0;

   if ( g_UserNb == 0 )
   {
      return (GE_HOST_PORT_CLOSE);
   }
   if (*MsgLen < 4)
   {
      return (GE_HI_CMD_LEN);
   }
   edc  = Msg[j++] = HOST2IFD; //(handle_GBP[Handle]);
   edc ^= Msg[j++] = MK_RBLOCK_PCB; //(handle_GBP[Handle]);
   edc ^= Msg[j++] = 0;
   Msg[j++] = edc;
   *MsgLen = (WORD16)j;

   return (G_OK);
}
/*******************************************************************************
* INT16 G_DECL G_GBPBuildSBlock
* (
*          WORD16 G_FAR *MsgLen,
*          WORD8  G_FAR  Msg[]
* )
*
* Description :
* -------------
* This function builds a Synchro request Gemplus Block Protocol.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the communication handle to associate to the channel.
*  - MsgLen indicates the number of available bytes in Msg.
*    This value must be at least 4 to allow to build the message.
*
* Out         :
* -------------
*  - MsgLen and Msg are updated with the message length and the message bytes.
*
* Responses   :
* -------------
* If everything is Ok:
*    G_OK               (   0).
* If an error condition is raised:
*  - GE_HI_CMD_LEN      (-313) if MsgLen is too small to receive the built 
*    message.
*  - GE_HOST_PORT_CLOSE (-412) if the selected channel has not been opened.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the valid range.
*  
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  gtgbp_channel is read.

*******************************************************************************/
INT16 G_DECL G_GBPBuildSBlock
(
         WORD16 G_FAR *MsgLen,
         WORD8  G_FAR  Msg[]
)
{
   WORD8
      edc;
   WORD16
      j = 0;

   if (g_UserNb == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }
   if (*MsgLen < 4)
   {
      return (GE_HI_CMD_LEN);
   }
   edc  = Msg[j++] = HOST2IFD; //(handle_GBP[Handle]); 
   edc ^= Msg[j++] = RESYNCH_REQUEST;
   edc ^= Msg[j++] = 0;
   Msg[j++] = edc;
   *MsgLen = (WORD16)j;
   return (G_OK);
}

/*******************************************************************************
* INT16 G_DECL G_GBPDecodeMessage
* (
*    const WORD16        MsgLen, 
*    const WORD8  G_FAR  Msg[], 
*          WORD16 G_FAR *RspLen,
*          WORD8  G_FAR  Rsp[]
* )
*
* Description :
* -------------
* This function takes a Gemplus Block Protocol message and extract the response 
* from it.
*
* Remarks     :
* -------------
* The awaited sequence bit is updated when a valid I-Block has been received.
* The sequence bits are reseted when a valid RESYNCH RESPONSE has been received.
*
* In          :
* -------------
*  - Handle holds the communication handle to associate to the channel.
*  - MsgLen indicates the number of bytes in the Msg buffer.
*  - Msg holds the command bytes.
*  - RspLen indicates the number of available bytes in Msg.
*
* Out         :
* -------------
*  - RspLen and Rsp are updated with the response length and the response bytes.
*
* Responses   :
* -------------
* If everything is Ok: 
*    G_OK (0).
* If an error condition is raised:
*  - GE_HI_LRC          (-302) if an EDC error has been detected.
*  - GE_HI_LEN          (-311) if a bad value has been detected in LN field 
*    or if the response buffer is too small.
*  - GE_HI_FORMAT       (-312) if the received message is neither I-Block,
*    neither R-Block and neither S-Block.
*  - GE_HI_NACK         (-314) if a R-Block has been received.
*  - GE_HI_RESYNCH      (-315) if a S-Block has been received.
*  - GE_HI_ADDRESS      (-316) if the NAD is not valid for the selected channel.
*  - GE_HI_SEQUENCE     (-317) if a bad sequence number has been received.
*  - GE_HOST_PORT_CLOSE (-412) if the selected channel has not been opened.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the valid range.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  gtgbp_channel is read and the selected channel is eventually updated.

*******************************************************************************/
INT16 G_DECL G_GBPDecodeMessage
(
   const WORD16        MsgLen, 
   const WORD8  G_FAR  Msg[], 
         WORD16 G_FAR *RspLen,
         WORD8  G_FAR  Rsp[]
)
{
/*------------------------------------------------------------------------------
Local variables:
 - edc receives the exclusive or between all the character from <NAD> to the
   last data byte.
 - j will point on next free byte in Rsp.
 - response is updated with the function status.
------------------------------------------------------------------------------*/
   WORD8
      edc;
   WORD16
      j;
   INT16
      response;
      
/*------------------------------------------------------------------------------
<= Test channel state (UserNb different from 0): GE_HOST_PORT_CLOSE
------------------------------------------------------------------------------*/
   if (g_UserNb == 0)
   {
      *RspLen =0;
      return (GE_HOST_PORT_CLOSE);
   }
/*------------------------------------------------------------------------------
Reset the associated error field.
------------------------------------------------------------------------------*/
   g_Error = 0;
/*------------------------------------------------------------------------------
Verifies the message frame and copies the data bytes:
<= Test NAD (HostAdd | IFDAdd): GE_HI_ADDRESS
------------------------------------------------------------------------------*/
   if (Msg[0] != IFD2HOST )
   {
      *RspLen =0;
      return (GE_HI_ADDRESS);
   }
   edc = Msg[0];
/*------------------------------------------------------------------------------
   Updates response variable with the PCB type:
    - GE_HI_RESYNCH if a S-Block has been detected
    - GE_HI_NACK    if a T-Block has been detected
------------------------------------------------------------------------------*/
   if (Msg[1] == RESYNCH_RESPONSE)
   {
      response = GE_HI_RESYNCH;
   }
   else if (ISA_RBLOCK_PCB(Msg[1]))
   {
      response = GE_HI_NACK;
   }
/*------------------------------------------------------------------------------
    - For I-Block
<=    Test PCB sequence bit: GE_HI_SEQUENCE
------------------------------------------------------------------------------*/
   else if (ISA_IBLOCK_PCB(Msg[1]))
   {
      if ( (WORD8) SEQ_IBLOCK(Msg[1]) != g_RSeq)
      {
         return (GE_HI_SEQUENCE);
      }
      response = G_OK;
   }
/*------------------------------------------------------------------------------
    - For other cases
<=       GE_HI_FORMAT
------------------------------------------------------------------------------*/
   else
   {
      return(GE_HI_FORMAT);
   }
   edc ^= Msg[1];
/*------------------------------------------------------------------------------
<= Test Len (Len + 4 = MsgLen and RspLen >= Len): GE_HI_LEN
   This error update the Error.other bit.
------------------------------------------------------------------------------*/
   if (((WORD16)Msg[2] > *RspLen) )
   {
      *RspLen =0;
      g_Error |= ERR_OTHER;
      D(bug("G_GBPDecod returning GE_HI_LEN: resplen not match\n"));
      return (GE_HI_LEN);
   }
   if ((WORD16)(Msg[2] + 4) != MsgLen)
   {
      *RspLen =0;
      g_Error |= ERR_OTHER;
	 D(bug("G_GBPDecod returning GE_HI_LEN: msg len not match\n"));
      return (GE_HI_LEN);
   }



   edc ^= Msg[2];
/*------------------------------------------------------------------------------
   Copies the data bytes, updates RspLen and calculated edc.
------------------------------------------------------------------------------*/
   *RspLen = (WORD16)Msg[2];
   for (j = 0; j < *RspLen; j++)
   {
      Rsp[j] = Msg[j + 3];
      edc ^= Rsp[j];
   }
/*------------------------------------------------------------------------------
<= Test the read EDC: GE_HI_LRC
   This error update the Error.EDC bit.
------------------------------------------------------------------------------*/
   if (edc != Msg[j + 3])
   {
      *RspLen = 0;
      g_Error |= ERR_EDC;
      return (GE_HI_LRC);
   }
/*------------------------------------------------------------------------------
Updates the awaited sequence bit when a valid I-Block has been received.
------------------------------------------------------------------------------*/
   if (response == G_OK)
   {
      INC_SEQUENCE(g_RSeq);
   }
/*------------------------------------------------------------------------------
Reset the sequence bits when a valid S-Block has been received.
------------------------------------------------------------------------------*/
   else if (response == GE_HI_RESYNCH)
   {
      g_SSeq = g_RSeq = 0;
   }
/*------------------------------------------------------------------------------
<= GE_HI_RESYNCH / GE_HI_NACK / G_OK
------------------------------------------------------------------------------*/
   return (response);
}
/*******************************************************************************
* INT16 G_DECL G_GBPChannelToPortComm
* (
*    const INT16  Handle
* )
*
* Description :
* -------------
* This function return a physical port associate with the Logical Channel 
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the communication handle to associate to the channel.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_GBPChannelToPortComm
(
)
{
   if (g_UserNb == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }

   return(g_PortCom);
}



