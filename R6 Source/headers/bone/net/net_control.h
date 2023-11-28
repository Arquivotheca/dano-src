/*
	net_control.h
	
	API for routing, interface, ... control helper functions
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_NET_CONTROL
#define H_BONE_NET_CONTROL

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/param.h> 
#include <sys/ioctl.h> 
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h> 
#include <net/if_types.h>
#include <netinet/in.h>
#include <net/route.h> 
#include <ByteOrder.h>


/* Interface control and info */

status_t get_interface_configuration(struct ifconf *ifc);
status_t get_interface_by_index(uint32 if_index, ifreq_t *ifr);
status_t get_interface_by_name(const char *if_name, ifreq_t *ifr);
status_t get_interface_by_route(const struct sockaddr *dst, ifreq_t *ifr);
void free_interface_configuration(struct ifconf *ifc);

status_t open_iface_control_socket(void);
void close_iface_control_socket(int s);
status_t set_iface_flag_by_name(const char *iface_name, int flag);
status_t clr_iface_flag_by_name(const char *iface_name, int flag);
status_t set_iface_flag_by_number(if_index_t if_index, int flag);
status_t clr_iface_flag_by_number(if_index_t if_index, int flag);
status_t get_iface_count(void);


/* Routing control and info */
int	open_route_control_socket(void);
void close_route_control_socket(int s);

/* get_route_table returns null-terminated linked list of route entries
	for the given family.  Must be released by free(). */
status_t get_route_table(route_table_req_t *table);
status_t add_route(route_req_t *rt_req);
status_t del_route(route_req_t *rt_req);


#ifdef __cplusplus
}
#endif

#endif /* H_BONE_NET_CONTROL */
