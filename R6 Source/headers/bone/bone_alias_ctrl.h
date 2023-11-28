/*
	bone_alias_ctrl.h
	
	constants/structures used to configure ip aliasing
	
	Copyright 2001, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_ALIAS_CTRL
#define H_BONE_ALIAS_CTRL

#include <bone_ioctl.h>
#include <support/SupportDefs.h>
#include <net/if.h>

enum {
	BONE_ALIAS_SET_DEVICE = BONE_ALIAS_IOCTL_BASE, // ba_device_t
	BONE_ALIAS_GET_DEVICE, // ba_device_t
	BONE_ALIAS_SET_ENABLED,	// int
	BONE_ALIAS_GET_ENABLED,	// int
	BONE_ALIAS_ADD_REDIRECT_PROTO, // ba_redirect_t
	BONE_ALIAS_GET_REDIRECT_PROTOS, // ba_redirect_t[n]
	BONE_ALIAS_DEL_REDIRECT_PROTO, // ba_redirect_t
	BONE_ALIAS_DEL_REDIRECT_PROTOS, // void
	BONE_ALIAS_PROXY_RULE, // ba_proxy
};

typedef struct ba_device
{
	char 		if_name[IFNAMSIZ];
	status_t	status;
} ba_device_t;

typedef struct ba_redirect
{
	uint32			id;
	
	struct in_addr 	local_addr;
	u_short 		local_port;
	
	struct in_addr 	remote_addr;
	u_short 		remote_port;
	
	struct in_addr 	alias_addr;
	u_short 		alias_port;
	u_char 			alias_proto;
} ba_redirect_t;

typedef struct ba_proxy
{
	char proxy_rule[1024];
	int status;
} ba_proxy_t;

#endif /* H_BONE_ALIAS_CTRL */
