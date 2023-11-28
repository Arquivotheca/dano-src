

#include<stdio.h>
#include "./T0cases.h"
#include "./IFDerror.h"
#include "defines.h"


INT16 T0Case1
(
   const WORD32                handle,
   const APDU_COMMAND 	   *ApduComm,
         APDU_RESPONSE 	  *ApduResp,
         INT16      	  *IsoIn
         
	(
             const WORD32        handle,
             const WORD8    Command[5],
             const WORD8    Data[],
                  WORD16  *RespLen,
                  BYTE    RespBuff[]
         )
)
{
	WORD8 cmd [5], resp[3];
	WORD16   rlen = 3;
	INT16   response;

	   memcpy(cmd,ApduComm->Command,COMMAND_LEN);
	   cmd[T0CASES_P3] = 0;

	   response = IsoIn(handle,cmd,NULL,&rlen,resp);
	if (response ==IFD_SUCCESS)
	   {
	 //     response = Translate(resp[0]);
	   }
	   if (response !=IFD_SUCCESS)
	   {
	      return (response);
	   }
	   if (rlen < 3)
	   {
	      return (GE_HI_LEN);
	   }
	   ApduResp->LengthOut = 0;
	   ApduResp->Status    = (WORD16)((resp[rlen - 2]<<8) + resp[rlen - 1]);

   return (IFD_SUCCESS);
}

INT16  T0Case2S
(
   const WORD32                handle,
   const APDU_COMMAND 	  *ApduComm,
         APDU_RESPONSE    *ApduResp,
         INT16         *IsoIn
         (
            const WORD32        handle,
            const WORD8    Command[5],
            const WORD8    Data[],
                  WORD16  *RespLen,
                  BYTE    RespBuff[]
         )
)
{
	BYTE   cmd [6],   resp[3];
	WORD16   rlen = 3;
	INT16   response;

   	memcpy(cmd,ApduComm->Command,COMMAND_LEN);
	cmd[T0CASES_P3] = (BYTE)ApduComm->LengthIn;
   
	response =IsoIn(handle,cmd,ApduComm->DataIn,&rlen,resp);

	   if (response ==IFD_SUCCESS)
	   {
	//      response = Translate(resp[0]);
	   }
	   if (response !=IFD_SUCCESS)
	   {
	      return (response);
	   }
	   if (rlen < 3)
	   {
	      return (GE_HI_LEN);
	   }
	   ApduResp->LengthOut = 0;
	   ApduResp->Status    = (WORD16)((resp[rlen - 2]<<8) + resp[rlen - 1]);

   return (IFD_SUCCESS);
}



INT16  T0Case3S
(
   const WORD32                handle,
   const APDU_COMMAND  		*ApduComm,
        APDU_RESPONSE 		 *ApduResp,
         INT16        * IsoOut
         (
            const WORD32        handle,
            const WORD8 	  Command[5],
                  WORD16 	 *RespLen,
                  BYTE   	  RespBuff[]
         )
)
{
	BYTE   cmd [5],   resp[T0CASES_MAX_SIZE];
	WORD16   rlen = T0CASES_MAX_SIZE,   length;
	BYTE    *ptr;
	INT16   response;

	   memcpy(cmd,ApduComm->Command,COMMAND_LEN);
	   cmd[T0CASES_P3] = (BYTE)ApduComm->LengthExpected;
	   
	put_msg("T0Cases.c: T0Case3S():");

	response =IsoOut(handle,cmd,&rlen,resp);
 
	if (response == IFD_SUCCESS)
	   {
	//      response = Translate(resp[0]);
	   }
	   if (response != IFD_SUCCESS)
	   {
	      return (response);
	   }
	   if (rlen < 3)
	   {
	      return (GE_HI_LEN);
	   }

	   switch(resp[rlen - 2])
	   {
	      case T0CASES_CASE_3S_2:
	      {
		 ApduResp->LengthOut = 0;
	         break;
	      }
      		case T0CASES_CASE_3S_3:
		      {
		         cmd[T0CASES_P3] = resp[rlen - 1];
		         rlen = T0CASES_MAX_SIZE;
		         response = IsoOut(handle,cmd,&rlen,resp);
	         if (response == IFD_SUCCESS)
        	 {
	//            response = Translate(resp[0]);
	         }
        	 if (response !=IFD_SUCCESS)
	         {
        	    return (response);
	         }	
         	if (rlen == 3)
	         {
	            ApduResp->LengthOut = 0;
	         }
	         else
	         {
	            ApduResp->LengthOut=T0CASES_MIN
	            (
	               ApduComm->LengthExpected,
	               (WORD32)T0CASES_VAL(cmd[T0CASES_P3])
	            );
            memcpy(ApduResp->DataOut,resp + 1,(size_t)ApduResp->LengthOut);
	         }
	         break;
	      }
      		case T0CASES_CASE_3S_4:
	      case T0CASES_CASE_3S_4_SIM:
	      {
	         if (rlen == 3)
	         {
	            length = 0;
	         }
	         else
	         {
	            length = (WORD16)T0CASES_MIN
	            (
	               ApduComm->LengthExpected,
	               (WORD32)T0CASES_VAL(cmd[T0CASES_P3])
	            );
	            memcpy(ApduResp->DataOut,resp + 1,length);
	         }
	         ApduResp->LengthOut = length;

         	if (cmd[T0CASES_INS] == T0CASES_GET_RESPONSE)
	         {
	            cmd[T0CASES_P1] = (BYTE)((cmd[T0CASES_P1] + 1) % 2);
        	 }
	         else
	         {
	            cmd[T0CASES_INS] = T0CASES_GET_RESPONSE;
	            cmd[T0CASES_P1]  = 0x00;
	         }
	         cmd[T0CASES_P2]  = 0x00;
         
	while
         (
            (
               (resp[rlen - 2] == T0CASES_CASE_3S_4)
            || (resp[rlen - 2] == T0CASES_CASE_3S_4_SIM)
            )
         && (ApduResp->LengthOut < ApduComm->LengthExpected)
         )
         {
            cmd[T0CASES_P3]  = resp[rlen - 1];
            rlen = T0CASES_MAX_SIZE;
            response =IsoOut(handle,cmd,&rlen,resp);
            if (response == IFD_SUCCESS)
            {
       //        response = Translate(resp[0]);
            }
            if (response !=IFD_SUCCESS)
            {
               return (response);
            }
            if (rlen == 3)
            {
               break;
            }
            else
            {
               length = (WORD16)T0CASES_MIN
               (
                  ApduComm->LengthExpected,
                  (WORD32)T0CASES_VAL(cmd[T0CASES_P3])
               );
               ptr = ApduResp->DataOut + ApduResp->LengthOut;
               memcpy(ptr,resp + 1,length);
               ApduResp->LengthOut += length;
            }
            cmd[T0CASES_P1] = (BYTE)((cmd[T0CASES_P1] + 1) % 2);
         }
         break;
      }
        default: 
	{
	if (rlen == 3)
         {
	   
            ApduResp->LengthOut = 0;
         }
         else
         {
            ApduResp->LengthOut = T0CASES_MIN
            (
               ApduComm->LengthExpected,
               (WORD32)T0CASES_VAL(cmd[T0CASES_P3])
            );
            memcpy(ApduResp->DataOut,resp + 1,(size_t)ApduResp->LengthOut);
         }
      }
  } 

   ApduResp->Status = (WORD16) ((resp[rlen - 2]<<8) + resp[rlen - 1]);

   if (ApduComm->LengthExpected == ApduResp->LengthOut)
	{
	      return (IFD_SUCCESS);
	   }
	   else
	   {
	      return (GW_APDU_LE);
	   }

return IFD_SUCCESS;

}




INT16 T0Case3E
(
   const WORD32                handle,
   const APDU_COMMAND   *ApduComm,
         APDU_RESPONSE   *ApduResp,
         INT16        *IsoOut
         (
            const WORD32        handle,
            const WORD8    Command[5],
                  WORD16  *RespLen,
                  BYTE     RespBuff[]
         )
)
{
BYTE  cmd [5],  resp[T0CASES_MAX_SIZE];
WORD16  rlen = T0CASES_MAX_SIZE,  length;
BYTE   *ptr;
INT16   response;

   memcpy(cmd,ApduComm->Command,COMMAND_LEN);
   cmd[T0CASES_P3] = 0;
   
	response = IsoOut(handle,cmd,&rlen,resp);
	   if (response== IFD_SUCCESS)
	   {
	//      response = Translate(resp[0]);
	   }
	   if (response !=IFD_SUCCESS)
	   {
	      return (response);
	   }
   
	switch(resp[rlen - 2])
	   {
	      case T0CASES_CASE_3E_2A:
	      {
	         ApduResp->LengthOut = 0;
	         break;
	      }
      		case T0CASES_CASE_3E_2B:
	      {
	               cmd[T0CASES_P3] = resp[rlen - 1];
	         rlen = T0CASES_MAX_SIZE;
	         response = IsoOut(handle,cmd,&rlen,resp);
	         if (response == IFD_SUCCESS)
	         {
	//            response = Translate(resp[0]);
	         }
	         if (response != IFD_SUCCESS)
	         {
	            return (response);
	         }
         	if (rlen == 3)
	         {
	            ApduResp->LengthOut = 0;
	         }
	         else
	         {
	            ApduResp->LengthOut= T0CASES_MIN
	            (
	               ApduComm->LengthExpected,
	               (WORD32)T0CASES_VAL(cmd[T0CASES_P3])
	            );
            memcpy(ApduResp->DataOut,resp + 1,(size_t)ApduResp->LengthOut);
	         }
        	 break;
	      }
	      case T0CASES_CASE_3E_2D:
	      case T0CASES_CASE_3E_2D_SIM:
	      {
	         if (rlen == 3)
	         {
	            length = 0;
	         }
	         else
	         {
	            length = (WORD16) T0CASES_MIN
	            (
	               ApduComm->LengthExpected,
	               (WORD32)T0CASES_VAL(cmd[T0CASES_P3])
	            );
	            memcpy(ApduResp->DataOut,resp + 1,length);
	         }
	         ApduResp->LengthOut = length;

         if (cmd[T0CASES_INS] == T0CASES_GET_RESPONSE)
         {
            cmd[T0CASES_P1] = (BYTE)((cmd[T0CASES_P1] + 1) % 2);
         }
         else
         {
            cmd[T0CASES_INS] = T0CASES_GET_RESPONSE;
            cmd[T0CASES_P1]  = 0x00;
         }
         cmd[T0CASES_P2]  = 0x00;
         
	while
         (
            (
               (resp[rlen - 2] == T0CASES_CASE_3E_2D)
            || (resp[rlen - 2] == T0CASES_CASE_3E_2D_SIM)
            )
         && (ApduResp->LengthOut < ApduComm->LengthExpected)
         )
         {
            cmd[T0CASES_P3]  = resp[rlen - 1];
            rlen = T0CASES_MAX_SIZE;
            response = IsoOut(handle,cmd,&rlen,resp);
            if (response == IFD_SUCCESS)
            {
//               response = Translate(resp[0]);
            }
            if (response !=IFD_SUCCESS)
            {
               return (response);
            }
            if (rlen == 3)
            {
               break;
            }
            else
            {
               length = (WORD16) T0CASES_MIN
               (
                  ApduComm->LengthExpected,
                  (WORD32)T0CASES_VAL(cmd[T0CASES_P3])
               );
               ptr = ApduResp->DataOut + ApduResp->LengthOut;
               memcpy(ptr,resp + 1,length);
               ApduResp->LengthOut += length;
            }
            cmd[T0CASES_P1] = (BYTE)((cmd[T0CASES_P1] + 1) % 2);
         }
         break;
      }
      default:
      {
         if (rlen == 3)
         {
            ApduResp->LengthOut = 0;
         }
         else
         {
            ApduResp->LengthOut = T0CASES_MIN
            (
               ApduComm->LengthExpected,
               (WORD32)T0CASES_VAL(cmd[T0CASES_P3])
            );
            memcpy(ApduResp->DataOut,resp + 1,(size_t)ApduResp->LengthOut);
         }
      }
   }

   ApduResp->Status = (WORD16) ((resp[rlen - 2]<<8) + resp[rlen - 1]);

	   if (ApduComm->LengthExpected == ApduResp->LengthOut)
	   {
	      return (IFD_SUCCESS);
	   }
	   else
	   {
	      return (GW_APDU_LE);
	   }
return IFD_SUCCESS;
}



INT16 T0Case4S
(
   const WORD32                handle,
   const APDU_COMMAND   *ApduComm,
         APDU_RESPONSE   *ApduResp,
         INT16        *IsoIn
         (
            const WORD32        handle,
            const WORD8    Command[5],
            const WORD8    Data[],
                  WORD16  *RespLen,
                  BYTE     RespBuff[]
         ),
         INT16         *IsoOut
         (
            const WORD32        handle,
            const WORD8    Command[5],
                  WORD16  *RespLen,
                  BYTE     RespBuff[]
         )
)
{
APDU_COMMAND   apdu_in;
INT16   response;

   response = T0Case2S(handle,ApduComm,ApduResp,IsoIn);

   if (response!=IFD_SUCCESS)
   {
      return (response);
   }
   
	apdu_in.Command[0] = ApduComm->Command[0];
	   apdu_in.Command[1] = T0CASES_GET_RESPONSE;
	   apdu_in.Command[2] = 0;
	   apdu_in.Command[3] = 0;
	   apdu_in.LengthIn   = 0;
   
	switch(HIBYTE(ApduResp->Status))
	   {
	      case T0CASES_CASE_4S_2:
	      {
	         apdu_in.LengthExpected = ApduComm->LengthExpected;
	         return (T0Case3S(handle,&apdu_in,ApduResp,IsoOut));
	      }
      		case T0CASES_CASE_4S_3:
	      case T0CASES_CASE_4S_3_SIM:
	      {
	         apdu_in.LengthExpected = T0CASES_MIN
	         (
	            ApduComm->LengthExpected,
	            (WORD32)T0CASES_VAL(LOBYTE(ApduResp->Status))
	         );
	         response = T0Case3S(handle,&apdu_in,ApduResp,IsoOut);
         	if (response !=IFD_SUCCESS)
	         {
	            return (response);
	         }
         	else if (ApduComm->LengthExpected == ApduResp->LengthOut)
	         {
	            return (IFD_SUCCESS);
	         }
         	else
	         {
	            return (GW_APDU_LE);
	         }
	      }
	   }

   	return (GW_APDU_LE);
}



INT16 T0Case4E
(
   const WORD32                handle,
   const APDU_COMMAND    *ApduComm,
         APDU_RESPONSE    *ApduResp,
         INT16         *IsoIn
         (
            const WORD32        handle,
            const WORD8    Command[5],
            const WORD8    Data[],
                  WORD16  *RespLen,
                  BYTE     RespBuff[]
         ),
         INT16         *IsoOut
         (
            const WORD32        handle,
            const WORD8    Command[5],
                  WORD16  *RespLen,
                  BYTE     RespBuff[]
         )
         )
{
APDU_COMMAND   apdu_in;
INT16   response;

   if (ApduComm->LengthIn > 255)
   {
      ApduResp->LengthOut = 0;
      ApduResp->Status    = T0CASES_WRONG_LENGTH;
      return (GW_APDU_LE);
   }
   
	response = T0Case2S(handle,ApduComm,ApduResp,IsoIn);
	   if (response !=IFD_SUCCESS)
	   {
	      return (response);
	   }
   
	apdu_in.Command[0] = ApduComm->Command[0];
	   apdu_in.Command[1] = T0CASES_GET_RESPONSE;
	   apdu_in.Command[2] = 0;
	   apdu_in.Command[3] = 0;
	   apdu_in.LengthIn   = 0;
   
	switch(HIBYTE(ApduResp->Status))
	   {
	      case T0CASES_CASE_4E_1B:
	      {
	         apdu_in.LengthExpected = ApduComm->LengthExpected;
	         return (T0Case3E(handle,&apdu_in,ApduResp,IsoOut));
	      }
      		case T0CASES_CASE_4E_1C:
	      case T0CASES_CASE_4E_1C_SIM:
	      {
	         if (T0CASES_VAL(LOBYTE(ApduResp->Status)) < 256)
	         {
	            apdu_in.LengthExpected = LOBYTE(ApduResp->Status);
	            response = T0Case3S(handle,&apdu_in,ApduResp,IsoOut);
	            if (response !=IFD_SUCCESS)
	            {
	               return (response);
	            }
	            else
	            {
	               return (GW_APDU_LE);
        	    }
	         }
	         else
	         {
	            apdu_in.LengthExpected = ApduComm->LengthExpected;
        	    return (T0Case3E(handle,&apdu_in,ApduResp,IsoOut));
	         }
	     }
	   }
  
 return (GW_APDU_LE);
}

