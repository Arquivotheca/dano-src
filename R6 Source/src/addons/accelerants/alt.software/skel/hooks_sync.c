//////////////////////////////////////////////////////////////////////////////
// Synchronization Hooks
//
//  This file implements hardware-independent hook functions used for drawing
// engine synchronization.
//
//////////////////////////////////////////////////////////////////////////////
// NOTE: At present, this code assumes that only one engine exists. This
// could be generalized by having a fifo count for each engine. However, as
// most cards have only one drawing engine, this should be a non-issue for
// the near future.

//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <common_includes.h>
#include <accel_includes.h>


//////////////////////////////////////////////////////////////////////////////
// Macros ////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Set this to "1" to enable tattletales on the engine manipulation functions,
// or "0" to disable tattletales.
#define ENGINE_TATTLETALES 0


//////////////////////////////////////////////////////////////////////////////
// Globals ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// This is the one and only engine token. We return a pointer to it when the
// user acquires the engine. IMO, this is a bit dangerous, as the user could
// overwrite this if they wanted to, but it will be adequate for now.
// NOTE: Make this more robust at some point.
static engine_token gds_engine_token = { 1, B_2D_ACCELERATION, NULL };


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Accelerant Engine Count
//    Returns the number of independent acceleration engines available within
// the card. for most cards, this is "1".

uint32 AccelerantEngineCount(void)
{
  return 1;
}


//////////////////////////////////////////////////////////////////////////////
// Sync to Token
//    Block until the last drawing operation specified by the given token has
// completed.

status_t SyncToToken(sync_token *st)
{
  uint64 fifo_diff;
  uint64 fifo_limit = si->engine.fifo_limit;

  // Diagnostics.
#if ENGINE_TATTLETALES
  ddprintf(("SyncToToken called.\n"));
#endif

  // a quick out
  if (st->counter <= si->engine.last_idle_fifo) return B_OK;

  // NOTE: This loop will eat quite a bit of processor time. Ideally, we'd
  // wait about 10-100 microseconds between polls, but granularity isn't
  // that fine for snooze yet.

  do
    {
      // calculate the age of the sync token

      fifo_diff = (volatile uint64)si->engine.fifo_count - st->counter;

      // anything more than fifo_limit fifo slots ago is guaranteed done
      // if the engine is idle, bail out 

    } while((fifo_diff < fifo_limit) && (ENGINE_2D_ACTIVE));

  // HACK - This isn't thread-safe.
  si->engine.last_idle_fifo = st->counter;

  // Diagnostics.
#if ENGINE_TATTLETALES
  ddprintf(("SyncToToken finished.\n"));
#endif

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Acquire Engine
//    Gain exclusive control of a drawing engine.

status_t AcquireEngine(uint32 caps,
                       uint32 max_wait,
                       sync_token *st,
                       engine_token **token)
{
  // Diagnostics.
#if ENGINE_TATTLETALES
  ddprintf(("AcquireEngine called.\n"));
#endif

  // acquire the shared benaphore

  ACQUIRE_GDS_BENAPHORE(&(si->engine.engine_ben));

  // sync if required

  if(st) SyncToToken(st);

  // return an engine token

  *token = &gds_engine_token;

  // Diagnostics.
#if ENGINE_TATTLETALES
  ddprintf(("AcquireEngine finished.\n"));
#endif

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Release Engine
//    Release exclusive control of a drawing engine.

status_t ReleaseEngine(engine_token *et,
                       sync_token *st)
{
  // Diagnostics.
#if ENGINE_TATTLETALES
  ddprintf(("ReleaseEngine called.\n"));
#endif

  // update the sync token, if any

  if(st)
    {
      st->engine_id = et->engine_id;
      st->counter = si->engine.fifo_count;
    }

  // release the shared benaphore

  RELEASE_GDS_BENAPHORE(&(si->engine.engine_ben));

  // Diagnostics.
#if ENGINE_TATTLETALES
  ddprintf(("ReleaseEngine finished.\n"));
#endif

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// Wait for the Engine to be Idle

void WaitEngineIdle(void)
{
  // Diagnostics.
#if ENGINE_TATTLETALES
  ddprintf(("WaitEngineIdle called.\n"));
#endif

  // HACK: If something else is feeding in drawing operations concurrently,
  // this may screw up future sync_to_token calls. Problems occur if something
  // increments fifo_count between these two operations.

  while (ENGINE_2D_ACTIVE)
    ; // Spin.
  si->engine.last_idle_fifo = si->engine.fifo_count;

  // Diagnostics.
#if ENGINE_TATTLETALES
  ddprintf(("WaitEngineIdle finished.\n"));
#endif
}


//////////////////////////////////////////////////////////////////////////////
// Get a Sync Token
//    Obtain a token uniquely specifying the last drawing operation performed,
// so that we can later determine whether or not it has been completed.

status_t GetSyncToken(engine_token *et,
                      sync_token *st)
{
  st->engine_id = et->engine_id;
  st->counter = si->engine.fifo_count;

  return B_OK;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
