#ifndef _BOOT_VESA_H
#define _BOOT_VESA_H

status_t set_vesa_mode(int x, int y, int bits);
int platform_bootmenu_pre(void);
void platform_bootmenu_post(void);
void vesa_boot(void);

#endif
