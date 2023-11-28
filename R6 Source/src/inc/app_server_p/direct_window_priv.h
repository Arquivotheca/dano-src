#ifndef	_DIRECT_WINDOW_PRIV_H
#define	_DIRECT_WINDOW_PRIV_H

#include <SupportDefs.h>

enum {
	B_BLA_BLA = 0
};
	
/* private struct used to clone driver type one (old add-ons) */
typedef struct {
    area_id               memory_area;
    area_id               io_area;
	char                  addon_path[64+B_FILE_NAME_LENGTH];
} direct_driver_info1;

typedef struct {
	area_id		clipping_area;
	sem_id		disable_sem;
	sem_id		disable_sem_ack;
} dw_init_info;

enum {
	B_SUPPORTS_WINDOW_MODE = 0x0001
};

typedef struct {
	uint32		flags;
} direct_window_info;

#endif


