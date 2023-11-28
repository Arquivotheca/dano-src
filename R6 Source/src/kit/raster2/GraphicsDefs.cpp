, uchar channel, uchar speed,
		int32 num_buffers, struct ieee1394_iio_buffer *buffers)
{
	struct _1394_bus *bus = (struct _1394_bus *)cookie;
	status_t err;

	if ((err = acquire_sem(busses_sem)) < 0)
		return err;

	if ((err = verify_cookie(bus)) != B_OK)
		goto err1;

	err = (bus->module->queue_isochronous)(bus->bus_cookie,
			port, flags, data, channel, speed, num_buffers, buffers);

err1:
	release_sem(busses_sem);
	return err;
}

static status_t
_1394_stop_isochronous(void *cookie, struct ieee1394_istop *istop)
{
	struct _1394_bus *bus = (struct _1394_bus *)cookie;
	status_t err;

	if ((err = acquire_sem(busses_sem)) < 0)
		return err;

	if ((err = verify_cookie(bus)) != B_OK)
		goto err1;

	err = (bus->module->stop_isochronous)(bus->bus_cookie, istop);

err1:
	release_sem(busses_sem);
	return err;
}

static struct _1394_module_info driver_module = {
	{
		{ B_1394_FOR_DRIVER_MODULE_NAME, 0, &_1394_std_ops },
		NULL
	},
	&_1394_get_bus_count,
	&_1394_get_bus_cookie,
	&_1394_get_bus_info,
	&_1394_get_node_info,
	&_1394_reset_bus,
	&_1394_map_addresses,
	&_1394_unmap_addresses,
	&_1394_register_bus_reset_notification,
	&_1394_unregister_bus_reset_notification,
	&_1394_acquire_guid,
	&_1394_release_guid,
	&_1394_quadlet_read,
	&_1394_block_read,
	&_1394_quadlet_write,
	&_1394_block_write,
	&_1394_lock_32,
	&_1394_send_phy_packet,

	&_1394_init_fcp_context,
	&_1394_uninit_fcp_context,
	&_1394_send_fcp_command,
	&_1394_wait_for_fcp_response,

	&_1394_get_cycle_time,
	&_1394_acquire_isochronous_port,
	&_1394_release_isochronous_port,
	&_1394_reserve_bandwidth,
	&_1394_reserve_isochronous_channel,
	&_1394_queue_isochronous,
	&_1394_stop_isochronous
};

static status_t
_1394_for_bus_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT :
		case B_MODULE_UNINIT :
			return B_OK;
	}

	return ENOSYS;
}

static status_t
notify_bus_reset(void *cookie)
{
	struct _1394_bus *bus = (struct _1394_bus *)cookie;
	status_t err;
	int32 i;

	if ((err = acquire_sem(busses_sem)) < 0)
		return err;

	if ((err = verify_cookie(bus)) != B_OK)
		goto err1;

	for (i=0;i<bus->bstate->num_bus_reset_sems;i++)
		release_sem_etc(bus->bstate->bus_reset_sems[i], 1, B_DO_NOT_RESCHEDULE);

err1:
	release_sem(busses_sem);
	return err;
}

static status_t
bus_quadlet_read(void *cookie, uchar speed, struct nodeid *node, uint64 offset,
		uint32 *quadlet, bigtime_t timeout)
{
	struct _1394_bus *bus = (struct _1394_bus *)cookie;
	status_t err;
	uint32 *q;

	if ((err = acquire_sem(busses_sem)) < 0)
		return err;

	if ((err = verify_cookie(bus)) != B_OK)
		goto err1;

	if (!(q = malloc(4))) {
		err = ENOMEM;
		goto err1;
	}

	if ((err = SQuadletRead(bus, speed, node, offset, q, timeout)) == B_OK)
		*quadlet = *q;

	free(q);

err1:
	release_sem(busses_sem);
	return err;
}

static status_t
bus_block_read(void *cookie, uchar speed, struct nodeid *node, uint64 offset,
		int32 veclen, struct iovec *vec, bigtime_t timeout)
{
	struct _1394_bus *bus = (struct _1394_bus *)cookie;
	status_t err;
	int32 nveclen;
	struct iovec *nvec;

	if ((err = acquire_sem(busses_sem)) < 0)
		return err;

	if ((err = verify_cookie(bus)) != B_OK)
		goto err1;

	if ((err = split_and_lock_buffer(veclen, vec, &nveclen, &nvec, B_DMA_IO | B_READ_DEVICE)) < B_OK)
		goto err1;

	err = SBlockRead(bus, speed, node, offset, nveclen, nvec, timeout);

	unlock_and_delete_buffer(veclen, vec, nveclen, nvec, B_DMA_IO | B_READ_DEVICE);

err1:
	release_sem(busses_sem);
	return err;
}

static status_t
bus_quadlet_write(void *cookie, uchar speed, struct nodeid *node, uint64 offset,
		uint32 quadlet, bigtime_t timeout)
{
	struct _1394_bus *bus = (struct _1394_bus *)cookie;
	status_t err;

	if ((err = acquire_sem(busses_sem)) < 0)
		return err;

	if ((err = verify_cookie(bus)) != B_OK)
		goto err1;

	err = SQuadletWrite(bus, speed, node, offset, quadlet, timeout);

err1:
	release_sem(busses_sem);
	return err;
}

static status_t
bus_block_write(void *cookie, uchar speed, struct nodeid *node, uint64 offset,
		int32 veclen, struct iovec *vec, bigtime_t timeout)
{
	struct _1394_bus *bus = (struct _1394_bus *)cookie;
	status_t err;
	int32 nveclen;
	struct iovec *nvec;

	if ((err = acquire_sem(busses_sem)) < 0)
		return err;

	if ((err = verify_cookie(bus)) != B_OK)
		goto err1;

	if ((err = split_and_lock_buffer(veclen, vec, &nveclen, &nvec, 0)) < B_OK)
		goto err1;

	err = SBlockWrite(bus, speed, node, offset, nveclen, nvec, timeout);

	unlock_and_delete_buffer(veclen, vec, nveclen, nvec, 0);

err1:
	release_sem(busses_sem);
	return err;
}

static status_t
bus_lock_32(void *cookie, uchar speed, struct nodeid *node, uint64 offset,
		uchar extended_tcode, uint32 arg, uint32 data, bigtime_t timeout)
{
	struct _1394_bus *bus = (struct _1394_bus *)cookie;
	status_t err;

	if ((err = acquire_sem(busses_sem)) < 0)
		return err;

	if ((err = verify_cookie(bus)) != B_OK)
		goto err1;

	err = SLock32(bus, speed, node, offset, extended_tcode, arg, data,
			timeout);

err1:
	release_sem(busses_sem);
	return err;
}

static status_t
bus_send_phy_packet(void *cookie, uint32 data, bigtime_t timeout)
{
	struct _1394_bus *bus = (struct _1394_bus *)cookie;
	status_t err;

	if ((err = acquire_sem(busses_sem)) < 0)
		return err;

	if ((err = verify_cookie(bus)) != B_OK)
		goto err1;

	err = SSendPhyPacket(bus, data, timeout);

err1:
	release_sem(busses_sem);
	return err;
}


static struc