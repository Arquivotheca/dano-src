
/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1994. All rights Reserved.
*                             
*******************************************************************************
*/

/*****************************************************************************
*  @(#)emuerrs.h	1.1 12:06:28 3/15/95 12:06:35
*
* Filename: emuerrs.h
*
* Description: Header file for Standard E-mu 8200 Error Codes                 **
*              For use with E-mu 8200 Sound Engine Software
*
* Revision: 0.03, 7/14/94
*******************************************************************************
*/

#ifndef __EMUERRS_H
#define __EMUERRS_H

/*************
** Includes
*************/

#include "datatype.h"

/************
** Defines
************/

#define SUCCESS  0

/***********************
** Enumeration Tables
***********************/



enum RIFFERRORTAG  
{
  RIFF_IDERROR = SUCCESS + 1,
  RIFF_ERROR,
  RIFF_FINDERROR,
  RIFF_OPENFILEERROR,  
  RIFF_READFILEERROR,
  RIFF_WRITEFILEERROR
};



enum SFERRORTAG  
{
  SF_ERROR = RIFF_WRITEFILEERROR + 1,  
  SF_BUFFERERROR,                      
  SF_MEMORYERROR,                      
  SF_INVALIDBANK,                      
  SF_BANKLINKERROR,                    
  SF_BANKLOADERROR,                    
  SF_WRONGWAVETABLE,                   
  SF_WRONGVERSION,                     
  SF_PRESETNOTFOUND                    
};



enum QUEUEERRORTAG    
{
  QUEUE_OVERFLOW = SF_PRESETNOTFOUND + 1,
  QUEUE_NODATA
};



enum CHIPERRORSTAG   
{
  CHIP_INSUFFICIENT_DRAM = QUEUE_NODATA + 1
};



enum AUDMAERRORSTAG  
{
  DMA_INVALID_STREAM = CHIP_INSUFFICIENT_DRAM + 1,
  DMA_READ_TIMEOUT,
  DMA_WRITE_TIMEOUT,
  DMA_INVALID_DATA
};




enum EMUCHIPERRORSTAG
{
  EMUCHIP_TIMEOUT = DMA_INVALID_DATA + 1,
  EMUCHIP_READ_ONLY_REGISTER,
  EMUCHIP_UNKNOWN_FUNCTION,
  EMUCHIP_INIT_FAILURE,
  EMUCHIP_NOT_INIT
};



enum EMUHLAPIERRORTAG
{
  HLD_NO_SF_LOADED =  EMUCHIP_NOT_INIT + 1,  
  HLD_SF_ALREADY_LOADED,
  HLD_SC_NOT_INIT,
  HLD_SC_ALREADY_INIT,
  HLD_NOT_IMPLEMENTED,
  HLD_PARAM_OUT_OF_RANGE,
  HLD_NO_MORE_SOUNDFONT_ROOM, 
  HLD_INVALID_SFID,
  HLD_NOT_DA_STREAM,
  HLD_BANK_ALREADY_USED,  
  HLD_AUDIO_ACTIVE        
};



enum EMUFXERRORTAG  
{ 
  EMUFX_NOT_VALID = HLD_AUDIO_ACTIVE + 1,
  EMUFX_NOT_IMPLEMENTED
};

enum EMUDAERRORTAG  
{
  EMUDA_NOT_SUPPORTED = EMUFX_NOT_IMPLEMENTED + 1,
  EMUDA_INVALID_ID,
  EMUDA_DISABLED
};

enum VIENNAERRORTAG
{
   VIENNA_NOT_SUPPORTED = EMUDA_DISABLED + 1,
   VIENNA_OUT_OF_MEM,
   VIENNA_INVALID_SAMPLE,
   VIENNA_OUT_OF_DRAM,
   VIENNA_OUT_OF_GCHANNEL,
   VIENNA_INVALID_PRESET
};

/* 
   IMPORTANT! If you add another error AFTER EMUDA_DISABLED, update
   this macro!
*/

#define EMUERR_MAX_ERROR VIENNA_INVALID_PRESET + 1

/**************************
** External Declarations
***************************/

/*
Create your own custom error messages to be indexed by the above
enumeration table.

I.E.

 char sfEr1[80];
 strcpy(sfEr1, "E-mu 8000 Initialization Error");
 emuErrors[EMUCHIP_TIMEOUT] = sfEr1;
 ...
 suEnviron et;
 ...
 stat = EmuInit(&et);
 if (stat != SUCCESS) {
   printf("%s\n", emuErrors[stat]);
   return(-1);
   }

*/

BEGINEMUCTYPE
extern CHAR* emuErrors[];
ENDEMUCTYPE

#endif /* __EMUERRS_H */

