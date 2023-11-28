/*
	bone_datalink.h
	
	networking datalink module
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_DATALINK
#define H_BONE_DATALINK

#include <OS.h>
#include <SupportDefs.h>
#include <module.h>
#include <bone_util.h>
#include <bone_dl_proto.h>
#include <bone_dl_route.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct domain_info
{
	/*
	 * filled out by the protocol module
	 */
	const char				*name;				/* e.g. "internet", "appletalk", etc. */
	const char				*module;			/* e.g. "network/bone_ipv4" */
	uint16					e_type;				/* e.g. 0x800 for inet, etc. See net/if_types.h */ 
	int    					family;       	    /* e.g. AF_INET */

	status_t				(*receive_data)(bone_data_t *data);	/* called to deliver an incoming packet to the domain */

	bone_routing_info_t		*routing_info;		/* datalink routing info - see bone_dl_route.h */


	/*
	 * filled out by the datalink (but for use by protocol modules)
	 */

	/* protects domain-specific info (routes, logical interfaces) */
	bone_rwlock_t			lock;

	/* list of interface protocol addresses for this address family */
	struct ifaddr *our_addrs;


	/*
	 * for datalink use only
	 */
	int32 log_index;

	route_t *routes;
	bone_benaphore_t routes_lock;

	struct domain_info *next;

} domain_t;


typedef struct bone_datalink_info
{
	struct module_info info;                                             
	 
	/*
	 * register/unregister an address family
	 */
	status_t			(*register_domain)(domain_t *domain);
	status_t			(*unregister_domain)(domain_t *domain);
	
	/*
	 * datalink routing table support
	 */
	status_t			(*add_route)(struct bone_routing_info *rt_info,
									 const struct rtentry *new_route);
	status_t			(*del_route)(struct bone_routing_info *rt_info,
									 const struct rtentry *route_desc);
	rtentry_t			*(*get_route)(struct bone_routing_info *rt_info,
									  const struct sockaddr *dst);
	rtentry_t			*(*find_route)(struct bone_routing_info *rt_info,
									   const struct rtentry *route_desc);

	/*
	 * route_t management
	 */
	status_t			(*register_route)(domain_t *domain,
										  struct route *route);
	status_t			(*unregister_route)(domain_t *domain,
											struct route *route);
	status_t			(*update_routes)(domain_t *domain);

	/*
	 * outbound path
	 */
	status_t			(*send_data)(rtentry_t *route,
									 struct bone_data *data);

	/*
	 * ioctls
	 */
	status_t			(*control)(domain_t *domain,
								   int command,
								   void *data,
								   int *len);

/*****************************************************************************/

	/*
	 * these functions are called by network device drivers to
	 * make themselves (un)know to the datalink
	 */
	status_t			(*register_interface)(struct ifnet *ifnet);
	void				(*unregister_interface)(struct ifnet *ifnet);

	/*
	 * register_handler() and unregister_handler() should be called
	 * by datalink protocol modules to start/stop receiving incoming
	 * data of the specified type (note that 'e_type' should be in
	 * host byte order)
	 */
	status_t			(*register_handler)(uint16 e_type,
											receive_data_func receive_data,
											struct bone_dl_proto_node *me,
											void *cookie);
	void				(*unregister_handler)(uint16 e_type,
											  struct bone_dl_proto_node *me);

	int					(*register_framing_module)(deframe_data_func func,
												   int nFrameTypes);
	void				(*unregister_framing_module)(int frameID);

} bone_datalink_info_t;

#define BONE_DATALINK_MODULE "network/bone_datalink"
#ifdef __cplusplus
}
#endif


#endif
