/*
	bone_pppoe.h
	
	constants/structures used to communicate with the bone_pppoe
	datalink protocol module

	Copyright 2001, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_PPPOE
#define H_BONE_PPPOE

#include <bone_ioctl.h>
#include <support/SupportDefs.h>
#include <net/if.h>


#ifdef __cplusplus
extern "C" {
#endif

//ioctl() command values
enum {
	BONE_PPPOE_SET_USER_PASS = BONE_PPPOE_IOCTL_BASE,
};


//a bpppoe_user_pass_t is used for BONE_PPPOE_SET_USER_PASS ioctl()s
typedef struct bpppoe_user_pass_t {

	//allow this ioctl() to be sent from above the datalink
	char if_name[IFNAMSIZ];

	//username and password, to be passed to PAP
	const char *username;
	const char *password;

} bpppoe_user_pass_t;


#ifdef __cplusplus
}
#endif

#endif	/* H_BONE_PPPOE */
