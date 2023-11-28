/*******************************************************************************
*                 Copyright (c) 1991-1997 Gemplus Development
*
* Name        : Or3COMM.H
*
* Description : Public interface for OROS 3.x IFD communication module.
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
   _OR3COMM_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _OR3COMM_H
#define _OR3COMM_H

/*------------------------------------------------------------------------------
Constant section:
 - HOR3COMM_MAX_TRY communication try is launched before the channel is declared
   broken. Today 3.
------------------------------------------------------------------------------*/
#define HOR3COMM_MAX_TRY         3
/*------------------------------------------------------------------------------
 - HOR3COMM_CHAR_TIMEOUT is the timeout at character level: today 1000 ms.
 - HOR3COMM_NACK_TIME is the time out used when a nack command is sent to IFD:
      today 1000 ms are used.
 - HOR3COMM_CHAR_TIME is the time for IFD to forget any previously received
      byte: today 300 ms.
------------------------------------------------------------------------------*/
#define HOR3COMM_CHAR_TIMEOUT 1000
#define HOR3COMM_NACK_TIME    1000
#define HOR3COMM_CHAR_TIME     300

/*------------------------------------------------------------------------------
C++ Section:
------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
Prototype section:
------------------------------------------------------------------------------*/
INT16 G_DECL G_Oros3SendCmd
(
   const WORD16       CmdLen,
   const WORD8  G_FAR Cmd[],
	const BOOL         Resynch
);
INT16 G_DECL G_Oros3ReadResp
(
   const WORD32        Timeout,
         WORD16 G_FAR *RspLen,
         WORD8  G_FAR  Rsp[]
);

#ifdef __cplusplus
}
#endif

#endif
