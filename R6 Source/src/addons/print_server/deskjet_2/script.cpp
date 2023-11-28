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

#include "debug.h"

#if defined(CAPTURE) || defined (PROTO)

#ifdef PROTO
#include "../include/Header.h"
#else
#include "Header.h"
#endif
#include "script.h"

enum OPEN_PROP { OPEN, PROP };

#ifdef PROTO
extern int argProprietary;      // lives with harness code
#else
int argProprietary;             // just here for linker -- referencing code in ParseVer
                                //              is only for Replay under test harness
#endif

extern char* Version(int bCompressed);

Scripter::Scripter(SystemServices* pSS)
: pSys(pSS), GlobalBuffer(NULL), buffsize(0)
{ }
Scripter::~Scripter()
{ }

AsciiScripter::AsciiScripter(SystemServices* pSS)
: Scripter(pSS)
{ }
AsciiScripter::~AsciiScripter()
{ }

BinaryScripter::BinaryScripter(SystemServices* pSS)
: AsciiScripter(pSS)
{ }
BinaryScripter::~BinaryScripter()
{ }

DRIVER_ERROR SystemServices::InitScript(const char* FileName, BOOL ascii, BOOL read)
{
    if (ascii)
        pScripter = new AsciiScripter(this);
    else pScripter = new BinaryScripter(this);

    if (read)
    {
        replay=TRUE;
        if (pScripter->OpenDebugStreamR(FileName))
            return SYSTEM_ERROR;
        else return NO_ERROR;
    }
    else
    {
        replay=FALSE;
        if (pScripter->OpenDebugStreamW(FileName))
                return SYSTEM_ERROR;
            else return NO_ERROR;
    }

}   

DRIVER_ERROR SystemServices::EndScript()
{
    if (pScripter==NULL)
        return SYSTEM_ERROR;

    if (replay)
    {
        if (pScripter->CloseDebugStreamR())
            return SYSTEM_ERROR;
        else return NO_ERROR;
    }
    else
    {
        if (pScripter->CloseDebugStreamW())
            return SYSTEM_ERROR;
        else return NO_ERROR;
    }

    delete pScripter;
}


BOOL Scripter::ParseVer(char* str)
// set argProprietary
{
// example:
//"[platform-specific information]!!002.001_09-12-00 prop debug little_endian fonts:CTLU DJ400 DJ540 DJ600 DJ6xx DJ6xxPhoto DJ8xx DJ9xx Aladdin DJ630 eprinter"

    char c; 
    char ver[5];
    int i=0;
    c=str[i];
	while ((c!='!') && (c!=EOF))
		c=str[++i];
    if (c!='!')
        return TRUE;
    while ((c!=' ') && (c!=EOF))
		c=str[++i];
    if (c!=' ')
        return TRUE;

    for (int j=0; j<4; j++)
        ver[j]=str[++i];
    ver[4]='\0';

    if (!strcmp(ver,"open"))
        argProprietary=OPEN;
    else if (!strcmp(ver,"prop"))
            argProprietary=PROP;
          else return TRUE;
 return FALSE;
}

BOOL AsciiScripter::OpenDebugStreamR(const char* FileName)
{ 
    int x;
    char *str;

    strcpy(ScriptFileName,FileName);
	ScriptFile = fopen(ScriptFileName, "ra");

	if (ScriptFile == NULL)
	    return TRUE;
    
    char c=fgetc(ScriptFile);
	fseek(ScriptFile,0, 0);

    if (c==' ')             // spaces were never overwritten with tokencount
    {
        x = 2000;       // arbitrary guess
        
    }
    else if (GetDebugInt(x))
            return TRUE;

    TokenCount=x;
    ReplayTokenCount=0;
    fontcount=0;

    if (GetDebugString(str,x))      // get $Version
        return TRUE;

    if (ParseVer(str))
        return TRUE;

  return FALSE;
}

BOOL AsciiScripter::CloseDebugStreamR()
{           
    if (GlobalBuffer != NULL)
        pSys->FreeMem((BYTE*)GlobalBuffer);

    if (ScriptFile==NULL)
        return TRUE; 
    return fclose(ScriptFile);
}
#include <errno.h>
#include <strings.h>
BOOL AsciiScripter::OpenDebugStreamW(const char* FileName)
{ 
    strcpy(ScriptFileName,FileName);
    ScriptFile = fopen(ScriptFileName, "wa");
printf("ascii: %s : %s\n", ScriptFileName, strerror(errno));

	if (ScriptFile == NULL)
	    return TRUE;

    pSys->Capturing=TRUE; 
    TokenCount=0;
    fontcount=0; 

    // set up debugging token strings
    strcpy(TokString[0],"Job constructor");
    strcpy(TokString[1],"Job destructor");
    strcpy(TokString[2],"NewPage");
    strcpy(TokString[3],"SendRasters");
    strcpy(TokString[4],"TextOut");
    strcpy(TokString[5],"UNUSED1");
    strcpy(TokString[6],"Font destructor");
    strcpy(TokString[7],"PrintContext constructor");
    strcpy(TokString[8],"PrintContext destructor");
    strcpy(TokString[9],"SetPixelsPerRow");
    strcpy(TokString[10],"RealizeFont");
    strcpy(TokString[11],"SelectDevice");
    strcpy(TokString[12],"SelectPrintMode");
    strcpy(TokString[13],"SetPaperSize");
    strcpy(TokString[14],"UseBlackOnly");
    strcpy(TokString[15],"UseColor");
    strcpy(TokString[16],"SystemServices destructor");
    strcpy(TokString[17],"Scaler constructor");
    strcpy(TokString[18],"Scaler destructor");
    strcpy(TokString[19],"Scale::Process");
    strcpy(TokString[20],"String");
    strcpy(TokString[21],"RLE stream");
    strcpy(TokString[22],"RAW stream");
    strcpy(TokString[23],"stream token");
    strcpy(TokString[24],"NULL token");

	for (int i=0; i<25; i++)
		TokCount[i]=0;

    fprintf(ScriptFile,"      ");

    char version[TEMPLEN];
    sprintf(version, Version(FALSE) );
    int len = strlen(version);
       
    if (PutDebugString(version, len))
        return TRUE;        
   

    return FALSE;
}

BOOL AsciiScripter::CloseDebugStreamW()
{ 
    if (ScriptFile==NULL)
        return TRUE; 

	if (GlobalBuffer != NULL)
        pSys->FreeMem((BYTE*)GlobalBuffer);

    fseek(ScriptFile,0, 0);
    PutDebugInt(TokenCount);

    pSys->Capturing=FALSE;


    return fclose(ScriptFile);
}


BOOL AsciiScripter::PutDebugToken(const int token)
{ 
    TokenCount++;
	TokCount[token] =  TokCount[token]+1;

    int res=fprintf(ScriptFile,"%%%0x ",token);
    if (res<1)
        return TRUE;

    res=fprintf(ScriptFile,"\n%s:%d\n",TokString[token], TokCount[token]);
	
    return (res < 1);   // bad if res<1
}


BOOL AsciiScripter::PutDebugInt(const int data)
{ 
    int res=fprintf(ScriptFile,"%%%0x ",data);
  return (res < 1);   // bad if res<1
}

BOOL AsciiScripter::PutDebugByte(const BYTE data)
{ return PutDebugInt(data); }

BOOL AsciiScripter::PutDebugString(const char* str,const int len)
{ 
   if (PutDebugToken(tokCharPtr))
       return TRUE;
   if (PutDebugInt(len))
       return TRUE;

   fputc('"',ScriptFile);
   for (int i=0; i < len; i++)
		fprintf(ScriptFile,"%c",str[i]);
   fputc('"',ScriptFile);

return FALSE;
}


BOOL AsciiScripter::PutDebugStream(const BYTE* str,const int len)
{ 
// Does modified run-length-encoding from stream.
// len=length of raw stream
    int total=1; short count,i,j; BYTE current,next,last;
    BOOL rle=TRUE;
    short chunkarray[3];
    
    if ((len==0) || (str==NULL))
    {
        PutDebugToken(tokRawStream);
	    fprintf(ScriptFile,"%%%0x ", len);
        return FALSE;
    }

    next=0;	// suppress stupid compiler warning
    int alloclen= len*2;		// in worst case each byte produces [count=1][byte]
    if (buffsize < alloclen)
      {
	    if (GlobalBuffer!=NULL)
		    pSys->FreeMem((BYTE*)GlobalBuffer);
	    GlobalBuffer = (short*)pSys->AllocMem(alloclen);
	    buffsize=alloclen;
      }

    short* buff = GlobalBuffer;
	    
    short *thebuff=buff; 
	    if (buff==NULL)
		    return TRUE;
    const BYTE *startstream=str;

    for (i=0;i<3;i++)	// go through 3 times, for r,g,b
    {
	    short* startbuff=buff;
	    const BYTE* stream=startstream+i;
	    total=1;
	    current = *stream; stream+=3;
	    while (total<(len/3))
	    {	
		    count=1;
		    while ((total<(len/3)) && (current == (next= *stream)) ) 
			    { count++; total++; stream+=3; }
		    if (total<(len/3))
			    { next= *stream; stream+=3; total++; }
		    buff[0]=count; buff[1]=current;
		    buff+=2;
		    last=current;
		    current=next;				
	    }
	    // get last item if it is a single
	    if (current != last)
	      { 	
		    buff[0]=count; buff[1]=current;
		    buff+=2;
	      }

	    int chunks=(buff-startbuff)/2;
	    if (chunks>(len/6))
	    { rle=FALSE; break; }
	    else chunkarray[i]=chunks;
    }

    if (!rle)
    {
		PutDebugToken(tokRawStream);
	    fprintf(ScriptFile,"%%%0x ", len);
	    for (i=0;i<len;i++)
		    fprintf(ScriptFile,"%%%0x ",startstream[i]);

    }
    else
    { total=0;
	  PutDebugToken(tokRLEstream);
      fprintf(ScriptFile,"%%%0x ",len);
      for (i=0;i<3;i++)
	    { count=chunkarray[i];
		    for (j=0;j<count;j++)
			    fprintf(ScriptFile,"%%%0x %%%0x ",thebuff[total+2*j],thebuff[total+2*j+1]);
	      total += count*2;
	    }
    }

return FALSE;
}
///////////////////////////////////////////////////////////////////
BOOL AsciiScripter::FindPercent()
// find our token '%'
// return TRUE iff EOF
{ BOOL done=FALSE; char c;
  c=fgetc(ScriptFile);
	while ((c!='%') && (c!=EOF))
		c=fgetc(ScriptFile);
return (c==EOF);
}


char* AsciiScripter::digits()
// includes filtering of CR/LF's
{
    char c;
    char *str=scanner;
    BOOL done=FALSE;

    while (!done)
    {
        c=fgetc(ScriptFile);
        if (((c>='0')&&(c<='9')) ||
            ((c>='a')&&(c<='f')) ||
            ((c>='A')&&(c<='F'))
           )
            *str++ = c;
        else if (c==' ')
                { done=TRUE; *str++=c; }
             else if ((c!=10)&&(c!=13))
                { scanner[0]=0; done=TRUE; }
    }
    *str=0;
    return scanner;
}







void AsciiScripter::ReadRLE(int instreamlen, BYTE* outstream)
// Does run-length-decoding from stream.
{ int count,total,i,j; BYTE b; int bi;
BYTE* buff=(BYTE*)pSys->AllocMem(instreamlen*2);
int colorsize=instreamlen/3;

// first fill up 3-part buffer with all reds,then all greens, all blues

for (i=0;i<3;i++)	// for each color
  {	total=0;
	  while (total<colorsize)		
	  // assumes correct input, so that total count=instreamlen/3
	  {
		if (FindPercent())
			break;
        sscanf(digits(),"%p ",&count);
		if (FindPercent())
			break;
		sscanf(digits(),"%p ",&bi);
		b= (BYTE)bi;
		for (j=0;j<count;j++)
			   buff[ total + (i*colorsize) + j ] = b;
		total+=count;
	  }
  }

// now interleave

for (i=0;i<(instreamlen/3);i++)
  {
	*outstream++ = buff[i];
	*outstream++ = buff[i+colorsize];
	*outstream++ = buff[i+(2*colorsize)];
  }
pSys->FreeMem(buff);
}


void AsciiScripter::ReadRaw(int instreamlen, BYTE* outstream)
{ BYTE b; int bi;
	for (int i=0;i<instreamlen;i++)
	  { if (FindPercent())
			break;
		sscanf(digits(),"%p ",&bi); 
		b=(BYTE)bi;
		*outstream++ = b; }
}



///////////////////////////////////////////////////////////////////


BOOL AsciiScripter::GetDebugToken(int& token)
{
    char c;

	if (GetDebugInt(token))
        return TRUE;
    if ((token<0)||(token>LAST_TOKEN))
        return TRUE;
    
    // now expect LF<string>LF
    c=fgetc(ScriptFile);
    if (c!=10)
        return TRUE;
   
    while ( (c=fgetc(ScriptFile))!=10) ;
    // successfully ate debug string

    ReplayTokenCount++;

 return FALSE;
} 


BOOL AsciiScripter::GetDebugInt(int& data)
{ int res,k;
	
	if (FindPercent())
	  { data=-1; return TRUE; }    
 
    res = sscanf(digits(),"%p ",&k);
    if (!res)
		data = -1;
	else data=k;
 return FALSE;
}

BOOL AsciiScripter::GetDebugByte(BYTE& data)
{ 
    int temp;
    BOOL res=GetDebugInt(temp); 
    if (res)
        return TRUE;
    data=temp;
    return FALSE;
}


BOOL AsciiScripter::GetDebugString(char*& str,int& length)
{
	int res,len,i;
	str=NULL;	// in case of err rtn

	for (i=0; i<TEMPLEN; i++)
		tempStr[i]=0;

	GetDebugInt(res);	// look for tokCharPtr
	if (res != tokCharPtr)
		return TRUE;
	GetDebugInt(len);	// get length

	if (len >= TEMPLEN)
		return TRUE;

	char c=fgetc(ScriptFile);
	if (c!='"')
		return TRUE;
	i=0;
	tempStr[i]=fgetc(ScriptFile);
	while ((i<TEMPLEN) && (tempStr[i] != '"'))
		tempStr[++i]=fgetc(ScriptFile);

	if ((i==TEMPLEN) || (i!=len))
		return TRUE;

	tempStr[i]=0;
	str=tempStr;
	length=len;

    return FALSE;
}
  

BOOL AsciiScripter::GetDebugStream(const unsigned int buffersize, BYTE*& buffer)
{ 
	BYTE tok;
    int token;

	GetDebugToken(token);
	if (token == tokStream)
	{
		buffer = (BYTE*)pSys->AllocMem(buffersize);
		if (buffer==NULL)
			return TRUE;
		int len;

		GetDebugInt(len);
        if (len==0)
            return FALSE;
		if (((unsigned int)len) != buffersize)
			return TRUE;

		for (int i=0; i < len; i++)
		{
			GetDebugByte(tok);
			buffer[i] = tok;
		}		
	}
	else if ((token == tokRLEstream) || (token == tokRawStream))
		  {
			int len;
			
			GetDebugInt(len);
            if (len==0)
                buffer=NULL;
            else
            {
			    buffer = (BYTE*)pSys->AllocMem(len);
			    if (token == tokRLEstream)
				    ReadRLE(len, buffer);
			    else ReadRaw(len, buffer);
            }
		  }
        else return TRUE;
	
 return FALSE;
}


/////////////////////////////////////////////////////////////////////

BOOL BinaryScripter::OpenDebugStreamR(const char* FileName)
{ 
    strcpy(ScriptFileName,FileName);

	ScriptFile = fopen(ScriptFileName, "rb");

	if (ScriptFile == NULL)
	    return TRUE;
    
    int x;
    if (GetDebugInt(x))
        return TRUE;
    TokenCount=x;
    ReplayTokenCount=0;
    fontcount=0;

    char *ver;
    if (GetDebugString(ver,x))
        return TRUE;

    if (ParseVer(ver))
        return TRUE;

  return FALSE;
}

BOOL BinaryScripter::OpenDebugStreamW(const char* FileName)
{ 
    strcpy(ScriptFileName,FileName);
    ScriptFile = fopen(ScriptFileName, "wb");

	if (ScriptFile == NULL)
	    return TRUE;

    pSys->Capturing=TRUE; 
    TokenCount=0;
    fontcount=0;

    PutDebugInt(-1);  // leave space for token count

    char version[TEMPLEN];
    sprintf(version, Version(FALSE) );
    int len = strlen(version);
       
    if (PutDebugString(version, len))
        return TRUE;

    return FALSE;
}

BOOL BinaryScripter::PutDebugToken(const int token)
{ 
    TokenCount++;

   return PutDebugByte(token);
}


BOOL BinaryScripter::PutDebugInt(const int data)
{ 
    int res=fwrite( &data, sizeof(int), 1, ScriptFile );
  return (res == EOF);   
}

BOOL BinaryScripter::PutDebugByte(const BYTE data)
{ 
    int res=fputc( data, ScriptFile );
  return (res == EOF);   
}


BOOL BinaryScripter::PutDebugString(const char* str,const int len)
{ 
   if (PutDebugToken(tokCharPtr))
       return TRUE;
   if (PutDebugInt(len))
       return TRUE;
       
   for (int i=0; i < len; i++)
		if ((fputc(str[i],ScriptFile))==EOF)
            return TRUE;

return FALSE;
}

BOOL BinaryScripter::PutDebugStream(const BYTE* str,const int len)
{ 
   if (PutDebugToken(tokStream))
       return TRUE;
   if (PutDebugInt(len))
       return TRUE;
       
   for (int i=0; i < len; i++)
		if ((fputc(str[i],ScriptFile))==EOF)
            return TRUE;

return FALSE;
}

BOOL BinaryScripter::GetDebugToken(int& token)
{
    BYTE b;
	if (GetDebugByte(b))
        return TRUE;
    token=b;
    if (token>LAST_TOKEN)
        return TRUE;

    ReplayTokenCount++;

 return FALSE;
} 


BOOL BinaryScripter::GetDebugInt(int& data)
{ 
    int temp;
    int res = fread(&temp, sizeof(int), 1, ScriptFile);
    data=temp;
    return (res<1);
}

BOOL BinaryScripter::GetDebugByte(BYTE& data)
{ 
    data=fgetc(ScriptFile);
 return FALSE;
}


BOOL BinaryScripter::GetDebugString(char*& str,int& length)
{
	int res,len,i;
	str=NULL;	// in case of err rtn

	for (i=0; i<TEMPLEN; i++)
		tempStr[i]=0;

	GetDebugToken(res);	// look for tokCharPtr
	if (res != tokCharPtr)
		return TRUE;
	GetDebugInt(len);	// get length

	if (len >= TEMPLEN)
		return TRUE;

    for (i=0; i<len; i++)
        if ((tempStr[i]=fgetc(ScriptFile))==EOF)
            return TRUE;

	tempStr[i]=0;
	str=tempStr;
	length=len;

    return FALSE;
}
  

BOOL BinaryScripter::GetDebugStream(const unsigned int buffersize, BYTE*& buffer)
{ 
	int token;
    BYTE tok;

	GetDebugToken(token);
	if (token != tokStream)
        return TRUE;
	
	buffer = (BYTE*)pSys->AllocMem(buffersize);
	if (buffer==NULL)
		return TRUE;
	int len;

	GetDebugInt(len);
    if (len==0)
    {
        pSys->FreeMem(buffer);
        buffer=NULL;
        return FALSE;
    }
	if (((unsigned int)len) != buffersize)
		return TRUE;

	for (int i=0; i < len; i++)
	{
		GetDebugByte(tok);
		buffer[i] = tok;
	}		

	
 return FALSE;
}
 
#endif
