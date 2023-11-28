/* define.c     Handle all the #define statements coming
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

#if defined(JSE_DEFINE) && (0!=JSE_DEFINE)

struct Define * defineNew(struct Define *pParent)
{
   struct Define *this = jseMalloc(struct Define,sizeof(struct Define));

   if( this != NULL )
   {
      this->RecentLink = NULL;
      this->Parent = pParent;
   }

   return this;
}

void defineDelete(struct Define *this)
{
   struct Link *link = this->RecentLink;

   while ( NULL != link )
   {
      struct Link *prev = link->Prev;

      jseMustFree(link->Find);
      jseMustFree(link->Replace);
      jseMustFree(link);
      link = prev;
   }

   jseMustFree(this);
}


void defineAddInt(struct Define *this,const jsechar *FindString,slong l)
{
   jsechar buf[50];
   long_to_string(l,buf);
   defineAdd(this,FindString,buf);
}


#if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
void defineAddFloat(struct Define *this,const jsechar *FindString,jsenumber f)
{
   jsechar buf[50];
   jse_sprintf(buf,UNISTR("%f"),f);
   defineAdd(this,FindString,buf);
}
#endif

void defineAdd(struct Define *this,const jsechar *FindString,
               const jsechar *ReplaceString)
{
   assert( NULL != FindString  &&  NULL != ReplaceString );
   defineAddLen(this,FindString,(int)strlen_jsechar(FindString),ReplaceString,
                (int)strlen_jsechar(ReplaceString));
}

void defineAddLen(struct Define *this,const jsechar *FindString,int FindStringLen,
                  const jsechar *ReplaceString,int ReplaceStringLen)
{
   struct Link **plink;
   struct Link *NewLink;

   assert( NULL != FindString  &&  NULL != ReplaceString );
   assert( 0 != *FindString );

   /* look to see if there is already a link in the current context.
    * if there is then remove it
    */
   for ( plink = &(this->RecentLink); NULL != *plink;
         plink = &((*plink)->Prev) )
   {
      if ( 0 == memcmp((void*)FindString,(*plink)->Find,sizeof(jsechar)*FindStringLen)
        && 0 == (*plink)->Find[FindStringLen] )
      {
         /* this link already exists, and so delete this old one */
         struct Link *OldLink = *plink;

         jseMustFree((*plink)->Find);
         jseMustFree((*plink)->Replace);
         *plink = OldLink->Prev;
         jseMustFree(OldLink);
         break;
      }
   }

   /* add this new value as the most recent link */
   NewLink = jseMustMalloc(struct Link,sizeof(struct Link));
   NewLink->Find = StrCpyMallocLen(FindString,(size_t)FindStringLen);
   NewLink->Replace = StrCpyMallocLen(ReplaceString,(size_t)ReplaceStringLen);
   NewLink->Prev = this->RecentLink;
   this->RecentLink = NewLink;
}


jsebool defineProcessSourceStatement(struct Source **source,struct Call *call)
{
   jsechar *src = sourceGetPtr(*source);
   jsechar *end = src + strlen_jsechar(src);
   int in_string = 0;

   /* replace all commented areas on this line with spaces, so that comments
    * don't confuse our parsing. Do not do this if within a string.
    */

   jsebool WithinComment = False;
   jsechar *BeginComment = NULL;
   jsechar *c;

   for ( c = src; 0 != *c; c++ ) {
      if( *c=='"' ) in_string = 1 - in_string;
      if ( !in_string && !WithinComment )
      {
         if ( '/' == *c )
         {
            if ( '/' == c[1] ) {
               /* all the rest of the line is comment, and so end it here */
               end = c;
               break;
            } else if ( '*' == c[1] ) {
               BeginComment = c;
               WithinComment = True;
               c++; /* so won't see the '*' as a possible end-comment */
            }
         }
      } else {
         if ( '*' == *c  &&  '/' == c[1] )
         {
            /* end of comment, so compact comment into a single space */
            *BeginComment = ' ';
            memmove(BeginComment+1,c+2,sizeof(jsechar)*(strlen_jsechar(c+2)+1));
            c = BeginComment;
            end = c + strlen_jsechar(c);
            WithinComment = False;
         }
      }
   }
   if ( WithinComment )
   {
      /* if ended within comment, then stop where that comment begins */
      end = BeginComment;
   }

   sourceSetPtr(*source,end); /* will continue parsing from here */

   /* remove whitespace from beginning of line */
   SKIP_WHITESPACE(src);

   /* remove whitespace from end */
   while ( IS_WHITESPACE(end[-1])  &&  src < end )
   {
      end--;
   }

   assert( src <= end );
   if ( src < end )
   {
      jsechar *Find = src;
      int FindLen;
      jsechar *Replace;
      int ReplaceLen;

      assert( 0 != *src );
      /* get Find label, which is text up to whitespace */
      while ( ++src < end  &&  !IS_WHITESPACE(*src) ) ;
      FindLen = (int)(src - Find);
      /* get the replacement string */
      assert( src <= end );
      Replace = UNISTR(" "); ReplaceLen = 1; /* default in case nothing found */
      while( src < end  &&  IS_WHITESPACE(*src) ) { src++; }
      assert( src <= end );
      if ( src < end )
      {
         /* get replacement string, which is string up to end */
         Replace = src;
         ReplaceLen = (int)(end - src);
      }
      defineAddLen(call->session.Definitions,Find,FindLen,Replace,ReplaceLen);
   }
   return True;
}

JSECALLSEQ( void ) jsePreDefineLong(jseContext jsecontext,
                                    const jsechar *FindString,
                                    slong ReplaceL)
{
   JSE_API_STRING(ThisFuncName,UNISTR("jsePreDefineLong"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
   JSE_API_ASSERT_(FindString,2,ThisFuncName,return);
   
   defineAddInt(jsecontext->session.Definitions,FindString,ReplaceL);
}

#if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
   JSECALLSEQ( void )
jsePreDefineNumber(jseContext jsecontext,const jsechar *findString,
                   jsenumber replaceL)
{
   JSE_API_STRING(ThisFuncName,UNISTR("jsePreDefineNumber"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
   JSE_API_ASSERT_(findString,2,ThisFuncName,return);

   defineAddFloat(jsecontext->session.Definitions,findString,replaceL);
}
#endif

   JSECALLSEQ( void )
jsePreDefineString(jseContext jsecontext,const jsechar *FindString,
                   const jsechar *ReplaceString)
{
   JSE_API_STRING(ThisFuncName,UNISTR("jsePreDefineString"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
   JSE_API_ASSERT_(FindString,2,ThisFuncName,return);
   JSE_API_ASSERT_(ReplaceString,3,ThisFuncName,return);

   defineAdd(jsecontext->session.Definitions,FindString,ReplaceString);
}


/* search this context, and parent contexts for replacement string */
const jsechar * defineFindReplacement(struct Define *this,const jsechar *FindString)
{
   struct Define *define = this;
   do {
      struct Link *link;

      assert( NULL != define );
      for ( link = define->RecentLink; NULL != link; link = link->Prev )
      {
         if ( !strcmp_jsechar(FindString,link->Find) )
         {
            return link->Replace;
         }
      }
   } while ( NULL != (define = define->Parent) );
   return NULL;
}

#endif
