#ifndef SMBDES_C
#define SMBDES_C

extern "C" void E_P16(unsigned char *key, unsigned char *p16);
extern "C" void E_P24(unsigned char *key, unsigned char *c8, unsigned char *p24);

#endif // SMBDES_C