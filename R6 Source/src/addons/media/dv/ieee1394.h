/* ieee1394.h
 *
 * Access to the 1394 bus.
 *
 */

#ifndef IEEE1394_H
#define IEEE1394_H

#include <SupportDefs.h>
#include <OS.h>
#include "1394.h" // XXX

#ifdef __cplusplus
extern "C" {
#endif

/* temporary functions to isolate opening the driver */
int open_1394(int bus);
void close_1394(int fd);

/*
 * gets information about the bus; generation, GUID -> node
 * mapping, et al
 */

status_t get_1394_bus_info(
			int fd,								// fd of 1394 driver
			struct ieee1394_bus_info *info);	// structure to fill in

/*
 * gets information about the node; configuration rom only for now
 */

status_t get_1394_node_info(
			int fd,								// fd of 1394 driver
			uint64 guid,						// GUID of the node
			struct ieee1394_node_info *info);	// structure to fill in

status_t reset_1394_bus(
			int fd);							// fd of 1394 driver

/*
 * 1394 bus reset notification
 */

status_t register_1394_bus_reset_notification(
			int fd,
			sem_id sem);

status_t unregister_1394_bus_reset_notification(
			int fd,
			sem_id sem);

/*
 * Bus I/O
 */
status_t quadlet_read(
			int fd,						// fd of 1394 driver
			uint64 guid,				// guid of device on the bus
			uint64 offset,				// address to read from
			uint32 *quadlet,			// buffer for the quadlet
			bigtime_t timeout);			// timeout in microseconds

status_t quadlet_read_retry(
			int fd,						// fd of 1394 driver
			uint64 guid,				// guid of device on the bus
			uint64 offset,				// address to read from
			uint32 *quadlet,			// buffer for the quadlet
			bigtime_t timeout,			// timeout in microseconds per retry
			uint32 retries);			// number of retries

status_t block_read(
			int fd,						// fd of 1394 driver
			uint64 guid,				// guid of device on the bus
			uint64 offset,				// address to read from
			void *buffer,				// buffer for the returned data
			uint32 length,				// how much data to read
			bigtime_t timeout);			// timeout in microseconds

status_t block_read_retry(
			int fd,						// fd of 1394 driver
			uint64 guid,				// guid of device on the bus
			uint64 offset,				// address to read from
			void *buffer,				// buffer for the returned data
			uint32 length,				// how much data to read
			bigtime_t timeout,			// timeout in microseconds
			uint32 retries);			// number of retries

status_t quadlet_write(
			int fd,						// fd of 1394 driver
			uint64 guid,				// guid of device on the bus
			uint64 offset,				// address to write to
			uint32 quadlet,				// quadlet to write
			bigtime_t timeout);			// timeout in microseconds

status_t quadlet_write_retry(
			int fd,						// fd of 1394 driver
			uint64 guid,				// guid of device on the bus
			uint64 offset,				// address to write to
			uint32 quadlet,				// quadlet to write
			bigtime_t timeout,			// timeout in microseconds per retry
			uint32 retries);			// number of retries

status_t block_write(
			int fd,						// fd of 1394 driver
			uint64 guid,				// guid of device on the bus
			uint64 offset,				// address to write to
			void *buffer,				// buffer to write
			uint32 length,				// how much data to write
			bigtime_t timeout);			// timeout in microseconds

status_t block_write_retry(
			int fd,						// fd of 1394 driver
			uint64 guid,				// guid of device on the bus
			uint64 offset,				// address to write to
			void *buffer,				// buffer to write
			uint32 length,				// how much data to write
			bigtime_t timeout,			// timeout in microseconds
			uint32 retries);			// number of retries

status_t compare_swap_32(
			int fd,						// fd of 1394 driver
			uint64 guid,				// guid of device on the bus
			uint64 offset,				// address to write to
			uint32 old_value,			// if equals old_value,
			uint32 new_value,			// replace with new_value
			bigtime_t timeout);			// timeout in microseconds

status_t compare_swap_32_retry(
			int fd,						// fd of 1394 driver
			uint64 guid,				// guid of device on the bus
			uint64 offset,				// address to write to
			uint32 old_value,			// if equals old_value,
			uint32 new_value,			// replace with new_value
			bigtime_t timeout,			// timeout in microseconds
			uint32 retries);			// number of retries

status_t send_phy_packet(
			int fd,						// fd of 1394 driver
			uint32 phy_packet,			// packet data
			bigtime_t timeout);			// timeout in microseconds

#define FCP_PAYLOAD_MAX             512

status_t send_fcp_command(
			int fd,						// fd of 1394 driver
			uint64 guid,				// guid of device on the bus
			const void *command,		// command data
			size_t command_length,		// size of command data
			void *response,				// response data buffer
			size_t response_length,		// size of response data buffer
			bigtime_t timeout);			// timeout in microseconds

status_t acquire_isochronous_port(int fd, uint32 type);
status_t release_isochronous_port(int fd, int32 port);

/* XXX these should be taken out of the 1394 driver and done somewhere else */
void *malloc_locked(int fd, struct ieee1394_area *area, uint32 size, uint32 flags);
status_t free_locked(int fd, struct ieee1394_area *area);

#ifdef __cplusplus
}
#endif

#endif /* IEEE1394_H */
