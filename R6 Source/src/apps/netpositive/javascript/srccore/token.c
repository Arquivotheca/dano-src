/* token.c -  Code for reading or writing (to/from memory) okenized jse.
 */

/* (c) COPYRIGHT 1993-98           NOMBAS, INC.
 *                                 64 SALEM ST.
 *                                 MEDFORD, MA 02155  USA
 * 
 * ALL RIGHTS RESERVED
 * 
 * This software is the property of Nombas, Inc. and is furnished under
 * license by Nombas, Inc.; this software may be used only in accordance
 * with the terms of said license.  This copyright notice may not be removed,
 * modified or obliterated without the prior written permission of Nombas, Inc.
 * 
 * This software is a Trade Secret of Nombas, Inc.
 * 
 * This software may not be copied, transmitted, provided to or otherwise made
 * available to any other person, company, corporation or other entity except
 * as specified in the terms of said license.
 * 
 * No right, title, ownership or other interest in the software is hereby
 * granted or transferred.
 * 
 * The information contained herein is subject to change without notice and
 * should not be construed as a commitment by Nombas, Inc.
 */

#include "srccore.h"

#if ( defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC) ) \
 || ( defined(JSE_TOKENDST) && (0!=JSE_TOKENDST) )
/* *****************************************************************
 * ******************** TOKEN STRING STUFF *************************
 * ***************************************************************** */

   static void NEAR_CALL
tokenInit(struct Token *This)
{
   This->StringCount = 0;
   This->StringTable = jseMustMalloc(VarName,sizeof(VarName));
}

   static void NEAR_CALL
tokenTerm(struct Call *call,struct Token *this)
{
   uint i;
   for ( i = 0; i < this->StringCount; i++ ) {
      farRemoveFromStringTable(call,this->StringTable[i]);
   }
   jseMustFree(this->StringTable);
}

   uint NEAR_CALL
tokenFindString(struct Token *this,VarName str)
{
   uint i;

   assert( NULL != str );
   for ( i = 0; i < this->StringCount; i++ ) {
      if ( str == this->StringTable[i] )
         return i + 1;
   }
   return 0;   /* 0 indicates it wasn't found */
}

   void NEAR_CALL
tokenPutString(struct Token *this,VarName str)
{
   assert( NULL != str );
   assert( 0 == tokenFindString(this,str) );
   this->StringTable = jseMustReMalloc(VarName,this->StringTable,
                 (this->StringCount+1)*sizeof(this->StringTable[0]));
   this->StringTable[this->StringCount++] = str;
   AddStringUser(str);
}

/*******************************************************************
 ************************* JSE_TOKENSRC ****************************
 ****************************************************************** */
#if defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC)

   static void NEAR_CALL
tokensrcInit(struct TokenSrc *This)
{
   This->TokenMemSize = 0;
   This->TokenMem = jseMustMalloc(void,1);
   tokenInit(&(This->token));
}

   static void NEAR_CALL
tokensrcTerm(struct Call *call,struct TokenSrc *This)
{
   tokenTerm(call,&(This->token));
   if( This->TokenMem!=NULL )
      jseMustFree(This->TokenMem);
}

   void *
CompileIntoTokens(struct Call *call,
                  const jsechar *CommandString,jsebool FileSpec
                  /*else CommandString is text*/,
                  uint *BufferLen)
{
   void * TokenBuf = NULL;
   jsechar * Source = StrCpyMalloc(CommandString);

   if ( CompileFromText(call,&Source,FileSpec) )
   {
      struct TokenSrc tSrc;
      tokensrcInit(&tSrc);
#     if defined(JSE_LINK) && (0!=JSE_LINK)
         assert( NULL != call->session.ExtensionLib );
         extensionTokenWrite(call->session.ExtensionLib,call,&tSrc);
#     else
         tokenWriteByte(&tSrc,0); /* store no extension libraries */
#     endif

      /* write out all of the local functions */
      tokenWriteAllLocalFunctions(&tSrc,call);

      /* end with code to show we're all finished */
      tokenWriteCode(&tSrc,(uword8)ALL_DONE_BYE_BYE);

      TokenBuf = tSrc.TokenMem;
      tSrc.TokenMem = NULL;

      assert( NULL != TokenBuf );
      *BufferLen = tSrc.TokenMemSize;
      tokensrcTerm(call,&tSrc);
   } /* endif */
   jseMustFree(Source);
   return TokenBuf;
}


   void
tokenWriteBuffer(struct TokenSrc *this,const void *buf,uint ByteCount)
{
   if ( 0 != ByteCount ) {
      this->TokenMem =
         jseMustReMalloc(void,this->TokenMem,ByteCount+this->TokenMemSize);
      memcpy((ubyte *)(this->TokenMem)+this->TokenMemSize,buf,ByteCount);
      this->TokenMemSize += ByteCount;
   } /* endif */
}

   void
tokenWriteByte(struct TokenSrc *this,uword8 data)
{
   tokenWriteBuffer(this,&data,1);
}

#if BIG_ENDIAN
   void NEAR_CALL
tokenWriteNumericDatum(struct TokenSrc *this,
                       void * datum,uint datumlen)
{
   uword8 buffer[20];
   int i;
   assert( datumlen < sizeof(buffer) );
   for ( i = 0; i < datumlen; i++ ) {
      buffer[i] = ((uword8 *)datum)[datumlen - i - 1];
   } /* endfor */
   tokenWriteBuffer(this,buffer,datumlen);
}
#endif  /* BIG_ENDIAN */

   void
tokenWriteNumber(struct TokenSrc *this,jsenumber n)
{
   /* try to write in a 32, 16, or 8-bit form if possible */
   sword32 n32 = (sword32)n;
   if ( (jsenumber)n32 == n ) {
      sword16 n16 = (sword16)n32;
      if ( (sword32)n16 == n32 ) {
         sword8 n8 = (sword8)n16;
         if ( (sword16)n8 == n16 ) {
            /* write as 8-bit number */
            tokenWriteByte(this,8);
            tokenWriteByte(this,(ubyte)n8);
         } else {
            /* write as 16-bit number */
            tokenWriteByte(this,16);
            tokenWriteNumericDatum(this,&n16,sizeof(n16));
         } /* endif */
      } else {
         /* write as 32-bit number */
         tokenWriteByte(this,32);
         tokenWriteNumericDatum(this,&n32,sizeof(n32));
      } /* endif */
   } else {
      /* cannot write as a shorter form; write the entire number */
      tokenWriteByte(this,0);
      tokenWriteNumericDatum(this,&n,sizeof(n));
   } /* endif */
}

   void
tokenWriteLong(struct TokenSrc *this,sword32 number)
{
   tokenWriteNumber(this, (jsenumber)number );
}

   void
tokenWriteString(struct Call *call,struct TokenSrc *this,VarName string)
{
   uint StringIndex = tokenFindString(&(this->token),string);
   if ( 0 == StringIndex )
   {
      stringLengthType len;
      const jsechar *str = farGetStringTableEntry(call,string,&len);
      /* to handle unicode/ascii translation, both system will print
       * number of pure ascii characters, followed by pure unicode,
       * followed by pure ascii, etc.. until len bytes written
       */
#     if defined(JSE_UNICODE) && (0!=JSE_UNICODE)
         stringLengthType i;
         for ( i = 0; i < len; i++ )
         {
            if ( 255 < str[i] )
               break;
         }
         if ( i < len )
         {
            /* this string contains some unicode */
            tokenWriteCode(this,(uword8)NEW_STRING_UNICODE);
            tokenWriteLong(this,len);
            for ( i = 0; i < len; i++ )
            {
               tokenWriteByte(this,str[len] & 0xff );
               tokenWriteByte(this,(str[len] >> 8) & 0xff );
            }
         }
         else
         {
            /* this string contains no unicode, save space by writing ascii */
            tokenWriteCode(this,(uword8)NEW_STRING_ASCII);
            tokenWriteLong(this,len);
            for ( i = 0; i < len; i++ )
            {
               tokenWriteByte(this,(uword8)(str[len]));
            }
         }
#     else
         /* unicode not supported.  Pure buffer write. */
         tokenWriteCode(this,(uword8)NEW_STRING_ASCII);
         tokenWriteLong(this,(slong)len);
         tokenWriteBuffer(this,str,(uint)len);
#     endif
      /* add string to our list of already-written strings */
      tokenPutString(&(this->token),string);
   } else {
      /* string is already in the table, so just write its index */
      tokenWriteCode(this,(uword8)OLD_STRING);
      tokenWriteLong(this,(slong)StringIndex);
   }
}

#endif

/* *****************************************************************
 * ************************ JSE_TOKENDST ***************************
 * ***************************************************************** */
#if defined(JSE_TOKENDST) && (0!=JSE_TOKENDST)

static void NEAR_CALL
tokendstInit(struct TokenDst *This,const void *mem)
{
   This->TokenMem = mem;
   tokenInit(&(This->token));
}

   static void NEAR_CALL
tokendstTerm(struct Call *call,struct TokenDst *This)
{
   tokenTerm(call,&(This->token));
}

   void
CompileFromTokens(struct Call *call,const void *CodeBuffer)
{
   struct TokenDst tDst;
   TokenCodes tc;

   tokendstInit(&tDst,CodeBuffer);
#  if defined(JSE_LINK) && (0!=JSE_LINK)
      assert( NULL != call->session.ExtensionLib );
      extensionTokenRead(call->session.ExtensionLib,call,&tDst);
#  else
      tokenReadByte(&tDst); /* ignore number of link libraries */
#  endif
   while ( NEW_FUNCTION_NAME == (tc = tokenReadCode(&tDst)) ) {
      localTokenRead(call,&tDst);
   }
   if ( ALL_DONE_BYE_BYE != tc ) {
      tokenFatalError();
   }
   tokendstTerm(call,&tDst);
}

   void
tokenFatalError()
{
/*   InstantDeath(TextCore::TOKEN_READ_FAILURE); */
   exit(1);
}


   void
tokenReadBuffer(struct TokenDst *this,void *buf,uint ByteCount)
{
   memcpy(buf,this->TokenMem,ByteCount);
   this->TokenMem = (ubyte *)(this->TokenMem) + ByteCount;
}


   uword8
tokenReadByte(struct TokenDst *this)
{
   uword8 b;
   tokenReadBuffer(this,&b,1);
   return b;
}


#if BIG_ENDIAN
   void NEAR_CALL
tokenReadNumericDatum(struct TokenDst *this,
                      void * datum,uint datumlen)
{
   uword8 buffer[20];
   int i;
   assert( datumlen < sizeof(buffer) );
   tokenReadBuffer(this,buffer,datumlen);
   for ( i = 0; i < datumlen; i++ ) {
      ((uword8 *)datum)[i] = buffer[datumlen - i - 1];
   } /* endfor */
}
#endif

   jsenumber
tokenReadNumber(struct TokenDst *this)
{
   /* first byte tells how many bits 8, 16, 32, or 0 for a full float */
   jsenumber n;
   uword8 ByteCount = tokenReadByte(this);
   assert( 0 == ByteCount  ||  8 == ByteCount  ||  16 == ByteCount  ||
           32 == ByteCount );
   switch ( ByteCount ) {
      case 8:
      {  sword8 n8 = (sword8) tokenReadByte(this);
         n = (jsenumber)n8;
      }  break;
      case 16:
      {  sword16 n16;
         tokenReadNumericDatum(this,&n16,sizeof(n16));
         n = (jsenumber)n16;
      }  break;
      case 32:
      {  sword32 n32;
         tokenReadNumericDatum(this,&n32,sizeof(n32));
         n = (jsenumber)n32;
      }  break;
      default:
         tokenReadNumericDatum(this,&n,sizeof(n));
         break;
   }
   return n;
}

   uword32
tokenReadLong(struct TokenDst *this)
{
   return (uword32)tokenReadNumber(this);
}


   VarName NEAR_CALL
tokenGetString(struct Token *this,uint index)
{
   assert(0 < index  &&  index <= this->StringCount);
   return this->StringTable[index-1];
}


   VarName
tokenReadString(struct Call *call,struct TokenDst *this)
{
   TokenCodes toke = tokenReadCode(this);
   VarName string;
   if ( OLD_STRING != toke )
   {
      stringLengthType len, i;
      jsechar *str;
      assert( NEW_STRING_ASCII ==toke  ||  NEW_STRING_UNICODE == toke );
      /* string not already in table, and so add it and get
       * the length and string
       */
      len = (stringLengthType)tokenReadLong(this);
      str = jseMustMalloc(jsechar,(size_t)len*sizeof(jsechar)+1/*so never 0*/);
      for ( i = 0; i < len; i++ )
      {
         jsechar c;
         if ( NEW_STRING_UNICODE == toke )
         {
            c = (jsechar) (tokenReadByte(this) | (tokenReadByte(this) << 8 ));
         }
         else
         {
            c = (jsechar)tokenReadByte(this);
         }
         str[i] = c;
      }
      /* enter this string into our string table */
      string = farEnterIntoStringTable(call,str,len);
      jseMustFree(str);
      /* add this to growing list of known strings */
      tokenPutString(&(this->token),string);
   } else {
      /* string is already in the table.
       * This is just its index into that table
       */
      assert( OLD_STRING == toke );
      string = (VarName)tokenReadLong(this);
   }
   return string;
}

#endif  /* defined(JSE_TOKENDST) */

#elif defined(__MWERKS__)
   #if __option(ANSI_strict)
      /* With ANSI_strict ON, empty files are errors, so this dummy variable is added */
      static ubyte DummyVariable;
   #endif /* ANSI_strict */
#endif 
