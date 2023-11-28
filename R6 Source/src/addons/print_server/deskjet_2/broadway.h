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

#ifndef BROADWAY_H
#define BROADWAY_H

class Broadway : public Printer
{
public:
	Broadway(SystemServices* pSS, BOOL proto=FALSE);

	virtual ~Broadway();	

	BOOL UseGUIMode(unsigned int PrintModeIndex);
    Header* SelectHeader(PrintContext* pc);
    Compressor* CreateCompressor(unsigned int RasterSize);
 	DISPLAY_STATUS ParseError(BYTE status_reg);
    DRIVER_ERROR VerifyPenInfo();

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	Font* RealizeFont(const int index,const BYTE bSize,
						const TEXTCOLOR eColor=BLACK_TEXT,
						const BOOL bBold=FALSE,const BOOL bItalic=FALSE,
						const BOOL bUnderline=FALSE);
#endif
 
    virtual DRIVER_ERROR ParsePenInfo(PEN_TYPE& ePen, BOOL QueryPrinter=TRUE);

    void SetHelpType(const char* model)
    {
        if      (!strncmp(model, DEVID_MODEL_93X, strlen(DEVID_MODEL_93X))) help=dj930help;
        else if (!strncmp(model, DEVID_MODEL_95X, strlen(DEVID_MODEL_95X))) help=dj950help;
        else if (!strncmp(model, DEVID_MODEL_97X, strlen(DEVID_MODEL_97X))) help=dj970help;
        else    help = dj9xxhelp;
    }

    PAPER_SIZE MandatoryPaperSize();

protected:
    virtual BOOL PhotoTrayInstalled(BOOL QueryPrinter); 
};

class BroadwayMode1 : public PrintMode
{
public:
    BroadwayMode1();

};

class BroadwayMode2 : public PrintMode
{
public:
    BroadwayMode2();

};


#ifdef PROTO
extern PEN_TYPE ProtoPenType;
class ProtoBroadway : public Broadway
{
public:
	ProtoBroadway(ProtoServices* pSS);
	
    DRIVER_ERROR ParsePenInfo(PEN_TYPE& ePen){ ePen=ProtoPenType; return NO_ERROR; }
    BOOL PhotoTrayInstalled(); 
};


#endif

#endif
