



#include "./ReaderManagement.h"
#include "./RMInterface.h"
#include "./InterfaceCmd.h"
#include "./IFDerror.h"
#include "./T0cases.h"
#include  <stdio.h>





#define CASE_1_LEN              4lu
#define CASE_2S_LEN             5lu
#define CASE_2E_LEN             7lu
#define CASE_3S_LEN             5lu
#define CASE_3E_LEN             7lu
#define CASE_4S_LEN             6lu
#define CASE_4E_LEN             9lu

#define EXTEND_FLAG             0



extern struct DEVICE_CAPABILITIES Device;
extern struct ICC_STATE ICC;
extern struct PROTOCOL_OPTIONS Protocol;




INT16 ifdpowerUP(const BYTE ICCVcc,const BYTE PTSMode,const BYTE PTS0,
		const BYTE PTS1,const BYTE PTS2,const BYTE PTS3)
		
{


	//power up code

	RESPONSECODE response;


	WORD8 cmd[7],CFG,PCK;
	int len=0,i,l,offset,k;
	INT16 rlen;
	WORD8  rbuf[256];

	put_msg("ReaderManagement.c:ifdPowerUP():IFD Power UP");


{ // Magic Command to receive a correct ATR
	INT16 rlen;
	WORD8  rbuf[256];
	WORD8 c[3] = {1, 0, 0};
	Exchange(handle,3,c,&rlen,rbuf);
}

	cmd[len++] =IFD_CMD_ICC_POWER_UP;

	   switch(ICCVcc)
	   {
	   case ICC_VCC_3V:
	      CFG = 0x02;
	      break;
	   case ICC_VCC_5V:
	      CFG = 0x01;
	      break;
	   default:
	      CFG = 0x00;
	      break;
	   }

	   switch(PTSMode)
	   {
	   case IFD_WITHOUT_PTS_REQUEST:
	      CFG |= 0x10;
	      cmd[len++] = CFG;
	     response = Exchange(handle,len,cmd,&rlen,rbuf);
	      break;
	   case IFD_NEGOTIATE_PTS_OPTIMAL:
	      CFG |= 0x20;
	      cmd[len++] = CFG;
	      response = Exchange(handle,len,cmd,&rlen,rbuf);
	      break;
	   case IFD_NEGOTIATE_PTS_MANUALLY:
	      // first reset Icc without PTS management
	      CFG |= 0x10;
	      cmd[len++] = CFG;
	      response = Exchange(handle,len,cmd,&rlen,rbuf);
	   if (response == IFD_SUCCESS)
	   {
	   // then send PTS parameters
	   	len = 1;
	   	CFG |= 0xF0;
		cmd[len++] = CFG;
		cmd[len++] = PTS0;
		if ((PTS0 & IFD_NEGOTIATE_PTS1) != 0)
			cmd[len++] = PTS1;
		if ((PTS0 & IFD_NEGOTIATE_PTS2) != 0)
			cmd[len++] = PTS2;
		if ((PTS0 & IFD_NEGOTIATE_PTS3) != 0)
			cmd[len++] = PTS3;
	// computes the exclusive-oring of all characters from CFG to PTS3
		PCK = 0xFF;
		for (i=2; i<len; i++)
		{
			PCK ^= cmd[i];
		}
		cmd[len++] = PCK;
	      response = Exchange(handle,len,cmd,&rlen,rbuf);
	   }
	   break;
	   case IFD_DEFAULT_MODE:
	   default:
	      if (CFG != 0x00)
	      {
	         cmd[len++] = CFG;
	      }
	     response = Exchange(handle,len,cmd,&rlen,rbuf);
	      break;
	   }
	
	


	//if(response==IFD_SUCCESS)
	// response=Translate(rbuf[0]);
	
	if(response!=IFD_SUCCESS)
	 return IFD_ERROR_POWER_ACTION;
	
	  memset(ICC.ATR, 0, sizeof(ICC.ATR));
	
       	  memcpy(ICC.ATR, rbuf+1, rlen-1);	
	
	 l = 1;

	while (ICC.ATR[l] & 0x80)
	{
		offset = 0;
		for(k = 0x10; k > 0;  k <<=1)
         	{
			if ( ICC.ATR[l] & k)
            		{
				offset++;
            		}
         	}
         	l += offset;
		Protocol.Protocol_Type = (WORD16)(ICC.ATR[l] & 0x0F);
	}
return IFD_SUCCESS;
}


INT16 ifdpowerDOWN()
{
	WORD8 command[]={IFD_CMD_ICC_POWER_DOWN};
	INT16 rlen,len;
	WORD8 rbuf[256];
	int i;

	RESPONSECODE response;

	put_msg("ReaderManagement.c:ifdPowerDOWN():IFD Power DOWN");
	
	rlen = BUFFER_SIZE;

	response = Exchange(handle,1,command,&rlen,rbuf);

	//	if ( response == IFD_SUCCESS )
	//	{
	//		response = Translate(rbuf[0]);
	//	}

	if ( response != IFD_SUCCESS )
	{
		return IFD_ERROR_POWER_ACTION;
	}


 return IFD_SUCCESS;
}



INT16 ifdRESET()
{

	RESPONSECODE response;
	BYTE ICCVcc;
	BYTE PTSMode,PTS0,PTS1,PTS2,PTS3;
	INT16 rlen;
	WORD8 rbuf[256];
	int l,offset,k;

	ICCVcc=  ICC_VCC_5V;

	put_msg("ReaderManagement.c:ifdPowerRESET():IFD Power RESET");
	
	rlen = BUFFER_SIZE;
	if ( ICC.SelectionFlags !=0 )
	{
		PTSMode= IFD_NEGOTIATE_PTS_MANUALLY;
		PTS0 = ICC.SelectionFlags;
		PTS1 = ICC.PTS1;
		PTS2 = ICC.PTS2;
		PTS3 = ICC.PTS3;
	}
	else
	{
		PTSMode = PTS0 = PTS1 = PTS2 = PTS3 = 0;
	}

	if ( PTS0 > 0x07 )
	{
		PTS0 &= 0x07;
	}

	rlen = BUFFER_SIZE;

//Power Up
	response= ifdpowerUP(ICCVcc,PTSMode,PTS0,PTS1,PTS2,PTS3);

	//	if ( response == IFD_SUCCESS)
	//	{
	//		response = Translate(rbuf[0]);
	//	}

	if ( response !=IFD_SUCCESS)
	{

		return IFD_ERROR_POWER_ACTION;
	}


return IFD_SUCCESS;
}

INT16 EstablishConnection(DWORD ChannelID)
{

	if((ChannelID&0x00010020)==0x00010020) //USB Port
  	{
	 handle= OpenReader(ChannelID%256);
	if(handle<0)
	  return IFD_COMMUNICATION_ERROR;

	put_msg("\n\nReaderManagement.c:EstablishConnection():Handle retuned i		s:%d",handle);

	}
	else
	{
	 return IFD_NOT_SUPPORTED;
	}

	put_msg("\n\nConnection established");

return IFD_SUCCESS;   

}

INT16 CloseConnection()
{
	RESPONSECODE response;
	  response= CloseReader(handle);  //low level driver interface module

	if(response!=IFD_SUCCESS)
	return IFD_COMMUNICATION_ERROR;

return IFD_SUCCESS;
}


INT16 PowerICC(DWORD ActionRequested)
{
	if(ActionRequested==IFD_POWER_UP)
	{
		RESPONSECODE response;
		int l , offset,len;
		WORD8 * rbuf;
		INT16 rlen;
		BYTE ICCVcc,PTSMode,PTS0,PTS1,PTS2,PTS3;

		ICCVcc=  ICC_VCC_5V;
		len=0;
        	
		response=ifdpowerDOWN();

	//	if(response>G_OK)
	//	  response=Translate(rbuf);

		if(response!=IFD_SUCCESS)
		 return IFD_ERROR_POWER_ACTION;
		
		if(ICC.SelectionFlags!=0)
		{
		PTSMode=IFD_NEGOTIATE_PTS_MANUALLY;
		PTS0=ICC.SelectionFlags;
		PTS1=ICC.PTS1;
		PTS2=ICC.PTS2;
		PTS3=ICC.PTS3;
		}
		else
		{
		PTSMode=PTS0=PTS1=PTS2=PTS3=0;
		}
		
		if(PTS0>0x07)
		  PTS0 &=0x07;
		        	
		  ifdpowerUP(ICCVcc,PTSMode,PTS0,PTS1,PTS2,PTS3);

		put_msg ("power up complete");
	
	}
	else
	if(ActionRequested==IFD_POWER_DOWN)
	{
	  ifdpowerDOWN();
	}	
	else
	if(ActionRequested==IFD_RESET)
	{
	  ifdRESET();
	}

return IFD_SUCCESS;
}


INT16  ApduSpliter
(
   
   const APDU_COMMAND 		*ApduComm,
         APDU_RESPONSE   		*ApduResp,
         INT16     		 *IsoIn
		(
		const WORD32        Timeout,
		const WORD8  	  Command[5],
		const WORD8    	  Data[],
		WORD16 		 *RespLen,
		BYTE   		  RespBuff[]
		),
		INT16       	 *IsoOut
		(
		const WORD32        Timeout,
		const WORD8  	  Command[5],
		WORD16 		 *RespLen,
		BYTE   		  RespBuff[]
		)

)
{

put_msg("ReaderManagement: ApduSpliter()");
put_msg("LengthExpected=%d,LengthIn=%d",ApduComm->LengthExpected,ApduComm->LengthIn); 

 if (ApduComm->LengthExpected == 0)
   {
      if (ApduComm->LengthIn == 0)
      {
         return
         (
            T0Case1(handle,ApduComm, ApduResp,IsoIn)
         );
      }
      else if (ApduComm->LengthIn <= T0CASES_LIN_SHORT_MAX)
      {
         return(T0Case2S(handle,ApduComm,ApduResp,IsoIn));
      }
      ApduResp->LengthOut = 0;
      ApduResp->Status    = T0CASES_WRONG_LENGTH;
      return (IFD_SUCCESS);
   }
   else if (ApduComm->LengthIn == 0)
   {
      if (ApduComm->LengthExpected <= T0CASES_LEX_SHORT_MAX)
      {
	return	(T0Case3S(handle,ApduComm,ApduResp,IsoOut));
      }
      else
      {
         return (T0Case3E(handle,ApduComm,ApduResp,IsoOut));
      }
   }
   else if (   (ApduComm->LengthIn        <= T0CASES_LIN_SHORT_MAX)
            && (ApduComm->LengthExpected  <= T0CASES_LEX_SHORT_MAX)
           )
   {
      return(T0Case4S(handle,ApduComm,ApduResp,IsoIn,IsoOut));
   }
   else
   {
     return(T0Case4E(handle,ApduComm,ApduResp,IsoIn,IsoOut));
   }



}



INT16 ApduBuilder( APDU_COMMAND  *ApduComm,
         WORD8       *Buffer,
         WORD32      *Length
		)
{
	
	  WORD8  *   ptr;

	   if (*Length < COMMAND_LEN)
	   {
	      return (GE_HI_CMD_LEN);
	   }	
	   memcpy(Buffer,ApduComm->Command,COMMAND_LEN);

	   if (ApduComm->LengthIn == 0)
	   {
	      if (ApduComm->LengthExpected == 0)
	      {
	         if (*Length < CASE_1_LEN)
	         {
	            return (GE_HI_CMD_LEN);
	         }
	         *Length = CASE_1_LEN;
	      }
    	else
	     {	
	        if (ApduComm->LengthExpected <= 256)
	        {
	           if (*Length < CASE_3S_LEN)
	           {
	              return (GE_HI_CMD_LEN);
	           }
	           *(Buffer + COMMAND_LEN) = LOBYTE(ApduComm->LengthExpected);
	           *Length = CASE_3S_LEN;
	        }
     	else
        	 {
	            if (*Length < CASE_3E_LEN)
	            {
	               return (GE_HI_CMD_LEN);
	            }
	            if (ApduComm->LengthExpected > 65536lu)
	            {
	               return (GE_APDU_LE);
	            }
	            *(Buffer + COMMAND_LEN) = EXTEND_FLAG;
        	 *(Buffer + COMMAND_LEN + 1) = HIBYTE(ApduComm->LengthExpected);
	         *(Buffer + COMMAND_LEN + 2) = LOBYTE(ApduComm->LengthExpected);
	         *Length = CASE_3E_LEN;
	        }	
	     }
	  }
	   else
	  {
	      if (ApduComm->LengthExpected == 0)
	      {
	         if (ApduComm->LengthIn < 256)
	         {
	            if (*Length < CASE_2S_LEN + ApduComm->LengthIn)
	            {
	               return (GE_HI_CMD_LEN);
	            }
	            *(Buffer + COMMAND_LEN) = LOBYTE(ApduComm->LengthIn);
	            ptr = Buffer + CASE_2S_LEN;
	            memcpy(ptr,ApduComm->DataIn,(size_t)ApduComm->LengthIn);
        	    *Length = CASE_2S_LEN + ApduComm->LengthIn;
	         }
	       else
	         {
	            if (*Length < CASE_2E_LEN + ApduComm->LengthIn)
	            {
        	       return (GE_HI_CMD_LEN);
	            }
	            if (ApduComm->LengthIn > 65535)
	            {
	               return (GE_APDU_LE);
	            }
	            *(Buffer + COMMAND_LEN) = EXTEND_FLAG;
	            *(Buffer + COMMAND_LEN + 1) = HIBYTE(ApduComm->LengthIn);
	            *(Buffer + COMMAND_LEN + 2) = LOBYTE(ApduComm->LengthIn);
	            ptr = Buffer + CASE_2E_LEN;
	            memcpy(ptr,ApduComm->DataIn,(size_t)ApduComm->LengthIn);
	            *Length = CASE_2E_LEN + ApduComm->LengthIn;
        	 }
	      }
	  else
	      {
	    if ((ApduComm->LengthIn < 256) && (ApduComm->LengthExpected <= 256))
	         {
	            if (*Length < CASE_4S_LEN + ApduComm->LengthIn)
	            {
	               return (GE_HI_CMD_LEN);
	            }
	            *(Buffer + COMMAND_LEN) = LOBYTE(ApduComm->LengthIn);
	            ptr = Buffer + COMMAND_LEN + 1;
	            memcpy(ptr,ApduComm->DataIn,(size_t)ApduComm->LengthIn);
	            ptr += ApduComm->LengthIn;
	            *ptr = LOBYTE(ApduComm->LengthExpected);
	            *Length = CASE_4S_LEN + ApduComm->LengthIn;
	         }
	   else
	         {
	            if
	            (
	               (ApduComm->LengthIn > 0xFFFFFFFFlu - CASE_4E_LEN)
	            || (*Length < CASE_4E_LEN + ApduComm->LengthIn)
	            )
	            {
	               return (GE_HI_CMD_LEN);
	            }
	            if (ApduComm->LengthExpected > 65536L)
	            {
	               return (GE_APDU_LE);
	            }
	            *(Buffer + COMMAND_LEN) = EXTEND_FLAG;
	            *(Buffer + COMMAND_LEN + 1) = HIBYTE(ApduComm->LengthIn);
	            *(Buffer + COMMAND_LEN + 2) = LOBYTE(ApduComm->LengthIn);
	            ptr = Buffer + COMMAND_LEN + 3;
	            memcpy(ptr,ApduComm->DataIn,(size_t)ApduComm->LengthIn);
	            ptr += ApduComm->LengthIn;
	            *ptr++ = HIBYTE(ApduComm->LengthExpected);
	            *ptr   = LOBYTE(ApduComm->LengthExpected);
	            *Length = CASE_4E_LEN + ApduComm->LengthIn;
	         }
	      }
	   }

  return IFD_SUCCESS;
}

RESPONSECODE UpdateCardStatus()
{

	 RESPONSECODE response;
	 INT16 rlen;
	 WORD8 rbuf[256];

	  WORD8 cmd[2] = {IFD_CMD_ICC_STATUS};
	  rlen = MAX_IFD_STRING;
	
	  response = Exchange(handle, 1, cmd, &rlen, rbuf);
	
	  if ( response != IFD_SUCCESS )
	  {
		return IFD_NOT_SUPPORTED; // should not return this
	  }

	  ICC.ICC_Presence = ICC.ICC_Interface_Status = 0;

	  if ( rlen == 7 )
	  {
		if ( (rbuf[1] & 0x04) == 0 ) // ICC ABSENT
		{
		        put_msg("ICC absent");
			ICC.ICC_Presence = 0;
			// Make ATR and everything to 0s..
			//memset(&ICC, 0, sizeof(ICC));
			ICC.ICC_Presence = 0;
			ICC.ICC_Interface_Status = 0;
			memset(ICC.ATR, 0, sizeof(ICC.ATR));
			ICC.ICC_Type = 0;
			return IFD_ICC_NOT_PRESENT;
		}
		else if ( (rbuf[1] & 0x02) == 0 ) // ICC PRESENT. POWER-DN
		{        put_msg("ICC present power down");
			ICC.ICC_Presence = 2;
			ICC.ICC_Interface_Status = 0; // Contact Inactive..
			return IFD_ICC_PRESENT;
		}
		else if ( (rbuf[1] & 0x08) == 0) // ICC PRESENT. POWERUP
		{       put_msg("icc present powerup");
			ICC.ICC_Presence = 2;
			ICC.ICC_Interface_Status = 1; // Contact Active..
			Protocol.Protocol_Type = 0;
			if ( (rbuf[2] != ISOCARD) && (rbuf[2] != FASTISOCARD) )
			{
				ICC.ICC_Type = 0; // Non ISO Card
			}
			else
			{
				ICC.ICC_Type = 1; // ISO Card
			}
			return IFD_ICC_PRESENT;
		}
		else if ( (rbuf[2] == ISOCARD) || (rbuf[2] == FASTISOCARD) )
		{
			ICC.ICC_Presence = 2;
			ICC.ICC_Interface_Status = 1; //Contact Active..
			Protocol.Protocol_Type = 1;
			ICC.ICC_Type = 1;	
			return IFD_ICC_PRESENT;
		}
		else
		{
			ICC.ICC_Type = 0;
		}
	  }
	
  return IFD_ICC_NOT_PRESENT;
}






INT16 IsoT1
(
   const WORD32       handle,
   const WORD8         OrosCmd,
   const WORD16        ApduLen,
   const WORD8    ApduCommand[],
         WORD16  *RespLen,
         BYTE     RespBuff[]
)
{
	INT16	   response;
	WORD8   cmd[BUFFER_SIZE];
	WORD16   length_expected,  resp_len;
	BYTE   resp_buff[BUFFER_SIZE];

	   cmd[0] = OrosCmd;

	   if (ApduLen > 5)
	   {
	      if (ApduLen > (WORD16)(5 + ApduCommand[4]))
	      {
	         length_expected = ApduCommand[5 + ApduCommand[4]];
	         if (length_expected == 0)
	         {
        	    length_expected = 256;
	         }
	      }
	      else
	      {
	         length_expected = 0;
	      }
	   }
	   else if (ApduLen == 5)
	   {
	      length_expected = ApduCommand[4];
	      if (length_expected == 0)
	      {
	         length_expected = 256;
	      }	
	   }
	   else if (ApduLen == 4)
	   {
	      length_expected = 0;
	   }	
	   else
	   {
	      return (GE_HI_CMD_LEN);
	   }	
	
	   if (length_expected + 3 > *RespLen)
	   {
	      return (GE_HI_CMD_LEN);
	   }
	   if (ApduLen > 261)
	   {
	      return (GE_HI_CMD_LEN);
	   }
	   if (ApduLen <= (MAX_DATA - 1))
	   {
	      memcpy(cmd + 1, ApduCommand, ApduLen);
	      (WORD16) (ApduLen + 1),
      response = Exchange(       cmd,
                                 RespLen,
                                 RespBuff
                                );
   	}
	   else	
	   {
	      memcpy(cmd + 1,"\xFF\xFF\xFF\xFF", 4);
	      cmd[5] = (WORD8) (ApduLen - (MAX_DATA - 1));
	      memcpy(cmd + 6, ApduCommand + (MAX_DATA - 1), cmd[5]);
	      resp_len = *RespLen;

	      response = Exchange
                                 (
                                 (WORD16)(6 + cmd[5]),
                                 cmd,
                                 RespLen,
                                 RespBuff
                                 );
if ((response !=IFD_SUCCESS ) || (RespBuff[0] != 0x00) || (*RespLen != 1))
	      {
        	 return(response);
	      }

      memcpy(cmd + 1, ApduCommand,(MAX_DATA - 1));
      *RespLen = resp_len;
      response = Exchange(      (WORD16)MAX_DATA,
                                 cmd,
                                 RespLen,
                                 RespBuff
                                );
	   }
	   if ((length_expected > 252) && (RespBuff[0] == 0x1B) &&
	       (*RespLen == (MAX_DATA - 1)))
	   {
	      memcpy(cmd + 1,"\xFF\xFF\xFF\xFF", 4);
	      cmd[5] = (WORD8) (length_expected - *RespLen + 1);
	      cmd[5] += 2;
	      resp_len = BUFFER_SIZE;

	      response = Exchange(6,cmd,&resp_len,resp_buff);

      if ((response != IFD_SUCCESS) || (resp_buff[0] != 0x00))
	      {
        	 memcpy(RespBuff,resp_buff,resp_len);
	         *RespLen = resp_len;
	         return(response);
	      }
	
     	 memcpy(RespBuff + *RespLen,resp_buff + 1,resp_len - 1);
         *RespLen += (WORD16) (resp_len - 1);
   	}
	
   return(response);
}

INT16 ICCIsoT1
(
   const WORD32        handle,
   const WORD16        ApduLen,
   const WORD8    ApduCommand[],
         WORD16  *RespLen,
         BYTE     RespBuff[]
)
{
   return (IsoT1(
                        handle,
			IFD_CMD_ICC_APDU,
			ApduLen,
                        ApduCommand,
                        RespLen,
                        RespBuff
                        ));
}

