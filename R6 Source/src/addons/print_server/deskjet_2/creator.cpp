//
//  Copyright (c) 2000, Hewlett-Packard Co.
//  All rights reserved.
//  
//  This software is licensed solely for use with HP products.  Redistribution
//  and use with HP products in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//  
//  -	Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//  -	Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//  -	Neither the name of Hewlett-Packard nor the names of its contributors
//      may be used to endorse or promote products derived from this software
//      without specific prior written permission.
//  -	Redistributors making defect corrections to source code grant to
//      Hewlett-Packard the right to use and redistribute such defect
//      corrections.
//  
//  This software contains technology licensed from third parties; use with
//  non-HP products is at your own risk and may require a royalty.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
//  CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
//  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//  DISCLAIMED. IN NO EVENT SHALL HEWLETT-PACKARD OR ITS CONTRIBUTORS
//  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
//  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
//  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
//  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//  DAMAGE.
//

//===========================================================================
//
//  Filename     :  creator.cpp
//
//  Module       :  Open Source Imaging
//
//  Description  :  Creates Imager_Open
//
//============================================================================

#include "Header.h"
#include "imaging.h"
#include "resources.h"

// routine to return generic Imager to Job
Imager* Create_Imager( SystemServices* pSys, PrintMode* pPM,
                unsigned int iInputWidth,            
                unsigned int iNumRows[],        			 
                unsigned int HiResFactor                
			  )
{ return new Imager_Open(pSys,pPM,iInputWidth,iNumRows,HiResFactor); }


BYTE* GetHT3x3_4()
{ return (BYTE*)HT300x3004level_open; }

BYTE* GetHT6x6_4()
{ return (BYTE*)HT600x6004level895_open; }

BYTE* GetHTBinary()
{ return (BYTE*)HTBinary_open; }

BYTE* GetHT6x6_4_970()
{ return (BYTE*)HT600x6004level970_open; }

// function to identify version of system
BOOL Proprietary()
{ return FALSE; }
