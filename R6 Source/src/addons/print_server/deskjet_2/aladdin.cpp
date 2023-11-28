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

#ifdef _DJ9xxVIP

#ifdef PROTO
#include "../include/Header.h"
#include "../include/IO_defs.h"
#include "../Printer/aladdin.h"
#else
#include "Header.h"
#include "IO_defs.h"
#include "aladdin.h"
#endif

#define OUR_PJL_JOBNAME "_PJL_pjl_PJL_pjl_" // this can be anything we want it to be
#define DRIVERWARE_JOBNAME  "NEWDRIVERWARE" // don't change this - it is defined in firmware!

extern unsigned long ulMapVOLTAIRE_CCM_K[ 9 * 9 * 9 ];

const char GrayscaleSeq[]= {ESC, '*', 'o', '5', 'W', 0x0B, 0x01, 0x00, 0x00, 0x02};

void AsciiHexToBinary(BYTE* dest, char* src, int count)
{
    int i;
    BYTE bitPattern;
    for (i=0; i<count ; i++)
    {
        switch (src[i])
        {
            case '0':
                bitPattern = 0x00;
                break;
            case '1':
                bitPattern = 0x01;
                break;
            case '2':
                bitPattern = 0x02;
                break;
            case '3':
                bitPattern = 0x03;
                break;
            case '4':
                bitPattern = 0x04;
                break;
            case '5':
                bitPattern = 0x05;
                break;
            case '6':
                bitPattern = 0x06;
                break;
            case '7':
                bitPattern = 0x07;
                break;
            case '8':
                bitPattern = 0x08;
                break;
            case '9':
                bitPattern = 0x09;
                break;
            case 'a':
            case 'A':
                bitPattern = 0x0a;
                break;
            case 'b':
            case 'B':
                bitPattern = 0x0b;
                break;
            case 'c':
            case 'C':
                bitPattern = 0x0c;
                break;
            case 'd':
            case 'D':
                bitPattern = 0x0d;
                break;
            case 'e':
            case 'E':
                bitPattern = 0x0e;
                break;
            case 'f':
            case 'F':
                bitPattern = 0x0f;
                break;
        }
        
        if ((i%2) == 0)
        {
            dest[i/2] = bitPattern << 4;
        }
        else
        {
            dest[i/2] |= bitPattern;
        }
    }
}


Aladdin::Aladdin(SystemServices* pSS, BOOL proto)
	: Printer(pSS, NUM_DJ6XX_FONTS, proto), PCL3acceptsDriverware(TRUE)
{	
    if (IOMode.bDevID)
    {
        constructor_error = VerifyPenInfo();
        CERRCHECK;
    }

    PCL3acceptsDriverware = IsPCL3DriverwareAvailable();
	
    pMode[GRAYMODE_INDEX] = new GrayModeAladdin(ulMapVOLTAIRE_CCM_K,PCL3acceptsDriverware);
    pMode[DEFAULTMODE_INDEX] = new AladdinMode();

    ModeCount = 2;

}

GrayModeAladdin::GrayModeAladdin(unsigned long *map, BOOL PCL3OK)
: GrayMode(map)
{
    Config.bErnie=TRUE;   
    Config.bColorImage=FALSE;

    if (!PCL3OK)
        bFontCapable = FALSE; 
}

AladdinMode::AladdinMode()
: PrintMode(NULL)
{
    
    BaseResX=BaseResY=ResolutionX[0]=ResolutionY[0]= 600;

    medium = mediaAuto;     // enable media-detect

    Config.bErnie=TRUE;
    Config.bColorImage=FALSE;
}



BOOL Aladdin::UseGUIMode(unsigned int PrintModeIndex)
{
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
    return ((PrintModeIndex==GRAYMODE_INDEX) && (!PCL3acceptsDriverware));
#else
    return TRUE;
#endif
}

Mode10::Mode10(SystemServices* pSys, unsigned int PlaneSize)
	: Compressor(pSys, PlaneSize+3, TRUE)
{ 
    if (constructor_error != NO_ERROR)  // if error in base constructor
        return;

    // In the worst case, compression expands data by 50%
    compressBuf = (BYTE*)pSS->AllocMem(PlaneSize + PlaneSize/2); 
	if (compressBuf == NULL)
		constructor_error=ALLOCMEM_ERROR;
    memset(SeedRow,0xFF,PlaneSize);
}

Mode10::~Mode10()
{
}


unsigned int Mode10::get3Pixel(unsigned char *pixAddress, int pixelOffset)
{
	pixAddress += 3*pixelOffset;
	
	unsigned int toReturn = *((unsigned int*)pixAddress); // load toReturn with XRGB
	toReturn &= kWhite; // Strip off unwanted X. EGW stripped lsb blue.
	
	return toReturn;
}


void Mode10::put3Pixel(unsigned char *pixAddress, int pixelOffset, unsigned int pixel)
{
	unsigned int *uLongPtr = (unsigned int*)(pixAddress + pixelOffset +  2 * pixelOffset); // REVISIT just mult by 3.
	unsigned int temp = *uLongPtr & 0xFF000000;

	temp |= (pixel & kWhite);

	*uLongPtr = temp;
}



void Mode10::putPixel(unsigned char *pixAddress, int pixelOffset, int pixelSizeInBytes, unsigned int pixel)
{
	if (3 == pixelSizeInBytes)
	{
		put3Pixel(pixAddress, pixelOffset, pixel);
	}
	else
	{
		ASSERT(4 == pixelSizeInBytes);
		put4Pixel(pixAddress, pixelOffset, pixel);
	}
}


unsigned short Mode10::ShortDelta(int lastPixel, int lastUpperPixel)
{
	int dr,dg,db;
	int result;

	dr = GetRed(lastPixel) - GetRed(lastUpperPixel);
	dg = GetGreen(lastPixel) - GetGreen(lastUpperPixel);
	db = GetBlue(lastPixel) - GetBlue(lastUpperPixel);

	if ((dr <= 15) && (dr >= -16) && (dg <= 15) && (dg >= -16) && (db <= 30) && (db >= -32))
	{	// Note db is divided by 2 to double it's range from -16..15 to -32..30
		result = ((dr << 10) & 0x007C00) | (((dg << 5) & 0x0003E0) | ((db >> 1) & 0x01F) | 0x8000);   // set upper bit to signify short delta
	}
	else
		result = 0;  // upper bit is zero to signify delta won't work

	return result;
}




BOOL Mode10::Process(BYTE* input, unsigned int size)

/****************************************************************************
Initially written by Elden Wood
August 1998

Similar to mode 9, though tailored for pixel data.
For more information see the Bert Compression Format document.

This function compresses a single row per call.
****************************************************************************/

{
   if (input==NULL)    // flushing pipeline
    {
        compressedsize=0;
        iRastersReady=0;
        return FALSE;
    }

    unsigned int originalsize=size;

    unsigned char *seedRowPtr = (unsigned char*)SeedRow;
 
    unsigned char *compressedDataPtr = compressBuf;
    unsigned char *curRowPtr =  (unsigned char*)input;
    int pixelSizeInBytes = 3;
    int rowWidthInBytes = size;

	ASSERT(curRowPtr);
	ASSERT(seedRowPtr);
	ASSERT(compressedDataPtr);
	ASSERT((pixelSizeInBytes == 3) || (pixelSizeInBytes == 4));
	ASSERT(rowWidthInBytes >= pixelSizeInBytes);
	ASSERT((rowWidthInBytes % pixelSizeInBytes) == 0);
	
	unsigned char *compressedDataStart = compressedDataPtr;
	unsigned int lastPixel = (rowWidthInBytes / pixelSizeInBytes) - 1;

	// Setup sentinal value to replace last pixel of curRow. Simplifies future end condition checking.
	unsigned int realLastPixel = getPixel(curRowPtr, lastPixel, pixelSizeInBytes);
	while ((getPixel(curRowPtr, lastPixel-1, pixelSizeInBytes) == 
            getPixel(curRowPtr, lastPixel, pixelSizeInBytes)) || 
			(getPixel(seedRowPtr, lastPixel, pixelSizeInBytes) == 
             getPixel(curRowPtr, lastPixel, pixelSizeInBytes))
          )
	{
		putPixel(curRowPtr, lastPixel, pixelSizeInBytes, getPixel(curRowPtr, lastPixel, pixelSizeInBytes) + 0x100); // add one to green.
	}

	unsigned int curPixel = 0;
	unsigned int seedRowPixelCopyCount;
	unsigned int cachedColor = kWhite;
	
	do // all pixels in row
	{
		unsigned char CMDByte;
		int replacementCount;
			
		// Find seedRowPixelCopyCount for upcoming copy
		seedRowPixelCopyCount = curPixel;
		while (getPixel(seedRowPtr, curPixel, pixelSizeInBytes) == getPixel(curRowPtr, curPixel, pixelSizeInBytes))		
				curPixel++;
	
		seedRowPixelCopyCount = curPixel - seedRowPixelCopyCount;
		ASSERT (curPixel <= lastPixel);

		int pixelSource;
		
		if (curPixel == lastPixel) // On last pixel of row. RLE could also leave us on the last pixel of the row from the previous iteration.
		{
			putPixel(curRowPtr, lastPixel, pixelSizeInBytes, realLastPixel);
			
			if (getPixel(seedRowPtr, curPixel, pixelSizeInBytes) == realLastPixel)
			    goto mode10rtn;
			else // code last pix as a literal
			{

				CMDByte = eLiteral;
				pixelSource = eeNewPixel;
				replacementCount = 1;
				curPixel++;
			}
		}
		else // prior to last pixel of row
		{
			ASSERT(curPixel < lastPixel);
			
			replacementCount = curPixel;
			unsigned int RLERun = getPixel(curRowPtr, curPixel, pixelSizeInBytes);
			
			curPixel++; // Adjust for next pixel.
			while (RLERun == getPixel(curRowPtr, curPixel, pixelSizeInBytes)) // RLE
			{
				curPixel++;
			}
			curPixel--; // snap back to current.
			replacementCount = curPixel - replacementCount;
			ASSERT(replacementCount >= 0);

			if (replacementCount > 0) // Adjust for total occurance and move to next pixel to do.
			{
				curPixel++;
				replacementCount++;

				if (cachedColor == RLERun) pixelSource = eeCachedColor;
				else if (getPixel(seedRowPtr, curPixel-replacementCount+1, pixelSizeInBytes) == RLERun) pixelSource = eeNEPixel;
				else if ((curPixel-replacementCount > 0) &&  (getPixel(curRowPtr, curPixel-replacementCount-1, pixelSizeInBytes) == RLERun)) pixelSource = eeWPixel;
				else
				{
					pixelSource = eeNewPixel;
					cachedColor = RLERun;
				}

				CMDByte = eRLE; // Set default for later.

			}

			if (curPixel == lastPixel)
			{
				ASSERT(replacementCount > 0); // Already found some RLE pixels
				
				if (realLastPixel == RLERun) // Add to current RLE. Otherwise it'll be part of the literal from the seedrow section above on the next iteration.
				{
					putPixel(curRowPtr, lastPixel, pixelSizeInBytes, realLastPixel);
					replacementCount++;
					curPixel++;
				}
			}
	
			if (0 == replacementCount) // no RLE so it's a literal by default.
			{
				ASSERT(getPixel(curRowPtr, curPixel, pixelSizeInBytes) != getPixel(curRowPtr, curPixel+1, pixelSizeInBytes)); // not RLE
				ASSERT(getPixel(curRowPtr, curPixel, pixelSizeInBytes) != getPixel(seedRowPtr, curPixel, pixelSizeInBytes)); // not seedrow copy

				CMDByte = eLiteral;
				
				unsigned int tempPixel = getPixel(curRowPtr, curPixel, pixelSizeInBytes);
				if (cachedColor == tempPixel)
				{	
					pixelSource = eeCachedColor;

				}
				else if (getPixel(seedRowPtr, curPixel+1, pixelSizeInBytes) == tempPixel)
				{
					pixelSource = eeNEPixel;

				}
				else if ((curPixel > 0) &&  (getPixel(curRowPtr, curPixel-1, pixelSizeInBytes) == tempPixel))
				{
					pixelSource = eeWPixel;

				}
				else
				{

					pixelSource = eeNewPixel;
					cachedColor = tempPixel;
				}
	
				replacementCount = curPixel;
				do
				{
					if (++curPixel == lastPixel)
					{
						putPixel(curRowPtr, lastPixel, pixelSizeInBytes, realLastPixel);
						curPixel++;
						break;
					}
				} while ((getPixel(curRowPtr, curPixel, pixelSizeInBytes) != getPixel(curRowPtr, curPixel+1, pixelSizeInBytes)) &&
						(getPixel(curRowPtr, curPixel, pixelSizeInBytes) != getPixel(seedRowPtr, curPixel, pixelSizeInBytes)));
				
				replacementCount = curPixel - replacementCount;
				
				ASSERT(replacementCount > 0);
			}
		}

		ASSERT(seedRowPixelCopyCount >= 0);

		// Write out compressed data next.
		if (eLiteral == CMDByte)
		{
			ASSERT(replacementCount >= 1);

			replacementCount -= 1; // normalize it

			CMDByte |= pixelSource; // Could put this directly into CMDByte above.
			CMDByte |= MIN(3, seedRowPixelCopyCount) << 3;
			CMDByte |= MIN(7, replacementCount);

			*compressedDataPtr++ = CMDByte;
			
			if (seedRowPixelCopyCount >= 3) outputVLIBytesConsecutively(seedRowPixelCopyCount - 3, compressedDataPtr);
			
			replacementCount += 1; // denormalize it
		
			int	totalReplacementCount = replacementCount;
			int upwardPixelCount = 1;

			if (eeNewPixel != pixelSource)
			{
				replacementCount -= 1; // Do not encode 1st pixel of run since it comes from an alternate location.
				upwardPixelCount = 2;	
			}
							
			for ( ; upwardPixelCount <= totalReplacementCount; upwardPixelCount++)
			{
				ASSERT(totalReplacementCount >= upwardPixelCount);

				unsigned short compressedPixel = ShortDelta(	getPixel(curRowPtr, curPixel-replacementCount, pixelSizeInBytes),
																getPixel(seedRowPtr, curPixel-replacementCount, pixelSizeInBytes));
				if (compressedPixel)
				{
					*compressedDataPtr++ = compressedPixel >> 8;
					*compressedDataPtr++ = (unsigned char)compressedPixel;

				}
				else
				{
					int uncompressedPixel = getPixel(curRowPtr, curPixel-replacementCount, pixelSizeInBytes);
					
					uncompressedPixel >>= 1; // Lose the lsb of blue and zero out the msb of the 3 bytes.
					
					*compressedDataPtr++ = uncompressedPixel >> 16;
					*compressedDataPtr++ = uncompressedPixel >> 8;
					*compressedDataPtr++ = uncompressedPixel;
					 
				}
				
				if (((upwardPixelCount-8) % 255) == 0)  // See if it's time to spill a single VLI byte.
				{
					*compressedDataPtr++ = MIN(255, totalReplacementCount - upwardPixelCount);
				}

				replacementCount--;
			}
		}
		else // RLE 
		{
			ASSERT(eRLE == CMDByte);
			ASSERT(replacementCount >= 2);
			
			replacementCount -= 2; // normalize it
			
			CMDByte |= pixelSource; // Could put this directly into CMDByte above.
			CMDByte |= MIN(3, seedRowPixelCopyCount) << 3;
			CMDByte |= MIN(7, replacementCount);

			*compressedDataPtr++ = CMDByte;
			
			if (seedRowPixelCopyCount >= 3) outputVLIBytesConsecutively(seedRowPixelCopyCount - 3, compressedDataPtr);
			
			replacementCount += 2; // denormalize it
			
			if (eeNewPixel == pixelSource)
			{
				unsigned short compressedPixel = ShortDelta(getPixel(curRowPtr, curPixel - replacementCount, pixelSizeInBytes),
															getPixel(seedRowPtr, curPixel - replacementCount, pixelSizeInBytes));
				if (compressedPixel)
				{
					*compressedDataPtr++ = compressedPixel >> 8;
					*compressedDataPtr++ = (unsigned char)compressedPixel;
				}
				else
				{
					int uncompressedPixel = getPixel(curRowPtr, curPixel - replacementCount, pixelSizeInBytes);
					
					uncompressedPixel >>= 1;
					
					*compressedDataPtr++ = uncompressedPixel >> 16;
					*compressedDataPtr++ = uncompressedPixel >> 8;
					*compressedDataPtr++ = uncompressedPixel;
				}
			}
			
			if (replacementCount-2 >= 7) outputVLIBytesConsecutively(replacementCount - (7+2), compressedDataPtr);
		}
	} while (curPixel <= lastPixel);
mode10rtn:
	size = compressedDataPtr - compressedDataStart; // return # of compressed bytes.
    compressedsize = size;
    memcpy(SeedRow, input, originalsize);
    iRastersReady=1;
    return TRUE;
}


Compressor* Aladdin::CreateCompressor(unsigned int RasterSize)
{
	return new Mode10(pSS,RasterSize);
}


Header* Aladdin::SelectHeader(PrintContext* pc)
{ 
	return new HeaderAladdin(this,pc); 
}

HeaderAladdin::HeaderAladdin(Printer* p,PrintContext* pc)
	: Header(p,pc)
{ }

DRIVER_ERROR HeaderAladdin::ConfigureRasterData()
// This is the more sophisticated way of setting color and resolution info.
//
// NOTE: Will need to be overridden for DJ5xx.
{ 
char buff[20];      // 12 + 3 for crdstart + 3 for ##W
char *out=buff;


	// begin the CRD command
	memcpy(out,crdStart,sizeof(crdStart) );
	out += sizeof(crdStart);

	// now set up the "#W" part, where #= number of data bytes in the command
	// #= 12
    //      format,ID,components(2bytes),resolutions (4bytes),
    //      compresssion method, orientation, bits/component,planes/component
	char sBuffer[3];
	int num = 12;
	sprintf( sBuffer, "%d", num );
	*out++ = sBuffer[0];    // "1"
	*out++ = sBuffer[1];    // "2"
	*out++ = 'W';

#define VIPcrdFormat 6
#define VIPsRGBID 7

	*out++ = VIPcrdFormat;
	*out++ = VIPsRGBID;

    // 2-byte component count field
    // number of components for sRGB is 1 by defintiion
    *out++ = 0;     // leading byte
    *out++ = 1;     // 1 "component"

	*out++ = ResolutionX[0]/256;
	*out++ = ResolutionX[0]%256;
	*out++ = ResolutionY[0]/256;
	*out++ = ResolutionY[0]%256;

    // compression method 
    *out++ = 10;    // mode 10 compression

    // orientation
    *out++ = 1;     // pixel major code

    // bits per component
    *out++ = 32;    // 8 bits each for s,R,G,B

    // planes per component
    *out++ = 1;     // sRGB = one component, one "plane"
 

    return thePrinter->Send((const BYTE*) buff, out-buff);
 
}

DRIVER_ERROR HeaderAladdin::ConfigureImageData()
{
    DRIVER_ERROR err = thePrinter->Send((const BYTE*)cidStart, sizeof(cidStart));
    ERRCHECK;
    char sizer[3];
    sprintf(sizer,"%dW",6);
    err = thePrinter->Send((const BYTE*)sizer,2);
    ERRCHECK;

    BYTE colorspace = 2;    // RGB
    BYTE coding = 3;
    BYTE bitsindex = 3;     // ??
    BYTE bitsprimary = 8;
    BYTE CID[6];
    CID[0]=colorspace; CID[1]=coding;
    CID[2]=bitsindex; CID[3]=CID[4]=CID[5]=bitsprimary;

    return thePrinter->Send((const BYTE*) CID, 6);
}


DRIVER_ERROR HeaderAladdin::Send()
/// ASSUMES COMPRESSION ALWAYS ON -- required by aladdin
{	DRIVER_ERROR err;

	StartSend();		
    
    err = ConfigureImageData();
    ERRCHECK;

    err = ConfigureRasterData();
	ERRCHECK;

    if (thePrintMode->dyeCount == 1)    // grayscale
    {
        err=thePrinter->Send((const BYTE*)GrayscaleSeq, sizeof(GrayscaleSeq) );
        ERRCHECK;
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // unit-of-measure command --- seems to be need in aladdin PCL3 mode
    char uom[32];	// don't play with fire!!
    sprintf(uom,"%c%c%c%d%c",ESC,'&','u',thePrintMode->ResolutionX[K],'D');
    err=thePrinter->Send((const BYTE*)uom, 7 );
    ERRCHECK;
    
    // another command that helps PCLviewer
    sprintf(uom,"%c%c%c%d%c",ESC,'*','r',thePrintContext->OutputPixelsPerRow()*2,'S');
    err=thePrinter->Send((const BYTE*)uom, 8 );
    ERRCHECK;
    ////////////////////////////////////////////////////////////////////////////////////


    // no need for compression command, it's in the CRD

    err=thePrinter->Send((const BYTE*)grafStart, sizeof(grafStart) );
    ERRCHECK;

return err;
}


DRIVER_ERROR HeaderAladdin::StartSend()
{
	DRIVER_ERROR err;

	err = thePrinter->Send((const BYTE*)Reset,sizeof(Reset));
	ERRCHECK;

	err = thePrinter->Send((const BYTE*)UEL,sizeof(UEL));
	ERRCHECK;

	err = thePrinter->Send((const BYTE*)EnterLanguage,sizeof(EnterLanguage));
	ERRCHECK;

    if (!thePrinter->UseGUIMode(thePrintContext->CurrentPrintMode()))	
		err = thePrinter->Send((const BYTE*)PCL3,sizeof(PCL3));
	else 
        err = thePrinter->Send((const BYTE*)PCLGUI,sizeof(PCLGUI));
	ERRCHECK;

	
	err = thePrinter->Send((const BYTE*)&LF,1);
	ERRCHECK;		
		
	err=Modes();			// Set media source, type, size and quality modes.
	ERRCHECK;	
				

    if (!thePrinter->UseGUIMode(thePrintContext->CurrentPrintMode()))	
		err=Margins();			// set margins
    else    // special GUI mode top margin set
    {
        // we take the hard unprintable top to be .04 (see define in Header.cpp)
        // so here start out at 1/3"-.04" = 88 if dpi=300
        unsigned int ResMultiple = (thePrintContext->InputResolution() / 300);
        CAPy = 88 / ResMultiple;
        SendCAPy( CAPy );
    }

   
	return err;
}

DRIVER_ERROR Aladdin::VerifyPenInfo()
{
	DRIVER_ERROR err=NO_ERROR;

	if(IOMode.bDevID == FALSE) 
        return err;

    PEN_TYPE ePen;

	err = ParsePenInfo(ePen);

    if(err == UNSUPPORTED_PEN) // probably Power Off - pens couldn't be read
    {
        DBG1("DJ9xxVIP::Need to do a POWER ON to get penIDs\n");

		// have to delay for Broadway or the POWER ON will be ignored
        if (pSS->BusyWait((DWORD)2000) == JOB_CANCELED)
            return JOB_CANCELED;

		DWORD length=sizeof(Venice_Power_On);
		err = pSS->ToDevice(Venice_Power_On,&length);
        ERRCHECK;

		err = pSS->FlushIO();
        ERRCHECK;

        // give the printer some time to power up
        if (pSS->BusyWait((DWORD)1000) == JOB_CANCELED)
            return JOB_CANCELED;

	    err = ParsePenInfo(ePen);
    }

    ERRCHECK;

	// check for the normal case
	if (ePen == BOTH_PENS)
		return NO_ERROR;

	while ( ePen != BOTH_PENS 	)
	{

		switch (ePen)
		{
			case BLACK_PEN:
				// black pen installed, need to install color pen
				pSS->DisplayPrinterStatus(DISPLAY_NO_COLOR_PEN);
				break;
			case COLOR_PEN:
				// color pen installed, need to install black pen
				pSS->DisplayPrinterStatus(DISPLAY_NO_BLACK_PEN);
				break;
			case NO_PEN:
				// neither pen installed
			default:
				pSS->DisplayPrinterStatus(DISPLAY_NO_PENS);
				break;
		}

		if (pSS->BusyWait(500) == JOB_CANCELED)
			return JOB_CANCELED;

		err =  ParsePenInfo(ePen);
		ERRCHECK;
	}

	pSS->DisplayPrinterStatus(DISPLAY_PRINTING);

	return NO_ERROR;
}

DRIVER_ERROR Aladdin::ParsePenInfo(PEN_TYPE& ePen, BOOL QueryPrinter)
{
	char* str;
    int num_pens = 0;
	PEN_TYPE temp_pen1 = NO_PEN;
    BYTE penInfoBits[2];
    DRIVER_ERROR err = SetPenInfo(str, QueryPrinter);
    ERRCHECK;

    // the first byte indicates how many pens are supported
    if ((str[0] >= '0') && (str[0] <= '9'))
    {
        num_pens = str[0] - '0';        
    }
    else if ((str[0] >= 'A') && (str[0] <= 'F'))
    {
        num_pens = 10 + (str[0] - 'A');                
    }
    else
    {
        return BAD_DEVICE_ID;
    }
    
    if (num_pens < 2)
    {
        return UNSUPPORTED_PEN;
    }
    
	// parse pen1 (should be black)
    AsciiHexToBinary(penInfoBits, str+1, 4);
    penInfoBits[1] &= 0xf8; // mask off ink level trigger bits
    if ((penInfoBits[0] == 0xc1) && (penInfoBits[1] == 0x10))
    {   // Hobbes
        temp_pen1 = BLACK_PEN;
    }
    else if (penInfoBits[0] == 0xc0)
    {   // missing pen
        temp_pen1 = NO_PEN;
    }
    else
    {
        return UNSUPPORTED_PEN;
    }

	// now check pen2 (should be color)
    AsciiHexToBinary(penInfoBits, str+9, 4);
    penInfoBits[1] &= 0xf8; // mask off ink level trigger bits
    if ((penInfoBits[0] == 0xc2) && (penInfoBits[1] == 0x08))
    {   // Chinook
        if (temp_pen1 == BLACK_PEN)
        {
            ePen = BOTH_PENS;
        }
        else
        {
            ePen = COLOR_PEN;
        }
    }
    else if (penInfoBits[0] == 0xc0)
    {   // missing pen
        ePen = temp_pen1;
    }
    else
    {
        return UNSUPPORTED_PEN;
    }

    return NO_ERROR;
}

BOOL Aladdin::IsPCL3DriverwareAvailable()
{
    BOOL pcl3driverware = TRUE;     // default to TRUE since this is the case for all but some early units
    BOOL inAJob = FALSE;
    char *pStr;
    char *pEnd;
    BYTE devIDBuff[DevIDBuffSize];
    int maxWaitTries;
    int i;

    // if don't have bidi can't check so assume driverware is ok since only certain 
    // 990s don't handle driverware in PCL3 mode
    if (!IOMode.bDevID)
    {
        return TRUE;
    }

    if (pSS->GetDeviceID(devIDBuff, DevIDBuffSize, TRUE) != NO_ERROR)
    {
        goto cleanup;
    }

    // if printer does not have firmware based on the first 990 release
    // don't bother checking, can assume driverware commands are OK
    if (!strstr((const char*)devIDBuff+2,"MDL:DESKJET 990C") &&
        !strstr((const char*)devIDBuff+2,"MDL:PHOTOSMART 1215") &&
        !strstr((const char*)devIDBuff+2,"MDL:PHOTOSMART 1218") )
    {
        return TRUE;
    }
 
    // high-level process to check if driverware is available in PCL3 mode:
    //	1. set JobName through the normal PJL command to some string X
    //	2. poll DeviceID until see this JobName (syncs host with printer in case 
    //     printer is still printing an earlier job etc.)
    //	3. go into PCL3 mode
    //	4. send driverware command to set JobName
    //	5. get the DeviceID and look at the JobName, if it is not "NEWDRIVERWARE" (what
    //     the firmware driverware command sets it to) conclude that driverware is 
    //     not available in PCL3 mode; if  the JobName is "NEWDRIVERWARE" then conclude 
    //     that driverware is available in PCL3 mode
    //	6. exit this "job" (send a UEL)

 
     
    // set the JobName via PJL
    if (Flush() != NO_ERROR) goto cleanup;    
	if (Send((const BYTE*)UEL,sizeof(UEL)) != NO_ERROR) goto cleanup;
    inAJob = TRUE;
	if (Send((const BYTE*)JobName,sizeof(JobName)) != NO_ERROR) goto cleanup;
    if (Send((const BYTE*)&Quote,1) != NO_ERROR) goto cleanup;
	if (Send((const BYTE*)OUR_PJL_JOBNAME,strlen(OUR_PJL_JOBNAME)) != NO_ERROR) goto cleanup;
    if (Send((const BYTE*)&Quote,1) != NO_ERROR) goto cleanup;
    if (Send((const BYTE*)&LF,1) != NO_ERROR) goto cleanup;
    if (pSS->FlushIO() != NO_ERROR) goto cleanup;
    
    // wait for printer to see this and set JobName in the DeviceID
    // we know printer will respond, it is just a matter of time so wait 
    // a while until see it since it may take a few seconds
    // for it to sync up with us if it is busy printing or picking
    //
    // this is a pretty long timeout but if the printer is finishing up
    // a preceding job we need to give it time to finish it since it won't
    // set the new jobname until the last job is complete
    // one possible enhancement would be to look at the flags in the DeviceID
    // to see if a job is active on more than one i/o connection
    maxWaitTries = 120;  // wait max of 60sec
    for (i=0; i<maxWaitTries; i++)
    {
        if (pSS->GetDeviceID(devIDBuff, DevIDBuffSize, TRUE) != NO_ERROR)    goto cleanup;
        if ( (pStr=strstr((const char*)devIDBuff+2,";J:")) )
        {
            pStr += 3;
            if ( (pEnd=strstr((const char*)pStr,";")) )
            {
                *pEnd = '\0';
                while (pEnd > pStr) // take out trailing spaces in JobName before compare
                {
                    if (*(pEnd-1) == ' ')
                    {
                        *(pEnd-1) = '\0';
                    }
                    else
                    {
                        break;
                    }
                    pEnd--;
                }
                if (!strcmp(pStr, OUR_PJL_JOBNAME))
                {
                    break;     
                }
            }
            pSS->BusyWait((DWORD)500);
        }
        else
        {
            DBG1("JobName missing from DeviceID strings");
            goto cleanup;
        }
    }
    if (i>=maxWaitTries)
    {
        // printer didn't respond to driverware in PCL3GUI mode withing allowed timeout
        DBG1("Printer didn't respond to PJL\n");
        goto cleanup;
    }
    
    // now printer is in sync with us so try PCL3 mode and expect it to react to command
    // immediately or will assume that it ignores driverware in that mode
	if (Send((const BYTE*)EnterLanguage,sizeof(EnterLanguage)) != NO_ERROR) goto cleanup;
    if (Send((const BYTE*)PCL3,sizeof(PCL3)) != NO_ERROR) goto cleanup;
    if (Send((const BYTE*)&LF,1) != NO_ERROR) goto cleanup;
    if (Send((const BYTE*)DriverwareJobName,sizeof(DriverwareJobName)) != NO_ERROR) goto cleanup;
    if (pSS->FlushIO() != NO_ERROR) goto cleanup;
    
    // wait for printer to see this and set DeviceID to reflect this command
    // since we are sending in PCL3 mode we don't know if printer will respond
    // so don't wait very long
    maxWaitTries = 4;
    for (i=0; i<maxWaitTries; i++)
    {
        if (pSS->GetDeviceID(devIDBuff, DevIDBuffSize, TRUE) != NO_ERROR)    goto cleanup;
        if ( (pStr=strstr((const char*)devIDBuff+2,";J:")) )
        {
            pStr += 3;
            if ( (pEnd=strstr((const char*)pStr,";")) )
            {
                *pEnd = '\0';
                // firmware may have garbage in remainder of JobName buffer - truncate
                // it to the length of the string that should be set before compare
                if (!strncmp(pStr, DRIVERWARE_JOBNAME, strlen(DRIVERWARE_JOBNAME)))
                {
                    pcl3driverware = TRUE;
                    break;                   
                }
            }
            pSS->BusyWait((DWORD)500);
        }
        else
        {
            DBG1("JobName missing from DeviceID string");
            goto cleanup;
        }
    }
    if (i>=maxWaitTries)
    {
        // since we haven't gotten a response assume that the printer ignores driverware
        // commands in PCL3 mode
        pcl3driverware = FALSE;
    }
    
    
cleanup:
    if (inAJob)   // send UEL in case left printer in the context of a job
    {
	    if (Send((const BYTE*)UEL,sizeof(UEL)) != NO_ERROR) goto bailout;
        if (pSS->FlushIO() != NO_ERROR) goto bailout;
    }

bailout:    
    return pcl3driverware;
}

/// ERNIE ////////////////////////////////////////////////////////////////
BOOL TErnieFilter::Process(BYTE* ImageData, unsigned int size)
{
    BYTE temp;
 
    size = size/3;          // turn bytes into pixels

    // first turn RGB into BGR
    for (unsigned int i=0; i < size; i++)
    {
        temp = ImageData[3*i];
        ImageData[3*i] = ImageData[3*i+2];
        ImageData[3*i+2] = temp;
    }

   submitRowToFilter(ImageData); 

   // something ready after 4th time only
   return (fNumberOfBufferedRows == 0);

}
unsigned int TErnieFilter::GetOutputWidth()
{ 
    return fRowWidthInPixels*3; 
}


BYTE* TErnieFilter::NextOutputRaster()
{
    if (iRastersReady==0)
        return (BYTE*)NULL;

    iRastersReady--;
    
    return fRowPtr[iRastersDelivered++];
}

void TErnieFilter::Flush()
{
    writeBufferedRows();
    iRastersDelivered=0;
    iRastersReady = fNumberOfBufferedRows;
    fNumberOfBufferedRows = 0;
}

#endif  // _DJ9xxVIP
