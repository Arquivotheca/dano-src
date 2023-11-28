/*
	bone_dialer.h
	
	dialer module
		
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_DIALER
#define H_BONE_DIALER

#include <BeBuild.h>
#include <SupportDefs.h>
#include <KernelExport.h>
#include <module.h>
#include <bone_data.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bone_dialer_stats
{
	bigtime_t 	conntime;
	int 	  	baudrate;
	size_t		bytes_sent;
	size_t 		bytes_received;
	
} bone_dialer_stats_t;

typedef struct bone_dialer_cookie
{
	bigtime_t				timeout;
	char			 		*device;
	int 					fd;
	int 					port_speed;
	uint32 					flags;
#define BDF_AUTODIAL 	0x1
#define BDF_RETRY 		0x2
#define BDF_NO_CHAT		0x4
#define BDF_NO_HW_FLOW_CONTROL 0x8
#define BDF_NO_SW_FLOW_CONTROL 0x10
#define BDF_PULSE_DIAL		0x20
#define BDF_NO_LF			0x40
#define BDF_NO_ECHO			0x80
#define BDF_LOG				0x100
	char 					*init_string;
	char 					*phone_number;
	char 					*user;
	char					*password;
	int 					scriptfd;

	bone_dialer_stats_t 	*stats;	
} bone_dialer_cookie_t;

#define DIALER_PREFS_PATH "/etc/dialer/"
#define DIALER_DEFAULT_SCRIPT_PATH "/etc/dialer/defaultscript"
#define DIALER_LOGFILE	"/tmp/dialer_chat"

#define	BDP_TAG_PORTSPEED	"portspeed"
#define	BDP_TAG_AUTODIAL	"autodial"
#define	BDP_TAG_RETRY	 	"retry"
#define	BDP_TAG_NO_CHAT		"disable_chat"
#define	BDP_TAG_HWFLOW		"disable_rts_cts"
#define	BDP_TAG_SWFLOW		"disable_xonn_xoff"
#define	BDP_TAG_INIT		"modeminit"
#define	BDP_TAG_PHONENUMBER "phone_number"
#define	BDP_TAG_PULSE		"pulse_dial"
#define BDP_TAG_DEVICE		"serialport"
#define BDP_TAG_SCRIPT		"scriptfile"
#define BDP_TAG_USER		"user"
#define BDP_TAG_PASSWORD	"password"
#define BDP_TAG_NO_LF		"no_lf"
#define BDP_TAG_NO_ECHO		"no_echo"
#define BDP_TAG_TIMEOUT		"timeout"
#define BDP_TAG_LOG			"log"

#define BDS_TAG_SEND		"send"
#define BDS_TAG_EXPECT		"expect"
#define BDS_TAG_PAUSE		"pause"
#define BDS_TAG_CHAT		"chat"

typedef struct bone_dialer
{
	struct module_info info;
	status_t (*open)(bone_dialer_cookie_t **bdc, char *opt_profile);  /* profile can be 0 */
	status_t (*dial)(bone_dialer_cookie_t *bdc);
	int32    (*send)(bone_dialer_cookie_t *bdc, bone_data_t *bdt);
	int32    (*recv)(bone_dialer_cookie_t *bdc, char *buf, int32 size);
	void	 (*close)(bone_dialer_cookie_t *bdc);
	
} bone_dialer_t;

#define BONE_DIALER "bone/dialer"
#define DIALER_PROFILES "/etc/dialer/"


#ifdef __cplusplus
}
#endif


#endif /* H_BONE_DIALER */
