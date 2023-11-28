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

#ifndef DJ895_H
#define DJ895_H

class DeskJet895 : public Printer
{
public:
	DeskJet895(SystemServices* pSS,int numfonts=0, BOOL proto=FALSE);

    PrintMode* GetMode(int index);

	virtual Header* SelectHeader(PrintContext* pc); 
	virtual DRIVER_ERROR VerifyPenInfo();
	virtual DRIVER_ERROR ParsePenInfo(PEN_TYPE& ePen, BOOL QueryPrinter=TRUE);
 	virtual DISPLAY_STATUS ParseError(BYTE status_reg);

	Compressor* CreateCompressor(unsigned int RasterSize);

	virtual BOOL UseGUIMode(unsigned int PrintModeIndex); 
    DRIVER_ERROR CleanPen();

    void SetHelpType(const char* model)
    {
        if      (!strncmp(model, DEVID_MODEL_81X, strlen(DEVID_MODEL_81X))) help=dj810help;
        else if	(!strncmp(model, DEVID_MODEL_83X, strlen(DEVID_MODEL_83X))) help=dj830help;
        else if	(!strncmp(model, DEVID_MODEL_84X, strlen(DEVID_MODEL_84X))) help=dj840help;
        else if	(!strncmp(model, DEVID_MODEL_88X, strlen(DEVID_MODEL_88X))) help=dj880help;
        else if	(!strncmp(model, DEVID_MODEL_895, strlen(DEVID_MODEL_895))) help=dj895help;
        else    help = dj8xxhelp;
    }

protected:
  

};

class VeniceMode1 : public PrintMode
{
public:
    VeniceMode1();

};
class VeniceMode2 : public PrintMode
{
public:
    VeniceMode2();

};
class VeniceMode3 : public PrintMode
{
public:
    VeniceMode3();

};


#ifdef PROTO
extern PEN_TYPE ProtoPenType;
class ProtoDeskJet895 : public DeskJet895
{
public:
	ProtoDeskJet895(ProtoServices* pSS);

    DRIVER_ERROR ParsePenInfo(PEN_TYPE& ePen) { ePen=ProtoPenType; return NO_ERROR; }
	
};


#endif

#endif
