#ifndef _BOOT_MISC_H
#define _BOOT_MISC_H

extern bool dprintf_enabled;
extern bool boot_menu_enabled;
extern bool fast_boot_enabled;

#define COMMAND_LINE_OPTIONS_LENGTH 1024
struct command_line_options {
	char options[COMMAND_LINE_OPTIONS_LENGTH];
	struct command_line_options *next;
};
extern struct command_line_options *command_line_options;

void parse_command_line_options(void);

void check_boot_keys(void);

extern uchar colormap[];

#define KEY_BS		0x08
#define KEY_ENTER	0x0d
#define KEY_ESC		0x1b

#define KEY_UP		0x0100
#define KEY_DOWN	0x0101
#define KEY_LEFT	0x0102
#define KEY_RIGHT	0x0103
#define KEY_HOME	0x0104
#define KEY_END		0x0105
#define KEY_PGUP	0x0106
#define KEY_PGDN	0x0107
#define KEY_DEL		0x0108

#define KEY_F1		0x0201
#define KEY_F8		0x0208

status_t recurse(const char *path, status_t (*f)(void *, const char *),
		void *cookie);
status_t scan_partitions();

#endif
