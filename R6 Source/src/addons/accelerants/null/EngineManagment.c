#include "GlobalData.h"
#include "generic.h"


uint32 ACCELERANT_ENGINE_COUNT(void) {
	return 1;
}

status_t ACQUIRE_ENGINE(uint32 capabilities, uint32 max_wait, sync_token *st, engine_token **et) {
	(void)capabilities;(void)max_wait;(void)st;(void)et;
	return B_OK;
}

status_t RELEASE_ENGINE(engine_token *et, sync_token *st) {
	(void)et;(void)st;
	return B_OK;
}

void WAIT_ENGINE_IDLE(void) {
}

status_t GET_SYNC_TOKEN(engine_token *et, sync_token *st) {
	static uint64 engine_count = 0;
	(void)et;
	st->engine_id = 0;
	st->counter = engine_count++;
	return B_OK;
}

status_t SYNC_TO_TOKEN(sync_token *st) {
	(void)st;
	return B_OK;
}

