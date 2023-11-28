
#ifndef _USB_HCD_H
#define _USB_HCD_H

#include <module.h>
#include <iovec.h>

#include <USB_spec.h>
#include <USB.h>


/* The HCD Info and Endpoint objects are private to the
** particular HCD they were created by and opaque to all
** upper layers.  A HCD will redefine these two structures
** in its own image, insuring that they are compatible with
** the versions below:
*/
typedef struct hcd_info hcd_info;
typedef struct hcd_endpoint hcd_endpoint;
//typedef struct hcd_transaction hcd_transaction;

typedef struct hcd_module_info hcd_module_info;
typedef struct hcd_transfer hcd_transfer;
typedef struct hcd_phys_entry hcd_phys_entry;

typedef void (*hcd_notify_cb)(hcd_info *info, void *cookie, int32 page_boundry_crossings_allowed);

struct copy_back_descriptor;

struct hcd_phys_entry
{
	hcd_phys_entry		* next;			/* This must be first for atomic_list */
	
	void				* address;		/* address in physical memory */
	ulong				size;			/* size of block */
	
	/* The bus manager uses these to undo its packet copying, busses should ignore them */
	struct copy_back_descriptor	* copy_back_descr;
	void						* packet_buffer;
};

struct hcd_transfer
{
	/* setup only used by control epts */
	const hcd_phys_entry*	setup_entries;
	const void*				setup;
	size_t					setup_size;
	
	/* data buffer and data length */
	const hcd_phys_entry*	entries;
	void*					user_ptr;
	size_t					user_length;
	
	/* set by HCD before callback is 
	** invoked or notify is released 
	*/
	size_t actual_length;
	status_t status;

	/* if callback is NULL, 
	** notify is released on completion
	*/
	void (*callback)(hcd_transfer *transfer);
	void *cookie;
	sem_id notify;
	
	/* used only for iso transfers */
	usb_iso_packet_descriptor*	iso_packets;
	uint32						iso_packet_count;
	uint32						starting_frame;

	size_t workspace_length;
	
	/* workspace_length bytes must be available here */
};

struct hcd_module_info 
{
	module_info minfo;
	
	/* Create a new Endpoint and (on success) return a
	** valid handle for that endpoint.
	*/
	status_t (*create_endpoint)(hcd_info *info,
								hcd_endpoint **endpoint,
								const usb_endpoint_descriptor *descr,
								int address, bool lowspeed,
								bool nonblocking);
	
	status_t (*get_endpoint_info)(const hcd_endpoint *endpoint,
								  size_t *workspace_length,
								  int *bandwidth);	

	/* Queue a Transfer to an Endpoint
	*/
	status_t (*queue_transfer)(hcd_endpoint *endpoint,
							   hcd_transfer *transfer);
	
	/* Cause any pending Transfers to complete with
	** a status of CANCELLED 
	*/
	status_t (*cancel_transfers)(hcd_endpoint *endpoint);
	
	/* Cancel any pending Transfers and then tear down
	** the endpoint.  Do not use the endpoint again.
	*/
	status_t (*destroy_endpoint)(hcd_endpoint *endpoint);
	
	
	/* Init any host controllers you may be aware of 
	** and register them using the provided notify callback
	*/
	status_t (*start)(hcd_notify_cb notify, void *cookie);
	
	/* Tear down all host controllers you are aware of and
	** do not do any further notification 
	*/
	status_t (*stop)(hcd_notify_cb notify, void *cookie);
	
	/* Reset the data toggle bit for the specified endpoint
	*/
	status_t (*reset_data_toggle)(hcd_endpoint *endpoint);
	
	status_t (*poll_hc)(hcd_info *info);
	
	uint32	 (*get_frame_number)(hcd_info* info);
};

#endif
