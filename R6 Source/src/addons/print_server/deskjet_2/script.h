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


class Scripter
{
public:
    Scripter(SystemServices* pSS);
    virtual ~Scripter();

    unsigned int TokenCount;
    unsigned int ReplayTokenCount;

    FILE* ScriptFile;
    char ScriptFileName[200];
    int buffsize; 
    unsigned int fontcount;

    SystemServices* pSys;
    short* GlobalBuffer;


    virtual BOOL OpenDebugStreamR(const char* filename)=0;
    virtual BOOL CloseDebugStreamR()=0;
    virtual BOOL OpenDebugStreamW(const char* filename)=0;
    virtual BOOL CloseDebugStreamW()=0;
    virtual BOOL PutDebugToken(const int token)=0;
    virtual BOOL PutDebugInt(const int data)=0;
    virtual BOOL PutDebugByte(const BYTE data)=0;
    virtual BOOL PutDebugStream(const BYTE* stream,const int len)=0;
    virtual BOOL PutDebugString(const char* str,const int len)=0;
    virtual BOOL GetDebugToken(int& token)=0;
    virtual BOOL GetDebugInt(int& data)=0;
    virtual BOOL GetDebugString(char*& str,int& len)=0;
    virtual BOOL GetDebugStream(const unsigned int buffersize, BYTE*& buffer)=0;
    virtual BOOL GetDebugByte(BYTE& data)=0;

    BOOL ParseVer(char* str);
};

class AsciiScripter : public Scripter
{
public:
    AsciiScripter(SystemServices* pSS);
    ~AsciiScripter();

    virtual BOOL OpenDebugStreamR(const char* filename);
    virtual BOOL CloseDebugStreamR();
    virtual BOOL OpenDebugStreamW(const char* filename);
    virtual BOOL CloseDebugStreamW();
    virtual BOOL PutDebugToken(const int token);
    virtual BOOL PutDebugInt(const int data);
    virtual BOOL PutDebugByte(const BYTE data);
    virtual BOOL PutDebugString(const char* str,const int len);
    virtual BOOL PutDebugStream(const BYTE* stream,const int len);
    virtual BOOL GetDebugToken(int& token);
    virtual BOOL GetDebugInt(int& data);
    virtual BOOL GetDebugByte(BYTE& data);
    virtual BOOL GetDebugString(char*& str,int& len);
    virtual BOOL GetDebugStream(const unsigned int buffersize, BYTE*& buffer );

    void ReadRLE(int instreamlen, BYTE* outstream);
    void ReadRaw(int instreamlen, BYTE* outstream);
    
    BOOL FindPercent();
    char* digits();
    char scanner[30];
#define TEMPLEN 200
    char tempStr[TEMPLEN];

    char TokString[25][40]; // 25=# of tokens in harness.h; table set in ProtoServices cons.
	unsigned int TokCount[25];

};

class BinaryScripter : public AsciiScripter
{
public:
    BinaryScripter(SystemServices* pSS);
    ~BinaryScripter();

    BOOL OpenDebugStreamR(const char* filename);
    BOOL OpenDebugStreamW(const char* filename);
    BOOL PutDebugToken(const int token);
    BOOL PutDebugInt(const int data);
    BOOL PutDebugByte(const BYTE data);
    BOOL PutDebugString(const char* str,const int len);
    BOOL PutDebugStream(const BYTE* stream,const int len);
    BOOL GetDebugToken(int& token);
    BOOL GetDebugInt(int& data);
    BOOL GetDebugString(char*& str,int& len);
    BOOL GetDebugStream(const unsigned int buffersize, BYTE*& buffer );
    BOOL GetDebugByte(BYTE& data);

};


