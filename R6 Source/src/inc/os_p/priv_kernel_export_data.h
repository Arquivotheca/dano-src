#ifndef _PRIV_KERNEL_EXPORT_DATA_H
#define _PRIV_KERNEL_EXPORT_DATA_H

#include <support/SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* kernel_export_data contains data that is readable from userspace in
   every team. Data here need to be updated in real time by the kernel
*/

typedef struct {
	/* system time data */
	int64	system_time_base;
	int64	system_real_time_base;
	int32	cv_factor;

} kernel_export_data_t;

extern kernel_export_data_t	*kernel_export_data;

/*   in kernel:  create area */
extern status_t init_kernel_export_data();

/* system call to map kernel export data read-only in usespace.
   Called by libroot during init */
extern status_t _kmap_kernel_export_data_(kernel_export_data_t **ptr);

#ifdef __cplusplus
}
#endif

#endif
