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

#ifndef PRINTER_H
#define PRINTER_H

#ifdef PROTO
#include "../Harness/proto.h"
#endif
////////////////////////////////////////////////////////////////////////////
// PRINTER classes

#define MAX_PRINTMODES 4
#define MAX_PRINTER_FONTS 4
#define NUM_DJ400_FONTS 3  // ultimately obsolete these values since
#define NUM_DJ6XX_FONTS 4  // number of fonts is flexible

#define MAX_COMPATIBLE_PENS 3

#define CANCEL_BUTTON_CHECK_THRESHOLD 150000  // will poll printer's cancel button state every
                                              // time this quantity of bytes is written out

class Compressor;

class PrintMode
{
friend class Job;
friend class Header;
friend class PrintContext;
friend class Translator;
friend class GraphicsTranslator;
friend class Imager;
public:
    PrintMode(unsigned long *map1,unsigned long *map2=(unsigned long*)NULL);

// The resolutions can be different for different planes
	unsigned int ResolutionX[MAXCOLORPLANES];
	unsigned int ResolutionY[MAXCOLORPLANES];

    unsigned int ColorDepth[MAXCOLORPLANES];

    MediaType medium;

    BOOL bFontCapable;

    char ModeName[12];

    PipeConfigTable Config;
    unsigned int dyeCount;		// number of inks in the pen(s)

protected:
    Quality theQuality;

    ColorMap cmap;	    

    unsigned int BaseResX,BaseResY;
    BOOL MixedRes;

    unsigned char* BlackFEDTable;
    unsigned char* ColorFEDTable;


    PEN_TYPE CompatiblePens[MAX_COMPATIBLE_PENS];
};

class GrayMode : public PrintMode
{
public:
    GrayMode(unsigned long *map);
};

class Printer
{
public:

	Printer(SystemServices* pSys, int numfonts, 
            BOOL proto = FALSE);

	virtual ~Printer();
	
	DRIVER_ERROR constructor_error;

    virtual unsigned int GetModeCount(void) { return ModeCount; }
    virtual PrintMode* GetMode(unsigned int index);


#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	ReferenceFont* EnumFont(int& iCurrIdx);

	virtual Font* RealizeFont(const int index, const BYTE bSize,
						const TEXTCOLOR eColor = BLACK_TEXT,
						const BOOL bBold = FALSE,
						const BOOL bItalic = FALSE,
						const BOOL bUnderline = FALSE);
#endif

	virtual DRIVER_ERROR Send(const BYTE* pWriteBuff);
	virtual DRIVER_ERROR Send(const BYTE* pWriteBuff, DWORD dwWriteLen);

	virtual DRIVER_ERROR Flush(int FlushSize = MAX_RASTERSIZE); // flush printer input buffer

	// select Header component of Translator
	virtual Header* SelectHeader(PrintContext* pc) = 0;


	virtual Compressor* CreateCompressor(unsigned int RasterSize);

	virtual DISPLAY_STATUS ParseError(BYTE status_reg);

    virtual DRIVER_ERROR CleanPen();


	// DEVID stuff
	IO_MODE IOMode;
    
    int iNumFonts;			// size of fontarray
	BOOL bCheckForCancelButton;
	unsigned long ulBytesSentSinceCancelCheck;

    HELP_TYPE help;

    virtual void SetHelpType(const char* model)=0;

	virtual BOOL UseGUIMode(unsigned int PrintModeIndex) { return FALSE; }
    virtual BOOL UseCMYK(unsigned int PrintModeIndex) { return TRUE;}

    virtual PAPER_SIZE MandatoryPaperSize() 
            { return UNSUPPORTED_SIZE; }    //code for "nothing mandatory"

    // tells currently installed pen type
    virtual DRIVER_ERROR ParsePenInfo(PEN_TYPE& ePen, BOOL QueryPrinter=TRUE) { ePen=NO_PEN; return NO_ERROR; }


protected:
	SystemServices* pSS;
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	ReferenceFont* fontarray[MAX_PRINTER_FONTS+1];
#endif

	int InSlowPollMode;
    int iTotal_SLOW_POLL_Count; // total SLOW_POLLs (multiple of iMax_SLOW_POLL_Count)
    int iMax_SLOW_POLL_Count;   // time-out value at which point we do error check
	BOOL ErrorTerminationState;

	virtual DRIVER_ERROR VerifyPenInfo() = 0;
	virtual BOOL TopCoverOpen(BYTE status_reg);
    DRIVER_ERROR SetPenInfo(char*& pStr, BOOL QueryPrinter);

    PrintMode* pMode[MAX_PRINTMODES];
    unsigned int ModeCount;
 
	// BeOS stuff
public:
	bool Error(bool retry);
};


#endif
