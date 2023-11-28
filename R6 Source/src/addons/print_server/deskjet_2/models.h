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

#define DEVID_MODEL_400 "HP DeskJet 4"
#define DEVID_MODEL_540 "DESKJET 540"
#define DEVID_MODEL_600 "DESKJET 600"
#define DEVID_MODEL_61X "DESKJET 61"
#define DEVID_MODEL_64X "DESKJET 64"
#define DEVID_MODEL_69X "DESKJET 69"
#define DEVID_MODEL_66X "DESKJET 66"
#define DEVID_MODEL_67X "DESKJET 67"
#define DEVID_MODEL_68X "DESKJET 68"
#define DEVID_MODEL_6XX "DESKJET 6"
#define DEVID_MODEL_81X "DESKJET 81"
#define DEVID_MODEL_83X "DESKJET 83"
#define DEVID_MODEL_84X "DESKJET 84"
#define DEVID_MODEL_88X "DESKJET 88"
#define DEVID_MODEL_895 "DESKJET 895"
#define DEVID_MODEL_93X "DESKJET 93"
#define DEVID_MODEL_95X "DESKJET 95"
#define DEVID_MODEL_97X "DESKJET 97"
#define DEVID_MODEL_63X "DESKJET 63"
#define DEVID_MODEL_99X "DESKJET 99"
#define DEVID_MODEL_E20 "e-printer e20"
#define DEVID_MODEL_APOLLO_P22XX "APOLLO P-22"
#define DEVID_MODEL_APOLLO_P21XX "P-2000U"  // yes, the P-2100 report itself as a P-2000


typedef enum { UNSUPPORTED=-1, 
                    DJ400, 
                    DJ540, 
                    DJ600, 
                    DJ6xx, 
                    DJ6xxPhoto, 
                    DJ8xx, 
                    DJ9xx,
                    DJ9xxVIP,
                    DJ630,
                    AP2100 } PRINTER_TYPE;

#define MAX_PRINTER_TYPE 9  // base-0
#define MAX_ID_STRING 23    // base-1

typedef enum { HELP_UNSUPPORTED=-1, 
                    dj400help = 100, 
                    dj540help = 101, 
                    dj600help = 102, 
                    dj630help = 103, 
                    dj6xxhelp = 104, 
                    dj660help = 105, 
                    dj670help = 106, 
                    dj680help = 107,
                    eprinterhelp = 108,
                    dj6xxPhotohelp = 109,
                    dj610help = 110, 
                    dj640help = 111, 
                    dj690help = 112,
                    apollo2200help = 113,
                    dj8xxhelp = 114, 
                    dj810help = 115, 
                    dj830help = 116, 
                    dj840help = 117, 
                    dj880help = 118, 
                    dj895help = 119, 
                    dj9xxhelp = 120,
                    dj930help = 121, 
                    dj950help = 122, 
                    dj970help = 123, 
                    dj9xxVIPhelp = 124,
                    dj990help = 125,
                    apollo2100help = 126 } HELP_TYPE;

