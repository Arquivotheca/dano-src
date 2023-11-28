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

#ifndef FONT_H
#define FONT_H
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)


#ifdef _COURIER
// fixed-pitch, serif

extern BYTE CourierSizes[];

class Courier : public ReferenceFont
{
friend class Printer;
friend class ProtoDeskJet660;
friend class ProtoDeskJet690;
friend class ProtoDeskJet600;
friend class ProtoDeskJet890;
friend class ProtoDeskJet895;
friend class ProtoBroadway;
public:
	Courier(BYTE size=0, 
			BOOL bold=FALSE, BOOL italic=FALSE, BOOL underline=FALSE,
			TEXTCOLOR=BLACK_TEXT, unsigned int SizesAvailable=3);
    virtual ~Courier();

	
	BYTE GetPitch(const BYTE pointsize)const;
	const char* GetName() const { return sCourier; }
	BOOL IsBoldAllowed() const { return TRUE; }		
	BOOL IsItalicAllowed() const { return TRUE; }			
	BOOL IsUnderlineAllowed() const { return TRUE; }		
	virtual BOOL IsColorAllowed() const { return TRUE; }
	BOOL IsProportional() const { return FALSE; }
	BOOL HasSerif() const { return TRUE; }

	int Index() { return COURIER_INDEX; }

	BYTE unused;	// left for future use by clients

protected:
	Courier(const Courier& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline);
	int Ordinal(unsigned int pointsize)const;
	virtual BYTE* GetSizes() const {  return CourierSizes; }
	virtual Font* CharSetClone(char* NewCharSet) const
	 { Courier* c = new Courier(*this,iPointsize,eColor,bBold,bItalic,bUnderline);
		if (c==NULL) return (Font*)NULL;
		strcpy(c->charset, NewCharSet);
		return c;
	 }
};

#ifdef _DJ400

extern BYTE Courier400Sizes[];

class Courier400 : public Courier
{
friend class DeskJet400;
friend class ProtoDeskJet400;
public:
	Courier400(BYTE size=0, 
			BOOL bold=FALSE, BOOL italic=FALSE, BOOL underline=FALSE);

	BOOL IsColorAllowed() const { return FALSE; }


protected:
	Courier400(const Courier400& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline);
	BYTE* GetSizes() const { return Courier400Sizes; }
	Font* CharSetClone(char* NewCharSet) const
	 { Courier400* c = new Courier400(*this,iPointsize,eColor,bBold,bItalic,bUnderline);
		if (c==NULL) return (Font*)NULL;
		strcpy(c->charset, NewCharSet);
		return c;
	 }
};

#endif // _DJ400

#endif // _COURIER


#ifdef _CGTIMES
// proportional, serif

extern BYTE CGTimesSizes[];

class CGTimes : public ReferenceFont
{
friend class Printer;
friend class ProtoDeskJet660;
friend class ProtoDeskJet690;
friend class ProtoDeskJet600;
friend class ProtoDeskJet890;
friend class ProtoDeskJet895;
friend class ProtoBroadway;
public:
	CGTimes(BYTE size=0, 
			BOOL bold=FALSE, BOOL italic=FALSE, BOOL underline=FALSE,
			TEXTCOLOR=BLACK_TEXT, unsigned int SizesAvailable=5);
	
	const char* GetName() const { return sCGTimes; }
	BOOL IsBoldAllowed() const { return TRUE; }		
	BOOL IsItalicAllowed() const { return TRUE; }			
	BOOL IsUnderlineAllowed() const { return TRUE; }		
	virtual BOOL IsColorAllowed() const { return TRUE; }
	BOOL IsProportional() const { return TRUE; }
	BOOL HasSerif() const { return TRUE; }

	int Index() { return CGTIMES_INDEX; }

protected:
	CGTimes(const CGTimes& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline);
	int Ordinal(unsigned int pointsize)const;
	virtual BYTE* GetSizes() const { return CGTimesSizes; }
	virtual Font* CharSetClone(char* NewCharSet) const
	 { CGTimes* c = new CGTimes(*this,iPointsize,eColor,bBold,bItalic,bUnderline);
		if (c==NULL) return (Font*)NULL;
		strcpy(c->charset, NewCharSet);
		return c;
	 }
};

#ifdef _DJ400

extern BYTE CGTimes400Sizes[];

class CGTimes400 : public CGTimes
{
friend class DeskJet400;
friend class ProtoDeskJet400;
public:
	CGTimes400(BYTE size=0,
			BOOL bold=FALSE, BOOL italic=FALSE, BOOL underline=FALSE);

	BOOL IsColorAllowed() const { return FALSE; }


protected:
	CGTimes400(const CGTimes400& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline);
	int Ordinal(unsigned int pointsize)const;
	BYTE* GetSizes() const { return CGTimes400Sizes; }
	Font* CharSetClone(char* NewCharSet) const
	 { CGTimes400* c = new CGTimes400(*this,iPointsize,eColor,bBold,bItalic,bUnderline);
		if (c==NULL) return (Font*)NULL;
		strcpy(c->charset, NewCharSet);
		return c;
	 }
};

#endif  // ifdef _DJ400

#endif  // ifdef _CGTIMES


#ifdef _LTRGOTHIC
// fixed-pitch, sans-serif

extern BYTE LetterGothicSizes[];

class LetterGothic : public ReferenceFont
{
friend class Printer;
friend class ProtoDeskJet660;
friend class ProtoDeskJet690;
friend class ProtoDeskJet600;
friend class ProtoDeskJet890;
friend class ProtoDeskJet895;
friend class ProtoBroadway;
public:
	LetterGothic(BYTE size=0, 
			BOOL bold=FALSE, BOOL italic=FALSE, BOOL underline=FALSE,
			TEXTCOLOR=BLACK_TEXT, unsigned int SizesAvailable=3);
    virtual ~LetterGothic();

	
	BYTE GetPitch(const BYTE pointsize)const;
	const char* GetName() const { return sLetterGothic; }
	BOOL IsBoldAllowed() const { return TRUE; }		
	BOOL IsItalicAllowed() const { return TRUE; }			
	BOOL IsUnderlineAllowed() const { return TRUE; }		
	virtual BOOL IsColorAllowed() const { return TRUE; }
	BOOL IsProportional() const { return FALSE; }
	BOOL HasSerif() const { return FALSE; }

	int Index() { return LETTERGOTHIC_INDEX; }

	BYTE unused;	// left for future use by clients

protected:
	LetterGothic(const LetterGothic& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline);
	int Ordinal(unsigned int pointsize)const;
	virtual BYTE* GetSizes() const {  return LetterGothicSizes; }
	virtual Font* CharSetClone(char* NewCharSet) const
	 { LetterGothic* c = new LetterGothic(*this,iPointsize,eColor,bBold,bItalic,bUnderline);
		if (c==NULL) return (Font*)NULL;
		strcpy(c->charset, NewCharSet);
		return c;
	 }
};

#ifdef _DJ400

extern BYTE LetterGothic400Sizes[];

class LetterGothic400 : public LetterGothic
{
friend class DeskJet400;
friend class ProtoDeskJet400;
public:
	LetterGothic400(BYTE size=0, 
			BOOL bold=FALSE, BOOL italic=FALSE, BOOL underline=FALSE);

	BOOL IsColorAllowed() const { return FALSE; }


protected:
	LetterGothic400(const LetterGothic400& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline);
	BYTE* GetSizes() const { return LetterGothic400Sizes; }
	Font* CharSetClone(char* NewCharSet) const
	 { LetterGothic400* c = new LetterGothic400(*this,iPointsize,eColor,bBold,bItalic,bUnderline);
		if (c==NULL) return (Font*)NULL;
		strcpy(c->charset, NewCharSet);
		return c;
	 }
};

#endif // _DJ400

#endif  // _LTRGOTHIC


#ifdef _UNIVERS
// proportional, sans-serif

extern BYTE UniversSizes[];

class Univers : public ReferenceFont
{
friend class Printer;
friend class ProtoDeskJet660;
friend class ProtoDeskJet690;
friend class ProtoDeskJet600;
friend class ProtoDeskJet890;
friend class ProtoDeskJet895;
friend class ProtoBroadway;
public:
	Univers(BYTE size=0, 
			BOOL bold=FALSE, BOOL italic=FALSE, BOOL underline=FALSE,
			TEXTCOLOR=BLACK_TEXT, unsigned int SizesAvailable=3);
	
	const char* GetName() const { return sUnivers; }
	BOOL IsBoldAllowed() const { return TRUE; }		
	BOOL IsItalicAllowed() const { return TRUE; }			
	BOOL IsUnderlineAllowed() const { return TRUE; }		
	virtual BOOL IsColorAllowed() const { return TRUE; }
	BOOL IsProportional() const { return TRUE; }
	BOOL HasSerif() const { return FALSE; }

	int Index() { return UNIVERS_INDEX; }

protected:
	Univers(const Univers& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline);
	int Ordinal(unsigned int pointsize)const;
	virtual BYTE* GetSizes() const { return UniversSizes; }
	virtual Font* CharSetClone(char* NewCharSet) const
	 { Univers* c = new Univers(*this,iPointsize,eColor,bBold,bItalic,bUnderline);
		if (c==NULL) return (Font*)NULL;
		strcpy(c->charset, NewCharSet);
		return c;
	 }
};

#endif  // _UNIVERS


#endif  // if any fonts
#endif  // ifndef FONT_H

