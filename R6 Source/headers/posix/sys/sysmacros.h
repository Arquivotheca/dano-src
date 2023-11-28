#ifndef _SYS_SYSMACROS_H
#define _SYS_SYSMACROS_H

/* major part of a device */
#define major(x)    (int)((unsigned)((x)>>8)&0x7F)
#define bmajor(x)   (int)((unsigned)((x)>>8)&0x7F)

/* minor part of a device */
#define minor(x)    (int)((x)&0xFF)

/* make a device number */
#define makedev(x,y)    (dev_t)(((x)<<8) | (y))

#endif /* _SYS_SYSMACROS_H */
