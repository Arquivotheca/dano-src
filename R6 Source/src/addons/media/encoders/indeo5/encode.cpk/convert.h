/************************************************************************
*                                                                       *
*               INTEL CORPORATION PROPRIETARY INFORMATION               *
*                                                                       *
*    This listing is supplied under the terms of a license agreement    *
*      with INTEL Corporation and may not be copied nor disclosed       *
*        except in accordance with the terms of that agreement.         *
*                                                                       *
*************************************************************************
*                                                                       *
*               Copyright (C) 1994-1997 Intel Corp.                       *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

/*
 *  Tabs set to 4
 *
 *  convert.h
 *
 *  DESCRIPTION:
 *  Color converter module include file.
 */

#ifndef __CONVERT_H__
#define __CONVERT_H__ 

PIA_RETURN_STATUS ConvertToYVU9(PTR_CCIN_INST pInst,
               				    PTR_COLORIN_INPUT_INFO pInput,
                      			PTR_COLORIN_OUTPUT_INFO pOutput);

PIA_RETURN_STATUS YUY2ToYVU9(PTR_CCIN_INST pInst,
							PTR_COLORIN_INPUT_INFO pInput,
							PTR_COLORIN_OUTPUT_INFO pOutput);

PIA_RETURN_STATUS YVU12ToYVU9(PTR_CCIN_INST pInst,
							PTR_COLORIN_INPUT_INFO pInput,
							PTR_COLORIN_OUTPUT_INFO pOutput);


#ifdef SIMULATOR
PIA_RETURN_STATUS ConvertToYVU12(PTR_CCIN_INST pInst,
               				    PTR_COLORIN_INPUT_INFO pInput,
                      			PTR_COLORIN_OUTPUT_INFO pOutput);
#endif /* SIMULATOR */
#endif
