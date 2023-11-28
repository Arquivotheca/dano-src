#include "private.h"
#include "generic.h"
#include "SetRegisters.h"


static engine_token ati_engine_token = { 1, B_2D_ACCELERATION, NULL };

#define wait_for_slots(numSlots) { while ((regs[FIFO_STAT] & 0xffff) > (0x8000 >> (numSlots))); }
#define wait_empty_idle() wait_for_slots(16) ; while ((regs[GUI_STAT] & 1) != 0)

uint32 ACCELERANT_ENGINE_COUNT(void) {
	return 1;
}

status_t ACQUIRE_ENGINE(uint32 capabilities, uint32 max_wait, sync_token *st, engine_token **et) {
	/* acquire the shared benaphore */
	int32 old = atomic_add(&(ai->engine_ben), 1);
	if (old >= 1) acquire_sem(ai->engine_sem);

	/* sync if required */
	if (st) SYNC_TO_TOKEN(st);

	/* return an engine token */
	*et = &ati_engine_token;
	return B_OK;
}

status_t RELEASE_ENGINE(engine_token *et, sync_token *st) {
	int32 old;

	/* update the sync token, if any */
	if (st) {
		st->engine_id = et->engine_id;
		st->counter = ai->fifo_count;
	}

	/* release the shared benaphore */
	old = atomic_add(&(ai->engine_ben), -1);
	if (old > 1) release_sem(ai->engine_sem);
	return B_OK;
}

void WAIT_ENGINE_IDLE(void) {

	// note our current possition
	ai->last_idle_fifo = ai->fifo_count;

	wait_empty_idle();
}

status_t GET_SYNC_TOKEN(engine_token *et, sync_token *st) {
	st->engine_id = et->engine_id;
	st->counter = ai->fifo_count;
	return B_OK;
}

status_t SYNC_TO_TOKEN(sync_token *st) {
#if 0
	uint64 fifo_diff;
	uint64 fifo_limit;
	uint32 fifo_mask;
	
	/* a quick out */
	if (st->counter < ai->last_idle_fifo) return B_OK;

	/* the full monty */
	fifo_limit = ai->fifo_limit;
	fifo_mask = ai->fifo_mask;
	do {
		/* calculate the age of the sync token */
		fifo_diff = (vuint64)(ai->fifo_count) - st->counter;
		/* add in the number of free slots in the fifo */
		fifo_diff += (uint64)((regs[GUI_STAT] >> 16) & 0x003f);
		/*
		   The astute observer will notice that the free slot counter
		   doesn't have enough bits to represent the full FIFO depth.
		   This means that for "recent" operations, we end up waiting
		   on engine idle :-(
		*/
#if 0
		/* add one if the engine is idle (for when st->counter == ai->fifo_count) */
		if (!(regs[GUI_STAT] & 0x01)) fifo_diff++;
#endif
		/* anything more than fifo_limit fifo slots ago is guaranteed done */
		/* if the engine is idle, bail out */
	} while ((fifo_diff <= fifo_limit) && (regs[GUI_STAT] & 0x01));
	ai->last_idle_fifo = st->counter;
#else
	WAIT_ENGINE_IDLE();
#endif
	return B_OK;
}

