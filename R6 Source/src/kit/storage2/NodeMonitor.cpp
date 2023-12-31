n status;
}

#if 0 // Avoid compile warning
static 
void isoch_transfer_cb(hcd_transfer *tr)
{
}
#endif


status_t queue_isochronous(const usb_id id, void *data, size_t len,
						   rlea* rle_array, uint16	buffer_timelen,
						   usb_callback_func notify, void *cookie)
{
	bm_usb_pipe	* pipe;
	
	pipe = id_to_pipe(id);
	if (! pipe) {
		dprintf(ID "id_to_pipe failed for id %lu\n", id);
		return B_DEV_INVALID_PIPE;
	}

	LOCKM(pipe->lock, 1);

	UNLOCKM(pipe->lock, 1);

	return B_ERROR;
}


status_t set_pipe_policy(const usb_id id, 
						 uint8 max_num_queued_packets, 
						 uint16 max_buffer_duration_ms, 
						 uint16 sample_size)
{
	bm_usb_pipe	* pipe;
	
	pipe = id_to_pipe(id);
	if (! pipe) {
		dprintf(ID "id_to_pipe failed for id %lu\n", id);
		return B_DEV_INVALID_PIPE;
	}
	
	LOCKM(pipe->lock, 1);

	UNLOCKM(pipe->lock, 1);
	
	return B_OK;
}


status_t cancel_queued_transfers(const usb_id id)
{
	bm_usb_pipe	* pipe;
	status_t	err;
	
	pipe = id_to_pipe(id);
	if (! pipe) {
		dprintf(ID "id_to_pipe failed for id %lu\n", id);
		return B_DEV_INVALID_PIPE;
	}
	
	LOCKM(pipe->lock, 1);
	err = pipe->ifc->dev->bus->ops->cancel_transfers(pipe->ept);
	UNLOCKM(pipe->lock, 1);

	return err;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 #include <stdio.h>

#include "usbd.h"

static char namebuf[256];


static void dump_buf(void *buf, uint size)
{
	char out[16*4];
	char *p = out;
	uchar *x = bu