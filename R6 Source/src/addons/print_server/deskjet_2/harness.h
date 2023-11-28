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

// Job tokens
#define tokJob              0										
#define tokdJob             1
#define tokNewPage          2
#define tokSendRasters      3
#define tokTextOut          4

// Font tokens
#define UNUSED1             5
#define tokdFont            6

// Context tokens
#define tokPrintContext     7
#define tokdPrintContext    8
#define tokSetPixelsPerRow  9
#define tokRealizeFont      10
#define tokSelectDevice     11
#define tokSelectPrintMode  12
#define tokSetPaperSize     13
#define tokUseBlackOnly     14
#define tokUseColor         15
#define tokSetRes			16

// 
#define UNUSED2             17
#define UNUSED3             18
#define UNUSED4             19

// others
#define tokCharPtr          20
#define tokRLEstream        21
#define tokRawStream        22
#define tokStream           23
#define UNUSED5             24

#define LAST_TOKEN          25

////////////////////////////////////////////////
// error codes for errors encountered in replay
#define tokExec     1
#define tokDupJob   2
#define tokJobErr   3
#define tokPCErr    4
#define tokEOF      5
