

#include  "./defines.h"
#include"./InterfaceCmd.h"
#include "./T0cases.h"
#include "./IFDerror.h"
#include "./isoIO.h"
#include <stdarg.h>
#include <stdio.h>



INT16 IsoInput
(
   const WORD32        handle,
   const WORD8         iCmd,
   const WORD8    Command[5],
   const WORD8    Data[],
         WORD16  *RespLen,
         BYTE     RespBuff[]
)
{
WORD8
   cmd[BUFFER_SIZE];
INT16
   response;
WORD16
   resp_len = *RespLen;

   cmd[0] = iCmd;
   if (Command[4] <= (MAX_DATA - 7))
   {

      memcpy(cmd + 1, Command, 5);
      memcpy(cmd + 6, Data, Command[4]);
      return
      (
         Exchange
         (
            handle,
            (WORD16)(6 + Command[4]),
            cmd,
            RespLen,
            RespBuff
         )
      );
   }
   else if (Command[4] <= T0CASES_LIN_SHORT_MAX)
   {
      memcpy(cmd + 1,"\xFF\xFF\xFF\xFF", 4);
      cmd[5] = (WORD8) (Command[4] - 248);
      memcpy(cmd + 6, Data + 248, cmd[5]);
      response = Exchange
                                 (
                                 handle,
                                 (WORD16)(6 + cmd[5]),
                                 cmd,
                                 &resp_len,
                                 RespBuff
                                 );
      if ((response != IFD_SUCCESS) || (RespBuff[0] != 0x00) || (resp_len != 1))
      {
         if ((response == IFD_SUCCESS) && (RespBuff[0] == 0x1B))
         {
            RespBuff[0] = 0x12;
         }
         return(response);
      }
      memcpy(cmd + 1, Command, 5);
      memcpy(cmd + 6, Data,248);
      return
      (
         Exchange
         (
            handle,
            (WORD16)(6 + 248),
            cmd,
            RespLen,
            RespBuff
         )
      );
   }
   else
   {
      return (GE_HI_CMD_LEN);
   }
}




INT16 IccIsoInput
(
   const WORD32        handle,
   const WORD8    Command[5],
   const WORD8    Data[],
         WORD16  *RespLen,
         BYTE     RespBuff[]
)
{
   return (IsoInput(
                           handle,
			   IFD_CMD_ICC_ISO_IN,
                           Command,
                           Data,
                           RespLen,
                           RespBuff
                           ));
}


INT16 IsoOutput
(
   const WORD32        handle,
   const WORD8         iCmd,
   const WORD8   Command[5],
         WORD16  *RespLen,
         BYTE    RespBuff[]
)
{
WORD8
   cmd[6];
INT16
   response;
WORD16
   resp_len;
BYTE
   resp_buff[BUFFER_SIZE];

   cmd[0] = iCmd;
   put_msg("isoIO.c: isoOutput(): handle is %d",handle);
   
   if ((Command[4] <= (MAX_DATA - 3)) && (Command[4] != 0))
   {
      memcpy(cmd + 1, Command, 5);
      return (Exchange(handle,6,cmd,RespLen,RespBuff));
   }
   else if ((Command[4] > (MAX_DATA - 3)) || (Command[4] == 0))
   {
      memcpy(cmd + 1, Command, 5);
      response = Exchange(handle,6,cmd,RespLen,RespBuff);
      if ((response != IFD_SUCCESS) || (RespBuff[0] != 0x00))
      {
         return(response);
      }
      memcpy(cmd + 1,"\xFF\xFF\xFF\xFF", 4);
      if (Command[4] == 0x00)
      {
         cmd[5] = (WORD8) (256 - ((WORD8) (*RespLen - 1)));
      }
      else
      {
         cmd[5] -= ((WORD8) (*RespLen - 1));
      }
      resp_len = BUFFER_SIZE;
      response = Exchange(handle,6,cmd,&resp_len,resp_buff);
      
	if ((response != IFD_SUCCESS) || (resp_buff[0] != 0x00))
      {
         memcpy(RespBuff,resp_buff,resp_len);
         *RespLen = resp_len;
         return(response);
      }
      memcpy(RespBuff + *RespLen,resp_buff + 1,resp_len - 1);
      *RespLen += (WORD16) (resp_len - 1);
      return(response);
   }
   else
   {
      return (GE_HI_CMD_LEN);
   }
}


INT16 IccIsoOutput
(
   const WORD32        handle,
   const WORD8    Command[5],
         WORD16 *RespLen,
         BYTE     RespBuff[]
)
{
RESPONSECODE response;
int i;
put_msg("isoIO.c: IccIsoOutput()");
  response=(IsoOutput(
                            handle,
			    IFD_CMD_ICC_ISO_OUT,
                            Command,
                            RespLen,
                            RespBuff
                            ));
put_msg("isoIO.c: ICCISoOutput(): Response length is %d",*RespLen);
{
int i;
for(i=0;i<*RespLen;i++)
put_msg("#%x#",RespBuff[i]);
}
return response;
}





