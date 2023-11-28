/* token.h -    Code for reading or writing (to/from memory) tokenized jse.
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

#if defined(__cplusplus)
   extern "C" {
#endif

#if !defined(_TOKEN_H) \
 && ( ( defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC) ) \
   || ( defined(JSE_TOKENDST) && (0!=JSE_TOKENDST) ) )
#define _TOKEN_H

typedef sword8 TokenCodes;
   /* all bind codes are less than zero */
#define   NEW_FUNCTION_NAME     -1
#define   ALL_DONE_BYE_BYE      -2
#define   NEW_STRING_ASCII      -3 /* no unicode characters */
#define   NEW_STRING_UNICODE    -4 /* will contain unicode characters */
   /* used to indicate new string in a table */
#define   OLD_STRING            -5  
   /* indicate using string already in the table */
#define   END_FUNC_OPCODES      -6   
   /* indicate last byte in an opcode list */

struct Token
{
   VarName *StringTable;
   uint StringCount;
};

uint NEAR_CALL tokenFindString(struct Token *token,VarName str);
   /* return 0 if not found, else index for string */
void NEAR_CALL tokenPutString(struct Token *token,VarName str);
   /* add string (assume not already there)
    * caller must have allocated; this will free
    */
VarName NEAR_CALL tokenGetString(struct Token *token,uint index);


/* token source */


#if defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC)
struct TokenSrc
{
   struct Token token;

   void *TokenMem;
   uint TokenMemSize;
};


void * CompileIntoTokens(struct Call *call,
                         const jsechar *CommandString,jsebool FileSpec
                         /*else CommandString is text*/,
                          uint *BufferLen);
void tokenWriteAllLocalFunctions(struct TokenSrc *token_src,struct Call *call);
void tokenWriteBuffer(struct TokenSrc *token_src,const void *buf,
                      uint ByteCount);
void tokenWriteByte(struct TokenSrc *token_src,uword8 data);
void tokenWriteLong(struct TokenSrc *token_src,sword32 number);
void tokenWriteNumber(struct TokenSrc *token_src,jsenumber n);
void tokenWriteString(struct Call *call,struct TokenSrc *token_src,VarName string);

#define tokenWriteCode(TOKE,CODE) tokenWriteByte(TOKE,CODE)

#if !BIG_ENDIAN
   #define tokenWriteNumericDatum tokenWriteBuffer
#else
   void NEAR_CALL tokenWriteNumericDatum(struct TokenSrc *token_src,
                                         void * datum,uint datumlen);
#endif

#endif

#if defined(JSE_TOKENDST) && (0!=JSE_TOKENDST)

struct TokenDst
{
   struct Token token;
   const void *TokenMem;
};


void CompileFromTokens(struct Call *call,const void *CodeBuffer);
void tokenReadBuffer(struct TokenDst *token_dst,void *buf,uint ByteCount);
uword8 tokenReadByte(struct TokenDst *);
uword32 tokenReadLong(struct TokenDst *);
jsenumber tokenReadNumber(struct TokenDst *);
VarName tokenReadString(struct Call *call,struct TokenDst *);
   /* will have been allocated when returned; DO NOT FREE */
void tokenFatalError(void);
   /* show error; doesn't return */


#define tokenReadCode(this) ((TokenCodes)tokenReadByte(this))

#if !BIG_ENDIAN
   #define tokenReadNumericDatum tokenReadBuffer
#else
   void NEAR_CALL tokenReadNumericDatum(struct TokenDst *token_dst,
                                        void * datum,uint datumlen);
#endif

#endif
#endif

#if defined(__cplusplus)
   }
#endif
