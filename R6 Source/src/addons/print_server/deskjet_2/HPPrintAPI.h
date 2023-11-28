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

// HPPrintAPI.h
// Class definitions of external interfaces

#ifndef HPPRINTAPI_H
#define HPPRINTAPI_H

#include "global_types.h"

//////////////////////////////////////////////////////////////////////
// objects needed by client of Slimhost++ driver

//forward declarations
class DeviceRegistry;
class Header;
class Printer;
class TextManager;
class RasterSender;
class Imager;
class Scaler;
class Scripter;

class IO_MODE 
{
public:
 //NO_FEEDBACK, STATUS_ONLY, BIDI, USB
	BOOL bDevID;
	BOOL bStatus;
	BOOL bUSB;
};

#define DevIDBuffSize 255		// size of buffer used by SetDevInfo

////////////////////////
class SystemServices
{
friend class PrintContext;
friend class Printer;   // for saved device strings
public:
	SystemServices();
	virtual ~SystemServices();					

    // check for validty of constructed object
	DRIVER_ERROR constructor_error;	

    // must include in derived class constructor (if using bi-di)
    DRIVER_ERROR InitDeviceComm();   

	/////////////////////////////////////////////////////////////////////
    IO_MODE IOMode;    

    virtual DRIVER_ERROR FlushIO() { return 0; }

    virtual DRIVER_ERROR AbortIO() { return 0; }

    virtual void DisplayPrinterStatus (DISPLAY_STATUS ePrinterStatus)=0;

    virtual DRIVER_ERROR BusyWait(DWORD msec)=0;

    virtual DRIVER_ERROR ReadDeviceID(BYTE* strID, int iSize)=0;

    virtual BYTE* AllocMem (int iMemSize)=0;

	virtual void FreeMem (BYTE* pMem)=0;

    virtual BOOL PrinterIsAlive();

    virtual BOOL GetStatusInfo(BYTE* bStatReg)=0;

    virtual DRIVER_ERROR ToDevice(const BYTE* pBuffer, DWORD* dwCount)=0;

    virtual DRIVER_ERROR FromDevice(char* pReadBuff, WORD* wReadCount)=0;

    // override this function to implement DJ400 
    virtual DRIVER_ERROR GetECPStatus(BYTE *pStatusString,int *pECPLength, int ECPChannel);

    virtual BOOL YieldToSystem (void)=0;

    virtual BYTE GetRandomNumber()=0;

    virtual DWORD GetSystemTickCount (void)=0;

    virtual float power(float x, float y)=0;

// utilities ///////////////////////////////////////////////////////
    // call FreeMem after checking for null ptr
	DRIVER_ERROR FreeMemory(void *ptr);    
    DRIVER_ERROR GetDeviceID(BYTE* strID, int iSize, BOOL query);


#if defined(CAPTURE) || defined(PROTO)
    Scripter *pScripter;
    DRIVER_ERROR InitScript(const char* FileName, BOOL ascii, BOOL read=FALSE);
    DRIVER_ERROR EndScript();
    BOOL Capturing;
    BOOL replay;
#endif


protected:
    // reconcile printer's preferred settings with reality
    virtual void AdjustIO(IO_MODE IM) 
        { IOMode.bStatus=IM.bStatus && IOMode.bStatus; 
          IOMode.bDevID =IM.bDevID  && IOMode.bDevID; }

    BYTE strDevID[DevIDBuffSize]; // save whole DevID string

private:

	PORTID ePortID;

    DeviceRegistry* DR;

    char strModel[200]; // to contain the MODEL (MDL) from the DevID 
    char strPens[64];   // to contain the VSTATUS penID from the DevID 
    int  VIPVersion;    // VIP version from the DevID
};


#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)

// font names
const char sCourier[]="Courier";
const char sCGTimes[]="CGTimes";
const char sLetterGothic[]="LetterGothic";
const char sUnivers[]="Univers";
const char sBad[]="Bad";

class Font
// This is the base class for Fonts.
// It is not abstract, so that clients can request a font generically,
// but its constructor is not public -- Fonts are rather created
// through RealizeFont -- thus it is effectively "abstract" in this sense.
// Example:
//			Font* myFont = myJob->RealizeFont(FIXED_SERIF,12,...);
//
// Note that Printer initially constructs a dummy font (with default values)
// for each of its typefaces, so that the Is<x>Allowed functions can be 
// invoked (for EnumFonts) prior to choosing specific instances. 
// Then the Clone function is invoked by
// Printer::RealizeFont to provide instances to client.
{
friend class Printer;
friend class TextManager;
public:
	// constructors are protected -- clients use Job::RealizeFont()
	virtual ~Font();

	// public functions

	// the base class version is really for printer fonts
	virtual DRIVER_ERROR GetTextExtent(const char* pTextString,const int iLenString,
								int& iHeight, int& iWidth);

////// these functions allow access to properties of derived classes

	// return typeface name
	virtual const char* GetName() const { return sBad; }

	// functions to tell what treatments are possible
	virtual BOOL IsBoldAllowed() const { return FALSE; }			
	virtual BOOL IsItalicAllowed() const { return FALSE; }		
	virtual BOOL IsUnderlineAllowed() const { return FALSE; }		
	virtual BOOL IsColorAllowed() const { return FALSE; }	
	virtual BOOL IsProportional() const { return FALSE; }
	virtual BOOL HasSerif() const { return FALSE; }


	// return pitch for given point size
	virtual BYTE GetPitch(const BYTE pointsize) const
		{ return 0; }	// default for proportionals

////// these data members give the properties of the actual instance
	// as set by the user
	int			iPointsize;
	BOOL		bBold;		// boolean TRUE to request bold
	BOOL		bItalic;	// boolean TRUE to request italic
	BOOL		bUnderline;	// boolean TRUE to request underline
	TEXTCOLOR	eColor;		// enum
	int			iPitch;

	// string designating character set (as recognized by firmware)
	//
	// REVISIT: shouldn't really have Translator data here; we
	// should have an enum here, which is interpreted by Translator
	char charset[MAX_CHAR_SET];	
	
	BOOL PrinterBased;

	virtual int Index() { return -1; };
	// items for spooling
//	virtual BOOL Equal(Font* f);
//	virtual DRIVER_ERROR Store(FILE* sp, int& size);
//	virtual int SpoolSize();

protected:
	// constructor, invoked by derivative constructors
	Font(int SizesAvailable,BYTE size=0,
			BOOL bold=FALSE, BOOL italic=FALSE, BOOL underline=FALSE,
			TEXTCOLOR color=BLACK_TEXT,BOOL printer=TRUE,
			unsigned int pvres=300,unsigned int phres=300);
	
	// copy constructor used by RealizeFont
	Font(const Font& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline);
	
	// return a clone with a different character set
	// base class version should not be called -- this should be pure virtual!
	virtual Font* CharSetClone(char* NewCharSet) const;

	int numsizes;	// number of available pointsizes
	// return array of sizes allowed
	virtual BYTE* GetSizes() const { return (BYTE*)NULL; }
	// return index of pointsize from array of available pointsizes
	virtual int Ordinal(unsigned int /* pointsize */) const 
		{ return 0; }

	// match arbitrary input size to one we have
	int AssignSize(int Size);
	void Subst_Char(int& bCurrChar)const;	

	// pointers to the arrays containing widths for a given font
	//  separated into Lo (32..127) & Hi (160..255)	
	const BYTE *pWidthLo[MAX_POINTSIZES];	
	const BYTE *pWidthHi[MAX_POINTSIZES];

	unsigned int PrinterVRes;
	unsigned int PrinterHRes;

	
private:
#ifdef CAPTURE

	void Capture_dFont(const unsigned int ptr);

#endif

};   

class ReferenceFont : public Font
// The main purpose of this class is to hide the destructor, since
// the fonts that live with the Printer and are returned by EnumFont
// are meant to remain alive for the life of the Printer.
{
friend class Printer;           // deletes from its fontarray
friend class DeskJet400;        // replaces fontarray from base class
public:
    ReferenceFont(int SizesAvailable,BYTE size=0,
			BOOL bold=FALSE, BOOL italic=FALSE, BOOL underline=FALSE,
			TEXTCOLOR color=BLACK_TEXT,BOOL printer=TRUE,
			unsigned int pvres=300,unsigned int phres=300);
protected:
    ~ReferenceFont();  

    // copy constructor used by RealizeFont
	ReferenceFont(const ReferenceFont& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline);

};
#endif      // if fonts used

class PrintMode;
class Pipeline;
class Compressor;

class PrintContext
{
friend class Job;         // access to private (non-instrumented) versions of routines
friend class Header;      // access to private (non-instrumented) versions of routines
friend class RasterSender;    // access to current printmode
friend class GraphicsTranslator; // access to current printmode
friend class DeskJet690;  // to override media and quality settings
public:
	PrintContext(SystemServices *pSysServ, 
                 unsigned int InputPixelsPerRow=0,
                 unsigned int OutputPixelsPerRow=0,
                 PAPER_SIZE ps = LETTER);        	

	virtual ~PrintContext();

	DRIVER_ERROR constructor_error;


    void Flush(int FlushSize);  // used in connection with SendPrinterReadyData

	// used when constructor couldn't instantiate printer (no DevID) -- instantiate now
	DRIVER_ERROR SelectDevice(const PRINTER_TYPE Model);

    unsigned int GetModeCount();
    DRIVER_ERROR SelectPrintMode(const unsigned int index);
    unsigned int CurrentPrintMode() { return CurrentModeIndex; }
    char* GetModeName();

    PRINTER_TYPE SelectedDevice(); 

    HELP_TYPE GetHelpType();



#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	ReferenceFont* EnumFont(int& iCurrIdx); 
		//	{ return thePrinter->EnumFont(iCurrIdx); } 
	virtual Font* RealizeFont(const int index,const BYTE bSize,
							const TEXTCOLOR eColor=BLACK_TEXT,
							const BOOL bBold=FALSE,const BOOL bItalic=FALSE,
							const BOOL bUnderline=FALSE);
		//	{ return thePrinter->RealizeFont(eFont,bSize,eColor,
		//									bBold,bItalic,bUnderline); }
#endif

    // return the enum for the next model in DR (UNSUPPORTED when finished)
	PRINTER_TYPE EnumDevices(unsigned int& currIdx) const;
       
	// PerformPrinterFunction (clean pen, etc.)
	// this is the preferred function to call
	DRIVER_ERROR PerformPrinterFunction(PRINTER_FUNC eFunc); 
	
    ///////////////////////////////////////////////////////////////////////
    // routines to change settings
    DRIVER_ERROR SetPaperSize(PAPER_SIZE ps); 
    // these are dependent on printer model in use, thus can err
    DRIVER_ERROR SetPixelsPerRow(unsigned int InputPixelsPerRow,
                                 unsigned int OutputPixelsPerRow=0);
    //
    // routines to query selections ///////////////////////////////////////
    BOOL PrinterSelected() { return !(thePrinter==NULL); }
    BOOL PrinterFontsAvailable(unsigned int PrintModeIndex);    // return FALSE if no printer
    unsigned int InputPixelsPerRow() { return InputWidth; }
    unsigned int OutputPixelsPerRow() { return OutputWidth; }
    PAPER_SIZE GetPaperSize();

    const char* PrinterModel();
    const char* PrintertypeToString(PRINTER_TYPE pt); // returns string for use in UI

    unsigned int InputResolution() { return InputRes; }
	DRIVER_ERROR SetInputResolution(unsigned int Res);
    unsigned int EffectiveResolutionX();       // res we need in current mode
    unsigned int EffectiveResolutionY();       // res we need in current mode

    // get settings pertaining to the printer
    // note:these return zero if no printer selected
    // all results in inches
    float PrintableWidth();                         
	float PrintableHeight();                        
	float PhysicalPageSizeX();
	float PhysicalPageSizeY();
	float PrintableStartX();
	float PrintableStartY();

    // SPECIAL API -- NOT TO BE USED IN CONNECTION WITH JOB
    DRIVER_ERROR SendPrinterReadyData(BYTE* stream, unsigned int size);
	
	DeviceRegistry* DR;     // unprotected for replay system

private:

	SystemServices* pSS;
	Printer* thePrinter;
    PrintMode* CurrentMode;    
    unsigned int CurrentModeIndex;
	
    unsigned int InputRes;                 // input resolution
    unsigned int PageWidth;             // pixel width of printable area
    unsigned int InputWidth;
    unsigned int OutputWidth;           // after scaling
    PAPER_SIZE thePaperSize;

    struct PaperSizeMetrics
    {
        // all values are in inches
	    float	fPhysicalPageX;
	    float	fPhysicalPageY;
	    float	fPrintablePageX;
	    float	fPrintablePageY;
		float   fPrintableStartY;
    } PSM[4];  // the size of this struct is directly related to the PAPER_SIZE enum

    void InitPSMetrics();   // used by constructors


	// internal versions of public functions
	float printablewidth();
	float printableheight();
	unsigned int printerunitsY();

    BOOL ModeAgreesWithHardware(BOOL QueryPrinter);

	DRIVER_ERROR setpixelsperrow(unsigned int InputPixelsPerRow,
                                 unsigned int OutputPixelsPerRow);
           
    // code savers
    DRIVER_ERROR SetMode(unsigned int ModeIndex);
    BOOL SelectDefaultMode();

	
#ifdef CAPTURE
	void Capture_PrintContext(unsigned int InputPixelsPerRow, unsigned int OutputPixelsPerRow,                                
                              PAPER_SIZE ps,IO_MODE IOMode);
    void Capture_SelectDevice(const PRINTER_TYPE Model);
    void Capture_SelectPrintMode(unsigned int modenum);
    void Capture_SetPaperSize(PAPER_SIZE ps);
	void Capture_RealizeFont(const unsigned int ptr,const unsigned int index,const BYTE bSize,
							const TEXTCOLOR eColor=BLACK_TEXT,
							const BOOL bBold=FALSE,const BOOL bItalic=FALSE,
							const BOOL bUnderline=FALSE);
	void Capture_SetPixelsPerRow(unsigned int InputPixelsPerRow,unsigned int OutputPixelsPerRow);
    void Capture_SetInputResolution(unsigned int Res);
	void Capture_dPrintContext();

#endif

	// BeOS stuff
public:
	Printer& GetPrinter() { return *thePrinter; }	
};

class TErnieFilter;
class RasterSender;
class Header;
class TextTranslator;

class Job
{
friend class RasterSender;
public:
	Job(PrintContext* pPC);
													
	virtual ~Job();
	
	DRIVER_ERROR constructor_error;		// caller must check upon return

	DRIVER_ERROR SendRasters(BYTE* ImageData=(BYTE*)NULL);
	
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)	
	DRIVER_ERROR TextOut(const char* pTextString, unsigned int iLenString,
							const Font& font, int iAbsX, int iAbsY );
	// return theTextManager->TextOut(pTextString,iLenString,font,iAbsX,iAbsY);
#endif

	DRIVER_ERROR NewPage();

private:

	PrintContext* thePrintContext;
	SystemServices* pSS;
	Printer* thePrinter;
	Pipeline* thePipeline;
	TextTranslator* pText;
    RasterSender* pSender;
	Imager* pImager;
	Scaler* pResSynth;
    Scaler* pReplicator;
    Compressor* theCompressor;
	TErnieFilter* pErnie;
    Header* pHead;


#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	TextManager* theTextManager;
#endif

    PrintMode* CurrentMode;

    unsigned int InputDPI;
    unsigned int skipcount;
    unsigned int RowsInput;
    BYTE* BlankRaster;

	unsigned int CAPy;			// maintains cursor-pos for graphics purposes,
						// independent of intervening text positioning
	BOOL DataSent;

    unsigned int RowMultiple;               // used for sending rows more than once
    unsigned int ResBoost;                  // for horizontal expansion    
    unsigned int numrows[MAXCOLORPLANES];   // rows per call for mixed-res only
    unsigned int OutputWidth;
    float ScaleFactor;

	DRIVER_ERROR Configure();
	DRIVER_ERROR InitScaler();
	
	DRIVER_ERROR newpage();
	DRIVER_ERROR SetupColor();
    DRIVER_ERROR SendCAPy();
    DRIVER_ERROR sendrasters(BYTE* ImageData=(BYTE*)NULL);
    DRIVER_ERROR setblankraster();

#ifdef CAPTURE
	void Capture_Job(PrintContext* pPC);
	void Capture_dJob();
	void Capture_SendRasters(BYTE* ImageData);
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	void Capture_TextOut(const char* pTextString, unsigned int iLenString,
							const Font& font, unsigned int iAbsX, unsigned int iAbsY );
#endif
	void Capture_NewPage();
#endif

#ifdef USAGE_LOG
	int UTextCount;
	int UText;
#define UTextSize 100
	char UHeader[UTextSize*2];
#endif
};


#endif // HPPRINTAPI_H
