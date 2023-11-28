/*
	bone_dl_proto.h
	
	datalink protocol module template
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_DL_PROTO
#define H_BONE_DL_PROTO

#include <module.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bone_data;
struct ifnet;
struct bone_dl_proto_node;
struct domain_info;

typedef struct bone_dl_proto_info
{
	/*
	 * called by the datalink when instantiating a new instance
	 * of this protocol
	 */
	status_t (*init_protocol)(struct bone_dl_proto_node *you);
	
	/*
	 * called by the datalink when an instance of this protocol is deleted
	 */
	status_t (*uninit_protocol)(struct bone_dl_proto_node *you);

	/*
	 * called by the datalink when there is data to send.
	 * (after operating on the data, should call 'you->next->send_data()')
	 */
	status_t (*send_data)(struct bone_dl_proto_node *you,
						  struct bone_data *data);

	/*
	 * called when this interface is coming up (should pass along
	 * notification by calling 'you->next->if_up()' before returning)
	 */
	status_t (*if_up)(struct bone_dl_proto_node *you);
	
	/*
	 * called when this interface is going down (should pass along
	 * notification by calling 'you->next->if_down()' before returning)
	 */
	void (*if_down)(struct bone_dl_proto_node *you);

	/*
	 * catch all (if your protocol module doesn't understand a particular
	 * 'cmd' value, forward it to 'you->next->control()')
	 */
	status_t (*control)(struct bone_dl_proto_node *you,
						int cmd,
						void *arg);

} bone_dl_proto_info_t;

/*
 *	each protocol gets passed one of these at init and for most calls
 * 	to save state and determine who to hand data off to.
 */
typedef struct bone_dl_proto_node
{
	struct lognet				*lognet;	/* logical interface which sends data through this protocol node */
	bone_dl_proto_info_t 		*proto;		/* function pointers */
	void			  			*proto_state;	/* private protocol-specific data (mananaged by protocol module itself) */
	struct bone_dl_proto_node	*next;		/* protocol node below us in stack */
} bone_dl_proto_node_t;


/* this is the type of function a datalink protocol module must pass
 * to bone_datalink::register_handler()
 */
typedef status_t (*receive_data_func)(struct bone_data *data, void *cookie);


/*
 * framing modules, although implemented as bone_dl_proto_info modules
 * like everything else, are treated somewhat specially by the datalink -
 * they must be registered through 'register_framing_module()' (as opposed
 * to 'register_handler()'), and do not receive a list of protocol stacks
 * when they are called to deframe incoming data (a framing module probably
 * won't be sending any data when it is called to de-frame a packet, so
 * it doesn't need them)
 */
typedef int (*deframe_data_func)(struct bone_data *data);


/*
 * unlike other bone modules, a datalink protocol module publishes the
 * bone_dl_proto_info_t's it contains via a "factory" function - this
 * allows the packaging of multiple modules which differ slightly but
 * share lots of code (e.g., IPv4 ARP and Appletalk's AARP) in a single
 * kernel module.  this eliminates the need for lots of "utility modules"
 * containing this shared code, and/or multiple copies of the same code
 * in different modules.
 *
 * your datalink modules should publish only a "factory" through its
 * 'modules' variable.
 */

typedef struct bone_dl_proto_factory {

	struct module_info info;

	/*
	 * factory method - this will be called with a sub-module name,
	 * obtained from bone.conf (e.g., the datalink protocol name
	 * "bone_arp:ipv4" will result in the 'bone_arp's module's factory
	 * being called to obtain sub-module "ipv4"), or the empty string ("")
	 * if no sub-module name is specified (e.g., just "bone_arp" is
	 * specified in bone.conf)
	 */
	status_t (*get_dl_proto)(const char *name, bone_dl_proto_info_t **out);

} bone_dl_proto_factory_t;


#ifdef __cplusplus
}
#endif


#endif /* H_BONE_DL_PROTO */
