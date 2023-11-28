/*********************************************************************/
/*
	Contains:	This header defines the Private interface to the Profile
				Processor.

				Created by lsh, October 25, 1993

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1995 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   SPROFPRV.H  $
		$Logfile:   O:\pc_src\dll\stdprof\sprofprv.h_v  $
		$Revision:   2.0  $
		$Date:   21 Mar 1994 14:59:56  $
		$Author:   lsh  $

	SCCS Revision:
	 @(#)sprofprv.h	1.58 12/22/97

	To Do:
*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993-1995                 ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/
#ifndef SPROFPRIV_H
#define SPROFPRIV_H

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#if defined (KPWIN32)
#pragma pack (push, SpvLevel, 4)
#endif

#include "kcms_sys.h"
#include "kcmptdef.h"

/* Composition types */
typedef KpInt32_t SpConnectType_t;

#define SpConnect_Type_Mask		0x0f
#define	SpConnect_Std		0x0
#define	SpConnect_pf_8		0x1
#define	SpConnect_pf_16		0x2
#define	SpConnect_pf		0x3

#define SpConnect_Method_Mask	0xf0
#define SpConnect_Default		0x00
#define SpConnect_Chain			0x10
#define SpConnect_Chain_Std		SpConnect_Chain | SpConnect_Std
#define SpConnect_Chain_pf_8	SpConnect_Chain | SpConnect_pf_8
#define SpConnect_Chain_pf_16	SpConnect_Chain | SpConnect_pf_16
#define SpConnect_Chain_pf		SpConnect_Chain | SpConnect_pf

#define SpConnect_Combine		0x20
#define SpConnect_Combine_Std	SpConnect_Combine | SpConnect_Std
#define SpConnect_Combine_pf_8	SpConnect_Combine | SpConnect_pf_8
#define SpConnect_Combine_pf_16	SpConnect_Combine | SpConnect_pf_16
#define SpConnect_Combine_pf	SpConnect_Combine | SpConnect_pf

#define SpConnect_Mode_Mask		0xf00
#define	SpConnect_Largest		0x100

/*	Private color spaces to be used within the profile processor.  These	
	color spaces suppliment the color spaces defined in sprofile.h			*/
#define SpSpaceRCS 					SpSigMake('R', 'C', 'S', ' ')


/* Valid Paradigm Types for lab<->uvl conversions */
#define	SpParadigmRel	0	/* relative paradigm type */
#define	SpParadigmAbs	1	/* absolute paradigm type */

typedef KpInt32_t SpParadigm_t;


/* Defined for backward compatibility */
#define SpXformFromMFut	SpXformFromBuffer

#if defined (KPMAC)
void SpGetCPInstance(
				KpInt32_t *theCurCP);
				
void SpSetCPInstance(
				KpInt32_t theCurCP);
#endif

SpStatus_t SPAPI SpXformFromPTRefNumEx (
				SpConnectType_t	ConnectType,
				SpParadigm_t	ParadigmType,
				PTRefNum_t		FAR *RefNum,
				SpXform_t		FAR *Xform);

/*	For importing Raw PTs, no RCS to LAB conversion	*/
SpStatus_t SPAPI SpXformFromPTRefNumNC (
				PTRefNum_t		FAR *RefNum,
				SpXform_t		FAR *Xform);


/* Do Combine of PTs only - no chaining */
SpStatus_t SPAPI SpXformFromPTRefNumCombine (
				SpConnectType_t	ConnectType,
				SpParadigm_t	ParadigmType,
				PTRefNum_t		FAR *RefNum,
				SpXform_t		FAR *Xform);

SpStatus_t SPAPI SpXformFromPTRefNum (
				PTRefNum_t		FAR *RefNum,
				SpXform_t		FAR *Xform);

SpStatus_t SPAPI SpXformGetRefNum (
				SpXform_t		Xform,
				PTRefNum_t		FAR *RefNum);

SpStatus_t SPAPI SpXformCreateFromDataEx (
				SpConnectType_t	ConnectType,
				KpInt32_t		Size,
				KpLargeBuffer_t	Data,
				SpXform_t		FAR *Xform);

SpStatus_t SPAPI SpXformCreateMatTagsFromPT (
				SpProfile_t		Profile,
				PTRefNum_t		RefNum);

SpStatus_t SPAPI SpXformCreateMatTagsFromXform (
				SpProfile_t		Profile,
				SpXform_t		Xform);

SpStatus_t SPAPI SpXformCreateMatTags (
				SpProfile_t		Profile,
				KpInt32_t		DataSize,
				KpLargeBuffer_t	Data);

SpStatus_t SPAPI SpXformLCSCreate (
				KpF15d16XYZ_t	FAR *rXYZ,
				KpF15d16XYZ_t	FAR *gXYZ,
				KpF15d16XYZ_t	FAR *bXYZ,
				KpResponse_t	FAR *rTRC,
				KpResponse_t	FAR *gTRC,
				KpResponse_t	FAR *bTRC,
				KpUInt32_t	gridsize, 
				KpBool_t	invert, 
				SpXform_t	FAR *Xform);

SpStatus_t SPAPI SpXformLCSAdaptCreate (
				KpF15d16XYZ_t	FAR *rXYZ,
				KpF15d16XYZ_t	FAR *gXYZ,
				KpF15d16XYZ_t	FAR *bXYZ,
				KpResponse_t	FAR *rTRC,
				KpResponse_t	FAR *gTRC,
				KpResponse_t	FAR *bTRC,
				KpUInt32_t		gridsize, 
				KpBool_t		invert, 
				KpBool_t		adapt, 
				KpBool_t		lagrange, 
				SpXform_t		FAR *Xform);

SpStatus_t SPAPI SpXformCreate (
				KpF15d16XYZ_t	FAR *rXYZ,
				KpF15d16XYZ_t	FAR *gXYZ,
				KpF15d16XYZ_t	FAR *bXYZ,
				KpResponse_t	FAR *rTRC,
				KpResponse_t	FAR *gTRC,
				KpResponse_t	FAR *bTRC,
				KpUInt32_t	gridsize,
				KpBool_t	invert,
				KpBool_t	adapt,
				KpBool_t	lagrange,
				SpXform_t	FAR *Xform);

SpStatus_t SPAPI SpXformGrayCreate (
				KpResponse_t	FAR *gTRC,
				KpUInt32_t	gridsize,
				KpBool_t	invert,
				SpXform_t	FAR *Xform);
 
SpStatus_t SPAPI SpXformCreateFromMatTags (
				KpF15d16XYZ_t	FAR *rXYZ,
				KpF15d16XYZ_t	FAR *gXYZ,
				KpF15d16XYZ_t	FAR *bXYZ,
				KpResponse_t	FAR *rTRC,
				KpResponse_t	FAR *gTRC,
				KpResponse_t	FAR *bTRC,
				KpUInt32_t	gridsize,
				KpBool_t	invert,
				KpBool_t	Linear,
				SpXform_t	FAR *Xform);

SpStatus_t SPAPI SpXformGetParms (
				SpXform_t	Xform,
				SpTransRender_t	*WhichRender,
				SpTransType_t	*WhichTransform,
				KpF15d16XYZ_t	*HdrWhite,
				KpF15d16XYZ_t	*MedWhite,
				KpUInt32_t	*ChainIn,
				KpUInt32_t	*ChainOut);

SpStatus_t SPAPI SpXformSetParms (
				SpXform_t	Xform,
				SpTransRender_t	WhichRender,
				SpTransType_t	WhichTransform,
				KpF15d16XYZ_t	HdrWhite,
				KpF15d16XYZ_t	MedWhite,
				KpUInt32_t	ChainIn,
				KpUInt32_t	ChainOut);

SpStatus_t SPAPI SpXformToBlobGetDataSize (
				SpXform_t		Xform,
				KpInt32_t		FAR *BufferSize);

SpStatus_t SPAPI SpXformToBlobGetData (
				SpXform_t		Xform,
				KpInt32_t		BufferSize,
				SpHugeBuffer_t	Buffer);

SpStatus_t SPAPI SpXformFromBlob (
				KpInt32_t		BufferSize,
				SpHugeBuffer_t	Buffer,
				SpXform_t		FAR *Xform);

SpStatus_t SPAPI SpConnectSequenceEx (
				SpConnectType_t	ConnectType,
				KpInt32_t		XformCnt,
				SpXform_t		FAR *XformsSequence,
				SpXform_t		FAR *Result,
				KpInt32_t		FAR *FailingXform,
				SpProgress_t	ProgressFunc,
				void			FAR *Data);

SpStatus_t SPAPI SpConnectSequence (
				KpInt32_t		XformCnt,
				SpXform_t		FAR *XformsSequence,
				SpXform_t		FAR *Result,
				KpInt32_t		FAR *FailingXform,
				SpProgress_t	ProgressFunc,
				void			FAR *Data);

/*******************************************************************/
/* Raw Data Access (See Documentation before using these Function) */
/*******************************************************************/

SpStatus_t SPAPI SpRawTagDataGetSize (
				SpProfile_t	Profile,
				SpTagId_t	TagId,
				KpUInt32_t	FAR *TagDataSize);

SpStatus_t SPAPI SpRawTagDataGet (
				SpProfile_t	Profile,
				SpTagId_t	TagId,
				KpUInt32_t	FAR *TagDataSize,
				void		KPHUGE * FAR *TagData);

void SPAPI SpRawTagDataFree (
				SpProfile_t	Profile,
				SpTagId_t	TagId,
				void		KPHUGE *TagData);

SpStatus_t SPAPI SpRawTagDataSet (
				SpProfile_t	Profile,
				SpTagId_t	TagId,
				KpUInt32_t	TagDataSize,
				void		KPHUGE *TagData);

SpStatus_t SPAPI SpRawHeaderGet (
				SpProfile_t	Profile,
				KpUInt32_t	BufferSize,
				void		KPHUGE *Buffer);

/********************************************************************/
/* Misc																*/
/********************************************************************/

SpStatus_t SPAPI SpCvrtIOFileData (
				SpIOFileChar_t	*SpProps,
				ioFileChar		*KcmProps);

SpStatus_t SPAPI SpCvrtIOFileProps(
				SpIOFileChar_t	*SpProps,
				SpFileProps_t	*KcmProps);

SpStatus_t SPAPI SpCvrtSpFileData (
				SpFileProps_t	*SpProps,
				ioFileChar  	*KcmProps);

SpStatus_t SPAPI SpCvrtSpFileProps(
				SpFileProps_t	*SpProps,
				KpFileProps_t	*KcmProps);

SpStatus_t SPAPI SpXformFromLut(SpLut_t         Lut,
                                SpTransRender_t WhichRender,
                                SpTransType_t   WhichTransform,
                                SpSig_t         SpaceIn,
                                SpSig_t         SpaceOut,
				KpF15d16XYZ_t	HdrWhite,
				KpF15d16XYZ_t	MedWhite,
                                KpUInt32_t      ChainIn,
                                KpUInt32_t      ChainOut,
                                SpXform_t       FAR *Xform);

SpStatus_t SPAPI SpXformToLut  (SpXform_t       Xform,
                                SpLut_t         *Lut,
                                SpTransRender_t *WhichRender,
                                SpTransType_t   *WhichTransform,
                                SpSig_t         *SpaceIn,
                                SpSig_t         *SpaceOut,
				KpF15d16XYZ_t	*HdrWhite,
				KpF15d16XYZ_t	*MedWhite,
                                KpUInt32_t      *ChainIn,
                                KpUInt32_t      *ChainOut);


SpStatus_t SPAPI SpTagGetString(SpTagValue_t *TagValue,
				  KpInt32_p	BufSize,
			  	KpChar_p	Buffer);

SpStatus_t SPAPI SpProfileGetHeaderString(SpSearchType_t	hdrItem,
				SpHeader_t		*hdr,
				KpInt32_p		BufSize,
				KpChar_p		Buffer);

SpStatus_t SpStringToTextDesc(
			KpChar_p	pString,
			SpTextDesc_t	*pTextDesc);
 
KpBool_t SPAPI SpProfileValidHandle(SpProfile_t SpProf);

/************************************************/
/* Functions we want to phase out of public use */
/************************************************/
SpStatus_t SPAPI SpProfileSaveToFileEx (
				SpProfile_t Profile,
				char		FAR *Name,
				KpBool_t		ShareTags);

SpStatus_t SPAPI SpProfileSaveToFile (
				SpProfile_t Profile,
				char		FAR *Name);

SpStatus_t SPAPI SpProfileLoadFromFile (
				SpCallerId_t	CallerId,
				char			FAR *FileName,
				SpProfile_t		FAR *Profile);

SpStatus_t SPAPI SpProfileGetFileName (
				SpProfile_t		Profile,
				size_t			BufferSize,
				char			FAR *Buffer);

SpStatus_t SPAPI SpProfileSetFileName (
				SpProfile_t		Profile,
				char			FAR *FileName);
SpStatus_t SPAPI SpProfileGetSharedTags(
				SpProfile_t		Profile,
				SpTagId_t		TagId,
				SpTagId_t		*Matched_TagIds,
				KpInt32_t		*num_matched_tags);

SpStatus_t SPAPI SpProfileGetTagCount (
				SpProfile_t Profile,
				KpInt32_t		*tagCount);



#if defined (KPWIN32)
#pragma pack (pop, SpvLevel)
#endif


#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#endif	/* SPROFPRIV_H */

