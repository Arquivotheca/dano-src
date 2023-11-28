#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <OS.h>

#include <kb_mouse_driver.h>


#include "vt100.h"

status_t 
_kmap_physical_memory_(const char *, void *, ulong, ulong, ulong, void **);

int read_isa_io(int, void *addr, int size);
int write_isa_io(int, void *addr, int size, int val);
     
#define inb(port) read_isa_io(0,(void*)port,1)
#define outb(byte,port) write_isa_io(0,(void*)port,1,byte)

int32 ExecControlled(char *argv[], int argc, thread_id *thr);

void move_cursor(struct virtscreen *cur)   
{
    int offset = 80 * cur->ypos + cur->xpos;
    outb (0xe, 0x3d4);
    outb (offset / 256, 0x3d5);
    outb (0xf, 0x3d4);
    outb (offset % 256, 0x3d5);
}

#define ESC 27
#define BS 8
#define TAB 9
#define CR 13
#define LSHIFT 0x4b
#define RSHIFT 0x56

const uint8 scancodes[] = {
    /*   0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f */
/* 0 */ 0x00, 0x01, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x26,
/* 1 */ 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x47, 0x5c, 0x3c, 0x3d,
/* 2 */ 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x11, 0x4b, 0x33, 0x4c, 0x4d, 0x4e, 0x4f,
/* 3 */ 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x24, 0x5d, 0x5e, 0x3b, 0x02, 0x03, 0x04, 0x05, 0x06,
/* 4 */ 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x22, 0x0f, 0x37, 0x38, 0x39, 0x25, 0x48, 0x49, 0x4a, 0x3a, 0x58,
/* 5 */ 0x59, 0x5a, 0x64, 0x65, 0x7e, 0x00, 0x69, 0x0c, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 6 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 7 */ 0x6e, 0x00, 0x00, 0x6b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6d, 0x00, 0x6c, 0x00, 0x6a, 0x00, 0x00,
/* extended codes start here - we added 0x80 to the scancode */
/* 8 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 9 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5b, 0x60, 0x00, 0x00,
/* a */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* b */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x0e, 0x5f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* c */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x20, 0x57, 0x21, 0x00, 0x61, 0x00, 0x63, 0x00, 0x35,
/* d */ 0x62, 0x36, 0x1f, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x67, 0x68, 0x00, 0x00
};


char ScanTable[] = {
		/*  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
/* 0 */		0 ,ESC, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 1 */     0,'`','1','2','3','4','5','6','7','8','9','0','-','=', BS,  0,
/* 2 */		0,  0,  0,'/','*','-',TAB,'q','w','e','r','t','y','u','i','o',
/* 3 */	  'p','[',']','\\', 0,  0,  0,'7','8','9','+',  0,'a','s','d','f',
/* 4 */   'g','h','j','k','l',';','\'',CR,'4','5','6',  0,'z','x','c','v',
/* 5 */   'b','n','m',',','.','/',  0,  0,'1','2','3', CR,  0,  0,' ',  0,
/* 6 */		0,  0,  0,  0,'0','.',  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

char Vt100ScanTable[] = {
     /*		0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
/* 0 */		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 1 */		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 2 */		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 3 */		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 4 */		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 5 */		0,  0,  0,  0,  0,  0,  0,'A',  0,  0,  0,  0,  0,  0,  0,  0,
/* 6 */		0,'D','B','C',  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

char ShiftTable[] = {
		/*  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
/* 0 */		0 ,ESC, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 1 */     0,'~','!','@','#','$','%','^','&','*','(',')','_','+', BS,  0,
/* 2 */		0,  0,  0,'/','*','-',TAB,'Q','W','E','R','T','Y','U','I','O',
/* 3 */	  'P','{','}','|',  0,  0,  0,'7','8','9','+',  0,'A','S','D','F',
/* 4 */   'G','H','J','K','L',':','\"',CR,'4','5','6',  0,'Z','X','C','V',
/* 5 */   'B','N','M','<','>','?',  0,  0,'1','2','3', CR,  0,  0,' ',  0,
/* 6 */		0,  0,  0,  0,'0','.',  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};




status_t keyboard_thread(void *data)
{
	int fd = (int) data;
	int kb;
	uint8 key;
	int shift = 0;
	
	kb = open("/dev/input/keyboard/at/0", O_RDONLY);
	if (kb < 0) return kb;

	while (1) {
//		keyboard_io kbio;
		at_kbd_io kbio;
		
		if ((ioctl(kb, KB_READ, &kbio, sizeof(kbio)) == B_OK))
		{
			if(kbio.is_keydown)
			{
					key = scancodes[kbio.scancode];
					if(key == LSHIFT || key == RSHIFT)
					{
						shift = 1;
						continue;
					}
					if(Vt100ScanTable[key]) {
						char esc1= 27;
						char esc2= '[';
						
						key= Vt100ScanTable[key];
						
						write(fd, &esc1, 1);
						write(fd, &esc2, 1);
						write(fd, &key , 1);
					} else {
						key = shift ? ShiftTable[key] : ScanTable[key];
						write(fd, &key, 1);
					}
			} else {
					key = scancodes[kbio.scancode];
					if(key == LSHIFT || key == RSHIFT)
					{
						shift = 0;
					}
			}
		}
	}
}

void *get_screen_ram(ulong physical)
{
	area_id aid;
	void *addr;

	aid = _kmap_physical_memory_("text framebuffer", (void *) physical, 
			4096, B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA, &addr);
	return addr;
}

void vputs(struct virtscreen *vscr, char *s)
{
    while(*s) {
        char_to_virtscreen(vscr, *s);
        if(*s == '\n') char_to_virtscreen(vscr, '\r');
        s++;
    }
}  

int main(int argc, char *argv[])
{
	struct virtscreen virt;
	void *addr;
	int fd;
	char *shell[] = { "/boot/beos/bin/sh", NULL };
	char **execarray = shell;
	thread_id thr;
	char c;

 	addr = get_screen_ram(0xB8000);
	init_virtscreen(&virt, 25, 80);
	virt.data = addr;

#ifndef __ZRECOVER
	vputs(&virt,"\033[2JWelcome to the \033[31mB\033[34me\033[37mOS Text Console.\n\n");
#endif
	
	if(argc > 1)
	{
		argc--;
		argv++;
		execarray = argv;
	}
	fd = ExecControlled(execarray, argc, &thr);

	resume_thread(spawn_thread(keyboard_thread, "keyboard thread", B_NORMAL_PRIORITY, (void *) fd));

	while (1) {
		if (read(fd, &c, 1) == 1) {
			if(c != '\a') {
				char_to_virtscreen(&virt, c);
			}
		}
#ifdef __ZRECOVER
		 else {
			break;
		}
#endif
	}
	
#ifdef __ZRECOVER
	{
        extern void riad_detach_from_parent(int);
		int i;
	    /*
	     * HACK ALERT, this will removed when I fix riad_exit_team
	     */
        riad_detach_from_parent(0); // hackery
        for(i= 0; i< 32; i++) close(i);
        snooze(3600LL*1000000LL);   // hackery
	}
#endif

	return 0;	
}


#ifdef __ZRECOVER
# include <recovery/command_registry.h>
  REGISTER_STATIC_COMMAND(main,"/bin/console");
#endif
