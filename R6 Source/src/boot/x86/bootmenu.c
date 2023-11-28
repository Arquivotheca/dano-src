#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>

#include <boot.h>

#include "bootmenu.h"
#include "bt_misc.h"
#include "platform.h"
#include "release.h"
#include "vesa.h"
#include "video.h"

struct bootable_volume_info *bootable_volumes = NULL;

static status_t scan_bootable_volumes(void *cookie, const char *path)
{
	struct bootable_volume_info **tail = (struct bootable_volume_info **)cookie,
			*n, node;
	status_t err;
	struct stat st;
	int fd;

	if (mount("bfs", "/bscan", path, 0, NULL, 0) < 0)
		if (mount("cfs", "/bscan", path, 0, NULL, 0) < 0)
			return 0;

	memset(&node, 0, sizeof(node));
	strcpy(node.boot_device, path);

	fd = err = open(path, 0);
	if (err < 0)
		goto err1;
	node.biosid = 0xff;
	ioctl(fd, B_GET_BIOS_DRIVE_ID, &node.biosid, 1);
	ioctl(fd, B_GET_PARTITION_INFO, &node.pi, sizeof(node.pi));
	ioctl(fd, 'imfs', &node, sizeof(node));
	close(fd);

	fd = err = open("/bscan", 0);
	/* this volume was mis-identified as bfs */
	if (err < 0) {
		err = 0;
		goto err1;
	}
	err = ioctl(fd, 0x01020304, node.volume_name, sizeof(node.volume_name));
	close(fd);
	if (err < 0)
		goto err1;

	if (	(stat("/bscan/beos/system/kernel_intel", &st) == B_OK) &&
			/* only consider kernels with modification dates of 1/1/99 or
			 * later. this limits us to R4.5+ kernels only. */
			(st.st_mtime >= 0x368c8080)) {
		strcpy(node.kernel, "kernel_intel");
		n = malloc(sizeof(*n));
		if (!n) {
			err = ENOMEM;
			goto err1;
		}
		*n = node;
		if (!(*tail))
			bootable_volumes = n;
		else
			(*tail)->next = n;
		*tail = n;
	}

	if (	(stat("/bscan/beos/system/kernel_intel_alt", &st) == B_OK) &&
			(st.st_mtime >= 0x368c8080)) {
		strcpy(node.kernel, "kernel_intel_alt");
		n = malloc(sizeof(*n));
		if (!n) {
			err = ENOMEM;
			goto err1;
		}
		*n = node;
		if (!(*tail))
			bootable_volumes = n;
		else
			(*tail)->next = n;
		*tail = n;
	}

	err = B_OK;

err1:
	unmount("/bscan");
	return 0; // errors aren't handled anyway -- err;
}

status_t rescan_bootable_volumes()
{
	struct bootable_volume_info *c, *n;

	for (c=bootable_volumes;c;c=n) {
		n = c->next;
		free(c);
	}

	c = NULL;
	return recurse("/dev", scan_bootable_volumes, &c);
}

#define TOPMOST 2
#define LEFTMOST 10
#define RIGHTMOST (80 - LEFTMOST)
#define WIDTH (80 - 2*LEFTMOST + 1)

#define MENULINES 14

status_t appendmenuitem(struct menu *m, uint32 id, char extra_dy,
		const char text[64], char type, int (*f)(void *), void *data)
{
	struct menuitem *item, *p;

	item = malloc(sizeof(*item));
	if (!item)
		return ENOMEM;

	item->id = id;
	strcpy(item->text, text);
	item->type = type;
	item->f = f;
	item->data = data;
	
	p = m->items;

	if (!p) {
		item->y = 0;
		item->next = m->items;
		m->items = item;
		return B_OK;
	}
	
	while (p->next)
		p = p->next;

	item->y = p->y + 1 + extra_dy;
	item->next = p->next;
	p->next = item;

	return B_OK;
}

status_t removemenuitem(struct menu *m, uint32 id)
{
	struct menuitem *p, *q;
	
	q = p = m->items;
	while (p && (p->id != id)) {
		q = p;
		p = p->next;
	}

	if (!p) return ENOENT;
	
	if (p == q)
		m->items = m->items->next;
	else
		q->next = p->next;

	free(p);

	return B_OK;
}

void clearmenuitems(struct menu *m)
{
	struct menuitem *c, *n;
	c = m->items;
	while (c) {
		n = c->next;
		free(c);
		c = n;
	}
	m->items = NULL;
	m->top = 0;
	m->highlight = 0;
}

struct menuitem *findmenuitem(struct menu *m, uint32 id)
{
	struct menuitem *i = m->items;
	while (i && (i->id != id))
		i = i->next;
	return i;
}

static int32 *boot_order;
static struct bootable_volume_info *boot_volume_info;

static bool bootable_from_cd;

int flag_safemode = 0;
int flag_safemodevga = 0;
int flag_nobios = 0;
int flag_nomp = 0;
int flag_noidedma = 0;
int flag_nouseraddons = 0;
int flag_consoledebug = 0;

#ifdef MEMORY_TOOLS
int flag_noswap = 0;
int flag_limit16 = 0;
int flag_limit24 = 0;
#endif

struct menu mainmenu;

int setbootpartition(void *data)
{
	struct bootable_volume_info *vi = (struct bootable_volume_info *)data;

	if (data) {
		sprintf(findmenuitem(&mainmenu, 'PART')->text,
				"Select new boot volume (Current: " WHITE "%s" GREY ")",
				vi->volume_name);
		*boot_order = BOOT_ORDER_DISK;
		*boot_volume_info = *vi;
	} else {
		sprintf(findmenuitem(&mainmenu, 'PART')->text,
				"Select new boot volume (Current: " WHITE "%s" GREY ")",
				"CD-ROM");
		*boot_order = BOOT_ORDER_CD_ONLY;
	}

	return B_OK;
}

static void
rebuild_partition_list(void)
{
	struct bootable_volume_info *c;
	struct menu *m;
	struct menuitem *mi;

	/* rebuild menu */
	mi = findmenuitem(&mainmenu, 'PART');
	m = (struct menu *)(mi->data);
	if (!m)
		m = (struct menu *)mi->data = calloc(sizeof(*m), 1);
	clearmenuitems(m);

	for (c=bootable_volumes;c;c=c->next) {
		char text[64];
		sprintf(text, "%-40s (%s)", c->volume_name, c->kernel);
		appendmenuitem(m, 0, 0, text, TYPE_OTHER, setbootpartition, c);
	}

	if (bootable_from_cd)
		appendmenuitem(m, 0, 0, "Boot from CD-ROM", TYPE_OTHER, setbootpartition, NULL);

	appendmenuitem(m, 0, 1, "Rescan for bootable volumes", TYPE_RESCAN, NULL, NULL);
	appendmenuitem(m, 0, 0, "Return to main menu", TYPE_RETURN, NULL, NULL);
}

static void
input(struct menu *m, struct menuitem *item)
{
	char line[COMMAND_LINE_OPTIONS_LENGTH];
	int char1 = 0, curchar, i, len;
	struct command_line_options *p, *options;

	p = options = command_line_options;
	for (i=0;options&&(i<item->id);i++) {
		p = options;
		options = options->next;
	}
	if (!options) {
		options = calloc(sizeof(*options), 1);
		if (!p)
			command_line_options = options;
		else
			p->next = options;
	}

	strcpy(line, options->options);
	len = strlen(line);
	curchar = len;

	while (1) {
		char visible[WIDTH + 3];
		int i, key;
		
		if (curchar < 0) curchar = 0;
		if (curchar > len) curchar = len;

		if (curchar < char1)
			char1 = curchar;
		else if (curchar > char1 + WIDTH - 1)
			char1 = curchar - WIDTH + 1;

		platform_move_cursor(LEFTMOST + curchar - char1, TOPMOST + 3 + item->id - m->top);

		visible[0] = COLOR_TAG;
		visible[1] = 0x70;
		for (i=0;(i<WIDTH)&&line[i+char1];i++)
			visible[2+i] = line[i+char1];
		memset(visible + 2 + i, ' ', WIDTH - i);
		visible[WIDTH + 2] = 0;

		print_at(LEFTMOST, TOPMOST + 3 + item->id - m->top, visible);

		switch ((key = platform_get_key())) {
			case KEY_PGUP:
			case KEY_PGDN:
			case KEY_UP:
			case KEY_ENTER:
			case KEY_DOWN:
				strcpy(options->options, line);
				strncpy(item->text, line, sizeof(item->text) - 1);
				item->text[sizeof(item->text)-1] = 0;
				if (!options->next && line[0]) {
					/* insert new line for options */
					struct menuitem *m;
					m = calloc(sizeof(*m), 1);
					m->next = item->next;
					item->next = m;
					m->id = item->id + 1;
					m->y = item->y + 1;
					m->type = TYPE_INPUT;
					m->next->y++;
					options->next = calloc(sizeof(*options), 1);
				}
				if (key == KEY_PGUP)
					m->highlight -= MENULINES;
				else if (key == KEY_PGDN)
					m->highlight += MENULINES;
				else if (key == KEY_UP)
					m->highlight--;
				else
					m->highlight++;
				/* fall through */
			case KEY_ESC:
				platform_move_cursor(0,0);
				return;
			case KEY_HOME :
				curchar = 0;
				break;
			case KEY_END :
				curchar = COMMAND_LINE_OPTIONS_LENGTH;
				break;
			case KEY_LEFT :
				curchar--;
				break;
			case KEY_RIGHT :
				curchar++;
				break;
			case KEY_DEL :
				if (curchar < len) {
					memmove(line + curchar, line + curchar + 1, len - curchar);
					len--;
				}
				break;
			case KEY_BS :
				if (curchar > 0) {
					memmove(line + curchar - 1, line + curchar, len - curchar + 1);
					curchar--;
					len--;
				}
				break;
			default :
				if ((key >= 0x20) && (key < 0x80) && (len < sizeof(line) - 1)) {
					memmove(line + curchar + 1, line + curchar, sizeof(line) - curchar - 1);
					line[curchar++] = key;
					len++;
				}
				break;
		}
	}
}

static void
drawmenu(struct menu *m)
{
	int i;
	char s[81];
	struct menuitem *item;

	/* Draw header */
	center(TOPMOST, BLUE "B" RED "e" WHITE "OS Release " RELEASE);
	center(TOPMOST+1, "Boot Loader");

	for (i=0,item = m->items; item; item=item->next,i++) {
		if ((item->y < m->top) || (item->y >= m->top + MENULINES)) continue;

		memset(s, ' ', 80);
		s[80] = 0;
		memcpy(s + LEFTMOST, item->text, strlen(item->text));
		print_at(0, TOPMOST + 3 + item->y - m->top, s);
//		print_at(LEFTMOST, items[i].y, items[i].text);
		if (i == m->highlight) {
			invert(LEFTMOST, RIGHTMOST, TOPMOST + 3 + item->y - m->top);
		}
	}

	if (m->text1) print_at(LEFTMOST, TOPMOST + 3 + MENULINES + 1, m->text1);
	if (m->text2) print_at(LEFTMOST, TOPMOST + 3 + MENULINES + 2, m->text2);
	if (m->text3) print_at(LEFTMOST, TOPMOST + 3 + MENULINES + 3, m->text3);
}

static void
runmenu(struct menu *m)
{
	int i, oldtop, num_items;
	struct menuitem *item;

rescanned:
	for (num_items=0,item=m->items;item;item=item->next,num_items++)
		;

	while (1) {
		if (m->highlight < 0) m->highlight = 0;
		if (m->highlight >= num_items) m->highlight = num_items - 1;

		item = m->items;
		for (i=0;i<m->highlight;i++)
			item = item->next;

		oldtop = m->top;
		if (item->y >= m->top + MENULINES) m->top = item->y - MENULINES + 1;
		if (item->y < m->top) m->top = item->y;
		if (m->top != oldtop) clear_screen();

		drawmenu(m);
		
		item = m->items;
		for (i=0;i<m->highlight;i++)
			item = item->next;

		if (item->type != TYPE_INPUT) {
			switch (platform_get_key()) {
				case KEY_UP : m->highlight--; break;
				case KEY_DOWN : m->highlight++; break;
				case KEY_PGUP : m->highlight -= MENULINES; break;
				case KEY_PGDN : m->highlight += MENULINES; break;
				case KEY_HOME : m->highlight = 0; break;
				case KEY_END : m->highlight = 127; break;
				case KEY_F1 : dprintf_enabled = TRUE; break;
	
				case KEY_ESC :
					if (m != &mainmenu) return;
					break;
	
				case KEY_ENTER :
					if (item->type == TYPE_SUBMENU) {
						clear_screen();
						runmenu((struct menu *)item->data);
						clear_screen();
						break;
					}
					/* fall through */
				case ' ' :
					switch (item->type) {
						case TYPE_RETURN :
							if (	(m == &mainmenu) &&
									(*boot_order == BOOT_ORDER_NONE)) {
								print_at(LEFTMOST, TOPMOST + 9, WHITE "You must select a boot volume.");
								spin(3000000);
								print_at(LEFTMOST, TOPMOST + 9, "                              ");
								break;
							}
								
							m->top = m->highlight = 0;
							return;
						case TYPE_RADIO :
							*(int *)(item->data) ^= 1;
							item->text[1] = (*(int *)(item->data) ? 'X' : ' ');
							break;
						case TYPE_RESCAN :
						{
							unmount("/dev");
							mount("dev", "/dev", NULL, 0, NULL, 0);
							scan_partitions();
							rescan_bootable_volumes();
							rebuild_partition_list();
							clear_screen();
							goto rescanned;
						}
	
						case TYPE_OTHER :
							if ((item->f)(item->data) == B_OK)
								return;
					}
					break;
			}
		} else {
			input(m, item);
			clear_screen();
			goto rescanned;
		}
	}
}

void bootmenu(int32 *_boot_order, struct bootable_volume_info *_boot_volume_info)
{
	struct menu *safemode, *advancedsafemode;

#if _SUPPORTS_RELEASE_MODE
	extern bool is_release_mode(void);
	
	if(is_release_mode()) {
		panic("boot menu invoked in release mode\n");
	}
#endif

	boot_order = _boot_order;
	boot_volume_info = _boot_volume_info;

	bootable_from_cd = ((*boot_order == BOOT_ORDER_CD_DISK) ||
			(*boot_order == BOOT_ORDER_CD_ONLY));

	advancedsafemode = calloc(sizeof(*advancedsafemode), 1);
	appendmenuitem(advancedsafemode, 0, 0, "", TYPE_INPUT, NULL, NULL);
	appendmenuitem(advancedsafemode, 'EXIT', 1, "Return to safe mode menu", TYPE_RETURN, NULL, NULL);

	advancedsafemode->text1 =
		"This allows entry of additional driver-specific options not covered";
	advancedsafemode->text2 =
		"in the previous safe mode menu.";

	safemode = calloc(sizeof(*safemode), 1);
	appendmenuitem(safemode, 0, 0, "[ ] Safe mode", TYPE_RADIO, NULL, &flag_safemode);
	appendmenuitem(safemode, 0, 0, "[ ] Use fail-safe video mode", TYPE_RADIO, NULL, &flag_safemodevga);
	appendmenuitem(safemode, 0, 0, "[ ] Don't call the BIOS", TYPE_RADIO, NULL, &flag_nobios);
	appendmenuitem(safemode, 0, 0, "[ ] Disable multiprocessor support", TYPE_RADIO, NULL, &flag_nomp);
	appendmenuitem(safemode, 0, 0, "[ ] Disable IDE DMA", TYPE_RADIO, NULL, &flag_noidedma);
	appendmenuitem(safemode, 0, 0, "[ ] Disable user add-ons", TYPE_RADIO, NULL, &flag_nouseraddons);
	appendmenuitem(safemode, 0, 0, "[ ] Enable console debugging", TYPE_RADIO, NULL, &flag_consoledebug);
	appendmenuitem(safemode, 0, 1, "Advanced safe mode options", TYPE_SUBMENU, NULL, advancedsafemode);
	appendmenuitem(safemode, 0, 1, "Return to main menu", TYPE_RETURN, NULL, NULL);

	safemode->text1 =
		"Use these options to recover in the event of boot failure. Choosing";
	safemode->text2 =
		"the topmost option will automatically enable the others.";

	memset(&mainmenu, 0, sizeof(mainmenu));

	appendmenuitem(&mainmenu, 'PART', 0, "", TYPE_SUBMENU, NULL, NULL);
	rebuild_partition_list();

	appendmenuitem(&mainmenu, 'SAFE', 0, "Select safe mode options", TYPE_SUBMENU, NULL, safemode);
#ifdef MEMORY_TOOLS
	appendmenuitem(&mainmenu, 0, 0, "[ ] Limit Memory to 16MB", TYPE_RADIO, NULL, &flag_limit16);
	appendmenuitem(&mainmenu, 0, 0, "[ ] Limit Memory to 24MB", TYPE_RADIO, NULL, &flag_limit24);
	appendmenuitem(&mainmenu, 0, 0, "[ ] Disable Swap", TYPE_RADIO, NULL, &flag_noswap);
#endif			
	platform_bootmenu_pre();
	appendmenuitem(&mainmenu, 'CONT', 1,
			"Continue booting", TYPE_RETURN, NULL, NULL);

	if (*boot_order == BOOT_ORDER_CD_DISK) {
		strcpy(findmenuitem(&mainmenu, 'PART')->text,
				"Select new boot volume (Current: " WHITE "CD-ROM or hard drive" GREY ")");
	} else {
		sprintf(findmenuitem(&mainmenu, 'PART')->text,
				"Select new boot volume (Current: " WHITE "%s" GREY ")",
				boot_volume_info->boot_device[0] ?
					boot_volume_info->volume_name : "None");
	}

	platform_enter_console_mode();
	clear_screen();

	runmenu(&mainmenu);

	if (	(*boot_order == BOOT_ORDER_CD_DISK) ||
			(*boot_order == BOOT_ORDER_CD_ONLY)) {
		print_at(LEFTMOST, TOPMOST + 9,
			"Please wait while the disk drivers are loaded.");
	}

	clear_screen();
	platform_bootmenu_post();
}
