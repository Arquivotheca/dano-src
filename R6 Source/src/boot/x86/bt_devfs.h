#ifndef _BT_DEVFS_H
#define _BT_DEVFS_H

struct bt_driver_hooks {
	const char *name;

	status_t (*init_driver)(void);
	status_t (*uninit_driver)(void);

	const char **(*publish_devices)(void);

	status_t (*open)(const char *name, void **cookie);
	status_t (*close)(void *cookie);
	ssize_t (*read)(void *cookie, off_t pos, void *buffer, size_t bytes);
	status_t (*ioctl)(void *cookie, uint32 op, void *data, size_t len);
};

#endif
