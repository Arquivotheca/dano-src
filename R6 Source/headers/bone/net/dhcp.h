#ifndef __DHCP_H
#define __DHCP_H

#include <bone_ioctl.h>
#include <net/if.h>

//values for ioctl() 'op's
typedef enum {
	DHCP_ON = BONE_DHCP_IOCTL_BASE,
	DHCP_OFF,
	DHCP_RELEASE,
	DHCP_RENEW,
	DHCP_GET_IF_STATE,
	DHCP_GET_HOST_STATE,
	DHCP_RELEASE_ALL,
	DHCP_RENEW_ALL,
	DHCP_SET_VAR,
} dhcp_ioctl_t;

//special values for 'dhcp_ioctl_on_params.priority'
enum {
	DHCP_EXCLUDE = -1,
	DHCP_DEFAULT_PRIORITY = 0,
};

//values for 'dhcp_ioctl_name_value.name'
typedef enum {
	DHCP_LOG_LEVEL,
	DHCP_MAX_OFFERS,
	DHCP_HOSTNAME,
} dhcp_var_name_t;

//values for 'dhcp_ioctl_name_value.value' when setting logging level
typedef enum {
	DHCP_LOG_OFF = 0,
	DHCP_LOG_ON = 1,
	DHCP_LOG_ERROR = 1,
	DHCP_LOG_INFO = 1,
	DHCP_LOG_VERBOSE = 2,
} dhcp_log_level_t;

typedef struct dhcp_ioctl_on_params {
	//a previous address to reboot
	uint32 oldAddr;

	//host-wide param priority
	int priority;

} dhcp_ioctl_on_params_t;

typedef struct dhcp_ioctl_if_state {
	bool enabled;
	uint32 ipAddr;
	uint32 serverAddr;
	uint32 netmask;
	uint32 gateway;
	bigtime_t leaseStart;
	uint32 leaseTime;
	status_t lastTry;
} dhcp_ioctl_if_state_t;

typedef struct dhcp_ioctl_host_state
{
	uint32 gateway;
	char hostname[256];
	int domainOffset;
	uint32 dns1;
	uint32 dns2;
} dhcp_ioctl_host_state_t;

typedef struct dhcp_ioctl_name_value {
	int name;
	union {
		int intValue;
		char *strValue;
	} value;
} dhcp_ioctl_name_value_t;

typedef struct {
	if_index_t index;
	union {
		dhcp_ioctl_on_params_t onParams;
		dhcp_ioctl_if_state_t ifState;
		dhcp_ioctl_host_state_t hostState;
		dhcp_ioctl_name_value_t nameValue;
	} u;
} dhcp_ioctl_data_t;

#endif	/* __DHCP_H */
