#ifndef BINDER_DRIVER_H
#define BINDER_DRIVER_H

#include <SupportDefs.h>
#include <kernel/OS.h>
#include <drivers/Drivers.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BINDER_REQUEST_CODE '_RRQ'
#define BINDER_REPLY_CODE '_RRP'
#define BINDER_SYNC_CODE '_RSY'
#define BINDER_PUNT_CODE '_RPU'

typedef enum binder_request_type {
	BINDER_REQUEST_CONNECT,
	BINDER_REQUEST_DISCONNECT,
	
	BINDER_REQUEST_GET_PROPERTY,
	BINDER_REQUEST_PUT_PROPERTY,

	BINDER_REQUEST_OPEN_PROPERTIES,
	BINDER_REQUEST_NEXT_PROPERTY,
	BINDER_REQUEST_CLOSE_PROPERTIES,

	BINDER_REQUEST_REDIRECT,
	BINDER_REQUEST_NOTIFY
} binder_request_type;

typedef struct binder_node_id {
	port_id   port;
	uint32    token;
} binder_node_id;

typedef struct _put_status_t {
	status_t    error;
	int         fallthrough      : 1;
	int         reserved         : 31;

#ifndef LIBBE2
#ifdef __cplusplus
	operator put_status_t() {
		return *((put_status_t*)this);
	}
	
	_put_status_t & operator =(const put_status_t &s) {
		*this = *((_put_status_t*)&s);
		return *this;
	}
#endif
#endif
} _put_status_t;

typedef struct _get_status_t {
	status_t    error;
	int         resultCacheable  : 1;
	int         ignoreArgs       : 1;
	int         reserved         : 30;

#ifndef LIBBE2
#ifdef __cplusplus
	operator get_status_t() {
		return *((get_status_t*)this);
	}

	_get_status_t & operator =(const get_status_t &s) {
		*this = *((_get_status_t*)&s);
		return *this;
	}
#endif
#endif
} _get_status_t;

#define CURRENT_BINDER_API_VERSION 1

enum {
	BINDER_CMD = B_DEVICE_OP_CODES_END+1,
	BINDER_GET_API_VERSION,
	BINDER_REGISTER_REPLY_THREAD,
	BINDER_UNREGISTER_REPLY_THREAD_AND_GET_SYNC_PORT,
	BINDER_FREE_SYNC_REPLY_PORT,
	BINDER_UNREGISTER_REPLY_THREAD,
	BINDER_WAIT_FOR_ROOT_NODE
};

typedef enum {
	BINDER_SET_ROOT_NODE_ID,
	BINDER_GET_ROOT_NODE_ID,
	BINDER_GET_NODE_ID,
	BINDER_START_HOSTING,
	BINDER_STOP_HOSTING,
	BINDER_CONNECT,
	BINDER_DISCONNECT,
	BINDER_STACK,
	BINDER_INHERIT,
	BINDER_BREAK_LINKS,
	BINDER_NOTIFY_CALLBACK,
	BINDER_GET_PROPERTY,
	BINDER_PUT_PROPERTY,
	BINDER_OPEN_PROPERTIES,
	BINDER_NEXT_PROPERTY,
	BINDER_CLOSE_PROPERTIES,
	BINDER_START_WATCHING,
	BINDER_STOP_WATCHING,
	_BINDER_COMMAND_COUNT
} binder_cmd_type;

typedef enum {
	REFLECT_CMD_NULL,
	REFLECT_CMD_PUNT,
	REFLECT_CMD_ACQUIRE,
	REFLECT_CMD_RELEASE,
	REFLECT_CMD_REDIRECT,
	REFLECT_CMD_UNREDIRECT,
	REFLECT_CMD_GET,
	REFLECT_CMD_REALLOCATE_GET_BUFFER,
	REFLECT_CMD_PUT,
	REFLECT_CMD_OPEN_PROPERTIES,
	REFLECT_CMD_NEXT_PROPERTY,
	REFLECT_CMD_CLOSE_PROPERTIES,
	REFLECT_CMD_NOTIFY
} reflection_cmd;

/* Link flags */
enum {
	linkInherit	= 0x01,
	linkFilter	= 0x02,
	linkAugment	= 0x04,
	linkFrom	= 0x08,
	linkTo		= 0x10,
	linkAll		= 0xFF
};

typedef struct reflection_struct {
	bool            active;
	team_id         team;
	uint32          cookie;
	reflection_cmd  command;
	int32           token;
	status_t        reflection_result;
} reflection_struct;

status_t add_reflection_command(reflection_struct *reflection,
                                reflection_cmd command, int32 token);

typedef struct {
	binder_node_id      node_id;
	uint32              source_node_handle;
	uint32              return_node_handle;
} binder_connect_cmd;

typedef struct {
	char				name[64];
	uint32				return_node_handle;
	size_t				returnBufSize;
	void *				returnBuf;
	int32				argsSize;
	void				*args;
	_get_status_t		result;
	bool				successful_reflect;
} get_property_struct;

//typedef struct {
//	uint32              cookie;
//	void               *buffer;
//	size_t              buffer_size;
//} get_property_data_struct;

typedef struct {
	int32				state;
	char				name[64];
	void				*value;
	int32				valueLen;
	_put_status_t		result;
} put_property_struct;

typedef struct {
	uint32 		event;
	char 		name[256];
} notify_callback_struct;

typedef struct {
	uint32			flags;
} break_links_struct;

typedef struct {
	uint32        cookie;
	status_t      result;
	void         *local_cookie;
	char          name[128];
	int32         len;
} property_iterator_struct;

typedef struct binder_cmd {
	uint32                         node_handle;
	binder_cmd_type                command;
	reflection_struct              reflection;
	union {
		binder_connect_cmd         connect;
		get_property_struct        get;
		//get_property_data_struct   getdata;
		put_property_struct        put;
		notify_callback_struct     notify;
		break_links_struct         break_links;
		binder_node_id             nodeid;
		uint32                     node_handle;
		property_iterator_struct   property_iterator;
	} data;
} binder_cmd;


#ifdef __cplusplus
}
#endif

#endif

