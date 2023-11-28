
//*****************************************************************************
//
//                          Copyright (c) 1996
//               E-mu Systems Proprietary All rights Reserved.
//
//*****************************************************************************



/*****************************************************************************
* Filename: RIFFPrsr.CPP
*
* Authors: Mike Guzewicz
*
* Description:  Read RIFF files
*
* 
* Visible Routines: 
*  
* History: 
*
* Version    Person        Date         Reason
* -------    ---------   -----------  ---------------------------------------
*   0.001      MG          3/13/96     Initial creation, port from RIFF.CPP
*
*********************************************************************
*/

#ifndef __RIFFPRSR_CPP
#define __RIFFPRSR_CPP

#include <string.h>

#ifdef DEBUG_DISPLAY
#include <iostream.h> 
#endif

/////////////////////////////
//        Includes         //
/////////////////////////////

#include "riffprsr.h"

CHAR RIFFParser::RIFF[] = {'R', 'I', 'F', 'F', '\0'};
CHAR RIFFParser::LIST[] = {'L', 'I', 'S', 'T', '\0'};

//////////////////////////
// The Constructor
//////////////////////////
RIFFParser::RIFFParser(void) : OmegaClass(RIFFParser::RIFF)
{

  io = NULL;
  current = head = NULL;
  dwRIFFSize = 0;
  atHeader = FALSE;
  ClearError();

  RIFFTokenStrings[tRIFF] = RIFF;
  RIFFTokenStrings[tLIST] = LIST;

}

//*********************************************************
//  The methods which open and read the RIFF files       
//  Note:  When we ReadChunkHeader, the file ptr is always     
//  left at the beginning of the chunk header.
//*********************************************************

EMUSTAT RIFFParser::OpenRIFF(void *(*alloc)(), void *openPtr)
{

  ClearError();

  if (openPtr == NULL)
  {
    return HLD_PARAM_OUT_OF_RANGE;
  }

  io = (EmuAbstractFileIO *)alloc();
  if (io == NULL)
  {
    SetError(oscErrorStatus, SF_MEMORYERROR);
    return GetError();
  }

  io->Open(openPtr, enFReadBinary);
  if (io->IsBad() == TRUE)
  {
    PropogateError(io);
    CloseRIFF();
    return GetError();
  }

  InitRIFF();
  if (IsBad())
      CloseRIFF();

  return GetError();
}

void RIFFParser::InitRIFF(void)
{

  ClearError();

  if ((current = ReadChunkHeader()) == NULL)
  {
    return;
  }

  else if (current->token != MAKE_ID(RIFF))
  {
     SetError(oscErrorStatus, RIFF_IDERROR);
     return;
  }

  dwRIFFSize = GetChunkSize();

  RIFFSeek(0, enFSeekEnd);
  DWORD realEnd = RIFFTell();
  if (realEnd < (dwRIFFSize + 8))
    dwRIFFSize = RIFFTell() - 8;

  RIFFSeek(current->position, enFSeekSet);

  head = current;
  return;

}

void RIFFParser::CloseRIFF(void)
{
  if (io != NULL)
  {
    while (Ascend(TRUE) == SUCCESS) ;
    RIFFClose();
  }
}


//*************************************************
// Reads the chunk header and retains there values
//*************************************************
chunkList *  RIFFParser::ReadChunkHeader(void)
{
  SWAP_DWORD_BYTE_INCOH_DECLARATIONS;
  chunkList *list;

  ClearError();

//  if ((list = new chunkList) == NULL)
  if ((list = (chunkList *)NewAlloc(sizeof(chunkList))) == NULL)
  {
    SetError(oscErrorStatus, SF_MEMORYERROR);
    return NULL;
  }

  list->parent = NULL;

  RIFFRead(&list->token, sizeof(RIFFToken), 1);
  RIFFRead(&list->size, sizeof(DWORD), 1);
  if (IsList(list->token) == TRUE)
     RIFFRead(&list->listToken, sizeof(RIFFToken), 1);
  else
     list->listToken = 0;

  SWAP_DWORD_BYTE_INCOH_ONLY(list->size);

  // The RIFF spec says that odd sized chunks are really even sized chunks
  // just one bigger than the chunk size. Let's hope so... ;-)
  
  // MG comment this out
  //if ((list->size % 2) == 1)
  //  list->size++;

  list->position = RIFFTell();
  atHeader = FALSE;

  return list;
}


//***********************************************************
// Descend into a chunk, i.e., a set of one or more subchunks
// contained in a parent chunk
//***********************************************************
EMUSTAT  RIFFParser::Descend(void)
{
  ClearError();

  chunkList *list;
  if (IsList(current->token))
  {
    if (current->position >= (LONG)(dwRIFFSize + GetChunkHeaderSize(current)))
    {
      SetError(oscErrorStatus, RIFF_READFILEERROR);
      return GetError();
    }
    RIFFSeek(current->position, enFSeekSet);
    list = ReadChunkHeader();
    list->parent = current;
    current = list;
    return SUCCESS;
  }
  //// There are no more RIFF or LIST chunks within this chunk ////
  SetError(oscErrorStatus, RIFF_READFILEERROR);
  return GetError();
}


//************************************************************
// Ascend to the chunk from whence we descended
//************************************************************
EMUSTAT  RIFFParser::Ascend(BOOL backward)
{
  ClearError();

  if ((current == NULL) || (current->parent == NULL))
  {
    SetError(oscErrorStatus, RIFF_READFILEERROR);
    return GetError();
  }

  LONG seekTo = current->parent->position;

  chunkList *list = current->parent;
  chunkList *save = current;

  RIFFSeek(seekTo, enFSeekSet);
  current = list;

  if (backward == FALSE)
  {
    if (Traverse() != SUCCESS)
    {
      current = save;
      RIFFSeek(current->position, enFSeekSet);
      SetError(oscErrorStatus, RIFF_READFILEERROR);
      return GetError();
    }
  }

  delete save;
  return SUCCESS;

}

EMUSTAT RIFFParser::TraverseInOrder(void)
{
  LONG position = current->position;

  if (Descend() != SUCCESS)
  {
    if (Traverse() != SUCCESS)
    {
      while (Ascend() != SUCCESS)
      {
	if ((current == head) || (current->parent == head))
	{
	  FindChunkAtPosition(position);
	  SetError(oscErrorStatus, RIFF_FINDERROR);
	  return GetError();
	}
	Ascend(TRUE);
      }
    }
  }
  return SUCCESS;
}

//************************************************************
// Move to the next chunk at this level
//************************************************************
EMUSTAT  RIFFParser::Traverse(BOOL)
{

  ClearError();

  LONG seekTo = GetNextChunkPosition();

  if (seekTo >= (LONG)dwRIFFSize)
  {
    SetError(oscErrorStatus, RIFF_READFILEERROR);
    return GetError();
  }

  else if ((current->parent != NULL) &&
	   (seekTo >= (LONG)(ERIFFGETSIZE(current->parent->size) +
		       current->parent->position - sizeof(DWORD))))
  {
    SetError(oscErrorStatus, RIFF_READFILEERROR);
    return GetError();
  }

  RIFFSeek(seekTo, enFSeekSet);

  chunkList *newList = ReadChunkHeader();
  newList->parent = current->parent;

  delete current;
  current = newList;
  return SUCCESS;
}

//**************************************
// Find a subchunk with the pID identity
//**************************************
EMUSTAT RIFFParser::FindChunk(RIFFToken dwID, BOOL withinChunk, BOOL descend)
{
  ClearError();
  lastSearchDescend = (withinChunk == TRUE) ? descend : TRUE;
  lastSearchID = dwID;
  LONG savePosition = current->position;

  if (withinChunk == TRUE)
  {
    if (IsList(current) == FALSE)
      Ascend(TRUE);
  }

  else
  {
    while (Ascend(TRUE) == SUCCESS) ;
  }

  if ((dwID == current->token) || (dwID == current->listToken))
    return SUCCESS;

  maxSearchSize = current->position + ERIFFGETSIZE(current->size);

  Descend();

  if ((dwID == current->token) || (dwID == current->listToken))
    return SUCCESS;

  EMUSTAT stat;

  if ((stat = FindNextChunk()) != SUCCESS)
  {
    if (FindChunkAtPosition(savePosition) == SUCCESS)
      SetError(oscErrorStatus, (BYTE)stat);
  }

  return stat;
}

EMUSTAT RIFFParser::FindChunkAtPosition(LONG position)
{

  while (Ascend(TRUE) == SUCCESS) ;

  while (current->position != position)
  {
    if (TraverseInOrder() != SUCCESS)
      return RIFF_FINDERROR;
  }

  return SUCCESS;
}

EMUSTAT RIFFParser::FindNextChunk(void)
{
  ClearError();

  LONG savePos = current->position;

  while (TRUE)
  {
    // Descending search
    if (lastSearchDescend == TRUE)
    {
      if (TraverseInOrder() != SUCCESS) break;
      if (GetNextChunkPosition() >= (LONG)maxSearchSize) break;
    }

    // Search limited by current subchunk, no descending allowed
    else if (Traverse() != SUCCESS) break;

    if ((lastSearchID == current->token) ||
	(lastSearchID == current->listToken))
      return SUCCESS;

  }

  FindChunkAtPosition(savePos);
  SetError(oscErrorStatus, RIFF_FINDERROR);
  return GetError();
}

EMUSTAT RIFFParser::StartDataStream(void)
{
  if (current == NULL) return RIFF_OPENFILEERROR;
  if (IsList(current)) return RIFF_READFILEERROR;
  RIFFSeek(current->position, enFSeekSet);
  totalChunkSize = GetNextChunkPosition() - current->position;
  totalCollected = 0L;
  return SUCCESS;
}

EMUSTAT RIFFParser::StopDataStream(void)
{
  if (current == NULL) return RIFF_OPENFILEERROR;
  if (totalChunkSize == 0)  return RIFF_READFILEERROR;
  RIFFSeek(current->position, enFSeekSet);
  totalCollected = totalChunkSize = 0L;
  return SUCCESS;
}

DWORD RIFFParser::ReadDataStream(void *bucket, DWORD sizeRead)
{
  ClearError();
  if (totalChunkSize == 0L) return 0;
  BOOL lastBucket = FALSE;

  if ((totalCollected + sizeRead) >= totalChunkSize)
  {
    lastBucket = TRUE;
    sizeRead = totalChunkSize - totalCollected;
  }
  RIFFRead(bucket, sizeRead, 1);
  totalCollected += sizeRead;
  if (lastBucket == TRUE)
    StopDataStream();
  return sizeRead;
}

void  RIFFParser::SeekDataStream(LONG seekBytes)
{
  ClearError();
  if (totalChunkSize == 0L) return; 
  //BOOL lastBucket = FALSE;

  if (((totalCollected + seekBytes) >= totalChunkSize) ||
      (((LONG)totalCollected + seekBytes) <  0))
  {
    StopDataStream();
    StartDataStream();
    SetError(oscErrorStatus, RIFF_FINDERROR);
    return;
  }
  RIFFSeek(seekBytes, enFSeekCur);
  totalCollected += seekBytes;
}

void  RIFFParser::GetCurPosition(RIFFPosition& pos)
{
   chunkList *tmp1, *tmp2;

   if (pos.current != NULL)
      for (tmp1 = pos.current; pos.current != NULL; tmp1 = pos.current)
      {
         pos.current = pos.current->parent;
         delete tmp1;
      }

   for (pos.current = NULL, tmp1 = current; tmp1 != NULL; tmp1 = tmp1->parent)
   {
      tmp2 = pos.current;
//      pos.current = new chunkList;
      pos.current = (chunkList *)NewAlloc(sizeof(chunkList));
      pos.current->token = tmp1->token;
      pos.current->size = tmp1->size;
      pos.current->listToken = tmp1->listToken;
      pos.current->position = tmp1->position;
      pos.current->parent = tmp2;
   }
   pos.lastSearchDescend = lastSearchDescend;
   pos.atHeader = atHeader;
   pos.lastSearchID = lastSearchID;
   pos.maxSearchSize = maxSearchSize;
   pos.totalChunkSize = totalChunkSize;
}

void  RIFFParser::SetCurPosition(RIFFPosition& pos)
{
   chunkList *tmp1, *tmp2;

   if (current != NULL)
      for (tmp1 = current; current != NULL; tmp1 = current)
      {
         current = current->parent;
         delete tmp1;
      }

   for (current = NULL, tmp1 = pos.current; tmp1 != NULL;
        tmp1 = tmp1->parent)
   {
      tmp2 = current;
//      current = new chunkList;
      current = (chunkList *)NewAlloc(sizeof(chunkList));
      current->token = tmp1->token;
      current->size = tmp1->size;
      current->listToken = tmp1->listToken;
      current->position = tmp1->position;
      current->parent = tmp2;
   }
   lastSearchDescend = pos.lastSearchDescend;
   atHeader = pos.atHeader;
   lastSearchID = pos.lastSearchID;
   maxSearchSize = pos.maxSearchSize;
   totalChunkSize = pos.totalChunkSize;

   RIFFSeek(current->position, enFSeekSet);
}

void  RIFFParser::SeekHeader(void)
{
  if (atHeader == TRUE) return;
  LONG back = -(LONG)(GetChunkHeaderSize(current));
  RIFFSeek(back, enFSeekCur);
  atHeader = TRUE;
}

void RIFFParser::SeekData(void)
{
  if (atHeader == FALSE) return;
  LONG forward = (LONG)GetChunkHeaderSize(current);
  RIFFSeek(forward, enFSeekCur);
  atHeader = FALSE;
}

///////////////////////////////////////////////////////////
//  Functions to help with manipulating the file         //
//  and to help read the data image whether it exists on //
//  disk or within memory                                //
///////////////////////////////////////////////////////////

void RIFFParser::RIFFClose()
{
  chunkList *tmp;

  if (io == NULL) return;
  for (tmp = current; current != NULL; tmp = current)
  {
    current = current->parent;
    delete tmp;
  }

  io->Close();
  delete io;
  io = NULL;
}

LONG  RIFFParser::RIFFRead(void * vStream, LONG size, LONG num)
{
  ClearError();

  if (io == NULL)
  {
    SetError(oscErrorStatus, HLD_PARAM_OUT_OF_RANGE);
    return 0;
  }
  LONG retVal = io->Read(vStream, size, num);
  PropogateError(io);
  return retVal;
}

LONG
RIFFParser::RIFFSeek(LONG offset, enWhence whence)
{
  ClearError();

  if (io == NULL) 
  {
    SetError(oscErrorStatus, HLD_PARAM_OUT_OF_RANGE);
    return 0;
  }
  LONG retVal = io->Seek(offset, whence);
  PropogateError(io);
  return retVal;
}

LONG  RIFFParser::RIFFTell(BOOL abs)
{
  ClearError();

  if (io == NULL)
  {
    SetError(oscErrorStatus, HLD_PARAM_OUT_OF_RANGE);
    return 0;
  }
  LONG retVal = io->Tell(abs);
  PropogateError(io);
  return retVal;
}

#endif // __RIFF_CPP

