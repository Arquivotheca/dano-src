/* ieee1394.c
 *
 * Access to the 1394 bus.  Just a wrapper around the driver.
 *
 */

#include <SupportDefs.h>
#include <OS.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>

#include <1394.h>

#include "ieee1394.h"

/* temporary stuff to isolate opening and closing the driver for
 * when we will probably have a different way of doing this
 */
int
open_1394(int bus)
{
	int fd;
	char name[64];

	sprintf(name, "/dev/bus/1394/%d/raw", bus);

	fd = open(name, O_RDWR);
	if (fd < 0)
		return errno;
	
	return fd;
}

void
close_1394(int fd)
{
	close(fd);
}

status_t get_1394_bus_info(int fd,
		struct ieee1394_bus_info *b)
{
	return ioctl(fd, B_1394_GET_BUS_INFO, b, sizeof(*b));
}

status_t get_1394_node_info(int fd,
		uint64 guid, struct ieee1394_node_info *n)
{
	n->guid = guid;

	return ioctl(fd, B_1394_GET_NODE_INFO, n, sizeof(*n));
}

status_t reset_1394_bus(int fd)
{
	return ioctl(fd, B_1394_RESET_BUS, NULL, 0);
}

status_t register_1394_bus_reset_notification(
		int fd, sem_id sem)
{
	return ioctl(fd, B_1394_REGISTER_BUS_RESET_NOTIFICATION, &sem, sizeof(sem));
}

status_t unregister_1394_bus_reset_notification(
		int fd, sem_id sem)
{
	return ioctl(
			fd, B_1394_UNREGISTER_BUS_RESET_NOTIFICATION, &sem, sizeof(sem));
}

status_t quadlet_read(int fd,
		uint64 guid, uint64 offset, uint32 *quadlet, bigtime_t timeout)
{
	struct ieee1394_io io;
	status_t err;

	io.speed = B_1394_SPEED_MAX_MBITS;
	io.guid = guid;
	io.offset = offset;
	io.timeout = timeout;

	if (!(err = ioctl(fd, B_1394_QUADLET_READ, &io, sizeof(io))))
		*quadlet = io.x.quadlet;

	return err;
}

status_t quadlet_read_retry(int fd,
		uint64 guid, uint64 offset, uint32 *quadlet, bigtime_t timeout,
		uint32 retries)
{
	status_t err = EINVAL;
	uint32 i;

	for (i=0;i<retries;i++) {
		if ((err = quadlet_read(fd, guid, offset, quadlet, timeout)) >= B_OK)
			break;
		snooze(10000);
	}

	return err;
}

status_t block_read(int fd,
		uint64 guid, uint64 offset, void *buffer, uint32 length,
		bigtime_t timeout)
{
	struct ieee1394_io io;
	struct iovec iov;

	iov.iov_base = buffer;
	iov.iov_len = length;

	io.speed = B_1394_SPEED_MAX_MBITS;
	io.guid = guid;
	io.offset = offset;
	io.x.block.veclen = 1;
	io.x.block.vec = &iov;
	io.timeout = timeout;

	return ioctl(fd, B_1394_BLOCK_READ, &io, sizeof(io));
}

status_t block_read_retry(int fd,
		uint64 guid, uint64 offset, void *buffer, uint32 length,
		bigtime_t timeout, uint32 retries)
{
	status_t err = EINVAL;
	uint32 i;

	for (i=0;i<retries;i++) {
		if ((err = block_read(fd, guid, offset, buffer, length, timeout)) >= B_OK)
			break;
		snooze(10000);
	}

	return err;
}

status_t quadlet_write(int fd,
		uint64 guid, uint64 offset, uint32 quadlet, bigtime_t timeout)
{
	struct ieee1394_io io;

	io.speed = B_1394_SPEED_MAX_MBITS;
	io.guid = guid;
	io.offset = offset;
	io.x.quadlet = quadlet;
	io.timeout = timeout;

	return ioctl(fd, B_1394_QUADLET_WRITE, &io, sizeof(io));
}

status_t quadlet_write_retry(int fd,
		uint64 guid, uint64 offset, uint32 quadlet, bigtime_t timeout,
		uint32 retries)
{
	status_t err = EINVAL;
	uint32 i;

	for (i=0;i<retries;i++) {
		if ((err = quadlet_write(fd, guid, offset, quadlet, timeout)) >= B_OK)
			break;
		snooze(10000);
	}

	return err;
}

status_t block_write(int fd,
		uint64 guid, uint64 offset, void *buffer, uint32 length,
		bigtime_t timeout)
{
	struct ieee1394_io io;
	struct iovec iov;

	iov.iov_base = buffer;
	iov.iov_len = length;

	io.speed = B_1394_SPEED_MAX_MBITS;
	io.guid = guid;
	io.offset = offset;
	io.x.block.veclen = 1;
	io.x.block.vec = &iov;
	io.timeout = timeout;

	return ioctl(fd, B_1394_BLOCK_WRITE, &io, sizeof(io));
}

status_t block_write_retry(int fd,
		uint64 guid, uint64 offset, void *buffer, uint32 length,
		bigtime_t timeout, uint32 retries)
{
	status_t err = EINVAL;
	uint32 i;

	for (i=0;i<retries;i++) {
		if ((err = block_write(fd, guid, offset, buffer, length, timeout)) >= B_OK)
			break;
		snooze(10000);
	}

	return err;
}

status_t compare_swap_32(int fd,
		uint64 guid, uint64 offset, uint32 oval, uint32 nval,
		bigtime_t timeout)
{
	struct ieee1394_io io;

	io.speed = B_1394_SPEED_MAX_MBITS;
	io.guid = guid;
	io.offset = offset;
	io.x.lock.extended_tcode = 2;
	io.x.lock.arg = oval;
	io.x.lock.data = nval;
	io.timeout = timeout;

	return ioctl(fd, B_1394_LOCK32, &io, sizeof(io));
}

status_t compare_swap_32_retry(int fd,
		uint64 guid, uint64 offset, uint32 oval, uint32 nval,
		bigtime_t timeout, uint32 retries)
{
	status_t err = EINVAL;
	uint32 i;

	for (i=0;i<retries;i++) {
		if ((err = compare_swap_32(fd, guid, offset, oval, nval, timeout)) >= B_OK)
			break;
		snooze(10000);
	}

	return err;
}

status_t send_phy_packet(int fd, uint32 phy_packet, bigtime_t timeout)
{
	struct ieee1394_io io;

	io.x.phy_packet = phy_packet;
	io.timeout = timeout;

	return ioctl(fd, B_1394_SEND_PHY_PACKET, &io, sizeof(io));
}

status_t
send_fcp_command(
		int fd,
		uint64 guid,
		const void *cmd, size_t cmd_len,
		void *rsp, size_t rsp_len,
		bigtime_t timeout)
{
	struct ieee1394_fcp fcp;
	status_t err;

	fcp.guid = guid;
	fcp.cmd = cmd;
	fcp.cmd_len = cmd_len;
	fcp.rsp = rsp;
	fcp.rsp_len = rsp_len;
	fcp.timeout = timeout;

	err = ioctl(fd, B_1394_INIT_FCP_CONTEXT, &fcp, sizeof(fcp));
	if (err < B_OK)
		return err;

	err = ioctl(fd, B_1394_SEND_FCP_COMMAND, &fcp, sizeof(fcp));

	ioctl(fd, B_1394_UNINIT_FCP_CONTEXT, &fcp, sizeof(fcp));

	return err;
}

status_t acquire_isochronous_port(int fd, uint32 type)
{
	status_t error;
	error = ioctl(fd, B_1394_ACQUIRE_ISOCHRONOUS_PORT, &type, sizeof(type));
	if (error < 0)
		return error;
	return type;
}

status_t release_isochronous_port(int fd, int32 port)
{
	return ioctl(fd, B_1394_RELEASE_ISOCHRONOUS_PORT, &port, sizeof(port));
}

void *malloc_locked(int fd, struct ieee1394_area *area, uint32 size, uint32 flags)
{
	area->size = size;
	area->flags = flags;
	if (ioctl(fd, B_1394_MALLOC_LOCKED, area, sizeof(*area)) < 0)
		return NULL;
	return area->addr;
}

status_t free_locked(int fd, struct ieee1394_area *area)
{
	return ioctl(fd, B_1394_FREE_LOCKED, area, sizeof(*area));
}
