/*******************************************************************************
*                 Copyright (c) 1991-1997 Gemplus Development
*
* Name        : GTGBP.H
*
* Description : This module holds the function needed for communication on a
*               serial line according to Gemplus Block Protocol.
*
* Release     : 4.31.001
*
* Last Modif  : 13/10/97: V4.31.001  (GP)
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/

/*------------------------------------------------------------------------------
Constant section:
   HGTGBPP maximal size for a message:
      <NAD> <PCB> <Len> <max is 255 bytes> <EDC>
------------------------------------------------------------------------------*/
#define HGTGBP_MAX_DATA        255
#define HGTGBP_MAX_BUFFER_SIZE HGTGBP_MAX_DATA + 4
/*------------------------------------------------------------------------------
Prototype section:
------------------------------------------------------------------------------*/
INT16 G_DECL G_GBPOpen
(
   const WORD16 HostAdd,
   const WORD16 IFDAdd,
   const INT16  PortCom   
);
INT16 G_DECL G_GBPClose
(
);
INT16 G_DECL G_GBPBuildIBlock
(
   const WORD16        CmdLen, 
   const WORD8  G_FAR  Cmd[], 
         WORD16 G_FAR *MsgLen,
         WORD8  G_FAR  Msg[]
);
INT16 G_DECL G_GBPBuildRBlock
(
         WORD16 G_FAR *MsgLen,
         WORD8  G_FAR  Msg[]
);
INT16 G_DECL G_GBPBuildSBlock
(
         WORD16 G_FAR *MsgLen,
         WORD8  G_FAR  Msg[]
);
INT16 G_DECL G_GBPDecodeMessage
(
   const WORD16        MsgLen, 
   const WORD8  G_FAR  Msg[], 
         WORD16 G_FAR *RspLen,
         WORD8  G_FAR  Rsp[]
);
INT16 G_DECL G_GBPChannelToPortComm
(
);
