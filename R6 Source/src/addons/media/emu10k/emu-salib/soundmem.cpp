//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: soundmem.cpp
//
// Author: Michael Preston
//
// Description: Sound memory code.
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Oct  4, 1996  Added SoundDataLoad class for reading
//                                   sample data.
// Michael Preston     Jul 24, 1996  Added fileNameList.
// Michael Preston     Jul 17, 1996  Removed unused code.
// Michael Preston     Jun 24, 1996  Added support for SoundMemory structure.
// Michael Preston     Jun 13, 1996  Initial import to CVS.
// Michael Preston     May  1, 1996  Initial development.
//
//*****************************************************************************

// Include files

#include <string.h>
#include "soundmem.h"

int sdrefmatch(void *s, void *sm)
{
   return ((((SoundData *)s)->wAllocatedRefCount ? 0 : 1) == *(BOOL *)sm);
}

int sdfnmatch(void *s, void *sm)
{
   return (*(Str *)sm == ((SoundData *)s)->strFileName);
}

int sdmatch(void *s, void *sm)
{
   return ((((SoundData *)s)->strFileName ==
				    ((SoundDataMatch *)sm)->strFileName) &&
	   (((SoundData *)s)->strSoundName ==
				     (*((SoundDataMatch *)sm)->strSoundName)));
}

SoundDataLoad::SoundDataLoad(void *(*alloc)(), void *token)
{
   io = (EmuAbstractFileIO *)alloc();
   io->Open(token, enFReadBinary);
}

SoundDataLoad::~SoundDataLoad()
{
   io->Close();
   delete io;
}

void SoundDataLoad::StartSoundData(SoundData *sd)
{
   b16Bit = (sd->bySoundDataType & sdt16BitMask) ? TRUE : FALSE;
   byNumChannels = sd->bySoundDataType & sdtChannelMask;
   dwCurPos = sd->dwStartLocInBytes;
   dwEndPos = dwCurPos+sd->dwSizeInBytes-1;
   io->Seek(dwCurPos, enFSeekSet);
}

DWORD SoundDataLoad::GetSoundData(BYTE *buffer, DWORD dwNumBytes)
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   DWORD ret, cnt, rcnt, dwTmpNumBytes;
#ifdef __BYTE_INCOHERENT
   WORD *tmp;
#endif
   BYTE *tmpbuf;

   if ((dwCurPos == 0) && (dwEndPos == 0))
     return 0;

#ifdef __BOUND_16BIT
   if (dwNumBytes > 65536)
   {
// Set error
      return 0;
   }
#endif

   if (b16Bit && (dwNumBytes % 2 != 0))
   {
// Set error
      return 0;
   }

   ret = MIN(dwEndPos - dwCurPos + 1, dwNumBytes);

   if (byNumChannels == 0)
      io->Read(buffer, 1, ret);
   else
   {
      dwTmpNumBytes = ret*(byNumChannels+1);
      tmpbuf = (BYTE *)NewAlloc(dwTmpNumBytes);
      io->Read(tmpbuf, 1, dwTmpNumBytes);
      for (rcnt = cnt = 0; cnt < dwTmpNumBytes;
           cnt += (b16Bit ? byNumChannels*2 : byNumChannels))
      {
         buffer[rcnt++] = tmpbuf[cnt++];
         if (b16Bit)
            buffer[rcnt++] = tmpbuf[cnt++];
      }
      DeleteAlloc(tmpbuf);
   }
   dwCurPos += ret;

#ifdef __BYTE_INCOHERENT
   if (b16Bit)
      for (tmp = (WORD *)buffer, cnt = 0; cnt < ret; tmp += 1, cnt += 2)
         SWAP_WORD_BYTE_INCOH_ONLY(*tmp);
#endif
   if (ret != dwNumBytes)
      dwCurPos = dwEndPos = 0;

   return ret;
}

void SoundMemory::UpdateLists()
{
   for (ActionList.First(); !ActionList.EndOfList();)
   {
      if (ActionList.GetCurItem()->wAllocatedRefCount == 0)
         ActionList.DeleteCurItem();
      else
         SoundList.Insert(ActionList.RemoveCurItem());
   }
}
