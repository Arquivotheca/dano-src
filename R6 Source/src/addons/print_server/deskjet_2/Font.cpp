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


////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//
Font::Font(int SizesAvailable,BYTE size, 
			BOOL bold, BOOL italic, BOOL underline,
			TEXTCOLOR color,BOOL printer,
			unsigned int pvres,unsigned int phres)
			:  bBold(bold),
			bItalic(italic), bUnderline(underline), eColor(color),
			PrinterBased(printer),
			numsizes(SizesAvailable),PrinterVRes(pvres),PrinterHRes(phres)
{  
	strcpy(charset,LATIN1);		// all start this way
	iPitch=0;	// default for proportional	
}

Font::Font(const Font& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline)
	: iPointsize(bSize), bBold(bold),
	bItalic(italic), bUnderline(underline), eColor(color),
	iPitch(f.iPitch), PrinterBased(f.PrinterBased),
	numsizes(f.numsizes)   
{ 
	strcpy(charset,f.charset);
	PrinterVRes=f.PrinterVRes;
	PrinterHRes=f.PrinterHRes;
}

Font::~Font()
{
#ifdef CAPTURE
	Capture_dFont((const unsigned int)this);
#endif
}

ReferenceFont::ReferenceFont(int SizesAvailable,BYTE size, 
			BOOL bold, BOOL italic, BOOL underline,
			TEXTCOLOR color,BOOL printer,
			unsigned int pvres,unsigned int phres)
			:  Font(SizesAvailable,size,bold,italic,underline,color)
{   }

ReferenceFont::ReferenceFont(const ReferenceFont& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline)
	: Font(f,bSize,color,bold,italic,underline)
{ }


ReferenceFont::~ReferenceFont()
{ }



//////////////////////////////////////////////////////////////////////////
int Font::AssignSize(int Size)
// Assign point size:  this assumes enumfont size array is IN ASCENDING ORDER!
// One routine in base class, used by all derivatives;
//  remember to use "this" to access any virtuals.
{
	int i;
	if (Size==0)	// dummy initialization from printer
		return 0;

	BYTE* sizes = GetSizes();

#if defined(DEBUG) && (DBG_MASK & DBG_LVL1)
//printf("AssignSize request is %d\n",Size);
//printf("- %d Sizes available are: ",numsizes);
//for(i=0;i<numsizes;i++) printf("[%d]=%d ",i,sizes[i]);
//printf("\n");
#endif
	
	// *1* if less than or equal to smallest available, assign to smallest
	if(Size <= sizes[0])
		return sizes[0];
	
	for(i=1; i < numsizes; i++) // we've already checked i=0 case above
	  {
		// *2* assign size if match found - obviously
		if(Size == sizes[i]) 
			return sizes[i];
			
		// *3* assign to closest match, use smaller size if it's a tie
		if(Size < sizes[i]) 
		  {
			if( (sizes[i] - Size) < (Size -sizes[i-1])) 
				return sizes[i];
			else 
				return sizes[i-1];
		  }
	  } // end for

	// *4* assign to biggest if bigger not available
	return sizes[numsizes-1];
}


/////////////////////////////////////////////////////////////////
DRIVER_ERROR Font::GetTextExtent(const char* pTextString,const int iLenString,
					int& iHeight, int& iWidth) 
{


	iHeight=0;		// clear any previous value before
	iWidth=0;		//   potential error return
		
	int iCalcWidth = 0;
	int iCalcHeight = 0;
	
	const BYTE *pFntWidthLo = NULL;	// pointers to the array containing widths for a given font
	const BYTE *pFntWidthHi = NULL;	//  separated into Lo (32..127) & Hi (160..255)
	
	int bCurrChar = 0;			// ASCII value of the current character
	int iBoldAdjust = 0;		// add boldness adjustment to char width if Bold is used

	int nthpoint=Ordinal(iPointsize);
#if defined(DEBUG) && (DBG_MASK & DBG_LVL1)
// printf("iPointsize=%d, nthpoint=%d\n",iPointsize,nthpoint);
#endif
	if (nthpoint == -1)
		return SYSTEM_ERROR;	//because something went wrong with AssignSize

	// Is this a fixed or prop font?
	if(IsProportional()) 
	  {

		pFntWidthLo = pWidthLo[nthpoint];
		pFntWidthHi = pWidthHi[nthpoint];
#if defined(DEBUG) && (DBG_MASK & DBG_LVL1)
// printf("*pWidthLo[0]=%d, *pWidthHi[0]=%d\n",*pWidthLo[0],*pWidthHi[0]);
#endif
	  		
		// determine boldness-adjust 
		if(bBold) 
		  {
			if(iPointsize <= 8) iBoldAdjust = 1;
			else iBoldAdjust = 2;
		  }
		for(int i=0;i<iLenString;i++) 
		  {

			// parse font metrics table
			bCurrChar=(BYTE)pTextString[i];
			Subst_Char(bCurrChar);
			
			if(bCurrChar > 32 && bCurrChar < 128) {
				// general ASCII chars 33..127
				// note: this does NOT include {space} (32)
				iCalcWidth += *(pFntWidthLo+bCurrChar-32) + iBoldAdjust;
			}
			else if(bCurrChar == 32) {
				// {space} is handled separately because it is not algor. bolded
				iCalcWidth += *(pFntWidthLo+bCurrChar-32);
			}
			else if(bCurrChar > 159) {
				// extended chars 160..255
				iCalcWidth += *(pFntWidthHi+bCurrChar-160) + iBoldAdjust;
			}
			else {
				// plan on substituting a block (127) for unsupported chars
				iCalcWidth += *(pFntWidthLo+95) + iBoldAdjust; // was 95 for char127
			}
		}

	} // proportional font
	else {
		// calculate width of fixed font
		//
		// char width is determined by pitch and calculated into printer units
		iCalcWidth = iLenString * PrinterHRes / iPitch;
		if((iLenString * PrinterHRes) % iPitch != 0) 
			iCalcWidth++;
	} // fixed font

	// calculate height
	//
	// text height (in pixels) = height (in pts) * DPI(300) / Points-per-inch(72) 
	iCalcHeight = iPointsize * PrinterVRes/72;

	// return height & width
	iHeight=iCalcHeight;
	iWidth=iCalcWidth;

	return NO_ERROR;

} // GetTextExtent


// REVISIT:  this function is directly tied to the GenericMapper values
// but could be logically tied together better.
// This function maps the widths of some characters relative to other 'known' characters.
// The values are not widths per se', but values of these other characters to use as reference.
void Font::Subst_Char(int& bCurrChar) const
{
	switch(bCurrChar) 
	 {
// REVISIT this entire width subst mechanism - it doesn't readily handle non-latin1 or multiple chars
		case 128: bCurrChar=35; break;
		case 129: bCurrChar=35; break;
		case 130: bCurrChar=44; break;
		case 131: bCurrChar=35; break;
		case 132: bCurrChar=127; break;
		case 133: bCurrChar=127; break;
		case 134: bCurrChar=127; break;
		case 135: bCurrChar=127; break;
		case 136: bCurrChar=94; break;
		case 137: bCurrChar=37; break;
		case 138: bCurrChar=83; break;
		case 139: bCurrChar=60; break;
		case 140: bCurrChar=127; break;
		case 145: bCurrChar=39; break;
		case 146: bCurrChar=39; break;
		case 147: bCurrChar=34; break;
		case 148: bCurrChar=34; break;
		case 149: bCurrChar=164; break;
		case 150: bCurrChar=173; break;
		case 151: bCurrChar=127; break;
		case 152: bCurrChar=126; break;
		case 153: bCurrChar=127; break;
		case 154: bCurrChar=115; break;
		case 155: bCurrChar=62; break;
		case 156: bCurrChar=127; break;
		case 159: bCurrChar=255; break;
	} // switch(bCurrChar) substitution
		
}

// this function should be pure virtual, but class isn't abstract
// so just return null
Font* Font::CharSetClone(char* /*NewCharSet*/) const
{ return NULL; }


#endif
