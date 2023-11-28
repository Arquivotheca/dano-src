/******************************************************************************
*
*                             Copyright (c) 1998
*                E-mu Systems Proprietary All rights Reserved
*
******************************************************************************/

/******************************************************************************
*
* @doc INTERNAL
* @module souttrns.h | 
* This file contains the class definitions required for using the synth
* parameter translation code with Streaming Audio output.  
*
* @iex 
* Revision History:
*
* Person              Date          Reason
* ------------------  ------------  --------------------------------------
* Michael Preston     Oct 20, 1998  Initial development.
*
******************************************************************************/

#ifndef __SOUTTRNS_H
#define __SOUTTRNS_H

#include "datatype.h"

class SAOutputSampleData
{
    public:

    DWORD dwSamplesPerSec;
};

#endif
