#ifndef _BOOTMENU_H
#define _BOOTMENU_H

#define TYPE_RETURN		0
#define TYPE_SUBMENU	1
#define TYPE_RADIO		2
#define TYPE_RESCAN		3
#define TYPE_INPUT		4
#define TYPE_OTHER		5

struct menuitem {
	uint32 id;
	char y;
	char text[62];
	char type;
	int (*f)(void *);
	void *data;
	
	struct menuitem *next;
};

struct menu {
	uint32 id;
	char top, highlight;
	char *text1, *text2, *text3;
	struct menuitem *items;
};

extern struct menu mainmenu;

status_t appendmenuitem(struct menu *m, uint32 id, char extra_dy,
		const char text[64], char type, int (*f)(void *), void *data);
status_t removemenuitem(struct menu *m, uint32 id);
void clearmenuitems(struct menu *m);
struct menuitem *findmenuitem(struct menu *m, uint32 id);

struct bootable_volume_info {
	char master_boot_device[32];
	char boot_device[32];
	char boot_image_file[32];
	char boot_fs[32];
	char kernel[32];

	partition_info pi;
	uchar biosid;

	char volume_name[32];

	struct bootable_volume_info *next;
};

extern struct bootable_volume_info *bootable_volumes;

status_t rescan_bootable_volumes(void);

void bootmenu(int32 *bootorder,
		struct bootable_volume_info *boot_volume_info);

#endif
