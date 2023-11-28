/* ++++++++++
   File:  apm.h
   Name:  ficus
   Date:  Thu May 14 02:58:41 PDT 1998
   Copyright (c) 1998 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _APM_H
#define _APM_H

#if __INTEL__

#define APM_SUPPORT

#include <priv_syscalls.h>		/* for _kapm_control_ */
#include <SupportDefs.h>

enum {
	APM_INIT = 0,
	APM_CHECK_ENABLED,
	APM_SLEEP,
	APM_STANDBY,
	APM_SHUTDOWN,
	APM_YO_MAMA_SO_FAT_SHE_FREEBASES_HAM   /* daaaamn! */
};

/* we probably don't need all of these extremely specific error
 * codes, but i don't think each value maps to a single error,
 * and i want to be sure of what the BIOS is telling me when i'm
 * trying to figure out why someone's machine is spuriously
 * powering down when they try to save files.
 */
enum {
	APM_INTERFACE_NOT_CONNECTED		= B_ERRORS_END + 1,
	APM_INTERFACE_NOT_ENGAGED,
	APM_NO_PENDING_EVENTS,
	APM_UNRECOGNIZED_DEVICE_ID,
	APM_OUT_OF_RANGE,
	APM_FUNCTIONALITY_DISABLED,
	APM_UNABLE_TO_ENTER_STATE,
	APM_HI_MOM
};

/* this is the information the BIOS gives us */
typedef struct _apm_bios_info apm_bios_info;
struct _apm_bios_info
{
	uint16 version;
	uint16 flags;
	uint16 cs32;
	uint32 offset;
	uint16 cs16;
	uint16 ds;
	uint16 cs_len;
	uint16 ds_len;
};

/* a 48-bit addr:seg entry point for the assembly call */
typedef struct _apm_bios_entry apm_bios_entry;
struct _apm_bios_entry {
	uint32 offset;
	uint16 cs;
};

extern apm_bios_info *apm_info;
extern apm_bios_entry apm_bios;
extern uchar apm_enabled;

/* APM startup function */
void apm_init(void);

/* kernel daemon callback */
void apm_daemon(void *data, int phase);

/* generic interface to calling the APM BIOS */
int8 apm_bios_call(uint16 *ax, uint16 *bx, uint16 *cx, uint16 *dx,
				   uint16 *si, uint16 *di);

/* textual representation of our error enum */
const char *apm_strerror(status_t err);

/* APM functions */
status_t apm_interface_disconnect(void);
status_t apm_set_power_state(uint16 device, uint16 state);
status_t apm_enable_power_management(uint16 device);
status_t apm_disable_power_management(uint16 device);
status_t apm_get_pm_event(uint16 *ecode, uint16 *einfo);
status_t apm_driver_version(uint16 version);
status_t apm_engage_power_management(uint16 device);
status_t apm_disengage_power_management(uint16 device);


/* currently unused stuff ...consider removing */

status_t apm_cpu_idle(void);
status_t apm_cpu_busy(void);

status_t apm_get_power_status(uint16 device, uint8 *ac_status,
							  uint8 *bat_status, uint8 *bat_flags,
							  uint8 *bat_life_p, uint16 *bat_life_t,
							  uint16 *n_battery_units);


/* APM BIOS event codes */

#define APM_REQUEST_STANDBY			0x01
#define APM_REQUEST_SUSPEND			0x02
#define APM_NORMAL_RESUME			0x03
#define APM_CRITICAL_RESUME			0x04
#define APM_BATTERY_LOW				0x05
#define APM_STATUS_CHANGE			0x06
#define APM_UPDATE_TIME				0x07
#define APM_CRITICAL_SUSPEND		0x08
#define APM_USER_REQUEST_STANDBY	0x09
#define APM_USER_REQUEST_SUSPEND	0x0a
#define APM_STANDBY_RESUME			0x0b
#define APM_CAPABILITY_CHANGE		0x0c

#endif /* __INTEL__ */

#endif /* _APM_H */

