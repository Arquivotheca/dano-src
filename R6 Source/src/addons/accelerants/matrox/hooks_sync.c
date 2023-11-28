//////////////////////////////////////////////////////////////////////////////
// Synchronization Hooks
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <Accelerant.h>
#include <Drivers.h>
#include <registers.h>
#include <private.h>

#include "globals.h"
#include "mga_bios.h"
#include "accelerant_info.h"
#include "hooks_sync.h"


//////////////////////////////////////////////////////////////////////////////
// Globals ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

static engine_token mga_engine_token = { 1, B_2D_ACCELERATION, NULL };


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Synchronize Acceleration Globals
//   This seems to have been gutted.  I think it may be discardable with
// few consequences.  Just move the clip-rect stuff to wherever this is
// called...

#if 0
void sync_acceleration_globals(void)
{
  // set clipping to entire frame buffer

  set_clip_rect(0,
                0,
                ai->pixels_per_row - 1,
                ai->dm.virtual_height - 1,
                ai->pixels_per_row,
                ai->YDstOrg);
}
#endif


//////////////////////////////////////////////////////////////////////////////
// Accelerant Engine Count
//    This looks a hell of a lot like a placeholder.  No comments in the
// original, but I'm going to out on a limb here and say that this code is
// supposed to do more than return 1.

uint32 AccelerantEngineCount(void)
{
  return 1;
}


//////////////////////////////////////////////////////////////////////////////
// Sync to Token

status_t SyncToToken(sync_token *st)
{
  uint64 fifo_diff;
  uint64 fifo_limit = ai->fifo_limit;

  // a quick out
  if (st->counter <= ai->last_idle_fifo) return B_OK;

  do
    {
      // calculate the age of the sync token

      fifo_diff = (volatile uint64)ai->fifo_count - st->counter;

      // anything more than fifo_limit fifo slots ago is guaranteed done
      // if the engine is idle, bail out 

    } while((fifo_diff < fifo_limit) && (STORM32(STORM_STATUS) & 0x00010000));

  ai->last_idle_fifo = st->counter;

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Acquire Engine

status_t AcquireEngine(uint32 caps,
                       uint32 max_wait,
                       sync_token *st,
                       engine_token **token)
{
  // acquire the shared benaphore

  if((atomic_add(&(ai->engine_ben), 1)) >= 1)
    {
      acquire_sem(ai->engine_sem);
    }

  // sync if required

  if(st) SyncToToken(st);

  // return an engine token

  *token = &mga_engine_token;

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Release Engine

status_t ReleaseEngine(engine_token *et,
                       sync_token *st)
{
  // update the sync token, if any

  if(st)
    {
      st->engine_id = et->engine_id;
      st->counter = ai->fifo_count;
    }

  // release the shared benaphore

  if((atomic_add(&(ai->engine_ben), -1)) > 1)
    {
      release_sem(ai->engine_sem);
    }

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Wait for the Engine to be Idle

void WaitEngineIdle(void)
{
  ai->last_idle_fifo = ai->fifo_count;
  STORM32POLL(STORM_STATUS, 0x00000000, 0x00010000);
}


//////////////////////////////////////////////////////////////////////////////
// Get a Sync Token

status_t GetSyncToken(engine_token *et,
                      sync_token *st)
{
  st->engine_id = et->engine_id;
  st->counter = ai->fifo_count;

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
