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

#ifndef DEBUG_H
#define DEBUG_H

#define NO_DEBUG    0x00
#define	DBG_ASSERT  0x01                    // 0001
#define	DBG_LVL1    0x02	                // 0010
#define DBG_DFLT    (DBG_ASSERT | DBG_LVL1) // 0011
#define PUMP        0x04                    // 0100
#define HARNESS     0x08                    // 1000


extern void hp_assert(const char *, const char *, int);

#ifndef ASSERT  // System may already have ASSERT handling
    #if defined(DEBUG) && (DBG_MASK & DBG_ASSERT)
        #define ASSERT(_EX)  ((_EX) ? (void) 0 : hp_assert(#_EX,__FILE__, __LINE__))
    #else
        #define ASSERT(_EX) (void(0))
    #endif
#endif

#ifndef DBG1 // this allows for a different implementation elsewhere (ie platform.h)
    #if defined(DEBUG) && (DBG_MASK & DBG_LVL1)
        #define DBG1(_STUFF) printf(_STUFF)
    #else
        #define DBG1(_STUFF) (void(0))
    #endif
#endif

#if defined(DEBUG) && (DBG_MASK & PUMP)
    #define USAGE_LOG
    #define NULL_IO
#endif

#if defined(DEBUG) && (DBG_MASK & HARNESS)
    #define CAPTURE
#endif

#endif // DEBUG_H
