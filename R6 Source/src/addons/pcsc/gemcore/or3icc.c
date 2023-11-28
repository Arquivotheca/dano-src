#define G_UNIX

#include <string.h>

#include "gemplus.h"
#include "gemgcr.h"
#include "gtser.h"
#include "gttimout.h"
#include "gtgbp.h"
#include "or3comm.h"
#include "t0cases.h"
#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
#include "gemansi.h"
#endif

#include "or3gll.h"

#include "IFD_Handler_private.h"

#include "config.h"		// for debug macros


INT16 G_DECL FindTA1(BYTE *Atr, BYTE *TA1)
{
	if ( Atr[1] & 0x10 )
	{
		*TA1 = Atr[2];
		return G_OK;
	}
	else
	{
		return -1;
	}
}

WORD16 Fi[] = { 372, 372, 558, 744, 1116, 1488, 1860, 0xFFFF, 0xFFFF, 512, 768, 
				1024, 1536, 2048, 0xFFFF, 0xFFFF };

WORD16 Di[] = { 0xFFFF, 1, 2, 4, 8, 16, 32, 0xFFFF, 12, 20, 0xFFFF, 
				0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };

INT16 G_DECL GetAtrParams(BYTE *Atr, BYTE *Pro)
{
int i, j, l, k;
int offset=0;
int loop=1;
BYTE ta1, ta3, tb3, tc1, tc2, tc3;
BYTE F, D;

short int InterfaceBytes[5][6];
double etu;
double tmp;

	etu = 372.0/3.68;

	etu = etu/1000000.0;

	for ( i = 0; i < 5; ++i )
	 	for ( j = 0; j < 6; ++j )
			InterfaceBytes[i][j] = -1; 

	l = 1;
	while ( Atr[l] & 0x80 )
	{
		offset = 0;
		for(i=0, k = 0x10; k > 0;  k <<=1, i++)
		{
			if (Atr[l] & k)
			{
				offset++;
				InterfaceBytes[loop-1][i] = Atr[l+offset];
            		}
		}
         	l += offset;
		++loop;
	}

	if ( InterfaceBytes[0][0] == -1 )
	{
		InterfaceBytes[0][0] = 0x11;
	}

	ta1=InterfaceBytes[0][0];

	D(bug("TA1=0x%x\n", InterfaceBytes[0][0]));
	F = (ta1 & 0xf0) >> 4;
	F & 0x0f;

	D = (ta1 & 0x0f);

	((struct PROTOCOL_OPTIONS*)Pro)->Current_F = Fi[F];	
	((struct PROTOCOL_OPTIONS*)Pro)->Current_D = Di[D];	

	if ( InterfaceBytes[0][2] == -1 ) // TC1
	{	
		InterfaceBytes[0][2] = 0;
	}
	tc1 = InterfaceBytes[0][2];
	D(bug("TC1=0x%x\n", tc1));
	

	tmp = 12 * etu;
	tmp += tc1 * etu;

	D(bug("ETU = %f\n", etu));
	D(bug("Guard Time = %f\n", tmp));
	((struct PROTOCOL_OPTIONS*)Pro)->Current_N = tc1;

	if ( ((struct PROTOCOL_OPTIONS*)Pro)->Protocol_Type == 1 )
	{
		if ( InterfaceBytes[1][2] == -1 )
		{
			InterfaceBytes[1][2] = 10; //Default value for WI
		}
	tc2 = InterfaceBytes[1][2];
	D(bug("TC2=0x%x\n", tc2));
	((struct PROTOCOL_OPTIONS*)Pro)->Current_W = tc2;
	}

	
	if ( ((struct PROTOCOL_OPTIONS*)Pro)->Protocol_Type == 1 )
	{
		if ( InterfaceBytes[2][0] == -1 )
		{	
			InterfaceBytes[2][0] = 32; //IFSD Default
		}	
		ta3 = InterfaceBytes[2][0];
		D(bug("TA3=0x%x\n", ta3));
		((struct PROTOCOL_OPTIONS*)Pro)->Current_IFSC = ta3;
		((struct PROTOCOL_OPTIONS*)Pro)->Current_IFSD = 32;
		
		D(bug("TA3=0x%x\n", ta3));

		if ( InterfaceBytes[2][1] == -1 ) // TB3
		{
			InterfaceBytes[2][1] = 0x4d;	
		}
		tb3 = InterfaceBytes[2][1];
		((struct PROTOCOL_OPTIONS*)Pro)->Current_BWT = (tb3 & 0xf0) >> 4;
		((struct PROTOCOL_OPTIONS*)Pro)->Current_CWT = tb3 & 0x0f;
		if ( InterfaceBytes[2][2] == -1 ) // TC3
		{
			InterfaceBytes[2][2] = 0;	
		}
		tc3 = InterfaceBytes[2][2];
		((struct PROTOCOL_OPTIONS*)Pro)->Current_EBC = (tc3 & 0x01);
		D(bug("TB3=0x%x, TC3=0x%x\n", tb3, tc3));
	}

	return G_OK;
}


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
)
{
//------------------------------------------------
//Local variables:
// - response holds the called function responses.
// - i is a counter.
// - len holds the length of the command.
// - CFG holds the PTS configuration parameter.
// - PCK holds the PTS check parameter.
// - cmd holds ICC reset command whose format is
//      <12h> [<CFG>] [<PTS0>] [<PTS1>] [<PTS2>] [<PTS3>] [<PCK>]
// ------------------------------------------------------
INT16
   response;
WORD16
   i,
   len = 0;
WORD8
   CFG = 0,
   PCK,
   cmd[7];
WORD16
   rlen=HOR3GLL_BUFFER_SIZE;
WORD8
   rbuff[HOR3GLL_BUFFER_SIZE];

// ---------------------------------------------------------
// The command is sent to IFD with the PTS parameters.
// <=    G_Oros3Exchange status.
// ----------------------------------------------------------
   cmd[len++] = HOR3GLL_IFD_CMD_ICC_POWER_UP;

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
      response = G_Oros3Exchange(Timeout,len,cmd,RespLen,RespBuff);
      break;
   case IFD_NEGOTIATE_PTS_OPTIMAL:
      CFG |= 0x20;
      cmd[len++] = CFG;
      response = G_Oros3Exchange(Timeout,len,cmd,RespLen,RespBuff);
      break;
   case IFD_NEGOTIATE_PTS_MANUALLY:
      // first reset Icc without PTS management
      CFG |= 0x10;
      cmd[len++] = CFG;
      response = G_Oros3Exchange(Timeout,len,cmd,RespLen,RespBuff);
   if (response == G_OK)
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
        response = G_Oros3Exchange(Timeout,len,cmd,&rlen,rbuff);
   }
   break;
   case IFD_DEFAULT_MODE:
   default:
      if (CFG != 0x00)
      {
         cmd[len++] = CFG;
      }
      response = G_Oros3Exchange(Timeout,len,cmd,RespLen,RespBuff);
      break;
   }

   return (response);
}



INT16 G_DECL G_Oros3IccPowerDown
(
   const WORD32        Timeout,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
WORD8
   cmd[1] = { HOR3GLL_IFD_CMD_ICC_POWER_DOWN };

   return (G_Oros3Exchange(Timeout,1,cmd,RespLen,RespBuff));
}

INT16 G_DECL G_Oros3IsoInput
(
   const WORD32        Timeout,
   const WORD8         OrosCmd,
   const WORD8  G_FAR  Command[5],
   const WORD8  G_FAR  Data[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
WORD8
   cmd[HOR3GLL_BUFFER_SIZE];
INT16
   response;
WORD16
   resp_len = *RespLen;

   cmd[0] = OrosCmd;
   if (Command[4] <= (HGTGBP_MAX_DATA - 7))
   {
      _fmemcpy(cmd + 1, Command, 5);
      _fmemcpy(cmd + 6, Data, Command[4]);
      return
      (
         G_Oros3Exchange
         (
            Timeout,
            (WORD16)(6 + Command[4]),
            cmd,
            RespLen,
            RespBuff
         )
      );
   }
   else if (Command[4] <= HT0CASES_LIN_SHORT_MAX)
   {
      _fmemcpy(cmd + 1,"\xFF\xFF\xFF\xFF", 4);
      cmd[5] = (WORD8) (Command[4] - 248);
      _fmemcpy(cmd + 6, Data + 248, cmd[5]);
      response = G_Oros3Exchange
                                 (
                                 Timeout,
                                 (WORD16)(6 + cmd[5]),
                                 cmd,
                                 &resp_len,
                                 RespBuff
                                 );
      if ((response != G_OK) || (RespBuff[0] != 0x00) || (resp_len != 1))
      {
         if ((response == G_OK) && (RespBuff[0] == 0x1B))
         {
            RespBuff[0] = 0x12;
         }
         return(response);
      }
      _fmemcpy(cmd + 1, Command, 5);
      _fmemcpy(cmd + 6, Data,248);
      return
      (
         G_Oros3Exchange
         (
            Timeout,
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



INT16 G_DECL G_Oros3IccIsoInput
(
   const WORD32        Timeout,
   const WORD8  G_FAR  Command[5],
   const WORD8  G_FAR  Data[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
   return (G_Oros3IsoInput(
                           Timeout,
			   HOR3GLL_IFD_CMD_ICC_ISO_IN,
                           Command,
                           Data,
                           RespLen,
                           RespBuff
                           ));
}


INT16 G_DECL G_Oros3IsoOutput
(
   const WORD32        Timeout,
   const WORD8         OrosCmd,
   const WORD8  G_FAR  Command[5],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
WORD8
   cmd[6];
INT16
   response;
WORD16
   resp_len;
BYTE
   resp_buff[HOR3GLL_BUFFER_SIZE];

   cmd[0] = OrosCmd;

   if ((Command[4] <= (HGTGBP_MAX_DATA - 3)) && (Command[4] != 0)) 
   {
      _fmemcpy(cmd + 1, Command, 5);
      return (G_Oros3Exchange(Timeout,6,cmd,RespLen,RespBuff));
   }
   else if ((Command[4] > (HGTGBP_MAX_DATA - 3)) || (Command[4] == 0))
   {
      _fmemcpy(cmd + 1, Command, 5);
      response = G_Oros3Exchange(Timeout,6,cmd,RespLen,RespBuff);
      if ((response != G_OK) || (RespBuff[0] != 0x00))
      {
         return(response);
      }
      _fmemcpy(cmd + 1,"\xFF\xFF\xFF\xFF", 4);
      if (Command[4] == 0x00)
      {
         cmd[5] = (WORD8) (256 - ((WORD8) (*RespLen - 1)));
      }
      else
      {
         cmd[5] -= ((WORD8) (*RespLen - 1));
      }
      resp_len = HOR3GLL_BUFFER_SIZE;
      response = G_Oros3Exchange(Timeout,6,cmd,&resp_len,resp_buff);
      if ((response != G_OK) || (resp_buff[0] != 0x00))
      {
         _fmemcpy(RespBuff,resp_buff,resp_len);
         *RespLen = resp_len;
         return(response);
      }
      _fmemcpy(RespBuff + *RespLen,resp_buff + 1,resp_len - 1);
      *RespLen += (WORD16) (resp_len - 1);
      return(response);
   }
   else
   {
      return (GE_HI_CMD_LEN);
   }
}


INT16 G_DECL G_Oros3IccIsoOutput
(
   const WORD32        Timeout,
   const WORD8  G_FAR  Command[5],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
   return (G_Oros3IsoOutput(
                            Timeout,
			    HOR3GLL_IFD_CMD_ICC_ISO_OUT,
                            Command,
                            RespLen,
                            RespBuff
                            ));
}


INT16 G_DECL G_Oros3IsoT1
(
   const WORD32        Timeout,
   const WORD8         OrosCmd,
   const WORD16        ApduLen,
   const WORD8  G_FAR  ApduCommand[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
INT16
   response;
WORD8
   cmd[HOR3GLL_BUFFER_SIZE];
WORD16
   length_expected,
   resp_len;
BYTE
   resp_buff[HOR3GLL_BUFFER_SIZE];

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
   if (ApduLen <= (HGTGBP_MAX_DATA - 1))
   {
      _fmemcpy(cmd + 1, ApduCommand, ApduLen);
      response = G_Oros3Exchange(
                                 Timeout,
                                 (WORD16) (ApduLen + 1),
                                 cmd,
                                 RespLen,
                                 RespBuff
                                );
   }
   else
   {
      _fmemcpy(cmd + 1,"\xFF\xFF\xFF\xFF", 4);
      cmd[5] = (WORD8) (ApduLen - (HGTGBP_MAX_DATA - 1));
      _fmemcpy(cmd + 6, ApduCommand + (HGTGBP_MAX_DATA - 1), cmd[5]);
      resp_len = *RespLen;
      response = G_Oros3Exchange
                                 (
                                 Timeout,
                                 (WORD16)(6 + cmd[5]),
                                 cmd,
                                 RespLen,
                                 RespBuff
                                 );
      if ((response != G_OK) || (RespBuff[0] != 0x00) || (*RespLen != 1))
      {
         return(response);
      }
      _fmemcpy(cmd + 1, ApduCommand,(HGTGBP_MAX_DATA - 1));
      *RespLen = resp_len;
      response = G_Oros3Exchange(
                                 Timeout,
                                 (WORD16)HGTGBP_MAX_DATA,
                                 cmd,
                                 RespLen,
                                 RespBuff
                                );
   }
   if ((length_expected > 252) && (RespBuff[0] == 0x1B) && 
       (*RespLen == (HGTGBP_MAX_DATA - 1)))
   {
      _fmemcpy(cmd + 1,"\xFF\xFF\xFF\xFF", 4);
      cmd[5] = (WORD8) (length_expected - *RespLen + 1);
      cmd[5] += 2;
      resp_len = HOR3GLL_BUFFER_SIZE;
      response = G_Oros3Exchange(Timeout,6,cmd,&resp_len,resp_buff);
      if ((response != G_OK) || (resp_buff[0] != 0x00))
      {
         _fmemcpy(RespBuff,resp_buff,resp_len);
         *RespLen = resp_len;
         return(response);
      }
      _fmemcpy(RespBuff + *RespLen,resp_buff + 1,resp_len - 1);
      *RespLen += (WORD16) (resp_len - 1);
   }
   return(response);
}

INT16 G_DECL G_Oros3IccIsoT1
(
   const WORD32        Timeout,
   const WORD16        ApduLen,
   const WORD8  G_FAR  ApduCommand[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
   return (G_Oros3IsoT1(
                        Timeout,
			HOR3GLL_IFD_CMD_ICC_APDU,
			ApduLen,
                        ApduCommand,
                        RespLen,
                        RespBuff
                        ));
}







