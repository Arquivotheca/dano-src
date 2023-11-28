/*
	bone_proto.h
	
	networking protocol module template
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_PROTO
#define H_BONE_PROTO

#include <BeBuild.h>
#include <SupportDefs.h>
#include <KernelExport.h>
#include <module.h>
#include <bone_data.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bone_endpoint;
struct bone_proto_node;
struct rtentry;

#define SET_OPTION		0x80000000
#define GET_OPTION		0x00000000
#define SOCKOPT_MASK	0x7fffffff

// Socket IO control level
#define SOCKET_IO		0x1000

typedef struct bone_proto_info
{
	struct module_info info;
	
	/*
	 * called when constructing a new endpoint with this protocol
	 */
	status_t (*init_protocol)(struct bone_proto_node *you);
	
	/*
	 * called when the the endpoint is deleted
	 */
	status_t (*uninit_protocol)(struct bone_proto_node *you);

	/*
	 * called by the API driver after all the modules in the protocol
	 * stack have been successfully initialized
	 */
	status_t (*open)(struct bone_proto_node *you);

	/*
	 * called by API driver when a user calls "close" on the socket
	 */
	status_t (*close)(struct bone_proto_node *you);
	
	/*
	 * called by API driver when "free" is called; after all I/O transactions have completed.
	 */
	status_t (*free)(struct bone_proto_node *you);
	
	/*
	 * called by API driver to create an end-to-end link
	 */
	status_t (*connect)(struct bone_proto_node *you, const struct sockaddr *them);
	
	/*
	 * called by an endpoint or upper-level protocol module to send data
	 * without a pre-determined route (i.e., the domain is not locked at
	 * the time of the call)
	 *
	 * protocols below the transport layer need to be prepared for "you"
	 * to be 0, since some protocols (e.g. tcp, icmp, etc) will send data
	 * without being associated with an endpoint.  An example would
	 * be TCP reflecting a RST to an inbound SYN segment for which there
	 * is no listener.
	 *
	 * send_data() must pass 'data' down the stack, or free it.
	 */
	status_t (*send_data)(struct bone_proto_node *you, bone_data_t *data);

	/*
	 * called by upper-level protocol modules to send pre-routed data
	 * (i.e., the upper level protcol has already made the routing decision,
	 * and read-locked the domain to ensure that the routing table doesn't
	 * change before the packet is sent)
	 *
	 * like send_data() above, 'you' may be NULL, and 'data' must be
	 * passed down or freed; 'snd_route' must not be NULL
	 *
	 * in addition, the domain must be unlocked before send_routed_data()
	 * returns (note that the same rule applies to the protocol module
	 * below this one; as such, a protocol module only needs to unlock
	 * the domain itself when it fails to call the send_routed_data()
	 * below it)
	 */
	status_t (*send_routed_data)(struct bone_proto_node *you,
								 struct rtentry *snd_route,
								 bone_data_t *data);

	/*
	 * Called by the API driver to determine how many bytes may be sent without
	 * blocking
	 */
	int32 (*send_avail)(struct bone_proto_node *you);

	/*
	 * called by the API driver to read data.  Data should be copied into the "into"
	 * iovecs accordingly.  bone_util_info_t::dequeue_fifo_data does this automatically,
	 * for example.
	 */
	status_t (*read_data)(struct bone_proto_node *you, size_t numBytes, uint32 flags, bone_data_t **data_ptr);
	
	/*
	 * called by the API driver to determine how many bytes of data may be immediately
	 * read without blocking
	 */	
	int32  (*read_avail)(struct bone_proto_node *you);


	/*
	 * called by datalink layer when data arrives
	 * note: receiver (api driver on success, protocol on error) frees data 
	 */
	status_t (*receive_data)(bone_data_t *data);
	

	/*
	 * called by API driver to block for a new connection
	 */
	status_t (*accept)(struct bone_proto_node *you, struct bone_endpoint **aep);
	
	/*
	 * called by API driver on ioctl or setsockopt
	 */
	status_t (*control)(struct bone_proto_node *you, int level, int cmd, void *arg, int *arglen);

	/* called by higher-level protocol to get the domain_t structure
	 * for this protocol's address family (the domain_t is necessary
	 * for locking and some routing calls)
	 */
	struct domain_info	*(*get_domain)(struct bone_proto_node *you);

	/*
	 * called from higher-level protocols to get route MTU - because
	 * this involves a routing decision, the domain must be locked before
	 * getMTU() is called, and the resulting mtu is guaranteed to be valid
	 * only as long as the domain remains locked
	 */
	int32	 (*getMTU)(struct bone_proto_node *you, const struct sockaddr *sa);
	
	/*
	 * called from higher-level protocols to get route to 'addr' - the
	 * resulting rtentry_t, if not NULL, can be used when calling
	 * send_routed_data()
	 *
	 * the domain must be locked before get_route() is called, and the
	 * resulting route is only valid as long as the route is locked
	 * (once the domain is unlocked, the routing table could change,
	 * the rtentry_t could be deleted, etc.)
	 */
	struct rtentry *(*get_route)(struct bone_proto_node *you,
								 const struct sockaddr *addr);
	
	/*
	 * called from api dricer to bind to local address
	 */
	status_t (*bind)(struct bone_proto_node *you, struct sockaddr *sa);

	/*
	 * called from api driver to unbind from local address
	 */
	status_t (*unbind)(struct bone_proto_node *you, struct sockaddr *sa);
	
	/*
	 * called from api driver to specify listen queue for ibd connections
	 */	
	status_t (*listen)(struct bone_proto_node *you, int count);
	
	/*
	 * called from api driver to close one direction of a connection (half-close)
	 */
	status_t (*shutdown)(struct bone_proto_node *you, int direction);
	
	/*
	 * interface for error codes (ICMP, etc) to be passed back to protocols
	 * from the network layer
	 */
	status_t (*error)(int32 error_code, bone_data_t *error_data);

	/*
	 * interface for error replies (ICMP, etc) to be sent from protocols
	 * to other sites.  Error codes can be passed, or a data ptr and length
	 * may be passed.  If error_data is non-0 error_code will be assumed to
	 * be a length, otherwise a code.
	 */
	status_t (*error_reply)(struct bone_proto_node *you, bone_data_t *caused_error, uint32 error_code, void *error_data);
	
} bone_proto_info_t;

/*
 *	each protocol gets passed one of these at init and for most calls
 * 	to save state and determine who to hand data off to.
 */
typedef struct bone_proto_node
{
	struct bone_endpoint 	*endpoint;
	bone_proto_info_t 		*proto;
	void			  		*proto_state;
	struct bone_proto_node	*next;
	struct bone_proto_node	*prev;
} bone_proto_node_t;

#ifdef __cplusplus
}
#endif


#endif /* H_BONE_PROTO */
