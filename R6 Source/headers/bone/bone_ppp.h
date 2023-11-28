/*
	bone_ppp.h
	
	template for bone ppp module

	Copyright 2000, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_PPP
#define H_BONE_PPP

#include <module.h>
#include <bone_ncp.h>
#include <bone_util.h>
#include <bone_dl_proto.h>

// ***
// PPP Constants
// ***

// PPP States
enum
{
	PPPS_INITIAL = 		0,
	PPPS_STARTING = 	1,
	PPPS_CLOSED = 		2,
	PPPS_STOPPED = 		3,
	PPPS_CLOSING = 		4,
	PPPS_STOPPING = 	5,
	PPPS_REQ_SENT = 	6,
	PPPS_ACK_RCVD = 	7,
	PPPS_ACK_SENT = 	8,
	PPPS_OPENED = 		9
};

// PPP Events
enum
{
	PPPE_UP = 			0,			// lower layer is Up
	PPPE_DOWN = 		1,			// lower layer is Down
	PPPE_OPEN = 		2,			// administrative Open
	PPPE_CLOSE = 		3,			// administrative close
	PPPE_TO_POS = 		4,			// Timeout with counter > 0
	PPPE_TO_NEG = 		5,			// Timeout with counter expired
	PPPE_RCR_POS = 		6,			// Receive-Configure-Request (Good)
	PPPE_RCR_NEG = 		7,			// Receive-Configure-Request (Bad)
	PPPE_RCA = 			8,			// Receive-Configure-Ack
	PPPE_RCN = 			9,			// Receive-Configure-Nak/Rej
	PPPE_RTR = 			10,			// Receive-Terminate-Request
	PPPE_RTA = 			11,			// Receive-Terminate-Ack
	PPPE_RUC = 			12,			// Receive-Unknown-Code
	PPPE_RXJ_POS = 		13,			// Receive-Code-Reject (permitted) or Receive-Protocol-Reject
	PPPE_RXJ_NEG = 		14,			// Receive-Code-Reject (catastrophic) or Receive-Protocol-Reject
	PPPE_RXR = 			15			// Receive-Echo-Request or Receive-Echo-Reply or Receive-Discard-Request
};

// Configuration Protocol Type Codes
enum
{
	PPPC_CONFIG_REQ = 	0x01,
	PPPC_CONFIG_ACK = 	0x02,
	PPPC_CONFIG_NAK = 	0x03,
	PPPC_CONFIG_REJ = 	0x04,
	PPPC_TERM_REQ = 	0x05,
	PPPC_TERM_ACK = 	0x06,
	PPPC_CODE_REJ = 	0x07,
	PPPC_PROTO_REJ = 	0x08,
	PPPC_ECHO_REQ = 	0x09,
	PPPC_ECHO_REP = 	0x0A,
	PPPC_DISCARD_REQ = 	0x0B,
	PPPC_IDENT = 		0x0C,
	PPPC_TIME_REMAIN = 	0x0D,
	PPPC_RESET_REQ = 	0x0E,
	PPPC_VENDOR_EXT = 	0x00
};

// Configuration Protocol Type Codes Mask; NCP use this to specify what codes they accept
enum
{
	PPPCM_CONFIG_REQ = 	0x00000002,
	PPPCM_CONFIG_ACK = 	0x00000004,
	PPPCM_CONFIG_NAK = 	0x00000008,
	PPPCM_CONFIG_REJ = 	0x00000010,
	PPPCM_TERM_REQ = 	0x00000020,
	PPPCM_TERM_ACK = 	0x00000040,
	PPPCM_CODE_REJ = 	0x00000080,
	PPPCM_PROTO_REJ = 	0x00000100,
	PPPCM_ECHO_REQ = 	0x00000200,
	PPPCM_ECHO_REP = 	0x00000400,
	PPPCM_DISCARD_REQ = 0x00000800,
	PPPCM_IDENT = 		0x00001000,
	PPPCM_TIME_REMAIN = 0x00002000,
	PPPCM_RESET_REQ = 	0x00004000,
	PPPCM_VENDOR_EXT = 	0x00000001,
};

// Configuration Ops
enum {
	// Session Level
	
	// LCP Level
	
	// Authentication Level
	PPPOP_LOCAL_AUTH = 		2001,	// int (bool): Authenticate ourselves to peer
	PPPOP_PEER_AUTH = 		2002,	// int (bool): Authenticate our peer
	
	// NCP Level
};

// ***
// command codes for bone_ppp::control()
// ***

enum {
	BONE_PPP_GET_NEGOCIATION_STATUS,
	BONE_PPP_SET_USER_PASS,
	BONE_PPP_GET_USER_PASS,
};

// ***
// structure passed to bone_ppp::control(BONE_PPP_*_USER_PASS)
// ***

typedef struct bone_ppp_user_pass {
	const char *username;
	const char *password;
} bone_ppp_user_pass_t;


// ***
// Configuration Proto Option Structs
// ***

// CP Header
typedef struct cp_header
{
	uint8		code;
	uint8		id;
	uint16		length;
} cp_header_t;

// CP option node; used in CP options list
typedef struct cp_option
{
	struct cp_option *next;
	struct cp_option **prev;
	
	uint8		type;
	uint8		len;		// Only includes data - two byte header _not_included
	char		data[0];
} cp_option_t;

// CP options list
typedef struct cp_options
{
	int32			count;
	int32			total_len;
	cp_option_t		*head;
} cp_options_t;

/*
 * Test for the acceptablility of options in 'opts.' Add rej'd options to 'rej' and
 * nak'd options to 'nak' with an acceptable value.
 */
typedef status_t (*test_options_func)(void *cookie, struct cp_options *opts, struct cp_options *nak, struct cp_options *rej);
	
struct lognet;
struct bone_data;
struct bone_ncp_info;
struct bone_ncp_cookie;
struct ppp_fsm;
struct bone_ppp_transport;
struct bone_ncp_lock;


/* PPP protocol types (these should be passed to get_ncp()) */

#define PPPTYPE_LCP 0xC021


/* this structure, which is private to the ppp module, is filled in
   by init_ppp() and must be passed to all subsequent ppp calls */
struct bone_ppp_cookie;


typedef struct bone_ppp_info {

	struct module_info module;

	/*
	 * called by a datalink protocol module to initialize a new ppp
	 * session ('framing_params' allows the datalink protocol module to
	 * set specific HDLC framing parameters to values it requires - these
	 * parameters will not be re-negociated/changed by LCP)
	 */
	status_t	(*init_session)(struct bone_ppp_transport *transport,
								void *transport_cookie,
								struct bone_ppp_cookie **out);
	
	/*
	 * called by a datalink protocol module to close down a ppp session
	 */
	status_t	(*uninit_session)(struct bone_ppp_cookie *you);
	
	/*
	 * register the specified module for use with the PPP session
	 */
	status_t 	(*register_ncp)(struct bone_ppp_cookie *you, struct lognet *lognet, char *module);

	/*
	 * called by a datalink protocol module to deframe 'data' (which
	 * must be an HDLC frame), and dispatch it to the proper NCP module
	 */
	status_t	(*receive_data)(struct bone_data *data,
								struct bone_ppp_cookie *you);
	/*
	 * open/close PPP session
	 */
	status_t	(*open_ppp)(struct bone_ppp_cookie *you);
	status_t	(*close_ppp)(struct bone_ppp_cookie *you, char *reason);
	
	/*
	 * NCP Info and control
	 */
	status_t	(*get_ncp_info)(struct bone_ppp_cookie *you, struct ncp_info **info, uint16 ppp_type);
	status_t	(*get_nth_ncp_info)(struct bone_ppp_cookie *you, struct ncp_info **info, int32 index);
	status_t	(*post_event)(struct bone_ppp_cookie *you, uint16 ppp_type, uint16 event);
	
	/*
	 * Transport module retrieval (used by LCP)
	 */
	void		(*get_transport)(struct bone_ppp_cookie *you,
								 struct bone_ppp_transport **out_transport,
								 void **out_transport_cookie);

	/*
	 * Configuration
	 */
	status_t	(*ncp_config)(struct bone_ppp_cookie *you, uint16 ppp_type, int32 op, void *value);
	
	/*
	 * Generates lower layer up and down events in LCP
	 */
	void		(*lower_layer_up)(struct bone_ppp_cookie *you);
	void		(*lower_layer_down)(struct bone_ppp_cookie *you);
	
	/*
	 * called by a datalink protocol module's control() hook
	 */
	status_t 	(*control)(struct bone_ppp_cookie *you, int cmd, void *arg);
	
	/*
	 * Send data through appropriate ncp module for specified destination domain
	 */
	status_t	(*ncp_send_data)(struct bone_ppp_cookie *you, struct bone_data *data);
	/*********************************************************************/

	/*
	 * called by an NCP modules
	 */

	/* Send outgoing data */
	status_t	(*send_data)(struct bone_ppp_cookie *you,
							 struct bone_data *data);
	
	/* Lock/Unlock NCP */
	status_t	(*lock_ncp)(struct bone_ncp_lock *lock);
	void		(*unlock_ncp)(struct bone_ncp_lock *lock);

	/*
	 * Configuration Option Utils
	 */
	
	/* Create and delete PPP options lists */
	cp_options_t *		(*new_options)(void);
	cp_options_t *		(*duplicate_options)(cp_options_t *options);
	void 				(*delete_options)(cp_options_t *options);
	
	/* Remove an options 'type' from list */
	status_t 			(*delete_option)(cp_options_t *options, uint8 type);
	
	/* Add a new option of length 'len' to the options list; returns pointer to 'data' portion of option */
	char *				(*add_option)(cp_options_t *options, uint8 type, uint16 len);
	
	/* Locate option of specified 'type' in the list and return a pointer */
	cp_option_t *		(*find_option)(cp_options_t *options, uint8 type);
	
	/* Add a copy of 'option' to the list 'options' */
	status_t 			(*copy_option)(cp_options_t *options, const cp_option_t *option);
	
	/* Parse bone_data for options and append to list */
	status_t 			(*data_to_options)(struct bone_data *data, cp_options_t *options);
	
	/* Flatten options list into bone_data; add header with 'code' and 'id' set */
	status_t 			(*options_to_data)(const cp_options_t *options, struct bone_data *data, uint8 code, uint8 id);
	
	 /* Process a configuration request packet and generate a reply packet by means of */
	 /* the ncp's test_options() function */
	 /* Return codes: */
	 /*	-1 	A serious error has occured; the request should be ignored and 'reply' is not valid */
	 /*	0 	All options were acceptable; Options have been set. */
	 /* 1	Some options were not acceptable; rej/nak or both */
	status_t			(*process_config_req)(	test_options_func test,
												void *cookie,
												struct bone_data *request,
												struct bone_data **reply,
												uint8 reply_id);

} bone_ppp_info_t;

#define BONE_PPP_MODULE "network/ppp/bone_ppp"

#endif	/* H_BONE_PPP */
