/* call.h  the main context structure
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

#ifndef _CALL_H
#define _CALL_H

/* need it for 'code_elem' */
#include "secode.h"

#if defined(JSE_LINK) && (0!=JSE_LINK)
#  include "selink.h"
#endif

struct Global_;

struct InvalidVarDescription
{
   jsechar VariableName[100]; /* set to \0 if cannot figure out a good name */
   jsechar VariableType[50];  /* put in message describing type of the variable */
   jsechar VariableWanted[200]; /* message describing type of variable needed */
};

/* IMPORTANT: This value MUST be an even number of bytes!!! */
#if (defined(JSE_MIN_MEMORY) && (0!=JSE_MIN_MEMORY)) \
 || ( defined(__JSE_WIN16__) || defined(__JSE_DOS16__) )
   typedef uword16 stringLengthType;
#else
   typedef uword32 stringLengthType;
#endif

#if defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS)

struct HashList
{
   uint   count;
#if defined(JSE_MEM_DEBUG) && (0!=JSE_MEM_DEBUG)
   jsechar * file;
   int line;
   int sequence;
#endif
   /* Storing this as a pointer to a pointer makes it easier to handle cases
    * where this is the first entry
    */
   struct HashList **prev;
   struct HashList * next;
   /* The string pointer is implicitly added after the end of the structure */
};

#endif

 

#if !defined(JSE_DYNAOBJ_HASHLIST) \
 && (defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS)) \
 && (defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS))
#  define JSE_DYNAOBJ_HASHLIST 1
#endif

#if ((!defined(JSE_DYNAMIC_OBJS) || (0==JSE_DYNAMIC_OBJS)) || \
     (!defined(JSE_HASH_STRINGS) || (0==JSE_HASH_STRINGS))) && \
     defined(JSE_DYNAOBJ_HASHLIST) && (0!=JSE_DYNAOBJ_HASHLIST)
#  error  JSE_DYNAOBJ_HASHLIST Requires dynamic objects and hash table
#endif
     

#if defined(JSE_DYNAOBJ_HASHLIST) && (0!=JSE_DYNAOBJ_HASHLIST) 
#  if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
#     define DYNAOBJ_COUNT 8
#  else
#     define DYNAOBJ_COUNT 7
#  endif
   struct DynamicHashList
   {
      struct HashList list;
      jsechar name[14 + sizeof(stringLengthType)/sizeof(jsechar)];   /* big enough for largest name */
      uword16  HasFlag; /* flag to set when objet has this property */
   };
#  define DynamicHashListFromName(name)                      \
      ((struct DynamicHashList *)(HashListFromName(name)))
void MoveDynaobjHashEntry(struct Global_ *global,uint offset,VarName *oldEntry,
                          uword16 attr);
void RestoreDynaobjHashEntry(struct Global_ *global,uint offset,VarName *oldEntry);
#endif



#if (0!=JSE_COMPILER)
   struct CompileStatus_
   {
      uint NowCompiling;
      const jsechar *CompilingFileName;
         /* while compiling, keep track of current file name for debugging */
      uint CompilingLineNumber;  /* while compiling remember line number */
   };
#endif

#define VARIABLE_CACHE_SIZE  32

struct variableCacheEntry {
   VarName  name;
   Var *    var;
};

/* Entries which cannot be initialized from static values go at the beginning */
#define userglobal_entry          0
/* All the rest can be initialized from static data */
#define init_function_entry       1
#define this_entry                2
#define constructor_entry         3
#define orig_prototype_entry      4
#define prototype_entry           5
#if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
#  define call_entry              6
#  define get_entry               7
#  define put_entry               8
#  define delete_entry            9
#  define canput_entry            10
#  define hasproperty_entry       11
#endif
#define defaultvalue_entry        12
#define preferred_entry           13
#define global_entry              14
#define arguments_entry           15
#define old_arguments_entry       16
#define callee_entry              17
#define caller_entry              18
#define length_entry              19
#define class_entry               20
#define array_entry               21
#define valueof_entry             22
#define tostring_entry            23
#define value_entry               24
#define parent_entry              25
#define main_entry                26
#define argc_entry                27
#define argv_entry                28
#if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
#  define operator_entry          29
#  define op_not_supported_entry  30
#endif

/* Do NOT alter these values without altering the associated table in call.c, and the
 * initialize functions
 */
#define GLOBAL_STRING_ENTRIES   31

#define GLOBAL_STRING(call,name_offset)  ((call)->Global->global_strings[name_offset])

struct Global_
{
   /* parameters that cannot change as context depth changes;
    * created only at top level
    */

   /* IMPORTANT - this must always be the first entry of this structure! */
#if defined(JSE_LINK) && (0!=JSE_LINK)
   struct jseFuncTable_t *jseFuncTable;
#endif

   struct secodeStack *thestack;
#if defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)
   struct variableCacheEntry * variableCache;
   sword16 maxCacheSize;
#endif
   /* The following is used to disable the cache in 'with' statements */
#if (defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)) || \
    (defined(JSE_CACHE_LOCAL_VARS) && (0!=JSE_CACHE_LOCAL_VARS))
   sword16 currentCacheSize;
#endif

#  if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
      struct Allocator allocate_var;
      struct Allocator allocate_varmem;
      struct Allocator allocate_call;
      struct Allocator allocate_tempvar;
      struct Allocator allocate_objmem;
#  endif

   /* strings are stored in a table, and referenced by their
    * 'jsechar *'. This makes sure that the same string has the
    * same pointer, so comparisons can be a simple pointer test
    * and memory is conserved.
    *
    * Start off with 100 items, since just inserting
    * the ECMAlib will use about 100 items. Also, each is 4 bytes, so
    * 400 bytes is not an unreasonable memory requirement.
    */
#  if defined(JSE_MIN_MEMORY) && (0!=JSE_MIN_MEMORY)
#     define INITIAL_STRING_TABLE_SIZE 10
#  else
#     define INITIAL_STRING_TABLE_SIZE 100
#  endif

#  if !defined(JSE_ONE_STRING_TABLE) || (0==JSE_ONE_STRING_TABLE)
#     if defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS)
         struct HashList ** hashTable;
         uint hashSize;
#     else
         jsechar **strings;
         uint stringsUsed,stringsAlloced;
#     endif
#  endif

#  if (0!=JSE_COMPILER)
      struct CompileStatus_ CompileStatus;
#  endif

   int number;
#  if defined(JSE_GETFILENAMELIST) && (0!=JSE_GETFILENAMELIST)
      jsechar **FileNameList;
#  endif

   struct Call *TopLevelCall;

   void _FAR_ * GenericData;
      /* this data may be specific to each implementation */
   struct jseExternalLinkParameters ExternalLinkParms;

   jsechar tempNumToStringStorage[12];
      /* when numbers are converted to strings they are converted here
       * and this data is returned.  Note that this means there is
       * one number used at a time per context thread.  This is big
       * enough for longest number, which is -2147483648.
       */

   /* all objects by default (if not HAS_PROTOTYPE) will get the prototype
    * from either Object.prototype or Function.protype.  These will already
    * be initialized.
    */
   VarRead *ObjectPrototype;
   VarRead *FunctionPrototype;

   /* These are stored pre-found to speed execution significantly. Each only
    * takes 4 bytes, and are stored once per context, so memory loss is
    * trivial, but performance really benefits - 28 entries = 112 bytes
    */

   VarName global_strings[GLOBAL_STRING_ENTRIES];
    
#if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
   VarRead * special_operator_var;
#endif

#  if defined(JSE_DYNAOBJ_HASHLIST) && (0!=JSE_DYNAOBJ_HASHLIST)
      struct DynamicHashList dynahashlist[DYNAOBJ_COUNT];
#  endif

#  if (defined(__JSE_WIN16__) || defined(__JSE_DOS16__)) && \
      (defined(__JSE_DLLLOAD__) || defined(__JSE_DLLRUN__))
      uword16 ExternalDataSegment;
#  endif

#  if (0!=JSE_CYCLIC_GC)
      struct Varmem *gcStartList;  /* a link of all varmems with object members */
      jsebool CollectingCyclicGarbage;
#  endif

#  if !defined(NDEBUG)
      ulong totalVarCount;
      ulong totalVarmemCount;
         /* count how many variables are in use.  Useful for debugging. */
#  endif
};

struct Session_
{
#  if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
      struct Security *SecurityGuard;
#  endif
   VarRead * GlobalVariable;
#  if defined(JSE_DEFINE) && (0!=JSE_DEFINE)
      struct Define *Definitions;
#  endif
   struct Library *TheLibrary;
   struct AtExit *AtExitFunctions;
#  if defined(JSE_LINK) && (0!=JSE_LINK)
      struct ExtensionLibrary *ExtensionLib;
#  endif
};

struct TempVar
{
   Var *var;    /* pointer to the variable this represents */
   uint count;  /* how many users of this temporary variable */
   struct TempVar *prev; /* point to variable previously added to this list */
};
#if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
#  define TEMP_VAR_ADD(VARLIST,VAR,SHARE,CALL) \
          tempvarAdd(VARLIST,VAR,SHARE,CALL)
#else
#  define TEMP_VAR_ADD(VARLIST,VAR,SHARE,CALL) \
          tempvarAdd(VARLIST,VAR,SHARE)
#endif
void NEAR_CALL tempvarAdd(struct TempVar **list,Var *var,jsebool ShareTempvars
#  if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
      ,struct Call *call
#  endif      
);
   /* add var to list; if Share is not false then will look if this is alread
    * in the list; else will add to the front of the list. (note that
    * this calls does not vadAddUser() on the variable.
    */
void NEAR_CALL tempvarRemoveTop(struct TempVar **list,struct Call *call);
   /* remove var from top of the list; also will remove_user */


#if defined(JSE_ONE_STRING_TABLE) && (0!=JSE_ONE_STRING_TABLE)
   void allocateGlobalStringTable();
   void freeGlobalStringTable();
#endif

struct Call
{
   /* IMPORTANT: The 'jseFuncTable' entry MUST be FIRST!!! The external
    *            libraries rely on this! Do NOT put anything else first.
    *            It is the first member of Global!!  (except for the
    *            cookie)
    */

   struct Global_ *Global;
      /* allocated only at the very top call level;
       * freed on last free; always copy parent
       */

#  if ( 2 <= JSE_API_ASSERTLEVEL )
      ubyte cookie;
#  endif

   struct Session_ session;
      /* possibly change at different levels;
       * copy from parent, then may override
       */

   VarName filename;
   uint linenum;
      /* this is how the secode interpreter keeps info for error reporting */

   struct Call *pPreviousCall;
   struct Call *pChildCall;
   uword8 CallSettings; /* save settings from initialization */
   jseReturnAction pReturnAction;

   /* the following parameters are for the passed-in variables */
   uint pInputVariableCount;
   uint stackMark;

   /* calling a function will set the current ThisVar */
   VarRead *pCurrentThisVar;


   struct Function *pFunction;
   VarRead *pReturnVar;
#  if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
      jsebool CBehaviorWanted;
#  endif

   struct TempVar *TempVars;
   struct TempVar *ScopeChain;

   jsebool ignore_past;

   VarRead *VariableObject;

   VarRead *call_variable;

   FlowFlag privReasonToQuit;
      /* this will be set to reason why call returned */
   VarRead *pExitVar;   /* If FlowExit is set, then will expect this to have
                         * ExitVar for variable to return to jseInterpret
                         */

   void _FAR_ *TempLibData;    /* Library data is stored here for temporary
                                * retrieval when a library function is about
                                * to be called.
                                */

   jsebool fudged;             /* used to allow destructors to access partially
                                * cleaned up global object.
                                */
   struct Varmem *save_varmem; /* ditto */


   /* ----------------------------------------------------------------------
    * To allow iterative execution, we need to store a lot more information
    * in the call. This first section is used by Interpret to store stuff so
    * it applies to the 'interpret' level.
    * ---------------------------------------------------------------------- */
   
   /* First, we store the old main/init function. If these are non-NULL, this
    * context was used to begin an interpret. Delete all children and it to
    * terminate the interpret
    */
   VarRead *oldMain,*oldInit;

   struct Call *LocalVariableCall;

   VarRead *saveexit;

#if defined(JSE_CACHE_LOCAL_VARS) && (0!=JSE_CACHE_LOCAL_VARS)
   uint localCacheLocation;
   uint localCacheCount;
#endif
   
   /* ----------------------------------------------------------------------
    * The 'function' level stuff.
    * ---------------------------------------------------------------------- */

   code_elem *sptr;          /* instruction pointer */
   uint with_count;          /* depth of 'with' */
   jsebool newfunc;
   jsebool no_clean;         /* don't do secode cleanup since init never done */
   VarRead *oldcaller;
   uint parentCount;
   VarRead *arg_obj;

   VarRead *lockFunc;        /* so function doesn't get deleted in middle of call */
};


struct Call * NEAR_CALL callNew(struct Call *PreviousCall,
                                struct Function *function,
                                uint InputVariableCount,
                                jseNewContextSettings NewContextSettings);
   /* remember function that initialized this context. if callerstack is NULL
    * then InputVariableCount better be 0.  NewSettings is flag with
    * OR of all NewCallSettings bits.
    */
void NEAR_CALL callDelete(struct Call *);

struct Call * callInitial(void _FAR_ *LinkData,
                          struct jseExternalLinkParameters *ExternalLinkParms,
                          const jsechar * globalVariableName,
                          stringLengthType GlobalVariableNameLength);
   /* this is only to be the top level call for any session. */

jsebool NEAR_CALL callInitialize(struct Call * call,
                              jseNewContextSettings NewContextSettings);

void NEAR_CALL callReturnVar(struct Call * call,struct Var *rawvar,
                             jseReturnAction Return);
   /* Put a safe copy of this return variable on top of the caller's stack. */
jseReturnAction NEAR_CALL callRetType(struct Call * call);
const jsechar * callCurrentName(struct Call * call);
struct Var * NEAR_CALL callGetVar(struct Call * call,uint InputVarOffset);
   /* return var from stack, starting at 0 and going up;
    * return NULL, message, and set FlowError if bad
    */
struct Call * callCurrent(struct Call *call);
struct Var * NEAR_CALL callGetVarNeed(struct Call * call,VarRead *pVar,
                                      uint InputVarOffset,jseVarNeeded need);
   /* return var from stack; return NULL if error (message already handled).
    * If var is not NULL then use that var and ignore InputVarOffset, else
    * use InputVarOffset to get variable off of the stack.
    */
void DescribeInvalidVar(struct Call * call,struct Var *v,jseDataType vType,
                        struct Function *FuncPtrIfObject,jseVarNeeded need,
                        struct InvalidVarDescription *BadDesc);
void callError(struct Call * call,enum textcoreID id,...);
   /* If not already FlowError then make it so, even if FlowExit.
    * If not already FlowError and TextID is not 0, then print error message.
    */

void NEAR_CALL callSetExitVar(struct Call * call,VarRead *var);
   /* set exit and exit variable if not already a reason to quit */
void callQuit(struct Call * call,enum textcoreID id,...);
   /* If !Quit() then set FlowError, else reason for error is
    * already FlowError or FlowExit or FlowEndThread.
    *
    * If !Quit() and TextID is not 0, then print error message.
    */
VarRead * NEAR_CALL callGetExitVar(struct Call * call);

#if !defined(NDEBUG)
   void callFatalID(struct Call * call,enum textcoreID id,...);
   void callFatalText(struct Call * call,const jsechar * FormatS,va_list arglist);
   void callFatal(struct Call * call,jsebool PrintMessage);
#endif

   /* Exits, with minor cleanup. Tries to restore the environment. */
void ErrorVPrintf(struct Call *call,const jsechar *FormatS,va_list arglist);
   /* print error and source line number */

struct Call * NEAR_CALL interpretInit(struct Call * call,const jsechar *OriginalSourceFile,
                           const jsechar * OriginalSourceText,
                           const void *PreTokenizedSource,
                           jseNewContextSettings NewContextSettings,
                           int HowToInterpret,struct Call *LocalVariableCall);
VarRead * NEAR_CALL interpretTerm(struct Call *call);

#if defined(JSE_GETFILENAMELIST) && (0!=JSE_GETFILENAMELIST)
   void NEAR_CALL callAddFileName(struct Call *call,jsechar *name);
#endif

/* Javascript scoping stuff */

#define AddScopeObject(CALL,VAR) \
        TEMP_VAR_ADD(&((CALL)->ScopeChain),VAR,False,(CALL))
#define RemoveScopeObject(CALL) tempvarRemoveTop(&((CALL)->ScopeChain),(CALL));

void NEAR_CALL CloneScopeChain(struct Call *call,struct Call *old);
void NEAR_CALL RemoveClonedScopeChain(struct Call *call,struct Call *old);

/* string table stuff */

#if defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS) && \
    (defined(JSE_MEM_DEBUG) && (0!=JSE_MEM_DEBUG))
VarName NEAR_CALL EnterIntoStringTableEx(struct Call *call,const jsechar *name,
                          stringLengthType length, const char * FILE, int line);
#define EnterIntoStringTable(call,name,length)  \
          EnterIntoStringTableEx(call,name,length,__FILE__,__LINE__)
#else
VarName NEAR_CALL EnterIntoStringTable(struct Call *call,const jsechar *name,
                          stringLengthType length);
#endif

const jsechar * NEAR_CALL GetStringTableEntry(struct Call *call,VarName entry,stringLengthType *length);
   /* pass length as NULL to ignore */

#if defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS)
   void NEAR_CALL reallyRemoveFromStringTable( struct Call * this, VarName name );
#  define HashListFromName(name)                      \
      ((struct HashList *)(((stringLengthType *)(name))-1)-1)
#  define NameFromHashList(hashlist) \
      ((jsechar *)(((stringLengthType *)((hashlist)+1))+1))
#  define LengthFromHashList(hashlist) \
      (*((stringLengthType *)((hashlist)+1)))
#  if defined(JSE_INLINES) && (0!=JSE_INLINES)
#     define RemoveFromStringTable( CALL, NAME )      \
         if( !((JSE_POINTER_UINT)(NAME) & 0x01) &&    \
             0 == --(HashListFromName(NAME)->count) ) \
            reallyRemoveFromStringTable(CALL,NAME)
#  else
#     define RemoveFromStringTable  reallyRemoveFromStringTable
#  endif
#  define AddStringUser(name)                 \
      if( !((JSE_POINTER_UINT)(name) & 0x01) )     \
         HashListFromName(name)->count++;
#else
   #define RemoveFromStringTable( call, name )
   #define AddStringUser(name)
#endif

/* variable lookup */

struct Var * NEAR_CALL FindVariableByName(struct Call * call, VarName name);

struct Var * NEAR_CALL LookupVariableByName(struct Call *call,
                                            VarName name,jsebool force);

void InitializeBuiltinObjects(struct Call *call);
void TerminateBuiltinObjects(struct Call *call);

/* formerly inline functions */

#define GetCurrentThisVar(this) ((this)->pCurrentThisVar)
#define SetCurrentThisVar(this,var) ((this)->pCurrentThisVar = (var))

#define GetCallVariable(this) ((this)->call_variable)
#define SetCallVariable(this,last) ((this)->call_variable = (last))

#define callTop(this) ((this)->Global->TopLevelCall)

#if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
  #define CBehavior(this) ((this)->CBehaviorWanted)
#endif

#define IgnorePastNewFunctionObjects(this) ((this)->ignore_past = True)

#if defined(JSE_GETFILENAMELIST) && (0!=JSE_GETFILENAMELIST)
  #define CALL_GET_FILENAME_LIST(this,number) \
          (*(number) = (this)->Global->number,(this)->Global->FileNameList)
#endif

#define callRetType(this) ((this)->pReturnAction)
#define callReasonToQuit(this) ((this)->privReasonToQuit)
#define callSetReasonToQuit(this,reason) ((this)->privReasonToQuit = (reason))

#define GetNumericStringTableEntry(this,entry) \
        ( ((JSE_POINTER_SINT)(entry)+1)/2 )
#define IsNumericStringTableEntry(this,entry) ( ((JSE_POINTER_SINT)(entry))&1 )
   /* odd is for numeric entries */

#define callParameterCount(this) ((this)->pInputVariableCount)
#define callPrevious(this) ((this)->pPreviousCall)
#define callQuitWanted(this) ((this)->privReasonToQuit & 0xff)
   /* the flags we care about are the ones in 8 bits */
#define EnterNumberIntoStringTable(number) ((VarName)((number)*2-1))
   /* This maps all numbers into odd slots since valid pointers will be in even
    * slots. Unfortunately it means we can only store half the numbers, but
    * who uses arrays with >2^30 members anyway???
    */

#define CALL_ADD_TEMP_VAR(this,tmp) \
        TEMP_VAR_ADD(&((this)->TempVars),tmp,True,(this))

#define SetTemporaryLibraryData(this,data) ((this)->TempLibData = (data))
#define GetTemporaryLibraryData(this) ((this)->TempLibData)

#define callFunc(this) ((this)->pFunction)

#define callErrorBase(this) callError(this,(enum textcoreID)0)

#if defined(__JSE_DOS16__)
   /* make these functions available outside core 64K */
   void farRemoveFromStringTable(struct Call * this, VarName name);
   const jsechar * farGetStringTableEntry(struct Call *this,VarName entry,stringLengthType *len);
   VarName farEnterIntoStringTable(struct Call *this,const jsechar *Name, stringLengthType length);
#else
   #define farRemoveFromStringTable(this,name)        RemoveFromStringTable(this,name)
   #define farGetStringTableEntry(this,entry,len)     GetStringTableEntry(this,entry,len)
   #define farEnterIntoStringTable(this,Name,length)  EnterIntoStringTable(this,Name,length)
#endif


#endif
