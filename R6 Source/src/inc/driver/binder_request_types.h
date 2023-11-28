#ifndef BINDER_REQUEST_TYPES_H
#define BINDER_REQUEST_TYPES_H

#include <kernel/OS.h>
#include <binder_driver.h>

typedef struct binder_request_basic {
	uint32					request_size;
	port_id					reply_port;
	uint32					token;
	binder_request_type		type;
} binder_request_basic;

typedef struct {
	binder_request_basic    basic;
	uint32                  arg;
} binder_request_one_arg;

typedef struct binder_reply_basic {
	port_id                 sync_port;
} binder_reply_basic;

typedef struct {
	binder_reply_basic      basic;
	status_t                result;
} binder_reply_status;

typedef struct {
	binder_reply_basic                basic;
	_get_status_t                     get_result;
} binder_reply_get_property_status;

typedef struct {
	binder_reply_get_property_status  status;
	size_t                            rlen;
} binder_reply_get_property_base;

typedef struct {
	binder_reply_get_property_base    base;
	uint8                             rbuf[1];
} binder_reply_get_property;

typedef struct {
	binder_reply_basic                basic;
	_put_status_t                     put_result;
} binder_reply_put_property;

typedef struct {
	binder_reply_status               status;
	void                             *user_cookie;
} binder_reply_open_open_properties;

typedef struct {
	binder_reply_status               status;
	size_t                            namelen;
} binder_reply_next_entry_base;

typedef struct {
	binder_reply_next_entry_base      base;
	char                              name[1];
} binder_reply_next_entry;

#endif

