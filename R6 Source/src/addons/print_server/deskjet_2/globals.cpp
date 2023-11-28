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

#ifdef PROTO
#include "../include/Header.h"
#else
#include "Header.h"
#endif
// Functions used in Translator

/////////////////////////////////////////////////////////////////////////
int stringlen(const char*s)
// may not be in some systems
{ int c=0;
  while (s[c++]) ;
  return c-1;
}
// utilities to save overhead for copying escape sequences

// notice that the input is assumed to be zero-terminated, but the results
// generated are not, in keeping with PCL

BYTE EscCopy(BYTE *dest, const char *s,int num, char end)
{ 
  dest[0]=ESC; strcpy((char*)&dest[1],s); 
  BYTE k = stringlen(s)+1;
  BYTE i = sprintf((char *)&dest[k],"%d",num);
  dest[k+i] = end;
  return (k+i+1);
}
BYTE EscAmplCopy(BYTE *dest, int num, char end)
{
  dest[0]=ESC; dest[1]='&';dest[2]='l';
  BYTE i = sprintf((char *)&dest[3],"%d",num);
  dest[3+i] = end;
  return (4+i);
}

MediaSize PaperToMediaSize(PAPER_SIZE psize)
{
	switch(psize) 
	{ 
	case LETTER:        return sizeUSLetter;	break; 
	case LEGAL:		    return sizeUSLegal;		break; 
	case A4:		    return sizeA4;			break;
    case PHOTO_SIZE:    return sizePhoto;       break;
	default:		    return sizeUSLetter;	break; 
	} 
}

PAPER_SIZE MediaSizeToPaper(MediaSize msize)
{
	switch(msize)
	{
	case  sizeUSLetter:	return LETTER;	    break;
	case sizeUSLegal:	return LEGAL;	    break;
	case sizeA4:		return A4;		    break;
    case sizePhoto:     return PHOTO_SIZE;  break;
	default:			return UNSUPPORTED_SIZE; break;
	}
}
