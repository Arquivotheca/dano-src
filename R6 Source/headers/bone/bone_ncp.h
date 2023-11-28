/*
	bone_ncp.h
	
	template for bone ncp modules

	Copyright 2000, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_NCP
#define H_BONE_NCP

#include <module.h>

struct lognet;
struct bone_data;
struct bone_ppp_cookie;
struct cp_header;
struct cp_options;
struct bone_ncp_lock;

typedef struct ncp_info {
	uint16		config_proto;
	uint16		proto_family;
	uint16		*data_protos;
	uint32		code_mask;
	
	enum {
		LCP_LEVEL,
		AUTH_LEVEL,
		NCP_LEVEL,
	} level;
} ncp_info_t;

typedef struct bone_ncp_info {

	struct module_info 	module;
	ncp_info_t			info;
	
	/*
	 * called by bone_ppp to initialize this ncp
	 */
	status_t	(*init_ncp)(struct bone_ppp_cookie *ppp_cookie,
							struct lognet *lognet,
							void **out);

	/*
	 * called by bone_ppp when it is done with this ncp
	 */
	status_t	(*uninit_ncp)(void *you);

	/*
	 * called by bone_ppp to perform NCP-specific framing/modifications
	 * to an outgoing packet, before it is sent
	 */
	status_t	(*send_data)(void *you, struct bone_data *data);
	
	/*
	 * called by bone_ppp to process an incoming data packet (note that
	 * 'lock' will be locked when receive_data() is called - it must
	 * unlock it before it returns)
	 */
	status_t	(*receive_data)(void *you, struct bone_data *data, uint16 ppp_proto, struct bone_ncp_lock *lock);
	status_t	(*receive_config_request)(void *you, struct cp_header *header, struct bone_data *data);
	status_t	(*receive_config_response)(void *you, struct cp_header *header, struct bone_data *data);
	
	/*
	 * Configuration
	 */
	status_t	(*config)(void *you, int32 op, void *value);
	
	/*
	 * Actions
	 */
	void 		(*this_layer_up)(void *you);			// This-Layer-Up
	void 		(*this_layer_down)(void *you);			// This-Layer-Down
	void 		(*this_layer_started)(void *you);		// This-Layer-Started
	void 		(*this_layer_finished)(void *you);		// This-Layer-Finished
	
	void 		(*send_config_reqest)(void *you);		// Send-Configure-Request
	void 		(*send_config_reply)(void *you);		// Send-Configure-Ack
	
} bone_ncp_info_t;

#endif	/* H_BONE_NCP */
