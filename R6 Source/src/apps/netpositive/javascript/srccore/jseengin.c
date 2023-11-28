/* jseengin.c   Initialize and terminate jse Engine
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

#if ((defined(JSE_MEM_DEBUG) && (0!=JSE_MEM_DEBUG))&& !defined(__JSE_LIB__)) || \
    (defined(JSE_ONE_STRING_TABLE) && (0!=JSE_ONE_STRING_TABLE))
   static VAR_DATA(sint) EngineThreadCount = 0;
#endif


#if ( 0 < JSE_API_ASSERTLEVEL )
# define ENGINE_NOT_INITIALIZED UNISTR("JSE Engine not initialized!")

static VAR_DATA(jsechar) jseApiErrorString[256] = ENGINE_NOT_INITIALIZED;

  static void NEAR_CALL
ClearApiError(void)
{
   memset(jseApiErrorString, 0, sizeof(jseApiErrorString));
}

/* These API calls are for diagnostic purposes.  They need never take
 * a context - that would defeat the purpose that you can get at an
 * error to see that your context is invalid, Null etc.  The calls do
 * not protect the error buffer from being written to by multiple threads.
 */
  JSECALLSEQ( const jsechar * )
jseGetLastApiError()
{
  return jseApiErrorString;
}

   void
SetLastApiError(const jsechar * formatS,...)
{
   va_list arglist;
   va_start(arglist,formatS);
   ClearApiError();
   jse_vsprintf(jseApiErrorString, formatS, arglist);
   va_end(arglist);
   assert( strlen_jsechar(jseApiErrorString) < (sizeof(jseApiErrorString)/sizeof(jsechar)) );
}

#if ( 1 == JSE_API_ASSERTNAMES )
   void
ApiParameterError(const jsechar *funcname,uint ParameterIndex)
{
   SetLastApiError(UNISTR("Parameter %d invalid to function: %s."),ParameterIndex,
                   funcname);
}
#else
   void
ApiParameterError(uint ParameterIndex)
{
   SetLastApiError("Parmeter %d invalid.",ParameterIndex);
}
#endif

  JSECALLSEQ( void )
jseClearApiError(void)
{
  ClearApiError();
}
#endif /* #if ( 0 < JSE_API_ASSERTLEVEL ) */


void InitializejseEngine(void)
{
#if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT) && \
   defined(__JSE_UNIX__)
   /* initialize the special_math global for UNIX */
#  if BIG_ENDIAN==True
      static CONST_DATA(uword32) orig_jse_special_math[10] = {
        0x00000000L, 0x00000000L, /* 0           */
        JSE_NEGNUM_BIT, 0x00000000L, /* -0          */
        JSE_INF_HIGH, JSE_NOTFINITE_LOW, /* infinity    */
        JSE_NEGNUM_BIT | JSE_INF_HIGH, JSE_NOTFINITE_LOW, /* -infinity   */
        JSE_NAN_HIGH, JSE_NOTFINITE_LOW  /* NaN         */
      };
#  else
      static CONST_DATA(uword32) orig_jse_special_math[10] = {
        0x00000000L, 0x00000000L, /* 0           */
        0x00000000L, JSE_NEGNUM_BIT, /* -0          */
        JSE_NOTFINITE_LOW, JSE_INF_HIGH, /* infinity    */
        JSE_NOTFINITE_LOW, JSE_NEGNUM_BIT | JSE_INF_HIGH, /* -infinity   */
        JSE_NOTFINITE_LOW, JSE_NAN_HIGH  /* NaN         */
      };
#  endif
   int x;
   for( x=0;x<5;x++ )
   {
      double *it = jse_special_math+x;
      uword32 *it2 = (uword32 *)it;
      *(it2++) = orig_jse_special_math[x*2];
      *(it2) = orig_jse_special_math[x*2+1];
   }
#endif

   #if defined(JSE_ONE_STRING_TABLE) && (0!=JSE_ONE_STRING_TABLE)
   if ( 0 == EngineThreadCount )
      allocateGlobalStringTable();
   #endif

   #if (defined(JSE_MEM_DEBUG) && (0!=JSE_MEM_DEBUG)) && !defined(__JSE_LIB__)
   if ( 0 == EngineThreadCount )
   {
      jseInitializeMallocDebugging();
      assert( 0 == jseMemReport(False) );
         /* anal check for no allocations yet */
   }
   #endif

#if ((defined(JSE_MEM_DEBUG) && (0!=JSE_MEM_DEBUG))&& !defined(__JSE_LIB__)) || \
    (defined(JSE_ONE_STRING_TABLE) && (0!=JSE_ONE_STRING_TABLE))
   EngineThreadCount++;
#endif

}

void TerminatejseEngine()
{
   #if (defined(JSE_MEM_DEBUG) && (0!=JSE_MEM_DEBUG)) && !defined(__JSE_LIB__)
      if ( 1 == EngineThreadCount )
         jseTerminateMallocDebugging();
   #endif

   #if defined(JSE_ONE_STRING_TABLE) && (0!=JSE_ONE_STRING_TABLE)
      if ( 1 == EngineThreadCount )
         freeGlobalStringTable();
   #endif
   
#if ((defined(JSE_MEM_DEBUG) && (0!=JSE_MEM_DEBUG))&& !defined(__JSE_LIB__)) || \
    (defined(JSE_ONE_STRING_TABLE) && (0!=JSE_ONE_STRING_TABLE))
   EngineThreadCount--;
   assert( EngineThreadCount >= 0 );
#endif

}

JSECALLSEQ(uint) jseInitializeEngine()
{
#  if defined(__JSE_LIB__)
      InitializejseEngine();
#  endif
#  if ( 0 < JSE_API_ASSERTLEVEL )
      ClearApiError();
#  endif
   return JSE_ENGINE_VERSION_ID;
}

   JSECALLSEQ(void)
jseTerminateEngine()
{
   #if defined(__JSE_LIB__)
      TerminatejseEngine();
   #endif
}


   JSECALLSEQ(jseContext)
jseInitializeExternalLink(void _FAR_ *LinkData,
   struct jseExternalLinkParameters * LinkParms,
   const jsechar * globalVarName,
   const char * AccessKey)
{
   struct Call * call;
   
   JSE_API_STRING(ThisFuncName,UNISTR("jseInitializeExternalLink"));
   
   JSE_API_ASSERT_(LinkParms,2,ThisFuncName,return NULL);
   JSE_API_ASSERT_(globalVarName,3,ThisFuncName,return NULL);
#  if ( 0 < JSE_API_ASSERTLEVEL )
      if ( NULL == LinkParms->PrintErrorFunc )
      {
         SetLastApiError(textcoreGet(textcorePRINTERROR_FUNC_REQUIRED));
         return NULL;
      }
#  endif
   
   call = callInitial(LinkData,LinkParms,globalVarName,strlen_jsechar(globalVarName));

   /* If user didnt supply required "options" then assert -
      If user not using dbg ver then thats their problem */
   #if defined(CHECK_FOR_VALID_USER_KEYS)
      if ( NULL != call  &&  !Verifivivication(AccessKey) ) {
         /* invalid key; Exit with a message and return NULL (and even don't
          * bother to clean up call, which may cause a far-segment call
          * problem in win16 anyway. Handle this error twice, just to make
          * sure the evaluator sees why this call doesn't work.
          */
         const jsechar * keystring = AccessKey ? 
               AsciiToUnicode((const char *)AccessKey) : (const jsechar *) textcorevtype_null;
#        if ( 0 < JSE_API_ASSERTLEVEL )          
            SetLastApiError(textcoreGet(textcoreINVALID_ACCESS_KEY),keystring);
#        endif
         callError(call,textcoreINVALID_ACCESS_KEY,keystring );
#        if defined(JSE_UNICODE) && (0!=JSE_UNICODE)
            if( AccessKey )
               FreeUnicodeString(keystring);
#        endif
         call = NULL;
      } /* endif */
   #else
      UNUSED_PARAMETER(AccessKey);
   #endif
   return call;
}

   JSECALLSEQ( void _FAR_ * )
jseGetLinkData(jseContext jsecontext)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseGetLinkData"),return NULL);
   return jsecontext->Global->GenericData;
}

   JSECALLSEQ(struct jseExternalLinkParameters *)
jseGetExternalLinkParameters(jseContext jsecontext)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseGetLinkData"),return NULL);
   return &(jsecontext->Global->ExternalLinkParms);
}



