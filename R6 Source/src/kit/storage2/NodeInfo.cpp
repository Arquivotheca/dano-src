16*)((char *)node->next->packet_buffer + sofar))));

					memcpy(virt_addr, (char *)node->next->packet_buffer + sofar, vec[i].iov_len);
					sofar += vec[i].iov_len;
				}
				free(node->next->copy_back_p_addrs);
			}
									
			/* Return the packet buffer */
			put_packet_buffer(bus, node->next->packet_buffer);
			
			/* Make sure to remove both phys entries */
			tmp = node;
			node = node->next;
			put_phys_entry(bus, tmp);
		}
		tmp = node;
		
		node = tmp->next;
		put_phys_entry(bus, tmp);
	}
}
#endif



typedef struct usb_request 
{
	uint8 request_type;
	uint8 request;
	uint16 value;
	uint16 index;
	uint16 length;
} usb_request;

status_t send_request_safe(const usb_id id, 
						   uint8 request_type, uint8 request,
						   uint16 value, uint16 index, uint16 length,
						   void *data, size_t *actual_len)
{
	/* some requests have side effects and may only be issued
	   by the USBD itself */
	if(request_type == USB_REQTYPE_DEVICE_OUT){
		if(request == USB_REQUEST_SET_ADDRESS) {
			return B_ERROR;
		}
		if(request == USB_REQUEST_SET_INTERFACE) {
			return B_ERROR;
		}
	}
	if(request_type == USB_REQTYPE_INTERFACE_OUT){
		if(request == USB_REQUEST_SET_INTERFACE) {
			return B_ERROR;
		}
	}
	
	return send_request(id,request_type,request,value,index,length,
						data,actual_len);
	
}

status_t send_request(const usb_id id, 
					  uint8 request_type, uint8 request,
					  uint16 value, uint16 index, uint16 length,
					  void *data, size_t *actual_len)
{
	status_t status;
	usb_request *req;
	bm_usb_device * d;
	bm_usb_pipe *pipe;
	usb_iob *iob;
	hcd_phys_entry	* entries, * setup_entries;
	iovec vec;
	
	d = id_to_device(id);
	if (!d) {
		return B_BAD_VALUE;
	}
	pipe = d->control;
	CHECK_PIPE();
	LOCKM(pipe->lock, 1);
	
	iob = get_iob(pipe,1);
	
	req = (usb_request*) iob->buffer;
	req->request_type = request_type;
	req->request = request;
	/*XXX endian? */
	req->value = value;
	req->index = index;
	req->length = length;
	
	/* Handle the user data */
	iob->transfer.user_ptr = data;
	iob->transfer.user_length = length;
	if (data && (length > 0)) {
		vec.iov_base = data;
		vec.iov_len = length;
		status = create_phys_entry_list(d->control, &vec, 1, &entries, req->request_type & USB_REQTYPE_DEVICE_IN);
		if (status) {
			dprintf(ID "error with create_phys_entry_list: %s\n", strerror(status));
			goto err;
		}
		if (!entries) {
			iob->transfer.entries = &null_packet_entry;
		} else {
			iob->transfer.entries = entries;
		}
	} else {
		iob->transfer.entries = NULL;
	}
	
	/* Handle the setup data */
	iob->transfer.setup = req;
	iob->transfer.setup_size = sizeof(usb_request);
	vec.iov_base = req;
	vec.iov_len = iob->transfer.setup_size;
	status = create_phys_entry_list(d->control, &vec, 1, &setup_entries, FALSE);
	if (status) {
		dprintf(ID "error with create_phys_entry_list: %s\n", strerror(status));
		goto err;
	}
	if (!setup_entries) {
		iob->transfer.setup_entries = &null_packet_entry;
	} else {
		iob->transfer.setup_entries = setup_entries;
	}
	
	/* Callback stuff */
	iob->transfer.callback = NULL;
	iob->transfer.notify = create_sem(0,"usbd:iob:notify"); /* BUGBUG should use already created sem */
	if(iob->transfer.notify < B_OK) {
		dprintf(ID "error creating semaphore\n");
		goto err;
	}
	
//	dprintf(ID "%02x %02x %04x %04 %04x\n",
//			request_type, request, value, index,length);
	
	status = pipe->ifc->dev->bus->ops->queue_transfer(pipe->ept,&iob->transfer);
	
	if(status == B_OK){
		status = acquire_sem_etc(iob->transfer.notify, 1, B_TIMEOUT, pipe->timeout);
		if(status == B_TIMED_OUT){
			cancel_queued_transfers(pipe->id);
			acquire_sem(iob->transfer.notify);				
			/*XXX should return timed out response */
		}
		status = iob->transfer.status;
		if(actual_len) {
			*actual_len = iob->transfer.actual_length;
		}
	} else {
		if(actual_len) {
			*actual_len = 0;
		}
	}
	
	/* Cleanup */
	delete_sem(iob->transfer.notify);
	put_iob(pipe,iob);
	
	UNLOCKM(pipe->lock, 1);
	return status;
	
err:
	UNLOCKM(pipe->lock, 1);
	return status;
}

static void 
transfer_cb(hcd_transfer *tr)
{
	usb_iob *iob = (usb_iob *) tr->cookie;
	
	usb_callback_func cb = iob->callback;
	
	/* If there was an error report it */
	if (tr->status) {
		dprintf(ID "transfer completed with error %08x\n", (unsigned)tr->status);
		print_phys_entries(tr->entries);
		
		/* Cleanup */
		cleanup_phys_entry_list((usb_bus *)((bm_usb_pipe *)iob->next)->ifc->dev->bus, tr->entries, true);
	} else {
		cleanup_phys_entry_list((usb_bus *)((bm_usb_pipe *)iob->next)->ifc->dev->bus, tr->entries, false);
	}
	
	/* Cleanup the resources this transfer was using */
	put_iob((bm_usb_pipe*) iob->next, iob);
	
	/* Make a callback to the user if we need to */
	if(cb) {
		cb(iob->cookie, tr->status, tr->user_ptr, tr->actual_length);
	}
}
	
status_t queue_request(const usb_id id, 
					   uint8 request_type, uint8 request,
					   uint16 value, uint16 index, uint16 length,
					   void *data, usb_callback_func notify, void *cookie)
					   
{
	status_t status;
	usb_request *req;
	bm_usb_device * d;
	bm_usb_pipe *pipe;
	usb_iob *iob;
	hcd_phys_entry	* entries, * setup_entries;
	iovec vec;
	
	d = id_to_device(id);
	if (!d) {
		return B_BAD_VALUE;
	}
	pipe = d->control;
	CHECK_PIPE();
	
	LOCKM(pipe->lock, 1);
	
	/* some requests have side effects and may only be issued
	   by the USBD itself */
	if(request_type == USB_REQTYPE_DEVICE_OUT){
		if(request == USB_REQUEST_SET_ADDRESS) return B_ERROR;
		if(request == USB_REQUEST_SET_INTERFACE) return B_ERROR;
	}
	if(request_type == USB_REQTYPE_INTERFACE_OUT){
		if(request == USB_REQUEST_SET_INTERFACE) return B_ERROR;
	}
	
	iob = get_iob(pipe,1);
	
	req = (usb_request*) iob->buffer;
	req->request_type = request_type;
	req->request = request;
	/*XXX endian? */
	req->value = value;
	req->index = index;
	req->length = length;
	
	/* Driver callback stuff */
	iob->cookie = cookie;
	iob->callback = notify;
	
	/* Handle the user data */
	iob->transfer.user_ptr = data;
	iob->transfer.user_length = length;
	if (data && (length > 0)) {
		vec.iov_base = data;
		vec.iov_len = length;
		status = create_phys_entry_list(d->control, &vec, 1, &entries, req->request_type & USB_REQTYPE_DEVICE_IN);
		if (status) {
			dprintf(ID "error with create_phys_entry_list: %s\n", strerror(status));
			goto err;
		}
		if (!entries) {
			iob->transfer.entries = &null_packet_entry;
		} else {
			iob->transfer.entries = entries;
		}
	} else {
		iob->transfer.entries = NULL;
	}

	/* Handle the setup data */
	iob->transfer.setup = req;
	iob->transfer.setup_size = sizeof(usb_request);
	vec.iov_base = req;
	vec.iov_len = iob->transfer.setup_size;
	status = create_phys_entry_list(d->control, &vec, 1, &setup_entries, FALSE);
	if (status) {
		dprintf(ID "error with create_phys_entry_list: %s\n", strerror(status));
		goto err;
	}
	if (!setup_entries) {
		iob->transfer.setup_entries = &null_packet_entry;
	} else {
		iob->transfer.setup_entries = setup_entries;
	}
	
	/* Bus callback stuff */
	iob->transfer.callback = transfer_cb;
	iob->transfer.cookie = iob;
	
//dprintf("queue_request: %02x %02x %04x %04 %04x\n",
//		request_type, request, value, index,length);
	
	status = pipe->ifc->dev->bus->ops->queue_transfer(pipe->ept,&iob->transfer);	

	if(status) put_iob((bm_usb_pipe*)pipe,iob);

err:
	UNLOCKM(pipe->lock, 1);
	return status;
}

status_t queue_interrupt(const usb_id id, void *data, size_t len,
						 usb_callback_func notify, void *cookie)
{
	status_t status;
	usb_iob *iob;
	hcd_phys_entry	* entries;
	iovec vec;
	bm_usb_pipe * pipe;
	
	pipe = id_to_pipe(id);
	if (! pipe) {
		dprintf(ID "id_to_pipe failed for id %lu\n", id);
		return B_DEV_INVALID_PIPE;
	}
	LOCKM(pipe->lock, 1);
	
	iob = get_iob((bm_usb_pipe*)pipe,1);

	iob->cookie = cookie;
	iob->callback = notify;
	
	vec.iov_base = data;
	vec.iov_len = len;
	status = create_phys_entry_list(pipe, &vec, 1, &entries, is_inbound_pipe(pipe));
	if (status) {
		dprintf(ID "error with create_phys_entry_list: %s\n", strerror(status));
		goto err;
	}
	
	if (!entries) {
		entries = &null_packet_entry;
	}
	
	iob->transfer.setup = NULL;
	iob->transfer.setup_size = 0;
	iob->transfer.setup_entries = NULL;
	iob->transfer.entries = entries;
	iob->transfer.user_ptr = data;
	iob->transfer.user_length = len;
	iob->transfer.callback = transfer_cb;
	iob->transfer.cookie = iob;
	
	iob->next = (usb_iob *) pipe; //XXX HACK
	
	status = pipe->ifc->dev->bus->ops->queue_transfer(pipe->ept,&iob->transfer);
	
	if(status) put_iob((bm_usb_pipe*)pipe,iob);
	
err:
	UNLOCKM(pipe->lock, 1);
	return status;
}

status_t queue_bulk(const usb_id id, void *data, size_t len,
					usb_callback_func notify, void *cookie)
{
	status_t status;
	usb_iob *iob;
	hcd_phys_entry	* entries;
	iovec vec;
	bm_usb_pipe * pipe;
	
	pipe = id_to_pipe(id);
	if (! pipe) {
		dprintf(ID "id_to_pipe failed for id %lu\n", id);
		return B_DEV_INVALID_PIPE;
	}
	
	LOCKM(pipe->lock, 1);
	
	iob = get_iob((bm_usb_pipe*)pipe,1);

	iob->cookie = cookie;
	iob->callback = notify;
	
	vec.iov_base = data;
	vec.iov_len = len;
	status = create_phys_entry_list(pipe, &vec, 1, &entries, is_inbound_pipe(pipe));
	if (status) {
		dprintf(ID "error with create_phys_entry_list: %s\n", strerror(status));
		goto err;
	}
	
	if (!entries) {
		entries = &null_packet_entry;
	}
	
	iob->transfer.setup = NULL;
	iob->transfer.setup_size = 0;
	iob->transfer.setup_entries = NULL;
	iob->transfer.entries = entries;
	iob->transfer.user_ptr = data;
	iob->transfer.user_length = len;
	iob->transfer.callback = transfer_cb;
	iob->transfer.cookie = iob;
	
	/* next is not in use when the iob is not in the pool, so reuse it here */
	iob->next = (usb_iob *) pipe; //XXX HACK
	
	status = pipe->ifc->dev->bus->ops->queue_transfer(pipe->ept,&iob->transfer);
	
	if(status) put_iob((bm_usb_pipe*)pipe,iob);
	
err:
	UNLOCKM(pipe->lock, 1);		
	return status;
}

status_t queue_bulk_v(const usb_id id, iovec * vec, size_t count,
					usb_callback_func notify, void *cookie)
{
	status_t status;
	usb_iob *iob;
	hcd_phys_entry	* entries;
	uint32 i;
	int32 len;
	iovec * tmp;
	bm_usb_pipe * pipe;
	
	/* Sanity check */
	if(! vec || count <= 0) {
		return B_BAD_VALUE;
	}

	pipe = id_to_pipe(id);
	if (! pipe) {
		dprintf(ID "id_to_pipe failed for id %lu\n", id);
		return B_DEV_INVALID_PIPE;
	}
	
	LOCKM(pipe->lock, 1);
	
	for(len = 0, tmp = vec, i = 0; i < count; i++, tmp++) {
		len += tmp->iov_len;
	}
	ddprintf(ID "len = %ld\n", len);
	
	
	iob = get_iob((bm_usb_pipe*)pipe,1);

	iob->cookie = cookie;
	iob->callback = notify;
	
	ddprintf(ID "---> count = %ld, len = %ld\n", count, len);
	
	status = create_phys_entry_list(pipe, vec, count, &entries, is_inbound_pipe(pipe));
	if (status) {
		dprintf(ID "error with create_phys_entry_list: %s\n", strerror(status));
		goto err;
	}
	
	if (!entries) 