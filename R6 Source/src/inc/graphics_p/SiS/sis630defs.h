#ifndef SIS630DEFS_H
#define SIS630DEFS_H

#ifndef SIS_VENDORID
#define SIS_VENDORID 0x1039
#endif

#define SIS630_DEVICEID 0x6300

#define VIDEO_INDEX 0x03

#define ioreg_inb(a)      *((vuchar *)(ci->ci_IORegBase+(a)))
#define ioreg_outb(a,b)   *((vuchar *)(ci->ci_IORegBase+(a)))=(b)
#define ioreg_inw(a)      *((vushort*)(ci->ci_IORegBase+(a)))
#define ioreg_outw(a,b)   *((vushort*)(ci->ci_IORegBase+(a)))=(b)
#define ioreg_inl(a)      *((vulong *)(ci->ci_IORegBase+(a)))
#define ioreg_outl(a,b)   *((vulong *)(ci->ci_IORegBase+(a)))=(b)

#endif
