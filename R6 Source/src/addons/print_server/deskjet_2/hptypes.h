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

#ifndef HPTYPES_H
#define HPTYPES_H
//===========================================================================
//
//  Filename     :  hptypes.h
//
//  Module       :  Open Source Imaging
//
//  Description  :  This file contains HP Types used by imaging modules.
//
//============================================================================

//=============================================================================
//  Header file dependencies
//=============================================================================

#define HPFAR

#define ENVPTR(typeName)                    HPFAR *typeName

typedef unsigned char   HPUInt8;

typedef const unsigned char   HPCUInt8;

typedef signed short    HPInt16, ENVPTR(HPInt16Ptr);

typedef const signed short  HPCInt16, ENVPTR(HPCInt16Ptr);

typedef unsigned short  HPUInt16;

typedef unsigned long   HPUInt32, ENVPTR(HPUInt32Ptr);

typedef int             HPBool;

typedef HPUInt8         HPByte, ENVPTR(HPBytePtr);

typedef HPCUInt8        HPCByte, ENVPTR(HPCBytePtr);

typedef HPUInt32    HPUIntPtrSize, ENVPTR(HPUIntPtrSizePtr);

#define kSpringsErrorType       HPInt16
#define kSpringsErrorTypePtr    HPInt16Ptr

#define HPTRUE    1
#define HPFALSE   0

#endif // HPTYPES_H
