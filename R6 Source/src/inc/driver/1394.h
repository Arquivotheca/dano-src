#ifndef _1394_H
#define _1394_H

/* 1394-specific errors */
enum {
	B_1394_BUS_RESET = B_ERRORS_END + 1
};

#define B_1394_MAX_NODES 63
#define B_1394_TOPOLOGY_MAP_QUADLETS 0x400
#define B_1394_SPEED_MAP_BYTES 0x1000

#define B_1394_BUS_SUPPORTS_ASYNCHRONOUS_STREAMS	1
#define B_1394_BUS_SUPPORTS_MULTIPLE_IR_TAGS		2

typedef struct ieee1394_bus_info {
	status_t	bus_status;
	uint32		flags;			/* B_1394_BUS_SUPPORTS_... */
	uint32		generation;
	uint32		nodes;
	uchar		self, isochronous_resource_manager;
	uchar		irports, itports;
	uint64		guids[B_1394_MAX_NODES];
	uint32		topology_map[B_1394_TOPOLOGY_MAP_QUADLETS];
	uchar		speed_map[B_1394_SPEED_MAP_BYTES];
} ieee1394_bus_info;


#define B_1394_MAX_UNITS	64

typedef struct ieee1394_unit_info {
	uint32		spec_id;
	uint32		sw_version;
	uint32		unit_dependent_directory_index;
	uint32		unit_dependent_leaf_index;
} ieee1394_unit_info;

typedef struct ieee1394_node_info {
	uint64 guid;
	uint32 config_rom[0x100];
	uint32 num_units;
	struct ieee1394_unit_info units[B_1394_MAX_UNITS];
} ieee1394_node_info;

/* CSR map notification events */
#define B_1394_MAP_READ			1
#define B_1394_MAP_WRITE		2
#define B_1394_MAP_LOCK			4
#define B_1394_MAP_FREE			0x80000000

#define B_1394_MAP_READ_WRITE	(B_1394_MAP_READ | B_1394_MAP_WRITE)
#define B_1394_MAP_READ_LOCK	(B_1394_MAP_READ | B_1394_MAP_LOCK)
#define B_1394_MAP_READ_WRITE_LOCK	(B_1394_MAP_READ_WRITE | B_1394_MAP_LOCK)

typedef uint32 (*_1394_map_notify)(
		void *cookie, uint64 guid, uint32 event, uint32 extended_tcode,
		uint64 start, uint32 len, uint64 segstart, uint32 seglen,
		void *mem1, void *mem2);

#define B_1394_SPEED_100_MBITS	0
#define B_1394_SPEED_200_MBITS	1
#define B_1394_SPEED_400_MBITS	2
#define B_1394_SPEED_MAX_MBITS	0xff

typedef struct ieee1394_io {
	uchar speed;
	uint64 guid;
	uint64 offset;
	union {
		struct {
			int32 veclen;
			struct iovec *vec;
		} block;
		uint32 quadlet;
		struct {
			uchar extended_tcode;
			uint64 arg;
			uint64 data;
		} lock;
		uint32 phy_packet;
	} x;
	bigtime_t timeout;
} ieee1394_io;

#define B_1394_ISOC_READ_PORT			0
#define B_1394_ISOC_WRITE_PORT			1

typedef struct ieee1394_iio_hdr {
	status_t	status;					/* bytes transferred or error */
	uint64		buffer_num;
} ieee1394_iio_hdr;

#define B_1394_ISOC_EVENT_NOW			0
#define B_1394_ISOC_EVENT_NEVER			1
#define B_1394_ISOC_EVENT_SYNC			2

typedef struct ieee1394_ievent {
	uchar		type;
	uint64		cookie;
} ieee1394_ievent;

#define B_1394_ISOCHRONOUS_TAG_00				1
#define B_1394_ISOCHRONOUS_TAG_01				2
#define B_1394_ISOCHRONOUS_TAG_10				4
#define B_1394_ISOCHRONOUS_TAG_11				8

typedef struct ieee1394_iio_buffer {
	struct ieee1394_iio_hdr		*hdr;		/* must point to locked memory */
	int32						veclen;
	struct iovec				*vec;		/* ptrs must be to locked memory */
	uchar						tagmask;
	uchar						sync;		/* for isochronous write only */

	sem_id 						sem;		/* notification semaphore */

	struct ieee1394_ievent		start_event;
} ieee1394_iio_buffer;

#define B_1394_ISOCHRONOUS_SINGLE_SHOT			1
#define B_1394_ISOCHRONOUS_CONCATENATE_PACKETS	2
#define B_1394_ISOCHRONOUS_FILL_CYCLE_TIME		4

typedef struct ieee1394_iio {
	uint32 port;
	uint32 flags;

	uchar channel;
	uchar speed;			/* for writes only */

	int32 num_buffers;
	struct ieee1394_iio_buffer *buffers;
} ieee1394_iio;

typedef struct ieee1394_istop {
	uint32 port;
	struct ieee1394_ievent event;
} ieee1394_istop;

/* PRIVATE */

#include <drivers/bus_manager.h>

struct nodeid {
	enum { NODEID_GUID, NODEID_ID } type;
	union {
		uint64  guid;
		struct {
			uint32 generation;
			uchar node;
		} id;
	} x;
};

typedef struct _1394_bus_functions {
	status_t	(*get_bus_info)(void *cookie, struct ieee1394_bus_info *info);
	status_t	(*reset_bus)(void *cookie, bool synchronous);

	status_t	(*map_addresses)(void *cookie, uint32 events,
						uint64 start, uint32 len,
						_1394_map_notify notify, void *notify_cookie);
	status_t	(*unmap_addresses)(void *cookie, uint32 events,
						uint64 start, uint32 len,
						_1394_map_notify notify, void *notify_cookie);

	status_t	(*aquadlet_read)(void *cookie,
						uchar speed, struct nodeid *target, uint64 offset,
						uint32 *data, 
						void (*notify)(void *, status_t), void *notify_cookie,
						uint32 *uniqueid);
	status_t	(*ablock_read)(void *cookie,
						uchar speed, struct nodeid *target, uint64 offset,
						int32 veclen, struct iovec *vec,
						void (*notify)(void *, status_t), void *notify_cookie,
						uint32 *uniqueid);
	status_t	(*aquadlet_write)(void *cookie,
						uchar speed, struct nodeid *target, uint64 offset,
						uint32 quadlet,
						void (*notify)(void *, status_t), void *notify_cookie,
						uint32 *uniqueid);
	status_t	(*ablock_write)(void *cookie,
						uchar speed, struct nodeid *target, uint64 offset,
						int32 veclen, struct iovec *vec,
						void (*notify)(void *, status_t), void *notify_cookie,
						uint32 *uniqueid);
	status_t	(*alock_32)(void *cookie,
						uchar speed, struct nodeid *target, uint64 offset,
						uchar extended_tcode, uint32 arg, uint32 data,
						void (*notify)(void *, status_t), void *notify_cookie,
						uint32 *uniqueid);
	status_t	(*asend_phy_packet)(void *cookie, uint32 data, 
						void (*notify)(void *, status_t), void *notify_cookie,
						uint32 *uniqueid);

	status_t	(*dequeue_atask)(void *cookie,
						uint32 uniqueid, status_t errorcode);

	status_t	(*get_cycle_time)(void *cookie, uint32 *time);

	status_t	(*queue_isochronous)(void *cookie, uint32 port,
						uint32 flags, uint32 *data, uchar channel, uchar speed,
						int32 num_buffers, struct ieee1394_iio_buffer *buffers);
	status_t	(*stop_isochronous)(void *cookie, struct ieee1394_istop *istop);
} _1394_bus_module_info;

#define B_1394_BUS_MODULE_PREFIX "busses/1394/"

typedef struct _1394_module_info {
	struct bus_manager_info		binfo;

	status_t	(*get_bus_count)(void);
	status_t	(*get_bus_cookie)(int bus, void **cookie);

	status_t	(*get_bus_info)(void *cookie, struct ieee1394_bus_info *info);
	status_t	(*get_node_info)(void *cookie, uint64 guid,
						struct ieee1394_node_info *info);
	status_t	(*reset_bus)(void *cookie, bool synchronous);

	status_t	(*map_addresses)(void *cookie, uint32 events,
						uint64 start, uint32 len,
						_1394_map_notify notify, void *notify_cookie);
	status_t	(*unmap_addresses)(void *cookie, uint32 events,
						uint64 start, uint32 len,
						_1394_map_notify notify, void *notify_cookie);

	status_t	(*register_bus_reset_notification)(void *cookie, sem_id sem);
	status_t	(*unregister_bus_reset_notification)(void *cookie, sem_id sem);

	status_t	(*acquire_guid)(void *cookie, uint64 guid);
	status_t	(*release_guid)(void *cookie, uint64 guid);

	status_t	(*quadlet_read)(void *cookie,
						uchar speed, uint64 guid, uint64 offset,
						uint32 *data, bigtime_t timeout);
	status_t	(*block_read)(void *cookie,
						uchar speed, uint64 guid, uint64 offset,
						int32 veclen, struct iovec *vec, bigtime_t timeout);
	status_t	(*quadlet_write)(void *cookie,
						uchar speed, uint64 guid, uint64 offset,
						uint32 quadlet, bigtime_t timeout);
	status_t	(*block_write)(void *cookie,
						uchar speed, uint64 guid, uint64 offset,
						int32 veclen, struct iovec *vec, bigtime_t timeout);
	status_t	(*lock_32)(void *cookie,
						uchar speed, uint64 guid, uint64 offset,
						uchar extended_tcode, uint32 arg, uint32 data,
						bigtime_t timeout);
	status_t	(*send_phy_packet)(void *cookie,
						uint32 data, bigtime_t timeout);

	status_t	(*init_fcp_context)(void *cookie, uint64 guid);
	status_t	(*uninit_fcp_context)(void *cookie, uint64 guid);
	status_t	(*send_fcp_command)(void *cookie,
						uint64 guid, const void *cmd, size_t cmd_len,
						void *rsp, size_t rsp_len, bigtime_t timeout);
	status_t	(*wait_for_fcp_response)(void *cookie, uint64 guid,
						void *rsp, size_t rsp_len, bigtime_t timeout);

	status_t	(*get_cycle_time)(void *cookie, uint32 *time);
	status_t	(*acquire_isochronous_port)(void *cookie, uint32 type);
	status_t	(*release_isochronous_port)(void *cookie, uint32 port);
	status_t	(*reserve_bandwidth)(void *cookie, uint32 units, bigtime_t timeout);
	status_t	(*reserve_isochronous_channel)(void *cookie,
						uint32 channel, bigtime_t timeout);

	status_t	(*queue_isochronous)(void *cookie, uint32 port,
						uint32 flags, uint32 *data, uchar channel, uchar speed,
						int32 num_buffers, struct ieee1394_iio_buffer *buffers);
	status_t	(*stop_isochronous)(void *cookie, struct ieee1394_istop *istop);

} _1394_module_info;

#define B_1394_FOR_DRIVER_MODULE_NAME "bus_managers/1394/driver/v1"

typedef struct _1394_for_bus_module_info {
	struct module_info minfo;

	status_t	(*register_bus)(struct _1394_bus_functions *module,
						void *bus_cookie, void **bus_manager_cookie);
	status_t	(*unregister_bus)(void *bus_manager_cookie);

	status_t	(*notify_bus_reset)(void *bus_manager_cookie);

	uint16		(*CRC16)(uint32 *buffer, uint32 quadlets);

	status_t	(*quadlet_read)(void *cookie,
						uchar speed, struct nodeid *node, uint64 offset,
						uint32 *data, bigtime_t timeout);
	status_t	(*block_read)(void *cookie,
						uchar speed, struct nodeid *node, uint64 offset,
						int32 veclen, struct iovec *vec, bigtime_t timeout);
	status_t	(*quadlet_write)(void *cookie,
						uchar speed, struct nodeid *node, uint64 offset,
						uint32 quadlet, bigtime_t timeout);
	status_t	(*block_write)(void *cookie,
						uchar speed, struct nodeid *node, uint64 offset,
						int32 veclen, struct iovec *vec, bigtime_t timeout);
	status_t	(*lock_32)(void *cookie,
						uchar speed, struct nodeid *node, uint64 offset,
						uchar extended_tcode, uint32 arg, uint32 data,
						bigtime_t timeout);
	status_t	(*send_phy_packet)(void *cookie,
						uint32 data, bigtime_t timeout);
} _1394_for_bus_module_info;

#define B_1394_FOR_BUS_MODULE_NAME "bus_managers/1394/bus/v1"

/* ioctls */
enum {
	B_1394_GET_BUS_INFO = '1394',
	B_1394_GET_NODE_INFO,
	B_1394_RESET_BUS,
	B_1394_REGISTER_BUS_RESET_NOTIFICATION,
	B_1394_UNREGISTER_BUS_RESET_NOTIFICATION,
	B_1394_ACQUIRE_GUID,
	B_1394_RELEASE_GUID,

	B_1394_QUADLET_READ,
	B_1394_BLOCK_READ,
	B_1394_QUADLET_WRITE,
	B_1394_BLOCK_WRITE,
	B_1394_LOCK32,
	B_1394_LOCK64,
	B_1394_SEND_PHY_PACKET,

	B_1394_INIT_FCP_CONTEXT,
	B_1394_UNINIT_FCP_CONTEXT,
	B_1394_SEND_FCP_COMMAND,
	B_1394_WAIT_FOR_FCP_RESPONSE,

	B_1394_GET_CYCLE_TIME,
	B_1394_ACQUIRE_ISOCHRONOUS_PORT,
	B_1394_RELEASE_ISOCHRONOUS_PORT,
	B_1394_RESERVE_BANDWIDTH,
	B_1394_RESERVE_CHANNEL,
	B_1394_QUEUE_ISOCHRONOUS,
	B_1394_STOP_ISOCHRONOUS,
	B_1394_MALLOC_LOCKED,
	B_1394_FREE_LOCKED,
	B_1394_DV_WRITE
};

typedef struct ieee1394_area {
	uint32 size;
	void *addr;
	area_id aid;
	uint32 flags;
} ieee1394_area;

typedef struct ieee1394_fcp {
	uint64		guid;
	const void	*cmd;
	size_t		cmd_len;
	void		*rsp;
	size_t		rsp_len;
	bigtime_t	timeout;
} ieee1394_fcp;

typedef struct ieee1394_dvwrite {
	uint32		port;
	uchar		*buffer;
	uint32		*headers;
	sem_id		sem;
	uint32		pal;
	struct		{
		uchar		seq;		/* zero these initially, then use as cookie */
		uint32		cycle_time;
		uint32		fraction;
	} state;
} ieee1394_dvwrite;

#endif
