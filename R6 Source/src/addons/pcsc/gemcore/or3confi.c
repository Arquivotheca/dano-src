#define G_UNIX

#define G_NAME     "Or3Confi"
#define G_RELEASE  "4.31.002"

#ifdef _MSC_VER
#pragma comment(exestr,"Gemplus(c) "G_NAME" Ver "G_RELEASE" "__DATE__)
#endif
#define __MSC

#include "gemplus.h"
#include "gemgcr.h"
#include "gttimout.h"
#include "or3comm.h"
#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
#include "gemansi.h"
#endif

#include "or3gll.h"

INT16 G_DECL G_Oros3SIOConfigure
(
   const WORD32        Timeout,
   const INT16         Parity,
   const INT16         ByteSize,
   const WORD32        BaudRate,
         WORD16 G_FAR *RspLen,
         WORD8  G_FAR  Rsp[]
)
{
WORD8
   cmd[2] = { HOR3GLL_IFD_CMD_SIO_SET };

   switch (BaudRate)
   {
      case 76800lu:
      {
         cmd[1] = 0x01;
         break;
      }
      case 38400lu:
      {
         cmd[1] = 0x02;
         break;
      }
      case 19200lu:
      {
         cmd[1] = 0x03;
         break;
      }
      case  9600lu:
      {
         cmd[1] = 0x04;
         break;
      }
      case  4800lu:
      {
         cmd[1] = 0x05;
         break;
      }
      case  2400lu:
      {
         cmd[1] = 0x06;
         break;
      }
      case  1200lu:
      {
         cmd[1] = 0x07;
         break;
      }
      default   :
      {
         return (GE_HOST_PARAMETERS);
      }
   }

   switch (ByteSize)
   {
      case 8 :
      {
         break;
      }
      case 7 :
      {
         cmd[1] += 0x08;
         break;
      }
      default:
      {
         return (GE_HOST_PARAMETERS);
      }
   }

   switch (Parity)
   {
      case 0 :
      {
         break;
      }
      case 2:
      {
         cmd[1] += 0x10;
         break;
      }
      default:
      {
         return (GE_HOST_PARAMETERS);
      }
   }

   return (G_Oros3Exchange(Timeout,2,cmd,RspLen,Rsp));
}

INT16 G_DECL G_Oros3SetMode
(
   const WORD32        Timeout,
   const WORD16        Option,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
WORD8
   cmd[3] = HOR3GLL_IFD_CMD_MODE_SET;

   cmd[2] = (WORD8)Option;

   return (G_Oros3Exchange(Timeout,3,cmd,RespLen,RespBuff));
}



