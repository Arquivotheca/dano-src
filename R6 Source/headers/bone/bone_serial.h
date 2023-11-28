/*
	bone_serial.h
	
	constants/structures used to communicate with the bone_serial
	interface module

	Copyright 2000, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_SERIAL
#define H_BONE_SERIAL

#include <bone_ioctl.h>
#include <support/SupportDefs.h>
#include <net/if.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	BONE_SERIAL_SET_COMM_PARAMS = BONE_SERIAL_IOCTL_BASE,
	BONE_SERIAL_SET_SEND_PARAMS,
	BONE_SERIAL_SET_RECV_PARAMS,
	BONE_SERIAL_SET_LOCAL,
	BONE_SERIAL_TRANSPORT_DOWN,
};

//number of data bits
typedef enum {
	BS_7_BITS,
	BS_8_BITS,
} bse_data_bits_t;

//parity setting
typedef enum {
	BS_EVEN_PARITY,
	BS_ODD_PARITY,
	BS_NO_PARITY,
} bse_parity_t;

//number of stop bits
typedef enum {
	BS_1_STOP_BIT,
	BS_2_STOP_BITS,
} bse_stop_bits_t;

//flow control setting
typedef enum {
	BS_HW_FLOW_CONTROL,
	BS_SW_FLOW_CONTROL,
	BS_NO_FLOW_CONTROL,
} bse_flow_control_t;

//a bs_serial_comm_params_t is used for BONE_SERIAL_SET_COMM_PARAMS ioctl()s
typedef struct bs_serial_comm_params {

	//allows this ioctl() to be sent from above the datalink
	char if_name[IFNAMSIZ];

	//serial port speed, in baud
	int port_speed;

	//number of data bits
	bse_data_bits_t data_bits;
	
	//parity setting
	bse_parity_t parity;

	//number of stop bits
	bse_stop_bits_t stop_bits;

	//flow control setting
	bse_flow_control_t flow_control;

} bs_comm_params_t;

//a bs_send_params_t is used for BONE_SERIAL_SET_SEND_PARAMS ioctl()s
typedef struct bs_send_params {

	//if true, a leading frame sperator character will never be sent
	//before an outgoing packet (a frame seperator is always sent at the
	//end of an outgoing packet)
	bool omit_leading_delimiter;

} bs_send_params_t;

//a bs_recv_params_t is used for BONE_SERIAL_SET_RECV_PARAMS ioctl()s
typedef struct bs_recv_params {

	//frame-seperator character
	uint8 frame_delimiter;

	//read timeout
	bigtime_t timeout;

} bs_recv_params_t;

//a bs_local_params_t is used for BONE_SERIAL_SET_LOCAL ioctl()s
typedef struct bs_local_params {

	//indicates whether the modem's DCD should be ignored
	bool local_mode;

} bs_local_params_t;

#ifdef __cplusplus
}
#endif

#endif	/* H_BONE_SERIAL */
