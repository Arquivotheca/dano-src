
//*****************************************************************************
//
//                          Copyright (c) 1996
//               E-mu Systems Proprietary All rights Reserved.
//                             
//*****************************************************************************

#ifndef __RIFFPRSR_HPP
#define __RIFFPRSR_HPP


#include "datatype.h"
#include "win_mem.h"
#include "emufilio.h"
#include "omega.h"

#include <stdio.h>
#include <string.h>


//*****************************************************************************
// riffprsr.h
//                             
// Filename: RIFFPRSR.H
//
// Authors: Mike Guzewicz
//
// Description: Read RIFF files
//
// History:
//
// Version    Person        Date         Reason
// -------    ---------   -----------  -------------------------------
//  0.000       MG        Mar 12, 96   Initial creation
//                                     Derived from 'riff.cpp'
//
//********************************************************************
 


/////////////////////////////////////////////////
//      Structures and enumeration lists       //
/////////////////////////////////////////////////

typedef DWORD RIFFToken;

#define MAKE_ID(a) (*(DWORD *)a)

// This returns the number of bytes of data actually contained
// in the RIFF chunk. For RIFF files this is always even due 
// to padding. The size stored in the header of a chunk does not
// include the padding.
#define ERIFFGETSIZE(a) (((a)%2)?((a)+1):(a))

// Forward declaration for class definition
struct chunkList
{
  RIFFToken token;
  DWORD     size;
  RIFFToken listToken;
  LONG      position;
  chunkList *parent;

  // These are necessary if we want a true RAM image of the RIFF chunk data
# if 0
  chunkList *child, *next;
# endif

};

class RIFFPosition
{
   public:
   
   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   RIFFPosition() {current = NULL;}
   ~RIFFPosition()
      {chunkList *tmp;
       if (current != NULL)
         for (tmp = current; current != NULL; tmp = current)
         {
            current = current->parent;
            delete tmp;
         }
      }
   
   chunkList *current;
   BOOL      lastSearchDescend;
   BOOL      atHeader;
   RIFFToken lastSearchID;
   DWORD     maxSearchSize;
   DWORD     totalChunkSize;
};

enum enToken
{
  tRIFF = 0,
  tLIST,

  // Add new tokens above this line!
  tLast
};

#define RIFF_TOKEN_COUNT  tLast
#define RIFF_TOKEN_LENGTH 5

// For code legibility purposes

// Use these in the BOOL backward flag for Ascend()
#define ASCEND_FORWARD  FALSE
#define ASCEND_BACKWARD TRUE

// Use these in the BOOL withinChunk flag in FindChunk();
#define SEARCH_WITHIN_CHUNK    TRUE
#define SEARCH_GLOBAL          FALSE

// Use these in the BOOL descend flag in FindChunk()
#define SEARCH_ALL_SUBCHUNKS   TRUE
#define SEARCH_ONLY_CHUNKS     FALSE

/////////////////////////////
//         Classes         //
/////////////////////////////

class RIFFParser : public OmegaClass
{
  public:
    RIFFParser(void);
    ~RIFFParser(void) {CloseRIFF();}

    void *operator new(size_t size) {return NewAlloc(size);}
    void operator delete(void *ptr) {DeleteAlloc(ptr);}

    void  Reset(void) {while (Ascend(TRUE) == SUCCESS);}

    // Open a RIFF file
    // First parameter should be a function defined in any of the 
    // derived classes of the 'EmuFileIO' base class which dynamically
    // allocates an instance of that class IE EmuANSIFileIO::Alloc. 
    // Second parameter should be whatever 'token' that derived class 
    // expects to uniquely identify a single file or single encapsulated
    // data chunk, IE a full path to a file name
    EMUSTAT  OpenRIFF(void *(*FunctionToAllocateIO)(), void *openToken);

    // Close the currently opened RIFF file
    // Return the File IO.
    // NOTE: code MUST know what kind of FileIO was passed in here!
    void CloseRIFF(void);

    // Navigation routines

    // All navigation routines leave you at the first byte AFTER the RIFF
    // header, and receive information for the given chunk. These routines
    // allow you to access that information.
    DWORD      GetChunkSize(void)
       {return ((current != NULL) ? current->size : 0);}
    DWORD      GetListSize(void);
    RIFFToken  GetChunkToken(void)
       {return ((current != NULL) ? current->token : 0);}
    RIFFToken  GetChunkListToken(void)
       {return ((current != NULL) ? current->listToken : 0);}

    // Go from a chunk to the next chunk at the same level as the current
    // chunk. Only go as far as the parent chunk limits set. Return of
    // failure means there are no chunks left to traverse and you are
    // left where you started.
    EMUSTAT  Traverse(BOOL autoAscend = FALSE);

    // Go from a LIST chunk into a subchunk. If you are not in a LIST
    // chunk, return is failure and you are left where you started.
    EMUSTAT  Descend(void);

    // Go from a subchunk to a chunk one level above. 
    // 'back==TRUE'  says go to the parent chunk of the current subchunk
    // 'back==FALSE' says go to the NEXT parent chunk, IE the chunk which 
    //               you would reach if you called Ascend(FALSE) and then 
    //               Traverse()
    // Return of failure says you could not get there, and you are left
    // where you started.
    EMUSTAT  Ascend(BOOL back=FALSE);

    // Go from a chunk to the chunk immediately following it, regardless
    // of how that chunk fits in the heirarchy. 
    EMUSTAT  TraverseInOrder(void);

    // Find a chunk with a given token or a list token.
    // 'withinChunk==TRUE'  says only search within the current subchunk.
    // 'withinChunk==FALSE' says search the whole file
    // 'descend==TRUE' takes effect only if withinChunk==TRUE, and it says
    //                 to descend into subchunks for your search.
    // 'descend==FALSE' says only traverse in the search.
    EMUSTAT  FindChunk(RIFFToken token, BOOL withinChunk = FALSE,
                                  BOOL descend = FALSE);

    // Find the next chunk with the same characteristics as the previous
    // call to FindChunk()
    EMUSTAT  FindNextChunk(void);

    // Move current position to the first byte in the RIFF header.
    void     SeekHeader(void);

    // Move current position to the first byte AFTER the RIFF header.
    void     SeekData(void);

    // Read Data Routines

    // Begin a stream to read data from a given chunk. Returns failure if you 
    // are in a list chunk. Client must allocate space for the 'bucket' and
    // pass a pointer to that bucket along with its size.
    EMUSTAT  StartDataStream(void);

    // Read '*bucketSize' bytes of data into 'bucket' and pass 'bucket' back.
    // If return is 0, you have already read all bytes in a given chunk.
    // In this case, StopDataStream() was called automatically.
    DWORD  ReadDataStream(void *bucket, DWORD numRead);

    // Move 'numBytes' forward or backward into a chunk stream.
    // Does NOT call StopDataStream() automatically.
    // If you exceed the size of the chunk, GetError() will tell you. 
    // And you will be on the first byte of the chunk.
    void   SeekDataStream(LONG seekBytes);

    // Return where you are relative to the chunk data begin position
    // Value of -1 indicates you did not open a stream
    LONG   TellDataStream(void)
       {ClearError();
       if (totalChunkSize == 0L) return -1L;
       return (totalCollected);}

    // Stop reading the data stream. NOTE: Call this BEFORE calling another
    // Navigation routine! (Unless ReadDataStream returned NULL, in which
    // case you do not need to.
    EMUSTAT  StopDataStream(void);
    
    void GetCurPosition(RIFFPosition& pos);
    void SetCurPosition(RIFFPosition& pos);

    EmuAbstractFileIO * GetFileIO(void) {return io;}

    DWORD    GetRIFFSize(void)
       {return dwRIFFSize;}

    EMUSTAT  RIFFOpen(void *);
    void     RIFFClose(void);
    LONG     RIFFRead(void * buf, LONG size, LONG num);
    LONG     RIFFSeek(LONG lOffset, enWhence whence);
    LONG     RIFFTell(BOOL absolute=FALSE); 
	
    RIFFToken GetRIFFToken(enToken wCurToken);

  private:

    static CHAR RIFF[5];
    static CHAR LIST[5];

    chunkList *  ReadChunkHeader(void);
    void     InitRIFF(void);
    DWORD    GetChunkHeaderSize(chunkList *list)
       {return (IsList(list) ? 12 : 8);}
    BOOL     IsList(chunkList *list)
       {return ((list != NULL) ? IsList(list->token) : FALSE);}
    BOOL     IsList(RIFFToken token)
       {return ((token == MAKE_ID(RIFF)) || (token == MAKE_ID(LIST)));}
    EMUSTAT  FindChunkAtPosition(LONG pos);
    LONG     GetNextChunkPosition(void)
       {LONG next = current->position + ERIFFGETSIZE(current->size);
        return (IsList(current) ? next-4 : next);}

    EmuAbstractFileIO *io;

    CHAR    *RIFFTokenStrings[RIFF_TOKEN_COUNT];
    BOOL    atHeader;

    chunkList *current, *head;
    BOOL       lastSearchDescend;
    RIFFToken  lastSearchID;
    DWORD      maxSearchSize;

    DWORD      totalChunkSize, totalCollected;
    
    DWORD dwRIFFSize;      // The size of the file - the header ID and Size
};



#endif // __RIFF_HPP
////////////////////////// End of RIFF.HPP //////////////////////////
