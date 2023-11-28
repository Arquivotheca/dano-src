
#ifndef _BINDER_PRIV_H_
#define _BINDER_PRIV_H_

#define BINDER_REQUEST_CODE '_RRQ'
#define BINDER_REPLY_CODE '_RRP'
#define BINDER_SYNC_CODE '_RSY'
#define BINDER_PUNT_CODE '_RPU'

#include <Drivers.h>
#include <Binder.h>

enum {
	REFLECT_CMD_NULL = 0,
	REFLECT_CMD_GET,
	REFLECT_CMD_PUT,
	REFLECT_CMD_ACQUIRE,
	REFLECT_CMD_RELEASE,
	REFLECT_CMD_REDIRECT,
	REFLECT_CMD_UNREDIRECT,
	REFLECT_CMD_PUNT,
	REFLECT_CMD_OPEN_PROPERTIES,
	REFLECT_CMD_NEXT_PROPERTY,
	REFLECT_CMD_CLOSE_PROPERTIES
};

enum {
	BINDER_CMD = B_DEVICE_OP_CODES_END+1,
	BINDER_ASSOCIATE_BUFFER,
	BINDER_ASSOCIATE_TRANSACTION_STATE,
	BINDER_FETCH_TRANSACTION_STATE,
	BINDER_INIT_PROPERTY_ITERATOR,
	BINDER_REGISTER_REPLY_THREAD,
	BINDER_UNREGISTER_REPLY_THREAD_AND_GET_SYNC_PORT,
	BINDER_FREE_SYNC_REPLY_PORT,
	BINDER_UNREGISTER_REPLY_THREAD,
	BINDER_INTERNAL_CONNECT
};

enum {
	BINDER_NOTIFY_CALLBACK = 0,
	BINDER_GET_PROPERTY,
	BINDER_PUT_PROPERTY,
	BINDER_STACK,
	BINDER_INHERIT,
	BINDER_START_HOSTING,
	BINDER_BREAK_LINKS,
	BINDER_CONNECT,
	BINDER_OPEN_PROPERTIES,
	BINDER_NEXT_PROPERTY,
	BINDER_CLOSE_PROPERTIES,
	BINDER_STOP_HOSTING,
	_BINDER_COMMAND_COUNT
};

enum binder_request_type {
	BINDER_REQUEST_MOUNT = 1,
	BINDER_REQUEST_UNMOUNT,
	BINDER_REQUEST_CONNECT,
	BINDER_REQUEST_DISCONNECT,
	
	BINDER_REQUEST_GET_PROPERTY,
	BINDER_REQUEST_PUT_PROPERTY,

	BINDER_REQUEST_OPEN_PROPERTIES,
	BINDER_REQUEST_NEXT_PROPERTY,
	BINDER_REQUEST_CLOSE_PROPERTIES,

	BINDER_REQUEST_REDIRECT
};

struct _put_status_t {
	status_t	error;
	bool		fallthrough		: 1;
	int32		reserved		: 31;

	operator put_status_t() {
		return *((put_status_t*)this);
	}
	
	_put_status_t & operator =(const put_status_t &s) {
		*this = *((_put_status_t*)&s);
		return *this;
	}
};

struct _get_status_t {
	status_t	error;
	bool		resultCacheable	: 1;
	bool		ignoreArgs		: 1;
	int32		reserved		: 30;
	
	operator get_status_t() {
		return *((get_status_t*)this);
	}

	_get_status_t & operator =(const get_status_t &s) {
		*this = *((_get_status_t*)&s);
		return *this;
	}
};

union binder_vnode_id {
	ino_t				value;
	struct {
		port_id			port;
		uint32			token;
	} address;
};

#define MAX_REFLECTION_CMDS 16
struct reflection_struct {
	team_id			team;
	int32			state;
	bool			active;
	int16			count;
	int8			cmd[MAX_REFLECTION_CMDS];
	int32			token[MAX_REFLECTION_CMDS];
	
	status_t add_cmd(int8 c, int32 t) {
		if (count < MAX_REFLECTION_CMDS) {
			active = true;
			cmd[count] = c;
			token[count++] = t;
			return B_OK;
		}
		return B_ERROR;
	}
};

struct get_property_struct {
	char				name[64];
	int32				streamSize;
	int32				returnDescriptor;
	size_t				returnBufSize;
	void *				returnBuf;
	int32				argsSize;
	void				*args;
	_get_status_t		result;
	bool				successful_reflect;
};

struct put_property_struct {
	int32				state;
	char				name[64];
	void				*value;
	int32				valueLen;
	_put_status_t		result;
};

struct notify_callback_struct {
	uint32 		event;
	char 		name[256];
};

struct break_links_struct {
	uint32			flags;
};

struct property_iterator_struct {
	int		fd;
	status_t result;
	void	*local_cookie;
	char	name[PROPERTY_NAME_LEN];
	int32	len;
};

struct binder_cmd {
	int32						command;
	reflection_struct			reflection;
	union {
		get_property_struct		get;
		put_property_struct		put;
		notify_callback_struct	notify;
		break_links_struct		break_links;
		binder_vnode_id			vnid;
		property_iterator_struct property_iterator;
	} data;
};

/* Link flags */
enum {
	linkInherit	= 0x01,
	linkFilter	= 0x02,
	linkAugment	= 0x04,
	linkFrom	= 0x08,
	linkTo		= 0x10,
	linkAll		= 0xFF
};

enum {
	pfRedirect = 0x00000001
};

#endif
