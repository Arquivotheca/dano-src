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

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)

#ifdef PROTO
#include "../include/Header.h"
#else
#include "Header.h"
#endif


TextManager::TextManager(TextTranslator* t,unsigned int PrintableX, unsigned int PrintableY)
	: theTranslator(t), PrintableRegionX(PrintableX), PrintableRegionY(PrintableY)
{
	constructor_error=NO_ERROR;

	// for now just use our generic mapping
	theMapper=new GenericMapper(theTranslator);
	CNEWCHECK(theMapper);
	// no other error possible in TextMapper
}

TextManager::~TextManager()
{
	delete theMapper;
}

TextMapper::TextMapper(TextTranslator* t)
	: SubstLen(1), theTranslator(t)
{ 
	strcpy(charset,"");
}

GenericMapper::GenericMapper(TextTranslator* t)
	: TextMapper(t)
{ }
//////////////////////////////////////////////////////////////////////////////////
DRIVER_ERROR TextManager::CheckCoords(unsigned int iAbsX, unsigned int iAbsY )
{ 		
	if ( iAbsX > PrintableRegionX ) 
	  {
		DBG1("TextOut: text coords set outside printable area\n");
		return ILLEGAL_COORDS; 
	  }
	if ( iAbsY > PrintableRegionY ) 
	  {
		DBG1("TextOut:  text baseline is beyond lower page boundary\n");
		return ILLEGAL_COORDS;
	  }
 return NO_ERROR;
}

#define SUBST_BUFF_SIZE  10

void TextMapper::Map(BYTE b,BYTE* bSubst)
// Generic dumb mapper, always returns checkerboard char.
{
	strcpy(charset, LATIN1);
	bSubst[0]= 127;
}
	
void GenericMapper::Map(BYTE b,BYTE* bSubst)
{
	strcpy(charset, LATIN1);
	SubstLen=1;
	switch(b) 
	  {
		case 128: SubstLen=theTranslator->TransparentChar(SUBST_BUFF_SIZE,16,bSubst);
				  strcpy(charset,PC8);
				  break;
		case 129: SubstLen=theTranslator->TransparentChar(SUBST_BUFF_SIZE,17,bSubst); 
				  strcpy(charset,PC8);
				  break;
		case 130: bSubst[0]=44; break;
		case 131: bSubst[0]=159; 
				  strcpy(charset,PC8);
				  break;
		case 132: bSubst[0]=44;
				  bSubst[1]=44;
				  SubstLen=2;
				  break;
		case 133: bSubst[0]=46;
				  bSubst[1]=46;
				  bSubst[2]=46;
				  SubstLen=3;
				  break;
		case 134: bSubst[0]=125;
				  strcpy(charset,HP_LEGAL);
				  break;
		case 135: bSubst[0]=216; 
				  strcpy(charset,PC8);
				  break;
		case 136: bSubst[0]=94; break;
		case 137: bSubst[0]=37; break;
		case 138: bSubst[0]=83; break;
		case 139: bSubst[0]=60; break;
		case 140: bSubst[0]=67;
				  bSubst[1]=69;
				  SubstLen=2;
				  break;
		case 145: bSubst[0]=39; break;
		case 146: bSubst[0]=39; break;
		case 147: bSubst[0]=34; break;
		case 148: bSubst[0]=34; break;
		case 149: SubstLen=theTranslator->TransparentChar(SUBST_BUFF_SIZE,0x07,bSubst);
				  strcpy(charset,PC8);
				  break;
		case 150: bSubst[0]=173; break;
		case 151: bSubst[0]=196;
				  strcpy(charset,PC8);
				  break;
		case 152: bSubst[0]=126; break;
		case 153: bSubst[0]=126; 
				  strcpy(charset, HP_LEGAL);
				  break;
		case 154: bSubst[0]=115; break;
		case 155: bSubst[0]=62; break;
		case 156: bSubst[0]=99;
				  bSubst[1]=101;
				  SubstLen=2;
				  break;
		case 159: bSubst[0]=255; break;
		default:  bSubst[0]=127; break;
	  } // switch - char substitution

}

DRIVER_ERROR TextManager::TextOut(const char* pTextString, unsigned int iLenString, 
				  const Font& font,
							  int iAbsX, int iAbsY ) 
{
	unsigned int iStart, iEnd;  // used for breaking up string into sections to handle char substitution
	DRIVER_ERROR err=NO_ERROR;  // for error checking and pass-thru
	
	BYTE bSubst[SUBST_BUFF_SIZE]; // character substitute string
	int iLenBSubst=1; // number of bytes to send for the char substitution

	err=CheckCoords(iAbsX, iAbsY );
	ERRCHECK;

	
	// since we don't have a buffer we can manipulate in these cases, 
	// we need to cycle through the pTextString breaking it into chunks 
	// for char substitution if necessary
	iStart=0;
	iEnd=0;
	do {
		// 1) cycle through the pTextString until illegal char is discovered
		// REVISIT:  what control codes (<32) do I need to support?
		iStart=iEnd;
		for (;iEnd<iLenString; iEnd++) 
		  {
			// check characters outside the 32..127, 160..255 range 
			// and stop when I get to one
			if((BYTE)pTextString[iEnd]<32 || 
				((BYTE)pTextString[iEnd]>127 && 
				 (BYTE)pTextString[iEnd]<160))
				 break;
		  }
		
		// 2a) now we have a checked string (iStart to <iEnd) 
		// to send to the Translator (if legal)
		//
		if(iEnd-iStart > 0) 
		  { // bad char will have a length of 0 from loop above
			err=theTranslator->TextOut(&pTextString[iStart], iEnd-iStart, 
									font, TRUE, iAbsX, iAbsY);
			// now reset my abs coords so subsequent calls will use default positioning
			iAbsX=-1;
			iAbsY=-1;
		  	if (err != NO_ERROR) 
				return err;
			
		  }

		// or 2b) send my substitution character (for invalid char)
		//
		if (iEnd<iLenString) 
		 {
			
			theMapper->Map(pTextString[iEnd],bSubst);
			iLenBSubst=theMapper->SubstLen;
			if (strcmp(theMapper->charset, LATIN1))
			 {
				Font* Alternate=font.CharSetClone(theMapper->charset);
				// change font in printer when TextOut called
				//theTranslator->ForceFontSend();
				err=theTranslator->TextOut((char*)&bSubst,iLenBSubst,
											*Alternate,TRUE,iAbsX,iAbsY);
				// change back next time
				//theTranslator->ForceFontSend();
				delete Alternate;
			 }
			else err=theTranslator->TextOut((char*)&bSubst,iLenBSubst,
								 font,TRUE,iAbsX,iAbsY);
			
			iEnd++; // go past bad char in my original string
			iAbsX=-1; // in case this was the first call to textout, I'll set
			iAbsY=-1; //  remaining coords to default positioning
			ERRCHECK;
		
		} //if (iEnd<iLenString) 

	} while (iEnd<iLenString);
	// logically this should be NO_ERROR or ILLEGAL_COORDS at this point
	return err;
	
} // TextOut

#endif
