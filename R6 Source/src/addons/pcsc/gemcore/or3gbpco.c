#define G_NAME     "Or3GBPCo"
#define G_RELEASE  "4.31.002"

#define G_UNIX

#define __MSC

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/times.h>

#include "gemplus.h"
#include "gemgcr.h"
#include "gtser.h"
#include "gttimout.h"
#include "gtgbp.h"
#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
#include "gemansi.h"
#endif

#include "or3comm.h"

#include "config.h"		// for debug macros

#ifndef G_WINDOWS
#define SPRINTF sprintf
#define STR_PTR
#else
#define SPRINTF wsprintf
#define STR_PTR (LPSTR)
#endif

#if (!defined WIN32) && (!defined G_OS2) && (!defined G_UNIX)
static BYTE
   s_buff[HGTGBP_MAX_BUFFER_SIZE];
static WORD16
   s_len;
static BYTE
   r_buff[HGTGBP_MAX_BUFFER_SIZE];
static WORD16
   r_len;
#endif

#define	TIMEOUT_STEP		50

#ifdef G_DEBUG
static char        
   temp_str[255];
static char        
   temp_name[255];
static time_t      
   current_time;
static struct tm  
   *local_time;
#endif

INT16 G_DECL G_Oros3SendCmd
(
   const WORD16       CmdLen,
   const WORD8  G_FAR Cmd[],
	const BOOL         Resynch
)
{
WORD16
   old_tx,
   tx_length,
   rx_length;
TGTSER_STATUS
   status;
INT16
   response,
   portcom;
#if (defined WIN32) || (defined G_OS2) || (defined G_UNIX)
WORD16
   s_len;
BYTE
   s_buff[HGTGBP_MAX_BUFFER_SIZE];
#endif

/*------------------------------------------------------------------------------
   Associate Handle( or ChannelNb) with portcom
------------------------------------------------------------------------------*/
   portcom = G_GBPChannelToPortComm();
/*------------------------------------------------------------------------------
The GBP message to send is build from the given command:
   If CmdLen is null
   Then
      If resynch is true
      Then
         a S-Block is built.
      Else
         a R-BLOCK message is built:
------------------------------------------------------------------------------*/
   s_len = HGTGBP_MAX_BUFFER_SIZE;
   if (CmdLen == 0)
   {
      if (Resynch)
      {
         response = G_GBPBuildSBlock(&s_len,s_buff);
      }
      else
      {
         response = G_GBPBuildRBlock(&s_len,s_buff);
      }
   }
/*------------------------------------------------------------------------------
   Else
      an I-BLOCK message is built:
------------------------------------------------------------------------------*/
   else
   {
      response = G_GBPBuildIBlock(CmdLen,Cmd,&s_len,s_buff);
   }
/*------------------------------------------------------------------------------
   If the last operation fails
   Then
<=    G_GBPBuildSBlock, G_GBPBuildRBlock or G_GBPBuildIBlock status.
------------------------------------------------------------------------------*/
   if (response < G_OK)
   {
      return (response);
   }
/*------------------------------------------------------------------------------
The communication queues are flushed before the writing.
   If this operation fails
   Then
<=    G_SerPortFlush status.
------------------------------------------------------------------------------*/
   response = G_SerPortFlush(portcom, HGTSER_TX_QUEUE | HGTSER_RX_QUEUE);
   if (response < G_OK)
   {
      return (response);
   }
/*------------------------------------------------------------------------------
The message is written. As the queues have been flushed, it remains enough
   place in the transmitt queue for the whole message.
   If this operation fails
   Then
<=    G_SerPortWrite status.
------------------------------------------------------------------------------*/
   response = G_SerPortWrite(portcom, s_len, s_buff);

	{
	int i;
	D(bug("Actual Cmd : "));
	for ( i = 0; i < s_len; ++i )
		D(bug("0x%x ", s_buff[i]));
	D(bug("\n"));
	}

   if (response < G_OK)
   {
      return (response);
   }
  //sleep(1);
   return (G_OK);
}

/*******************************************************************************
* INT16 G_DECL G_Oros3ReadResp
* (
*    const WORD32        Timeout,
*          WORD16 G_FAR *RspLen,
*          WORD8  G_FAR  Rsp[]
*    
* )
*
* Description :
* -------------
* This function reads the OROS 3.x IFD response to a command according to 
* Gemplus Block Protocol.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle is the serial channel handle.
*  - Timeout is the command timeout.
*  - RspLen indicates how many bytes can be stored in Rsp buffer.
*
* Out         :
* -------------
*  - RspLen and Rsp are updated with the read bytes.
*
* Responses   :
* -------------
* If everything is Ok:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (-201) if a character timeout is detected.
*  - GE_HI_COMM         (-300) if a communication error occurs.
*  - GE_HI_PARITY       (-301) if a parity error is encountered.
*  - GE_HI_LRC          (-302) if an EDC error has been detected.
*  - GE_HI_PROTOCOL     (-310) if a frame error is encountered.
*  - GE_HI_LEN          (-311) if a bad value has been detected in LN field
*    or if the response buffer is too small.
*  - GE_HI_FORMAT       (-312) if the received message is neither I-Block,
*    neither R-Block and neither S-Block.
*  - GE_HI_NACK         (-314) if a R-Block has been received.
*  - GE_HI_RESYNCH      (-315) if a S-Block has been received.
*  - GE_HI_ADDRESS      (-316) if the NAD is not valid for the selected channel.
*  - GE_HI_SEQUENCE     (-317) if a bad sequence number has been received.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3ReadResp
(
   const WORD32        Timeout,
         WORD16 G_FAR *RspLen,
         WORD8  G_FAR  Rsp[]
)
{
/*------------------------------------------------------------------------------
Local variables:
 - response hold the called function responses.
 - r_buff[HGTGBP_MAX_BUFFER_SIZE] and r_len are used to receive a IFD message.
------------------------------------------------------------------------------*/
INT16
   response,
   portcom;
#if (defined WIN32) || (defined G_OS2) || (defined G_UNIX)
WORD16
   r_len;
BYTE
   r_buff[HGTGBP_MAX_BUFFER_SIZE];
#endif

/*------------------------------------------------------------------------------
   Associate Handle( or ChannelNb) with portcom
------------------------------------------------------------------------------*/
   portcom = G_GBPChannelToPortComm();
/*------------------------------------------------------------------------------
Read the IFD response:
   The first three bytes are read by calling G_SerPortRead.
   If the reading fails
   Then
      The read length is set to 0 
<=    G_SerPortRead status
------------------------------------------------------------------------------*/
   r_len = 3;
   response = G_SerPortRead(portcom,&r_len,r_buff);
   if (response < G_OK)
   {
      *RspLen = 0;
      return (response);
   }
/*------------------------------------------------------------------------------
   r_len is udpated with the number of bytes which must be read to receive a 
      complete Gemplus Block Protocol: the number indicated by the length field
      of the GBP message + one byte for the EDC.
------------------------------------------------------------------------------*/
   r_len = (WORD16)(r_buff[2] + 1);
/*------------------------------------------------------------------------------
   The end of the message is read by calling G_SerPortRead. The character
      timeout pass to G_SerPortOpen will be used to determine broken 
      communication.
   If the selected number of bytes can't be read
   Then
      The read length is set to 0 
<=    G_SerPortRead status
------------------------------------------------------------------------------*/
   response = G_SerPortRead(portcom,&r_len, r_buff + 3);

   if (response < G_OK)
   {
      *RspLen = 0;
      return (response);
   }

/*------------------------------------------------------------------------------
   The message length is restored by adding 3 to r_len.
------------------------------------------------------------------------------*/
   r_len += 3;
	{
	int i;
	D(bug("GBP Resp=%d: ", response));
	for ( i = 0; i < r_len; ++i )
		D(bug("0x%x ", r_buff[i]));
	D(bug("\n"));
	}
/*------------------------------------------------------------------------------
The GBP message is controlled and decoded:
<= G_GBPDecodeMessage status.
------------------------------------------------------------------------------*/
   response = G_GBPDecodeMessage(r_len,r_buff,RspLen,Rsp);

return response;
}
/*******************************************************************************
* INT16 G_DECL G_Oros3Exchange
* ( 
*    const WORD32        Timeout,
*    const WORD8         CmdLen,
*    const BYTE   G_FAR  Cmd[],
*          WORD8  G_FAR *RspLen,
*          BYTE   G_FAR  Rsp[]
* )
*
* Description :
* -------------
* This function sends a command to an ORS2.x IFD according to Gemplus Block
* Protocol and read its response.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Timeout holds the command timeout, in ms.
*  - CmdLen  holds the command length.
*  - Cmd     holds the command bytes.
*  - RspLen indicates how many bytes can be stored in Rsp buffer.
*
* Out         :
* -------------
*  - RspLen and Rsp are updated with the read bytes.
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (-201) if a character/command timeout is detected.
*  - GE_HI_COMM         (-300) if a communication error occurs.
*  - GE_HI_PARITY       (-301) if a parity error is encountered.
*  - GE_HI_LRC          (-302) if an EDC error has been detected.
*  - GE_HI_PROTOCOL     (-310) if a frame error is encountered.
*  - GE_HI_LEN          (-311) if a bad value has been detected in LN field
*    or if the response buffer is too small.
*  - GE_HI_FORMAT       (-312) if the received message is neither I-Block,
*    neither R-Block and neither S-Block.
*  - GE_HI_CMD_LEN      (-313) if the CmdLen is greater than HGTGBP_MAX_DATA or
*    if MsgLen is too small to receive the built message.
*  - GE_HI_NACK         (-314) if a R-Block has been received.
*  - GE_HI_RESYNCH      (-315) if a S-Block has been received.
*  - GE_HI_ADDRESS      (-316) if the NAD is not valid for the selected channel.
*  - GE_HI_SEQUENCE     (-317) if a bad sequence number has been received.
*  - GE_HOST_PORT_BREAK (-404) if the bytes cannot be sent.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3Exchange
( 
   const WORD32        Timeout,
   const WORD16        CmdLen,
   const BYTE   G_FAR  Cmd[],
         WORD16 G_FAR *RspLen,
         BYTE   G_FAR  Rsp[]
)
{
/*------------------------------------------------------------------------------
Local variables:
 - timeout holds the value in use.
   It can be modified for NACK message timeout. 
 - end_time is used to detect command timeout.
 - tx_length, rx_length and status are used to get the serial port status.
 - cmdlen_used is a copy of CmdLen.
   It will be set to 0 if we don't understand IFD.
   It is initialized to CmdLen.
 - rsplen_save is a copy of RspLen parameter which can be altered during
   communication tries.
   It is initialized to RspLen.
 - try_counter memorises the number of communication try.
   It is initialized to 0
 - resynch_counter memorises the number of S-Block sent to IFD.
 - response holds the called function responses.
 - resynch is a boolean which indicates if a resynchronization is needed or if
   it's only a repeat block.
------------------------------------------------------------------------------*/
WORD32
   timeout = Timeout,
   start_time;
WORD16
   tx_length,
   rx_length;
TGTSER_STATUS
   status;
WORD16
   cmdlen_used = CmdLen,
   rsplen_save = *RspLen;
INT16
   try_counter  = 0,
   sync_counter = 0,
   response,
   portcom;
BOOL
   resynch;
struct  tms tm;
DWORD temp_timeout=0;

   portcom = G_GBPChannelToPortComm();

/*------------------------------------------------------------------------------
   A first try loop for sending the command is launched:
   This loop allows to try to send the command, then, if the try fails, to 
   resynchronize the reader and then to tru to send the command one more time.
------------------------------------------------------------------------------*/
   resynch = FALSE;
   while (sync_counter++ < 2)
   {
/*------------------------------------------------------------------------------
      A second try loop for sending the command is launched:
      We send the command and, if a communication error occurs, HOR3COMM_MAX_TRY
      repetitions are allowed before the communication is considered as broken.
         The given command is sent to IFD.
         If the operation fails
         Then
            RspLen parameter is set to 0.
            Release the serial communication semaphore.
<=          G_Oros3SendCmd status.
------------------------------------------------------------------------------*/
      try_counter = 0;
      while (try_counter++ < HOR3COMM_MAX_TRY)
      {
         response = G_Oros3SendCmd(cmdlen_used,Cmd,resynch);
         if (response < G_OK)
         {
            *RspLen = 0;
            return (response);
         }
/*------------------------------------------------------------------------------
         The command timeout is armed and we wait for at least three characters to
            be received (the 3th codes the message length in GBP, so we know the
            number of characters to read).
         If this timeout is raised
         Then
            If no synchronisation has been made
            Then
               we exit from the internal loop to launch a resynchronization.
            Else
               RLen parameter is set to 0;
               Release the serial communication semaphore.
<=             GE_IFD_MUTE
------------------------------------------------------------------------------*/
	 //start_time = clock();
	start_time = times(&tm);
        G_SerPortStatus(portcom, &tx_length, &rx_length, &status);
	temp_timeout = 0;
	while ( rx_length < 3 )
	{
		if ( temp_timeout > timeout )
		{
			*RspLen = 0;
			return (GE_IFD_MUTE);
		}
		wait_ms(TIMEOUT_STEP);
		temp_timeout += TIMEOUT_STEP;
            	G_SerPortStatus(portcom, &tx_length, &rx_length, &status);
	}

/*
         while(rx_length < 3)
         {
	DWORD temp;
	temp = times(&tm) - start_time;
	    if ( temp > timeout )
            {
               if (sync_counter == 0)
               {
                  break;
               }
               else
               {
                  *RspLen = 0;
                  return (GE_IFD_MUTE);
               }
            }
            G_SerPortStatus(portcom, &tx_length, &rx_length, &status);
         }
*/
/*------------------------------------------------------------------------------
         The IFD response is read (RspLen is restored for the cases where we have
            received a R-Block from the reader).
------------------------------------------------------------------------------*/
         *RspLen = rsplen_save;
         response = G_Oros3ReadResp(timeout, RspLen, Rsp);
/*------------------------------------------------------------------------------
         If a good message has been read
         Then
            Release the serial communication semaphore.
<=          G_OK
------------------------------------------------------------------------------*/
         if (response == G_OK)
         {
            return (G_OK);
         }
/*------------------------------------------------------------------------------
         Else if a S-Block response has been received
         Then
            try_counter is reseted.
------------------------------------------------------------------------------*/
         else if (response == GE_HI_RESYNCH)
         {
            try_counter = 0;
            cmdlen_used = CmdLen;
            resynch = FALSE;
         }
/*------------------------------------------------------------------------------
         If a communication error different from a R-Block message is raised
         Then
            cmdlen_used and timeout are initialized to send a R-Block.
------------------------------------------------------------------------------*/
         else if (response != GE_HI_NACK)
         {
            cmdlen_used = 0;
            resynch     = FALSE;
            timeout     = HOR3COMM_NACK_TIME;
         }
      }
      cmdlen_used = 0;
      resynch     = TRUE;
      timeout     = HOR3COMM_NACK_TIME;
   }
/*------------------------------------------------------------------------------
   When we have exceeded the try counter, no communication is possible: RspLen is
   set to 0 and
   Release the serial communication semaphore.
<= G_Oros3ReadResp status.
------------------------------------------------------------------------------*/
   *RspLen = 0;
   return (response);
}


