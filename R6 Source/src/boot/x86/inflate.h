#ifndef _BOOT_INFLATE_H
#define _BOOT_INFLATE_H

unsigned long gunzip(const unsigned char *in, unsigned char *out);
int is_gzip(const unsigned char *in);

#endif
