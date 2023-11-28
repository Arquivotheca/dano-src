#ifndef _T0CASES_H
#define _T0CASES_H


#include "./defines.h"
/*------------------------------------------------------------------------------
Constant section
 - T0CASES_MAX_SIZE define the maximum size for a T=0 response command.
 - T0CASES_LIN_SHORT_MAX define the maximum Lin value for a short case.
 - T0CASES_LEX_SHORT_MAX define the maximum Lexpected value for a short case.
------------------------------------------------------------------------------*/
#define T0CASES_MAX_SIZE        259
#define T0CASES_LIN_SHORT_MAX   255
#define T0CASES_LEX_SHORT_MAX   256
/*------------------------------------------------------------------------------
 - Byte position in T=0 commands.
------------------------------------------------------------------------------*/
#define T0CASES_INS            1
#define T0CASES_P1             2
#define T0CASES_P2             3
#define T0CASES_P3             4
/*------------------------------------------------------------------------------
 - ISO command length.
------------------------------------------------------------------------------*/
#define CMD_SIZE                5lu
/*------------------------------------------------------------------------------
 - Flags for recognized cases.
------------------------------------------------------------------------------*/
#define T0CASES_CASE_3S_2      0x67
#define T0CASES_CASE_3S_3      0x6C
#define T0CASES_CASE_3S_4      0x61
#define T0CASES_CASE_3S_4_SIM  0x9F
#define T0CASES_CASE_4S_2      0x90
#define T0CASES_CASE_4S_3      0x61
#define T0CASES_CASE_4S_3_SIM  0x9F
#define T0CASES_CASE_3E_2A     0x67
#define T0CASES_CASE_3E_2B     0x6C
#define T0CASES_CASE_3E_2D     0x61
#define T0CASES_CASE_3E_2D_SIM 0x9F
#define T0CASES_CASE_4E_1      0x0100
#define T0CASES_CASE_4E_1B     0x90
#define T0CASES_CASE_4E_1C     0x61
#define T0CASES_CASE_4E_1C_SIM 0x9F
#define T0CASES_WRONG_LENGTH   0x6700
/*------------------------------------------------------------------------------
 - Get response INS byte.
------------------------------------------------------------------------------*/
#define T0CASES_GET_RESPONSE   0xC0
/*------------------------------------------------------------------------------
Macro section
 - MIN returns the lower of two values.
 - VAL calculates the real length returned by an IsoOut command.
------------------------------------------------------------------------------*/
#define T0CASES_MIN(a,b)  (((a) < (b)) ? (a) : (b))
#define T0CASES_VAL(a)    (((a) == 0) ? 256 : (a))



#define T0CASES_LIN_SHORT_MAX 255 
/*------------------------------------------------------------------------------
C++ Section:
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
Prototype section
------------------------------------------------------------------------------*/
INT16 T0Case1
(
   const WORD32                handle,
   const APDU_COMMAND    *ApduComm,
         APDU_RESPONSE   *ApduResp,
         INT16        *IsoIn
         (
            const WORD32        handle,
            const WORD8    Command[5],
            const WORD8    Data[],
                  WORD16  *RespLen,
                  BYTE     RespBuff[]
         )
);
INT16 T0Case2S
(
   const WORD32                hanlde,
   const APDU_COMMAND    *ApduComm,
         APDU_RESPONSE   *ApduResp,
         INT16         *IsoIn
         (
            const WORD32        Timeout,
            const WORD8    Command[5],
            const WORD8    Data[],
                  WORD16 *RespLen,
                  BYTE     RespBuff[]
         )
);
INT16 T0Case3S
(
   const WORD32               handle,
   const APDU_COMMAND    *ApduComm,
         APDU_RESPONSE    *ApduResp,
         INT16         *IsoOut
         (
            const WORD32        Timeout,
            const WORD8    Command[5],
                  WORD16  *RespLen,
                  BYTE     RespBuff[]
         )
);
INT16 T0Case3E
(
   const WORD32                handle,
   const APDU_COMMAND  *ApduComm,
         APDU_RESPONSE   *ApduResp,
         INT16         *IsoOut
         (
            const WORD32        handle,
            const WORD8    Command[5],
                  WORD16  *RespLen,
                  BYTE     RespBuff[]
         )
);
INT16 T0Case4S
(
   const WORD32                handle,
   const APDU_COMMAND   *ApduComm,
         APDU_RESPONSE    *ApduResp,
         INT16         *IsoIn
         (
            const WORD32       handle,
            const WORD8    Command[5],
            const WORD8    Data[],
                  WORD16  *RespLen,
                  BYTE     RespBuff[]
         ),
         INT16         *IsoOut
         (
            const WORD32        Timeout,
            const WORD8    Command[5],
                  WORD16  *RespLen,
                  BYTE    RespBuff[]
         )
);
INT16 T0Case4E
(
   const WORD32                Timeout,
   const APDU_COMMAND    *ApduComm,
         APDU_RESPONSE    *ApduResp,
         INT16         *IsoIn
         (
            const WORD32        Timeout,
            const WORD8    Command[5],
            const WORD8    Data[],
                  WORD16  *RespLen,
                  BYTE     RespBuff[]
         ),
         INT16         *IsoOut
         (
            const WORD32        Timeout,
            const WORD8    Command[5],
                  WORD16  *RespLen,
                  BYTE     RespBuff[]
         )
);


#endif

