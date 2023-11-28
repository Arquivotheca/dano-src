/*****************************************************************
/
/ File   :   RMInterface.c
/ Author :   Prateesh C.
/ Date   :   
/ Purpose:   Interface module between Resource Manager and the IFD Handler
/
******************************************************************/




#include "RMInterface.h"
#include "ReaderManagement.h"
#include "IFDerror.h"
#include "InterfaceCmd.h"
#include "defines.h"
#include "isoIO.h"
#include <string.h>


char *vendor_name = "Gemplus";
char *ifd_type = "Gemcore Based Reader";
char *ifd_serial = "Gemcore Serial No. Not used";

//prototype declaration of ISO I/O defined in isoIO.c

extern INT16 IccIsoInput
			(
		   const WORD32        handle,
		   const WORD8    Command[5],
		   const WORD8    Data[],
		         WORD16  *RespLen,
		         BYTE     RespBuff[]
			);

extern INT16 IccIsoOutput
			(
		   const WORD32        handle,
		   const WORD8    Command[5],
	         WORD16 *RespLen,
	         BYTE     RespBuff[]
			);


RESPONSECODE IO_Create_Channel(DWORD ChannelID)
{
	RESPONSECODE response;

      put_msg("\n\nRMInterface.c: IO_Ceate_Channel: Inside IO_Create_Channel");
      response=EstablishConnection(ChannelID);  //defined in Reader Mgmt Module

	if(response!=IFD_SUCCESS)
	  {
	 return IFD_COMMUNICATION_ERROR;
	  }


     put_msg("\n\nRMInterface.c: IO_Ceate_Channel: Initialize Datastructures");

	
    //initialize variables with default data

	Device.Vendor_Name = vendor_name;
	Device.IFD_Type = ifd_type;
	Device.IFD_Serial = ifd_serial;
	Device.IFD_Channel_ID = ChannelID;
	Device.Asynch_Supported = 0x00000003; // Both T=0/T=1 Supported
	Device.Default_Clock = 3680;
	Device.Max_Clock = 3680;
	Device.Default_Data_Rate = 9600;
	Device.Max_Data_Rate = 115000;
	Device.Synch_Supported = 0;
	Device.Power_Mgmt = 1; // IFD Supports powerdown if ICC present;

	put_msg("\n\nChannel established");


  return IFD_SUCCESS;
}



RESPONSECODE IO_Close_Channel()   
{
	RESPONSECODE response;
	put_msg("\n\nRMInterface.c: IO_Close_Channel: Inside IO_Close_Channel");
	response=CloseConnection();       //defined in Reader Mgmt Module

	if(response!=IFD_SUCCESS)
	 {
      	  return IFD_COMMUNICATION_ERROR;
	 }

 return IFD_SUCCESS;
}

RESPONSECODE IFD_Get_Capabilities(DWORD Tag,BYTE Value[])
{
	DWORD HighNibble,LowNibble;
	
  put_msg("\n\nRMInterface.c: IO_Get_Capabilities: Inside IO_Get_Capabilities");
	
	HighNibble=Tag>>8;
	LowNibble=Tag-(HighNibble<<8);

	switch(HighNibble)
	{
	case 0x01:{
		switch(LowNibble)
		{
		case 0x00:
		    memcpy(Value,Device.Vendor_Name,sizeof(Device.Vendor_Name));
	   	    break;
	 	case 0x01:
		    memcpy(Value,Device.IFD_Type,sizeof(Device.IFD_Type));
	   	    break;
		case 0x02:
			*(DWORD*)Value=Device.IFD_Version;
			break;
		case 0x03:
		    memcpy(Value,Device.IFD_Serial,sizeof(Device.IFD_Serial));
	   	    break;
	   	case 0x10:
			*(DWORD*)Value=Device.IFD_Channel_ID;
			break;
		case 0x20:
			*(DWORD*)Value=Device.Asynch_Supported;
			break;
			
		case 0x21:
			*(DWORD*)Value=Device.Default_Clock;
			break;
		case 0x22:
			*(DWORD*)Value=Device.Max_Clock;
			break;
		case 0x23:
			*(DWORD*)Value=Device.Default_Data_Rate;
			break;
		case 0x24:
			*(DWORD*)Value=Device.Max_Data_Rate;
			break;
		case 0x25:
			*(DWORD*)Value=Device.Max_IFSD;
			break;
		case 0x26:
			*(DWORD*)Value=Device.Synch_Supported;
			break;
		case 0x31:
			*(DWORD*)Value=Device.Power_Mgmt;
			break;
		case 0x40:
			*(DWORD*)Value=Device.Card_Auth_Devices;
			break;
		case 0x42:
			*(DWORD*)Value=Device.User_Auth_Device;
			break;
		case 0x50:
			*(DWORD*)Value=Device.Mechanics_Supported;
			break;
		default: 
			return IFD_ERROR_TAG;
			break;
    	        }
	   }
	   break;
	case 0x02:{
		switch(LowNibble)
		{
		case 0x01:
			*(DWORD*)Value=Protocol.Protocol_Type;
			break;
		case 0x02:
			*(DWORD*)Value=Protocol.Current_Clock;
			break;
		case 0x03:
			*(DWORD*)Value=Protocol.Current_F;
			break;
		case 0x04:
			*(DWORD*)Value=Protocol.Current_D;
			break;
		case 0x05:
			*(DWORD*)Value=Protocol.Current_N;
			break;
		case 0x06:
			*(DWORD*)Value=Protocol.Current_W;
			break;
		case 0x07:
			*(DWORD*)Value=Protocol.Current_IFSC;
			break;
		case 0x08:
			*(DWORD*)Value=Protocol.Current_IFSD;
			break;
		case 0x09:
			*(DWORD*)Value=Protocol.Current_BWT;
			break;
		case 0x0A:
			*(DWORD*)Value=Protocol.Current_CWT;
			break;
		case 0x0B:
			*(DWORD*)Value=Protocol.Current_EBC;
			break;
		default: 
			return IFD_ERROR_TAG;
			break;
    	        }
	   }
	   break;
	case 0x03:{    
		IFD_Is_ICC_Present(); //initialize struct  ICC
		switch(LowNibble)
		{
		case 0x00:	*(BYTE*)Value = ICC.ICC_Presence;			break;	// 1 byte
		case 0x01:	*(BYTE*)Value = ICC.ICC_Interface_Status;	break;	// 1 byte
			
		case 0x03:
			if((ICC.ICC_Presence==2)&&(ICC.ICC_Interface_Status))
			{
			memcpy(Value,ICC.ATR,sizeof(ICC.ATR));
			}	
			else
			{
			Value[0]=0;
			}
			break;
		case 0x04:
			*(BYTE*)Value=ICC.ICC_Type;
			break;
		case 0x50:
			*(DWORD*)Value=ICC.ICC_LOCK_ID;
			break;
		case 0x51:
			*(DWORD*)Value=ICC.ICC_LOCK;
			break;
		default:
			return IFD_ERROR_TAG;
			break;
    	        }
	    }
	   break;
	default: 
		return IFD_ERROR_TAG;
		break;
 }

return IFD_SUCCESS;
}

RESPONSECODE IFD_Set_Capabilities(DWORD Tag,BYTE Value[])
{

	DWORD HighNibble,LowNibble;

	put_msg("\n\nRMInterface.c: Inside IO_Set_Capabilities");

	HighNibble=Tag>>8;
	LowNibble=Tag-(HighNibble<<8);

	switch(HighNibble)
	{
	case 0x03:{
		switch(LowNibble)
		{
		case 0x50:ICC.ICC_LOCK_ID=*(DWORD*)Value;	break;
		case 0x51:ICC.ICC_LOCK=*(DWORD*)Value;		break;
		default:break;
		}
	   }
	   break;
	default: break;
	}

 return IFD_SUCCESS;
}

RESPONSECODE IFD_Set_Protocol_Parameters(DWORD ProtocolType,BYTE SelectionFlags,BYTE PTS1,BYTE PTS2,BYTE PTS3)
{

	put_msg("\n\nRMInterface.c:  Inside IFD_Set_Protocol_Parameters()");
	if(ProtocolType&0x7ffffffe)
	{
	return IFD_PROTOCOL_NOT_SUPPORTED;
	}
	ICC.ProtocolType=ProtocolType;
	ICC.SelectionFlags=SelectionFlags;
	ICC.PTS1=PTS1;	
	ICC.PTS2=PTS2;
	ICC.PTS3=PTS3;
	
 return IFD_SUCCESS;
}

RESPONSECODE IFD_Power_ICC(DWORD ActionRequested)
{
	RESPONSECODE response;
	WORD8 buf[BUFFER_SIZE];  //BUFFER_SIZE=261
	int l;

	put_msg("\n\nRMInterface.c: Inside IFD_Power_ICC():Action Requested=%ld"               ,ActionRequested);

	response=PowerICC(ActionRequested);      //defined in Reader Mgmt Module

	//if(response==IFD_SUCCESS)
	//  response=Translate(buf[0]);
	// else

	 if(response!=IFD_SUCCESS)
	   return IFD_ERROR_POWER_ACTION;


 return IFD_SUCCESS;
}


RESPONSECODE IFD_Swallow_ICC()
{
 return   IFD_ERROR_NOT_SUPPORTED;        // This is not supported.
}

RESPONSECODE IFD_Eject_ICC()
{
   return   IFD_ERROR_NOT_SUPPORTED;        // This is not supported.
}

RESPONSECODE IFD_Confiscate_ICC()
{
 return   IFD_ERROR_NOT_SUPPORTED;        // This is not supported.
}

RESPONSECODE IFD_Transmit_to_ICC( struct SCARD_IO_HEADER SendPci,
				BYTE CommandData[],DWORD CommandSize,
				BYTE ResponseData[],DWORD* ResponseSize,
				struct SCARD_IO_HEADER* RecvPci)
{

RESPONSECODE lRetVal;
  int i;
  DWORD lc, Prot;
  BYTE *CmdData;
  BYTE data_in[280], data_out[280];

  APDU_COMMAND ApduComm;
  APDU_RESPONSE ApduResp;

  INT16 response,slen,rlen;
  BYTE *tmp,sbuff[256],rbuff[256];

  //lc       = ((struct SCARD_IO_HEADER *)CommandData)->Length;
  lc       = CommandSize;


	Prot = (SendPci.Protocol & 0x3)-1; // convert protocol from libpcsc to ATR-style
  CmdData = CommandData; // + sizeof( struct SCARD_IO_HEADER );

  *ResponseSize = 0;
  
  //IFD_Is_ICC_Present(); // dont make call to this function everytime.

  if ( Prot != Protocol.Protocol_Type )
  {
	return IFD_COMMUNICATION_ERROR;
  }


  if ( lc < 4 )
  {
	return IFD_COMMUNICATION_ERROR;
  }
  
  tmp = (BYTE*)&ApduComm;
  memset(tmp, 0, sizeof(APDU_COMMAND));

  tmp = (BYTE*)&ApduResp;
  memset(tmp, 0, sizeof(APDU_RESPONSE));

  // COMMAND_LEN is defined as 4
  memcpy(ApduComm.Command, CmdData, COMMAND_LEN);

  ApduComm.DataIn = data_in;
  ApduResp.DataOut = data_out;
 
  if ( lc == 4 ) //Case1
  {
	ApduComm.LengthIn = ApduComm.LengthExpected = 0;
  }
  else if ( lc == 5 ) //Case2..ie Lin=0, Le > 0
  {
	ApduComm.LengthIn = 0;
	ApduComm.LengthExpected = CmdData[COMMAND_LEN];
  }
  else // Case3 & Case4 
  {
	ApduComm.LengthIn = CmdData[COMMAND_LEN];
	ApduComm.LengthExpected = 0; // Case3
	if ( ApduComm.LengthIn > 0 )
	{
		memcpy(ApduComm.DataIn, CmdData+COMMAND_LEN+1,ApduComm.LengthIn);
 	}

	if ( lc > COMMAND_LEN+1+ApduComm.LengthIn ) //Case4
	{
	ApduComm.LengthExpected = *(CmdData+COMMAND_LEN+1+ApduComm.LengthIn);
	}
  }


  if ( Prot == 0 ) // If T=0 protocol...
  {
	// This will prevent execution of both IsoIn/IsoOut commands
	// at a time..
	if ( (ApduComm.LengthIn > 0) && (ApduComm.LengthExpected > 0) )
	{
		return (IFD_COMMUNICATION_ERROR);
	}

  	response = ApduSpliter(
				&ApduComm,
				&ApduResp,
                        	IccIsoInput,
                        	IccIsoOutput
                        	);

  	if ( response <IFD_SUCCESS )
  	{
		return IFD_COMMUNICATION_ERROR;
  	}
        *ResponseSize = 2+ApduResp.LengthOut; // Added by Dave C

	for ( i = 0; i < ApduResp.LengthOut; ++i )
		{
		ResponseData[i] = ApduResp.DataOut[i];
		}


	ResponseData[i] = (ApduResp.Status & 0xff00) >> 8;
	ResponseData[i] &= 0xff;
	ResponseData[i+1] = (ApduResp.Status & 0x00ff);
  }
  
  else if ( Prot == 1 )
  {
	if 
	( 
              (ApduComm.LengthExpected > (BUFFER_SIZE - 5))
           || (   (ApduComm.LengthExpected == 0) 
               && (ApduComm.LengthIn > (BUFFER_SIZE - 6))
              )
           || (   (ApduComm.LengthExpected != 0) 
               && (ApduComm.LengthIn > (BUFFER_SIZE - 7))
              )
         )
         {
            return(IFD_COMMUNICATION_ERROR);
         }
	
	 slen = BUFFER_SIZE;

         response = ApduBuilder(&ApduComm, sbuff, &slen);
         if (response <IFD_SUCCESS)
         {
            return (IFD_COMMUNICATION_ERROR);
         }

	 
         rlen = BUFFER_SIZE;

	 response = ICCIsoT1
	  			  (
				   DEFAULT_TIME,	
				   slen,
				   sbuff,
				   &rlen,
				   rbuff
				  );


	if ( response <IFD_SUCCESS )
	{
//		response = Translate(rbuff[0]);
	}
	if ( (response <IFD_SUCCESS) || (rlen < 3) )
	{
		return (IFD_COMMUNICATION_ERROR);
	}

	ApduResp.LengthOut = rlen - 3;
	// Added by Atul..(two byte status word response..)
        *ResponseSize = 2+ApduResp.LengthOut; 

	if ( ApduResp.LengthOut > 0 )
	{
		memcpy(ResponseData, rbuff+1, ApduResp.LengthOut);
	}
	else
	{
		ApduResp.LengthOut = 0;
	}

	ResponseData[ApduResp.LengthOut] = rbuff[rlen-2];
	ResponseData[ApduResp.LengthOut+1] = rbuff[rlen-1];
  }

	if (RecvPci)
	{
	  RecvPci->Protocol = SendPci.Protocol;
	  RecvPci->Length = *ResponseSize;
	}
  
  return IFD_SUCCESS;

	}


RESPONSECODE IFD_Is_ICC_Present()
{
	int i;
	RESPONSECODE response;
	put_msg("\n\nRMInterface.c: Inside IFD_Is_ICC_Present()");

	for (i=0 ; i<10 ; i++)
	{
		response = UpdateCardStatus();
		if ((response == IFD_ICC_NOT_PRESENT) || (response == IFD_ICC_PRESENT))
			break;
		printf("IFD_Is_ICC_Present: IFD_COMMUNICATION_ERROR - retrying...\n");
	}
	
//ICC.ICC_Presence==response ; //either IFD_ICC_PRESENT or IFD_ICC_NOT_PRESENT

return response;
}

RESPONSECODE IFD_Is_ICC_Absent()
{

	RESPONSECODE response;

	put_msg("\n\nRMInterface.c: IFD_IS_ICC_Absent()");

	response=IFD_Is_ICC_Present();
	
return  response;

} 

