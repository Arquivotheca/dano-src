/*
	bone_ppp_transport.h
	
	template for bone ppp transport modules

	Copyright 2000, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_PPP_TRANSPORT
#define H_BONE_PPP_TRANSPORT

#include <bone_dl_proto.h>

struct bone_data;

/*
 * a bone_ppp_transport is bone_dl_proto_info_t, with additional hooks
 * to facilitate communication with the bone_ppp module.
 */


/* values comprising 'bone_ppp_transport_params::valid_mask' */
enum {
	BPPPT_ACCM_MASK = 0x1,
	BPPPT_MRU_MASK = 0x2,
	BPPPT_PFC_MASK = 0x4,
	BPPPT_ACFC_MASK = 0x8,
};


/* this structure specifies the various framing/transport parameters
   PPP will negociate */
typedef struct bone_ppp_transport_params {

	//mask indicating which of the below values are valid
	int valid_mask;

	//maximum receive unit, in host byte order
	int mru;

	//the asynchronous control-character map, in host byte order
	uint32 accm;

	//indicates state of address and control field compression
	bool acfc;

	//indicates state of protocol field compression
	bool pfc;

} bpppt_params_t;


/* values describing the types of PPP configure response messages */
typedef enum {
	BPPPT_CONFIG_ACK,
	BPPPT_CONFIG_NAK,
	BPPPT_CONFIG_REJ,
} bpppt_cr_type_t;


typedef struct bone_ppp_transport {

	/* a bone_ppp_transport is a datalink protocol module */
	bone_dl_proto_info_t dl_proto_info;

	/* called by bone_ppp to send outgoing PPP packets */
	status_t		(*send_ppp_frame)(void *cookie, struct bone_data *data);

	/* called in LCP's This-Layer-Started state to determine the
	   starting point for transport/framing parameter negociation */
	void			(*get_desired_params)(void *cookie,
										  bpppt_params_t *out);
	
	/* called by LCP when a negative configure response packet is received -
	   'req_params' is an in-out parameter, initially containing the
	   values used in the last Configure Request, while 'cr_params'
	   specifies the parameters contained in the configure response packet.
	   adjust_params() should modify 'req_params' to make it (more)
	   acceptable to the peer */
	void			(*adjust_params)(void *cookie,
									 bpppt_params_t *req_params,
									 bpppt_cr_type_t response_type,
									 const bpppt_params_t *cr_params);


	/* called by LCP when a Configure Request packet is received - 'params'
	   is an in-out parameter, initially containing values from the
	   received config-req packet.  check_requested_params() will
	   determine if these parameters are acceptable, and indicate to LCP
	   what type of config response packet should be sent, as well as
	   fill 'params' with values to be included in that response */
	bpppt_cr_type_t	(*check_requested_params)(void *cookie,
											  bpppt_params_t *params);


	/* called by bone_ppp to set the transport/framing parameters
	   negociated by LCP - if 'send_params' and 'recv_params' are NULL,
	   the default parameters for the link must be used */
	status_t 		(*use_params)(void *cookie,
								  const bpppt_params_t *send_params,
								  const bpppt_params_t *recv_params);

} bone_ppp_transport_t;


#endif /* H_BONE_PPP_TRANSPORT */
