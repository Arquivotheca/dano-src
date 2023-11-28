/*
	bone_interface.h
	
	networking interface module template
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_IFACE
#define H_BONE_IFACE

#include <BeBuild.h>
#include <SupportDefs.h>
#include <KernelExport.h>
#include <module.h>
#include "bone_data.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <net/if.h>
#include <net/if_media.h>

#define BONE_LOOPBACK_INTERFACE "bone_loopback"

typedef struct bone_iface_hwaddr
{
	uint32 addrlen;
	unsigned char hw_addr[256];
} bone_iface_hwaddr_t;

struct bone_endpoint;
struct bone_datalink_info;

typedef struct bone_interface_params
{
	const char *interface_leaf;
	int len;
	void *buf;
} bone_interface_params_t;

typedef struct bone_ether_interface_params
{
	const char *device;
} bone_ether_interface_params_t;

typedef struct bone_interface_info
{
	struct module_info info;
	
	/*
	 * init the interface and register with the passed-in datalink.
	 * Datalink may be null; if so it means that you have been opened by a test
	 * driver.  If that is the case, simply return one instance of the ifnets
	 * you create in here to be used by the driver.
	 *
	 * If datalink is non-null, call the datalink's register_interface func
	 * once for each interface you create.  Return any of them (the return
	 * value is checked but not used by the datalink)
	 */ 
	ifnet_t *(*init)(struct bone_datalink_info *datalink, struct bone_interface_params *params);

	/*
	 * uninit the interface, freeing any structures init() allocated,
	 * including 'ifnet' itself
	 */
	void (*uninit)(ifnet_t *ifnet);

	/*
	 *	Bring the interface up.  
	 */
	status_t (*up)(ifnet_t *ifnet);
	
	/*
	 * Bring the interface down.  Any semaphores used for blocking reads
	 * MUST be deleted (and recreated, if not done in up()) at this time,
	 * so that the interface's reader thread may be unblocked in the datalink.
	 */
	void (*down)(ifnet_t *ifnet);
	
	/*
	 * send data on the specified interface.  Should DMA from the
	 * iovecs stored in data, if possible.  Data is already framed.
	 * Multiple MTU-sized framed chunks may be present.
	 *
	 * Note to ethernet card vendors:  the standard 802.3 framing module
	 * does NOT do trailers; frames will be handed off like this:
	 * <14 byte hdr><1500 byte data>[repeated n times]...<14 byte hdr><x bytes data>
	 * where x is the remaining data after the original data has been broken up
	 * into 1500 byte frames + headers.  The same is true of jumbograms, just
	 * using the higher MTU.  ETHER_MAX_LEN_NOTRAILER is your friend.  See
	 * net/ethernet.h.
	 */
	status_t (*send_data)(ifnet_t *ifnet, bone_data_t *data);
	
	/*
	 * block until data is received.  A bone_data_t will be allocated and
	 * data is be copied into it from the card's buffers.  This is the only
	 * mandated data copy in bone.  Does not deframe data.
	 */
	status_t (*receive_data)(ifnet_t *ifnet, bone_data_t **data);
	
	/*
	 * interface ioctls not covered by the following 3 functions.  Note that there
	 * are no "getters"; card info is available for inspection from the ifnet_t (and
	 * the interface module will update those values... see net/if.h)
	 */
	status_t (*control)(ifnet_t *ifnet, int cmd, void *arg);
	
	/*
	 * set the card's MTU
	 */
	status_t (*setMTU)(ifnet_t *ifnet, uint32 mtu);
	
	/*
	 * toggle promiscuous mode on/off
	 */
	status_t (*setPromiscuous)(ifnet_t *ifnet, int on);
	
	/*
	 *	Force card to use the specified media, if possible.  See net/if_media.h.
	 */
	status_t (*setMedia)(ifnet_t *ifnet, uint32 media);
	
	/*
	 * returns the hardware address of the card.  Used primarily by the framing modules.
	 */ 
	status_t (*getHardwareAddr)(ifnet_t *ifnet, bone_iface_hwaddr_t *addr);
	
	/*
	 * Ditto for getting or setting multicast addresses.
	 */ 	
	status_t (*getMulticastAddrs)(ifnet_t *ifnet, bone_iface_hwaddr_t **addrs, int numaddr);
	status_t (*setMulticastAddrs)(ifnet_t *ifnet, bone_iface_hwaddr_t **addrs, int numaddr);
} bone_interface_info_t;


#ifdef __cplusplus
}
#endif


#endif /* H_BONE_IFACE */
