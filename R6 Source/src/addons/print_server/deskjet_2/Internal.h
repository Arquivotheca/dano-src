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

#ifndef INTERNAL_H
#define INTERNAL_H

/////////////////////////////////////////////////////////////////////////
// contains all class declarations
// for Slimhost++ driver
//
// merged in from file "Objects.h" 5/18/98
/////////////////////////////////////////////////////////////////////////
// these correspond to PCL codes
typedef int TYPEFACE;
#define COURIER 3
#define LETTERGOTHIC 6
#define CGTIMES 4101
#define UNIVERS 52

// character set names for PCL
#define LATIN1 "0N"		// aka ECMA94
#define PC8 "10U"
#define HP_LEGAL "1U"


// used to encourage consistent ordering of color planes
#define K	0
#define C	1
#define M	2
#define Y	3
#define Clight	4
#define Mlight	5

#define RANDSEED 77


#define DEFAULT_SLOW_POLL_COUNT 30
#define DEFAULT_SLOW_POLL_BIDI 3

//////////////////////////////////////////

enum STYLE_TYPE { UPRIGHT, ITALIC };

enum WEIGHT_TYPE { NORMAL, BOLD };


///////////////////////////////////////////////////////////////////

#define MAX_ESC_SEQ 40
#define MAX_RASTERSIZE	10000	// REVISIT


// very frequently used fragments made into macros for readability
#define CERRCHECK if (constructor_error != NO_ERROR) {DBG1("CERRCHECK fired\n"); return;}
#define ERRCHECK if (err != NO_ERROR) {DBG1("ERRCHECK fired\n"); return err;}
#define NEWCHECK(x) if (x==NULL) return ALLOCMEM_ERROR;
#define CNEWCHECK(x) if (x==NULL) { constructor_error=ALLOCMEM_ERROR; return; }



//////// STATIC DATA ////////////////////////////////////////////////////////////////
// escape sequences -- see PCL Implementor's Guide or Software Developer's PCL Guides
// for documentation
#define ESC 0x1b

const char UEL[] = {ESC, '%', '-','1','2','3','4','5','X' };
const char EnterLanguage[] = {'@','P','J','L',' ','E','N','T','E','R',' ',
						'L','A','N','G','U','A','G','E','=' };
const char PCL3[] = {'P','C','L','3' };
const char PCLGUI[] = {'P','C','L','3','G','U','I' };	
const char JobName[] = {'@','P','J','L',' ','J','O','B',' ','N','A','M','E',' ','=',' '};
const char Reset[] = {ESC,'E'};
const char crdStart[] =	{ESC, '*', 'g'};			// configure raster data command
const char cidStart[] =	{ESC, '*', 'v'};			// configure image data command
const char crdFormat = 2; // only format for 600
const char grafStart[] = {ESC, '*', 'r', '1', 'A'};	// raster graphics mode
const char grafMode0[] = {ESC, '*', 'b', '0', 'M'};	// compression methods
const char grafMode9[] =	{ESC, '*', 'b', '9', 'M'};
const char grafMode2[] =	{ESC, '*', 'b', '2', 'M'};
const char SeedSame[] =	{ESC, '*', 'b', '0', 'S'};
//const char EjectPage[] = {ESC, '&', 'l', '0', 'H'};	// not needed by us; will pick if no page already picked
const char BlackExtractOff[] = {ESC, '*', 'o', '5', 'W', 0x04, 0xC, 0, 0, 0 };
const char LF = '\012';
const char Quote = '\042';
const BYTE Venice_Power_On[] = {ESC, '%','P','u','i','f','p','.',
        'p','o','w','e','r',' ','1',';',
        'u','d','w','.','q','u','i','t',';',ESC,'%','-','1','2','3','4','5','X' };
/*const BYTE Venice_Pre_Pick[] = {ESC, '&', 'l', -2, 'H'};
{ESC, '%','P','m','e','c','h','.',
        'l','o','a','d','_','p','a','p','e','r',';',
        'u','d','w','.','q','u','i','t',';' };//,ESC,'%','-','1','2','3','4','5','X' };
*/
const char EnableDuplex[] = { ESC,'&','l', '2', 'S'};
const char NoDepletion[] = {ESC, '*', 'o', '1', 'D'};
const char NoGrayBalance[] = {ESC, '*', 'b', '2', 'B'};
const char EnableBufferFlushing[] =  { ESC,'&','b','1','5','W','P','M','L',32,4,0,5,1,2,1,1,5,4,1,1 };
const char DisableBufferFlushing[] = { ESC,'&','b','1','5','W','P','M','L',32,4,0,5,1,2,1,1,5,4,1,2 };
const char DriverwareJobName[] = { ESC,'*','o','5','W',0x0d,0x06,0x00,0x00,0x01 };
//////////////////////////////////////////
class Pipeline;
//Scaler, Imager, ErnieFilter, PixelReplicator, (FRE object, ...) are subclasses of
class Processor
// Executes the "Process" method in its containee.
{
public:
	Processor();
	virtual ~Processor();

	virtual BOOL Process(BYTE* InputRaster=NULL, unsigned int size=0)=0;	// returns TRUE iff output ready
    virtual void Flush()=0;     // take any concluding actions based on internal state
    virtual BYTE* NextOutputRaster()=0;
    virtual unsigned int GetOutputWidth()=0;        // in bytes, not pixels
    virtual unsigned int GetMaxOutputWidth() { return GetOutputWidth(); }

    unsigned int iRastersReady, iRastersDelivered;
    Pipeline* myphase;
};

class Pipeline
{
public:
    Pipeline(Processor* E);                
	virtual ~Pipeline();

	void AddPhase(Pipeline* p);             // add p at end
    DRIVER_ERROR Execute(BYTE* InputRaster=NULL, unsigned int size=0);   // run pipeline
    DRIVER_ERROR Flush();

	BOOL Process(BYTE* InputRaster=NULL, unsigned int size=0);   // call processor for this phase

    BYTE* NextOutputRaster()      { return Exec->NextOutputRaster(); } 
    unsigned int GetOutputWidth() { return Exec->GetOutputWidth(); }
    unsigned int GetMaxOutputWidth() { return Exec->GetMaxOutputWidth(); }

    Pipeline* next;
	Pipeline* prev;

	Processor* Exec;

    DRIVER_ERROR err;
	
};

struct PipeConfigTable
{
    BOOL bResSynth;
    BOOL bErnie;
    BOOL bPixelReplicate;
    BOOL bColorImage;
    BOOL bCompress;
};



///////////////////////////////////////////////////////////////////////////
class Scaler : public Processor
{
public:
    // constructor protected -- use Create_Scaler()
	virtual ~Scaler();
	BOOL Process(BYTE* InputRaster=NULL, unsigned int size=0);
    void Flush() { Process(); }
	
	DRIVER_ERROR constructor_error;	

    float ScaleFactor;
	
    unsigned int GetOutputWidth();
    BYTE* NextOutputRaster();
	
protected:
    Scaler(SystemServices* pSys,unsigned int inputwidth,unsigned int numerator,unsigned int denominator);
	SystemServices* pSS;
	unsigned int ResSyn(const unsigned char *raster_in);
	int create_out(BOOL simple);
	virtual void rez_synth(RESSYNSTRUCT *ResSynStruct, unsigned char *raster_out)=0;
	void Pixel_ReplicateF(int color, int h_offset, int v_offset, 
						  unsigned char **out_raster, int plane);
    virtual void InitInternals() { }

	RESSYNSTRUCT* pRSstruct;
	BOOL scaling;		// false iff ScaleFactor==1.0
	BOOL ReplicateOnly;	// true iff 1<ScaleFactor<2	
	

	unsigned int iOutputWidth;
	unsigned int iInputWidth;
    BYTE* pOutputBuffer;

};


////////////////////////////////////////////////////////////////
// DeviceRegistry, for isolating all device dependencies
// The data is contained in Registry.cpp

// This object encapsulates all model-specific data for a build.
// Its features are presented to client through the PrintContext.
class DeviceRegistry
{
public:
	DeviceRegistry();
    virtual ~DeviceRegistry();


    // get model string from DevID string
    DRIVER_ERROR ParseDevIDString(const char* sDevID, char* strModel, int *pVIPVersion, char* strPens);

	// return the string representing the next model (NULL when finished)
	PRINTER_TYPE EnumDevices(unsigned int& currIdx) const;
	
	// set "device" to index of entry
	virtual DRIVER_ERROR SelectDevice(char* model, int* pVIPVersion, char* pens, SystemServices* pSS);

    virtual DRIVER_ERROR SelectDevice(const PRINTER_TYPE Model);
	
	virtual DRIVER_ERROR GetPrinterModel(char* strModel, int* pVIPVersion, char* strPens, SystemServices* pSS);

	char* ModelString[MAX_ID_STRING];

	// create a Printer object as pointee of p, using the given SystemServices
	// and the current value of device; still needs to be configured
	virtual DRIVER_ERROR InstantiatePrinter(Printer*& p,SystemServices* pSS);

	
	int device;							// ordinal of device from list (or UNSUPPORTED=-1)

};

////////////////////////////////////////////////
typedef struct 
{
	const unsigned long *ulMap1;
	const unsigned long *ulMap2;
} ColorMap;


///////////////////////////////////////////////////////////////////////////

class Compressor : public Processor
//  base class for compression methods
{
public:
	Compressor(SystemServices* pSys, unsigned int RasterSize, BOOL useseed);
	virtual ~Compressor();

    virtual BOOL Process(BYTE* InputRaster=NULL, unsigned int size=0)=0;
    void Flush() { }    // no pending output

    unsigned int GetOutputWidth() { return compressedsize; }
    virtual BYTE* NextOutputRaster();

	void SetSeedRow(BYTE* seed) { SeedRow=seed; }

	DRIVER_ERROR constructor_error;

	SystemServices* pSS;
	// buffer is public for use by GraphicsTranslator
	BYTE* compressBuf;		// output buffer
	BYTE* SeedRow;
	BOOL UseSeedRow;

    unsigned int compressedsize;
};

class Mode9 : public Compressor
{
public:
	Mode9(SystemServices* pSys,unsigned int RasterSize);
	virtual ~Mode9();
	DRIVER_ERROR Process(BYTE* input, unsigned int size);
};

class Mode2 : public Compressor
{
public:
	Mode2(SystemServices* pSys,unsigned int RasterSize);
	virtual ~Mode2();
	DRIVER_ERROR Process(BYTE* input, unsigned int size);
}; 


////////////////////////////////////////////////////////////////////////////
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
class TextTranslator
// Does encapsulation work specific to ascii data, including
// handling of fonts and treatments.
{
public:
	TextTranslator(Printer* p,int quality,unsigned int colorplanes);
	virtual ~TextTranslator();

    DRIVER_ERROR TextOut(const char* pTextString, unsigned int LenString, 
				const Font& font, BOOL sendfont=FALSE, 
				int iAbsX=-1, int iAbsY=-1);

    DRIVER_ERROR SendCAP(unsigned int iAbsX,unsigned int iAbsY);
	const BYTE* ColorSequence(TEXTCOLOR eColor);	
	BYTE ColorCode(TEXTCOLOR eColor);
	int TypefaceCode(const char* FontName);
	const BYTE* PointsizeSequence(unsigned int iPointsize);
	const BYTE* StyleSequence(BOOL bItalic);
	const BYTE* WeightSequence(BOOL bBold);
	const BYTE* CompleteSequence(const Font& font);
	const BYTE* UnderlineSequence();
	const BYTE* DisableUnderlineSequence();
	
	// "transparent mode" escape to treat control code (BYTE b) as normal char
	int TransparentChar(unsigned int iMaxLen, BYTE b, BYTE* outbuff);	

	DRIVER_ERROR constructor_error;

    DRIVER_ERROR SendFont(const Font& font);
	DRIVER_ERROR SendColorSequence(const TEXTCOLOR eColor);
	DRIVER_ERROR SendPointsize(const unsigned int iPointsize);
	DRIVER_ERROR SendStyle(const BOOL bItalic);
	DRIVER_ERROR SendWeight(const BOOL bBold);
	DRIVER_ERROR SendUnderline();
	DRIVER_ERROR SendCompleteSequence(const Font& font);
	DRIVER_ERROR DisableUnderline();


private:
    Printer* thePrinter;
	int qualcode;						// pcl code for text quality
	BYTE EscSeq[MAX_ESC_SEQ];			// storage for the command
	unsigned int iNumPlanes;						// color planes, based on pen
	BYTE ColorCode1(TEXTCOLOR eColor);	// if iNumPlanes==1 (black)
	BYTE ColorCode3(TEXTCOLOR eColor);	// if iNumPlanes==3 (CMY)
	BYTE ColorCode4(TEXTCOLOR eColor);	// if iNumPlanes==4 (KCMY)

    // items for avoiding redundant font resets 
	// (cheaper than copying whole font)
	TEXTCOLOR lastcolor;
	char lastname[20];
	char lastcharset[MAX_CHAR_SET];
	int lastpointsize;
	BOOL lastitalic;
	BOOL lastbold;
	void SetLast(const Font& font);
	
};

#endif

///////////////////////////////////////////////////////////////////////
class Header
// Composes a header stream, embodying specific requirements
// of the Printer.
{
friend class Job;
public:
	Header(Printer* p,PrintContext* pc);
			
	virtual DRIVER_ERROR Send()=0;

	virtual DRIVER_ERROR EndJob();

    DRIVER_ERROR SendCAPy(unsigned int iAbsY);
    DRIVER_ERROR FormFeed();

    unsigned int CAPy;	// may be moved during header; retrieved by Job

protected:
	Printer* thePrinter;
    PrintContext* thePrintContext;
    PrintMode* thePrintMode;
	/// routines to set values of internal variables
	void SetMediaType(MediaType mtype);
    void SetMediaSize(PAPER_SIZE papersize);
	void SetMediaSource(MediaSource msource);
	void SetQuality(Quality qual);	
	void SetSimpleColor();

	// components of a header
	DRIVER_ERROR Margins();
	virtual DRIVER_ERROR Graphics();
	DRIVER_ERROR Simple();
	DRIVER_ERROR Modes();
	DRIVER_ERROR ConfigureRasterData();

	// common escapes, plus mode and margin setting
	virtual DRIVER_ERROR StartSend();

////// data members /////////////////////////////////
	unsigned int ResolutionX[MAXCOLORPLANES];
	unsigned int ResolutionY[MAXCOLORPLANES];
	unsigned int dyeCount;
    
    // utilities
    
    unsigned int ColorLevels(unsigned int ColorPlane);

	// escape sequence constants 
	char SimpleColor[6]; BYTE sccount;		// color command string, and its size
	char mediatype[6]; BYTE mtcount;		// mediatype string, and its size
	char mediasize[6]; BYTE mscount;		// mediasize string, and its size
	char mediasource[6]; BYTE msrccount;	// mediasource string, and its size
	char quality[6]; BYTE qualcount;		// quality string, and its size	
	BYTE QualityCode();			// returns just the variable byte of quality

};

class Header400 : public Header
{
public:
	Header400(Printer* p,PrintContext* pc);
	DRIVER_ERROR Send();

};


class Header6XX : public Header
{
public:
	Header6XX(Printer* p,PrintContext* pc);
	virtual DRIVER_ERROR Send();
protected:

};

class Header600 : public Header6XX
{
public:
	Header600(Printer* p,PrintContext* pc);
	DRIVER_ERROR Send();

};

class Header690 : public Header
{
public:
	Header690(Printer* p,PrintContext* pc);
    DRIVER_ERROR Send();
};

class Header540 : public Header
{
public:
	Header540(Printer* p,PrintContext* pc);
	DRIVER_ERROR Send();

};


class Header895 : public Header
{
public:
	Header895(Printer* p,PrintContext* pc);
	virtual DRIVER_ERROR Send();

protected:
	DRIVER_ERROR Graphics();
	DRIVER_ERROR StartSend();
};

class Header900 : public Header895
{
public:
	Header900(Printer* p,PrintContext* pc);
	virtual DRIVER_ERROR Send();

protected:
	BOOL DuplexEnabled(BYTE* bDevIDBuff);

};

class HeaderAladdin : public Header
{
public:
	HeaderAladdin(Printer* p,PrintContext* pc);
    DRIVER_ERROR ConfigureRasterData();
    DRIVER_ERROR ConfigureImageData();
	DRIVER_ERROR Send();
    DRIVER_ERROR StartSend();
};

class Header630 : public Header 
{ 
public: 
    Header630(Printer* p,PrintContext* pc); 
    DRIVER_ERROR Send(); 
}; 

class Header2100 : public Header 
{ 
public: 
    Header2100(Printer* p,PrintContext* pc); 
    DRIVER_ERROR Send(); 
}; 

class RasterSender : public Processor
{
friend class Header;
friend class Header895;
friend class Header900;
public:
   // installs Header and Connection
	RasterSender(Printer* pP, PrintContext* pPC,
                Job* pJob,Imager* pImager);

    virtual ~RasterSender();

	// processor interface ////////////////////////////////////
	BOOL Process(BYTE* InputRaster=NULL, unsigned int size=0);
    void Flush() { };
    BYTE* NextOutputRaster() { return NULL; }   // end of pipeline
    unsigned int GetOutputWidth() { return 0; } // never called
	

	DRIVER_ERROR constructor_error;

	DRIVER_ERROR SendRaster(BYTE* InputRaster,unsigned int size);


private:
	Printer* thePrinter;

    PrintContext* thePrintContext;
    Job* theJob;
    Imager* theImager;
	

};
// end of RasterSender section ///////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)

class TextMapper
// Component of TextManager
// This class encapsulates mappings that may be 
// peculiar to different partners or data sources.
// The use is as follows:
//  1. invoke Map
//  2. now access SubstLen and charset
//
// Currently sets charset to LATIN1 at construction.
{
public:
	TextMapper(TextTranslator* t);

	// main function -- puts alternate string in buffer
	virtual void Map(BYTE b,BYTE* bSubst);

	// public members for access after call to Map()
	unsigned int SubstLen;
	char charset[MAX_CHAR_SET];

protected:
	TextTranslator* theTranslator;
};

class GenericMapper : public TextMapper
{
public:
	GenericMapper(TextTranslator* t);
	void Map(BYTE b,BYTE* bSubst);
};
/////////////////////////////////////////////////////////////////////

class TextManager
// Component of TextJob
{
public:
	TextManager(TextTranslator* t,unsigned int PrintableX, unsigned int PrintableY);
	virtual ~TextManager();
	
	virtual DRIVER_ERROR TextOut(const char* pTextString, unsigned int iLenString, 
				const Font& font, int iAbsX=-1, int iAbsY=-1);
	TextTranslator* theTranslator;

	DRIVER_ERROR constructor_error;

protected:
	
	unsigned int PrintableRegionX;
	unsigned int PrintableRegionY;
	
	DRIVER_ERROR CheckCoords(unsigned int iAbsX, unsigned int iAbsY );

	TextMapper* theMapper;

};

#endif     // FONTS
///////////////////////////////////////////////////////////////////////////



#endif // INTERNAL_H
