//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: quickfnt.cpp
//
// Author: Michael Preston
//
// Description: QuickFont data structures and code.
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Sep  5, 1996  Added MoveMIDIBank function.
// Michael Preston     Aug  5, 1996  Replaced comp() function with COMP() macro
// Michael Preston     Jul 17, 1996  Added DEBUG compile flag for iostream.
//                                   Removed unused code.
// Michael Preston     Jul 10, 1996  Added GetBankTitle().
// Michael Preston     Jun 13, 1996  Initial import to CVS.
// Michael Preston     Mar  1, 1996  Initial development.
//
//*****************************************************************************

#include <string.h>
#include "quickfnt.h"

int kvrngorder(void *rng1, void *rng2)
{
   int tmp = COMP(((QFKeyVelRange *)rng1)->byLow, ((QFKeyVelRange *)rng2)->byLow);
   if (tmp != 0)
      return tmp;

   return COMP(((QFKeyVelRange *)rng1)->byHigh, ((QFKeyVelRange *)rng2)->byHigh);
}

int rngmatch(void *rng, void *rngm)
{
   return ((((QFRangeMatch *)rngm)->byKey >= ((QFRange *)rng)->byKeyLow) &&
           (((QFRangeMatch *)rngm)->byVel >= ((QFRange *)rng)->byVelLow) &&
           (((QFRangeMatch *)rngm)->byKey <= ((QFRange *)rng)->byKeyHigh) &&
           (((QFRangeMatch *)rngm)->byVel <= ((QFRange *)rng)->byVelHigh));
}

int rngorder(void *rng1, void *rng2)
{
   int tmp = COMP(((QFRange *)rng1)->byKeyLow, ((QFRange *)rng2)->byKeyLow);
   if (tmp != 0)
      return tmp;

   tmp = COMP(((QFRange *)rng1)->byVelLow, ((QFRange *)rng2)->byVelLow);
   if (tmp != 0)
      return tmp;

   return COMP(((QFRange *)rng1)->dwStereoLink, ((QFRange *)rng2)->dwStereoLink);
}

int bnkmatch(void *bnk, void *bnkm)
{
   return (((QFBank *)bnk)->wBankNum == *(WORD *)bnkm);
}

int bnkorder(void *bnk1, void *bnk2)
{
   BOOL bBank1Var = ((QFBank *)bnk1)->wBankNum != ((QFBank *)bnk1)->wParentBankNum;
   BOOL bBank2Var = ((QFBank *)bnk2)->wBankNum != ((QFBank *)bnk2)->wParentBankNum;
   int tmp = COMP(bBank1Var, bBank2Var);
   if (tmp != 0)
      return tmp;

   return (COMP(((QFBank *)bnk1)->wBankNum, ((QFBank *)bnk2)->wBankNum));
}

int presetmatch(void *p, void *pm)
{
   return (((QFPreset *)p)->wPresetNum == *(WORD *)pm);
}

int presetorder(void *preset1, void *preset2)
{
   int tmp = COMP(((QFPreset *)preset1)->wPresetNum,
                  ((QFPreset *)preset2)->wPresetNum);
   if (tmp != 0)
      return tmp;

   return (COMP(((QFPreset *)preset2)->bSelfLoaded,
                ((QFPreset *)preset1)->bSelfLoaded));
}

CHAR *QFBank::GetBankTitle()
{
   static CHAR title[]="Multiple Banks";
   CHAR *tmpstr;

   preset.First();
   tmpstr = preset.GetCurItem()->strBankName;
   preset.Next();
   while (!preset.EndOfList())
   {
      if (strcmp(preset.GetCurItem()->strBankName, tmpstr) != 0)
         return title;
      tmpstr = preset.GetCurItem()->strBankName;
      preset.Next();
   }
   return tmpstr;
}

CHAR *QFBank::GetFileName()
{
   static CHAR title[]="Multiple Files";
   CHAR *tmpstr;

   preset.First();
   tmpstr = preset.GetCurItem()->strFileName;
   preset.Next();
   while (!preset.EndOfList())
   {
      if (strcmp(preset.GetCurItem()->strFileName, tmpstr) != 0)
         return title;
      tmpstr = preset.GetCurItem()->strFileName;
      preset.Next();
   }
   return tmpstr;
}

void QuickFont::MoveMIDIBank(WORD wOldBankNdx, WORD wNewBankNdx)
{

   QFBank *tmp;


   ClearError();
   list.Find(wNewBankNdx);
   if (!list.EndOfList())
   {
      SetError(SF_INVALIDBANK);
      return;
   }
   list.Find(wOldBankNdx);
   if (list.EndOfList())
   {
      SetError(SF_INVALIDBANK);
      return;
   }
//   list.GetCurItem()->wBankNum = wNewBankNdx;

   tmp = list.RemoveCurItem();

   tmp->wBankNum = wNewBankNdx;

   list.Insert(tmp);
}

void QuickFont::SetInstanceState(WORD wInstanceNum, qfPresetState state)
{
   QFBank *tmpBank;
   QFPreset *tmpPreset;

   for (list.First(); !list.EndOfList();)
   {
      tmpBank = list.GetCurItem();
      for (tmpBank->preset.First(); !tmpBank->preset.EndOfList();)
      {
         tmpPreset = tmpBank->preset.GetCurItem();
         if (tmpPreset->wInstanceNum == wInstanceNum)
         {
            switch(state) {
            case qfpsUnload:
               UpdateRefs(*tmpPreset);
               tmpBank->preset.DeleteCurItem();
               break;
            case qfpsNotAvailable:
            case qfpsAvailable:
               tmpPreset->state = state;
               tmpBank->preset.Next();
            };
         }
         else
            tmpBank->preset.Next();
      }
      if (tmpBank->preset.IsEmpty())
         list.DeleteCurItem();
      else
         list.Next();
   }
   if (state == qfpsUnload)
      UpdateMemLists(*sndmem);
}

void QuickFont::SetBankState(WORD wBankNum, qfPresetState state)
{
   QFBank *tmpBank;

   list.Find(wBankNum);
   if (!list.EndOfList())
   {
      tmpBank = list.GetCurItem();
      for (tmpBank->preset.First(); !tmpBank->preset.EndOfList();)
      {
         switch(state) {
         case qfpsUnload:
            UpdateRefs(*tmpBank->preset.GetCurItem());
            tmpBank->preset.DeleteCurItem();
            break;
         case qfpsNotAvailable:
         case qfpsAvailable:
            tmpBank->preset.GetCurItem()->state = state;
            tmpBank->preset.Next();
         };
      }
      if (state == qfpsUnload)
      {
         list.DeleteCurItem();
         UpdateMemLists(*sndmem);
      }
   }
}

void QuickFont::SetPresetState(WORD wBankNum, WORD wPresetNum,
                               qfPresetState state)
{
   QFBank *tmpBank;

   list.Find(wBankNum);
   if (!list.EndOfList())
   {
      tmpBank = list.GetCurItem();
      tmpBank->preset.Find(wPresetNum);
      if (!tmpBank->preset.EndOfList())
      {
         switch(state) {
         case qfpsUnload:
            UpdateRefs(*tmpBank->preset.GetCurItem());
            tmpBank->preset.DeleteCurItem();
            if (tmpBank->preset.IsEmpty())
               list.DeleteCurItem();
            UpdateMemLists(*sndmem);
            break;
         case qfpsNotAvailable:
         case qfpsAvailable:
            tmpBank->preset.GetCurItem()->state = state;
         };
      }
   }
}

void QuickFont::UpdateRefs(QFPreset& preset)
{
   QFKeyVelRange *tmpKVRange;
   QFRange *tmpRange;

   for (preset.kvrange.First(); !preset.kvrange.EndOfList();
        preset.kvrange.Next())
   {
      tmpKVRange = preset.kvrange.GetCurItem();
      for (tmpKVRange->range.First(); !tmpKVRange->range.EndOfList();
           tmpKVRange->range.Next())
      {
         tmpRange = tmpKVRange->range.GetCurItem();
         if (tmpRange->artdata.soundptr != NULL)
         {
            tmpRange->artdata.soundptr->wAllocatedRefCount--;
            if (preset.state != qfpsNotAvailable)
            {
               tmpRange->artdata.soundptr->wInUseRefCount--;
               if (tmpRange->artdata.soundptr->wInUseRefCount == 0)
                  tmpRange->artdata.soundptr->wInUseRefCount = 65535;
            }
         }
      }
   }
}

void QuickFont::UpdateMemLists(SoundMemory& sm)
{
   for (sm.SoundList.First(); !sm.SoundList.EndOfList();)
   {
      if (sm.SoundList.GetCurItem()->wInUseRefCount == 65535)
      {
         sm.SoundList.GetCurItem()->wInUseRefCount = 0;
         sm.ActionList.Insert(sm.SoundList.RemoveCurItem());
      }
      else
         sm.SoundList.Next();
   }
}
