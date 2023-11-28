/*
	bone_serial_ppp.h
	
	constants/structures used to communicate with the bone_serial_ppp
	datalink protocol module

	Copyright 2000, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_SERIAL_PPP
#define H_BONE_SERIAL_PPP

#include <bone_ioctl.h>
#include <support/SupportDefs.h>
#include <net/if.h>


#ifdef __cplusplus
extern "C" {
#endif

//ioctl() command values
enum {
	BONE_SERIAL_PPP_SET_PARAMS = BONE_SERIAL_PPP_IOCTL_BASE,
	BONE_SERIAL_PPP_GET_STATUS,
};


//connect script command types
typedef enum {
	BSPPP_CHAT_CMD_SEND = 0,
	BSPPP_CHAT_CMD_EXPECT,
	BSPPP_CHAT_CMD_PAUSE,
} bspppe_chat_cmd_t;

//an array of 'bsppp_chat_cmd_t's comprises a "compiled" connect script
typedef struct {
	//command type
	bspppe_chat_cmd_t cmd;

	//optional argument (will be NULL for "pause" commands)
	const char *arg;

} bsppp_script_cmd_t;


//automated chat modes
typedef enum {
	BSPPP_DISABLE_CHAT,	//skip chat - initiate LCP immediately after connect
	BSPPP_REQUIRE_CHAT,	//only initiate LCP after chat has sent user/pass
	BSPPP_AUTO_CHAT,	//if chat times out, initiate LCP
} bspppe_chat_mode_t;

//a bsppp_dial_params_t is used for BONE_SERIAL_PPP_SET_PARAMS ioctl()s
typedef struct bsppp_dial_params {

	//allow this ioctl() to be sent from above the datalink
	char if_name[IFNAMSIZ];

	//indicates whether bone_serial_ppp should wait to dial until
	//data is actually sent
	bool autodial;

	//number of connect retries before giving up (i.e., bone_serial_ppp
	//will attempt to connect 'retry_count + 1' times)
	int retry_count;

	//delay between attempts to connect
	bigtime_t retry_delay;

	//"compiled" connect script
	int n_script_commands;
	bsppp_script_cmd_t *connect_script;

	//upper-bound on the time bone_serial_ppp will wait for the connect
	//script to finish
	bigtime_t connect_timeout;

	//indiactes whether automated username/password chat should or
	//should not be used (or expected) after the connect script finishes
	bspppe_chat_mode_t chat_mode;

	//upper-bound on the amount of "dead air" automated chat will endure
	//before considering chat stalled
	bigtime_t chat_timeout;

	//username and password, for use by the automated chat
	const char *username;
	const char *password;

} bsppp_dial_params_t;


//values for describing the state of a serial ppp connection
typedef enum {
	BSPPP_DISCONNECTED = 0,	//not connected
	BSPPP_DIALING,			//dialing the modem
	BSPPP_CHATTING,			//modem connected, performing automated chat
	BSPPP_NEGOCIATING,		//chat completed, performing LCP/NCP negociation
	BSPPP_CONNECTED,		//negociation complete, ready to send/recv packets
	BSPPP_DISCONNECTING,	//tearing down connection, hanging up modem
	BSPPP_WAITING,			//a previous connection attempt failed - waiting to try again
} bsppp_connection_state_t;

//a bsppp_status_t is used for BONE_SERIAL_PPP_GET_STATUS ioctl()s
typedef struct bsppp_status {

	//allow this ioctl() to be sent from above the datalink
	char if_name[IFNAMSIZ];

	//current connection status (filled out by bone_serial_ppp module)
	bsppp_connection_state_t connection_status;

	//reason for last disconnect (e.g., busy signal, lost carrier)
	status_t last_error;

	//speed reported by the modem when we connected (note that this
	//isn't necessarily the current speed of the connection), or
	//zero if we're not connected/the speed couldn't be determined
	int connect_speed;

} bsppp_status_t;


#ifdef __cplusplus
}
#endif

#endif	/* H_BONE_SERIAL_PPP */
