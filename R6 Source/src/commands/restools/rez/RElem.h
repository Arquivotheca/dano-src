/*	$Id: RElem.h,v 1.3 1999/02/03 08:39:41 maarten Exp $
	
	Copyright 1996, 1997, 1998
	        Hekkelman Programmatuur B.V.  All rights reserved.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution.
	3. All advertising materials mentioning features or use of this software
	   must display the following acknowledgement:
	   
	    This product includes software developed by Hekkelman Programmatuur B.V.
	
	4. The name of Hekkelman Programmatuur B.V. may not be used to endorse or
	   promote products derived from this software without specific prior
	   written permission.
	
	THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
	FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
	AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
	EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
	OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
	OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
	ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 	

	Created: 12/02/98 15:36:15
*/

#ifndef RELEM_H
#define RELEM_H

#include <DataIO.h>

enum RELType {
	relEval, relString, relFill
};

struct REval;
class RState;

struct RElem {
	RElem *fNext;
	char *fLabel;
	RELType fType;
	int fSize;				// in bits
	int fOffset;			// in bits
	RState *fState;
	union {
		REval *fValue;
		char *fString;
	};
	
	RElem(REval *v, int size, int offset, RState *state);
	RElem(const char *s, int size, int offset, RState *state);
	RElem(int size, int offset, RState *state);
	
	void Write();
	
	int FindIdentifier(int v);

private:
	void Write(RElem *head, BPositionIO& stream);
};

void RAddElement(RElem **head, REval *v, int size, RState *state);
void RAddElement(RElem **head, const char *s, int size, RState *state);

#endif
