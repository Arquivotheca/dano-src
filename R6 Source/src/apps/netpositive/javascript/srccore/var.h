/* Var.h    Access to variables of all kinds.
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

#ifndef _VAR_H
#define _VAR_H
#if defined(__cplusplus)
   extern "C" {
#endif

/* define JSE_CYCLIC_CHECK, JSE_FULL_CYCLIC_CHECK< or neither */
#if !defined(JSE_CYCLIC_CHECK)
#  define JSE_CYCLIC_CHECK  1
#endif
#if !defined(JSE_FULL_CYCLIC_CHECK)
#  define JSE_FULL_CYCLIC_CHECK  0
#endif
#if (0!=JSE_FULL_CYCLIC_CHECK)
#  define JSE_CYCLIC_CHECK 1
#endif

typedef  sword8  VarType;
#define  VUndefined  ((VarType)jseTypeUndefined)
#define  VarTypeMin  VUndefined
#define  VNull       ((VarType)jseTypeNull)
#define  VBoolean    ((VarType)jseTypeBoolean)
#define  VObject     ((VarType)jseTypeObject)
#define  VString     ((VarType)jseTypeString)
#define  VNumber     ((VarType)jseTypeNumber)
#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
#  define VBuffer    ((VarType)jseTypeBuffer)
#  define VarTypeMax jseTypeBuffer
#else
#  define VarTypeMax jseTypeNumber
#endif
#define isValidVarType(v)  ((v) >= VarTypeMin && (v) <= VarTypeMax)


/* ----------------------------------------------------------------------
 * the varobj struct stores members of an object.
 * ---------------------------------------------------------------------- */

/* Each member has its own structure, we use an array of them */
struct StructureMember
{
   VarName Name;
   VarRead *var;
};

#if defined(JSE_MIN_MEMORY) && (0!=JSE_MIN_MEMORY)
   typedef uint MemCountUInt;
   typedef struct StructureMember * MemberArrayPtr;
#else
   typedef ulong MemCountUInt;
   typedef struct StructureMember _HUGE_ * MemberArrayPtr;
#endif


#  if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
      /* these flags are used to indicate if the object has certain special
       * members so we don't have to look for them all the time
       */
#     define HAS_PROTOTYPE_PROP 0x01
#     define HAS_PUT_PROP       0x02
#     define HAS_GET_PROP       0x04
#     define HAS_CANPUT_PROP    0x08
#     define HAS_HAS_PROP       0x10
#     define HAS_DELETE_PROP    0x20

#     define HAS_CALL_PROP      0x40

      /* these are used to turn off the given behavior */
#     define OFF_PUT_PROP    0x0200
#     define OFF_GET_PROP    0x0400
#     define OFF_CANPUT_PROP 0x0800
#     define OFF_HAS_PROP    0x1000
#     define OFF_DELETE_PROP 0x2000
#  endif

#  if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
#     define HAS_OPERATOR_PROP  0x80
#     define OFF_OPERATOR_PROP  0x8000
#     define HAS_ALL_PROP       0xFF
#  else
#     define HAS_ALL_PROP       0x7F
#  endif

#  define ACTIVATION 0x4000

struct Varobj
{
#  if !defined(JSE_MIN_MEMORY) || (0==JSE_MIN_MEMORY)
      MemCountUInt alloced;         /* how many members allocated */
#  endif
   MemCountUInt used;            /* how many members used */
   MemberArrayPtr members;      /* the members themselves.
                                 * NULL if used==alloced==0
                                 */
      /* The member array is sorted by the Name key. */
   uword16 flags;
};

struct StructureMember * NEAR_CALL varobjFindMember(struct Varobj * varobj,VarName name,MemCountUInt *hint);

struct StructureMember * NEAR_CALL varobjCreateMemberWithHint(
   struct Varobj *varobj,struct Call *call,VarName Name,
   VarType DType,MemCountUInt start);
#define varobjCreateMember(obj,call,name,type) \
        varobjCreateMemberWithHint(obj,call,name,type,0)

void NEAR_CALL varobjRemoveMember(struct Varobj *varobj,struct Call *call,
                                  struct StructureMember * member);
   /* needs call in case dynamic delete */

/* ----------------------------------------------------------------------
 * the varmem struct is where the data is actually stored. It is
 * manipulated via Varread/Varwrite.
 * ---------------------------------------------------------------------- */

#define VAROBJSIZE 10

struct AllVarmem { /* all varmem types share this data in the same place */
   uint userCount;         /* how many variables are using this */
   VarType dataType;       /* what variable type represented here */
   uword8 attributes;      /* variable attributes */
};

struct Varmem
{
   /* Union for data access. The same structure is copied at the
    * beginning of each in the *exact same location* for each so that we can
    * swith between fast mempool or least-used memory
    */
   union {
      struct AllVarmem vall; /* VUndefined, VNull, or generic checking */
      struct {
         struct AllVarmem vall;
         jsenumber value;  /* VBoolean, VNumber */
      } vnumber;
      struct {
         struct AllVarmem vall;
         struct Varobj members;
         struct Function *function;     /* !=NULL for an object */
#        if (0!=JSE_CYCLIC_GC)
            struct Varmem **gcPrevPtr; /* links in varmem chain */
            struct Varmem *gcNext;
#        endif
         uword8 been_here;         /* many recursive calls that must not
                                    * redo the same varmem twice. */
      } vobject;
      struct {
         struct AllVarmem vall;
         void _HUGE_ *memory;           /* actually memory for string/buffer */
         JSE_POINTER_UINDEX alloced;    /* space available */
         JSE_POINTER_UINDEX count;      /* number of items used */
         JSE_POINTER_SINDEX zeroIndex;  /* for 0 < ArrayDimension */
      } vpointer;
   } data;
};


struct Varmem * NEAR_CALL varmemNew(struct Call *call,VarType dataType);

void NEAR_CALL varmemValidateIndex(struct Varmem *varmem,
                                   JSE_POINTER_SINDEX Index,
                                   JSE_POINTER_UINDEX Count,
                                   jsebool DeleteFromStartIfTooMany,
                                   jsebool DeleteFromEndIfTooMany);

/* struct Var is a generic var - it can be unresolved, and is what is
 * used by the API. You must resolve it to either VarRead or VarWrite
 * before doing the 'meaty' stuff on it.
 *
 * The values of 'member', 'object', and 'varmem' determine the type
 * of the variable and they are asserted liberally through the code.
 *
 *
 * type                            parentObject  memberName   varmem
 * --------------------------------------------------------------
 *   VAR types (includes VARWRITE and VARREAD)
 * reference                       non-null      non-null     null
 * reference to undefined          null          non-null     null
 *   VARWRITE types (includes VARREAD)
 * dynamic put waiting             non-null      non-null     non-null
 *   VARREAD types
 * real variable (varread)         null          null         non-null
 *
 *
 * The following are illegal:
 * fake dynamic put (canput false) null          non-null     non-null
 *                                 non-null      null         null
 *                                 non-null      null         non-null
 *                                 null          null         null
 *
 * The following macros help remember these relationships.
 */

#define VAR_HAS_DATA(V)       (NULL!=(V)->varmem) /* can read and write to this var */
#define VAR_HAS_REFERENCE(V)  ((VarName)0!=(V)->reference.memberName)
#define VAR_DYNAMIC_WRITE(V)  (VAR_HAS_REFERENCE(V) && VAR_HAS_DATA(V))
#define VAR_PURE_REFERENCE(V) (NULL==(V)->varmem)

struct Var
{
   struct Varmem *varmem;
#  if ( 2 <= JSE_API_ASSERTLEVEL )
      ubyte cookie;
#  endif
   uint userCount;    /* Number of current users of this structure */

   struct {
      VarRead *parentObject;
      VarName memberName;
   } reference;
      /* the above fields are used if this is a reference.  They are either both
       * NULL for a pure variable, or both non-null if this variable is a reference
       */

   JSE_POINTER_SINDEX offset_or_createType;
      /* for array types this is the offset, but for references that haven't been
       * written to yet, this will be the creation type for the variable if
       * it needs to be created for writing.
       */
   jsetinybool deref;
     /* If varmem is NULL, this is a reference. To convert a reference to a
      * writeable var, if it doesn't have a dynamic put you do it
      * just like converting to a readable var (find the actual var
      * it refers to and return it, incrementing its user count.)
      * Otherwise you clone this variable, set its varmem to
      * a new undefined value and return the new variable.
      *
      * Otherwise (if object!=NULL && varmem!=NULL) this is a VarWrite waiting
      * to a dynamic put on deletion, with this varmem a placeholder for the
      * value that will be dynamically put. It should start as undefined.
      *
      * deref pretends that varmem->dataType==VNumber, and means to
      * access the varmem as though it were a string/buffer, and the
      * value to be used is the single byte at this variable's offset.
      */

   jsetinybool lvalue;
     /* it is either an lvalue or not. Once it is turned into an lvalue,
      * we never turn it back (for example, if 'a+b' is passed as a parameter
      * to a function, the Var structure becomes an lvalue. We never come
      * back from the function and have to use it is a non-lvalue again.
      * Constants passed as params to a function are copied, and the original
      * constant is not messed up.)
      */
};


/*void NEAR_CALL varThoroughRemoveUser(struct Var *var,struct Call *call); */
     /* replacement for 'delete_var()', pass it a call so I don't repeat that
        call-save fiasco */


VarRead * NEAR_CALL constructVarRead(struct Call *call,VarType dataType);
   /* the most basic constructor, just construct a variable referring
    * to some real varmem, no special puts or gets or anything.
    */
struct Var * NEAR_CALL constructReference(struct Call *call,VarRead *object,
                                                 VarName membername);
   /* constructs a reference. */
#if (defined(JSE_FAST_MEMPOOL) && (0!=JSE_FAST_MEMPOOL)) || !defined(NDEBUG)
   VarRead * NEAR_CALL constructSibling(struct Call *call,
                                        VarRead *relativeVar,
                                        JSE_POINTER_SINDEX offset,
                                        jsebool Deref);
#  define CONSTRUCT_SIBLING(C,V,O,D)  constructSibling(C,V,O,D)
   VarRead * NEAR_CALL constructValueLock(struct Call *call,VarRead *original);
#  define CONSTRUCT_VALUE_LOCK(C,O)   constructValueLock(C,O)
#else
   VarRead * NEAR_CALL constructSibling(VarRead *relativeVar,
                                        JSE_POINTER_SINDEX offset,
                                        jsebool Deref);
#  define CONSTRUCT_SIBLING(C,V,O,D)  constructSibling(V,O,D)
   VarRead * NEAR_CALL constructValueLock(VarRead *original);
#  define CONSTRUCT_VALUE_LOCK(C,O)   constructValueLock(O)
#endif

/* VarRead * NEAR_CALL constructLock(struct Call *call,VarRead *original); */
     /* The first instance is very similar to the second in that they both
      * generate locks. However, the first is slower than the second. But,
      * they are not identical. The first one will give you a lock of the
      * variable's value. The second one will give you a lock of the variable
      * itself with a value that changes to reflect any changes to the
      * variable. Note that for integral types, both locks produce
      * identical results. It only matters if the variable's varmem changes,
      * in which case they are different. If you want a copy of the variable's
       value you should use copyAssign(). Objects are never copied.
      */

VarRead * NEAR_CALL reallyGetReadableVar(struct Var *var,struct Call *call);
#if !defined(JSE_INLINES) || (0==JSE_INLINES)
#  define GET_READABLE_VAR(VAR,CALL)  reallyGetReadableVar(VAR,CALL)
#else
#  define GET_READABLE_VAR(VAR,CALL) \
   ( VAR_HAS_DATA(VAR) ? (varAddUser(VAR),(VAR)) : reallyGetReadableVar(VAR,CALL) )
#endif
VarWrite * NEAR_CALL getWriteableVar(struct Var *var,struct Call *call);
   /* Note that if you have a VarRead that can be converted to a VarWrite
    * by just casting - all VarReads are also VarWrites. Both of these routines
    * return locked variables - i.e. you must call removeUser() on the result
    * when you are finished with it.
    */

void _HUGE_ * NEAR_CALL GetWriteableData(struct Var *var,VarRead *vRead,
                                         struct Call *call,
                                         JSE_POINTER_UINDEX *len);
   /* This requires the call since it may be forced to save some stuff
    * to be deleted later. Also, it goes here since if we have a readable
    * var, then the data is always writeable. We need to know when it is
    * taking the data from the prototype (which is what forces us to make
    * a copy.)
    */

#if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
void NEAR_CALL varPutValue(struct Var *var,struct Call *call,VarRead *value);
   /* This routine simply does a dynamic put with this variable
    * expected to be a reference (object/member) and the 'value'
    * parameter is the value to be put. The name is historic.
    */
#endif

#if (0!=JSE_CYCLIC_CHECK) && (0==JSE_FULL_CYCLIC_CHECK)
   void NEAR_CALL varReallyRemoveUser(struct Var *var,struct Call *call,
                                      jsebool look_hard);
#  if !defined(JSE_INLINES) || (0==JSE_INLINES)
#     define VAR_REMOVE_USER(this,call) varReallyRemoveUser(this,call,False)
#     define VAR_THOROUGH_REMOVE_USER(this,call) \
                varReallyRemoveUser(this,call,True)
#  else
#     define VAR_REMOVE_USER(this,call) if (0 == --((this)->userCount)) \
                   varReallyRemoveUser(this,call,False)
#     define VAR_THOROUGH_REMOVE_USER(this,call) if(0==--((this)->userCount))\
                   varReallyRemoveUser(this,call,True)
#  endif
#else
   void NEAR_CALL varReallyRemoveUser(struct Var *var,struct Call *call);
#  if !defined(JSE_INLINES) || (0==JSE_INLINES)
#     define VAR_REMOVE_USER(this,call) varReallyRemoveUser(this,call)
#  else
#     define VAR_REMOVE_USER(this,call) if (0 == --((this)->userCount)) \
                                            varReallyRemoveUser(this,call)
#  endif
#  define VAR_THOROUGH_REMOVE_USER(this,call)   VAR_REMOVE_USER(this,call)
#endif

#if (0!=JSE_CYCLIC_GC)
   ulong CollectCyclicGarbage(struct Call *call);
#endif


/*
 * A VarWrite can be a VarRead (i.e. just a real variable somewhere.)
 * However, when a Var that has a dynamic put property is turned into
 * a VarWrite, the VarWrite is different. Its varmem is used as a
 * temp storage area and when the VarWrite is destroyed, the associated
 * varmem is put dynamically.
 */


/* these functions that change the datatype require a call because
   they need it to discard the old varmem */
jsebool NEAR_CALL varAssign(VarWrite *varwrite,struct Call *call,
                            VarRead *Original);
jsebool NEAR_CALL varCopyAssign(VarWrite *varwrite,struct Call *call,
                                VarRead *Original);
   /* although we don't yet copy objects, we could. This should
    * copy a string though. It replaces DuplicateLiteralString but
    * is more generic and therefore probably more useful.
    */
void NEAR_CALL varConvert(VarWrite *varwrite,struct Call *call,
                          VarType newType);

void NEAR_CALL varPutString(VarWrite *varwrite,
                            struct Call *call,const jsechar _HUGE_ *data);
void NEAR_CALL varPutNumber(VarWrite *varwrite,jsenumber f);
   /* use this form when we know it is already a number of boolean */
void NEAR_CALL varPutNumberFix(VarWrite *varwrite,struct Call *call,jsenumber f,sword8 varType);
   /* use this form when it may need to be adjusted to a VNumber or VBoolean */

void NEAR_CALL varPutData(VarWrite *varwrite,struct Call *call,
                          const void _HUGE_ *data,JSE_POINTER_UINDEX Count,
                          VarType vType);

jsebool NEAR_CALL varDoAssign(VarWrite *varwrite,struct Call *call,
                              VarRead *Original,jsebool copy);



/* Note: this struct is like VarWrite, no extra data, just functions. It is
 * always assertable for this class that:
 * (object==NULL && member==NULL && varmem!=NULL)
 *
 * A VarRead is just real memory somewhere. Its varmem points to
 * a real value, and that's it. No references or any such confusions.
 */


jsenumber NEAR_CALL varGetValidNumber(VarRead *varread,struct Call *call);
jsenumber NEAR_CALL varGetNumber(VarRead *varread);

/* arrays are really only for string and buffer types, which keep each element
 * in an allocated array in memory
 */
JSE_POINTER_UINDEX NEAR_CALL varGetArrayLength(VarRead *varread,
                                               struct Call *call,
                                               JSE_POINTER_SINDEX *MinIndex);


/* You must be able to read a variable to access its class members.
 * Even if you want to write to the class members, you are reading
 * the variable that has the class members.
 */


/* Both routines return NULL if the member is not found. GetDirectMember()
 * will not search the prototype.
 */
VarRead * NEAR_CALL varMemberGet(struct Call *call,VarRead *varread,VarName Name,
                       jsebool AllowDefaultPrototypes,jsebool SearchPrototypeChain);
#define varGetDirectMember(call,varread,Name,AllowDefaultPrototypes) \
           varMemberGet(call,varread,Name,AllowDefaultPrototypes,False)
#define varGetMember(call,varread,Name) \
           varMemberGet(call,varread,Name,True,True)

VarRead * NEAR_CALL varGetNext(VarRead *varread,struct Call *call,
                               struct Var *Prev,
                               VarName *Name);
   /* needs the call to be able to turn VarNames into jsechar strings. */

void NEAR_CALL varDeleteMember(VarRead *varread,struct Call *call,
                               VarName Name);
   /* May end up being a dynamic delete which will need the call */

void NEAR_CALL varSetMemberAsAlias(VarRead *varread,struct Call *call,
                                   VarName name,VarRead *set_to_this);
   /* The member is set to be an exact alias as the passed in variable,
    * so that any changes will affect the original variable. It is
    * used to set up the 'arguments' array to map exactly to the parameters
    * passed to a function.
    */

void NEAR_CALL varSetArrayLength(VarRead *varread,struct Call *call,
                                 JSE_POINTER_SINDEX MinIndex,
                                 JSE_POINTER_UINDEX Length);
void NEAR_CALL varSetFunctionPtr(VarRead *varread,struct Call *call,
                                 struct Function *func);
   /*
    * The reason these are here rather than writeable variables is this:
    * Writeable variables basically get a whole new value plopped into
    * them, and don't rely on existing information (you Assign() a var
    * to them, Convert() them or whatever.) These functions take an
    * existing object and modify some aspect of it while still keeping
    * the rest of the object. That means that: (1) we don't want to
    * create a blank object with only that aspect for dynamic put.
    * (2) We don't want to create a new blank member of an object
    * to work with, but instead modify the existing one in a prototype.
    *
    * This is analogous to creating members of an object - if we do
    * 'a.x = 4;' and 'a' is found in the prototype, we use that 'a'.
    * We do NOT create a new 'a' in the main object then create an
    * 'x' in it. We use Readable on 'a' then Writeable on 'a.x'.
    * These other cases are similar - To set the array length,
    * we get the object as Readable and then delete any bad members
    * and to set the function we find the existing object and
    * change its function (this is basically an internal property,
    * and we don't go thru put to do it.)
    *
    * Note that some of the existing code was wrong in this case, but it
    * didn't show up because it was only noticeable in obscure cases
    * involving prototypes and/or dynamic put. Should be fixed now.
    */


/* The dynamic object functions */
#if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
   void NEAR_CALL varCallDynamicProperty(VarRead *varread,struct Call *call,
                                         VarRead *property,uword16 off_flag,
                                         VarName PropertyName,
                                         VarRead *Parameter2,
               /* NULL if not send a second parameter to function */
                                         VarRead ** ResultVar);
               /* if NULL then don't care, else this is result var from stack
                * and must call RemoveUser() on it */
   jsebool NEAR_CALL _varHasProperty(VarRead *varread,struct Call *call,
                                     VarName property);

   /* Because 99 times out of 100 a variable doesn't even have a _has
    * property, don't waste time calling the function
    */
#  if defined(JSE_INLINES) && (0!=JSE_INLINES)
#     define varHasProperty(VAR,CALL,PROP) \
         (HAS_HAS_PROPERTY(&(VAR->varmem->data.vobject.members)) ? \
         _varHasProperty(VAR,CALL,PROP) : varGetMember(CALL,VAR,PROP) != NULL)
#  else
#     define varHasProperty  _varHasProperty
#  endif

   VarRead * NEAR_CALL varCallConstructor(VarRead *varread,struct Call *call,
                                          VarRead *SourceVar);
   jsebool NEAR_CALL varCanPut(VarRead *varread,struct Call *call,
                               VarName name);
   VarRead * NEAR_CALL varDefaultValue(VarRead *varread,struct Call *call,
                                       jsebool hintstring);
# else
#      define varHasProperty(VAR,CALL,PROP)\
          (varGetMember(CALL,VAR,PROP) != NULL)
# endif

/* called by MakeMember or CreateMember */
VarRead * NEAR_CALL varBringMemberIntoExistance(VarRead *varread,
                                                struct Call *call,
                                                VarName Name,
                                                VarType DType,
                                                jsebool ForceTypeConversion);

#define varCreateMember(this,call,name,type) \
            varBringMemberIntoExistance(this,call,name,type,True)
#define varMakeMember(this,call,name,type) \
            varBringMemberIntoExistance(this,call,name,type,False)
   /* CreateMember() converts any existing member if already there
    * (i.e. converts to the correct datatype if needed, and sets attributes
    * to 0.)
    */
   /* MakeMember() is like varCreateMember(), but doesn't change the existing
    * one if there */


/* And finally some functions that ought to be moved elsewhere (they should
 * just be functions, not class members.) I propose we put them in
 * varutil.cpp
 */

VarRead * NEAR_CALL convert_var(struct Call *call,VarRead *SourceVar,
                                jseConversionTarget dest_type);
jsebool NEAR_CALL AssignFromText(VarWrite *target,struct Call *call,
                                 jsechar *Source,
                                 jsebool *AssignSuccess,
                                 jsebool MustUseFullSourceString,jsechar **End);
   /* 'default' for the last two is False,NULL */
jsebool NEAR_CALL varCompare(struct Call *call,VarRead *v1,VarRead *v2,
                             slong *Result);

#if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
   jsebool NEAR_CALL varCompareEquality(struct Call *call,VarRead *v1,
                                        VarRead *v2);
   sint NEAR_CALL varCompareLess(struct Call *call,VarRead *vx,
                                 VarRead *vy);
#else
   jsebool NEAR_CALL varECMACompareEquality(struct Call *call,VarRead *vx,
                                            VarRead *vy);
#  define varCompareEquality(CALL,V1,V2) \
          varECMACompareEquality(CALL,V1,V2)
   sint NEAR_CALL varECMACompareLess(struct Call *call,VarRead *vx,
                                     VarRead *vy);
#  define varCompareLess(CALL,V1,V2) \
          varECMACompareLess(CALL,V1,V2)
#endif

VarRead * NEAR_CALL GetDotNamedVar(struct Call *call,VarRead *me,
                                   const jsechar *NameWithDots,
                                   jsebool FinalMustBeVObject);
jsebool NEAR_CALL ArrayIndexDifference(struct Call *call,VarRead *vLeft,
                                       VarRead *vRight,
                                       JSE_POINTER_SINDEX *Difference);
VarRead * NEAR_CALL AutoConvert(struct Call *call,VarRead *convert_me,
                                jseVarNeeded need);
void NEAR_CALL ConcatenateStrings(struct Call *call,VarWrite *dest,
                                  VarRead *str1,VarRead *str2);


jsebool FindNames(struct Call *call,struct Var *me,
                  jsechar * const Buffer,uint BufferLength);
   /* these functions are used to print the name of an error variable */



#if defined(JSE_TOKENDST) && (0!=JSE_TOKENDST)
   VarRead * TokenReadVar(struct Call *call,struct TokenDst *tDst);
#endif

#if defined(JSE_TOKENSRC) && (0!=JSE_TOKENSRC)
   void TokenWriteVar(struct Call *call,struct TokenSrc *tSrc,
                      VarRead *write_me);
#endif

/* var inline functions - macros */

#define varAddUser(This)      ((This)->userCount++)
#define varmemAddUser(This)   ((This)->data.vall.userCount++)

#define varSetLvalue(this,val) ((this)->lvalue = (jsetinybool)val)

#define varPutStringLen(this,call,data,count) \
           varPutData(this,call,data,count,VString)
#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
#  define varPutBuffer(this,call,data,count) \
             varPutData(this,call,data,count,VBuffer)
#endif

#if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)
   /* other macros removed because they use their 'this'>1 time which is bad */
#  define hasCallProperty(this) ((this)->flags & HAS_CALL_PROP)
#endif

#define varIsActivation(this) \
    ((this)->varmem->data.vobject.members.flags & ACTIVATION)
#define varSetActivation(this) \
    ((this)->varmem->data.vobject.members.flags |= ACTIVATION)

#define varOffsetFromZero(this) ((this)->offset_or_createType)


/* varread */


#define varGetByte(this) ((ubyte)varGetNumber(this))
#define varGetLong(this) ((slong)varGetNumber(this))
#define varGetBoolean(this) ((jsebool)varGetNumber(this))


/* varmem  */

#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
#  define varmemArrayElementSize(this) \
      (VBuffer==(this)->data.vall.dataType?sizeof(ubyte):sizeof(jsechar))
#else
#  define varmemArrayElementSize(this) (sizeof(jsechar))
#endif

/* these are the varread inline functions */

#define varGetOffset(this) ((this)->offset_or_createType)
#define varGetAttributes(this) ((this)->varmem->data.vall.attributes)
#define varIsDeref(this) ((this)->deref)


/* varutil inline objects */

struct Function * NEAR_CALL varmemGetFunction(struct Varmem *This,
                                              struct Call *call);

#define varIsLvalue(this) ((this)->lvalue)

#define varSetAttributes(this,attr) \
   ((this)->varmem->data.vall.attributes = (uword8)attr)

#define VAR_IS_FUNCTION(this,call) \
   ((this)->varmem->data.vall.dataType==VObject && \
    varmemGetFunction((this)->varmem,call)!=NULL)

#define varGetFunction(this,call) (varmemGetFunction((this)->varmem,call))

#define varPutByte(this,c) varPutNumber((this),(jsenumber)(c))
#define varPutLong(this,c) varPutNumber((this),(jsenumber)(c))
#define varPutBoolean(this,c) varPutNumber((this),(jsenumber)c)

#define varAssign(this,call,orig) varDoAssign((this),(call),(orig),False)
#define varCopyAssign(this,call,orig) varDoAssign((this),(call),(orig),True)

jsebool NEAR_CALL ToBoolean(struct Call *call,VarRead *var);

/* truly inline functions */

void _HUGE_ * NEAR_CALL varGetData(VarRead *th,JSE_POINTER_SINDEX index);

void NEAR_CALL varReplaceVarmem(struct Var *th,struct Call *call,
                                struct Varmem *newmem);

#if defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING)
#  define HAS_OPERATOR_PROPERTY(th) \
   (((th)->flags & HAS_OPERATOR_PROP) && ((th)->flags & OFF_OPERATOR_PROP)==0)
#endif

#define HAS_PROTOTYPE_PROPERTY(varobj)   ( (varobj)->flags & HAS_PROTOTYPE_PROP )

#if defined(JSE_DYNAMIC_OBJS) && (0!=JSE_DYNAMIC_OBJS)

/* even though these evaluate their argument twice, in all cases they are
 * used on a parameter rather than say the result of a function, so it is safe.
 * Also, each is used only in a very few places (most (all?) only once), so it
 * make the program smaller to macroize them!
 */
#define HAS_PUT_PROPERTY(th) \
   (((th)->flags & HAS_PUT_PROP) && ((th)->flags & OFF_PUT_PROP)==0)
#define HAS_GET_PROPERTY(th) \
   (((th)->flags & HAS_GET_PROP) && ((th)->flags & OFF_GET_PROP)==0)
#define HAS_CANPUT_PROPERTY(th) \
   (((th)->flags & HAS_CANPUT_PROP) && ((th)->flags & OFF_CANPUT_PROP)==0)
#define HAS_HAS_PROPERTY(th) \
   (((th)->flags & HAS_HAS_PROP) && ((th)->flags & OFF_HAS_PROP)==0)
#define HAS_DELETE_PROPERTY(th) \
   (((th)->flags & HAS_DELETE_PROP) && ((th)->flags & OFF_DELETE_PROP)==0)
#endif

/* again these are only called on a set of fixed arguments so evalating them
 * (the arguments) more than once is OK.
 */
#define DYNAMIC_PUT_FIX(th,call,type) \
   if( VAR_HAS_REFERENCE(th) && (type)!=(th)->varmem->data.vall.dataType ) \
      varConvert((th),(call),(type))

/* What is this you ask? Well the varmem that we've constructed for the
 * dynamic put is undefined, so we must convert it to whatever type it is
 * actually being used for.
 */
#define VARMEM_IS_FUNCTION(th,call) \
   ((th)->data.vall.dataType==VObject && varmemGetFunction((th),(call))!=NULL)

#define varSameObject(one,two) ((one)->varmem==(two)->varmem)

#if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
#  define VAR_ARRAY_POINTER(th) (VString==th->varmem->data.vall.dataType ||\
                                 VBuffer==th->varmem->data.vall.dataType)
#  define TYPE_IS_ARRAY(type)  (VString==type || VBuffer==type)
#else
#  define VAR_ARRAY_POINTER(th) (VString==th->varmem->data.vall.dataType)
#  define TYPE_IS_ARRAY(type)  (VString==type)
#endif

#define VAR_TYPE(th) (VarType)(((th)->deref?VNumber:(th)->varmem->data.vall.dataType))

#if defined(__cplusplus)
   }
#endif
#endif
