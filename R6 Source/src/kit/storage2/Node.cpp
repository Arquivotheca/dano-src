equest
			**	up into peices for the bus when we run out of resources,
			**	but we need DPCs to do that so this will work for now. (JWH)
			*/
			if (i >= PHYS_ENTRY_BUFFER_SIZE) {
				err = get_memory_map(((char *)buf + mapped_bytes), len - mapped_bytes, pe, PHYS_ENTRY_BUFFER_SIZE);
				if (err) {
					dprintf(ID "error in get_memory_map\n");
					return B_ERROR;
				}
				i = 0;
			}
			
			/* Set the base for this phys entry */
			base = pe[i].address;
				
			ddprintf("\n" ID "for loop - pe[%ld].address = %p, pe[%ld].size = %ld\n", i, pe[i].address, i, pe[i].size);
	
			sofar = 0;
			while(sofar < pe[i].size) {
				/* We do not need to finish a packet copy */
				if (! packet_buffer) {
					
					/* Find the next page boundry and calculate the last_packet */
					next_page = (uint8*)((uint32)(base + 4096) & ~(4096 - 1));
					size = (next_page - base);
					if(pe[i].size < sofar + size) {
						size = pe[i].size - sofar;
						last_packet = 0;
					} else {
						int32 tmp;
						tmp = (mapped_bytes + size) % packet_size;
						if (tmp) {
							last_packet = size - tmp;
						} else {
							last_packet = 0;
						}
						ddprintf(ID "last_packet calc: mapped_bytes = %ld, sofar = %ld, tmp = %ld, size = %ld, last_packet = %ld\n", mapped_bytes, sofar, tmp, size, last_packet);
					}
	
					/* Start a packet copy if it is needed */
					if (((bus->page_crossings_allowed == 0 && size < packet_size) || (len < packet_size)) && (mapped_bytes + size < len)) {
						if (inbound && !(pipe->addr_flags & PIPE_CONTROL)) {
							/* A read, only set aside buffer, do not copy */
							dprintf("do read\n");
							cur = get_phys_entry(bus);
							cur->packet_buffer = get_packet_buffer(bus);
							cur->size = packet_size;
							cur->address = phys_addr(cur->packet_buffer);
							cur->copy_back_p_addrs = NULL;
							
							/* Update pointers, etc */
							sofar += size;
							mapped_bytes += size;
							base += size;
						} else {
							/* A write, we must copy */
							void * src = buf + mapped_bytes;
							dprintf("do write\n");
							packet_buffer = get_packet_buffer(bus);
							if (! packet_buffer) {
								dprintf(ID "error in get_packet_buffer\n");
								return B_NO_MEMORY;
							}
							packet_copy_size = next_page - base;
							
							/* XXX this should be removed when we get DPCs and can use the virtual buffer
							** passed into the queue by the client. This is only here to create a list of
							** physical entries to use to get virtual addresses from later when we need to
							** undo the packet copy. (JWH)
							*/
							copy_vec = malloc(sizeof(iovec) * 65);
							copy_vec_offset = 0;
							
							dprintf(ID "about to do first copy, mapped_bytes = %ld, packet_copy_size = %ld\n", mapped_bytes, packet_copy_size);
							memcpy(packet_buffer, src, packet_copy_size);
							
							/* XXX see the above note (JWH) */
							copy_vec[copy_vec_offset].iov_base = phys_addr(src);
							copy_vec[copy_vec_offset].iov_len = packet_copy_size;
							copy_vec_offset++;
							
							/* Create a valid state */
							sofar += packet_copy_size;
							base += packet_copy_size;
							continue;
						}					
					} else { /* Do not need to start a packet copy, send block as is*/
						cur = get_phys_entry(bus);
						cur->address = base;
						cur->packet_buffer = NULL;
						cur->copy_back_p_addrs = NULL;
		
						/* If the hardware can't handle the page boundry split on packets */
						if (last_packet) {
							if (page_boundries_left <= 0) {
								ddprintf(ID "dis-allowing page boundry crossing\n");
								size = last_packet;
								page_boundries_left = bus->page_crossings_allowed;
							} else {
								page_boundries_left--;
							}
						} else {
							page_boundries_left--;
						}
						
						/* Update pointers, etc */
						cur->size = size;
						sofar += size;
						mapped_bytes += size;
						base += size;
					}
					
				} else { /* Finish a packet copy */
					bool	copy_done = true;
					int32	prev_copy_size = packet_copy_size;
					void	* src = buf + mapped_bytes + prev_copy_size;
					packet_copy_size = packet_size - prev_copy_size;
					
					/* Make sure we only copy the phys entry size */
					if (packet_copy_size > pe[i].size - sofar) {
						packet_copy_size = pe[i].size - sofar;
						
						/* If this is the end we are done, if not keep copying */
						if (j == count - 1 && mapped_bytes + packet_copy_size == len) {
							copy_done = true;
						} else {
							ddprintf(ID "setting copy_done to false, mapped_bytes + packet_copy_size = %ld, len = %ld\n", mapped_bytes + packet_copy_size, len);
							copy_done = false;
						}
					}
					
					dprintf(ID "about to finish packet copy, mapped_bytes = %ld, last_packet = %ld, packet_copy_size = %ld, copying = %ld\n", mapped_bytes, last_packet, packet_copy_size, packet_size - packet_copy_size);
					
					memcpy(packet_buffer + prev_copy_size, src, packet_copy_size);
					sofar += packet_copy_size;
					
					copy_vec[copy_vec_offset].iov_base = phys_addr(src);
					copy_vec[copy_vec_offset].iov_len = packet_copy_size;
					copy_vec_offset++;
					
					/* We didn't get enough data yet */
					if (! copy_done) {
						dprintf(ID "packet copy not complete\n");
						packet_copy_size += prev_copy_size;
						continue;
					}
					
					/* Setup the phys entry for the buffer */
					cur = get_phys_entry(bus);
					cur->packet_buffer = packet_buffer;
					cur->address = phys_addr(packet_buffer);
					cur->size = packet_size;
					
					/* Terminate the copy back list */
					copy_vec[copy_vec_offset].iov_base = NULL;
					copy_vec[copy_vec_offset].iov_len = 0;
					cur->copy_back_p_addrs = copy_vec;
					
#if 0
					{
						int tmp;
						for (tmp = 0; copy_vec[tmp].iov_len != 0; tmp++) {
							ddprintf(ID "copy_vec[%d].iov_base = %p, iov_len = %ld\n", tmp, copy_vec[tmp].iov_base, copy_vec[tmp].iov_len);
						}
					}
#endif
					/* Create a valid state */
					packet_buffer = NULL;
					
					/* Update pointers, etc */
					mapped_bytes += packet_size;
					base += packet_copy_size;
				}
				
				ddprintf("created a physical entry of size %ld, sofar = %ld\n", cur->size, sofar);
				
				/* Linked list maintenence */
				cur->next = NULL;
				if (last) {
					last->next = cur;
				}
				if (!head) {
					head = cur;
				}
				last = cur;
			}
		}
		total_bytes += mapped_bytes;
	}
	*pe_out = head;
	
#if _USBD_DEBUG
	if (len >= 512) {
		print_phys_entries(head);
	}
#endif

	ddprintf(ID "<--- total_bytes = %ld\n", total_bytes);
	return B_OK;
}

/*	cleanup_phys_entry_list
**	This function takes care of cleaning up a phys entry list after it has been used and returned by the bus.
**	It returns all allocated resources to the appropriate resource pools and also copies buffered data back
**	into the users buffer.
*/
static void cleanup_phys_entry_list(usb_bus * bus, hcd_phys_entry * node, bool print)
{
	hcd_phys_entry * tmp;
	
	/* Put the hcd_phys_entries back into the free pool */
	while (node) {
		if (node->next && node->next->packet_buffer) {
			iovec * vec = node->next->copy_back_p_addrs;
			
			if (vec) {
				int32 i, sofar;
				
				/* Print out the copy back array? */
				//if (print)
				{
					int tmp;
					for (tmp = 0; vec[tmp].iov_len != 0; tmp++) {
						dprintf(ID "vec[%d].iov_base = %p, iov_len = %ld\n", tmp, vec[tmp].iov_base, vec[tmp].iov_len);
						//dprintf("%d %p %ld\n", tmp, vec[tmp].iov_base, vec[tmp].iov_len);
					}
				}
				
				/* Perform the copy back */
				for(i = 0, sofar = 0; vec[i].iov_base; i++) {
					/*	XXX Only do this patova now because we don't have the VM context of the client buffer
					**	This should be removed as soon as we get DPCs and the VM context along 