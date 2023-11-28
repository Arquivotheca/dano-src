/*
	bone_dl_route.h - structures used in datalink-maintained routing tables
	
	Copyright 2000, Be Incorporated, All Rights Reserved.
*/

#ifndef __BONE_DL_ROUTE_H
#define __BONE_DL_ROUTE_H

#include <net/route.h>


/*
 * a datalink routing "tree" is really two trees, joined at their leaf nodes.
 * a tree composed of route_cmp_nodes is used to quickly find the most-
 * specific closest-matching route for a given address.  nodes in the
 * second tree, which are route_bt_nodes, are ordered on a route's mask
 * length (the route with the longest mask is at the leaf), so finding a
 * matching route is simply a matter of "backtracking" up the second tree,
 * stopping when the most specific matching route is found
 */

typedef struct route_cmp_node {
	/* a mask with a single bit set, used when searching the comparison
	   tree (note that 'bitMask' also exists at the same position in
	   route_bt_node_t, but is always zero there) */
	uint8 bitMask;

	/* specifies which word of an address 'bitMask' should be applied to */
	int cmp_offset;

	/* when searching the tree, if the result of applying 'bitMask' to
	   the address being searched for is zero, the tree node 'off' is
	   visited next - if non-zero, 'on' is visited (note that, although
	   'off' and 'on' are declared as route_cmp_nodes, they may actually
	   point to route_bt_nodes leaf nodes - this can be determined by
	   checking the value of the node's 'bitMask' field) */
	struct route_cmp_node *cmp_off;
	struct route_cmp_node *cmp_on;

} route_cmp_node_t;

typedef struct route_bt_node {
	/* always zero */
	uint8 bitMask;

	 /* the number of (leading, contiguous) bits set in 'addr's mask */
	int bt_masklen;

	/* pointer to this route's address (note that the actual address
	   will be stored somewhere else, most likely in 'bt_route') */
	uint8 *bt_addr;

	/* points to this route's parent (the most specific route which is
	   less specific than this node's route) */
	struct route_bt_node *bt_parent;

	/* each node keeps a list of its children (nodes whose 'parent' point
	   to this node), sorted by masked address */
	struct route_bt_node *bt_children;

	/* points to next sibling */
	struct route_bt_node *bt_nextAddr;

	/* the rtentry structure containing all the parameters for this route */
	rtentry_t *bt_route;

} route_bt_node_t;


/*
 * generally, there will be one routing table per domain.  as such,
 * the domain_t structure contains the 'route_info' member - if a
 * given domain wants to make use of datalink routing tables, it should
 * allocate/populate a bone_routing_info structure, and store its location
 * in 'route_info' before registering the domain with the datalink
 */

typedef struct routing_table {

	/* searching of the comparison tree begins at 'root' (note that
	   'root' can actually point to a route_bt_node_t, if the tree
	   contains no comparison nodes) */
	route_cmp_node_t *root;

	/* a routing tree will always contain a node for the "default route"
	   (a route with a 'bt_masklen' of zero, which will therefore match
	   iff no other route in the tree matches a given address) */
	route_bt_node_t *default_route;

} routing_table_t;

typedef struct bone_routing_info {

	/* routing table - populated and maintained by the datalink */
	routing_table_t table;

	/* offset, in bytes, of the actual address in the structure which
	   contains it (e.g., 4 for IP, since IP addresses are stored in
	   sockaddr_in structures, where the 'sin_addr' field starts at
	   byte four) */
	int addr_start;

	/* offset of the first non-address byte in the structure (e.g., 8
	   for IPv4) */
	int addr_end;

} bone_routing_info_t;


/* a 'route_iter_func' is passed to a domain's 'foreach_route' callback
   (see bone_datalink.h), where it will be called once for each rtentry_t
   in the domain's routing table.  a return value of 'true' indicates that
   the route should be deleted from the table. */
typedef bool (*route_iter_func)(rtentry_t *route, void *arg);


#endif	/* __BONE_DL_ROUTE_H */
