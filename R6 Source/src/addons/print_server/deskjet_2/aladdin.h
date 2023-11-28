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

#ifndef ALADDIN_H
#define ALADDIN_H

class Aladdin : public Printer
{
public:
	Aladdin(SystemServices* pSS, BOOL proto=FALSE);

	Header* SelectHeader(PrintContext* pc); 
	DRIVER_ERROR VerifyPenInfo();
	DRIVER_ERROR ParsePenInfo(PEN_TYPE& ePen, BOOL QueryPrinter=TRUE);
// 	DISPLAY_STATUS ParseError(BYTE status_reg);

	Compressor* CreateCompressor(unsigned int RasterSize);

    virtual BOOL UseGUIMode(unsigned int PrintModeIndex); 
    BOOL UseCMYK(unsigned int PrintModeIndex) { return FALSE; }
//    DRIVER_ERROR CleanPen();     ???

    void SetHelpType(const char* model)
    {
        if(!strncmp(model, DEVID_MODEL_99X, strlen(DEVID_MODEL_99X))) help=dj990help;
        else help = dj9xxVIPhelp;
    }

protected:
    BOOL PCL3acceptsDriverware;
private:
	BOOL IsPCL3DriverwareAvailable();

};

class AladdinMode : public PrintMode
{
public:
    AladdinMode();
};


class GrayModeAladdin : public GrayMode
{
public:
    GrayModeAladdin(unsigned long *map, BOOL PCL3OK);

};

/////////////////////////////
#define kWhite 0x00FFFFFE
#define GetRed(x) (((x >> 16) & 0x0FF))
#define GetGreen(x) (((x >> 8) & 0x0FF))
#define GetBlue(x) ((x & 0x0FF))

#define kBertDecompressPixelSize 3

// Follows are all the masks for the command byte.
#define kTypeMask			0x80
#define kTypeShiftAmount	7

#define kCacheLiteralBitsMask 0x60
#define kCacheLiteralBitsShiftAmount 5

#define kCacheBitsMask 0x60
#define kCacheBitsShiftAmount 5

#define kRoffMask			0x18
#define kRoffShiftAmount	3

#define kReplace_countMask	0x07

// Now have the compiler check to make sure none of the masks overlap/underlap bits accidently.
#if ((kTypeMask | kCacheLiteralBitsMask | kRoffMask | kReplace_countMask) != 255)
#error "Your mask bits are messed up!"
#endif

#if ((kTypeMask | kCacheBitsMask | kRoffMask | kReplace_countMask) != 255)
#error "Your mask bits are messed up!"
#endif


enum
{
	eLiteral = 0,
	eRLE = 0x80
};

enum
{
	eeNewPixel = 0x0,
	eeWPixel = 0x20,
	eeNEPixel = 0x40,
	eeCachedColor = 0x60,
};

enum
{
	eNewColor		= 0x0,
	eWestColor		= 0x1,
	eNorthEastColor	= 0x2,
	eCachedColor	= 0x3
};


// Literal
#define M10_MAX_OFFSET0 		2		/* Largest unscaled value an offset can have before extra byte is needed. */
#define M10_MAX_COUNT0 			6   	/* Largest unscaled and unbiased value a count can have before extra byte is needed */
#define M10_COUNT_START0 		1  		/* What a count of zero has a value of. */

// RLE
#define M10_MAX_OFFSET1 		2
#define M10_MAX_COUNT1 			6
#define M10_COUNT_START1 		2

#define MIN(a,b)	(((a)>=(b))?(b):(a))
#define MAX(a,b)    (((a)<=(b))?(b):(a))

class Mode10 : public Compressor
{
public:
	Mode10(SystemServices* pSys,unsigned int RasterSize);
	virtual ~Mode10();
	DRIVER_ERROR Process(BYTE* input, unsigned int size);

private:
 
    inline unsigned int get4Pixel(unsigned char *pixAddress)
        { return (((unsigned int*)pixAddress)[0]) & kWhite; }
    
    inline unsigned int get4Pixel(unsigned char *pixAddress, int pixelOffset)
        { return ((unsigned int*)pixAddress)[pixelOffset] & kWhite; }

    inline void put4Pixel(unsigned char *pixAddress, int pixelOffset, unsigned int pixel)
        { (((unsigned int*)pixAddress)[pixelOffset] = pixel & kWhite); }

    inline unsigned int get3Pixel(unsigned char *pixAddress)
        { return (*((unsigned int*)pixAddress)) & kWhite; }

    inline unsigned int getPixel(unsigned char *pixAddress, int pixelOffset, int pixelSizeInBytes)
            { if (3 == pixelSizeInBytes) return(get3Pixel(pixAddress, pixelOffset));
              else return(get4Pixel(pixAddress, pixelOffset)); 
            } 

    inline void outputVLIBytesConsecutively(int number, unsigned char *&compressedDataPtr)
        { do {	*compressedDataPtr++ = MIN(number, 255);
		        if (255 == number) *compressedDataPtr++ = 0;
		        number -= MIN(number,255); 
             } while (number);
        }

	        
    void put3Pixel(unsigned char *pixAddress, int pixelOffset, unsigned int pixel);
    void putPixel(unsigned char *pixAddress, int pixelOffset, int pixelSizeInBytes, unsigned int pixel);
    unsigned int get3Pixel(unsigned char *pixAddress, int pixelOffset);
    unsigned short ShortDelta(int lastPixel, int lastUpperPixel);


};


#ifdef PROTO
extern PEN_TYPE ProtoPenType;
class ProtoAladdin : public Aladdin
{
public:
	ProtoAladdin(ProtoServices* pSS);

    DRIVER_ERROR ParsePenInfo(PEN_TYPE& ePen) { ePen=ProtoPenType; return NO_ERROR; }
	
};


#endif

#endif
