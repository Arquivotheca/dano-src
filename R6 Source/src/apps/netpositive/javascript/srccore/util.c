/* util.c      Random utilities used by the core. 
 *
 * In most cases, they  are routines to big to put in the core segment 
 * and are all far calls on DOS/WIN16.
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

static jseLibFunc(Ecma_Object_construct)
{
   if( 0 < jseFuncVarCount(jsecontext) )
   {
      jseVariable var = jseFuncVar(jsecontext,0);
      jseDataType dt = jseGetType(jsecontext,var);

      if( dt==jseTypeObject )
      {
         jseReturnVar(jsecontext,var,jseRetCopyToTempVar);
         return;
      }
      if( dt==jseTypeString || dt==jseTypeBoolean || dt==jseTypeNumber )
      {
         jseReturnVar(jsecontext,jseCreateConvertedVariable(jsecontext,var,jseToObject),
                      jseRetTempVar);
         return;
      }
      assert( dt==jseTypeNull || dt==jseTypeUndefined );
     /* fallthru intentional */
   }
   /* create object, instead of leaving on stack, so it automatically
    * inherits Object.prototype
    */
   jseReturnVar(jsecontext,jseCreateVariable(jsecontext,jseTypeObject),jseRetTempVar);
}

static jseLibFunc(Ecma_Object_toString)
{
   jseVariable thisvar = jseGetCurrentThisVariable(jsecontext);
   jseVariable classvar =
      jseCreateConvertedVariable(jsecontext,
                                 jseMember(jsecontext,thisvar,CLASS_PROPERTY,jseTypeString),
                                 jseToString);

   const jsechar *classname = (const jsechar *)jseGetString( jsecontext, classvar, NULL);
   jsechar *buffer = jseMustMalloc(jsechar,sizeof(jsechar)*(strlen_jsechar(classname)+20));
   
   jseVariable ret;

   jse_sprintf(buffer,UNISTR("[object %s]"),classname);

   ret = jseCreateVariable(jsecontext,jseTypeString);

   jsePutString(jsecontext,ret,buffer);
   
   jseMustFree(buffer);
   jseDestroyVariable(jsecontext,classvar);

   jseReturnVar(jsecontext,ret,jseRetTempVar);
}

static jseLibFunc(Ecma_Object_valueOf)
{
   jseReturnVar(jsecontext,jseGetCurrentThisVariable(jsecontext),jseRetCopyToTempVar);
}

static CONST_DATA(jsechar) ObjectNameStr[] = UNISTR("\"Object\"");
static CONST_DATA(struct jseFunctionDescription) ObjectProtoList[] =
{
   JSE_VARSTRING( PROTOTYPE_PROPERTY, textcorevtype_null, jseDontEnum ),
   JSE_VARSTRING( CLASS_PROPERTY, ObjectNameStr, jseDontEnum ),
   JSE_LIBMETHOD( CONSTRUCT_PROPERTY, Ecma_Object_construct, 0, -1, jseDontEnum,  jseFunc_Secure ),
   JSE_LIBMETHOD( TOSTRING_PROPERTY, Ecma_Object_toString, 0, -1, jseDontEnum,  jseFunc_Secure ),
   JSE_LIBMETHOD( VALUEOF_PROPERTY, Ecma_Object_valueOf, 0, -1, jseDontEnum,  jseFunc_Secure ),
   JSE_FUNC_DESC_END
};

static jseLibFunc(Ecma_Function_prototype)
{
   jseReturnVar(jsecontext,jseCreateVariable(jsecontext,jseTypeUndefined),
                jseRetTempVar);
}

static jseLibFunc(Ecma_Function_toString)
{
   jseVariable th = jseGetCurrentThisVariable(jsecontext);
   if( !jseIsFunction(jsecontext,th))
   {
      jseLibErrorPrintf(jsecontext,UNISTR("'this' is not a function."));
   }
   else
   {
      jseVariable ret;
#  if defined(JSE_CREATEFUNCTIONTEXTVARIABLE) && (0!=JSE_CREATEFUNCTIONTEXTVARIABLE)
      ret = jseCreateFunctionTextVariable(jsecontext,th);
#  else
      ret = jseCreateVariable(jsecontext,jseTypeString);
      jsePutString(jsecontext,ret,UNISTR("function _() { }"));
#  endif
      jseReturnVar(jsecontext,ret,jseRetTempVar);
   }
}

static CONST_DATA(jsechar) FunctionNameStr[] = UNISTR("\"Function\"");
static CONST_DATA(jsechar) ZeroStr[] = UNISTR("0");
static CONST_DATA(struct jseFunctionDescription) FunctionProtoList[] =
{
   JSE_LIBMETHOD( ORIG_PROTOTYPE_PROPERTY, Ecma_Function_prototype, 0, -1, jseDontEnum | jseDontDelete | jseReadOnly, jseFunc_Secure ),
   JSE_VARSTRING( UNISTR("prototype._class"), FunctionNameStr, jseDontEnum ),
   JSE_VARSTRING( UNISTR("prototype.length"), ZeroStr, jseDontEnum | jseDontDelete | jseReadOnly ),
   JSE_PROTOMETH( TOSTRING_PROPERTY, Ecma_Function_toString, 0, -1, jseDontEnum, jseFunc_Secure ),
   JSE_FUNC_DESC_END
};

   void
InitializeBuiltinObjects(struct Call *call)
   /* initialize these Object.prototype fields
    * Object.prototype._prototype = null
    * Object.prototype._class = UNISTR("Object")
    * Object.prototype.construct = <function>
    * Object.prototype.toString = <function>
    * Object.prototype.valueOf = <function>
    *
    * initialize these Function.prototype fields
    * Function.prototype = <function>
    * Function.prototype._class = UNISTR("Function")
    * Function.prototype.length = 0
    * Function.prototype.toString = <function>
    */
{
   jseVariable o, op;
   jseVariable f, fp;
   jseVariable temp;

   /* initial fake references to global for when they're refered to */
   call->Global->FunctionPrototype = call->Global->ObjectPrototype
   = temp = jseCreateVariable(call,jseTypeNull);

   jseAddLibrary(call,UNISTR("Object.prototype"),ObjectProtoList,NULL,NULL,NULL);
   /* preserve Object.prototype */
   o = jseMemberEx(call,NULL,OBJECT_PROPERTY,jseTypeObject,
                   jseCreateVar|jseLockRead|jseDontSearchPrototype);
   assert( NULL != o );
   jseSetAttributes(call,o,jseDontEnum);
   op = jseMemberEx(call,o,ORIG_PROTOTYPE_PROPERTY,jseTypeObject,
                    jseCreateVar|jseLockRead|jseDontSearchPrototype);
   assert( NULL != op );
   jseSetAttributes(call,op,jseDontEnum | jseDontDelete | jseReadOnly);
   call->Global->ObjectPrototype = op;
   jseDestroyVariable(call,o);

   jseAddLibrary(call,FUNCTION_PROPERTY,FunctionProtoList,NULL,NULL,NULL);
   /* preserve Function.prototype */
   f = jseMemberEx(call,NULL,FUNCTION_PROPERTY,jseTypeObject,
                   jseCreateVar|jseLockRead|jseDontSearchPrototype);
   assert( NULL != f );
   jseSetAttributes(call,f,jseDontEnum);
   fp = jseMemberEx(call,f,ORIG_PROTOTYPE_PROPERTY,jseTypeObject,
                    jseCreateVar|jseLockRead|jseDontSearchPrototype);
   assert( NULL != fp );
   jseSetAttributes(call,fp,jseDontEnum | jseDontDelete | jseReadOnly);
   call->Global->FunctionPrototype = fp;
   jseDestroyVariable(call,f);

   jseDestroyVariable(call,temp);
}

   void
TerminateBuiltinObjects(struct Call *call)
{
   jseDestroyVariable(call,call->Global->FunctionPrototype);
   jseDestroyVariable(call,call->Global->ObjectPrototype);
}



#if defined(__JSE_UNIX__)
  static CONST_DATA(jsechar) eol[] = UNISTR("\n");
#elif defined(__JSE_MAC__)
  static CONST_DATA(jsechar) eol[] = UNISTR("\r");
#else
  static CONST_DATA(jsechar) eol[] = UNISTR("\r\n");
#endif

#if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
/* mempool.h routines */

jsebool
allocatorInit(struct Allocator *this,size_t size,uint initial)
{
   jsebool success = True;

   this->item_size = size;
   this->num_saved = 0;
   this->pool_size = initial;

   assert( initial*sizeof(void *)< (uint)0xFFFF );

   if( NULL != (this->saved_ptrs = jseMalloc(void *,(uint)(initial*sizeof(void *)))) )
   {
#if 0
#  if !defined(JSE_MIN_MEMORY) || (0==JSE_MIN_MEMORY)
   /* if we have enough memory then preallocate */
      while( this->num_saved<initial )
         if( NULL == (this->saved_ptrs[(this->num_saved)++] = jseMalloc(void,size)))
            break;
            
      if( this->num_saved != initial )
      {
         allocatorTerm(this);
         success = False;
      }

#   endif
#endif
   }
   /* on exit, num_saved = the number of items saved. */

   return success;
}


void allocatorTerm(struct Allocator *this)
{
   while( this->num_saved )
      jseMustFree(this->saved_ptrs[--(this->num_saved)]);
   jseMustFree(this->saved_ptrs);
   this->saved_ptrs = NULL;
}
#endif /* defined(JSE_FAST_MEMPOOL) */

#if defined(JSE_DYNAOBJ_HASHLIST) && (0!=JSE_DYNAOBJ_HASHLIST) 
   void
MoveDynaobjHashEntry(struct Global_ *global,uint offset,VarName *oldEntry,uword16 HasFlag)
{
   struct HashList *oldHL = HashListFromName(*oldEntry);
   struct DynamicHashList *dhl = global->dynahashlist + offset;
   /* copy all data from old hashlist entry */
   memcpy(&(dhl->list),oldHL,sizeof(struct HashList)+sizeof(stringLengthType)+(sizeof(jsechar)*(strlen_jsechar(*oldEntry)+1)));
   assert( (sizeof(jsechar)*(strlen_jsechar(*oldEntry)+1)) < (sizeof(dhl->name)-sizeof(stringLengthType)) );
   dhl->HasFlag = HasFlag;
   /* adjust chained link from old list to this one */
   *(dhl->list.prev) = &(dhl->list);
   *oldEntry = (VarName)NameFromHashList((struct HashList *)dhl);
   /* don't need the old link anymore */
   jseMustFree(oldHL);
}
   void
RestoreDynaobjHashEntry(struct Global_ *global,uint offset,VarName *oldEntry)
{
   struct DynamicHashList *dhl = global->dynahashlist + offset;
   struct HashList *newHL = jseMustMalloc(struct HashList,sizeof(*dhl));
   /* copy all data from old hashlist entry, copy all which wastes a little
    * space but no matter because this is only done at cleanup
    */
   memcpy(newHL,dhl,sizeof(*dhl));
   /* adjust chained link from old list to this one */
   *(newHL->prev) = newHL;
   *oldEntry = (VarName)NameFromHashList(newHL);
}
#endif

#if !defined(NDEBUG)

static VAR_DATA(jsebool) InFatal = False;
   /* sort-of OK because this is only used as last resort in fatal crash */
void callFatal(struct Call *this,jsebool PrintMessage)
{
   if ( !InFatal ) {
      InFatal = True;
      if ( PrintMessage ) {
         callError(this,(enum textcoreID)0);
      } /* endif */
   } /* endif */
   TerminatejseEngine();
   exit(EXIT_FAILURE);
}

void callFatalText(struct Call *this,const jsechar * FormatS,va_list arglist)
{
   if ( InFatal ) return;
   ErrorVPrintf(this,FormatS,arglist);
   callFatal(this,False);
}

void callFatalID(struct Call *this,enum textcoreID id,...)
{
   va_list arglist;
   va_start(arglist,id);
   callFatalText(this,textcoreGet(id),arglist);
   va_end(arglist);
}
#endif /* !defined(NDEBUG) */

#if (defined(JSE_DEFINE) && (0!=JSE_DEFINE)) \
 || (defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)) \
 || (defined(JSE_LINK) && (0!=JSE_LINK))
struct PreProcesses_ {
   const jsechar *Name;
   jsebool (*Function)(struct Source **source,struct Call *call);
   /* return False and print error if problem, else returns True */
};

jsebool PreprocessorDirective(struct Source **source,struct Call *call)
{
   jsebool success;
   static CONST_DATA(struct PreProcesses_) PreProcesses[] = {
#     if defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)
         { textcoreIncludeDirective,     sourceInclude },
#     endif
#     if defined(JSE_DEFINE) && (0!=JSE_DEFINE)
         { textcoreDefineDirective,      defineProcessSourceStatement },
#     endif
#     if defined(JSE_LINK) && (0!=JSE_LINK)
         { textcoreExtLinkDirective,     extensionLink },
#     endif
      { NULL, NULL } };
   jsechar *src = sourceGetPtr(*source);

   /* find end of the PreProcessor directive */
   jsechar *end = ++src;
   int srcLen;
   struct PreProcesses_ const *PrePro;

   assert( '#' == *(src-1) );
   while ( 0 != end  &&  !IS_SAMELINE_WHITESPACE(*end) ) {
      end++;
   } /* endwhile */
   srcLen = (int)(end - src);
   if ( 0 == srcLen  ||  IS_NEWLINE(*end) )
      goto UnknownDirective;
   /* find this preprocessor directive in the list of known ones */
   for ( PrePro = PreProcesses; NULL != PrePro->Name; PrePro++ ) {
      if ( 0 == strnicmp_jsechar(PrePro->Name,src,(size_t)srcLen)  &&
           0 == PrePro->Name[srcLen] )
      {
         break;
      }
   }

   if ( NULL == PrePro->Name ) {
      UnknownDirective:
      callError(call,textcoreUNRECOGNIZED_PREPROCESSOR_DIRECTIVE,src-1);
      success = False;
   } else {
      /* increment source beyond directive and up to the
         non-whitespace characters */
      SKIP_SAMELINE_WHITESPACE(end);
      /* call the chosen preprocessor directive routine */
      sourceSetPtr(*source,end);
      success = PrePro->Function(source,call);
   } /* endif */

   return success;
}
#endif

const jsechar * functionName(struct Function *this,struct Call *call)
{
   return functionTestIfLocal(this)
        ? farGetStringTableEntry(call,((struct LocalFunction *)this)->FunctionName,NULL)
        : ((struct LibraryFunction *)this)->FuncDesc->FunctionName ;
}

const jsechar * callCurrentName(struct Call *this)
{
   struct Function *func = callFunc(this);
   return (func!=NULL) ? functionName(func,this) : UNISTR("");
}

struct Call * callCurrent(struct Call *this)
{
   struct Call *call;
   for ( call = this; NULL != call->pChildCall; call = call->pChildCall )
   {
   } /* endfor */
   return call;
}


void callError(struct Call *this,enum textcoreID id,...)
{
   if ( FlowError != callReasonToQuit(this) ) {
      va_list arglist;
      va_start(arglist,id);
      if(id!=0)
         ErrorVPrintf(this,textcoreGet(id),arglist);
      else
         ErrorVPrintf(this,UNISTR(""),arglist);
      va_end(arglist);
   }
   assert( FlowError == callReasonToQuit(this) );
}

   JSECALLSEQ( void )
jseLibSetErrorFlag(jseContext jsecontext)
{
   assert( NULL != jsecontext );
   if(NULL == jsecontext)
   {
      return;
   }
   callErrorBase(jsecontext);
}


void ErrorVPrintf(struct Call *this,const jsechar *FormatS,va_list arglist)
{
   if ( (this->Global->ExternalLinkParms.PrintErrorFunc) )
   {
      jsechar error_msg[1024];
      const jsechar *SourceFileName; uint SourceLineNumber;

      error_msg[0] = '\0';
      jse_vsprintf(error_msg,FormatS,arglist);
      assert( strlen_jsechar(error_msg) < sizeof(error_msg) );

      /* If possible find line number information and add that */
      if ( secodeSourceLocation(this,&SourceFileName,&SourceLineNumber) ) {
         jse_sprintf(error_msg+strlen_jsechar(error_msg),UNISTR("%s%s %s:%d [%s()]."),eol,
                 textcoreErrorNear,
                 (NULL != SourceFileName) ? SourceFileName :
                       textcoreInlineSourceCodePhonyFilename,
                 SourceLineNumber,
                 (NULL != callFunc(this)) ? callCurrentName(this) :
                       textcoreUnknown );
         assert( strlen_jsechar(error_msg) < sizeof(error_msg) );
      }

      if( NULL != callFunc(this) )
      {
         struct Call *pc = this;
         int i=0;

         while( (pc = callPrevious(pc)) != NULL )
         {
            jsechar msg[1024];

            /* Func()==NULL for the 'not in any function' function,
             * higher levels probably represent calls to interpret and
             * such (like in the shell)
             */
            if( callFunc(pc)==NULL ) break;

            SourceFileName = textcoreUnknown; SourceLineNumber = 0;
            if( secodeSourceLocation(pc,&SourceFileName,&SourceLineNumber) )
            {
               if ( NULL == SourceFileName )
                  SourceFileName = textcoreInlineSourceCodePhonyFilename;

               /* upto 4 depth, more probably means recursion */
               if( i++ > 4 )
                  break;

               assert( NULL != callFunc(pc) );
               jse_sprintf(msg,UNISTR("%s      from %s:%d [%s()]"),eol,
                       SourceFileName,SourceLineNumber,callCurrentName(pc));
               assert( strlen_jsechar(msg) < sizeof(msg) );

               /* if buffer fills up, break */
               if( strlen_jsechar(error_msg) + strlen_jsechar(msg) + 1 >= sizeof(error_msg) )
                  break;

               strcat_jsechar(error_msg,msg);
               assert( strlen_jsechar(error_msg) < sizeof(error_msg) );
            }
         }
      }

      #if (defined(__JSE_WIN16__) || defined(__JSE_DOS16__)) \
       && (defined(__JSE_DLLLOAD__) || defined(__JSE_DLLRUN__))
         DispatchToClient(this->Global->ExternalDataSegment,
                          (ClientFunction)(this->Global->ExternalLinkParms.
                                           PrintErrorFunc),
                          (void *)this,(void *)error_msg);
      #else
         (*(this->Global->ExternalLinkParms.PrintErrorFunc))((jseContext)this,
                                                             error_msg);
      #endif
#     if !defined(NDEBUG) && (0<JSE_API_ASSERTLEVEL) && defined(_DBGPRNTF_H)
         if ( !jseApiOK )
         {
            DebugPrintf(UNISTR("Error calling PrintErrorFunc"));
            DebugPrintf(UNISTR("Error message: %s"),jseGetLastApiError());
         }
#     endif
      assert( jseApiOK );

  } /* endif */

  if ( FlowError != callReasonToQuit(this) ) {
    callSetReasonToQuit(this,FlowError);
  }
  assert( FlowError == callReasonToQuit(this) );
}

   JSECALLSEQ( void )
jseLibErrorPrintf(jseContext jsecontext,const jsechar * formatS,...)
{
   va_list arglist;

   assert( NULL != jsecontext );
   if(NULL == jsecontext)
   {
      return;
   }
   va_start(arglist,formatS);
   ErrorVPrintf(jsecontext,formatS,arglist);
   va_end(arglist);
}


void callQuit(struct Call *this,enum textcoreID id,...)
{
   if ( !callQuitWanted(this) ) {
      if ( 0 == id ) {
         callErrorBase(this);
      } else {
         va_list arglist;
         va_start(arglist,id);
         ErrorVPrintf(this,textcoreGet(id),arglist);
         va_end(arglist);
      }
   }
   assert( callQuitWanted(this) );
}


void DescribeInvalidVar(struct Call *this,struct Var *v,jseDataType vType,
                        struct Function *FuncPtrIfObject,jseVarNeeded need,
                        struct InvalidVarDescription *BadDesc)
{
   const jsechar *TypeWeGot;
   jsechar ValidBuf[sizeof(BadDesc->VariableWanted)];
   const jsechar *OrMsg = textcoreGet(textcorePARAM_TYPE_OR);

   /* prepare variable name if we can figure it out */
   if ( FindNames(this,v,BadDesc->VariableName+1,
                  sizeof(BadDesc->VariableName)-3) )
   {
      BadDesc->VariableName[0] = '(';
      strcat_jsechar(BadDesc->VariableName,UNISTR(") "));
   } else {
      BadDesc->VariableName[0] = '\0';
   } /* endif */

   /* prepare variable to show what type of buffer we did get */
   switch ( vType ) {
      case VUndefined:
         TypeWeGot = textcoreGet(textcorePARAM_TYPE_UNDEFINED);
         break;
      case VNull:
         TypeWeGot = textcoreGet(textcorePARAM_TYPE_NULL);
         break;
      case VBoolean:
         TypeWeGot = textcoreGet(textcorePARAM_TYPE_BOOLEAN);
         break;
      case VObject:
         TypeWeGot = textcoreGet( NULL == FuncPtrIfObject
                                  ? textcorePARAM_TYPE_OBJECT
                                  : textcorePARAM_TYPE_FUNCTION_OBJECT );
         break;
      case VString:
         TypeWeGot = textcoreGet(textcorePARAM_TYPE_STRING);
         break;
#     if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)      
      case VBuffer:
         TypeWeGot = textcoreGet(textcorePARAM_TYPE_BUFFER);
         break;
#     endif      
      case VNumber:
         TypeWeGot = textcoreGet(textcorePARAM_TYPE_NUMBER);
         break;
   }
   assert( strlen_jsechar(TypeWeGot) < sizeof(BadDesc->VariableType) );
   strcpy_jsechar(BadDesc->VariableType,TypeWeGot);

   /* prepare string showing what types would have been valid */
   memset(ValidBuf,0,sizeof(ValidBuf));
   if ( need & JSE_VN_UNDEFINED )
      strcat_jsechar(strcat_jsechar(ValidBuf,OrMsg),textcoreGet(textcorePARAM_TYPE_UNDEFINED));
   if ( need & JSE_VN_NULL )
      strcat_jsechar(strcat_jsechar(ValidBuf,OrMsg),textcoreGet(textcorePARAM_TYPE_NULL));
   if ( need & JSE_VN_BOOLEAN )
      strcat_jsechar(strcat_jsechar(ValidBuf,OrMsg),textcoreGet(textcorePARAM_TYPE_BOOLEAN));
   if ( need & JSE_VN_OBJECT )
      strcat_jsechar(strcat_jsechar(ValidBuf,OrMsg),textcoreGet(textcorePARAM_TYPE_OBJECT));
   if ( need & JSE_VN_STRING )
      strcat_jsechar(strcat_jsechar(ValidBuf,OrMsg),textcoreGet(textcorePARAM_TYPE_STRING));
#  if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)      
      if ( need & JSE_VN_BUFFER )
         strcat_jsechar(strcat_jsechar(ValidBuf,OrMsg),textcoreGet(textcorePARAM_TYPE_BUFFER));
#  endif         
   if ( need & JSE_VN_NUMBER )
      strcat_jsechar(strcat_jsechar(ValidBuf,OrMsg),textcoreGet(textcorePARAM_TYPE_NUMBER));
   if ( need & JSE_VN_FUNCTION )
      strcat_jsechar(strcat_jsechar(ValidBuf,OrMsg),
             textcoreGet(textcorePARAM_TYPE_FUNCTION_OBJECT));
   if ( need & JSE_VN_BYTE )
      strcat_jsechar(strcat_jsechar(ValidBuf,OrMsg),textcoreGet(textcorePARAM_TYPE_BYTE));
   if ( need & JSE_VN_INT )
      strcat_jsechar(strcat_jsechar(ValidBuf,OrMsg),textcoreGet(textcorePARAM_TYPE_INT));
   assert( strlen_jsechar(ValidBuf) < sizeof(ValidBuf) );
   strcpy_jsechar( BadDesc->VariableWanted, ValidBuf + strlen_jsechar(OrMsg));
}


void ParseSourceTextIntoArgv(jsechar *SourceText,int *_argc, jsechar ***_argv)
{
   int argc = *_argc;
   jsechar **argv = *_argv;
   jsechar *endcp; jsechar *cp;
   jsechar *TooFar;

   RemoveWhitespaceFromHeadAndTail(SourceText);
   cp = SourceText;
   TooFar = cp + strlen_jsechar(cp);
   if ( '\0' != *cp )
   {
      size_t len;

      assert( !IS_SAMELINE_WHITESPACE(*cp) );
      while ( TooFar != cp )
      {
         argv = jseMustReMalloc(jsechar *,argv,sizeof(jsechar *) * (++argc));
         /* cp is start of argument, move endcp to the end, including any
          * quotes along the way
          */
         assert( cp < TooFar  &&  *cp  &&  !IS_SAMELINE_WHITESPACE(*cp) );
         for ( endcp = cp; endcp != TooFar && !IS_SAMELINE_WHITESPACE(*endcp); endcp++ )
         {
            if ( '\"' == *endcp )
            {
               jsechar * endlen;
               /* eat-up this quote, then find matching quote */
               memmove(endcp,endcp+1,sizeof(jsechar)*(len = (size_t)(--TooFar - endcp)));
               endlen = endcp + len;
               if ( NULL == (endcp = strchr_jsechar(endcp,'\"'))  ||  endlen <= endcp  )
               {
                  endcp = TooFar;
               }
               else
               {
                  /* remove ending quote */
                  memmove(endcp,endcp+1,sizeof(jsechar)*((size_t)(--TooFar - endcp)));
               }
               endcp--;
            } /* endif */
         } /* endfor */
         len = (size_t)(endcp - cp);
         argv[argc-1] = jseMustMalloc(jsechar,sizeof(jsechar)*(len+1));
         memcpy(argv[argc-1],cp,sizeof(jsechar)*len);
         argv[argc-1][len] = 0;
         cp = endcp;
         assert( cp <= TooFar );
         if ( cp != TooFar )
         {
            if ( *cp == '\"' ) cp++;
            while( cp != TooFar  &&  IS_SAMELINE_WHITESPACE(*cp) ) cp++;
         } /* endif */
         assert( cp <= TooFar );
      } /* end while */
   } /* endif */
   *_argc = argc;
   *_argv = argv;
}

void FreeArgv(int argc,jsechar *argv[])
{
   int i;

   /* free all the argv fields and argv itself */
   for ( i = 0; i < argc; i++ ) {
      assert( NULL != argv[i] );
      jseMustFree(argv[i]);
   }
   jseMustFree(argv);
}

#ifndef NDEBUG
void JSE_CFUNC InstantDeath(enum textcoreID TextID,...)
{
#  if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
      FILE *fp = fopen_jsechar(UNISTR("JSEERROR.LOG"),UNISTR("at"));
      va_list arglist;

      va_start(arglist,TextID);
      if ( fp ) {
         vfprintf_jsechar(fp,textcoreGet(TextID),arglist);
         fclose(fp);
      }
      va_end(arglist);
#  endif

   assert( False );     /* so if debugging, will stop here. */

   exit(EXIT_FAILURE);
}
#endif



/* ***************************************************************************
 * *** The followinjg function are the external interface to the core that ***
 * *** do not need to be in the small core segment (those in the tight     ***
 * *** segment are in jselib.cpp                                           ***
 * ***************************************************************************
 */
JSE_POINTER_UINDEX  
jseGetNameLength(jseContext jsecontext, 
                 const jsechar * name )
{
   UNUSED_PARAMETER(jsecontext);
   
   return strlen_jsechar(name);
}

const jsechar * 
jseSetNameLength(jseContext jsecontext, const jsechar * name,
                 JSE_POINTER_UINDEX length)
{
   UNUSED_PARAMETER(jsecontext);
   UNUSED_PARAMETER(length);

   return name;
}


   JSECALLSEQ( void _FAR_ * )
jseLibraryData(jseContext jsecontext)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseLibraryData"),
                  return False);
   return GetTemporaryLibraryData(jsecontext);
}

   JSECALLSEQ( uint )
jseFuncVarCount(jseContext jsecontext)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseFuncVarCount"),
                  return False);
   return( callParameterCount(jsecontext) );
}

   JSECALLSEQ(jseVariable)
jseMemberWrapperFunction(jseContext jsecontext,jseVariable objectVar,
      const jsechar *functionName,
      void (JSE_CFUNC FAR_CALL *funcPtr)(jseContext jsecontext),
      sword8 minVariableCount, sword8 maxVariableCount,
      jseVarAttributes varAttributes, jseFuncAttributes funcAttributes, void _FAR_ *fData)
{
   jseVariable funcvar, membervar;
   JSE_API_STRING(ThisFuncName,UNISTR("jseMemberWrapperFunction"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return NULL);
#  if ( 0 < JSE_API_ASSERTLEVEL )
      if ( NULL != objectVar )
         JSE_API_ASSERT_C(objectVar,2,jseVariable_cookie,ThisFuncName,
                        return NULL);
#  endif
   JSE_API_ASSERT_(functionName,3,ThisFuncName,return NULL);
   JSE_API_ASSERT_(funcPtr,4,ThisFuncName,return NULL);

   funcvar = jseCreateWrapperFunction(jsecontext,functionName,funcPtr,
                                      minVariableCount,maxVariableCount,
                                      varAttributes,funcAttributes,fData);

   membervar = jseMember(jsecontext,objectVar,functionName,jseTypeUndefined); 
   jseSetAttributes(jsecontext, membervar, 0 );
   /* set to zero in case it existed and might be readonly */
   jseAssign(jsecontext,membervar,funcvar);
   jseDestroyVariable(jsecontext,funcvar);
   /* since we set the attributes to 0 above, reset them */
   jseSetAttributes(jsecontext,membervar,varAttributes);
   return membervar;
}

   JSECALLSEQ( jseVariable )
jseGlobalObject(jseContext jsecontext)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseGlobalObject"),
                  return NULL);
   return jsecontext->session.GlobalVariable;
}

   JSECALLSEQ( jseContext )
jsePreviousContext(jseContext jsecontext)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jsePreviousContext"),
                  return NULL);
   return callPrevious(jsecontext);
}

   JSECALLSEQ( const jsechar * )
jseLocateSource(jseContext jsecontext,uint *lineNumber)
{
   const jsechar *FileName;
   JSE_API_STRING(ThisFuncName,UNISTR("jseLocateSource"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return NULL);
   JSE_API_ASSERT_(lineNumber,2,ThisFuncName,return NULL);

   if ( !secodeSourceLocation(jsecontext,&FileName,lineNumber) )
      FileName = NULL;
   return (jsechar *)FileName;
}

   JSECALLSEQ( const jsechar * )
jseCurrentFunctionName(jseContext jsecontext)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseCurrentFunctionName"),
                  return NULL);
   return callCurrentName(jsecontext);
}

   JSECALLSEQ(jseContext)
jseAppExternalLinkRequest(jseContext jsecontext,jsebool Initialize)
{
   jseAppLinkFunc AppLinkFunc;

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseAppExternalLinkRequest"),
                  return NULL);

   AppLinkFunc = jsecontext->Global->ExternalLinkParms.AppLinkFunc;
   return ( NULL == AppLinkFunc ) ? (jseContext)NULL :
      (*AppLinkFunc)(jsecontext,Initialize) ;
}

#if defined(JSE_GETFILENAMELIST) && (0!=JSE_GETFILENAMELIST)
   JSECALLSEQ(jsechar **)
jseGetFileNameList(jseContext jsecontext,int *number)
{
   JSE_API_STRING(ThisFuncName,UNISTR("jseGetFileNameList"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return NULL);
   JSE_API_ASSERT_(number,2,ThisFuncName,return NULL);

   return CALL_GET_FILENAME_LIST(jsecontext,number);
}
#endif

   JSECALLSEQ(jseVariable)
jseGetCurrentThisVariable(jseContext jsecontext)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseGetCurrentThisVariable"),
                  return NULL);
   return GetCurrentThisVar(jsecontext);
}


#if 0 != JSE_MULTIPLE_GLOBAL
   JSECALLSEQ(void)
jseSetGlobalObject(jseContext jsecontext,jseVariable newGlobal)
{
   JSE_API_STRING(ThisFuncName,UNISTR("jseSetGlobalObject"));
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
// seb 99.01.11 - Uhhhh, I think this assert should check newGlobal, not jsecontext.
   JSE_API_ASSERT_C(newGlobal,2,jseVariable_cookie,ThisFuncName,return);
// seb 99.4.30 -- It would be really nice if we could make sure that the global is readable.
// But we can't, because this adds an extra reference to the variable which never gets deleted.
// Besides, Nombas sez it has other problems.
//   jsecontext->session.GlobalVariable = GET_READABLE_VAR(newGlobal, jsecontext);
	if (newGlobal->reference.parentObject != NULL)
		debugger("Setting invalid global variable!");
   jsecontext->session.GlobalVariable = newGlobal;
}
#endif

#if 0 != JSE_CYCLIC_GC
   JSECALLSEQ(ulong)
jseCyclicGC(jseContext jsecontext,jsebool Start)
{
   if ( Start )
   {
      if ( !jsecontext->Global->CollectingCyclicGarbage )
      {
         CollectCyclicGarbage(jsecontext);
      }
   }
   else
   {
      jsecontext->Global->CollectingCyclicGarbage = False;
   }
   return 0;
}
#endif

   struct jseCallStack * NEAR_CALL
jsecallstackNew(void)
{
   struct jseCallStack *This = jseMustMalloc(struct jseCallStack,
                                             sizeof(struct jseCallStack));
#  if ( 2 <= JSE_API_ASSERTLEVEL )
      This->cookie = (ubyte) jseStack_cookie;
#  endif
   This->var = jseMustMalloc(struct Var *,sizeof(struct Var *));
   This->Count = 0;
   return This;
}

   void
jsecallstackPush(struct jseCallStack *This,
                 struct Var *v,jsebool deleteme)
{
   This->var = jseMustReMalloc(struct Var *,This->var,
                               (++(This->Count)) * sizeof(struct Var *));
   This->var[This->Count-1] = v;
   if ( !deleteme )
      varAddUser(v);
}

   JSECALLSEQ( jseStack )
jseCreateStack(jseContext jsecontext)
{
   UNUSED_PARAMETER(jsecontext);
   return jsecallstackNew();
}

   JSECALLSEQ( void )
jseDestroyStack(jseContext jsecontext,jseStack jsestack)
{
   JSE_API_STRING(ThisFuncName,UNISTR("jseDestroyStack"));

   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return);
   JSE_API_ASSERT_C(jsestack,2,jseStack_cookie,ThisFuncName,return);

   jsecallstackDelete(jsestack,jsecontext);
}

   JSECALLSEQ( void )
jsePush(jseContext jsecontext,jseStack jsestack,jseVariable var,
        jsebool DeleteVariableWhenFinished)
{
   JSE_API_STRING(ThisFuncName,UNISTR("jsePush"));

   JSE_API_ASSERT_C(jsestack,2,jseStack_cookie,ThisFuncName,return);
   JSE_API_ASSERT_C(var,3,jseVariable_cookie,ThisFuncName,return);

   UNUSED_PARAMETER(jsecontext);

   jsecallstackPush(jsestack,var,DeleteVariableWhenFinished);
}

   JSECALLSEQ(jseVariable)  
jsePop(jseContext jsecontext, jseStack jsestack)
{
   jseVariable retvar;

   JSE_API_STRING(ThisFuncName,UNISTR("jsePop"));
   
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,return NULL);
   JSE_API_ASSERT_C(jsecontext,2,jseStack_cookie,ThisFuncName,return NULL);
   
   retvar = jsecallstackPop(jsestack);
   
   return retvar;
}

   JSECALLSEQ( uint )
jseQuitFlagged(jseContext jsecontext)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseCurrentContext"),
                  return True);

   assert( JSE_CONTEXT_ERROR == JSE_DEBUG_FEEDBACK(FlowError) );
   assert( JSE_CONTEXT_EXIT == JSE_DEBUG_FEEDBACK(FlowExit) );
   if ( callQuitWanted(jsecontext) ) {
      assert( JSE_CONTEXT_ERROR == callReasonToQuit(jsecontext) || \
              JSE_CONTEXT_EXIT == callReasonToQuit(jsecontext) );
      return callReasonToQuit(jsecontext);
   } /* endif */
   return 0;
}

   JSECALLSEQ( jseContext )
jseCurrentContext(jseContext jsecontext)
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseCurrentContext"),
                  return NULL);
   return callCurrent(jsecontext);
}

   JSECALLSEQ(jsebool)
jseGetVariableName(jseContext jsecontext,jseVariable variableToFind,
                   jsechar * const buffer, uint bufferSize)
{
   jseContext LocalVariableContext = jsecontext;
   
   JSE_API_STRING(ThisFuncName,UNISTR("jseFindName"));
   
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,ThisFuncName,
                    return False);
   JSE_API_ASSERT_C(variableToFind,2,jseVariable_cookie,ThisFuncName,
                    return False);
   
   /* caller may not know exactly how far up to find local variables,
    * so find most recent local function
    */
   while ( NULL != LocalVariableContext
       && NULL != callFunc(LocalVariableContext)
       && !functionTestIfLocal(callFunc(LocalVariableContext)) )
   {
      LocalVariableContext = callPrevious(LocalVariableContext);
   }
   
   /* Somehow we didn't find a local variable context, so use jsecontext */
   if( NULL == LocalVariableContext )
      LocalVariableContext = jsecontext;

   return FindNames(LocalVariableContext,variableToFind,buffer,bufferSize);
}

#if defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC)
   JSECALLSEQ( void )
jseDestroyCodeTokenBuffer(jseContext jsecontext, jseTokenRetBuffer buffer )
{
   JSE_API_ASSERT_C(jsecontext,1,jseContext_cookie,UNISTR("jseDestroyCodeTokenBuffer"),return);

   jseMustFree(buffer);
}
#endif
