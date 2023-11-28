/*
 * accel.c:	Basic rendering acceleration.
 *
 * Copyright 2000 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */

#include <graphics_p/neomagic/neomagic.h>
#include <graphics_p/neomagic/debug.h>
#include "protos.h"

/*****************************************************************************
 * Globals.
 */
extern neomagic_card_info	*ci;

engine_token	enginetoken = {
	1, B_2D_ACCELERATION, NULL
};


/*****************************************************************************
 * Engine and sync_token management.
 */
uint32
getenginecount (void)
{
//	dprintf (("neomagic_accel: getenginecount - ENTER\n"));
	return (1);
}

status_t
acquireengine (uint32 caps, uint32 max_wait, sync_token *st, engine_token **et)
{
	(void) max_wait;

//	dprintf (("neomagic_accel: acquireengine(et:0x%08x, st:0x%08x) - ENTER\n", et, st));
	if (caps & B_3D_ACCELERATION)
		/*  No 3D acceleration yet.  */
		return (B_ERROR);

	lockBena4 (&ci->ci_EngineLock);

	/*  Sync if required  */
	if (st)
		synctotoken (st);

	/*  Return an engine_token  */
	*et = &enginetoken;
	return (B_OK);
}

status_t
releaseengine (engine_token *et, sync_token *st)
{
//dprintf(("neomagic_accel: releaseengine(et:0x%08x, st:0x%08x) - ENTER\n", et, st));
	if (!et)
	{
		dprintf(("neomagic_accel: releaseengine - EXIT called with null engine_token!\n"));
		return (B_ERROR);
	}

	/*  Update the sync token, if any  */
	if (st)
	{
//dprintf(("neomagic_accel: updating sync token - id: %d, count %Ld\n", et->engine_id, ci->ci_PrimitivesIssued));
		st->engine_id	= et->engine_id;
		st->counter	= ci->ci_PrimitivesIssued;
	}

	unlockBena4 (&ci->ci_EngineLock);
//dprintf (("neomagic_accel: releaseengine - EXIT\n"));
	return (B_OK);
}

void
waitengineidle (void)
{
//dprintf (("neomagic_accel: waitengineidle - ENTER\n"));
	switch (ci->ci_device_id)
	{
		case DEVICEID_NM2070:
		case DEVICEID_NM2090:
		case DEVICEID_NM2093:
		case DEVICEID_NM2097:
		case DEVICEID_NM2160:
		case DEVICEID_NM2200:
		case DEVICEID_NM256ZV:
			WAIT_ENGINE_IDLE(50);
			break;
		default:
			break;
	}
}

status_t
getsynctoken (engine_token *et, sync_token *st)
{
//dprintf (("neomagic_accel: getsynctoken(et:0x%08x, st:0x%08x) - ENTER\n", et, st));

	if (et)
	{
		st->engine_id	= et->engine_id;
		st->counter	= ci->ci_PrimitivesIssued;
		return (B_OK);
	} else
		return (B_ERROR);
}

/*
 * This does The Cool Thing (!) with the sync_token values, and waits for the
 * requested primitive to complete, rather than waiting for the whole engine
 * to drop to idle.  BEWARE - Do not actually write to the FIFO in this
 * routine - the engine lock is not currently held and you'll corrupt the FIFO
 * command stream.
 */
status_t
synctotoken (sync_token *st)
{
	uint32 serial;
	
//dprintf (("neomagic_accel: synctotoken(st->counter = %Ld) - ENTER\n", st->counter));

	waitengineidle();
	ci->ci_PrimitivesCompleted = st->counter;

//dprintf (("neomagic_accel: synctotoken - EXIT\n"));
	return (B_OK);
}

/*****************************************************************************
 * Rendering code.
 */
/*
 * Handles 8-, 15-, 16-, and 32-bit modes.
 */

/*  FIXME:  Write something useful here...  */
status_t
AccelInit (register struct neomagic_card_info *ci)
{
	return (B_OK);
}

