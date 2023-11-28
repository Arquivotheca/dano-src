#ifndef _BOOT_PLATFORM_H
#define _BOOT_PLATFORM_H

#include <bootscreen.h>

extern screen curscreen;

int platform_key_hit(void);
int platform_get_key(void);
int platform_shift_state(void);
void platform_move_cursor(int x, int y);

void platform_notify_video_mode(
		int width, int height, int depth, int rowbyte, void *base);
void platform_enter_console_mode(void);
void platform_enter_graphics_mode(void);
status_t platform_initialize(void);
void platform_pass_boot_icons(void);
void platform_splash_screen(bool check_keys);
void platform_set_disk_state(int state);

void calculate_cpu_clock(void);
void init_timing(void);

#endif
