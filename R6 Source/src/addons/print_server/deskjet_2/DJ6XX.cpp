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

#if defined(_DJ540) || defined(_DJ600) || defined(_DJ6xx)

#ifdef PROTO
#include "../include/Header.h"
#include "../include/IO_defs.h"
#include "../Printer/DJ6XX.h"
#else
#include "Header.h"
#include "IO_defs.h"
#include "DJ6XX.h"
#endif


//
// ** DeskJet6XX:Printer CLASS **
//

DeskJet6XX::DeskJet6XX(SystemServices* pSS,
                       int numfonts, BOOL proto)
	   : Printer(pSS, numfonts, proto)
{
	DBG1("DeskJet6XX constructor called, colormap pending\n");
}



Header6XX::Header6XX(Printer* p,PrintContext* pc)
	: Header(p,pc)
{ }

DRIVER_ERROR Header6XX::Send()
{	DRIVER_ERROR err;

	StartSend();

	err = thePrinter->Send((const BYTE*)BlackExtractOff, 
						sizeof(BlackExtractOff)); // just pertains to 2-pen
	ERRCHECK;

	err = ConfigureRasterData();
	ERRCHECK;						
					
	err=Graphics();		// start raster graphics and set compression mode
	
return err;
}

#endif
