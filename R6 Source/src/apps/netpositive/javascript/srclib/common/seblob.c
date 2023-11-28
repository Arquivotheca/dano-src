/* seblob.c
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

#include "jseopt.h"

#if defined(JSE_SELIB_BLOB_GET) || \
    defined(JSE_SELIB_BLOB_PUT) || \
    defined(JSE_SELIB_BLOB_SIZE) || \
    defined(JSE_CLIB_FREAD)   || \
    defined(JSE_CLIB_FWRITE)  || \
    defined(JSE_SELIB_PEEK)    || \
    defined(JSE_SELIB_POKE)

static CONST_DATA(jsechar) blobSharedDataName[] = UNISTR("_BigEndianMode");
static CONST_DATA(jsechar) blobObjectName[] = UNISTR("blobDescriptor");
static CONST_DATA(jsechar) blobTypeName[] = UNISTR("blobType");
CONST_DATA(jsechar) blobInternalDataMember[] = UNISTR("__blobdata");

jsebool NEAR_CALL blobBigEndianMode(jseContext jsecontext);
void NEAR_CALL blobInitializeBigEndianMode(jseContext jsecontext,jsebool InitialBigEndianMode);
jseVariable blobCreateBLobType(jseContext jsecontext, long value);
ulong blobLength( slong type );
jsebool blobPutByValue(jseContext jsecontext, ubyte _HUGE_ * mem, ulong datalen, slong type,
                 jseVariable source, jsebool bigEndian);
jsebool blobGetByValue(jseContext jsecontext,ubyte _HUGE_ * mem, ulong datalen, slong type, 
                 jseVariable dest, jsebool bigEndian);

static jseLibFunc(BLOBObjectDelete);
static jseLibFunc(BLOBObjectGet);
static jseLibFunc(BLOBObjectPut);

struct BLobDescNode {
   struct BLobDescNode * prev;
   struct BLobDescNode * next;
   jsechar         * name;
   slong          value;
   jseVariable    variable;
};

struct BLobDescriptor {
   struct BLobDescNode * root;
};

   static struct BLobDescriptor * NEAR_CALL
blobdescriptorNew(void)
{
   struct BLobDescriptor *This = jseMustMalloc(struct BLobDescriptor,sizeof(struct BLobDescriptor));
   This->root = NULL;
   return This;
}


   static struct BLobDescNode * NEAR_CALL
blobDescNew(void)
{
   struct BLobDescNode *This = jseMustMalloc(struct BLobDescNode,sizeof(struct BLobDescNode));
   This->prev = NULL;
   This->next = NULL;
   This->name = NULL;
   This->value = 0;
   This->variable = NULL;
   return This;
}

   static void NEAR_CALL
blobdescriptorDelete(struct BLobDescriptor * This)
{
   assert( This->root == NULL );  /* delete_all should always be called first */
   jseMustFree(This);
}

   static jsebool NEAR_CALL
isBlobDescriptor( jseContext jsecontext, jseVariable var )
{
  jseVariable classname;
  const jsechar _HUGE_ *cname;
  jsebool result;
  
  if ( jseGetType( jsecontext, var ) != jseTypeObject )
    return False;

  classname = jseGetMember( jsecontext, var, CLASS_PROPERTY );
  if ( classname == NULL )
    return False;

  cname = jseGetString(jsecontext,classname,NULL);
  result =  strcmp_jsechar(cname, blobObjectName ) != 0;
  
  if ( result )
    return False;

  /* This is an extra paranoid check that should never fail */
  assert( NULL != jseGetMember(jsecontext, var, blobInternalDataMember) );

  return True;
}

   jsebool NEAR_CALL
isBlobType( jseContext jsecontext, jseVariable var );

   jsebool NEAR_CALL
isBlobType( jseContext jsecontext, jseVariable var )
{
  jseVariable classname;
  #ifndef NDEBUG
    jseVariable member;
    slong value;
  #endif /* !NDEBUG */
  jsebool result;
  
  if( jseGetType(jsecontext,var) != jseTypeObject )
    return False;

  classname = jseGetMember(jsecontext,var,CLASS_PROPERTY);
  if(classname == NULL)
    return False;
  result = strcmp_jsechar(jseGetString(jsecontext,classname,NULL),blobTypeName) != 0;

  if( result )
     return False;

  /* Unless some user is messing around, the following should always be true */
  #ifndef NDEBUG
    member = jseGetMember(jsecontext,var,blobInternalDataMember);
    assert( member != NULL );
    value = jseGetLong(jsecontext,member);
    assert( value < 0 && value > blobLAST_DATUM_TYPE);
  #endif /* !NDEBUG */

  return True;
}

   static jsebool NEAR_CALL
blobDescPutObject(struct BLobDescriptor * blobDesc,jseContext jsecontext,ubyte _HUGE_ * mem,ulong datalen,jseVariable source,jsebool bigEndian)
{
   struct BLobDescNode * current = blobDesc->root;
   jseVariable member;

   while( NULL != current )
   {
      ulong memberLen;
      if ( NULL == current->variable )
      {
         memberLen = blobLength(current->value);
      }
      else
      {
#        ifndef NDEBUG
            assert( blobDataTypeLen(jsecontext,current->variable,&memberLen) );
#        else
            blobDataTypeLen(jsecontext,current->variable,&memberLen);
#        endif
      }
      member = jseGetMember(jsecontext,source,current->name);

      if( ('_' == current->name[0])  ||  (NULL == member) )
      {
         /* fill with zeros - safer if what we're calling wants a NULL */
         HugeMemSet(mem,0,memberLen);
      }
      else
      {
         if( NULL == current->variable )  /* We have a numeric datum, just call putByValue directly */
         {
            if( !blobPutByValue(jsecontext,mem,memberLen,current->value,member,bigEndian) )
               return False;
         }
         else
         {
            struct BLobDescriptor * desc;
            assert( isBlobDescriptor(jsecontext,current->variable) );
            desc = (struct BLobDescriptor *) jseGetLong( jsecontext,
               jseGetMember( jsecontext, current->variable, blobInternalDataMember ) );
            if( !blobDescPutObject(desc,jsecontext,mem,datalen,member,bigEndian) )
               return False;
         }
      }

      mem += memberLen;
      current = current->next;
   }

   return True;
}

   static jsebool NEAR_CALL
blobDescGetObject(struct BLobDescriptor * blobDesc,jseContext jsecontext,ubyte _HUGE_ * mem,ulong datalen,jseVariable source,jsebool bigEndian)
{
   struct BLobDescNode * current = blobDesc->root;
   jseVariable member;

  while( current != NULL )
  {
    ulong memberLen;
    if ( current->variable == NULL )
      memberLen = blobLength(current->value);
    else
      #ifndef NDEBUG
        assert( blobDataTypeLen(jsecontext,current->variable,&memberLen) );
      #else
        blobDataTypeLen(jsecontext,current->variable,&memberLen);
      #endif
    member = jseMember(jsecontext,source,current->name,jseTypeUndefined);

    if ( current->name[0] != '_')
    {
      if( current->variable == NULL )  /* We have a numeric datum, just call putByValue directly */
      {
        if( !blobGetByValue(jsecontext,mem,memberLen,current->value,member,bigEndian) )
          return False;
      }
      else
      {
        struct BLobDescriptor * desc;
        assert( isBlobDescriptor(jsecontext,current->variable) );
        desc = (struct BLobDescriptor *) jseGetLong( jsecontext,jseGetMember( jsecontext, current->variable, blobInternalDataMember ) );
        jseConvert(jsecontext,member,jseTypeObject);
        if( !blobDescGetObject(desc,jsecontext,mem,datalen,member,bigEndian) )
          return False;
      }
    }

    mem += memberLen;
    current = current->next;
  }

  return True;
}

   static jsebool NEAR_CALL
blobDescLength(struct BLobDescriptor * blobDesc,jseContext jsecontext,ulong * datalen)
{
  struct BLobDescNode * current = blobDesc->root;
  ulong total_length = 0;

  while( current != NULL )
  {
    if( current->variable == NULL )
      total_length += blobLength(current->value);
    else
    {
      struct BLobDescriptor * desc;
      ulong memberLen = 0;
      assert( isBlobDescriptor(jsecontext,current->variable) );
      desc = (struct BLobDescriptor *) jseGetLong( jsecontext,jseGetMember( jsecontext, current->variable, blobInternalDataMember ) );
      if( !blobDescLength(desc,jsecontext,&memberLen) )
        return False;
      total_length += memberLen;
    }

    current = current->next;
  }

  *datalen = total_length;
  return True;
}

   static void NEAR_CALL
blobDescDelete_all(struct BLobDescriptor * blobDesc,jseContext jsecontext)
{
  struct BLobDescNode * temp;
  while( blobDesc->root != NULL )
  {
    temp = blobDesc->root->next;
    jseMustFree( blobDesc->root->name );
    if( blobDesc->root->variable != NULL )
    {
      jseDestroyVariable(jsecontext,blobDesc->root->variable);
    }
    jseMustFree(blobDesc->root);
    blobDesc->root = temp;
  }
}

   static void NEAR_CALL
blobDescAdd(struct BLobDescriptor * blobDesc,jseContext jsecontext,
            const jsechar * name, jseVariable variable)
{
  struct BLobDescNode * new_node;
  struct BLobDescNode * current = blobDesc->root;

  if ( !strcmp_jsechar(name,PUT_PROPERTY) ||
       !strcmp_jsechar(name,PROTOTYPE_PROPERTY) ||
       !strcmp_jsechar(name,GET_PROPERTY) ||
       !strcmp_jsechar(name,DELETE_PROPERTY) ||
       !strcmp_jsechar(name,CLASS_PROPERTY) ||
       !strcmp_jsechar(name,blobInternalDataMember) )
     return;

  while( current != NULL )
  {
    if( !strcmp_jsechar( current->name, name ) )
    {
      return;
    }
    current = current->next;
  }

  new_node = blobDescNew();

  new_node->variable = NULL;
  new_node->name = StrCpyMalloc( name );
  if ( jseTypeNumber == jseGetType(jsecontext,variable) )
    new_node->value = jseGetLong(jsecontext,variable);
  else
  {
    assert( jseTypeObject == jseGetType(jsecontext,variable) );
    if ( isBlobType(jsecontext,variable) )
      new_node->value = jseGetLong(jsecontext,
                                   jseGetMember(jsecontext,variable,
                                                blobInternalDataMember));
    else
    {
      assert( isBlobDescriptor(jsecontext,variable) );
      new_node->variable = jseCreateSiblingVariable(jsecontext,variable,0);
    }
  }

  if ( blobDesc->root == NULL )
  {
    new_node->prev = NULL;
    new_node->next = NULL;
    blobDesc->root = new_node;
  }
  else
  {
    current = blobDesc->root;
    while ( current->next != NULL )
      current = current->next;

    new_node->prev = current;
    new_node->next = NULL;
    current->next = new_node;
  }
}

   static void NEAR_CALL
blobDescRemove(struct BLobDescriptor * blobDesc,jseContext jsecontext,
               const jsechar * name )
{
  struct BLobDescNode * current = blobDesc->root;

  while( current != NULL )
  {
    if( !strcmp_jsechar( current->name, name ) )
    {
      break;
    }
    current = current->next;
  }

  if ( current == NULL )
    return;

  jseMustFree( current->name );
  if( current->variable != NULL )
    jseDestroyVariable(jsecontext,current->variable);

  if ( current->next )
    current->next->prev = current->prev;
  if ( current->prev )
    current->prev->next = current->next;
  if ( current == blobDesc->root )
    blobDesc->root = current->next;

  /*delete current;*/
}

   static jsebool NEAR_CALL
blobDescPresent( struct BLobDescriptor * blobDesc,const jsechar * name )
{
  struct BLobDescNode * current = blobDesc->root;

  while( current != NULL )
  {
    if( !strcmp_jsechar( current->name, name ) )
    {
      return True;
    }
    current = current->next;
  }

  return False;
}

   static jseVariable NEAR_CALL
blobDescGet( struct BLobDescriptor * blobDesc,jseContext jsecontext,
             const jsechar * name, jsebool *deleteIt )
{
  struct BLobDescNode * current = blobDesc->root;

  while( current != NULL )
  {
    if( !strcmp_jsechar( current->name, name ) )
    {
      if( current->variable == NULL )
      {
        *deleteIt = True;
        return blobCreateBLobType(jsecontext,current->value);
      }
      else
      {
        *deleteIt = False;
        return current->variable;
      }
    }
    current = current->next;
  }

  return NULL;
}

/*********************************************
 ***** END INTERNAL BLOBDESCRIPTOR STUFF *****
 *********************************************/

   jsebool NEAR_CALL
blobBigEndianMode(jseContext jsecontext)
{
   assert( NULL != jseGetSharedData(jsecontext,blobSharedDataName) );
   return ((int) jseGetSharedData(jsecontext,blobSharedDataName)) == 2;
}

   void NEAR_CALL
blobInitializeBigEndianMode(jseContext jsecontext,jsebool InitialBigEndianMode)
{
   if( jseGetSharedData(jsecontext,blobSharedDataName) == NULL )
   {
      #if !defined(BIG_ENDIAN) && \
          (BIG_ENDIAN != True && BIG_ENDIAN != False)
      #error Must define BIG_ENDIAN to use blobs
      #endif
      /* We cannot use 1 or 0 because 0 is the equivalent of null */
      if( InitialBigEndianMode )
         jseSetSharedData(jsecontext,blobSharedDataName,(void _FAR_ *) 2,NULL);
      else
         jseSetSharedData(jsecontext,blobSharedDataName,(void _FAR_ *) 1,NULL);
   }
}

static void NEAR_CALL
memrev(ubyte *Mem,uint Len)
{
   ubyte *EndMem;
   for ( EndMem = Mem + Len - 1; Mem < EndMem; Mem++, EndMem-- ) {
      ubyte Temp = *EndMem;
      *EndMem = *Mem;
      *Mem = Temp;
   } /* endfor */
}

static jseVariable
makeBlobDescriptor(jseContext jsecontext)
{
  jseVariable obj = jseCreateVariable( jsecontext, jseTypeObject );
  jseVariable temp;
  struct BLobDescriptor * desc;
  
  /* "class" member */
  temp = jseMember( jsecontext, obj, CLASS_PROPERTY, jseTypeString );
  jsePutString( jsecontext, temp, blobObjectName );
  jseSetAttributes( jsecontext, temp, jseDontEnum | jseReadOnly );

  /* "_data" member */
  desc = blobdescriptorNew();
  temp = jseMember( jsecontext, obj, blobInternalDataMember, jseTypeNumber );
  jsePutLong( jsecontext, temp, (slong) desc );
  jseSetAttributes( jsecontext, temp, jseDontEnum | jseReadOnly );

  /* "delete" member */
  jseMemberWrapperFunction( jsecontext, obj, DELETE_PROPERTY, BLOBObjectDelete,1,1,jseDontEnum,jseFunc_PassByReference,NULL);

  /* "get" member */
  jseMemberWrapperFunction( jsecontext, obj, GET_PROPERTY, BLOBObjectGet,1,1,jseDontEnum,jseFunc_PassByReference,NULL);

  /* "put" member */
  jseMemberWrapperFunction( jsecontext, obj, PUT_PROPERTY, BLOBObjectPut,2,2,jseDontEnum,jseFunc_PassByReference,NULL);

  return obj;
}

jseVariable
blobCreateBLobType(jseContext jsecontext, long value)
{
  jseVariable var = jseCreateVariable(jsecontext,jseTypeObject);
  jseVariable member = jseMemberEx(jsecontext,var,CLASS_PROPERTY,jseTypeString,jseCreateVar);
  
  jsePutString(jsecontext,member,blobTypeName);
  
  jseSetAttributes(jsecontext,member,jseDontEnum|jseReadOnly|jseDontDelete);
  jseDestroyVariable(jsecontext,member);
  member = jseMemberEx(jsecontext,var,blobInternalDataMember,jseTypeNumber,jseCreateVar);
  jsePutLong(jsecontext,member,value);
  jseSetAttributes(jsecontext,member,jseDontEnum|jseReadOnly|jseDontDelete);
  jseDestroyVariable(jsecontext,member);
  return var;
}

ulong
blobLength( slong type )
{
  ulong length;
#ifndef NDEBUG
  slong testlen;
#endif
  
  if( type >= 0 )
    return (ulong) type;

  #ifdef __JSE_MAC__
  if( type == blobSTR255 || type == blobSTR63 || type == blobSTR31 )
  {
    /* This is a pascal string, add one byte for length at beggining */
    switch( type )
    {
      case blobSTR255:
        return (ulong) 256;
      case blobSTR63:
        return (ulong) 64;
      case blobSTR31:
        return (ulong) 32;
      default:
        assert( JSE_DEBUG_FEEDBACK(False) ); /* Why are we here? */
        return (ulong) 0;
    }
  } else
  #endif /* __JSE_MAC__ */

  #if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
  if ( type <= blobFLOAT32 )
  {
    /* Floating point number */
    switch( type )
    {
    #if !defined(_MSC_VER)
    case blobFLOAT80:
      return (ulong) 10;
    #endif
    case blobFLOAT64:
      return (ulong) 8;
    case blobFLOAT32:
      return (ulong) 4;
    default:
      assert( JSE_DEBUG_FEEDBACK(False) );
      return (ulong) 0;
    }
  } 
  #endif /* JSE_FLOATING_POINT */

  /* Integral value */
  length = (ulong)(((-type)+1) >> 1);
  #ifndef NDEBUG
    /* Check math assumptions */
    switch( type )
    {
      case blobUWORD8:  case blobSWORD8:  testlen = 1; break;
      case blobUWORD16: case blobSWORD16: testlen = 2; break;
      case blobUWORD24: case blobSWORD24: testlen = 3; break;
      case blobUWORD32: case blobSWORD32: testlen = 4; break;
      default:  testlen = 0;  assert( JSE_DEBUG_FEEDBACK(False) );
    }
    assert( testlen == (slong)length );
  #endif
  return (ulong) length;
}

jsebool
blobPutByValue(jseContext jsecontext, ubyte _HUGE_ * mem, ulong datalen, slong type,
                 jseVariable source, jsebool bigEndian)
{
  assert( datalen == blobLength(type) );

  if( type >= 0 )
  {
    if( !jseVarNeed(jsecontext,source,JSE_VN_CONVERT(JSE_VN_STRING|JSE_VN_BUFFER,JSE_VN_BUFFER)) )
      return False;
    if( datalen > 0 )
    {
      ulong bufflen = jseGetArrayLength(jsecontext,source,NULL);
      if( bufflen < datalen )
      {
        /* Size to put is bigger than data vable, so set extra as null bytes */
        HugeMemSet(mem,0,datalen);
        datalen = bufflen;
      }
      HugeMemCpy(mem,jseGetBuffer(jsecontext,source,&bufflen),datalen);
      assert( datalen <= bufflen );
    }
  } else

  #ifdef __JSE_MAC__
  if( type == blobSTR255 || type == blobSTR63 || type == blobSTR31 )
  {
    ulong str_len;
    const jsechar * theString = jseGetString(jsecontext,source,&str_len);

    if( jseGetType(jsecontext,source) == jseTypeUndefined ) /* This is OK */
      return True;
    if( !jseVarNeed(jsecontext,source,JSE_VN_STRING) )
      return False;
    if( str_len > datalen - 1 )
      str_len = datalen - 1;
    ((unsigned char *)mem)[0] = (unsigned char) str_len;
    memcpy(mem+sizeof(jsechar),theString,sizeof(jsechar)*str_len);
  } else
  #endif /* __JSE_MAC__ */

  {
  #if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
    if( type <= blobFLOAT32 )
    {
      jsenumber f;
      if( !jseVarNeed(jsecontext,source,JSE_VN_NUMBER) )
        return False;
      f = jseGetNumber(jsecontext,source);
      switch( type )
      {
        case blobFLOAT32:
          ((float32 _FAR_ *)mem)[0] = (float32) f;
          break;
        case blobFLOAT64:
          ((float64 _FAR_ *)mem)[0] = (float64) f;
          break;
        #if !defined(_MSC_VER)
        case blobFLOAT80:
          ((float80 _FAR_ *)mem)[0] = (float80) f;
          break;
        #endif /* !_MSC_VER */
        default:
          assert(JSE_DEBUG_FEEDBACK(False));
      }
    } else
    #endif /* JSE_FLOATING_POINT */
    
    /* This must be a datum */
    {
      slong l = jseGetLong(jsecontext,source);
      #if BIG_ENDIAN == True
        assert( sizeof(ubyte) == JSE_DEBUG_FEEDBACK(1) );
        memcpy(mem,((ubyte *)(&l))+(sizeof(l)-datalen),(uint)datalen);
      #elif BIG_ENDIAN == False
        memcpy(mem,&l,(uint)datalen);
      #else
        #error Invalid BIG_ENDIAN
      #endif
    }

    /* Now we must check to see if we must reverse the memory */
    #if BIG_ENDIAN == True
    if( !bigEndian )
    #elif BIG_ENDIAN == False
    if( bigEndian )
    #else
      #error Invalid BIG_ENDIAN
    #endif
      memrev((ubyte _FAR_ *)mem,(uint)datalen);
  }

  return True;
}

jsebool
blobGetByValue(jseContext jsecontext,ubyte _HUGE_ * mem, ulong datalen, slong type, jseVariable dest, jsebool bigEndian)
{

  if( type >= 0 )
  {
    jseConvert(jsecontext,dest,jseTypeBuffer);
    jsePutBuffer(jsecontext,dest,mem,datalen);
  } else

  #if defined(__JSE_MAC__)
  if ( type == blobSTR255 || type == blobSTR63 || type == blobSTR31 )
  {
    ubyte len = *((ubyte *)mem);
    jseConvert(jsecontext,dest,jseTypeString);
    jsePutStringLength(jsecontext,dest,(jsechar *)mem+1,(ulong)len);
  } else
  #endif /* __JSE_MAC__ */

  {
    ubyte datum[30];  /* This should be large enough */
    assert(datalen <= 30);
    memcpy(datum,mem,(uint)datalen);
    jseConvert(jsecontext,dest,jseTypeNumber);
   #if BIG_ENDIAN==True
    if (!bigEndian)
   #elif BIG_ENDIAN == False
    if (bigEndian)
   #else
    #error Invalid BIG_ENDIAN
   #endif
      memrev(datum,(uint)datalen);

    #if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
    if ( type <= blobFLOAT32 )
    {
      jsenumber f;
      switch( type )
      {
        case blobFLOAT32: f = ((float32 *)datum)[0]; break;
        case blobFLOAT64: f = ((float64 *)datum)[0]; break;
        #if !defined(_MSC_VER)
        case blobFLOAT80: f = ((float80 *)datum)[0]; break;
        #endif
        default:
          assert( JSE_DEBUG_FEEDBACK(False) );
      }
      jsePutNumber(jsecontext,dest,f);
    } else
    #endif /* JSE_FLOATING_POINT */

    {
      if( type == blobUWORD8)
        jsePutByte(jsecontext,dest,((uword8 *)mem)[0]);
      else
      {
        slong l = 0;
        assert( datalen <= sizeof(l) );
        if ( !((-type) & 1) )
        {
          assert( blobSWORD8 == type || blobSWORD16 == type || blobSWORD24 == type || blobSWORD32 == type );
         #if BIG_ENDIAN == True
          if ( ((sword8 *)datum)[0] < 0 )
         #elif BIG_ENDIAN == False
          if ( ((sword8 *)datum)[(uint)datalen-1] < 0 )
         #else
          #error Invalid BIG_ENDIAN
         #endif
           l = -1;
        }
        else
          assert( blobUWORD16 == type || blobUWORD24 == type || blobUWORD32 == type );
        #if BIG_ENDIAN == True
          memcpy(((ubyte *)(&l))+(sizeof(l)-datalen),datum,(uint)datalen);
        #elif BIG_ENDIAN == False
          memcpy(&l,datum,(uint)datalen);
        #else
          #error Invalid BIG_ENDIAN
        #endif
        jsePutLong(jsecontext,dest,l);
      }
    }
  }

  return True;
}

jsebool NEAR_CALL
blobDataTypeLen(jseContext jsecontext, jseVariable TypeOrLenVar, ulong *DataLength)
{
  slong type;
  jseDataType vType;
  struct BLobDescriptor * desc;

  assert( NULL != TypeOrLenVar );

  vType = jseGetType(jsecontext,TypeOrLenVar);

  if ( jseTypeNumber == vType )
  {
    type = jseGetLong(jsecontext,TypeOrLenVar);
    if ( type >= 0 )
    {
      *DataLength = blobLength(type);
      return True;
    }
    /* This will fall through to below where the error is printed */
  }
  else if ( jseTypeObject == vType && isBlobType(jsecontext,TypeOrLenVar) )
  {
    type = jseGetLong(jsecontext,jseGetMember(jsecontext,TypeOrLenVar,blobInternalDataMember));
    *DataLength = blobLength(type);
    return True;
  }
  else if ( jseTypeObject == vType && isBlobDescriptor(jsecontext,TypeOrLenVar) )
  {
    desc = (struct BLobDescriptor *) jseGetLong(jsecontext,jseGetMember(jsecontext,TypeOrLenVar,blobInternalDataMember));
    return blobDescLength(desc,jsecontext,DataLength);
  }

  jseLibErrorPrintf(jsecontext,textlibGet(textlibINVALID_DATA_DESCRIPTION));
  return False;
}

jsebool NEAR_CALL
blobPut(jseContext jsecontext,ubyte _HUGE_ *mem,ulong datalen,
          jseVariable TypeOrLenVar,jseVariable SrcVar,jsebool BigEndianState)
{
   jseDataType vType;
   #ifndef NDEBUG
      ulong TempDataLen;
      assert( blobDataTypeLen(jsecontext,TypeOrLenVar,&TempDataLen) );
      assert( datalen == TempDataLen );
      assert( jseTypeObject == jseGetType(jsecontext,TypeOrLenVar) || jseTypeNumber == jseGetType(jsecontext,TypeOrLenVar) );
      assert( NULL != SrcVar );
   #endif

   vType = jseGetType(jsecontext,TypeOrLenVar);
   if ( vType == jseTypeNumber )
   {
      slong type;
      type = jseGetLong(jsecontext,TypeOrLenVar);
      if ( type < 0 )
        return False;
      else
        return blobPutByValue(jsecontext,mem,datalen,type,SrcVar,BigEndianState);
   }
   else if ( vType == jseTypeObject && isBlobType(jsecontext,TypeOrLenVar) )
   {
      slong type;
      type = jseGetLong(jsecontext,jseGetMember(jsecontext,TypeOrLenVar,blobInternalDataMember));
      return blobPutByValue(jsecontext,mem,datalen,type,SrcVar,BigEndianState);
   }
   else
   {
      assert(jseTypeObject == vType);
      /* BLOb::Put each matching element of structure */
      if ( jseTypeUndefined == jseGetType(jsecontext,SrcVar) ) {
         /* not a structure, but could convert to one; no matching elements; OK */
         HugeMemSet(mem,0,datalen);
      }
      else
      {
         struct BLobDescriptor * desc;

         if ( !jseVarNeed(jsecontext,SrcVar,JSE_VN_OBJECT) )
            return False;
         assert( isBlobDescriptor(jsecontext,TypeOrLenVar ) );
         desc = (struct BLobDescriptor *) jseGetLong( jsecontext,
                                                                jseGetMember( jsecontext, TypeOrLenVar, blobInternalDataMember ) );
         return blobDescPutObject(desc,jsecontext,mem,datalen,SrcVar,BigEndianState);
      }
   }
   return True;
}

jsebool NEAR_CALL
blobGet(jseContext jsecontext,jseVariable GetVar,
        ubyte _HUGE_ *mem,jseVariable TypeOrLenVar,jsebool BigEndianState)
{
   ulong datalen;
   jseDataType vType;

   assert( NULL != TypeOrLenVar );
   assert( NULL != mem );

   if ( !blobDataTypeLen(jsecontext,TypeOrLenVar,&datalen) )
      return False;

   vType = jseGetType(jsecontext,TypeOrLenVar);

   if(vType == jseTypeNumber)
   {
      slong len = jseGetLong(jsecontext,TypeOrLenVar);
      if ( len < 0 )
        return False;
      return blobGetByValue(jsecontext,mem,datalen,len,GetVar,BigEndianState);
   }
   else if(vType == jseTypeObject && isBlobType(jsecontext,TypeOrLenVar))
   {
      slong type = jseGetLong(jsecontext,jseGetMember(jsecontext,TypeOrLenVar,blobInternalDataMember));
      return blobGetByValue(jsecontext,mem,datalen,type,GetVar,BigEndianState);
   }
   else
   {
      struct BLobDescriptor * desc;
      assert( isBlobDescriptor( jsecontext, TypeOrLenVar ) );
      jseConvert(jsecontext,GetVar,jseTypeObject);
      desc = (struct BLobDescriptor *) jseGetLong( jsecontext,
                                                                jseGetMember( jsecontext, TypeOrLenVar, blobInternalDataMember ) );
      return blobDescGetObject(desc,jsecontext,mem,datalen,GetVar,BigEndianState);
   }

}

static jseLibFunc( BLOBObjectConstructor )
{
  jseVariable obj = makeBlobDescriptor(jsecontext);

  jseReturnVar( jsecontext, obj, jseRetTempVar );
}

static jseLibFunc(BLOBObjectPut)
{
  struct BLobDescriptor * desc;
  jseVariable thisvar;
  jseVariable data;
  jseVariable jseValue;
  jseVariable jseName;
  const jsechar _HUGE_ * name;
  
  thisvar = jseGetCurrentThisVariable( jsecontext );

  data = jseGetMember( jsecontext, thisvar, blobInternalDataMember );
  if ( data == NULL )
     return;

  desc = (struct BLobDescriptor *) jseGetLong( jsecontext, data );

  JSE_FUNC_VAR_NEED( jseName, jsecontext, 0, JSE_VN_STRING );
  jseValue = jseFuncVar( jsecontext, 1 );
  if ( jseValue == NULL )
    return;

  name = jseGetString( jsecontext, jseName, NULL );
  
  if ( !strcmp_jsechar( name, GET_PROPERTY ) ||
       !strcmp_jsechar( name, PUT_PROPERTY ) )
  {
     return;
  }

  if ( jseGetType( jsecontext, jseValue ) == jseTypeNumber )
  {
    if ( jseGetLong(jsecontext,jseValue) < 0 )
    {
      jseLibErrorPrintf(jsecontext,textlibGet(textlibINVALID_BLOB_DESC_MEMBER),blobObjectName);
      return;
    }
  }
  else if ( jseGetType( jsecontext, jseValue ) == jseTypeObject )
  {
    if ( !isBlobType(jsecontext,jseValue) && !isBlobDescriptor(jsecontext,jseValue ) )
    {
      /* This is test code to catch instances of 'descriptor.a.b = SWORD32', where 'a' is not defined
         In the future we should try to convert the object to a blobDescriptor */
         
      /* jseLibErrorPrintf(jsecontext,textlibGet(INVALID_BLOB_DESC_MEMBER),blobObjectName); */
      jseVariable temp = makeBlobDescriptor(jsecontext);
      blobDescAdd( desc, jsecontext, name, temp );
      jseDestroyVariable( jsecontext, temp );
      return;
    }
  }
  else
  {
    jseLibErrorPrintf(jsecontext,textlibGet(textlibINVALID_BLOB_DESC_MEMBER),blobObjectName);
    return;
  }

  jseAssign( jsecontext, jseMember(jsecontext,thisvar,name,jseTypeUndefined), jseValue );
  blobDescAdd( desc,jsecontext, name, jseValue );
}

static jseLibFunc(BLOBObjectGet)
{
  jseVariable thisvar;
  jseVariable data;
  jseVariable jseName;
  const jsechar * name;
  struct BLobDescriptor * desc;

  thisvar = jseGetCurrentThisVariable( jsecontext );

  JSE_FUNC_VAR_NEED( jseName, jsecontext, 0, JSE_VN_STRING );
  name = (const jsechar *)jseGetString( jsecontext, jseName, NULL );
  
  data = jseGetMember( jsecontext, thisvar, blobInternalDataMember );
  assert( data != NULL );

  if ( !strcmp_jsechar(name,PUT_PROPERTY) ||
       !strcmp_jsechar(name,PROTOTYPE_PROPERTY) ||
       !strcmp_jsechar(name,GET_PROPERTY) ||
       !strcmp_jsechar(name,DELETE_PROPERTY) ||
       !strcmp_jsechar(name,CLASS_PROPERTY) ||
       !strcmp_jsechar(name,blobInternalDataMember) )
  {
    jseReturnVar(jsecontext,jseMember(jsecontext,thisvar,name,jseTypeUndefined),
                 jseRetCopyToTempVar);
  }
  else
  {
     desc = (struct BLobDescriptor *) jseGetLong( jsecontext, data );

     if ( !blobDescPresent(desc,name) )
     {
       jseReturnVar(jsecontext,jseCreateVariable(jsecontext,jseTypeUndefined),jseRetTempVar);
     }
     else
     {
       jsebool deleteIt;
       jseVariable result;
       result = blobDescGet(desc,jsecontext,name,&deleteIt);
       assert( result != NULL );
       jseReturnVar(jsecontext,result, deleteIt ? jseRetTempVar : jseRetCopyToTempVar);
     }
  }
}

static jseLibFunc(BLOBObjectDelete)
{
  struct BLobDescriptor * desc;
  jseVariable thisvar;
  const jsechar _HUGE_ * name;
  jseVariable data;
  jseVariable jseName;

  thisvar = jseGetCurrentThisVariable( jsecontext );

  JSE_FUNC_VAR_NEED( jseName, jsecontext, 0, JSE_VN_STRING );
  name = jseGetString( jsecontext, jseName, NULL );
  
  data = jseGetMember( jsecontext, thisvar, blobInternalDataMember );
  assert( data != NULL );

  desc = (struct BLobDescriptor *) jseGetLong( jsecontext, data );

  /* Who's trying to delete these? */
  if ( strcmp_jsechar(name,blobInternalDataMember)!=0 &&
       strcmp_jsechar(name,GET_PROPERTY)!=0 &&
       strcmp_jsechar(name,PUT_PROPERTY)!=0 )
  {
     if ( !strcmp_jsechar(name,DELETE_PROPERTY) )  /* This means we are deleting the object */
     {
        blobDescDelete_all(desc,jsecontext);
        blobdescriptorDelete(desc);
     }
  }
  else
  {
     blobDescRemove( desc,jsecontext, name );
  }
}

static void NEAR_CALL
SetBlobType(jseContext jsecontext,const jsechar *Name,sint Value)
{
   jseVariable temp, var;

   var = blobCreateBLobType(jsecontext,Value);
   temp = jseMemberEx(jsecontext,NULL,Name,jseTypeNumber,jseCreateVar);
   jseAssign(jsecontext,temp,var);
   jseSetAttributes(jsecontext,temp,jseDontEnum | jseReadOnly | jseDontDelete);
   jseDestroyVariable(jsecontext,temp);
   jseDestroyVariable(jsecontext,var);
}

static void * JSE_CFUNC
BlobInitFunction(jseContext jsecontext,void * userdata)
{
   UNUSED_PARAMETER(userdata);

   /* initialize state of BigEndian conversion for this library level */
   assert( JSE_DEBUG_FEEDBACK(True) == BIG_ENDIAN  ||  JSE_DEBUG_FEEDBACK(False) == BIG_ENDIAN );
   blobInitializeBigEndianMode(jsecontext,BIG_ENDIAN);

   /* Note that these blob types are now variables,
    * not defines (So we can do a direct comparison)
    */

   SetBlobType(jsecontext,UNISTR("UWORD8"),blobUWORD8);
   SetBlobType(jsecontext,UNISTR("SWORD8"),blobSWORD8);
   SetBlobType(jsecontext,UNISTR("UWORD16"),blobUWORD16);
   SetBlobType(jsecontext,UNISTR("SWORD16"),blobSWORD16);
   SetBlobType(jsecontext,UNISTR("UWORD24"),blobUWORD24);
   SetBlobType(jsecontext,UNISTR("SWORD24"),blobSWORD24);
   SetBlobType(jsecontext,UNISTR("UWORD32"),blobUWORD32);
   SetBlobType(jsecontext,UNISTR("SWORD32"),blobSWORD32);
#  if defined(__JSE_MAC__)
   SetBlobType(jsecontext,UNISTR("STR255"),blobSTR255);
   SetBlobType(jsecontext,UNISTR("STR63"),blobSTR63);
   SetBlobType(jsecontext,UNISTR("STR31"),blobSTR31);
#  endif
#  if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
   SetBlobType(jsecontext,UNISTR("FLOAT32"),blobFLOAT32);
   SetBlobType(jsecontext,UNISTR("FLOAT64"),blobFLOAT64);
#    if !defined(_MSC_VER)
   SetBlobType(jsecontext,UNISTR("FLOAT80"),blobFLOAT80);
#    endif
#  endif

   return NULL;
}

static CONST_DATA(struct jseFunctionDescription) BLobObjectFunctionList[] = {
   JSE_LIBOBJECT( blobObjectName, BLOBObjectConstructor,  0, 0, jseDontEnum, jseFunc_Secure ),
   JSE_FUNC_END
};

void NEAR_CALL
InitializeLibrary_Blob(jseContext jsecontext)
{
   static CONST_DATA(jsechar) blobbyKludgeName[] = UNISTR("** Blobby Kludge variable **");
   if( jseGetMember(jsecontext,NULL,blobbyKludgeName) == NULL )
   {
      jseAddLibrary(jsecontext,NULL,BLobObjectFunctionList,NULL,BlobInitFunction,NULL);
      jseMember(jsecontext,NULL,blobbyKludgeName,jseTypeNull);
   }
}

#else
ALLOW_EMPTY_FILE
#endif
