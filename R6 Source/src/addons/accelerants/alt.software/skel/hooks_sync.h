//////////////////////////////////////////////////////////////////////////////
// Synchronization Hooks
//
//  This file declares hardware-independent hook functions used for drawing
// engine synchronization.
//
//////////////////////////////////////////////////////////////////////////////

status_t SyncToToken(sync_token *st);

uint32 AccelerantEngineCount(void);

status_t AcquireEngine(uint32 caps,
                       uint32 max_wait,
                       sync_token *st,
                       engine_token **token);

status_t ReleaseEngine(engine_token *et,
                       sync_token *st);

void WaitEngineIdle(void);

status_t GetSyncToken(engine_token *et,
                      sync_token *st);


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
