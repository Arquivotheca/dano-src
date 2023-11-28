#ifndef _DRIVER_SETTINGS_P_H
#define _DRIVER_SETTINGS_P_H

#include <OS.h>

extern status_t init_driver_settings();
extern status_t uninit_driver_settings(); /* testing only */
extern bool parse_bool_string(const char * s, bool * result);

#if !BOOT
extern status_t init_driver_settings_prot();
extern status_t init_driver_settings_boot_mounted();
#endif

/* bootloader only */
#if BOOT
extern status_t add_safemode_setting(const char *data);
extern status_t read_boot_settings(const char *volume);
#endif

#define KERNEL_DRIVER_SETTINGS			"kernel"
#define VM_DRIVER_SETTINGS				"virtual_memory"

#define KERNEL_SETTING_BIOS_CALLS		"bios_calls"
#define KERNEL_SETTING_MP_SUPPORT		"multiprocessor_support"
#define KERNEL_SETTING_APM				"apm"
#define KERNEL_SETTING_DISK_CACHE_SIZE	"disk_cache_size"
#define KERNEL_SETTING_HLT				"hlt"
#define KERNEL_SETTING_KEEP_LOADED		"keep_drivers_loaded"
#define KERNEL_SETTING_SHUTDOWN			"enable_shutdown"
#define KERNEL_SETTING_KMALLOC_TRACKING	"kmalloc_tracking"
#define KERNEL_SETTING_LOAD_SYMBOLS		"load_symbols"
#define SMP_IO_INTERRUPTS				"smp_io_interrupts"
#define CHECK_FOCUS_CPU					"check_focus_cpu"
#define MAX_CPU_NUMBER					"max_cpu_number"
#define KERNEL_SETTING_SHOW_PATCHING	"show_patching"
#define KERNEL_SETTING_LOAD_SYMBOLS		"load_symbols"

#define SAFEMODE_DISABLE_IDE_DMA		"disableidedma"
#define SAFEMODE_DISABLE_USER_ADDONS	"disableuseraddons"

#define VM_ENABLED_SETTING				"vm"
#define VM_SWAP_SIZE_SETTING			"swap_size"

#endif
