/*******************************************************************************
*                   Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : ApduSpli.c
*
* Description : Module which splits APDU commands for being transported
*               according to T=0 protocole.
*
* Release     : 4.31.001
*
* Last Modif  : 13/10/97: V4.31.001  (GP)
*               18/03/97: V4.30.001  (TF)
*                 - Start of development.
*
********************************************************************************
*
* Warning     : The 7616-4 standard indicates that only the transmission system
*               can initiate GET_RESPONSE and ENVELOPE command. This two
*               commands cannot be transmitted by this module.
*
* Remark      : The true cases 2E and 4E.2 are not supported because the command
*               ENVELOPE is not supported by this module.
*
*******************************************************************************/

/*------------------------------------------------------------------------------
Name definition:
   _APDUSPLI_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _APDUSPLI_H
#define _APDUSPLI_H

/*------------------------------------------------------------------------------
C++ Section:
------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
Prototype section
------------------------------------------------------------------------------*/
INT16 G_DECL ApduSpliter
(
   const WORD32                Timeout,
   const G4_APDU_COMM G_FAR   *ApduComm,
         G4_APDU_RESP G_FAR   *ApduResp,
         INT16        (G_DECL *IsoIn)
         (
            const WORD32        Timeout,
            const WORD8  G_FAR  Command[5],
            const WORD8  G_FAR  Data[],
                  WORD16 G_FAR *RespLen,
                  BYTE   G_FAR  RespBuff[]
         ),
         INT16        (G_DECL *IsoOut)
         (
            const WORD32        Timeout,
            const WORD8  G_FAR  Command[5],
                  WORD16 G_FAR *RespLen,
                  BYTE   G_FAR  RespBuff[]
         )
);

#ifdef __cplusplus
}
#endif

#endif

