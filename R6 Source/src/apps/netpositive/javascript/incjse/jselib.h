/* jseLib.h    Interface to external (or internal) libraries.
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

#ifndef _JSELIB_H
#define _JSELIB_H

#if !defined(__JSE_UNIX__) && !defined(__JSE_MAC__) \
 && !defined(__JSE_PSX__) && !defined(__JSE_PALMOS__)
#  if defined(__BORLANDC__)
#     pragma option -a1
#  else
#     pragma pack( 1 )
#  endif
#endif /*!defined(__JSE_UNIX__) && !define(__JSE_MAC__) ... */

#ifdef __cplusplus
   extern "C" {
#endif



/*****************************************
 *** MUST HAVE DEFINED THE LINK METHOD ***
 *****************************************/
/* don't put the comments on the same line as the #if - confuses makedepend */
#if   defined(__JSE_DLLLOAD__)
   /* linking DLL at load time */
#elif defined(__JSE_DLLRUN__)
   /* linking DLL at run time */
#elif defined(__JSE_LIB__)
   /* linking with the jse library */
#else
#  error UNDEFINED JSE LINK METHOD
#endif  /* defined(__JSE_DLLLOAD__) */

/*****************************************************
 *** MUST HAVE DEFINED WHETHER APPLICATION OR CORE ***
 *****************************************************/
#if !defined(JSETOOLKIT_APP) && !defined(JSETOOLKIT_CORE) && !defined(JSETOOLKIT_LINK)
   /* for the customers ease lets assume a toolkit application */
#  define JSETOOLKIT_APP
#endif
#if defined(JSETOOLKIT_APP) && defined(JSETOOLKIT_CORE)
#  error Must not define both JSETOOLKIT_APP and JSETOOLKIT_CORE
#endif /* !JSETOOLKIT_APP && !JSETOOLKIT_CORE ... */

/******************************************************************************
 *** DEFAULT OPTIONS; TOOLKIT ASSUMES THESE OPTIONS; DO NOT UNDEFINE ANY OF ***
 *** THESE OPTIONS WITHOUT RECOMPILING ALL OF THE TOOLKIT SOURCE CODE;      ***
 *** UNDEFINING ANY OF THESE OPTIONS WILL MAKE USELESS ANY #LINK'S NOT      ***
 *** COMPILED WITH THE SAME OPTIONS.                                        ***
 ******************************************************************************/
#  if !defined(JSE_MEM_DEBUG) && !defined(NDEBUG)
#     define JSE_MEM_DEBUG 1
#  endif
#  if !defined(JSE_TOKENSRC)
#     define JSE_TOKENSRC 1
#  endif
#  if !defined(JSE_TOKENDST)
#     define JSE_TOKENDST 1
#  endif
#  if !defined(JSE_SECUREJSE)
#     if defined(__JSE_DOS16__)
#        define JSE_SECUREJSE 0
#     elif defined(__JSE_EPOC32__)
#        define JSE_SECUREJSE 0
#     else
#        define JSE_SECUREJSE 1
#     endif
#endif
#if !defined(JSE_C_EXTENSIONS)
#  define JSE_C_EXTENSIONS    1
#endif
#if !defined(JSE_LINK)
#  if defined(__JSE_DOS16__) || defined(__JSE_DOS32__) \
   || defined(__JSE_PSX__) || defined(__JSE_EPOC32__) \
   || defined(__JSE_390__) || defined(__JSE_PALMOS__)
#     define JSE_LINK  0
#  else
#     define JSE_LINK  1
#  endif
#endif
#if !defined(JSE_INCLUDE)
#  define JSE_INCLUDE 1
#endif
#if !defined(JSE_DEFINE)
#  define JSE_DEFINE 1
#endif
#if !defined(JSE_CONDITIONAL_COMPILE)
#  define JSE_CONDITIONAL_COMPILE 1
#endif
#if !defined(JSE_TOOLKIT_APPSOURCE)
#  define JSE_TOOLKIT_APPSOURCE 1
#endif
#if !defined(JSE_TOOLKIT_APPSOURCE) || (0==JSE_TOOLKIT_APPSOURCE)
#  if defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)
#     error cannot define JSE_INCLUDE without JSE_TOOLKIT_APPSOURCE
#  endif
#  if defined(JSE_LINK) && (0!=JSE_LINK)
#     error cannot define JSE_LINK without JSE_TOOLKIT_APPSOURCE
#  endif
#endif

/* Prototypes allow jse classes to inherit from other classes */
#if !defined(JSE_PROTOYPES)
#  define JSE_PROTOTYPES 1
#endif
/* Dynamic Objects allow overwritting of _get, _put, _delete, and
 * other methods of objects.
 */
#if !defined(JSE_DYNAMIC_OBJS)
#  define JSE_DYNAMIC_OBJS 1
#endif

#if !defined(JSE_OPERATOR_OVERLOADING) && \
    defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
#  define JSE_OPERATOR_OVERLOADING  1
#endif

#if !defined(JSE_DYNAMIC_OBJ_INHERIT)
   /* if have dynamic objs, then by default inherit them
    * through the _prototype
    */
#  define JSE_DYNAMIC_OBJ_INHERIT JSE_DYNAMIC_OBJS
#endif
/*********************************************************************
 * Error messages define the level and type of API messages available
 * from the API (see jseGetLastApiError, jseClearLastApiError, and 
 * jseApiOK).  If these default levels are change then you must also
 * change the core API.  Error Level 0 corresponds to no error
 * reporting, Error Level 1 is for simple error messages such as
 * checks against NULL values where NULL is not allowed.  Error Level
 * 2 provides a finer validation on all parameters that are of the
 * common types (jseContext,jseVariable,jseCallStack).  Setting
 * JSE_API_ASSERTNAMES to 0 means that errors will not be
 * accompanied with function names, else they will have funciton names.
 */
#if !defined(JSE_API_ASSERTLEVEL)
#  if defined(__JSE_DOS16__)
#     define JSE_API_ASSERTLEVEL 0
#  else
#     define JSE_API_ASSERTLEVEL 2
#  endif
#endif
#if !defined(JSE_API_ASSERTNAMES)
#  if defined(__JSE_DOS16__)
#     define JSE_API_ASSERTNAMES 0
#  else
#     define JSE_API_ASSERTNAMES 1
#  endif
#endif
#if ( JSE_API_ASSERTLEVEL < 1 ) && (JSE_API_ASSERTNAMES == 1)
#  error cannot define JSE_API_ASSERTNAMES if JSE_API_ASSERTLEVEL is zero
#endif

/* Resource strings are often the error messages that report on a problem.
 * To save memory you may want to revert to the compact resource strings
 * which may only be an error number
 */
#if !defined(JSE_SHORT_RESOURCE)
#  if defined(__JSE_DOS16__)
#     define JSE_SHORT_RESOURCE 0
#  else
#     define JSE_SHORT_RESOURCE 0
#  endif
#endif

/* The compiler can be turned off, so this can only play tokens. Without
 * the compiler this cannot read source code from string or from file
 */
#if !defined(JSE_COMPILER)
#  define JSE_COMPILER 1
#endif
#if (0 == JSE_COMPILER)
   /* if compiler is off then many other options don't make sense either */
#  if 0 != JSE_TOKENSRC
#     error JSE_TOKENSRC invalid if not JSE_COMPILER
#  endif
#  if 0 == JSE_TOKENDST
#     error JSE_COMPILER requires that JSE_TOKENDST also be set
#  endif
#  if 0 != JSE_INCLUDE
#     error JSE_COMPILER does not allow for JSE_INCLUDE
#  endif
#  if 0 != JSE_DEFINE
#     error JSE_COMPILER does not allow for JSE_DEFINE
#  endif
#  if 0 != JSE_CONDITIONAL_COMPILE
#     error JSE_COMPILER does not allow for JSE_CONDITIONAL_COMPILE
#  endif
#  if 0 != JSE_TOOLKIT_APPSOURCE
#     error JSE_COMPILER does not allow for JSE_TOOLKIT_APPSOURCE
#  endif
#  if 0 != JSE_LINK
#     error JSE_COMPILER does not allow for JSE_LINK
#  endif
#endif

/* JSE_INLINES can let some functions be compiled in-line, via
 * macros. This can improve performance but uses more memory
 */
#if !defined(JSE_INLINES)
#  if defined(__JSE_DOS16__) || defined(__JSE_WIN16__)
#     define JSE_INLINES 0
#  else
#     define JSE_INLINES 1
#  endif
#endif
#if !defined(JSE_MIN_MEMORY)
#  if defined(__JSE_DOS16__)
#     define JSE_MIN_MEMORY 1
#  else
#     define JSE_MIN_MEMORY 0
#  endif
#endif
#if !defined(JSE_MULTIPLE_GLOBAL)
   /* multiple globals allows for the global variable to be changed */
#  define JSE_MULTIPLE_GLOBAL 1
#endif


#if defined(JSETOOLKIT_APP)
#  define JSE_WIN32_DECL             __declspec(dllimport)
#else /* defined(JSETOOLKIT_CORE) */
#  define JSE_WIN32_DECL             __declspec(dllexport)
#endif

#if defined(__JSE_UNIX__) || defined(__JSE_NWNLM__) \
 || defined(__JSE_390__) \
 || defined(__DJGPP__) || defined(__JSE_PSX__) \
 || defined(__JSE_PALMOS__)
#  define JSE_CFUNC
#  define JSE_PFUNC
#elif defined (__JSE_MAC__)
#  define JSE_CFUNC
#  define JSE_PFUNC        pascal
#else
#  define JSE_CFUNC        __cdecl
#  define JSE_PFUNC        __pascal
#endif

#if defined(__JSE_LIB__)
   /* the interpreter is a static library */
#  if   defined(__JSE_DOS16__)
#     define JSECALLSEQ(type)         type __far __cdecl
#  elif defined(__JSE_DOS32__) && !defined(__DJGPP__)
#     define JSECALLSEQ(type)         type __cdecl
#  elif defined(__JSE_OS2TEXT__)
#     define JSECALLSEQ(type)         type __cdecl
#  elif defined(__JSE_OS2PM__)
#     define JSECALLSEQ(type)         type __cdecl
#  elif defined(__JSE_WIN16__)
#     define JSECALLSEQ(type)         type __far __cdecl
#  elif defined(__JSE_WIN32__)
#     define JSECALLSEQ(type)         type __cdecl
#  elif defined(__JSE_CON32__)
#     define JSECALLSEQ(type)         type __cdecl
#  elif defined(__JSE_NWNLM__)
#     define JSECALLSEQ(type)         type
#  elif defined(__JSE_UNIX__) || defined(__DJGPP__)
#     define JSECALLSEQ(type)         type
#  elif defined(__JSE_MAC__)
#     define JSECALLSEQ(type)         type
#  elif defined(__JSE_PSX__) || defined(__JSE_PALMOS__)
#     define JSECALLSEQ(type)         type
#  elif defined(__JSE_EPOC32__)
#     define JSECALLSEQ(type)         type
#  else
#     error platform not defined
#  endif
#else
   /* the interpreter is a dynamic-linked library */
#  if   defined(__JSE_DOS16__)
#     define JSECALLSEQ(type)         type __far __cdecl
#  elif defined(__JSE_DOS32__)
#     define JSECALLSEQ(type)         type __cdecl
#  elif defined(__JSE_OS2TEXT__)
#     define JSECALLSEQ(type)         type __export __cdecl
#  elif defined(__JSE_OS2PM__)
#     define JSECALLSEQ(type)         type __export __cdecl
#  elif defined(__JSE_WIN16__)
#     define JSECALLSEQ(type)         type __export __far __cdecl
#  elif defined(__JSE_WIN32__) || defined(__JSE_CON32__)
#     if defined(__BORLANDC__) || defined(__WATCOMC__)
#        define JSECALLSEQ(type)         type __export __cdecl
#     else
#        define JSECALLSEQ(type)         JSE_WIN32_DECL type __cdecl
#     endif
#  elif defined(__JSE_NWNLM__)
#     define JSECALLSEQ(type)         type
#  elif defined(__JSE_UNIX__)
#     define JSECALLSEQ(type)         type
#  elif defined(__JSE_390__)
#     define JSECALLSEQ(type)         type
#  elif defined(__JSE_MAC__)
#     define JSECALLSEQ(type)         type
#  elif defined(__JSE_PSX__) || defined(__JSE_PALMOS__)
#     define JSECALLSEQ(type)         type
#  else
#     error platform not defined
#  endif
#endif

/*********************************
 *** ENUMERATED VARIABLE TYPES ***
 *********************************/
#if !defined(JSE_TYPE_BUFFER)
#  define JSE_TYPE_BUFFER 1
#endif

/* enumerate all possible type of jse variables */
typedef int jseDataType;
#  define  jseTypeUndefined  0
#  define  jseTypeNull       1
#  define  jseTypeBoolean    2
#  define  jseTypeObject     3
#  define  jseTypeString     4
#  define  jseTypeNumber     5
#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
#  define  jseTypeBuffer     6
#endif

/* enumerate treatment of variables returned from jse library functions */
typedef int jseReturnAction;
#  define jseRetTempVar       0
      /* This is a temporary variable and may be removed when it is popped */
#  define jseRetCopyToTempVar 1
      /* Create a temp var and copy to that; don't remove this variable */
#  define jseRetKeepLVar      2
      /* This LVar cannot be popped */


/* These correspond to ToBoolean(), et al in section 9 of the
 * ECMAScript document
 */
typedef int jseConversionTarget;
#  define jseToPrimitive  0
#  define jseToBoolean    1
#  define jseToNumber     2
#  define jseToInteger    3
#  define jseToInt32      4
#  define jseToUint32     5
#  define jseToUint16     6
#  define jseToString     7
#  define jseToObject     8

#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
#  define jseToBytes      9  /* Converts to a buffer type, the buffer
                              * containing the ra bytes (i.e. a number is the
                              * 80 bits of the number, NULL is a pointer (4
                              * bytes), etc.) Objects just give you the same
                              * as their ToString value, not the bytes it
                              * represents
                              */
#  define jseToBuffer    10  /* Converts to a string just like ToString, but
                              * the string i  0 in ascii (i.e. the number 10
                              * gives you "10"), and for unicode string
                              * characters, the upper byte is ignored. It IS
                              * null-terminated.
                              */
#endif

/***************************************************************
 *** HANDLES (ABSTRACTIONS) TO INTERNAL DATA REPRESENTATIONS ***
 ***************************************************************/

/* define some classes to use for stronger typechecking */
#if defined(__MWERKS__) && !defined(NDEBUG) && defined(JSETOOLKIT_APP)
   /* We cannot debug properly with blank structures */
   typedef void * jseContext;
   typedef void * jseVariable;
   typedef void * jseStack;
#else
#if defined(JSETOOLKIT_APP)
   struct Call { int call_unused; };
   struct Var { int var_unused; };
   struct jseCallStack { int jsecallstack_unused; };
#endif
typedef struct Call * jseContext;
   /* Calling context for parameters, returns, re-entrancy, and multitasking */
typedef struct Var * jseVariable;
   /* all jse variable types */
typedef struct jseCallStack * jseStack;
   /* temporary variables, and parameter passing */
#endif

/***********************************************************
 *** JSE VARIABLE CREATION, DEFINITIONS, AND DESTRUCTION ***
 ***********************************************************/

JSECALLSEQ(jseVariable) jseCreateVariable(jseContext jsecontext,
                                          jseDataType VType);
JSECALLSEQ(jseVariable) jseCreateSiblingVariable(jseContext jsecontext,
   jseVariable olderSiblingVar,
   JSE_POINTER_SINDEX elementOffsetFromOlderSibling);
JSECALLSEQ(jseVariable) jseCreateConvertedVariable(jseContext jsecontext,
   jseVariable variableToConvert,jseConversionTarget targetType);
JSECALLSEQ(jseVariable) jseCreateLongVariable(jseContext jsecontext,
                                              slong value);  /* shortcut */
JSECALLSEQ(void) jseDestroyVariable(jseContext jsecontext,jseVariable variable);
JSECALLSEQ(jseVariable)  jseFindVariable(jseContext jsecontext,
                                               const jsechar * name, ulong flags);
JSECALLSEQ(jsebool) jseGetVariableName(jseContext jsecontext, 
   jseVariable variableToFind, jsechar * const buffer, uint bufferSize);

#if !defined(JSE_CREATEFUNCTIONTEXTVARIABLE)
#  if defined(__JSE_DOS16__)
#     define JSE_CREATEFUNCTIONTEXTVARIABLE  0
#  else
#     define JSE_CREATEFUNCTIONTEXTVARIABLE  1
#  endif
#endif
#if defined(JSE_CREATEFUNCTIONTEXTVARIABLE) \
 && (0!=JSE_CREATEFUNCTIONTEXTVARIABLE)
#  if (0==JSE_COMPILER)
#     error JSE_CREATEFUNCTIONTEXTVARIABLE is incompatible if not JSE_COMPILER
#  endif
JSECALLSEQ( jseVariable ) jseCreateFunctionTextVariable(jseContext jsecontext,
                                                        jseVariable FuncVar);
   /* Returns a jseVariable of type jseTypeString that contains the text of the
    * function (ex: "function foo(a) { a=4; }") The variable provided must be
    * a function
    */
#endif

JSECALLSEQ(JSE_POINTER_UINDEX) jseGetArrayLength(jseContext jsecontext,
   jseVariable variable,JSE_POINTER_SINDEX *MinIndex);
   /* return Length, set MinIndex if not NULL */
JSECALLSEQ(void) jseSetArrayLength(jseContext jsecontext,jseVariable variable,
   JSE_POINTER_SINDEX MinIndex,JSE_POINTER_UINDEX Length);
   /* grow or shrink to this size */

/* Members of objects have the following properties */

typedef uword16 jseVarAttributes;
#  define  jseDefaultAttr       0x00
      /* Do nothing special */
#  define  jseDontEnum          0x01
      /* Don't enumerate(list when all are requested) this item. */
#  define  jseDontDelete        0x02
      /* Don't allow deletes on this element */
#  define  jseReadOnly          0x04
      /* Make this Read Only */
   /* These two only apply to functions */
#  define  jseImplicitThis      0x08
      /* add this to the scoping chain */
#  define  jseImplicitParents   0x10
      /* also add the prototype chain of this */

JSECALLSEQ(void) jseSetAttributes(jseContext jsecontext,jseVariable variable,
                                  jseVarAttributes attr);
JSECALLSEQ(jseVarAttributes) jseGetAttributes(jseContext jsecontext,
                                              jseVariable variable);

/********************************
 *** JSE VARIABLE DATA ACCESS ***
 ********************************/

JSECALLSEQ(jseDataType) jseGetType(jseContext jsecontext,jseVariable variable);
JSECALLSEQ(void) jseConvert(jseContext jsecontext,jseVariable variable,
                            jseDataType dType);
   /* False and print error and LibError is set; else True */
JSECALLSEQ(jsebool) jseAssign(jseContext jsecontext,jseVariable destVar,
                              jseVariable srcVar);
   /* False and print error; else True */

JSECALLSEQ(ubyte) jseGetByte(jseContext jsecontext,jseVariable variable);
JSECALLSEQ(void) jsePutByte(jseContext jsecontext,jseVariable variable,
                            ubyte byteValue);

JSECALLSEQ(slong) jseGetLong(jseContext jsecontext,jseVariable variable);
JSECALLSEQ(void) jsePutLong(jseContext jsecontext,jseVariable variable,
                            slong longValue);

JSECALLSEQ(jsebool) jseGetBoolean(jseContext jsecontext,jseVariable variable);
JSECALLSEQ(void) jsePutBoolean(jseContext jsecontext,jseVariable variable,jsebool boolValue);

JSECALLSEQ(void) jsePutNumber(jseContext jsecontext,jseVariable variable,
                              jsenumber number);
#if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
   /* because we cannot be assured that all compilers and link methods return
    * floats in the same way, we'll return them indirectly and let a local
    * version of the call in globldat.c handle it
    */
   JSECALLSEQ(void) jseGetFloatIndirect(jseContext jsecontext,
                                        jseVariable variable,
                                        jsenumber *GetFloat);
   jsenumber jseGetNumber(jseContext jsecontext,jseVariable variable);
#endif

typedef void _HUGE_ * jseHugeRetPtr;

JSECALLSEQ( const jsechar _HUGE_ * ) \
   jseGetString(jseContext jsecontext,jseVariable variable,
                JSE_POINTER_UINDEX *filled);
#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
   JSECALLSEQ( const void _HUGE_ * ) \
      jseGetBuffer(jseContext jsecontext,jseVariable variable,
                   JSE_POINTER_UINDEX *filled);
   JSECALLSEQ( void _HUGE_ * ) jseGetWriteableBuffer(jseContext jsecontext,
      jseVariable variable,JSE_POINTER_UINDEX *filled);
   JSECALLSEQ( void ) jsePutBuffer(jseContext jsecontext,
      jseVariable variable,const void _HUGE_ *data,
      JSE_POINTER_UINDEX size);
   JSECALLSEQ( JSE_POINTER_UINDEX ) jseCopyBuffer(jseContext jsecontext,
      jseVariable variable,void _HUGE_ *buffer,
      JSE_POINTER_UINDEX start,JSE_POINTER_UINDEX length);
#endif
JSECALLSEQ( jsechar _HUGE_ * ) jseGetWriteableString(jseContext jsecontext,
   jseVariable variable,JSE_POINTER_UINDEX *filled);
JSECALLSEQ( void ) jsePutString(jseContext jsecontext,
   jseVariable variable,const jsechar _HUGE_ *data);
JSECALLSEQ( void ) jsePutStringLength(jseContext jsecontext,
   jseVariable variable,const jsechar _HUGE_ *data,
   JSE_POINTER_UINDEX length);
JSECALLSEQ( JSE_POINTER_UINDEX ) jseCopyString(jseContext jsecontext,
   jseVariable variable,jsechar _HUGE_ *buffer,
   JSE_POINTER_UINDEX start,JSE_POINTER_UINDEX length);

/* The jseSetErrno API function has been rendered obsolete.  This macro
 * is kept for backwards compatibility
 */
#define jseSetErrno(jsecontext,errno_val)  \
   errno = (int) errno_val

JSECALLSEQ(jsebool) jseEvaluateBoolean(jseContext jsecontext,
                                       jseVariable variable);
   /* return boolean True or False */

JSECALLSEQ(jsebool) jseCompare(jseContext jsecontext,
   jseVariable variable1,jseVariable variable2,
   slong *CompareResult /* negative, zero, or positive long returned */);
   /* if error (comparing incomparable types) then error will have printed
    * and return False, else return True
    */


/* We have extended the jseCompare() function to act like the old function
 * but also be able to mimic two new functions which handle ECMA compares
 * much better. All old code will still work. You ought not to call
 * jseCompare() directly in this new way, instead using the new functions
 * jseCompareEquality() and jseCompareLess().
 */
#define JSE_COMPEQUAL ((slong *)-1)
#define JSE_COMPLESS  ((slong *)-3)


/* Test to see if the first variable is less than the second in the
 * ECMA definition. It is more than twice as fast as using jseCompare().
 * Note that you can do any relation in this way (greater than is less
 * with parameters swapped, greater than or equal is !less, etc.)
 *
 * Relational (<,>,<=,>=) are done differently than equality comparisons
 * in ECMAScript. Please see the ECMA document section 11.8.5 and 11.9.3
 * for the algorithms used.
 */
#define jseCompareLess(c,v1,v2) (jseCompare((c),(v1),(v2),JSE_COMPLESS))


/* In ECMAscript, equality is tested differently than relations. This
 * routine allows you test is two variables to see if they are the same
 * by this definition. jseCompare() will always give you an answer based
 * on the relational algorithm (see jseCompareLess() for a description
 * of the difference.) If you want to test if they are the same, use this
 * routine. This routine will test to see if two objects are the same
 * the way you would expect (i.e. do they point to the same object) whereas
 * jseCompare() tries to convert them to primitives and compare that way
 * as per ECMA relational comparisons.
 */
#define jseCompareEquality(c,v1,v2) (jseCompare((c),(v1),(v2),JSE_COMPEQUAL))


/******************************************************
 *** OBJECT VARIABLES AND THEIR MEMBER VARIABLES ***
 ******************************************************/

/* The above four functions are also available in an Ex form, where the extra
 * flags define more control over the variables returned.  If the
 * jseCreateVar flag is not set then the variable references are automatially
 * cleaned up when the current context (or callback function) returns.  The
 * above four functions are identical to these with the last parameter set to
 * jseDefault (i.e. 0).  The jseCreateVar can be used with either (or neither),
 * but jseLockRead or jseLockWrite are mutually exclusive. The LOCK flags lock
 * the variable reference for the given state until the variable is deleted
 * (either through jseDestroyVariable on jseCreateVar or through implicit
 * auto-cleanup delete when the local context is exited).
 */
typedef uword16 jseActionFlags;
#define jseCreateVar 0x1    /* ISDK user must explicitly jseDeleteVariable */
#define jseLockRead  0x4000 /* lock variable state for reading */
#define jseLockWrite 0x8000 /* lock variable state for writing */
#define jseDontCreateMember 0x08
#define jseDontSearchPrototype 0x10

#define jseGetMember(jsecontext,objectVariable,Name) \
        jseMemberEx(jsecontext ,objectVariable,Name,jseTypeUndefined,jseDontCreateMember)        
#define jseGetMemberEx(jsecontext,objectVar,Name,flags) \
        jseMemberEx(jsecontext,objectVar,Name,jseTypeUndefined,flags|jseDontCreateMember)
#define jseMember(jsecontext,objectVar,Name,DType) \
        jseMemberEx(jsecontext,objectVar,Name,DType,0)

#define jseGetIndexMember(jsecontext,objectVariable,index) \
        jseGetIndexMemberEx(jsecontext,objectVariable,index,jseDontCreateMember)
#define jseGetIndexMemberEx(jsecontext,objectVar,index,flags) \
        jseIndexMemberEx(jsecontext,objectVar,index,jseTypeUndefined,flags|jseDontCreateMember)
#define jseIndexMember(jsecontext,objectVar,index,DType) \
        jseIndexMemberEx(jsecontext,objectVar,index,DType,0)

JSECALLSEQ(jseVariable) jseMemberEx(jseContext jsecontext,
      jseVariable objectVar,
      const jsechar *Name,jseDataType DType,jseActionFlags flags);
JSECALLSEQ(jseVariable) jseIndexMemberEx(jseContext jsecontext,
      jseVariable objectVar,
      JSE_POINTER_SINDEX index,jseDataType DType,jseActionFlags flags);

JSECALLSEQ(jseVariable) jseGetNextMember(jseContext jsecontext,
   jseVariable objectVariable,jseVariable prevMemberVariable,
   const jsechar * * name);
   /* return next object member after PrevVariable.  If PrevVariable is NULL
    * then return first member.  Name will point to the variable name; DO NOT
    * ALTER NAME DATA
    */
JSECALLSEQ(void) jseDeleteMember(jseContext jsecontext,
   jseVariable objectVariable,const jsechar *name);
   /* remove the member of this object variable with the given name.  If name
    * is NULL then delete all members.  It is not an error to pass in a Name
    * that does not exist, in which case this function just returns without
    * doing anything.
    */

/******************************
 *** GLOBAL VARIABLE OBJECT ***
 ******************************/

JSECALLSEQ(jseVariable) jseGlobalObject(jseContext jsecontext);
   /* return the global object */

#if 0 != JSE_MULTIPLE_GLOBAL
   JSECALLSEQ(void) jseSetGlobalObject(jseContext jsecontext,jseVariable newGlobal);
#endif

JSECALLSEQ(jseVariable) jseActivationObject(jseContext jsecontext);
   /* return the local object for function being called; or NULL if
    * in global code area
    */

JSECALLSEQ(jseVariable) jseGetCurrentThisVariable(jseContext jsecontext);
   /* get current var referred to by "this" in function calls; the var passed
    * to jseCallFunction or the global
    */



/*********************
 *** THE JSE STACK ***
 *********************/

JSECALLSEQ(jseStack) jseCreateStack(jseContext jsecontext);
JSECALLSEQ(void) jseDestroyStack(jseContext jsecontext,jseStack stack);
JSECALLSEQ(void) jsePush(jseContext jsecontext,jseStack jsestack,
                         jseVariable var,jsebool DeleteVariableWhenFinished);
JSECALLSEQ(jseVariable)  jsePop(jseContext jsecontext, jseStack jsestack);


/***************************************************************
 *** ACCESS INPUT PARAMETERS FROM EXTERNAL LIBRARY FUNCTIONS ***
 ***************************************************************/

/* function parameter type-checking; specify type if input variable;
 * or dimension with the NEED type
 */
typedef uword32 jseVarNeeded;

#define JSE_VN_UNDEFINED   ( (jseVarNeeded)(1 << jseTypeUndefined) )
#define JSE_VN_NULL        ( (jseVarNeeded)(1 << jseTypeNull) )
#define JSE_VN_BOOLEAN     ( (jseVarNeeded)(1 << jseTypeBoolean) )
#define JSE_VN_OBJECT      ( (jseVarNeeded)(1 << jseTypeObject) )
#define JSE_VN_STRING      ( (jseVarNeeded)(1 << jseTypeString) )
#define JSE_VN_NUMBER      ( (jseVarNeeded)(1 << jseTypeNumber) )
#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
# define JSE_VN_BUFFER     ( (jseVarNeeded)(1 << jseTypeBuffer) )
#endif

#define JSE_VN_FUNCTION    ( (jseVarNeeded) 0x0080 )
   /* special test of object type AND callable function */
#define JSE_VN_BYTE        ( (jseVarNeeded) 0x0100 )
   /* special case of VN_NUMBER, not used for PREFER or CONVERT */
#define JSE_VN_INT         ( (jseVarNeeded) 0x0200 )
   /* special case of VN_NUMBER, not used for PREFER or CONVERT */
#define JSE_VN_COPYCONVERT ( (jseVarNeeded) 0x0400 )
   /* special flag to create a copy of this variable if it has to be
    * converted; might also add an auto-temp variable unless
    * JSE_VN_CREATEVAR is set
    */
#define JSE_VN_LOCKREAD    ( (jseVarNeeded) 0x1000 )
   /* var will be used only for reading. An auto-temp var unless JSE_VN_CREATEVAR */
#define JSE_VN_LOCKWRITE   ( (jseVarNeeded) 0x2000 )
   /* var will be used only for writing. An auto-temp var unless JSE_VN_CREATEVAR */
#define JSE_VN_CREATEVAR   ( (jseVarNeeded) 0x0800 )
   /* if JSE_VN_LOCKREAD or JSE_VN_LOCKWRITE then this puts the caller
    * in charge of destroying this variable reference, else it will
    * be destroyed automatically when wrapper function returns
    */

#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
#  define JSE_VN_ANY \
      (JSE_VN_UNDEFINED|JSE_VN_NULL|JSE_VN_BOOLEAN|JSE_VN_OBJECT\
       |JSE_VN_STRING|JSE_VN_BUFFER|JSE_VN_NUMBER)
#else
#  define JSE_VN_ANY \
      (JSE_VN_UNDEFINED|JSE_VN_NULL|JSE_VN_BOOLEAN|JSE_VN_OBJECT\
       |JSE_VN_STRING|JSE_VN_NUMBER)
#endif
#define JSE_VN_NOT(UNWANTED_TYPES)  ((JSE_VN_ANY)&(~(UNWANTED_TYPES)))

#define JSE_VN_CONVERT(VN_FROM_TYPES,VN_TO_TYPE) \
   (VN_TO_TYPE|(((uword32)VN_TO_TYPE)<<16)|(((uword32)VN_FROM_TYPES)<<24))

JSECALLSEQ(uint) jseFuncVarCount(jseContext jsecontext);
JSECALLSEQ(jseVariable) jseFuncVar(jseContext jsecontext,uint ParameterOffset);
   /* return NULL for error; message already printed and error flag already set
    * will only return NULL if ParameterOffset is invalid, and so if you
    * know offset is valid (e.g., by FunctionList min and max) then you
    * don't need to check for NULL.
    */
JSECALLSEQ(jseVariable) jseFuncVarNeed(jseContext jsecontext,
                                       uint parameterOffset,
                                       jseVarNeeded need);
   /* return NULL for error; message already printed and error flag already
    * set; no data tying, not even dimension, is checked if need==0 (i.e.,
    * no bits set in need)
    */
JSECALLSEQ(jsebool) jseVarNeed(jseContext jsecontext,
                               jseVariable variable,jseVarNeeded need);
   /* similar to FuncVarNeed but already have variable */

/* The macro below is convenient when initializing library functions
 * to get the variables from the stack.  If there is an error then it
 * automatically sets error flags and then return from the function.
 * CAUTION! If there is an error then the statements following these
 * macros are not executed, and so don't use these if you need to perform
 * cleanup. Note that the 'C' version must have already declared the variable
 * uses as the first parameter.
 */
#define JSE_FUNC_VAR_NEED(varname,context,ParameterOffset,need) \
   if ( NULL == (varname = jseFuncVarNeed(context,ParameterOffset,need)) ) \
      return

/*****************************************************************
 *** METHODS TO RETURN VARIABLE FROM EXTERNAL LIBRARY FUNCTION ***
 *****************************************************************/

JSECALLSEQ(void) jseReturnVar(jseContext jsecontext,jseVariable variable,
                              jseReturnAction RetAction);
JSECALLSEQ(void) jseReturnLong(jseContext jsecontext,slong longValue);
JSECALLSEQ(void) jseReturnNumber(jseContext jsecontext,jsenumber number);

/**********************************************************
 *** ENGINE: INITIALIZE AND TERMINATE ENGINE ONLY ONCE. ***
 *** GLOBALY CALL BEFORE/AFTER *ANY OTHER* CALLS        ***
 **********************************************************/
#if defined(JSE_MEM_DEBUG) && (0!=JSE_MEM_DEBUG) && defined(__JSE_LIB__)
   /* alter version so core-debug doesn't link with non-core debug */
#  define JSE_ENGINE_VERSION_ID    (410 | 0x8000)
#  define JSE_VERSION_STRING       "A - JSE_MEM_DEBUG"
#else
#  define JSE_ENGINE_VERSION_ID    410
#  define JSE_VERSION_STRING       "A"   /* Minor version string */
#endif

JSECALLSEQ(uint) jseInitializeEngine(void);
   /* Call this before any other call in the jse toolkit. Return ID of engine
    * for version # verification.
    */
JSECALLSEQ(void) jseTerminateEngine(void);
   /* this must be that absolute last call. No more toolkit functions may be
    * called after this one.
    */



/***********************************************************
 *** EXTERNAL LINK: FOR ANY CREATED TOP-LEVEL JSECONTEXT ***
 *** INITIALIZE EXTERNAL LINK MUST BE THE FIRST CALL AND ***
 *** TERMINATE EXTERNAL LINK MUST BE THE LAST TO USE THE ***
 *** JSECONTEXT..  LINKDATA IS ALWAYS AVAILABLE          ***
 ***********************************************************/

typedef void    (JSE_CFUNC FAR_CALL *jseErrorMessageFunc)\
   (jseContext jsecontext,const jsechar *ErrorString);
typedef jsebool (JSE_CFUNC FAR_CALL *jseMayIContinueFunc)(jseContext);
typedef jsebool (JSE_CFUNC FAR_CALL *jseFileFindFunc)(jseContext,
   const jsechar * FileSpec, jsechar * FilePathResults,
   uint FilePathLen,jsebool FindLink);
   /* FindLink is True for finding file from #link statement */
typedef jseContext (JSE_CFUNC FAR_CALL *jseAppLinkFunc)\
   (jseContext jsecontext,jsebool Initialize);
   /* can be used to let the application create a new context initialized wih
    * globas, libs, defines, etc.., maybe in new thread
    */

   /* enumerate all possible type of jse variables */
   typedef int jseToolkitAppSourceFlags;
   #define  jseNewOpen      1
   #define  jseGetNext      2
   #define  jseClose        3

   struct jseToolkitAppSource
   {
      jsechar *           code;      /* toolkit app sets this value */
      const jsechar * const name;      /* toolkit app cannot write to this value */
      uint               lineNumber;/* app and core both can read/write */
      void *             userdata;  /* toolkit app uses this as it pleases */
   #  if defined(__cplusplus)
         /* C++ demands constructor for the const field */
         jseToolkitAppSource() : name((const jsechar * const)0) { }
         ~jseToolkitAppSource() { }
   #  endif
   };

   typedef jsebool (JSE_CFUNC FAR_CALL *jseGetSourceFunc)\
      (jseContext jsecontext,struct jseToolkitAppSource * ToolkitAppSource,\
       jseToolkitAppSourceFlags flag);

typedef uword32  jseLinkOptions;

struct jseExternalLinkParameters {
  /* required: */
   jseErrorMessageFunc          PrintErrorFunc;

  /* for optional file I/O */
     jseFileFindFunc              FileFindFunc;
     jseGetSourceFunc             GetSourceFunc;

  /* optional: set to NULL if not wanted */
   jseMayIContinueFunc          MayIContinue;
   jseAppLinkFunc               AppLinkFunc;
   
   const jsechar      * jseSecureCode;
   
   jseLinkOptions  options; /* flags for different behaviors */

   /* hashtable size refers to number of hash entries in table for strings
    * representing variable names (objects, properties).  use 0 for default.
    * if underlying interpreter was built without the hash option then
    * this field will be ignored.
    */
   uint        hashTableSize;
};
#define jseOptDefault               0     
   /* all default behavior */
#define jseOptReqVarKeyword      0x01  
   /* "var" keyword required for all variables */
#define jseOptDefaultLocalVars   0x02  
   /* default local vars if not global */
#define jseOptReqFunctionKeyword 0x04  
   /* "function" or "cfunction" keyword required */
#if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
#  define jseOptDefaultCBehavior   0x08  
   /* C function behavior by default */
#endif
#define jseOptWarnBadMath        0x10  
   /* treat NaN results and div-by-0 as errors */
#define jseOptLenientConversion  0x20
   /* Convert any type of parameter with JSE_VN_CONVERT
    * Allow getting of any data type from any type of var 
    */
#define jseOptIgnoreExtraParameters 0x40
   /* Ignore extra parameters to library functions */

JSECALLSEQ(jseContext) jseInitializeExternalLink(
#if defined(JSETOOLKIT_LINK)
              jseContext jsecontext,
#endif
              void _FAR_ *LinkData,
              struct jseExternalLinkParameters * LinkParms,
              const jsechar * globalVarName,
              const char * AccessKey
          );
   /* First call to get the initial top-level context.  This may be
    * called multiple times after jseInitializeEngine for various
    * contexts in threads or multiple levels of interpretation.
    * GlobalObjectName is the name of the global object.
    * AccessKey is your Nombas-supplied string for identification with
    * this library.
    */
JSECALLSEQ(void) jseTerminateExternalLink(jseContext jsecontext);
   /* Final call. No calls available after this.  Any context valid in
    * this function is valid for this call.
    */
JSECALLSEQ(void _FAR_ *) jseGetLinkData(jseContext jsecontext);
   /* get the arbitrary data element available to your link */
JSECALLSEQ( struct jseExternalLinkParameters * )\
   jseGetExternalLinkParameters(jseContext jsecontext);
  /* Get the current set of User specified parameters */

JSECALLSEQ(jseContext) jseAppExternalLinkRequest(jseContext jsecontext,
                                                 jsebool Initialize);
   /* Get a new jsecontext from the toolkit application, initialized
    * as the toolkit application chooses to initialize apps, with its
    * own calls to jseInitializeExternalLink, initializing libraries, etc...
    * If Initialize is False then this is to terminate a link create with
    * earlier call, and jsecontext is context returned from that earlier call.
    * PreviousLinkContext Application can return NULL if it won't support this
    * Engine will return NULL if AppInitLinkFunc in jseExternalLinkParameters
    * is null for PreviousLinkContext
    */

/*******************************************
 *** DEFINING EXTERNAL LIBRARY FUNCTIONS ***
 *******************************************/

/* define extra var attributes for function definition table */
typedef uword16  jseFuncAttributes;
#  define jseFunc_Default          0x00
      /* want default ECMAScript behaviour */
#  define jseFunc_PassByReference  0x80
      /* want to pass all lvalue variables by reference */
#  define jseFunc_CBehavior        jseFunc_PassByReference  
      /* this flag is now deprecated, but it must be kept for backwards
       * compatibility.  Use jseFunc_PassByReference instead
       */
#  define jseFunc_Secure    0x40
      /* this function is safe to call; else is a security risk; this is
       * only used if JSE_SECUREJSE but must always be defined so that
       * external links have the right number of charactrs
       */
   /* the following values are wrapped into the JSE_FUNC macros - you don't
    * need to use them explicitly; these types tell just what type of entry
    * this is in the object table (note that these cannot be ORed together)
    */
#  define jseFunc_FuncObject       1
      /* this is a function object */
#  define jseFunc_ObjectMethod     2
      /* this is an object of previously-defined function object */
#  define jseFunc_PrototypeMethod  3
      /* method of the .prototype of previously-defined function object */
#  define jseFunc_AssignToVariable 4
      /* specify "string.something.other" to assign to an existing object */
#  define jseFunc_LiteralValue     5
      /* function pointer is really a literal string to assign as property */
#  define jseFunc_LiteralNumberPtr 6
      /* function pointer is really a literal string to assign as property */
#  define jseFunc_SetAttributes    7
      /* only set the attributes on this object */

typedef void (JSE_CFUNC FAR_CALL *jseLibraryFunction)(jseContext jsecontext);

struct jseFunctionDescription {
   /* define jse function that can be called by interpreted jse code */
   const jsechar *FunctionName;  /* list ends when this is NULL */
   jseLibraryFunction FuncPtr;
   sword8 MinVariableCount, MaxVariableCount; /*-1 for no max */
   jseVarAttributes VarAttributes;   /* bitwise-OR jseVarAttributes */
   jseFuncAttributes FuncAttributes;  /* bitwise-OR */
};

/* OLD TEMPORARY MACROS - MAKE THESE GO AWAY */
#define JSE_FUNC_DESC(NAME,ADDR,MINCT,MAXCT,PASS_BY_REF,SAFE) \
   { NAME, ADDR, MINCT, MAXCT, jseDontEnum, \
     ( (PASS_BY_REF) ? jseFunc_PassByReference : 0 ) \
     | ( (SAFE) ? jseFunc_Secure : 0 ) \
     | jseFunc_ObjectMethod }
#define JSE_FUNC_DESC_END  JSE_FUNC(NULL,NULL,0,0,0,0)

#define JSE_FUNC(NAME,ADDR,MINCT,MAXCT,VARATTR,FUNCATTR) \
   { NAME, ADDR, MINCT, MAXCT, VARATTR, FUNCATTR }
#define JSE_LIBOBJECT(NAME,ADDR,MINCT,MAXCT,VARATTR,FUNCATTR) \
   { NAME, ADDR, MINCT, MAXCT, VARATTR, FUNCATTR | jseFunc_FuncObject }
#define JSE_LIBMETHOD(NAME,ADDR,MINCT,MAXCT,VARATTR,FUNCATTR) \
   { NAME, ADDR, MINCT, MAXCT, VARATTR, FUNCATTR | jseFunc_ObjectMethod }
#define JSE_PROTOMETH(NAME,ADDR,MINCT,MAXCT,VARATTR,FUNCATTR) \
   { NAME, ADDR, MINCT, MAXCT, VARATTR, FUNCATTR | jseFunc_PrototypeMethod }
#define JSE_VARASSIGN(NAME,CONST_VARNAME,VARATTR) \
   { NAME, (jseLibraryFunction)CONST_VARNAME, 0, 0, VARATTR, \
     jseFunc_AssignToVariable }
#define JSE_VARSTRING(NAME,CONST_VAR_STRING,VARATTR) \
   { NAME, (jseLibraryFunction)CONST_VAR_STRING, 0, 0, VARATTR, \
     jseFunc_LiteralValue }
#define JSE_VARNUMBER(NAME,CONST_VAR_NUMBER,VARATTR) \
   { NAME, (jseLibraryFunction)CONST_VAR_NUMBER, 0, 0, VARATTR, \
     jseFunc_LiteralNumberPtr }
#define JSE_ATTRIBUTE(NAME,VARATTR) \
   { NAME, NULL, 0, 0, VARATTR, jseFunc_SetAttributes }
#define JSE_FUNC_END  JSE_FUNC(NULL,NULL,0,0,0,0)

typedef void _FAR_ * (JSE_CFUNC FAR_CALL *jseLibraryInitFunction)\
   (jseContext jsecontext,void _FAR_ *PreviousInstanceLibraryData);
   /* PreviousInstanceLibraryData was returned by a previous instance of
    * initializing this library (sub-instances may be created with new
    * calls to interpret()), or was the object used in jseAddLibrary if
    * this is the first call.
    */
typedef void (JSE_CFUNC FAR_CALL *jseLibraryTermFunction)(
   jseContext jsecontext,void _FAR_ *InstanceLibraryData);
JSECALLSEQ(void _FAR_ *) jseLibraryData(jseContext jsecontext);
   /* following call to LibraryInitFunction this will always
    * return the value returned by LibraryInitFunction.
    */
JSECALLSEQ(void) jseAddLibrary(jseContext jsecontext,
   const jsechar * object_var_name/*NULL for global object*/,
   const struct jseFunctionDescription *FunctionList,
   void _FAR_ *InitLibData,
   jseLibraryInitFunction LibInit,jseLibraryTermFunction LibTerm);
   /* if LibInit or LibTerm are NULL then not called. */



/**********************************************************
 *** WORKING WITH OTHER INTERNAL AND EXTERNAL FUNCTIONS ***
 **********************************************************/

JSECALLSEQ(jseVariable) jseGetFunction(jseContext jsecontext,
   jseVariable object,const jsechar *functionName,
   jsebool errorIfNotFound);
   /* Return NULL if not found; If ErrorIfNotFound then also
    * call Oops() to print error;
    */
JSECALLSEQ(jsebool) jseIsFunction(jseContext jsecontext,
                                  jseVariable functionVariable);
   /* Return False if this variable is not a valid callable function object. */
JSECALLSEQ(jsebool) jseCallFunction(jseContext jsecontext,
   jseVariable jsefunc,jseStack jsestack,
   jseVariable *returnVar,jseVariable thisVar);
   /* ThisVar will be object used for any use of this.;
    * If NULL then will use the global ThisVar
    */
JSECALLSEQ(jseContext) jsePreviousContext(jseContext jsecontext);
   /* may be NULL */
JSECALLSEQ(jseContext) jseCurrentContext(jseContext ancestorContext);
   /* not NULL */
   /* Return the current context for the the current thread of execution. If
    * you're being called by the jse interpreter then this will be the same
    * value passed to your function in the context parameter and this call is
    * not needed (and should be avoided because of its extra overhead) but if
    * you're in a callback function then you can use this.  If you know the
    * ancestor jsecontext, such as from InitializejseLink, then this will
    * return the context currently in use (descendat of AncestorContext).
    * This is intended only for use by callback or interrupt-like functions.
    */
JSECALLSEQ(jseVariable) jseCreateWrapperFunction(jseContext jsecontext,
   const jsechar *functionName,
   jseLibraryFunction funcPtr,
   sword8 minVariableCount, sword8 maxVariableCount, /* -1 for no max */
   jseVarAttributes varAttributes,   /* bitwise-OR jseVarAttributes */
   jseFuncAttributes funcAttributes,  /* bitwise-OR */
   void _FAR_ *fData);
      /* fData is available to the function through jseLibraryData() */

JSECALLSEQ(jseVariable) jseMemberWrapperFunction(jseContext jsecontext,
   jseVariable objectVar,
      /* object to add function to (as a member), NULL for global object */
   const jsechar *functionName,
   jseLibraryFunction funcPtr,
   sword8 minVariableCount, sword8 maxVariableCount, /* -1 for no max */
   jseVarAttributes varAttributes,   /* bitwise-OR jseVarAttributes */
   jseFuncAttributes funcAttributes,  /* bitwise-OR */
   void _FAR_ *fData);   /* available to function through jseLibraryData() */

JSECALLSEQ(jsebool) jseIsLibraryFunction(jseContext jsecontext,
                                         jseVariable functionVariable);

/************************************************
 *** #DEFINE STATEMENTS DURING INITIALIZATION ***
 ************************************************/


#if defined(JSE_DEFINE) && (0!=JSE_DEFINE)
   JSECALLSEQ(void) jsePreDefineLong(jseContext jsecontext,
                                     const jsechar *FindString,
                                     slong ReplaceL);
#  if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
      JSECALLSEQ(void) jsePreDefineNumber(jseContext jsecontext,
                                          const jsechar *findString,
                                          jsenumber replaceF);
#  endif
   JSECALLSEQ(void) jsePreDefineString(jseContext jsecontext,
                                       const jsechar *FindString,
                                       const jsechar *ReplaceString);
#endif

/***************************
 ******** Interpret ********
 ***************************/

typedef int jseInterpretMethod;
#define JSE_INTERPRET_LOAD             0x04
  /* this flag is deprecated. */
#define JSE_INTERPRET_NO_INHERIT       0x01
/* set the interpret up so local variables and with() scopes don't
 * apply. It only makes a differnce if the local variable context isn't NULL
 */
#define JSE_INTERPRET_CALL_MAIN        0x02
   /* call main(argc,argv) after runnnig initialization code */
#define JSE_INTERPRET_DEFAULT          0x00
   /* none of the above flags set; default behavior */

typedef int jseNewContextSettings;
#  define jseNewNone           0x00
#  define jseNewDefines        0x01
#  define jseNewGlobalObject   0x02
#  define jseNewFunctions      0x04
#  define jseNewLibrary        0x08
#  define jseNewAtExit         0x10
#  define jseNewSecurity       0x20
#  define jseNewExtensionLib   0x40
/* Don't include jseNewFunctions, 'cause that it something that is beyond
 * 'make everything new' - it is a special behaivor that should only be
 * explicitly used
 */
#  define jseAllNew            0x7B

JSECALLSEQ(jsebool) jseInterpret(jseContext jsecontext,
   const jsechar * SourceFile,
      /* NULL if pure text; token buffer if JSE_INTERPRET_PRETOKENIZED */
   const jsechar * SourceText,
      /* text or options if SourceFile */
   const void * PreTokenizedSource,
      /*NULL or data is already precompiled as in jseCreateCodeTokenBuffer */
   jseNewContextSettings NewContextSettings,
   jseInterpretMethod howToInterpret,
      /* flags, may be JSE_INTERPRET_xxxx */
   jseContext localVariableContext,
      /* NULL if not inherit local variables */
   jseVariable *returnVar);

   JSECALLSEQ( jseContext )
jseInterpInit(jseContext jsecontext,
              const jsechar * SourceFile,
              const jsechar * SourceText,
              const void * PreTokenizedSource,
              jseNewContextSettings NewContextSettings,
              jseInterpretMethod howToInterpret,
              jseContext localVariableContext);

   JSECALLSEQ( jseVariable )
jseInterpTerm(jseContext jsecontext);

   JSECALLSEQ( jseContext )
jseInterpExec(jseContext jsecontext);

/********************************
 ******** Tokenized Code ********
 ********************************/

   typedef void *  jseTokenRetBuffer;
#if defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC)
   JSECALLSEQ( jseTokenRetBuffer) jseCreateCodeTokenBuffer(
      jseContext jsecontext,
      const jsechar *source,
      jsebool sourceIsFileName/*else is source string*/,
      uint *bufferLen);
       /* returns buffer or NULL if error; buffer must be freed by the
        * caller. if return non-NULL then *BufferLen is set to length
        * of data in the buffer
        */
   
   JSECALLSEQ( void )  jseDestroyCodeTokenBuffer(jseContext jsecontext,
                                                 jseTokenRetBuffer buffer);
#endif

/*********************
 *** MISCELLANEOUS ***
 *********************/

JSE_POINTER_UINDEX  jseGetNameLength(jseContext jsecontext, 
                                     const jsechar * name );
const jsechar * jseSetNameLength(jseContext jsecontext, const jsechar * name,
                                 JSE_POINTER_UINDEX length);

typedef void (JSE_CFUNC FAR_CALL * jseAtExitFunc)(jseContext jsecontext,
                                                  void _FAR_ *Param);
JSECALLSEQ(jsebool) jseCallAtExit(jseContext jsecontext,
                               jseAtExitFunc exitFunction,void _FAR_ *Param);
   /* call this function at exit time with the Param parameter */

JSECALLSEQ(void) jseLibSetErrorFlag(jseContext jsecontext);
   /* set flag that there has been an error */
JSECALLSEQ(void) jseLibErrorPrintf(jseContext exitContext,
                                   const jsechar * formatS,...);
   /* print error; set error flag in context */

JSECALLSEQ(void) jseLibSetExitFlag(jseContext jsecontext,
                                   jseVariable ExitVariable);
   /* Sets exit flag for this jsecontext, and saves copy of exit variable
    * (or ExitVariable may be NULL for return EXIT_SUCCESS) */

JSECALLSEQ(uint) jseQuitFlagged(jseContext jsecontext);
   /* return 0 if a call has been made on this context to Exit or for
    * one of the above error functions.  Return one of the following
    * non-0 (non-False) defines if should exit.
    */
#  define JSE_CONTEXT_ERROR  1
#  define JSE_CONTEXT_EXIT   2

JSECALLSEQ( const jsechar * ) jseLocateSource(jseContext jsecontext,
                                              uint *lineNumber);
   /* Return pointer to the name of the source file for the code currently
    * executing, and set LineNumber to the current line number executing
    * or being parsed.
    * Do not alter the returned string.  If no current file, such as when
    * interpreting string (e.g. not INTERPRET_JSE_FILE), then return NULL.
    */
JSECALLSEQ( const jsechar * ) jseCurrentFunctionName(jseContext jsecontext);
   /* return pointer to name of the current function; don't write into this
    * memory; NULL if no current function name
    */

#if !defined(JSE_GETFILENAMELIST)
#  if defined(__JSE_DOS16__)
#     define JSE_GETFILENAMELIST  0
#  else
#     define JSE_GETFILENAMELIST  1
#  endif
#endif
#if defined(JSE_GETFILENAMELIST) && (0!=JSE_GETFILENAMELIST)
   JSECALLSEQ(jsechar * *) jseGetFileNameList(jseContext jsecontext,
                                                int *number);
     /* Get a list of the files currently opened by the interpreter */
#endif

#if !defined(JSE_BREAKPOINT_TEST)
#  if defined(__JSE_DOS16__)
#     define JSE_BREAKPOINT_TEST  0
#  else
#     define JSE_BREAKPOINT_TEST  1
#  endif
#endif
#if defined(JSE_BREAKPOINT_TEST) && (0!=JSE_BREAKPOINT_TEST)
JSECALLSEQ(jsebool) jseBreakpointTest(jseContext jsecontext,
                                      const jsechar *FileName,
                                      uint LineNumber);
  /* To help debugger. Test if this is valid place to break. Check if
   * currently-running script thinks it has a breakpoint in this file
   * at this line number.  True if breakpoint, else False.
   */
#endif

#if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
JSECALLSEQ(jsebool) jseTellSecurity(jseContext jsecontext,jseVariable infoVar);
   /* pass this variable as the second parameter to jseSecurityInit()
    * Will return FALSE (but no error or message will have been printed) if no
    * security script is running, else True
    */
#endif

#if ( 0 < JSE_API_ASSERTLEVEL )
   JSECALLSEQ( const jsechar * ) jseGetLastApiError(void);
   JSECALLSEQ( void ) jseClearApiError(void);
#  define jseApiOK  ( 0 == (jseGetLastApiError())[0] )
   /* These API calls are for diagnostic purposes.  They need never take
    * a context - that would defeat the purpose that you can get at an
    * error to see that your context is invalid, Null etc.  The calls do
    * not protect the error buffer from being written to by multiple threads.
    * The jseApiOK macro is here for convenient placement in assert statements
    * such as:  assert( jseApiOK )
    */
#else
#  define jseApiOK True
   /* Because warnings are off this will pretend that there were no API
    * errors, and so allow "assert(jseApiOK)" to be in code that may
    * or may not check for errors.  Be warned that this does not mean
    * there were no errors, and so do not get a false sense of security
    * from this definition when warning levels are 0.
    */
#endif

#if defined(JSETOOLKIT_LINK)
#  if defined(__JSE_WIN16__)
#      define jseLibFunc(FuncName)    \
     void _FAR_ _cdecl _export FuncName(jseContext jsecontext)
#  elif defined(__JSE_WIN32__) || defined(__JSE_CON32__)
#     if !defined(_MSC_VER)
#        define jseLibFunc(FuncName)  \
     void _cdecl _export FuncName(jseContext jsecontext)
#     else
#        define jseLibFunc(FuncName)   \
     _declspec(dllexport) void _cdecl FuncName(jseContext jsecontext)
#     endif
#  elif defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
#     if defined(__IBMCPP__)
#        define jseLibFunc(FuncName)  \
     void _Export FuncName(jseContext jsecontext)
#     else
#        define jseLibFunc(FuncName)   \
     void _export _cdecl FuncName(jseContext jsecontext)
#     endif
#  elif defined(__JSE_UNIX__)
#     define jseLibFunc(FuncName)    \
    void FuncName(jseContext jsecontext)
#  elif defined(__JSE_NWNLM__)
#     define jseLibFunc(FuncName)    \
    void JSE_CFUNC _FAR_ FuncName(jseContext jsecontext)
#  elif defined(__JSE_MAC__)
#     define jseLibFunc(FuncName)    \
    void FuncName(jseContext jsecontext)
#  else
#    error define the extension qualifiers
#  endif
#else
#  define jseLibFunc(FuncName) \
      void JSE_CFUNC FuncName (jseContext jsecontext)
#endif

/* These are kept for backwards compatibility, but they mean the same thing */
#define jseExtensionLibFunc     jseLibFunc
#define InternalLibFunc         jseLibFunc


#if defined(JSETOOLKIT_LINK) && defined(__JSE_WIN16__)
#  define jseLibInitFunc(FuncName) \
      void * JSE_CFUNC _export FAR_CALL FuncName \
             (jseContext jsecontext, void _FAR_ *PreviousInstanceData)
#  define jseLibTermFunc(FuncName) \
      void JSE_CFUNC _export FAR_CALL FuncName  \
             (jseContext jsecontext, void _FAR_ *InstanceLibraryData)
#else
#  define jseLibInitFunc(FuncName) \
      void * JSE_CFUNC FAR_CALL FuncName \
             (jseContext jsecontext, void _FAR_ *PreviousInstanceData)
#  define jseLibTermFunc(FuncName) \
      void JSE_CFUNC FAR_CALL FuncName \
             (jseContext jsecontext, void _FAR_ *InstanceLibraryData)
#endif

/* garbage collection - The garbage collection routines are, by default
 * turned off because with the core built with JSE_CYCLIC_CHECK there is
 * very little chance that any garbage is left behind (at the expense of
 * running up to 10% slower), and with JSE_FULL_CYCLIC_CHECK there is no
 * chance of garbage left behind (at the expense of runnning much
 * slower).  So the following section is off by default, but if turned
 * on then it allows the application to force a garbage collection at any
 * time.  Note that garbage collection here is not for any use of any
 * variable (it's still up to the application to destroy variables it
 * has created) but for the case where objects may have cyclicly refered
 * to themselves and not been cleanup up.
 */
#if !defined(JSE_CYCLIC_GC)
#  define JSE_CYCLIC_GC 0
#endif
#if (0!=JSE_CYCLIC_GC)
   JSECALLSEQ(ulong) jseCyclicGC(jseContext jsecontext,jsebool Start);
      /* If Start then begin garbage collection, returning how many
       * orphaned cyclic loops were cleaned up.  If !Start then stop
       * any collection (for example, if another thread wants this
       * GC to stop, which won't be immediate but may be much faster
       * than letting it run to completion.
       */
#endif


#ifndef UNUSED_PARAMETER
#  if defined(__BORLANDC__)
#     define UNUSED_PARAMETER(PARAMETER)     /* ignore */
#  else
#     define UNUSED_PARAMETER(PARAMETER)       PARAMETER = PARAMETER
#  endif
#endif


#ifdef __cplusplus
   }
#endif

/* Return all struct packing to previous
*/
#if !defined(__JSE_UNIX__) && !defined(__JSE_MAC__) \
 && !defined(__JSE_PSX__) && !defined(__JSE_PALMOS__)
#  if defined(__BORLANDC__)
#     pragma option -a.
#  else
#     pragma pack( )
#  endif
#endif

#endif
