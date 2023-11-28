/*******************************************************************************
*                   Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : T0Cases
*
* Description : Module which gathers individual function for APDU transport in
*               T=0 protocole.
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
   _T0CASES_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _T0CASES_H
#define _T0CASES_H

/*------------------------------------------------------------------------------
Constant section
 - HT0CASES_MAX_SIZE define the maximum size for a T=0 response command.
 - HT0CASES_LIN_SHORT_MAX define the maximum Lin value for a short case.
 - HT0CASES_LEX_SHORT_MAX define the maximum Lexpected value for a short case.
------------------------------------------------------------------------------*/
#define HT0CASES_MAX_SIZE        259
#define HT0CASES_LIN_SHORT_MAX   255
#define HT0CASES_LEX_SHORT_MAX   256
/*------------------------------------------------------------------------------
 - Byte position in T=0 commands.
------------------------------------------------------------------------------*/
#define HT0CASES_INS            1
#define HT0CASES_P1             2
#define HT0CASES_P2             3
#define HT0CASES_P3             4
/*------------------------------------------------------------------------------
 - ISO command length.
------------------------------------------------------------------------------*/
#define CMD_SIZE                5lu
/*------------------------------------------------------------------------------
 - Flags for recognized cases.
------------------------------------------------------------------------------*/
#define HT0CASES_CASE_3S_2      0x67
#define HT0CASES_CASE_3S_3      0x6C
#define HT0CASES_CASE_3S_4      0x61
#define HT0CASES_CASE_3S_4_SIM  0x9F
#define HT0CASES_CASE_4S_2      0x90
#define HT0CASES_CASE_4S_3      0x61
#define HT0CASES_CASE_4S_3_SIM  0x9F
#define HT0CASES_CASE_3E_2A     0x67
#define HT0CASES_CASE_3E_2B     0x6C
#define HT0CASES_CASE_3E_2D     0x61
#define HT0CASES_CASE_3E_2D_SIM 0x9F
#define HT0CASES_CASE_4E_1      0x0100
#define HT0CASES_CASE_4E_1B     0x90
#define HT0CASES_CASE_4E_1C     0x61
#define HT0CASES_CASE_4E_1C_SIM 0x9F
#define HT0CASES_WRONG_LENGTH   0x6700
/*------------------------------------------------------------------------------
 - Get response INS byte.
------------------------------------------------------------------------------*/
#define HT0CASES_GET_RESPONSE   0xC0
/*------------------------------------------------------------------------------
Macro section
 - MIN returns the lower of two values.
 - VAL calculates the real length returned by an IsoOut command.
------------------------------------------------------------------------------*/
#define HT0CASES_MIN(a,b)  (((a) < (b)) ? (a) : (b))
#define HT0CASES_VAL(a)    (((a) == 0) ? 256 : (a))

/*------------------------------------------------------------------------------
C++ Section:
------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
Prototype section
------------------------------------------------------------------------------*/
INT16 G_DECL G_T0Case1
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
         )
);
INT16 G_DECL G_T0Case2S
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
         )
);
INT16 G_DECL G_T0Case3S
(
   const WORD32                Timeout,
   const G4_APDU_COMM G_FAR   *ApduComm,
         G4_APDU_RESP G_FAR   *ApduResp,
         INT16        (G_DECL *IsoOut)
         (
            const WORD32        Timeout,
            const WORD8  G_FAR  Command[5],
                  WORD16 G_FAR *RespLen,
                  BYTE   G_FAR  RespBuff[]
         )
);
INT16 G_DECL G_T0Case3E
(
   const WORD32                Timeout,
   const G4_APDU_COMM G_FAR   *ApduComm,
         G4_APDU_RESP G_FAR   *ApduResp,
         INT16        (G_DECL *IsoOut)
         (
            const WORD32        Timeout,
            const WORD8  G_FAR  Command[5],
                  WORD16 G_FAR *RespLen,
                  BYTE   G_FAR  RespBuff[]
         )
);
INT16 G_DECL G_T0Case4S
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
INT16 G_DECL G_T0Case4E
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

