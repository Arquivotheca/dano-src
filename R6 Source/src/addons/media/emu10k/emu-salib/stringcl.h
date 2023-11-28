//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: stringcl.h
//
// Author: Michael Preston
//
// Description: string package
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Jul 17, 1996  Made changes to linked lists.
// Michael Preston     Jun 19, 1996  Changed name from StringCl to Str.
// Michael Preston     Jun 13, 1996  Initial import to CVS.
// Michael Preston     Mar  1, 1996  Initial development.
//
//*****************************************************************************

#ifndef __STRINGCL_H
#define __STRINGCL_H

#include "datatype.h"
#include "win_mem.h"

#ifdef DEBUG
#  ifdef DEBUG_DISPLAY
#    include <iostream.h>
#  endif
#endif

#include <string.h>

#ifdef DEBUG_DISPLAY
#include <ctype.h>

class Str;
inline ostream& operator<<(ostream& stream, Str strcl);
#endif

class Str
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   Str() {str = NULL;}
   Str(CHAR *achString)
      {if (achString != NULL)
       {
//          str = new CHAR[strlen(achString)+1];
          str = (CHAR *)NewAlloc(strlen(achString)+1);
          strcpy(str, achString);
       }
       else
          str = NULL;}
//   ~Str() {if (str != NULL) delete [] str;}
   ~Str() {if (str != NULL) DeleteAlloc(str);}

   Str& operator=(const Str& strcl)
//      {if (str != NULL) delete [] str;
      {if (str != NULL) DeleteAlloc(str);
	  if (strcl.str != NULL)
          {
//             str = new CHAR[strlen(strcl.str)+1];
             str = (CHAR *)NewAlloc(strlen(strcl.str)+1);
             strcpy(str, strcl.str);
          }
          else
             str = NULL;
          return *this;}
   Str& operator=(CHAR* achString)
      {*this = Str(achString);
       return *this;}
   Str(const Str& strcl)
      {if (strcl.str != NULL)
       {
//          str = new CHAR[strlen(strcl.str)+1];
          str = (CHAR *)NewAlloc(strlen(strcl.str)+1);
          strcpy(str, strcl.str);
       }
       else
          str = NULL;
      }

   operator CHAR*() {return str;}
   operator const CHAR*() {return str;}

#  ifdef DEBUG_DISPLAY
   friend ostream& operator<<(ostream& stream, Str strcl);
#  endif

/*
   BOOL operator==(const Str& cmpstr)
      {if ((str != NULL) && (cmpstr.str != NULL))
	 return (strcmp(str, cmpstr.str) == 0);
       else
	 return (str == cmpstr.str);}
*/
   BOOL operator==(const Str& cmpstr) const
      {if ((str != NULL) && (cmpstr.str != NULL))
	 return (strcmp(str, cmpstr.str) == 0);
       else
	 return (str == cmpstr.str);}
/*
   BOOL operator==(const CHAR* cmpstr)
      {if ((str != NULL) && (cmpstr != NULL))
	 return (strcmp(str, cmpstr) == 0);
       else
	 return (str == cmpstr);}
*/
   BOOL operator==(const CHAR* cmpstr) const
      {if ((str != NULL) && (cmpstr != NULL))
	 return (strcmp(str, cmpstr) == 0);
       else
	 return (str == cmpstr);}
/*
   BOOL operator!=(const Str& cmpstr)
      {if ((str != NULL) && (cmpstr.str != NULL))
         return (strcmp(str, cmpstr.str) != 0);
       else
         return (str != cmpstr.str);}
*/
   BOOL operator!=(const Str& cmpstr) const
      {if ((str != NULL) && (cmpstr.str != NULL))
         return (strcmp(str, cmpstr.str) != 0);
       else
	 return (str != cmpstr.str);}

   static void dealloc(void *ptr) {delete (Str *)ptr;}
   static void *ccons(void *ptr)
      {return new Str(*(Str *)ptr);}

   private:

   CHAR *str;
};

#ifdef DEBUG_DISPLAY
inline ostream& operator<<(ostream& stream, Str strcl)
{
   stream << strcl.str;
   return stream;
}
#endif

#endif
