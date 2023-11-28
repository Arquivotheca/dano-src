#include <drivers/KernelExport.h>

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bios.h"
#include "bootmenu.h"
#include "bt_misc.h"
#include "platform.h"
#include "vesa.h"
#include "video.h"

struct vesa_mode {
	int modenum;
	int x, y, bits, rowbyte, base;
	struct vesa_mode *next;
};

static struct vesa_mode standard_vga_mode;
static struct vesa_mode *modes = NULL;
static struct vesa_mode *vesamode = NULL; /* video mode to use in safe mode */

static status_t vesa_init(void)
{
	struct vbe_info info;
	status_t result = B_ERROR;

#define validate_ptr(a) \
	(((char *)&info.a >= (char *)&info) && \
	 ((char *)&info.a < ((char *)&info + sizeof(info))))
	
	if (modes)
		return B_OK;

	memset(&info, 0, sizeof(info));
	info.VbeSignature = '2EBV';
	if ((vesa_get_info(&info) == B_OK) && (info.VbeSignature == 'ASEV') &&
		(info.VbeVersion >= 0x200) && (validate_ptr(OemStringPtr)) &&
		(validate_ptr(VideoModePtr)) && (validate_ptr(OemVendorNamePtr)) &&
		(validate_ptr(OemProductNamePtr)) && (validate_ptr(OemProductRevPtr))) {
		uint16 *modep;
		dprintf("VBE2-compliant card: %s %s %s revision %s detected\n",
				(char *)info.OemStringPtr, (char *)info.OemVendorNamePtr,
				(char *)info.OemProductNamePtr, (char *)info.OemProductRevPtr);
		for (modep = (uint16 *)info.VideoModePtr;
				((char *)modep < ((char *)&info + sizeof(info))) &&
						(*modep != 0xffff);
				modep++) {
			struct vbe_mode_info m;
			int bits;

			memset(&m, 0, sizeof(m));
			dprintf("probing mode %x\n", *modep);
			if (	(vesa_get_mode_info(*modep, &m) != B_OK) ||
					/* D0 = supported in hardware, D3 = color mode,
					 * D4 = graphics mode, D7 = linear frame buffer */
					((m.ModeAttributes & 0x99) != 0x99) ||
					(m.NumberOfPlanes != 1) ||
					(m.NumberOfBanks != 1) ||
					(m.PhysBasePtr == 0))
				continue;
			bits = m.BitsPerPixel;
			if (m.MemoryModel == 6)
				bits = m.RedMaskSize + m.GreenMaskSize + m.BlueMaskSize;
			dprintf("%dx%dx%d @ %lx -- planes = %d, banks = %d, model %d\n",
					m.XResolution, m.YResolution, bits, m.PhysBasePtr,
					m.NumberOfPlanes, m.NumberOfBanks, m.MemoryModel);
			if ((bits != 8) && (bits != 15) && (bits != 16) && (bits != 32))
				continue;
			if ((m.XResolution < 640) || (m.YResolution < 400))
				continue;
			if ((m.MemoryModel == 4) || (m.MemoryModel == 6)) {
				struct vesa_mode *vm;
				vm = malloc(sizeof(*vm));
				vm->modenum = *modep;
				vm->x = m.XResolution;
				vm->y = m.YResolution;
				vm->bits = bits;
				vm->rowbyte = m.BytesPerScanLine;
				vm->base = m.PhysBasePtr;
				vm->next = modes;
				modes = vm;
				result = B_OK;
			}
		}
	}

	return result;
}

static status_t _set_vesa_mode_(struct vesa_mode *m)
{
	status_t error;
	
	error = vesa_set_mode(m->modenum);
	if (error != B_OK)
		return error;

	if (m->bits == 8) {
		uchar palette[0x400];
		int i;
		for (i=0;i<0x100;i++) {
			palette[4*i] = colormap[3*i+2] >> 2;
			palette[4*i+1] = colormap[3*i+1] >> 2;
			palette[4*i+2] = colormap[3*i] >> 2;
		}
		vesa_set_palette((void *)palette);
	}

	platform_notify_video_mode(m->x, m->y, m->bits,
			m->rowbyte, (void *)m->base);

	return B_OK;
}

status_t set_vesa_mode(int x, int y, int bits)
{
	struct vesa_mode *m;

	vesa_init();
		
	m = modes;
	while(m) {
		if(m->x == x && m->y == y && m->bits == bits) {
			vesamode = m;
			return _set_vesa_mode_(m);
		}
		m = m->next;
	}
	dprintf("no vesa mode for %dx%dx%d\n", x, y, bits);

	return B_ERROR;
}

static int vesamenucallback(void *data)
{
	vesamode = (struct vesa_mode *)data;
	if(data == &standard_vga_mode)
		sprintf(findmenuitem(&mainmenu, 'VESA')->text, 
				"Select fail-safe video mode (Current: " WHITE "Standard VGA"
				GREY ")");
	else if(data == NULL)
		sprintf(findmenuitem(&mainmenu, 'VESA')->text, 
				"Select fail-safe video mode (Current: " WHITE "Default"
				GREY ")");
	else
		sprintf(findmenuitem(&mainmenu, 'VESA')->text, 
				"Select fail-safe video mode (Current: " WHITE "%dx%dx%d" GREY ")",
				vesamode->x, vesamode->y, vesamode->bits);
		
	return B_OK;
}

int platform_bootmenu_pre(void)
{
	int num;
	struct vesa_mode *m;
	struct menu *vesamenu;

	if (vesa_init() != B_OK) return 0;
	
	vesamenu = calloc(sizeof(*vesamenu), 1);
	
	for (num=0,m=modes;m;num++,m=m->next)
		;

	appendmenuitem(vesamenu, 0, 0, "Default", TYPE_OTHER, vesamenucallback, vesamode);
	appendmenuitem(vesamenu, 0, 0, "Standard VGA", TYPE_OTHER, vesamenucallback, &standard_vga_mode);

	for (m=modes;m;m=m->next) {
		char text[64];
		sprintf(text, "%dx%dx%d", m->x, m->y, m->bits);
		appendmenuitem(vesamenu, 0, 0, text, TYPE_OTHER, vesamenucallback, m);
	}

	appendmenuitem(vesamenu, 0, 1, "Return to main menu", TYPE_RETURN, NULL, NULL);

	vesamenu->text1 =
		"This setting only takes effect if you have an unsupported video";
	vesamenu->text2 =
		"card or if you have selected \"Use fail-safe video mode\" in the";
	vesamenu->text3 =
		"safe mode menu.";

	appendmenuitem(&mainmenu, 'VESA', 0, 
			"Select fail-safe video mode (Current: " WHITE "Default" GREY ")",
			TYPE_SUBMENU, NULL, vesamenu);

	return 1;
}

void platform_bootmenu_post(void)
{
	extern int flag_consoledebug;

	if (vesamode == NULL || vesamode == &standard_vga_mode || flag_consoledebug)
		return;

	if (_set_vesa_mode_(vesamode) == B_OK)
		platform_splash_screen(FALSE);
}

void vesa_boot(void)
{
	char buf[256];
	char *bp;
	size_t size;
	int w,h,d;
	static struct vesa_mode *m;
	int fd;

	if(vesamode)	/* user selected mode */
		return;

	fd = open("/boot/home/config/settings/kernel/drivers/vesa", O_RDONLY);
	if(fd < 0)
		return;

	size = read(fd, buf, sizeof(buf)-1);
	close(fd);
	
	if(size <= 0)
		return;

	buf[size] = '\0';
	bp = buf;

	while(*bp == '#') {
		while(*bp && *bp != '\n')
			bp++;
		if(*bp)
			bp++;
	}
	if(strncmp("mode", bp, 4) != 0) {
		dprintf("could not find mode setting in vesa config\n");
		return;
	}
	bp += 4;

	w = strtoul(bp, &bp, 0);
	h = strtoul(bp, &bp, 0);
	d = strtoul(bp, &bp, 0);
	if(!(w && h && d)) {
		dprintf("invalid mode setting, %dx%dx%d\n", w, h, d);
		return;
	}

	vesa_init();
		
	m = modes;
	while(m) {
		if(m->x == w && m->y == h && m->bits == d) {
			vesamode = m;
			platform_bootmenu_post();
			return;
		}
		m = m->next;
	}
	dprintf("no vesa mode for %dx%dx%d\n", w, h, d);
}
