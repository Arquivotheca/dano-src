/* call.c   Parameters and code cards within a function.
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

static void cleanupCallInitial( struct Call * call );
#if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
   static jsebool initializeAllocators(struct Global_ * global);
#endif
static void cleanupCallInitialize( struct Call * call );

/* ---------------------------------------------------------------------- */

/* Note: all of the 'jsechar *' must be aligned to even boundaries -
 * if this fails, my cool scheme for fast number entries won't work,
 * and this would be *bad*.
 */

#if defined(JSE_ONE_STRING_TABLE) && (0!=JSE_ONE_STRING_TABLE)
#  if defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS)
      static struct HashList ** hashTable = NULL;
      static uint hashSize = JSE_HASH_SIZE;
#  else
      static jsechar **strings;
      static uint stringsUsed,stringsAlloced;
#  endif

   void 
allocateGlobalStringTable()
{
#  if defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS)
      hashSize = JSE_HASH_SIZE;
      hashTable = jseMustMalloc(struct HashList *,
                  hashSize*sizeof(struct HashList *));
      memset(hashTable,0,hashSize*sizeof(struct HashList *));
#  else
      stringsUsed = 0;
      stringsAlloced = 0;
      strings = NULL;
#  endif
}

   void
freeGlobalStringTable()
{
#  if defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS)
#  ifndef NDEBUG
      {
         uint i;
         jsebool stringTableOK = True;
         for( i = 0; i < hashSize; i++ )
         {
            if( NULL != hashTable[i] )
            {
               struct HashList * current = hashTable[i];
               while(current != NULL )
               {
                  DebugPrintf("String table entry \"%s\" not removed, sequence %d, file \"%s\", line %d\n",
                              NameFromHashList(current),
                              current->sequence,
                              current->file,
                              current->line );
                  current = current->next;
               }
               stringTableOK = False;
            }
         }
        /* if it fails, don't assert, or we won't get the debug information
         * on memory not freed, which is probably causing this
         */
        /* assert( stringTableOK ); */
      }
#  endif      
      jseMustFree(hashTable);
#  else
      uint x;
      for( x=0;x<stringsUsed;x++ )
         jseMustFree(strings[x]);
      if( strings ) 
         jseMustFree(strings);      
#  endif
}

#endif

#if defined(JSE_ONE_STRING_TABLE) && (0!=JSE_ONE_STRING_TABLE)
#  define getStringParameter(call,name)   (name)
#else
#  define getStringParameter(call,name)   (call->Global->name)
#endif


/* All Strings are stored here, this table was a linked list, now it
 * is changed to a dynamic array because it will use less memory and
 * search faster. As with object members, strings are in the table a
 * lot more often than they are added to the table.
 */
#if defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS) \
 && (defined(JSE_MEM_DEBUG) && (0!=JSE_MEM_DEBUG))
static int stringHashCount = 0;
#endif

/* Returns the location it should be at, as well as a boolean indicating
 * if it in fact was there. Does a binary search on the array.
 */
#if defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS)

#  if defined(JSE_MEM_DEBUG) && (0!=JSE_MEM_DEBUG)
   VarName NEAR_CALL
EnterIntoStringTableEx(struct Call *this,const jsechar *Name,stringLengthType length,
                       const char * file, int line)
#  else
   VarName NEAR_CALL
EnterIntoStringTable(struct Call *this,const jsechar *Name, stringLengthType length)
#  endif
{
   unsigned int location;
   struct HashList *list, **prev;
   const jsechar * copy;
   stringLengthType i;
   sint result;

   if( isdigit_jsechar(Name[0]) || Name[0]=='-' )
   {
      /* speed is of the essence. This avoids calling atoi() and at the
       * same time determines if this truely is a numeric index and not
       * say a phone number.
       */
      JSE_POINTER_SINT x = 0;
      jsebool neg = False;

      copy = Name;

      /* Negative only applies if the second digit is between 1 and 9
       * 0 is not valid because the string "0" and "-0" will both
       * result in the same number
       */
      if( copy[0]=='-' && (copy[1] >= '1' && copy[1] <= '9'))
      {
         neg = True;
         copy++;
      }

      if( copy[0]!='0' || !copy[1] )
      {
         while( isdigit_jsechar(*copy))
         {
            x = 10*x + (*copy++) - '0';
         }
         if( neg ) x = -x;
         if( ((stringLengthType)(copy - Name) == length) && x < (MAX_SLONG / 2) && x > (MIN_SLONG / 2))
            return EnterNumberIntoStringTable(x);
      }
   }
   
#  if defined(JSE_ONE_STRING_TABLE) && (0!=JSE_ONE_STRING_TABLE)
      UNUSED_PARAMETER(this);
#  endif

   for( location = 0, copy = Name, i = 0;  i < length; copy++, i++ )
      location = (location << 5) ^ *copy ^ location;
      
   location %= getStringParameter(this,hashSize);

   list = getStringParameter(this,hashTable)[location];
   prev = &(getStringParameter(this,hashTable)[location]);

   while( list != NULL )
   {
      if( length < LengthFromHashList(list) )
         break;
      else if( length == LengthFromHashList(list) )
      {
         result = jsecharCompare( NameFromHashList(list), LengthFromHashList(list),
                                  Name, length );
         if( result < 0 )
            break;
         if( result == 0 )
         {
            list->count++;
            return (VarName)NameFromHashList(list);
         }
      }
      
      prev = &(list->next);
      list = list->next;
   }


   list = jseMustMalloc(struct HashList,sizeof(struct HashList)+sizeof(stringLengthType)+
                        ((size_t)length+1) * sizeof(jsechar));

   list->count = 1;
   memcpy(NameFromHashList(list),Name,(size_t)length*sizeof(jsechar));
   NameFromHashList(list)[length] = 0;
   
   *((stringLengthType *)(list+1)) = (stringLengthType)strlen_jsechar(Name);
   
   list->prev = prev;
   if( NULL != (list->next = *prev))
      list->next->prev = &(list->next);
   *prev = list;
#  if defined(JSE_MEM_DEBUG) && (0!=JSE_MEM_DEBUG)
   {
      const jsechar * us;
      list->line = line;
      us = AsciiToUnicode(file);
      list->file = StrCpyMalloc(us);
      FreeUnicodeString(us);
      list->sequence = stringHashCount++;
   }
#  endif

   return (VarName)NameFromHashList(list);
}

   void NEAR_CALL
reallyRemoveFromStringTable( struct Call * this, VarName name )
{
   struct HashList * list;

#  if !defined(JSE_INLINES) || (0==JSE_INLINES)
   if(((JSE_POINTER_UINT)name & 1))
      return;
#  endif
   assert( !((JSE_POINTER_UINT)name & 1));

   list = HashListFromName(name);

#  ifndef NDEBUG
   {
      stringLengthType namelen = *(((stringLengthType *)name)-1);
      VarName realname = EnterIntoStringTable(this,name,namelen);
      assert(realname == name);
      assert( 0 == memcmp(realname,name,sizeof(jsechar)*(size_t)namelen));
      list->count--;
   }
#  endif

#  if defined(JSE_ONE_STRING_TABLE) && (0!=JSE_ONE_STRING_TABLE)
      UNUSED_PARAMETER(this);
#  endif

#  if !defined(JSE_INLINES) || (0==JSE_INLINES)
   if( --(list->count) == 0 )
#  endif
   {
      assert( list->count == 0 );
      if( NULL != (*(list->prev) = list->next))
         list->next->prev = list->prev;

#  if defined(JSE_MEM_DEBUG) && (0!=JSE_MEM_DEBUG)
      jseMustFree(list->file);
#  endif
      jseMustFree(list);
   }
}

#else
   static uint NEAR_CALL
FindEntry(struct Call *this,const jsechar *Name,stringLengthType length,jsebool *was_there)
{
   jsechar **table = getStringParameter(this,strings);
   uint lower = 0,upper = getStringParameter(this,stringsUsed) - 1;
   jsechar *test;
   int val;
   uint middle;

   if( getStringParameter(this,stringsUsed)==0 )
   {
      *was_there = False;
      return 0;
   }

   while( 1 )
   {
      assert( lower<=upper );
      if( lower==upper )
      {
         test = table[lower];
         *was_there = length == *(((stringLengthType *)test)-1)
                   && 0 == memcmp(Name,test,length*sizeof(jsechar)) ;
         middle = lower;
         break;
      }

      middle = lower + (upper-lower)/2;
      test = table[middle];
      val = length == *(((stringLengthType *)test)-1)
         && 0 == memcmp(Name,test,length*sizeof(jsechar)) ;
      if( val==0 )
      {
         *was_there = True;
         break;
      }

      if( val<0 )
      {
         if( middle==lower )
         {
            *was_there = False;
            break;
         }
         upper = middle - 1;
      }
      else
      {
         if( middle==upper )
         {
            *was_there = False;
            break;
         }
         lower = middle + 1;
      }
   }

   if( !(*was_there) && val>0 )
      return middle + 1;
   else
      return middle;
}


/* ---------------------------------------------------------------------- */

   VarName NEAR_CALL
EnterIntoStringTable(struct Call *this,const jsechar *Name,stringLengthType length)
{
   jsebool was_there;
   uint entry;
   const jsechar * copy;
   jsechar *ret;

   if( Name==NULL ) return NULL;

   if( isdigit_jsechar(Name[0]) || Name[0]=='-' )
   {
      /* speed is of the essence. This avoids calling atoi() and at the
       * same time determines if this truely is a numeric index and not
       * say a phone number.
       */
      JSE_POINTER_SINT x = 0;
      jsebool neg = False;

      copy = Name;

      /* Negative only applies if the second digit is between 1 and 9
       * 0 is not valid because the string "0" and "-0" will both
       * result in the same number
       */
      if( copy[0]=='-' && (copy[1] >= '1' && copy[1] <= '9'))
      {
         neg = True;
         copy++;
      }

      if( copy[0]!='0' || !copy[1] )
      {
         while( isdigit_jsechar(*copy))
         {
            x = 10*x + (*copy++) - '0';
         }
         if( neg ) x = -x;
         if( ((stringLengthType)(copy - Name) == length) )
         {
            assert( x < (MAX_SLONG / 2)  &&  x > (MIN_SLONG / 2) );
            return EnterNumberIntoStringTable(x);
         }
      }
   }

   entry = FindEntry(this,Name,length,&was_there);

   if( was_there ) return getStringParameter(this,strings)[entry];

   /* it wasn't found, so insert it into the table at the given location */

   assert( getStringParameter(this,stringsUsed)<=getStringParameter(this,stringsAlloced) );
   if( getStringParameter(this,stringsUsed)==getStringParameter(this,stringsAlloced) )
   {
      /* a good heuristic is double the space each time. It minimizes the
       * number of calls well.
       */
#  if defined(JSE_MIN_MEMORY) && (0!=JSE_MIN_MEMORY)
      getStringParameter(this,stringsAlloced) += INITIAL_STRING_TABLE_SIZE;
#  else
      getStringParameter(this,stringsAlloced)
         += (getStringParameter(this,stringsAlloced)==0)
         ? INITIAL_STRING_TABLE_SIZE
         : getStringParameter(this,stringsAlloced);
#  endif
      getStringParameter(this,strings)
         = jseMustReMalloc(jsechar *,getStringParameter(this,strings),
                           (uint)(getStringParameter(this,stringsAlloced)*sizeof(jsechar *)));
   }

   /* move the rest of the stuff */

   if( entry<getStringParameter(this,stringsUsed) )
   {
#  if defined(__JSE_WIN16__)
      HugeMemMove(getStringParameter(this,strings)+entry+1,getStringParameter(this,strings)+entry,
                  (uint)((getStringParameter(this,stringsUsed)-entry)*sizeof(jsechar *)));
#  else
      memmove(getStringParameter(this,strings)+entry+1,getStringParameter(this,strings)+entry,
              (uint)((getStringParameter(this,stringsUsed)-entry)*sizeof(jsechar *)));
#  endif
   }
   
   ret = jseMustMalloc(jsechar,(length+1)*sizeof(jsechar)+sizeof(stringLengthType));
   *((stringLengthType *)ret) = (stringLengthType)strlen_jsechar(Name);
   ret += sizeof(stringLengthType)/sizeof(jsechar);

   memcpy( ret, Name, sizeof(jsechar) * length);
   ret[length] = 0;

   getStringParameter(this,strings)[entry] = ret;
   /* assert that the entry is on a word boundary. If this fails, bad news */
#  ifndef NDEBUG
   {
      JSE_POINTER_UINT ptr = (JSE_POINTER_UINT)getStringParameter(this,strings)[entry];
      assert( sizeof(ptr) == sizeof(getStringParameter(this,strings)[entry]));
      assert((ptr&0x01)==0x00 );
   }
#  endif

   getStringParameter(this,stringsUsed)++;

   assert( memcmp(ret,Name,length*sizeof(jsechar))==0 );

   return ret;
}
#endif

   const jsechar * NEAR_CALL
GetStringTableEntry(struct Call *this,VarName entry,stringLengthType *lenptr)
{
   jsechar * ret;
   stringLengthType len;

   if( 0 == ((JSE_POINTER_UINT)entry & 1))
   {
      /* even address so is actual pointer to string name */
      len = *(((stringLengthType *)entry)-1);
      ret = (jsechar *)entry;
   }
   else
   {
      /* odd number represents an integer value; convert to string */
      JSE_POINTER_SINT x = ((JSE_POINTER_SINT)entry+1)/2;
      ret = this->Global->tempNumToStringStorage
          + ((sizeof(this->Global->tempNumToStringStorage)/sizeof(jsechar)) - 1);

      assert( *ret == 0 );

      if( x )
      {
         jsebool neg = False;
         len = 0;
         if( x < 0 )
         {
            neg = True;
            x *= -1;
         }
         while( x )
         {
            *(--ret) = (jsechar)((x%10) + '0' );
            x /= 10;
            len++;
         }
         if ( neg ) 
         {
            *(--ret) = '-';
            len++;
         }
      }
      else
      {
         *(--ret) = '0';
         len = 1;
      }
      assert( this->Global->tempNumToStringStorage <= ret );
   }
   if ( NULL != lenptr )
      *lenptr = len;
   return (const jsechar *)ret;
}


/* ---------------------------------------------------------------------- */

#if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
#  define CLONE_END_OF_CHAIN(OLDVAR,CHAINPTR,CALL) \
          CloneEndOfChain(OLDVAR,CHAINPTR,CALL)
#else
#  define CLONE_END_OF_CHAIN(OLDVAR,CHAINPTR,CALL) \
          CloneEndOfChain(OLDVAR,CHAINPTR)
#endif
   static void NEAR_CALL
CloneEndOfChain(struct TempVar *OldTempVar,struct TempVar **NewChain
#if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
   ,struct Call *call
#endif
)
{
   assert( NULL != OldTempVar );
   /* clone down to the last member which is the global, and don't copy that
    * global because we have our own
    */
   if ( NULL != OldTempVar->prev )
   {
      CLONE_END_OF_CHAIN(OldTempVar->prev,NewChain,call);
      varAddUser(OldTempVar->var);
      TEMP_VAR_ADD(NewChain,OldTempVar->var,False,call);
   }
}

   void NEAR_CALL
CloneScopeChain(struct Call *this,struct Call *old)
{
   if ( old )
   {
      /* the global object is already in this chain.  Call the
       * recursive CloneEndOfChain to get a copy of the rest
       * of the chain in its existing order.
       */
      CLONE_END_OF_CHAIN(old->ScopeChain,&(this->ScopeChain),this);
   }
}

   void NEAR_CALL
RemoveClonedScopeChain(struct Call *this,struct Call *old)
{
   if ( old )
   {
      /* remove all up to the last element of chain, which is the global */
      assert( NULL != this->ScopeChain );
      while ( NULL != this->ScopeChain->prev )
         RemoveScopeObject(this);
   }
}

   Var * NEAR_CALL
FindVariableByName(struct Call * this, VarName name)
{
   struct TempVar *lookin;
   struct Call *loop;

   if( name==GLOBAL_STRING(this,this_entry) )
   {
      varAddUser(this->pCurrentThisVar);
      return this->pCurrentThisVar;
   }
   if( name==GLOBAL_STRING(this,global_entry)
    || name==GLOBAL_STRING(this,userglobal_entry) )
   {
      varAddUser(this->session.GlobalVariable);
      return this->session.GlobalVariable;
   }

#if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
   /* If doing security, look for it there */
   if( this->session.SecurityGuard && this->session.SecurityGuard->InSecurityCall )
   {
      if( varHasProperty(this->session.SecurityGuard->PrivateVariable,this,name))
      {
         return constructReference(this,this->session.SecurityGuard->PrivateVariable,name);
      }
   }
#endif

   for ( lookin = this->ScopeChain; NULL != lookin; lookin = lookin->prev )
   {
      VarRead *vr = (VarRead *)(lookin->var);
      /* assert that this is already a varread so above case is valid */
      assert( VAR_HAS_DATA(vr));
      if( varHasProperty(vr,this,name))
      {
         return constructReference(this,vr,name);
      }
   }

   /* finally search all past new function objects */
   loop = this;
   while( loop )
   {
      if( loop->CallSettings & (jseNewFunctions|jseNewGlobalObject))
      {
         VarRead *last = loop->session.GlobalVariable;
         /* look in this object */
         if( varHasProperty(last,this,name))
         {
            return constructReference(this,last,name);
         }
      }
      if( loop->ignore_past ) break;
      loop = loop->pPreviousCall;
   }

   return NULL;
}

/* ---------------------------------------------------------------------- */

   void NEAR_CALL
callSetExitVar(struct Call *this,VarRead *ExitVar)
{
   if ( !callQuitWanted(this))
   {
      Var *OldExitVar = callGetExitVar(this);
      callSetReasonToQuit(this,FlowExit);

      /* copy their ExitVar into our local pExitVar to be passed on up or
       * removed when CALL is destroyed
       */
      this->pExitVar = constructVarRead(this,VUndefined);
      varCopyAssign(this->pExitVar,this,ExitVar);

      if ( NULL != OldExitVar )
      {
         VAR_THOROUGH_REMOVE_USER(OldExitVar,this);
      }
   } /* endif */
   assert( callQuitWanted(this));
}


   static VarRead * NEAR_CALL
NewGlobalVariable(struct Call *call)
{
   VarRead *newGlobalVariable = constructVarRead(call,VObject);

   varSetLvalue(newGlobalVariable,True);
   varSetAttributes(varCreateMember(
                       newGlobalVariable,call,
                       GLOBAL_STRING(call,argc_entry),
                       VNumber),
                    jseDontEnum);
   varSetAttributes(varCreateMember(
                       newGlobalVariable,call,
                       GLOBAL_STRING(call,argv_entry),
                       VObject),
                    jseDontEnum);

#if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
   varSetAttributes(varCreateMember(
                       newGlobalVariable,call,
                       GLOBAL_STRING(call,op_not_supported_entry),
                       VUndefined),
                    jseDontEnum|jseReadOnly);
   call->Global->special_operator_var = varGetDirectMember(call,newGlobalVariable,
                       GLOBAL_STRING(call,op_not_supported_entry),False);
#endif

   return newGlobalVariable;
}

   static void 
cleanupCallInitialize( struct Call * this )
{
    /* NYI: Remove scope object */
#if defined(JSE_LINK) && (0!=JSE_LINK)
    if( this->CallSettings & jseNewExtensionLib )
       extensionDelete(this->session.ExtensionLib,this);
#endif
#if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
    if( this->CallSettings &jseNewSecurity )
       securityDelete(this->session.SecurityGuard);
#endif
    if( this->CallSettings & jseNewAtExit)
       atexitDelete(this->session.AtExitFunctions);
    if( this->CallSettings & jseNewLibrary)
       libraryDelete(this->session.TheLibrary,this);
    if( this->CallSettings & (jseNewGlobalObject | jseNewFunctions) )
       VAR_THOROUGH_REMOVE_USER(this->session.GlobalVariable,this);
}

   jsebool NEAR_CALL
callInitialize(struct Call *this,jseNewContextSettings NewContextSettings)
{
   VarRead *GlobalVar;
   jsebool success = True;

   callSetReasonToQuit(this,FlowNoReasonToQuit);

   this->TempVars = NULL;
   this->ScopeChain = NULL;

#  if ( 2 <= JSE_API_ASSERTLEVEL )
      this->cookie = (uword8) jseContext_cookie;
#  endif

   if ( 0 != (this->CallSettings = (uword8)NewContextSettings))
   {
      success = False;
#if defined(JSE_DEFINE) && (0!=JSE_DEFINE)
      if ( !(NewContextSettings & jseNewDefines) ||
           NULL != (this->session.Definitions = defineNew(this->session.Definitions)))
      {
#endif
         if ( !(NewContextSettings & (jseNewGlobalObject | jseNewFunctions)) ||
              NULL != (this->session.GlobalVariable =  NewGlobalVariable(this)))
         {
            if ( !(NewContextSettings & jseNewLibrary) ||
                NULL != (this->session.TheLibrary = libraryNew(this,this->session.TheLibrary)))
            {
               if ( !(NewContextSettings & jseNewAtExit) ||
                   NULL != (this->session.AtExitFunctions
                        = atexitNew(this->session.AtExitFunctions)))
               {
#if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
                  if ( !(NewContextSettings & jseNewSecurity) ||
                      NULL != (this->session.SecurityGuard
                     = securityNew(this->Global->ExternalLinkParms.jseSecureCode)))
                  {
#endif
#if defined(JSE_LINK) && (0!=JSE_LINK)
                     if ( !(NewContextSettings & jseNewExtensionLib) ||
                        NULL != (this->session.ExtensionLib
                        = extensionNew(this,this->session.ExtensionLib)))
                     {
#endif
                        success = True;
#if defined(JSE_LINK) && (0!=JSE_LINK)
                     }
#endif
#if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
                  }
#endif
               }
            }
         }
#if defined(JSE_DEFINE) && (0!=JSE_DEFINE)
      }
#endif
   } 

   if( success )
   {
       this->pCurrentThisVar = GlobalVar = this->session.GlobalVariable;

       assert( NULL != GlobalVar );
                        /* construct value lock because global variable may be in the middle
                         * of being deleted! This will lock its varmem but not screw us up
                         * otherwise.
                         */
                        this->ignore_past = False;

                        /* NYI: Check for NULL */
                        AddScopeObject(this,CONSTRUCT_VALUE_LOCK(this,GlobalVar));
   }

   if( !success)
      cleanupCallInitialize(this);

   return success;
}


   struct Call * NEAR_CALL
callNew(struct Call *PreviousCall,struct Function *function,
        uint InputVariableCount,jseNewContextSettings NewContextSettings)
{
   struct Call *newcall =
#  if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
      (struct Call *)allocatorAllocItem(&PreviousCall->Global->allocate_call);
#  else
      jseMalloc(struct Call,sizeof(*newcall));
#  endif
   jsebool success = False;
   
   if( newcall != NULL )
   {

      /* explicitly copy those fields that are inherited */
      assert( NULL != PreviousCall );
      newcall->Global              = PreviousCall->Global;
      newcall->session             = PreviousCall->session;
      newcall->filename            = PreviousCall->filename;
      newcall->linenum             = PreviousCall->linenum;

      /* We must increase the user count on the filename, if it exists */
      if( NULL != newcall->filename )
         AddStringUser(PreviousCall->filename);

      /* link this to previous call, and vice versa */
      newcall->pPreviousCall = PreviousCall;
      assert( NULL == PreviousCall->pChildCall );
      PreviousCall->pChildCall = newcall;

      /* set input values for this call level */
      newcall->pChildCall = NULL;
      newcall->pFunction = function;
#     if 0 != JSE_MULTIPLE_GLOBAL
         /* set GlobalVariable for this new function */
         if ( NULL != function )
            newcall->session.GlobalVariable = function->global_object;
#     endif
      /* note where parameters are */
      newcall->stackMark = secodestackDepth(PreviousCall->Global->thestack);
      newcall->pInputVariableCount = InputVariableCount;
      newcall->pReturnVar = NULL;
      newcall->pExitVar = NULL;

#     if defined(JSE_CACHE_LOCAL_VARS) && (0!=JSE_CACHE_LOCAL_VARS)
         newcall->localCacheCount = 0;
         newcall->localCacheLocation = 0;
#     endif
   
#     if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
         newcall->CBehaviorWanted = ( NULL != function )
            ? functionCBehavior(function)
            : (jsebool)( jseOptDefaultCBehavior
                       & newcall->Global->ExternalLinkParms.options);
#     endif

      newcall->VariableObject = NULL;

      newcall->call_variable = NULL;

      /* the iterative interpretting settings must not be copied */

      newcall->oldMain = NULL;
      newcall->oldInit = NULL;
      newcall->with_count = 0;
      newcall->no_clean = False;

      if( callInitialize(newcall,NewContextSettings) )
      {

         newcall->fudged = False;
         if( newcall->session.GlobalVariable->userCount==0 )
         {
            newcall->fudged = True;
            newcall->session.GlobalVariable = constructVarRead(newcall,VUndefined);
            newcall->save_varmem = newcall->session.GlobalVariable->varmem;
            newcall->session.GlobalVariable->varmem =
               PreviousCall->session.GlobalVariable->varmem;
            varmemAddUser(newcall->session.GlobalVariable->varmem);
         }
         success = True;
      }
   }
   
   if( !success )
   {
      FreeIfNotNull(newcall);
      newcall = NULL;
   }

   return newcall;
}

static const jsechar * globalStringTable[GLOBAL_STRING_ENTRIES] = {
   textcoreInitializationFunctionName,
   textcoreThisVariableName,
   CONSTRUCT_PROPERTY,
   ORIG_PROTOTYPE_PROPERTY,
   PROTOTYPE_PROPERTY,
#  if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
   CALL_PROPERTY,
   GET_PROPERTY,
   PUT_PROPERTY,
   DELETE_PROPERTY,
   CANPUT_PROPERTY,
   HASPROPERTY_PROPERTY,
#  else
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
#  endif   
   DEFAULT_PROPERTY,
   PREFERRED_PROPERTY,
   textcoreGlobalVariableName,
   ARGUMENTS_PROPERTY,
   OLD_ARGS_PROPERTY,
   CALLEE_PROPERTY,
   CALLER_PROPERTY,
   LENGTH_PROPERTY,
   CLASS_PROPERTY,
   ARRAY_PROPERTY,
   VALUEOF_PROPERTY,
   TOSTRING_PROPERTY,
   VALUE_PROPERTY,
   PARENT_PROPERTY,
   textcoreMainFunctionName,
   textcore_ArgcName,
   textcore_ArgvName,
#if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
   OPERATOR_PROPERTY,
   OP_NOT_SUPPORTED_PROPERTY
#else
   NULL,
   NULL,
#endif
};

   static void
cleanupGlobalStrings(struct Call * call)
{
   int i;
   for( i = 0; i < GLOBAL_STRING_ENTRIES; i++ )
   {
      if( call->Global->global_strings[i] != NULL )
         RemoveFromStringTable(call,call->Global->global_strings[i]);
   }
}

   static jsebool
createGlobalStrings( struct Call * call, const jsechar * globalVariableName,
                     stringLengthType GlobalVariableNameLength )
{
   jsebool success = False;
   
   if( NULL != (call->Global->global_strings[userglobal_entry] = 
          EnterIntoStringTable(call,globalVariableName,GlobalVariableNameLength)) )
   {
      int i;
      for( i = 1; i < GLOBAL_STRING_ENTRIES; i++ )
      {
         if( (NULL != globalStringTable[i-1]) && NULL == (call->Global->global_strings[i] = 
                      EnterIntoStringTable(call,globalStringTable[i-1],strlen_jsechar(globalStringTable[i-1])) ) )
            break;
      }
      if( i == GLOBAL_STRING_ENTRIES )
         success = True;
   }
   
   if( !success )
      cleanupGlobalStrings(call);

   return success;
}

   static void
cleanupCallInitial( struct Call * call )
{
   if( call != NULL )
   {
      if( call->Global != NULL )
      {
         struct Global_ * global = call->Global;

         FreeIfNotNull(global->FunctionPrototype);
         FreeIfNotNull(global->ObjectPrototype);

         if( global->thestack != NULL )
            FreeIfNotNull(global->thestack->items);
         FreeIfNotNull(global->thestack);

         cleanupGlobalStrings(call);

#        if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
            if( global->allocate_call.saved_ptrs != NULL )
               allocatorTerm(&global->allocate_call);
            if( global->allocate_tempvar.saved_ptrs != NULL )
               allocatorTerm(&global->allocate_tempvar);
            if( global->allocate_varmem.saved_ptrs != NULL )
               allocatorTerm(&global->allocate_varmem);
            if( global->allocate_var.saved_ptrs != NULL )
               allocatorTerm(&global->allocate_var);
            if( global->allocate_objmem.saved_ptrs != NULL )
               allocatorTerm(&global->allocate_objmem);
#        endif

#        if defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)
            FreeIfNotNull(global->variableCache);
#        endif
         FreeIfNotNull(global->ExternalLinkParms.jseSecureCode);
#        if defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS) && \
            (!defined(JSE_ONE_STRING_TABLE) || (0==JSE_ONE_STRING_TABLE))
            FreeIfNotNull(global->hashTable);
#        endif

         jseMustFree(global);
      }

      jseMustFree(call);
   }
}


   struct Call *
callInitial(void _FAR_ *LinkData,
            struct jseExternalLinkParameters *ExternalLinkParms,
            const jsechar * globalVariableName,
            stringLengthType GlobalVariableNameLength)
{
   struct Call *this = jseMalloc(struct Call,sizeof(struct Call));
   struct Global_ *global;
   jsebool success = False;

   if( NULL != this )
   {
      memset( this, 0, sizeof(struct Call));
      assert( !this->pPreviousCall );
      assert( !this->pChildCall );
      assert( NULL == this->pFunction );
      /* including this assert causes the lib init func to be called; so compare
       * to NULL instead of NOT operator
       */
      assert( !this->pInputVariableCount );
      assert( !this->pReturnVar );

      global = this->Global = jseMalloc(struct Global_,sizeof(struct Global_));
      if( NULL != global )
      {
         memset(global,0,sizeof(struct Global_));
#        if (0!=JSE_COMPILER)
            assert( NULL == global->CompileStatus.CompilingFileName );
            assert( 0 == global->CompileStatus.NowCompiling );
#        endif
#        if (0!=JSE_CYCLIC_GC)
            assert( NULL == global->gcStartList );
            assert( !global->CollectingCyclicGarbage );
#        endif
         global->GenericData = LinkData;

#        if (defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS)) \
         && (!defined(JSE_ONE_STRING_TABLE) || (0==JSE_ONE_STRING_TABLE))
            global->hashSize = ExternalLinkParms->hashTableSize
               ? ExternalLinkParms->hashTableSize : JSE_HASH_SIZE;
            global->hashTable = jseMustMalloc(struct HashList *,
                                           global->hashSize*sizeof(struct HashList *));
            if( NULL != global->hashTable )
            {
               memset(global->hashTable,0,global->hashSize*sizeof(struct HashList *));
#        endif

               global->ExternalLinkParms = *ExternalLinkParms;
#              if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
                  if ( NULL == global->ExternalLinkParms.jseSecureCode
                    || NULL != (global->ExternalLinkParms.jseSecureCode =
                           StrCpyMalloc(global->ExternalLinkParms.jseSecureCode)))
                  {
#              endif
#              if (defined(__JSE_WIN16__) || defined(__JSE_DOS16__)) \
               && (defined(__JSE_DLLLOAD__) || defined(__JSE_DLLRUN__))
                  /* determine caller's DS by reading where it was pushed on stack
                   * it is pushed right after sp is saved into bp
                   */
                  global->ExternalDataSegment = (Get_SS_BP())[-1];
#              endif

#              if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
                  this->CBehaviorWanted = (jsebool)( jseOptDefaultCBehavior
                                                   & ExternalLinkParms->options);
#              endif

#              if defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)
                  global->variableCache = jseMustMalloc(struct variableCacheEntry,
                                      VARIABLE_CACHE_SIZE*sizeof(struct variableCacheEntry));
                  if( NULL != global->variableCache )
                  {
                     global->maxCacheSize = VARIABLE_CACHE_SIZE;
#              endif
#              if (defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)) || \
                  (defined(JSE_CACHE_LOCAL_VARS) && (0!=JSE_CACHE_LOCAL_VARS))
                     global->currentCacheSize = 0;
#              endif
                  
#                    if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
                        if( initializeAllocators(global))
                        {
#                    endif
                           assert( sizeof(VarRead)==sizeof(Var));
                           assert( sizeof(VarWrite)==sizeof(Var));

                           if( createGlobalStrings(this,globalVariableName,GlobalVariableNameLength) )
                           {

#                             if defined(JSE_DYNAOBJ_HASHLIST) && (0!=JSE_DYNAOBJ_HASHLIST)
                                 MoveDynaobjHashEntry(global,0,&(GLOBAL_STRING(this,delete_entry)),HAS_DELETE_PROP);
                                 MoveDynaobjHashEntry(global,1,&(GLOBAL_STRING(this,put_entry)),HAS_PUT_PROP);
                                 MoveDynaobjHashEntry(global,2,&(GLOBAL_STRING(this,canput_entry)),HAS_CANPUT_PROP);
                                 MoveDynaobjHashEntry(global,3,&(GLOBAL_STRING(this,get_entry)),HAS_GET_PROP);
                                 MoveDynaobjHashEntry(global,4,&(GLOBAL_STRING(this,hasproperty_entry)),HAS_HAS_PROP);
                                 MoveDynaobjHashEntry(global,5,&(GLOBAL_STRING(this,call_entry)),HAS_CALL_PROP);
                                 MoveDynaobjHashEntry(global,6,&(GLOBAL_STRING(this,prototype_entry)),HAS_PROTOTYPE_PROP);
#                                if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
                                    MoveDynaobjHashEntry(global,7,&(GLOBAL_STRING(this,operator_entry)),HAS_OPERATOR_PROP);
#                                endif
#                             endif
                              global->TopLevelCall = this;

                              global->thestack = secodestackNew();

                              if( callInitialize(this,jseAllNew) )
                              {

#                                if defined(JSE_DEFINE) && (0!=JSE_DEFINE)
#                                   if defined(__JSE_OS2TEXT__)
                                       defineAddInt(this->session.Definitions,UNISTR("_OS2_"),1L);
#                                   elif defined(__JSE_OS2PM__)
                                       defineAddInt(this->session.Definitions,UNISTR("_PM_"),1L);
#                                   elif defined(__JSE_DOS16__)
                                       defineAddInt(this->session.Definitions,UNISTR("_DOS_"),1L);
#                                   elif defined(__JSE_DOS32__)
                                       defineAddInt(this->session.Definitions,UNISTR("_DOS32_"),1L);
#                                   elif defined(__JSE_WIN16__)
                                       defineAddInt(this->session.Definitions,UNISTR("_WINDOWS_"),1L);
#                                   elif defined(__JSE_CON32__) || defined(__JSE_WIN32__)
#                                      if defined(__JSE_WINCE__)
                                          defineAddInt(this->session.Definitions,UNISTR("_WINDOWS_CE_"),1L);
#                                      else
                                          defineAddInt(this->session.Definitions,UNISTR("_WIN32_"),1L);
#                                      endif
#                                   elif defined(__JSE_NWNLM__)
                                       defineAddInt(this->session.Definitions,UNISTR("_NWNLM_"),1L);
#                                   elif defined(__JSE_UNIX__)
                                       defineAddInt(this->session.Definitions,UNISTR("_UNIX_"),1L);
#                                   elif defined(__JSE_MAC__)
                                       defineAddInt(this->session.Definitions,UNISTR("_MAC_"),1L);
#                                   elif defined(__JSE_PSX__)
                                       defineAddInt(this->session.Definitions,UNISTR("_PSX_"),1L);
#                                   elif defined(__JSE_PALMOS__)
                                       defineAddInt(this->session.Definitions,UNISTR("_PALMOS_"),1L);
#                                   elif defined(__JSE_390__)
                                       defineAddInt(this->session.Definitions,UNISTR("_OS390_"),1L);
#                                   elif defined(__JSE_EPOC32__)
                                       defineAddInt(this->session.Definitions,UNISTR("_EPOC32_"),1L);
#                                   else
#                                      error define OS type
#                                   endif
#                                endif

                                 InitializeBuiltinObjects(this);
                                 /* Finally!  we have successfully built the call */
                                 success = True;
                     
                              }
                           }
#                    if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
                        }
#                    endif

#              if defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)
                  }
#              endif

#              if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
                  }
#              endif
#        if (defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS)) \
         && (!defined(JSE_ONE_STRING_TABLE) || (0==JSE_ONE_STRING_TABLE))
            }
#        endif
      }
   }

   if( !success )
   {
      cleanupCallInitial(this);
      this = NULL;
   }

   return this;
}


   void NEAR_CALL
callDelete(struct Call *this)
{
   FlowFlag RememberReasonToQuit = callReasonToQuit(this);
   struct Global_ *global = this->Global;
   struct secodeStack *stack = global->thestack;

   /* make sure we don't leave any with's hanging around */
   while( this->with_count>0 )
   {
      this->with_count--;
      RemoveScopeObject(this);
   }

   /* make sure the stack is up-to-snuff - there can be extra junk on the
    * stack due to 'return's in the middle of for..in loops, ctrl-c, etc.
    * clean that up now
    */
   while( this->stackMark<secodestackDepth(stack))
   {
      Var *top = SECODE_STACK_POP(stack);
#     if   defined(JSE_CACHE_LOCAL_VARS) && (0!=JSE_CACHE_LOCAL_VARS)
         if( top && secodestackDepth(stack) >= this->localCacheLocation +
             (this->localCacheCount * 2))
         {
            VAR_REMOVE_USER(top,this);
         }
#     else
         assert( top!=NULL );
         VAR_REMOVE_USER(top,this);
#     endif
   }

   /* temporarily pretend that there's no error (in case there is one) */
   if ( this->CallSettings & jseNewAtExit )
   {
      assert( NULL != this->session.AtExitFunctions );
      callSetReasonToQuit(this,FlowNoReasonToQuit);
      atexitCallFunctions(this->session.AtExitFunctions,this);
      atexitDelete(this->session.AtExitFunctions);
   }

   /* ---------------------------------------------------------------------- */

   if( NULL != this->VariableObject )
      VAR_THOROUGH_REMOVE_USER(this->VariableObject,this);

   /* When we destroy the context, the only remaining item on the
    * scope chain should be the global variable.
    */
   if ( this->ScopeChain )
   {
      assert( NULL == this->ScopeChain->prev );
      RemoveScopeObject(this);
   }

   /* ---------------------------------------------------------------------- */

   if ( this->CallSettings & jseNewLibrary )
   {
      callSetReasonToQuit(this,FlowNoReasonToQuit);
      libraryDelete(this->session.TheLibrary,this);
   } /* endif */

   while( this->TempVars )
   {
      tempvarRemoveTop(&(this->TempVars),this);
   }

   if( this->fudged )
   {
      this->session.GlobalVariable->varmem->data.vall.userCount--;
      this->session.GlobalVariable->varmem = this->save_varmem;
      VAR_REMOVE_USER(this->session.GlobalVariable,this);
   }


   /* We do this here, because while deleting this thing, we may do some
   * destructors, and we need a valid call to do them in.
   */
   if ( this->CallSettings & (jseNewGlobalObject | jseNewFunctions))
   {
      assert( !this->fudged );
      VAR_THOROUGH_REMOVE_USER(this->session.GlobalVariable,this);
   }

   callSetReasonToQuit(this,RememberReasonToQuit);

   /* if there is an ExitVar and no one asked for it from
    * getExitVar() then destroy that now
    */
   if ( NULL != this->pExitVar )
   {
      /* if the parent call doesn't have an exit var yet, then pass this
       * one on up the chain
       */
      if ( NULL != this->pPreviousCall
           && NULL == this->pPreviousCall->pExitVar )
      {
         this->pPreviousCall->pExitVar = this->pExitVar;
      }
      else
      {
         VAR_THOROUGH_REMOVE_USER(this->pExitVar,this);
      }
   }


   /* we free any tempvars here because the destruction of the global
    * variable could make more
    */
   while( this->TempVars )
   {
      tempvarRemoveTop(&(this->TempVars),this);
   }
#  if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
      if ( this->CallSettings & jseNewSecurity )
      {
         if ( securityEnabled(this->session.SecurityGuard))
            securityTerm(this->session.SecurityGuard,this);
         securityDelete(this->session.SecurityGuard);
         this->session.SecurityGuard = NULL;
      }
#  endif

   while( this->TempVars )
   {
      /* security closure *might* have created temp vars. Because they
       * were created in the security close function, the fact that the
       * call is no longer secure is moot - the security function doesn't
       * have to pass through security anyway.
       */
      tempvarRemoveTop(&(this->TempVars),this);
   }

   if( this->filename != NULL )
      RemoveFromStringTable(this,this->filename);

#  if defined(JSE_LINK) && (0!=JSE_LINK)
      if ( this->CallSettings & jseNewExtensionLib )
      {
         callSetReasonToQuit(this,FlowNoReasonToQuit);
         extensionDelete(this->session.ExtensionLib,this);
      } /* endif */
#  endif
#  if defined(JSE_DEFINE) && (0!=JSE_DEFINE)
      if ( this->CallSettings & jseNewDefines )
         defineDelete(this->session.Definitions);
#  endif

   if ( NULL != this->pPreviousCall )
   {
      assert( this->pPreviousCall->pChildCall == this );
      this->pPreviousCall->pChildCall = NULL;
#     if ( 2 <= JSE_API_ASSERTLEVEL )
         this->cookie = 0;
#     endif
#     if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
         allocatorFreeItem(&(global->allocate_call),(void *)this);
#     else
         jseMustFree(this);
#     endif
   }
   else
   {

#     if (0!=JSE_CYCLIC_GC)
         CollectCyclicGarbage(this);
#     endif
#     if defined(JSE_CACHE_GLOBAL_VARS) && (0!=JSE_CACHE_GLOBAL_VARS)
         assert( 0 == global->currentCacheSize);
         jseMustFree(global->variableCache);
#     endif
      
#     if defined(JSE_GETFILENAMELIST) && (0!=JSE_GETFILENAMELIST)
         if( global->FileNameList )
         {
            int i;
            for( i=0;i<global->number;i++ )
               jseMustFree(global->FileNameList[i]);
            jseMustFree(global->FileNameList);
         }
#     endif

#     if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
         FreeIfNotNull((jsechar *)(global->ExternalLinkParms.jseSecureCode));
#     endif

      TerminateBuiltinObjects(this);

      secodestackDelete(global->thestack,this);

#     if defined(JSE_DYNAOBJ_HASHLIST) && (0!=JSE_DYNAOBJ_HASHLIST)
         RestoreDynaobjHashEntry(global,0,&(GLOBAL_STRING(this,delete_entry)));
         RestoreDynaobjHashEntry(global,1,&(GLOBAL_STRING(this,put_entry)));
         RestoreDynaobjHashEntry(global,2,&(GLOBAL_STRING(this,canput_entry)));
         RestoreDynaobjHashEntry(global,3,&(GLOBAL_STRING(this,get_entry)));
         RestoreDynaobjHashEntry(global,4,&(GLOBAL_STRING(this,hasproperty_entry)));
         RestoreDynaobjHashEntry(global,5,&(GLOBAL_STRING(this,call_entry)));
         RestoreDynaobjHashEntry(global,6,&(GLOBAL_STRING(this,prototype_entry)));
#        if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
            RestoreDynaobjHashEntry(global,7,&(GLOBAL_STRING(this,operator_entry)));
#        endif
#     endif  
      cleanupGlobalStrings(this);

#     if !defined(JSE_ONE_STRING_TABLE) || (0==JSE_ONE_STRING_TABLE)
#        if defined(JSE_HASH_STRINGS) && (0!=JSE_HASH_STRINGS)
            /* If all added names are removed, this should be true */
#           ifndef NDEBUG
            {
               uint i;
               /*jsebool stringTableOK = True;*/
               for( i = 0; i < global->hashSize; i++ )
               {
                  if( NULL != global->hashTable[i] )
                  {
                     struct HashList * current = global->hashTable[i];
                     while(current != NULL )
                     {
                        DebugPrintf(UNISTR("String table entry \"%s\" not removed, sequence %d, file \"%s\", line %d\n"),
                                    NameFromHashList(current),
                                    current->sequence,
                                    current->file,
                                    current->line );
                        current = current->next;
                     }
                     /*stringTableOK = False;*/
                  }
               }
               /* if it fails, don't assert, or we won't get the debug information
                * on memory not freed, which is probably causing this
                */
               /* assert( stringTableOK ); */
            }
#           endif
            jseMustFree(global->hashTable);
#        else
         {
            uint x;
            for( x=0;x<global->stringsUsed;x++ )
               jseMustFree(global->strings[x] - sizeof(stringLengthType));
            if( global->strings ) jseMustFree(global->strings);
         }
#        endif
#     endif

#     if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
         allocatorTerm(&(global->allocate_objmem));
         allocatorTerm(&(global->allocate_varmem));
         allocatorTerm(&(global->allocate_var));
         allocatorTerm(&(global->allocate_tempvar));
         allocatorTerm(&(global->allocate_call));
#     endif

#     if (0!=JSE_CYCLIC_GC)
         assert( NULL == global->gcStartList );
#     endif

#     if !defined(NDEBUG)
         assert( 0 == global->totalVarCount );
         assert( 0 == global->totalVarmemCount );
#     endif

      jseMustFree(global);

      jseMustFree(this);
   }
}

   void NEAR_CALL
callReturnVar(struct Call *this,Var *rawvar,jseReturnAction RAction)
{
   Var *OldReturnVar = this->pReturnVar; /* may need to clean up later */
   assert( NULL != rawvar );

   if ( jseRetKeepLVar == (this->pReturnAction = RAction))
   {
      varAddUser(this->pReturnVar = rawvar);
   }
   else
   {
      VarRead *RetVar = GET_READABLE_VAR(rawvar,this);
      if ( jseRetTempVar == RAction )
      {
         this->pReturnVar = RetVar;
         VAR_THOROUGH_REMOVE_USER(rawvar,this);
      }
      else
      {
         assert( jseRetCopyToTempVar == RAction );
         this->pReturnVar = constructVarRead(this,VUndefined);
         varCopyAssign(this->pReturnVar,this,RetVar);
         VAR_REMOVE_USER(RetVar,this);
      }
   }

   if ( NULL != OldReturnVar ) /* will have been replaced by new return var */
   {
      VAR_REMOVE_USER(OldReturnVar,this);
   }
}

   Var * NEAR_CALL
callGetVar(struct Call *this,uint InputVarOffset)
{
   Var *var;
   if ( this->pInputVariableCount <= InputVarOffset )
   {
      callError(this,textcoreFUNCPARAM_NOT_PASSED,1 + InputVarOffset,
                callCurrentName(this));
      var = NULL;
   }
   else
   {
      struct secodeStack *thestack = this->Global->thestack;
      var = SECODE_STACK_PEEK(thestack,
                              this->pInputVariableCount - InputVarOffset - 1 +
                              (secodestackDepth(thestack)-this->stackMark));
#     if defined(JSE_MEM_DEBUG) && (0!=JSE_MEM_DEBUG)
         assert( jseMemValid(var,0)  &&  jseMemValid(var,sizeof(*var)-1));
#     endif
   } /* endif */
   assert( NULL != var  ||  FlowError == callReasonToQuit(this));
   return var;
}

   Var * NEAR_CALL
callGetVarNeed(struct Call *this,VarRead * pVar,
               uint InputVarOffset,jseVarNeeded need)
{
   Var *v;
   VarRead *tmp;
   VarType vType;
   jsebool isfunc;
   struct Function *itsfunc;
   jsenumber f;
   jseVarNeeded VNConvertTo;
   jsebool tempvar; /* a newly-created variable is being returned */

   if ( NULL == (v = pVar) && NULL == (v = callGetVar(this,InputVarOffset)))
   {
      /* error even getting the variable; Leave; */
      assert( FlowError == callReasonToQuit(this));
      return NULL;
   } /* endif */

   tempvar = ( 0 != (need & (JSE_VN_LOCKREAD|JSE_VN_LOCKWRITE)) );
   if ( tempvar )
   {
      v = ( need & JSE_VN_LOCKREAD )
         ? GET_READABLE_VAR(v,this)
         : getWriteableVar(v,this) ;
   }

   if ( 0 == (need & 0x3FF))
   {
      /* caller doesn't care what type of variable it is.  So quit. */
      goto Validated;
   } /* endif */

   /* get the current type of the variable */
   tmp = GET_READABLE_VAR(v,this);
   vType = VAR_TYPE(tmp);
   isfunc = VAR_IS_FUNCTION(tmp,this);
   itsfunc = isfunc?varGetFunction(tmp,this):(struct Function *)NULL;
   f = (vType==VNumber) ? varGetNumber(tmp) : 0;
   VAR_REMOVE_USER(tmp,this);

   /* if already one of the valid types then return it as it is */
   if ( (1 << vType) & need )
      goto Validated;

   /* if a function is wanted, and this is an object function, then okeedokee */
   if ((JSE_VN_FUNCTION & need)  &&  VObject == vType  &&  isfunc )
   {
      goto Validated;
   } /* endif */

   if ( vType==VNumber )
   {
      /* can be cases where non-number acts as number, or where we need to be
       * more anal.  Rich: this is no longer true
       */
      if ( VNumber != vType  &&  (need & JSE_VN_NUMBER))
      {
         /* element of string or buffer can always act as a number */
         goto Validated;
      } /* endif */
      if ( need & (JSE_VN_BYTE|JSE_VN_INT))
      {
         if ( need & JSE_VN_INT )
         {
            /* check if can cast OK as integer */
            if ((jsenumber)((slong)f) == f )
               goto Validated;
         }
         else
         { /* if can't cast as integer then sure won't cast as a byte */
            assert( need & JSE_VN_BYTE );
            if ((jsenumber)((ubyte)f) == f )
               goto Validated;
         } /* endif */
      } /* endif */
   } /* endif */

   /* if reached this part of the code then is not already one of the
    * acceptable types. Last acceptable chance is to convert it to the
    * desired JSE_VN_CONVERT type
    */
   VNConvertTo = (need >> 16) & 0xFF;
   if ( 0 != VNConvertTo )
   {
      /* if one of the allowable convert-to types do the requested conversion
       * and leave OK
       */
      /* Also, if the lenient conversion flag is set, then convert from
       * anything.
       */
      /* Also, if the variable is read-only, then we cannot replace it
       * so treat implicitly as if JSE_VN_COPY
       */
      if (((1 << vType) & (need >> 24)) ||
          (jseOptLenientConversion & this->Global->ExternalLinkParms.options))
      {
         VarRead  *convar;
         VarWrite *setvar;

         tmp = GET_READABLE_VAR(v,this);
         convar = AutoConvert(this,tmp,VNConvertTo);
         VAR_REMOVE_USER(tmp,this);

         if ( (need & JSE_VN_COPYCONVERT)
           || ( (VAR_HAS_DATA(v))  &&  (varGetAttributes(v) & jseReadOnly)) )
         {
            if ( tempvar )
            {
               VAR_REMOVE_USER(v,this);
            }
            tempvar = True;
            v = convar;
         }
         else
         {
            setvar = getWriteableVar(v,this);
            varAssign(setvar,this,convar);
            VAR_REMOVE_USER(convar,this);
            VAR_REMOVE_USER(setvar,this);
         }

         goto Validated;
      } /* endif */
   } /* endif */

   /* all attempts to validate this variable have failed.  Pretty a return
    * string telling what types of variables would have been valid, and what
    * was invalid.
    */
   {
      /* prepare buffer giving parameter offset, if we can figure it out */
      jsechar ParameterOffset[10];
      struct InvalidVarDescription BadDesc;
      if ( NULL == pVar )
         jse_sprintf(ParameterOffset,UNISTR(" %d"),InputVarOffset+1);
      else
         ParameterOffset[0] = '\0';

      /* tell error (include types we could have converted from) */
      DescribeInvalidVar(this,v,vType,itsfunc, need | (need >> 24),&BadDesc);

      callError(this,textcoreVARNEEDED_PARAM_ERROR,ParameterOffset,
                BadDesc.VariableName,callCurrentName(this),
                BadDesc.VariableType,BadDesc.VariableWanted);
   }
   if ( tempvar )
   {
      VAR_REMOVE_USER(v,this);
   }
   return NULL;

 Validated:
   assert( NULL != v );
   if ( need & JSE_VN_CREATEVAR )
   {
      if ( !tempvar )
         varAddUser(v);
   }
   else
   {
      if ( tempvar )
         CALL_ADD_TEMP_VAR(this,v);
   }

   return v;
}

   VarRead * NEAR_CALL
callGetExitVar(struct Call *this)
{
   VarRead * ExitVar = this->pExitVar;
   this->pExitVar = NULL;
   return ExitVar;
}

   void NEAR_CALL
tempvarAdd(struct TempVar **list,Var *var,jsebool ShareTempvars
#if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
   ,struct Call *call
#endif
)
{
   struct TempVar *tv;
   if ( ShareTempvars )
   {
      /* check first to see if tempvar is already there */
      for ( tv = *list; NULL != tv; tv = tv->prev )
      {
         if ( var == tv->var )
         {
            tv->count++;
            return;
         }
      }
   }
   /* add this var to the beginning of the list */
#  if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
      tv = (struct TempVar *)allocatorAllocItem(
                             &(call->Global->allocate_tempvar));
#  else
      tv = jseMustMalloc(struct TempVar,sizeof(*tv));
#  endif
   tv->count = 1;
   tv->var = var;
   tv->prev = *list;
   *list = tv;
}

   void NEAR_CALL
tempvarRemoveTop(struct TempVar **list,struct Call *call)
{
   struct TempVar *tv = *list;

   assert( NULL != tv );
   assert( 0 != tv->count );

   VAR_REMOVE_USER(tv->var,call);
   if ( 0 == --(tv->count))
   {
      /* remove from tempvar list and point to next top-of-list */
      *list = tv->prev;
#     if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
         allocatorFreeItem(&(call->Global->allocate_tempvar),(void *)tv);
#     else
         jseMustFree(tv);
#     endif
   }
}


#if defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)
   static jsebool
initializeAllocators(struct Global_ * global)
{
   jsebool success = False;
   /* The size of the pool is determined by how many are needed at once.
    * For example, if we allocate 1000 of something then allocate and free
    * a few at a time, a small pool suffices.
    */

   /* not too many of these are being allocated and freed */
   if (allocatorInit(&global->allocate_call,sizeof(struct Call),50))
   {

      /* lots and lots of these are used and freed */
#  if defined(JSE_MIN_MEMORY) && (0!=JSE_MIN_MEMORY)
      if( allocatorInit(&global->allocate_tempvar,
                    sizeof(struct TempVar),100))
      {
         if (allocatorInit(&global->allocate_varmem,
                       sizeof(struct Varmem),300))
         {
            if (allocatorInit(&global->allocate_var,
                          sizeof(struct Var),300))
            {
               if( allocatorInit(&global->allocate_objmem,
                             VAROBJSIZE*sizeof(struct StructureMember),25))
               {
#  else
      if( allocatorInit(&global->allocate_tempvar,
                    sizeof(struct TempVar),500))
      {
         if( allocatorInit(&global->allocate_varmem,
                       sizeof(struct Varmem),900))
         {
            if(allocatorInit(&global->allocate_var,
                          sizeof(struct Var),900))
            {
               if( allocatorInit(&global->allocate_objmem,
                             VAROBJSIZE*sizeof(struct StructureMember),250))
               {
#  endif
                  success = True;
               }
            }
         }
      }
   }
   return success;
}
#endif

#if defined(__JSE_DOS16__)
   /* function so that far-call TOKEN can make these near calls */
   void farRemoveFromStringTable(struct Call * this, VarName name)
      { RemoveFromStringTable(this,name); }
   const jsechar * farGetStringTableEntry(struct Call *this,VarName entry,stringLengthType *len)
      { return GetStringTableEntry(this,entry,len); };
   VarName farEnterIntoStringTable(struct Call *this,const jsechar *Name, stringLengthType length)
      { return EnterIntoStringTable(this,Name,length); }
#endif
