//*****************************************************************************
//
//                              Copyright (c) 1996-97
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: QFData.h
//
// Author: Michael Preston
//
// Description: Data associated with a single note-on event
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Mike Guz            Feb  3, 1997  Seperated from quickfnt.h and qfnav.h
//                                   for simplicity
//
// Note on compile flags:
//   __NO_MOD_CTRL: Removes elements pertinent to SoundFont 2.1 implementation
//   __USE_ARRAYCACHE: Adds elements pertinant to value caching mechanism
// Someday we'll do this in a way which does not involve compile flags.
//
//*****************************************************************************

#ifndef __QFDATA_H
#define __QFDATA_H

// Include files

#include "datatype.h" 
#include "win_mem.h"
#include "omega.h"
#include "llist.h"

//*****************************************************************************
// The Quick Font Data is the data associated with a single sample used in
// a single note on event. Quick Font data consists of three parts:
//
// The Articulation Data Table: Contains the SoundFont "Generators"
//   NOTE: With array caching, the articulation data table is an array cached
//   data structure. Without, the articulation data table is simply an
//   array of LONGs. The __USE_ARRAYCACHE creates the former.
//
// The Modulation Data Table: Contains the SoundFont "Modulators"
//   NOTE: This table is omitted if SoundFont 2.1 is not supported.
//   The __NO_MOD_CTRL disables this table.
//
// The Sound Memory Data: Contains the global pointers to data in sound memory
//   NOTE: If Sound ROM is used, this table will be NULL and the 
//   pointers will be in the articulation data table.
//   There are offset pointer data in the Articulation data table to
//   accommodate offsets from the global pointers. This list is shared
//   with the Sample Manager.
//
// All data in the QuickFont data structure should be treated as CONSTANT!
// and NEVER MANIPULATED by the runtime engine!
//*****************************************************************************

#ifndef __USE_ARRAYCACHE
   typedef LONG * QFArtTable;
#else
#  include "arrcache.h"
   typedef ArrayCache QFArtTable;
#endif

#include "soundmem.h"
#ifndef __NO_MOD_CTRL
#  include "sfmodtbl.h"
#endif

//*****************************************************************************
//
// Class: QFArtData
//
// Description:
//    This class defines the structure for the articulation data for a single
// key/velocity range.
//
//*****************************************************************************
class QFArtData
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   //
   // Constructor/Destructor for using array caching mechanism in
   // the articulation data table
   //
#  ifndef __USE_ARRAYCACHE

   QFArtData() 
   {
     soundptr = NULL; 
     arttable = NULL;
#    ifndef __NO_MOD_CTRL
     modtable = NULL;
#    endif
   }

   ~QFArtData() 
   {
     if (arttable != NULL) 
       DeleteAlloc(arttable);
#    ifndef __NO_MOD_CTRL
     if (modtable->DeleteRef() == 0)
        delete modtable;
#    endif
   }

   // Note this is only used by reader using array caching 
   QFArtData(QFArtData& qfad)
   {
     soundptr = qfad.soundptr;
#    ifndef __NO_MOD_CTRL
     modtable = qfad.modtable;
#    endif
   }


   //
   // Constructor/Destructor for simple array mechanism for the
   // articulation data table
   //
#  else

   QFArtData() 
   {
     soundptr = NULL;
#    ifndef __NO_MOD_CTRL
     modtable = NULL;
#    endif
   }

   ~QFArtData()
   {
#    ifndef __NO_MOD_CTRL
     if (modtable->DeleteRef() == 0)
        delete modtable;
#    endif
   }

#  endif

   //
   // The three fundamental parts of the QuickFont data
   //
   QFArtTable     arttable;
   SoundData      *soundptr;
#  ifndef __NO_MOD_CTRL
   ModulatorTable *modtable;
#  endif

   //
   // Function calls to handle auto-create and destruct by client
   //
   static void dealloc(void *ptr) {delete (QFArtData *)ptr;}
   static void *ccons(void *ptr)
      {return new QFArtData(*(QFArtData *)ptr);}
};

//*****************************************************************************
//
// Class: QFArtDataList
//
// Description:
//    This class defines a linked list of QFArtData structures. The 
// sum of all QFArtData structures in a single list makes up a single
// note-on event.
//
// These lists are created at Note-on time by the QuickFont navigator
// and processed at note-on time by the Synthesizer Engine.
//
//*****************************************************************************
class QFArtDataList : public UnorderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFArtDataList() : UnorderedLinkedList(QFArtData::dealloc, QFArtData::ccons,
                                         FALSE) {}

   void Insert(QFArtData* newitm, InsertLoc loc=InsertEnd)
      {UnorderedLinkedList::Insert(newitm, loc);}
   void Insert(QFArtData& newitm, InsertLoc loc=InsertEnd)
      {QFArtData *tmpitm;
       tmpitm = new QFArtData(newitm);
       UnorderedLinkedList::Insert(tmpitm, loc);}
   QFArtData *GetCurItem()
      {return (QFArtData *)UnorderedLinkedList::GetCurItem();}
};

#endif
